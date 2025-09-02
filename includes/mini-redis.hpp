#pragma once

#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <vector>

struct Conn {
    int fd{-1};
    bool want_read{true}, want_write{false}, want_close{false};
    std::vector<uint8_t> incoming, outgoing;
};