#pragma once
#include <cstdint>
#include <cstddef>

struct Buffer {
    uint8_t *buffer_begin, *buffer_end, *data_begin, *data_end;
};

// Inline makes sense?
inline size_t buf_size(const Buffer &b);
inline uint8_t *buf_at(Buffer &b, size_t off);
inline void buf_truncate(Buffer &b, size_t new_size);

void buf_append(Buffer &buf, const uint8_t *data, size_t len);
void buf_consume(Buffer &buf, size_t n);