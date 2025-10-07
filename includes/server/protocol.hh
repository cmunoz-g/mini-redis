#pragma once
#include "buffer.hh"
#include <cstdint>
#include <vector>
#include <string>

constexpr size_t MSG_SIZE_LIMIT = 4096; 
constexpr size_t MAX_ARGS = 32u << 20;

enum {
    RES_OK = 0,
    RES_ERR = 1,
    RES_KEY_NOT_FOUND = 2
};

enum {
    TAG_NIL = 0,
    TAG_ERR = 1,
    TAG_STR = 2,
    TAG_INT = 3,
    TAG_DBL = 4,
    TAG_ARR = 5,
    TAG_CLOSE = 6
};

enum {
    ERR_UNKNOWN = 1,
    ERR_TOO_BIG = 2,
    ERR_BAD_TYPE = 3,
    ERR_BAD_ARG = 4,
    ERR_EMPTY = 5,
};

int32_t parse_request(const uint8_t *data, const size_t size, std::vector<std::string> &out);
void response_begin(Buffer &out, size_t *header);
void response_end(Buffer &out, size_t header);

/* Serialization */
void out_nil(Buffer &out);
void out_str(Buffer &out, const char *s, size_t size);
void out_int(Buffer &out, int64_t val);
void out_arr(Buffer &out, uint32_t n);
void out_err(Buffer &out, uint32_t code, const std::string &msg);
void out_dbl(Buffer &out, double val);
size_t out_begin_arr(Buffer &out);
void out_end_arr(Buffer &out, size_t ctx, uint32_t n);
void out_close(Buffer &out, const char *s, size_t size);