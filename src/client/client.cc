#include "client.hh"
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <stdint.h>
#include <assert.h>

/* Internal helpers */

static std::vector<std::string> split(std::string s) {
    std::vector<std::string> cmd;
    std::stringstream ss(s);
    std::string word;
    while (ss >> word) {
        cmd.push_back(word);
    }
    return cmd;
}

static int32_t handle_write(int fd, const char *buf, size_t n) {
    
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                printf("Error: write() failed, non-fatal\n");
                return 0;
            }
            printf("Error: write() failed, closing connection\n");
            return -1;
        }
        n -= static_cast<size_t>(rv);
        buf += rv;
    }
    return 0;
}

static int32_t handle_read(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) return -1;
        n -= rv;
        buf += rv;
    }
    return 0;
}

static int get_socket(uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        fprintf(stderr, "Error: Could not create socket\n");
        return -1;
    }
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);

    int rv = connect(fd, reinterpret_cast<const struct sockaddr *>(&addr), sizeof(addr));
    if (rv) {
        close(fd);
        fprintf(stderr, "Error: Could not connect to server\n");
        return -1;
    }
    return fd;
}

static int32_t send_req(int fd, std::vector<std::string> &cmd) {
    uint32_t len = sizeof(uint32_t);
    for (std::string &s : cmd) len += sizeof(uint32_t) + s.size();

    if (len > msg_size_limit) {
        printf("Error: Message size is bigger than limit (%d)\n", msg_size_limit);
        return 0;
    }

    char wbuf[4 + msg_size_limit];
    memcpy(wbuf, &len, sizeof(uint32_t));
    uint32_t n = static_cast<uint32_t>(cmd.size()); 
    memcpy(&wbuf[4], &n, sizeof(uint32_t));

    size_t cur = 8;
    for (std::string &s : cmd) {
        uint32_t p = static_cast<uint32_t>(s.size());
        memcpy(&wbuf[cur], &p, sizeof(uint32_t));
        cur += 4;
        memcpy(&wbuf[cur], s.data(), s.size());
        cur += s.size();
    }

    return handle_write(fd, wbuf, len + 4); 
}

static uint32_t read_res(int fd) {
    char rbuf[4 + msg_size_limit];
    errno = 0;
    int32_t err = handle_read(fd, rbuf, 4);
    if (err < 0) {
        fprintf(stderr, "Error: read() failed, closing connection\n");
        return resp_could_not_read;
    }

    uint32_t len_be = 0;
    memcpy(&len_be, rbuf, sizeof(uint32_t));
    uint32_t len = be32toh(len_be);

    if (len > msg_size_limit) {
        fprintf(stderr, "Error: Server response is too long\n");
        return resp_corrupt;
    }
    err = handle_read(fd, &rbuf[4], len);
    if (err < 0) {
        fprintf(stderr, "Error: read() failed, closing connection\n");
        return resp_could_not_read;
    }

    int rv = print_response(reinterpret_cast<uint8_t *>(&rbuf[4]), len);
    if (rv == resp_incomplete) {
        printf("Error: Server response is incomplete\n");
    }
    return rv;
}

/* API */

int run_client(uint16_t port) {
    int fd = get_socket(port);
    if (fd < 0) return -1;

    for (;;) {
        printf("> ");
        std::string line;
        if (!std::getline(std::cin, line)) {
            fprintf(stderr, "Error: getline() failed\n");
            return -1;
        }
        std::vector<std::string> cmd = split(line);
        if (cmd.empty()) continue;
        if (send_req(fd, cmd)) {
            close(fd);
            return -1;
        }
        int rv = read_res(fd);
        if (rv == resp_corrupt) return -1;
        if (rv == resp_close) {
            close(fd);
            return 0;
        }; 
    
    }
}