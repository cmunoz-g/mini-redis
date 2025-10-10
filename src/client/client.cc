#include "client.hh"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <cstring>
#include <cstdint>
#include <cassert>

static constexpr int MSG_SIZE_LIMIT = 4096;

enum {
    TAG_NIL = 0,
    TAG_ERR = 1,   
    TAG_STR = 2,   
    TAG_INT = 3,  
    TAG_DBL = 4,
    TAG_ARR = 5,
    TAG_CLOSE = 6,
    TAG_OK = 7
};

int get_socket(uint16_t port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        fprintf(stderr, "Error: Could not create socket\n");
        return -1;
    }
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);

    int rv = ::connect(fd, reinterpret_cast<const struct sockaddr *>(&addr), sizeof(addr));
    if (rv) {
        ::close(fd);
        fprintf(stderr, "Error: Could not connect to server\n");
        return -1;
    }
    return fd;
}

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
        ssize_t rv = ::write(fd, buf, n);
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
        ssize_t rv = ::read(fd, buf, n);
        if (rv <= 0) return -1;
        n -= rv;
        buf += rv;
    }
    return 0;
}

static uint32_t print_response(const uint8_t *data, size_t size);

static int32_t print_arr(const uint8_t *data, size_t size) {
    (void)size;
    uint32_t words_be = 0;
    memcpy(&words_be, &data[1], sizeof(uint32_t));

    uint32_t words = be32toh(words_be);
    size_t arr_bytes = 1 + sizeof(uint32_t);

    if (words == 0) printf("(empty array)\n");

    for (uint32_t i = 0; i < words; ++i) {
        printf("%d) ", i+1);
        uint8_t tag = data[arr_bytes];
        arr_bytes++;
        if (tag == TAG_STR) {
            uint32_t bytes_be = 0;
            memcpy(&bytes_be, &data[arr_bytes], sizeof(uint32_t));
            uint32_t bytes = be32toh(bytes_be);
            arr_bytes += sizeof(uint32_t);
            printf("\"%.*s\"\n", bytes, &data[arr_bytes]);
            arr_bytes += static_cast<size_t>(bytes);
        }
        else {
            double val = 0;
            memcpy(&val, &data[arr_bytes], sizeof(double));
            printf("\"%g\"\n", val);
            arr_bytes += sizeof(double);
        }
    }
    return 0;
}

enum {
    RESP_OK = 0,
    RESP_INCOMPLETE = 1,
    RESP_CLOSE = 2,
    RESP_CORRUPT = 3,
    RESP_COULD_NOT_READ =4
};

static uint32_t print_response(const uint8_t *data, size_t size) {
    if (size < 1) return RESP_INCOMPLETE;

    switch (data[0]) {
        case TAG_NIL: printf("(nil)\n"); return RESP_OK;
        case TAG_ERR: {
            if (size < 1 + 8) return RESP_INCOMPLETE;
            uint32_t code_be = 0;
            uint32_t len_be = 0;
            memcpy(&code_be, &data[1], sizeof(uint32_t));
            memcpy(&len_be, &data[1 + sizeof(uint32_t)], sizeof(uint32_t));
            
            uint32_t code = be32toh(code_be);
            uint32_t len = be32toh(len_be);

            if (static_cast<uint32_t>(size) < 1 + 8 + len) return RESP_INCOMPLETE;
            printf("(err) %d : %.*s\n", code, len, &data[1 + 8]);
            return RESP_OK;
        }
        case TAG_STR: {
            if (size < 1 + 4) return RESP_INCOMPLETE;
            uint32_t len_be = 0;
            memcpy(&len_be, &data[1], sizeof(uint32_t));
            uint32_t len = be32toh(len_be);
            if (size < 1 + 4 + len) return RESP_INCOMPLETE;
            printf("\"%.*s\"\n", len, &data[1 + sizeof(uint32_t)]);
            return RESP_OK;
        }
        case TAG_OK: {
            if (size < 1 + 4) return RESP_INCOMPLETE;
            uint32_t len = 2;
            if (size < 1 + 4 + len) return RESP_INCOMPLETE;
            printf("%.*s\n", len, &data[1 + sizeof(uint32_t)]);
            return RESP_OK;
        }
        case TAG_INT: {
            if (size < 1 + 8) return RESP_INCOMPLETE;
            int64_t val_be = 0;
            memcpy(&val_be, &data[1], sizeof(int64_t));
            int64_t val = be64toh(val_be);
            printf("(int) %ld\n", val);
            return RESP_OK;
        }
        case TAG_DBL: {
            if (size < 1 + 8) return RESP_INCOMPLETE;
            double val = 0;
            memcpy(&val, &data[1], sizeof(double));
            printf("(dbl) %g\n", val);
            return RESP_OK;
        }
        case TAG_ARR: {
            if (size < 1 + 4) return RESP_INCOMPLETE;
            return print_arr(data, size);
        }
        case TAG_CLOSE: {
            uint32_t len_be = 0;
            memcpy(&len_be, &data[1], sizeof(uint32_t));
            uint32_t len = be32toh(len_be);
            printf("Notification: %.*s", len, &data[1 + sizeof(uint32_t)]);
            return RESP_CLOSE;
        }
        default: {
            return RESP_INCOMPLETE;
        }
    }
}

static uint32_t read_res(int fd) {
    char rbuf[4 + MSG_SIZE_LIMIT];
    errno = 0;
    int32_t err = handle_read(fd, rbuf, 4);
    if (err < 0) {
        fprintf(stderr, "Error: read() failed, closing connection\n");
        return RESP_COULD_NOT_READ;
    }

    uint32_t len_be = 0;
    memcpy(&len_be, rbuf, sizeof(uint32_t));
    uint32_t len = be32toh(len_be);

    if (len > MSG_SIZE_LIMIT) {
        fprintf(stderr, "Error: Server response is too long\n");
        return RESP_CORRUPT;
    }
    err = handle_read(fd, &rbuf[4], len);
    if (err < 0) {
        fprintf(stderr, "Error: read() failed, closing connection\n");
        return RESP_COULD_NOT_READ;
    }

    int rv = print_response(reinterpret_cast<uint8_t *>(&rbuf[4]), len);
    if (rv == RESP_INCOMPLETE) {
        printf("Error: Server response is incomplete\n");
    }
    return rv;
}

static int32_t send_req(int fd, std::vector<std::string> &cmd) {
    uint32_t len = sizeof(uint32_t);
    for (std::string &s : cmd) len += sizeof(uint32_t) + s.size();

    if (len > MSG_SIZE_LIMIT) {
        printf("Error: Message size is bigger than limit (%d)\n", MSG_SIZE_LIMIT);
        return 0;
    }

    char wbuf[4 + MSG_SIZE_LIMIT];
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
            ::close(fd);
            return -1;
        }
        int rv = read_res(fd);
        if (rv == RESP_CORRUPT) return -1;
        if (rv == RESP_CLOSE) {
            ::close(fd);
            return 0;
        }; 
    
    }
}