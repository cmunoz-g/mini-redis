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

struct Buffer {
    uint8_t *buffer_begin;
    uint8_t *buffer_end;
    uint8_t *data_begin;
    uint8_t *data_end;
};

struct Conn {
    int fd{-1};
    bool want_read{true}, want_write{false}, want_close{false};
    Buffer in, out;
};
