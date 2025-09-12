#include "protocol.hh"
#include <cstring>
#include <limits>

// test

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
    *header = out.data_end - out.data_begin;
    buf_append(out, 0, sizeof(uint32_t));
}

static size_t response_size(Buffer &out, size_t header) {
    size_t size = out.data_end - out.data_begin;
    return size - header + 4;
}

void response_end(Buffer &out, size_t header) {
    size_t msg_size = response_size(out, header);
    if (msg_size > MSG_SIZE_LIMIT) {
        // out.resize(header + 4)
        out_err(out, ERR_TOO_BIG, "response is too big");
    }
}

/* Serialization */
void out_nil(Buffer &out) {
    uint8_t tag = TAG_NIL;
    buf_append(out, &tag, sizeof(tag));
}

void out_str(Buffer &out, const char *s, size_t size) {
    uint8_t tag = TAG_STR;
    buf_append(out, &tag, sizeof(tag));

    if (size > std::numeric_limits<uint32_t>::max()) return ; // error handling todo
    uint32_t len = static_cast<uint32_t>(size);
    uint32_t be = htobe32(len);

    buf_append(out, reinterpret_cast<uint8_t *>(&be), sizeof(be));
    buf_append(out, reinterpret_cast<const uint8_t *>(s), size);
}

void out_int(Buffer &out, int64_t val) {
    uint8_t tag = TAG_INT;
    buf_append(out, &tag, sizeof(tag));

    uint64_t u = static_cast<uint64_t>(val);
    int64_t be = htobe64(val);

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

    uint32_t code_be = htobe32(code);
    buf_append(out, reinterpret_cast<uint8_t *>(&code_be), sizeof(code_be));

    size_t size = msg.size();
    if (size > std::numeric_limits<uint32_t>::max()) return ; // error handling todo
    uint32_t len = static_cast<uint32_t>(size);
    uint32_t size_be = htobe32(len);

    buf_append(out, reinterpret_cast<const uint8_t *>(&msg), size);
}