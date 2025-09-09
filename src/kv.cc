#include "kv.hh"
#include "utils.hh"
#include <cassert>

static bool entry_eq(HNode *lhs, HNode *rhs) {
    struct Entry *le = container_of(lhs, struct Entry, node);
    struct Entry *re = container_of(rhs, struct Entry, node);
    return le->key == re->key;
}

static uint64_t hash(const uint8_t *data, size_t len) { // review
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; ++i) {
        h = (h + data[i]) * 0x01000193;
    }
    return h;
}

void do_get(HMap *db, std::vector<std::string> &cmd, Response &resp) {
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = hash(reinterpret_cast<uint8_t *>(key.key.data()), key.key.size());
    HNode *node = hm_lookup(db, &key.node, &entry_eq);
    if (!node) { resp.status = RES_KEY_NOT_FOUND; return; }
    
    const std::string &val = container_of(node, Entry, node)->value;
    assert(val.size() <= MSG_SIZE_LIMIT); 
    resp.data.assign(val.begin(), val.end());
}

// for set and del, take out response ?

void do_set(HMap *db, std::vector<std::string> &cmd, Response &) {
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = hash(reinterpret_cast<uint8_t*>(key.key.data()), key.key.size());
    HNode *node = hm_lookup(db, &key.node, &entry_eq);
    if (node)
        container_of(node, Entry, node)->value.swap(cmd[2]);
    else {
        Entry *ent = new Entry();
        ent->key.swap(key.key);
        ent->node.hcode = key.node.hcode;
        ent->value.swap(cmd[2]);
        hm_insert(db, &ent->node);
    }
}
void do_del(HMap *db, std::vector<std::string> &cmd, Response &resp) {
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = hash(reinterpret_cast<uint8_t*>(key.key.data()), key.key.size());

    HNode *node = hm_delete(db, &key.node, &entry_eq);
    if (node) {
        delete container_of(node, Entry, node);
    }
}