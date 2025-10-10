#pragma once
#include <stdint.h>
#include <stdio.h>
#include <netinet/in.h>
#include <cstring>
#include <cerrno>

constexpr int msg_size_limit = 4096;

enum tags {
    tag_nil = 0,
    tag_err = 1,   
    tag_str = 2,   
    tag_int = 3,  
    tag_dbl = 4,
    tag_arr = 5,
    tag_close = 6,
    tag_ok = 7
};

enum responses {
    resp_ok = 0,
    resp_incomplete = 1,
    resp_close = 2,
    resp_corrupt = 3,
    resp_could_not_read =4
};

uint32_t print_response(const uint8_t *data, size_t size);
int run_client(uint16_t port);
