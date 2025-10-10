#include "client.hh"

/* Internal helpers */
static int32_t print_arr(const uint8_t *data) {
    uint32_t words_be = 0;
    memcpy(&words_be, &data[1], sizeof(uint32_t));

    uint32_t words = be32toh(words_be);
    size_t arr_bytes = 1 + sizeof(uint32_t);

    if (words == 0) printf("(empty array)\n");

    for (uint32_t i = 0; i < words; ++i) {
        printf("%d) ", i+1);
        uint8_t tag = data[arr_bytes];
        arr_bytes++;
        if (tag == tag_str) {
            uint32_t bytes_be = 0;
            memcpy(&bytes_be, &data[arr_bytes], sizeof(uint32_t));
            uint32_t bytes = be32toh(bytes_be);
            arr_bytes += sizeof(uint32_t);
            printf("\"%.*s\"\n", bytes, &data[arr_bytes]);
            arr_bytes += static_cast<size_t>(bytes);
        }
        else {
            double val = 0;
            memcpy(&val, &data[arr_bytes], sizeof(double));
            printf("\"%g\"\n", val);
            arr_bytes += sizeof(double);
        }
    }
    return 0;
}

/* API */

uint32_t print_response(const uint8_t *data, size_t size) {
    if (size < 1) return resp_incomplete;

    switch (data[0]) {
        case tag_nil: printf("(nil)\n"); return resp_ok;
        case tag_err: {
            if (size < 1 + 8) return resp_incomplete;
            uint32_t code_be = 0;
            uint32_t len_be = 0;
            memcpy(&code_be, &data[1], sizeof(uint32_t));
            memcpy(&len_be, &data[1 + sizeof(uint32_t)], sizeof(uint32_t));
            
            uint32_t code = be32toh(code_be);
            uint32_t len = be32toh(len_be);

            if (static_cast<uint32_t>(size) < 1 + 8 + len) return resp_incomplete;
            printf("(err) %d : %.*s\n", code, len, &data[1 + 8]);
            return resp_ok;
        }
        case tag_str: {
            if (size < 1 + 4) return resp_incomplete;
            uint32_t len_be = 0;
            memcpy(&len_be, &data[1], sizeof(uint32_t));
            uint32_t len = be32toh(len_be);
            if (size < 1 + 4 + len) return resp_incomplete;
            printf("\"%.*s\"\n", len, &data[1 + sizeof(uint32_t)]);
            return resp_ok;
        }
        case tag_ok: {
            if (size < 1 + 4) return resp_incomplete;
            uint32_t len = 2;
            if (size < 1 + 4 + len) return resp_incomplete;
            printf("%.*s\n", len, &data[1 + sizeof(uint32_t)]);
            return resp_ok;
        }
        case tag_int: {
            if (size < 1 + 8) return resp_incomplete;
            int64_t val_be = 0;
            memcpy(&val_be, &data[1], sizeof(int64_t));
            int64_t val = be64toh(val_be);
            printf("(int) %ld\n", val);
            return resp_ok;
        }
        case tag_dbl: {
            if (size < 1 + 8) return resp_incomplete;
            double val = 0;
            memcpy(&val, &data[1], sizeof(double));
            printf("(dbl) %g\n", val);
            return resp_ok;
        }
        case tag_arr: {
            if (size < 1 + 4) return resp_incomplete;
            return print_arr(data);
        }
        case tag_close: {
            uint32_t len_be = 0;
            memcpy(&len_be, &data[1], sizeof(uint32_t));
            uint32_t len = be32toh(len_be);
            printf("Notification: %.*s", len, &data[1 + sizeof(uint32_t)]);
            return resp_close;
        }
        default: {
            return resp_incomplete;
        }
    }
}