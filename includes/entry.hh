#pragma once
#include <string>
#include "sortedset.hh"
#include "hashtable.hh"

enum {
    T_INIT = 0,
    T_STR = 1,
    T_ZSET = 2,
};

constexpr size_t HEAP_INVALID = SIZE_MAX; // caps ?

struct Entry {
    struct HNode node;
    std::string key;
    uint32_t type = 0;
    std::string str;
    ZSet zset;
    size_t heap_idx = HEAP_INVALID;
};

struct LookupKey {
    struct HNode node;
    std::string key;
};

bool entry_eq(HNode *lhs, HNode *rhs);
Entry *entry_new(uint32_t type);
void entry_del(g_data &data, Entry *ent);
void entry_set_ttl(std::vector<HeapItem> &heap, Entry *ent, int64_t ttl_ms);