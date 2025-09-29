#include "commands.hh"
#include "entry.hh"
#include "utils.hh"
#include "hashtable.hh"
#include <cassert>
#include <cmath>

// DOUBT: should i change all string.swap() for string_view and = operator ?

void do_get(HMap &db, std::vector<std::string> &cmd, Buffer &out) {
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = hash(reinterpret_cast<uint8_t *>(key.key.data()), key.key.size());
    HNode *node = hm_lookup(&db, &key.node, &entry_eq);
    if (!node) return;
    
    const std::string &val = container_of(node, Entry, node)->value;
    assert(val.size() <= MSG_SIZE_LIMIT); 
    return out_str(out, val.data(), val.size()); // why return ?
}

void do_set(HMap &db, std::vector<std::string> &cmd, Buffer &out) {
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = hash(reinterpret_cast<uint8_t*>(key.key.data()), key.key.size());
    HNode *node = hm_lookup(&db, &key.node, &entry_eq);
    if (node)
        container_of(node, Entry, node)->value.swap(cmd[2]);
    else {
        Entry *ent = new Entry();
        ent->key.swap(key.key);
        ent->node.hcode = key.node.hcode;
        ent->value.swap(cmd[2]);
        hm_insert(&db, &ent->node);
    }
    return out_nil(out);
}
void do_del(HMap &db, std::vector<std::string> &cmd, Buffer &out) {
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = hash(reinterpret_cast<uint8_t*>(key.key.data()), key.key.size());

    HNode *node = hm_delete(&db, &key.node, &entry_eq);
    if (node) {
        delete container_of(node, Entry, node);
    }
    return out_int(out, node ? 1 : 0);
}

static bool cb_keys(HNode *node, void *arg) {
    Buffer &out = *(static_cast<Buffer *>(arg));
    const std::string &key = container_of(node, Entry, node)->key;
    out_str(out, key.data(), key.size());
}

bool do_keys(HMap &db, Buffer &out) { // should be bool ? what should i do with result
    out_arr(out, static_cast<uint32_t>(hm_size(&db)));
    return hm_foreach(&db, &cb_keys, (void *)&out);
}

static ZSet k_empty_set{}; // empty zset. should it be in a header ? if so, inline constexpr ?

static ZSet *expect_zset(HMap &db, std::string s) {
    LookupKey key;
    key.key.swap(s);
    key.node.hcode = hash(reinterpret_cast<uint8_t *>(key.key.data()), key.key.size());
    HNode *hnode = hm_lookup(&db, &key.node, &entry_eq);
    
    if (hnode) {
        Entry *ent = container_of(hnode, Entry, node);
        return ent->type == T_ZSET ? &ent->zset : nullptr;
    }

    return &k_empty_set;
}

static bool str_to_double(const std::string s, double &d) {
    char *endp = nullptr;
    d = std::strtod(s.c_str(), &endp);
    return endp == s.c_str() + s.size() && !std::isnan(d);
}

static bool str_to_int(const std::string s, int64_t &i) {
    char *endp = nullptr;
    i = std::strtoll(s.c_str(), &endp, 10);
    return endp == s.c_str() + s.size();
}

void do_zquery(HMap &db, std::vector<std::string> &cmd, Buffer &out) {
    // parse zquery zset score name offset limit from str to each respective type
    // seek to the key
    // output

    ZSet *zset = expect_zset(db, cmd[1]);
    if (!zset) return out_err(out, ERR_BAD_TYPE, "expect zset");

    double score = 0;
    if (!str_to_double(cmd[2], score))
        return out_err(out, ERR_BAD_ARG, "expected fp number");

    const std::string name = cmd[3];

    int64_t offset = 0, limit = 0;
    if (!str_to_int(cmd[4], offset) || !str_to_int(cmd[5], limit))
        return out_err(out, ERR_BAD_ARG, "expected int");
    
        // unfinished
}