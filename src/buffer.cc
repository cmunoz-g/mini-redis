#include "buffer.hh"
#include <cstring>
#include <cassert>

void buf_append(Buffer &buf, const uint8_t *data, size_t len) {
    if (len == 0) return;
    assert(buf.buffer_begin <= buf.data_begin
        && buf.data_begin <= buf.data_end && buf.data_end <= buf.buffer_end);

    size_t used = static_cast<size_t>(buf.data_end - buf.data_begin);
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
}

void buf_consume(Buffer &buf, size_t n) {
    size_t used = static_cast<size_t>(buf.data_end - buf.data_begin);
    assert(n <= used);

    buf.data_begin += n;
    
    if (buf.data_begin == buf.data_end) {
        buf.data_begin = buf.buffer_begin;
        buf.data_end = buf.buffer_begin;
    }
}