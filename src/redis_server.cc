#include "mini-redis.h"

#define BUFFER_SIZE_KB 64
#define BYTES_IN_KB 1024
#define MSG_SIZE_LIMIT 4096

int set_non_blocking(int fd) {
    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return (::fcntl(fd, F_FETFL, flags | O_NONBLOCK));
}

static void buf_append(Buffer &buf, const uint8_t *data, size_t len) {
    if (len == 0) return;
    assert(b.buffer_begin <= b.data_begin && b.data_begin <= b.data_end && b.data_end <= b.buffer_end);

    size_t used = static_cast<size_t>(buf.data_end - buf.data_begin);
    size_t free = static_cast<size_t>(buf.buffer_end - buf.data_end);

    if (free < len) {
        std::memmove(buf.buffer_begin, buf.data_begin, used);
        buf.data_begin = buf.buffer_begin;
        buf.data_end = buf.data_begin + used;
        free = static_cast<size_t>(buf.buffer_end - buf.data_end);
    }

    if (free < len) void(0); // meaning, still no room left to append. buf_append()
    // could return a bool and the situation be managed in the caller ft.
    std::memcpy(buf.data_end, data, len);
    buf.data_end += len;
}

static void buf_consume(Buffer &buf, size_t n) {
    size_t used = static_cast<size_t>(buf.data_end - buf.data_begin);
    assert(n <= used);

    buf->data_begin += n;
    
    if (buf->data_begin == buf->data_end) {
        buf->data_begin = buf->buffer_begin;
        buf->data_end = buf->buffer_begin;
    }
}

static bool handle_request(Conn *conn) {
    size_t size = conn->in.data_end - conn->in.data_begin;
    if (size < 4) return false;

    uint32_t len = 0;
    memcpy(&len, conn->in.data_begin, 4);
    if (len > MSG_SIZE_LIMIT) {
        conn->want_close = true;
        return false;
    }

    if (4 + len > size) return false;

    const uint8_t *request = conn->in.data_begin + 4;
    buf_append(conn->out, static_cast<const uint8_t *>(&len), 4);
    buf_append(conn->out, request, len);

    buf_consume(conn->in, 4 + len);
    return true;
}

static void handle_write(Conn *conn) {
    size_t size = conn->out.data_end - conn->out.data_begin;

    assert(size > 0);
    ssize_t rv = write(conn->fd, conn->out.data_begin, size);
    if (rv < 0) {
        if (errno == EAGAIN) return;
        conn->want_close = true;
        return;
    }

    buf_consume(conn->out, reinterpret_cast<size_t>(rv));
    if (conn->out.data_begin == conn->out.data_end) {
        conn->want_read = true;
        conn->want_write = false;
    }
}

static void handle_read(Conn *conn) {
    uint8_t buf[BUFFER_SIZE_KB * BYTES_IN_KB];
    ssize_t rv = ::read(conn->fd, buf, sizeof(buf));
    if (rv <= 0) {
        conn->want_close = true;
        return;
    }
    buf_append(conn->in, buf, reinterpret_cast<size_t>(rv));
    while (handle_request(conn)) {};
    if (conn->out.data_begin != conn->out.data_end) {
        conn->want_read = false;
        conn->want_write = true;
        return handle_write(conn);
    }
}

static Conn *handle_accept(int fd) { 
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);
    int connfd = ::accept(fd, reinterpret_cast<sockaddr *>(&client_addr), &addrlen);
    if (connfd < 0) return nullptr;
    if (set_non_blocking(connfd)) (void)0; // handle error
    Conn *conn = new Conn();
    conn->fd = connfd;
    conn->want_read = true;

    constexpr size_t buffer_size = BUFFER_SIZE_KB * BYTES_IN_KB; // consider: should this be a macro?
    uint8_t *inbuf = new uint8_t[buffer_size];
    uint8_t *outbuf = new uint8_t[buffer_size];

    conn->in = {inbuf, inbuf + buffer_size, inbuf, inbuf};
    conn->out = {outbuf, outbuf + buffer_size, outbuf, outbuf};

    return conn;
}

static int socket_listen(const char *host, uint16_t port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    if (set_non_blocking(fd) < 0) (void)0; // TODO: handle error

    int val = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0)
        (void)0; // TODO: handle error
    
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(port);
    if (!host || std::strcmp(host, "0.0.0.0") == 0)
        addr.sin_addr.s_addr = ntohl(0);
    else {
        int r = ::inet_pton(AF_INET, host, &addr.sin_addr);
        if (r == 0) {
            ::close(fd);
            errno = EINVAL;
            return -1;
        }
        else if (r == -1) {
            int e = errno;
            ::close(fd);
            errno = e;
            return -1;
        }
    }

    // review bind() and listen() on failure , decide error mgmt
    if (::bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        ::close(fd);
        return -1;
    }

    if (::listen(fd, SOMAXCONN) < 0) {
        ::close(fd);
        return -1;
    }

    return fd;
}

int run_server(const char* host, uint16_t port) {
    int server_fd = socket_listen(host, port);
    if (server_fd < 0) return 1; // think through error mgmt

    std::vector<Conn *> fd2conn;
    std::vector<struct pollfd> pfds;
    
    for (;;) {
        pfds.clear();
        pfds.push_back(pollfd{server_fd, POLLIN, 0});

        for (int fd = 0; fd < (int)pfds.size(); ++fd) {
            Conn *c = fd2conn[fd];
            if (!c) continue;
            struct pollfd pfd = {c->fd, POLLERR, 0};
            if (c->want_read) pfd.events |= POLLIN;
            if (c->want_write) pfd.events |= POLLOUT;
            pfds.push_back(pfd);
        }
        
        int rv = ::poll(pfds.data(), (nfds_t)pfds.size(), -1);
        if (rv < 0) {
            if (errno == EINTR) continue;
            //else manage fatal error 
        }

        if (pfds[0].revents & POLLIN) { // revents in the server_fd aka new connections
            if (Conn *c = handle_accept(server_fd)) {
                if (fd2conn.size() <= (size_t)c->fd) fd2conn.resize(c->fd + 1);
                fd2conn[c->fd] = conn;
            }
        }

        for (size_t i = 1; i < pfds.size(); ++i) {
            uint32_t re = pfds[i].revents;
            Conn *c = fd2conn[pfds[i].fd];
            if (re & POLLIN) handle_read(c);
            if (re & POLLOUT) handle_write(c);
            if (re & POLLERR || c->want_close) {
                int fd = c->fd;
                handle_destroy(c)
                if (fd >= 0 && static_cast<size_t>(fd) < fd2conn.size())
                    fd2conn[fd] = nullptr;
            }
        }
    } 

    return (0);
}

int main(void) {
    return run_server("0.0.0.0", 1234);
}