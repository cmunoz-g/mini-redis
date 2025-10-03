#include "buffer.hh"
#include <cstring>
#include <cassert>
#include <cstdio> // quitar

/* Utils */

size_t buf_size(const Buffer &b) {
    return static_cast<size_t>(b.data_end - b.data_begin);
}

uint8_t *buf_at(Buffer &b, size_t off) {
    size_t used = buf_size(b);
    if (off > used) return nullptr;
    return b.data_begin + off;
}

void buf_truncate(Buffer &b, size_t new_size) {
    size_t used = buf_size(b);
    if (new_size > used) return;
    b.data_end = b.data_begin + new_size;
}

/* Append & consume*/

#include <cstdlib> // borrar

void buf_append(Buffer &buf, const uint8_t *data, size_t len) {
    static int i = 0;
    if (len == 0) return;

    printf("%d time\n", i);
    if (i > 10) {
        printf("exiting buf_append");
        exit(1);
    }

    assert(buf.buffer_begin <= buf.data_begin);
    assert(buf.data_begin <= buf.data_end);
    assert(buf.data_end <= buf.buffer_end);

    size_t used = buf_size(buf);
    size_t free = static_cast<size_t>(buf.buffer_end - buf.data_end);

    if (free < len) {
        std::memmove(buf.buffer_begin, buf.data_begin, used);
        buf.data_begin = buf.buffer_begin;
        buf.data_end = buf.data_begin + used;
        free = static_cast<size_t>(buf.buffer_end - buf.data_end);
    }

    if (free < len) void(0); // meaning, still no room left to append. buf_append()
    // could return a bool and the situation be managed in the caller ft.
    std::memcpy(buf.data_end, data, len);
    buf.data_end += len;
    i++;
}

void buf_consume(Buffer &buf, size_t n) {
    printf("buf consume is called\n");
    size_t used = buf_size(buf);
    assert(n <= used);

    buf.data_begin += n;
    
    if (buf.data_begin == buf.data_end) {
        buf.data_begin = buf.buffer_begin;
        buf.data_end = buf.buffer_begin;
    }
}