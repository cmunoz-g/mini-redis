#pragma once
#include <stdint.h>
#include <stddef.h>

struct Buffer {
    uint8_t *buffer_begin, *buffer_end, *data_begin, *data_end;
};

/* Utils*/
size_t buf_size(const Buffer &b);
uint8_t *buf_at(Buffer &b, size_t off);
void buf_truncate(Buffer &b, size_t new_size);
void buf_reset(Buffer &b);

/* Append & consume*/
void buf_append(Buffer &buf, const uint8_t *data, size_t len);
void buf_consume(Buffer &buf, size_t n);