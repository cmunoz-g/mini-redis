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
    TAG_CLOSE = 6
};

int get_socket(uint16_t port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);

    int rv = ::connect(fd, reinterpret_cast<const struct sockaddr *>(&addr), sizeof(addr));
    if (rv) void(0); // error mgm, i think should be fatal, close(fd);

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
        if (rv <= 0) return -1;
        n -= static_cast<size_t>(rv);
        buf += rv;
    }
    return 0;
}

static int32_t handle_read(int fd, char *buf, size_t n) {
    // static int i = 0;
    //size_t len = n;
    // if (i) fprintf(stderr, "n : %zu\n", n);
    while (n > 0) {
        ssize_t rv = ::read(fd, buf, n);
        //if (i) fprintf(stderr, "bytes read : %zu\n", rv);
        if (rv <= 0) return -1;
        n -= rv;
        buf += rv;
    }

    // if (!i) {
    //     fprintf(stderr, "bye\n");
    //     exit(0);
    // }
    // i++;
    return 0;
}

static int32_t print_response(const uint8_t *data, size_t size);

static int32_t print_arr(const uint8_t *data, size_t size) {
    uint32_t len_be = 0;
    memcpy(&len_be, &data[1], sizeof(uint32_t));

    uint32_t len = be32toh(len_be);

    printf("(arr) len=%u\n", len); // think : do i want it printign like so ?
    size_t arr_bytes = 1 + sizeof(uint32_t);
    for (uint32_t i = 0; i < len; ++i) {
        int32_t rv = print_response(&data[arr_bytes], size - arr_bytes);
        if (rv < 0) return rv;
        arr_bytes += static_cast<size_t>(rv);
    }
    printf("(arr) end\n");
    return 0;
}

enum {
    RESP_OK = 0,
    RESP_INCOMPLETE = 1,
    RESP_CLOSE = 2
};

// Standardize either uint32_t or int32_t for sizes

static int32_t print_response(const uint8_t *data, size_t size) {
    if (size < 1) return -1; // print msg : bad response

    switch (data[0]) {
        case TAG_NIL: printf("(nil)\n"); return RESP_OK;
        case TAG_ERR: {
            if (size < 1 + 8) return RESP_INCOMPLETE; //msg bad resp
            int32_t code_be = 0;
            uint32_t len_be = 0;
            memcpy(&code_be, &data[1], sizeof(int32_t));
            memcpy(&len_be, &data[1 + sizeof(int32_t)], sizeof(uint32_t));
            
            int32_t code = be32toh(code_be);
            int32_t len = be32toh(len_be);

            if (static_cast<int>(size) < 1 + 8 + len) return RESP_INCOMPLETE; // bad resp
            printf("(err) %d : %.*s\n", code, len, &data[1 + 8]);
            return RESP_OK; // substitute references to 8 bytes for explicit sizeofs ?
                                // also: maybe define some constexpr values  ??? 
        }
        case TAG_STR: {
            if (size < 1 + 4) return RESP_INCOMPLETE; // msg bad resp
            uint32_t len_be = 0;
            memcpy(&len_be, &data[1], sizeof(int32_t));

            uint32_t len = be32toh(len_be);

            if (size < 1 + 4 + len) return RESP_INCOMPLETE; // msg bad resp
            printf("(str) %.*s\n", len, &data[1 + sizeof(uint32_t)]);
            return RESP_OK;
        }
        case TAG_INT: {
            if (size < 1 + 8) return RESP_INCOMPLETE; // msg bad resp
            int64_t val_be = 0;
            memcpy(&val_be, &data[1], sizeof(int64_t));
            int64_t val = be64toh(val_be);
            printf("(int) %ld\n", val);
            return RESP_OK;
        }
        case TAG_DBL: {
            if (size < 1 + 8) return RESP_INCOMPLETE; //msgbr
            double val = 0;
            memcpy(&val, &data[1], sizeof(double));
            printf("(dbl) %g\n", val); // will this work since double wasnt covnerted ?
            return RESP_OK;
        }
        case TAG_ARR: {
            if (size < 1 + 4) return RESP_INCOMPLETE; //msgbr
            return print_arr(data, size);
        }
        case TAG_CLOSE: { // REDO FORMAT
            uint32_t len_be = 0;
            memcpy(&len_be, &data[1], sizeof(int32_t));
            uint32_t len = be32toh(len_be);
            printf("(close) %.*s\n", len, &data[1 + sizeof(uint32_t)]);
            return RESP_CLOSE;
        }
        default: {
            //msg bad resposne
            return RESP_INCOMPLETE;
        }
    }
}

static int32_t read_res(int fd) {
    char rbuf[4 + MSG_SIZE_LIMIT];
    errno = 0;
    int32_t err = handle_read(fd, rbuf, 4);
    if (err) {
        if (errno == 0) void(0); //EOF , print msg
        else void(0); // read() error, print msg
        return err;
    }

    uint32_t len_be = 0;
    memcpy(&len_be, rbuf, sizeof(uint32_t));
    uint32_t len = be32toh(len_be);

    if (len > MSG_SIZE_LIMIT) return -1; // print err msg : msg too long . should stop client ?

    //fprintf(stderr, "len : %d\n", len);
    //fprintf(stderr, "Checkpoint 1\n");
    err = handle_read(fd, &rbuf[4], len);
    if (err) {
        // read() error, msg
        return err;
    }

    int rv = print_response(reinterpret_cast<uint8_t *>(&rbuf[4]), len);
    if (rv == RESP_INCOMPLETE) {
        printf("resp incomplete\n"); // response incomplete, figure out format later.
    }
    return rv;
}

static int32_t send_req(int fd, std::vector<std::string> &cmd) {
    uint32_t len = sizeof(uint32_t);
    for (std::string &s : cmd) len += sizeof(uint32_t) + s.size();

    if (len > MSG_SIZE_LIMIT) return -1;

    char wbuf[4 + MSG_SIZE_LIMIT];
    memcpy(wbuf, &len, sizeof(uint32_t)); // [size of total payload]
    uint32_t n = static_cast<uint32_t>(cmd.size()); 
    memcpy(&wbuf[4], &n, sizeof(uint32_t)); // [n of cmds]

    size_t cur = 8;
    for (std::string &s : cmd) {
        uint32_t p = static_cast<uint32_t>(s.size());
        memcpy(&wbuf[cur], &p, sizeof(uint32_t)); // [size of each cmd/arg]
        cur += 4;
        memcpy(&wbuf[cur], s.data(), s.size()); // [cmd/arg content]
        cur += s.size();
    }

    return handle_write(fd, wbuf, len + 4); // len + 4 len represents [size of total payload] but doesnt include the 4 bytes of itself
}

// Situation : Server side code regarding closing seems to work fine
// The issue is that the client is not printing the exit msg and is not exiting the program

int run_client(uint16_t port) {
    int fd = get_socket(port);
    if (fd < 0) return -1;

    for (;;) {
        printf("> ");
        std::string line;
        if (!std::getline(std::cin, line)) void(0); // error . what to do ?

        std::vector<std::string> cmd = split(line);
        if (cmd.empty()) continue;
        if (send_req(fd, cmd)) void(0); // errmgmt problem: could return non-zero if msg is larger than max size(non fatal) or if write fails in handle_write(fatal)
        if (read_res(fd) == RESP_CLOSE) {
            printf("got here\n");
            ::close(fd);
            return 0;
        }; 
    
    }
}