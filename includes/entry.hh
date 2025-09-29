#pragma once
#include <string>
#include "sortedset.hh"
#include "hashtable.hh"

enum {
    T_INIT = 0,
    T_STR = 1,
    T_ZSET = 2,
};

struct Entry {
    struct HNode node;
    std::string key;
    uint32_t type = 0;
    std::string str;
    ZSet zset;
};

struct LookupKey {
    struct HNode node;
    std::string key;
};

bool entry_eq(HNode *lhs, HNode *rhs);
Entry *entry_new(uint32_t type);
void entry_del(Entry *ent);