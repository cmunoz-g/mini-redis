#include "protocol.hh"
#include <cstring>
#include <limits>
#include <cassert>

/* Helpers */
static bool read_str(const uint8_t *&cur, const uint8_t *end, const size_t n, std::string &out) {
    if (cur + n > end) return false;
    out.assign(cur, cur + n);
    cur += n;
    return true;
}

static bool read_u32(const uint8_t *&cur, const uint8_t *end, uint32_t &out) {
    if (cur + 4 > end) return false;
    memcpy(&out, cur, 4);
    cur += 4;
    return true;
}

/* Request handling */
int32_t parse_request(const uint8_t *data, const size_t size, std::vector<std::string> &out) {
    const uint8_t *end = data + size;
    uint32_t nstr = 0;

    if (!read_u32(data, end, nstr)) return -1; 
    if (nstr > MAX_ARGS) return -1;

    while (out.size() < nstr) {
        uint32_t len = 0;
        if (!read_u32(data, end, len)) return -1;
        std::string s;
        if (!read_str(data, end, len, s)) return -1;
        out.push_back(s);
    }
    if (data != end) return -1;
    return 0;
}

/* Response header */
void response_begin(Buffer &out, size_t *header) {
    *header = buf_size(out);
    uint32_t zero = 0;
    buf_append(out, reinterpret_cast<uint8_t *>(&zero), sizeof(uint32_t));
}

static size_t response_size(Buffer &out, size_t header) {
    size_t size = buf_size(out);
    return size - header - sizeof(uint32_t); 
}

void response_end(Buffer &out, size_t header) {
    size_t msg_size = response_size(out, header);
    //printf("response_end() : msg_size : %zu\n", msg_size);
    if (msg_size > MSG_SIZE_LIMIT) {
        buf_truncate(out, header + sizeof(uint32_t));
        out_err(out, ERR_TOO_BIG, "response is too big");
        msg_size = response_size(out, header);
    }

    uint32_t len = static_cast<uint32_t>(msg_size);
    //printf("response_end() : len : %u\n", len);
    uint32_t be = htobe32(len);
    std::memcpy(buf_at(out, header), &be, sizeof(uint32_t));
}

/* Serialization */
void out_nil(Buffer &out) {
    uint8_t tag = TAG_NIL;
    buf_append(out, &tag, sizeof(tag));
}

void out_str(Buffer &out, const char *s, size_t size) {
    uint8_t tag = TAG_STR;
    buf_append(out, &tag, sizeof(tag));

    if (size > std::numeric_limits<uint32_t>::max()) return out_err(out, ERR_TOO_BIG, "string len too big");
    uint32_t len = static_cast<uint32_t>(size);
    uint32_t be = htobe32(len);

    buf_append(out, reinterpret_cast<uint8_t *>(&be), sizeof(be));
    buf_append(out, reinterpret_cast<const uint8_t *>(s), size);
}

void out_int(Buffer &out, int64_t val) {
    uint8_t tag = TAG_INT;
    buf_append(out, &tag, sizeof(tag));

    uint64_t u = static_cast<uint64_t>(val);
    int64_t be = htobe64(u);

    buf_append(out, reinterpret_cast<uint8_t *>(&be), sizeof(be));
}

void out_arr(Buffer &out, uint32_t n) {
    uint8_t tag = TAG_ARR;
    buf_append(out, &tag, sizeof(tag));

    uint32_t be = htobe32(n);

    buf_append(out, reinterpret_cast<uint8_t *>(&be), sizeof(be));
}

void out_err(Buffer &out, uint32_t code, const std::string &msg) {
    uint8_t tag = TAG_ERR;
    buf_append(out, &tag, sizeof(tag));
    //printf("out_err() : size after app1 : %zu\n", buf_size(out));

    uint32_t code_be = htobe32(code);
    buf_append(out, reinterpret_cast<uint8_t *>(&code_be), sizeof(code_be));
    //printf("out_err() : size after app2 : %zu\n", buf_size(out));

    uint32_t len = static_cast<uint32_t>(msg.size());
    uint32_t size_be = htobe32(len);

    buf_append(out, reinterpret_cast<uint8_t *>(&size_be), sizeof(size_be));
    //printf("out_err() : size after app3 : %zu\n", buf_size(out));
    buf_append(out, reinterpret_cast<const uint8_t *>(&msg), msg.size());

    //printf("out_err() : size after app4 : %zu\n", buf_size(out));
}

void out_dbl(Buffer &out, double val) { // assumes IEEE-754 and BE
    uint8_t tag = TAG_DBL;
    buf_append(out, &tag, sizeof(tag));
    buf_append(out, reinterpret_cast<uint8_t *>(&val), sizeof(val));
}

size_t out_begin_arr(Buffer &out) {
    uint8_t tag = TAG_ARR;
    buf_append(out, &tag, sizeof(tag));

    uint32_t zero = 0;
    buf_append(out, reinterpret_cast<uint8_t *>(&zero), sizeof(zero));
    return buf_size(out) - sizeof(zero);
}

void out_end_arr(Buffer &out, size_t ctx, uint32_t n) {
    uint8_t tag = *buf_at(out, ctx - 1);
    assert(tag == TAG_ARR);

    uint32_t be = htobe32(n);
    memcpy(buf_at(out, ctx), &be, sizeof(be));
}