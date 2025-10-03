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

static constexpr int MSG_SIZE_LIMIT = 4096;

int get_socket(char *port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234); // change to stroi(port)
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

int32_t send_req(int fd, std::vector<std::string> &cmd) {
    uint32_t len = sizeof(uint32_t);
    for (auto &s : cmd) len += sizeof(uint32_t) + s.size();

    if (len > MSG_SIZE_LIMIT) return -1;

    char wbuf[4 + MSG_SIZE_LIMIT];
    memcpy(wbuf, &len, sizeof(uint32_t)); // [size of total payload]
    uint32_t n = static_cast<uint32_t>(cmd.size()); 
    memcpy(&wbuf[4], &len, sizeof(uint32_t)); // [n of cmds]

    size_t cur = 8;
    for (auto &s : cmd) {
        uint32_t p = static_cast<uint32_t>(s.size());
        memcpy(&wbuf[cur], &p, sizeof(uint32_t)); // [size of each cmd/arg]
        cur += 4;
        memcpy(&wbuf[cur], s.data(), s.size()); // [cmd/arg content]
        cur += s.size();
    }
    return handle_write(fd, wbuf, len + 4); // len + 4 len represents [size of total payload] but doesnt include the 4 bytes of itself
}

int run_client(int ac, char *av[]) {
    int fd = get_socket(av[1]);
    if (fd < 0) return -1;

    int bytes_read = 0;
    for (;;) {
        printf("> ");
        std::string line;
        if (!std::getline(std::cin, line)) {
            void(0); // error . what to do ?
        }
        std::vector<std::string> cmd = split(line); // test empty input
        if (send_req(fd, cmd)) void(0); // notify and continue, trying to send over msg limit is fine
        if (cmd[0] == "exit" || cmd[0] == "quit") {
            ::close(fd);
            return 0;
        }
        if (read_res(fd)) void(0); // same thing
    }
}