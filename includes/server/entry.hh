#pragma once
#include "server.hh"
#include "sortedset.hh"
#include "hashtable.hh"
#include <string>

constexpr size_t heap_invalid = SIZE_MAX;

enum entry_type {
    t_init = 0,
    t_str = 1,
    t_zset = 2,
};

struct Entry {
    struct HNode node;
    std::string key;
    uint32_t type = 0;
    std::string str;
    ZSet zset;
    size_t heap_idx = heap_invalid;
};

struct LookupKey {
    struct HNode node;
    std::string key;
};

/* API */
bool entry_eq(HNode *lhs, HNode *rhs);
Entry *entry_new(uint32_t type);
void entry_del(g_data &data, Entry *ent);
void entry_set_ttl(std::vector<HeapItem> &heap, Entry *ent, int64_t ttl_ms);