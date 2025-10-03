#include "socket.hh"
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>

int set_non_blocking(int fd) {
    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return (::fcntl(fd, F_SETFL, flags | O_NONBLOCK));
}

int socket_listen(const char *host, uint16_t port) {
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