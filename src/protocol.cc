#include "protocol.hh"
#include <cstring>

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

void do_response(const Response &resp, Buffer &out) {
    uint32_t resp_len = 4 + static_cast<uint32_t>(resp.data.size());
    buf_append(out, reinterpret_cast<const uint8_t *>(&resp_len), sizeof(resp_len));
    buf_append(out, reinterpret_cast<const uint8_t *>(&resp.status), sizeof(uint32_t));
    buf_append(out, resp.data.data(), resp.data.size());
}