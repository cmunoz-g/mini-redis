#include "mini-redis.h"

static void handle_client(int fd) { // placeholder code
    char rbuf[64] = {};
    ssize_t n = ::read(fd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        std::fprintf(stderr, "read() error");
        return ;
    }
    std::printf("client says: %s\n", rbuf);
}

static int socket_listen(const char *host, uint16_t port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags == -1) (void)0; // TODO:handle error
    if (::fcntl(fd, F_FETFL, flags | O_NONBLOCK) == -1) (void)0; // TODO: handle error

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
            struct pollfd pfd = {conn->fd, POLLERR, 0};
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
            if (Conn *conn = handle_accept(server_fd)) {
                if (fd2conn.size() <= (size_t)conn->fd) fd2conn.resize(conn->fd + 1);
                fd2conn[conn->fd] = conn;
            }
        }

        for (size_t i = 1; i < pfds.size(); ++i) {
            uint32_t re = pfds[i].revents;
            Conn *conn = fd2conn[pfds[i].fd];
            if (re & POLLIN) handle_read(conn);
            if (re & POLLOUT) handle_write(conn);
            if (re & POLLERR || conn->want_close) handle_destroy(conn);
        }
    } 

    return (0);
}

int main(void) {
    return run_server("0.0.0.0", 1234);
}