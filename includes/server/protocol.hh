#pragma once
#include "buffer.hh"
#include <stdint.h>
#include <vector>
#include <string>

constexpr size_t msg_size_limit = 4096; 
constexpr size_t max_args = 32u << 20;

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

enum errs {
    err_unknown = 1,
    err_too_big = 2,
    err_bad_type = 3,
    err_bad_arg = 4,
    err_empty = 5,
};

/* API */
// Request handling
int32_t parse_request(const uint8_t *data, const size_t size, std::vector<std::string> &out);
// Response handling
void response_begin(Buffer &out, size_t *header);
void response_end(Buffer &out, size_t header);
// Serialization
void out_nil(Buffer &out);
void out_str(Buffer &out, const char *s, size_t size, int tag_ok);
void out_int(Buffer &out, int64_t val);
void out_arr(Buffer &out, uint32_t n);
void out_err(Buffer &out, uint32_t code, const std::string &msg);
void out_dbl(Buffer &out, double val);
size_t out_begin_arr(Buffer &out);
void out_end_arr(Buffer &out, size_t ctx, uint32_t n);
void out_close(Buffer &out, const char *s, size_t size);