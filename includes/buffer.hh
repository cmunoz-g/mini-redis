#pragma once
#include <cstdint>
#include <cstddef>

struct Buffer {
    uint8_t *buffer_begin, *buffer_end, *data_begin, *data_end;
};

void buf_append(Buffer &buf, const uint8_t *data, size_t len);
void buf_consume(Buffer &buf, size_t n);