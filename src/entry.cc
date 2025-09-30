#include "entry.hh"
#include <utils.hh>

bool entry_eq(HNode *lhs, HNode *rhs) {
    struct Entry *le = container_of(lhs, struct Entry, node);
    struct Entry *re = container_of(rhs, struct Entry, node);
    return le->key == re->key;
}

Entry *entry_new(uint32_t type) {
    Entry *ent = new Entry();
    ent->type = type;
    return ent;
}

void entry_del(Entry *ent) {
    if (ent->type == T_ZSET) zset_destroy(&ent->zset);
    delete ent;
}