#pragma once
#include <cstdint>
#include <cstddef>

struct Buffer {
    uint8_t *buffer_begin, *buffer_end, *data_begin, *data_end;
};

void buf_append(Buffer&, const uint8_t*, size_t);
void buf_consume(Buffer&, size_t);