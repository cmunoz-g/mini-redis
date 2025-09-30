#include "commands.hh"
#include "entry.hh"
#include "utils.hh"
#include "hashtable.hh"
#include "sortedset.hh"
#include <cassert>
#include <cmath>

void do_get(HMap &db, std::vector<std::string> &cmd, Buffer &out) {
    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = hash(reinterpret_cast<uint8_t *>(key.key.data()), key.key.size());
    
    HNode *node = hm_lookup(&db, &key.node, &entry_eq);
    if (!node) return out_nil(out);

    Entry *ent = container_of(node, Entry, node);
    if (ent->type != T_STR) return out_err(out, ERR_BAD_TYPE, "not a string value");

    return out_str(out, ent->str.data(), ent->str.size());
}

void do_set(HMap &db, std::vector<std::string> &cmd, Buffer &out) {
    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = hash(reinterpret_cast<uint8_t *>(key.key.data()), key.key.size());
    HNode *node = hm_lookup(&db, &key.node, &entry_eq);

    if (node) {
        Entry *ent = container_of(node, Entry, node);
        if (ent->type != T_STR) return out_err(out, ERR_BAD_TYPE, "not a string value");
        ent->str.swap(cmd[2]);
    }
    else {
        Entry *ent = entry_new(T_STR);
        ent->key.swap(key.key);
        ent->node.hcode = key.node.hcode;
        ent->str.swap(cmd[2]);
        hm_insert(&db, &ent->node);
    }
    return out_nil(out);
}

void do_del(HMap &db, std::vector<std::string> &cmd, Buffer &out) {
    LookupKey key;
    key.key.swap(cmd[2]);
    key.node.hcode = hash(reinterpret_cast<uint8_t *>(key.key.data()), key.key.size());
    HNode *node = hm_lookup(&db, &key.node, &entry_eq);

    if (node) entry_del(container_of(node, Entry, node));

    return out_int(out, node ? 1 : 0);
}

static bool cb_keys(HNode *node, void *arg) {
    Buffer &out = *(static_cast<Buffer *>(arg));
    const std::string &key = container_of(node, Entry, node)->key;
    out_str(out, key.data(), key.size());
    //return true;
}

void do_keys(HMap &db, std::vector<std::string> &cmd, Buffer &out) {
    // is it correct for this ft to receive &cmd even though it doesnt use them ? Only doing it so the compiler doesnt complain in command_list
    // would be either this or changing up do_request()
    out_arr(out, static_cast<uint32_t>(hm_size(&db)));
    hm_foreach(&db, &cb_keys, (void *)&out);
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

// missing: a reversed verseion that does seek and iterate in descending order
void do_zquery(HMap &db, std::vector<std::string> &cmd, Buffer &out) {
    ZSet *zset = expect_zset(db, cmd[1]);
    if (!zset) return out_err(out, ERR_BAD_TYPE, "expect zset");

    double score = 0;
    if (!str_to_double(cmd[2], score))
        return out_err(out, ERR_BAD_ARG, "expected fp number");

    const std::string name = cmd[3];

    int64_t offset = 0, limit = 0;
    if (!str_to_int(cmd[4], offset) || !str_to_int(cmd[5], limit))
        return out_err(out, ERR_BAD_ARG, "expected int");
    
    ZNode *znode = zset_seekge(zset, score, name.data(), name.size());
    znode = znode_offset(znode, offset);

    size_t ctx = out_begin_arr(out);
    int64_t n = 0;
    while (znode && n < limit) {
        out_str(out, znode->name, znode->len);
        out_dbl(out, znode->score);
        znode = znode_offset(znode, +1);
        n += 2;
    }
    out_end_arr(out, ctx, static_cast<uint32_t>(n));
}

// zadd zset score name
void do_zadd(HMap &db, std::vector<std::string> &cmd, Buffer &out) {
    double score = 0;
    if (!str_to_double(cmd[2], score)) out_err(out, ERR_BAD_ARG, "expected fp number");

    // this is done instead of calling expect_zset because the results
    // from hash() and hm_lookup() are both needed
    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = hash(reinterpret_cast<uint8_t *>(key.key.data()), key.key.size());
    HNode *node = hm_lookup(&db, &key.node, &entry_eq);

    Entry *ent = nullptr;
    if (!node) {
        ent = entry_new(T_ZSET);
        ent->key.swap(key.key);
        ent->node.hcode = key.node.hcode;
        hm_insert(&db, &ent->node);
    }
    else {
        ent = container_of(node, Entry, node);
        if (ent->type != T_ZSET) out_err(out, ERR_BAD_TYPE, "expected zset");
    }

    const std::string &name = cmd[3];
    bool added = zset_insert(&ent->zset, name.data(), name.size(), score);
    return out_int(out, static_cast<int64_t>(added));
}

void do_zrem(HMap &db, std::vector<std::string> &cmd, Buffer &out) {
    ZSet *zset = expect_zset(db, cmd[1]);
    if (!zset) out_err(out, ERR_BAD_TYPE, "expected zset");

    const std::string &name = cmd[2];
    ZNode *znode = zset_lookup(zset, name.data(), name.size());
    if (znode) zset_delete(zset, znode);
    return out_int(out, znode ? 1 : 0);
}

void do_zscore(HMap &db, std::vector<std::string> &cmd, Buffer &out) {
    ZSet *zset = expect_zset(db, cmd[1]);
    if (!zset) out_err(out, ERR_BAD_TYPE, "expected zset");
    
    const std::string &name = cmd[2];
    ZNode *znode = zset_lookup(zset, name.data(), name.size());
    return znode ? out_dbl(out, znode->score) : out_nil(out);
}