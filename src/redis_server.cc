#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>

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

    for (;;) {
        struct sockaddr_in addr = {};
        socklen_t len = sizeof(addr);
        int connfd = ::accept(server_fd, reinterpret_cast<sockaddr*>(&addr), &len);
        if (connfd < 0) continue;
        handle_client(connfd);
        ::close(connfd);
    }

    return (0);
}