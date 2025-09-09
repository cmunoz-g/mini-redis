#pragma once
#include "buffer.hh"
#include <cstdint>
#include <vector>
#include <string>

inline constexpr size_t MSG_SIZE_LIMIT = 4096; 
inline constexpr size_t MAX_ARGS = 32u << 20;

enum {
    RES_OK = 0,
    RES_ERR = 1,
    RES_KEY_NOT_FOUND = 2
};

struct Response {
    uint32_t status = 0;
    std::vector<uint8_t> data;
};

int32_t parse_request(const uint8_t *data, const size_t size, std::vector<std::string> &out);
void do_response(const Response &resp, Buffer &out);