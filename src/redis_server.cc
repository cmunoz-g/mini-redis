#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

// run_server has the loop, working with the fd of the server (fetched from socket_listen())

// socket_listen  creates the socket, configs, bind() and listen()

// handle_client() is the do_something() function in the book

int run_server(const char* host, uint16_t port) {
    int fd = socket_listen(host, port);
    if (fd < 0) return 1; // think through error mgmt

    for (;;) {
        struct sockaddr_in addr = {};
        socklen_t len = sizeof(addr);
        int connfd = ::accept(fd, reinterpret_cast<sockaddr*>(&addr), &len);
        if (connfd < 0) continue;
        handle_client(connfd);
        ::close(connfd);
    }

    return (0);
}