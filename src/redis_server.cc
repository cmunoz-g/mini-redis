#include "mini-redis.h"

int set_non_blocking(int fd) {
    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return (::fcntl(fd, F_FETFL, flags | O_NONBLOCK));
}

static Conn *handle_accept(int fd) { 
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);
    int connfd = accept(fd, reinterpret_cast<sockaddr *>(&client_addr), &addrlen);
    if (connfd < 0) return nullptr;
    if (set_non_blocking(connfd)) (void)0; // handle error
    Conn *conn = new Conn();
    conn->fd = connfd;
    conn->want_read = true;
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
            // manage fatal error 
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