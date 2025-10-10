#include "server.hh"
#include "protocol.hh"
#include "commands.hh"
#include "entry.hh"
#include "utils.hh"
#include "hashtable.hh"
#include "sortedset.hh"
#include <assert.h>
#include <math.h>
#include <unistd.h>

static ZSet k_empty_set{};

/* Internal helpers */

static void cb_keys(HNode *node, void *arg) { 
    Buffer &out = *(static_cast<Buffer *>(arg));
    const std::string &key = container_of(node, Entry, node)->key;
    out_str(out, key.data(), key.size(), 0);
}

static ZSet *expect_zset(HMap &db, std::string s) {
    LookupKey key;
    key.key.swap(s);
    key.node.hcode = hash(reinterpret_cast<uint8_t *>(key.key.data()), key.key.size());
    HNode *hnode = hm_lookup(&db, &key.node, &entry_eq);
    
    if (hnode) {
        Entry *ent = container_of(hnode, Entry, node);
        return ent->type == t_zset ? &ent->zset : nullptr;
    }

    return &k_empty_set;
}

static bool str_to_double(const std::string s, double &d) {
    char *endp = nullptr;
    d = std::strtod(s.c_str(), &endp);
    if (d == 0.0) d = 0.0;
    return endp == s.c_str() + s.size() && std::isfinite(d);
}

static bool str_to_int(const std::string s, int64_t &i) {
    char *endp = nullptr;
    i = std::strtoll(s.c_str(), &endp, 10);
    return endp == s.c_str() + s.size();
}

/* API */

void do_get(Request &req) {
    g_data &data = req.data;
    std::vector<std::string> &cmd = req.cmd;
    Buffer &out = req.out;

    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = hash(reinterpret_cast<uint8_t *>(key.key.data()), key.key.size());
    
    HNode *node = hm_lookup(&data.db, &key.node, &entry_eq);
    if (!node) return out_nil(out);

    Entry *ent = container_of(node, Entry, node);
    if (ent->type != t_str) return out_err(out, err_bad_type, "Not a string value");

    return out_str(out, ent->str.data(), ent->str.size(), 0);
}

void do_set(Request &req) {
    g_data &data = req.data;
    std::vector<std::string> &cmd = req.cmd;
    Buffer &out = req.out;

    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = hash(reinterpret_cast<uint8_t *>(key.key.data()), key.key.size());
    HNode *node = hm_lookup(&data.db, &key.node, &entry_eq);

    if (node) {
        Entry *ent = container_of(node, Entry, node);
        if (ent->type != t_str) return out_err(out, err_bad_type, "Not a string value");
        ent->str.swap(cmd[2]);
    }
    else {
        Entry *ent = entry_new(t_str);
        ent->key.swap(key.key);
        ent->node.hcode = key.node.hcode;
        ent->str.swap(cmd[2]);
        hm_insert(&data.db, &ent->node);
    }
    std::string ok_msg = "OK";
    return out_str(out, ok_msg.data(), ok_msg.size(), 1);
}

void do_del(Request &req) {
    g_data &data = req.data;
    std::vector<std::string> &cmd = req.cmd;
    Buffer &out = req.out;

    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = hash(reinterpret_cast<uint8_t *>(key.key.data()), key.key.size());
    HNode *node = hm_lookup(&data.db, &key.node, &entry_eq);

    if (node) {
        hm_delete(&data.db, node, &entry_eq); 
        entry_del(data, container_of(node, Entry, node));
    }

    return out_int(out, node ? 1 : 0);
}

void do_keys(Request &req) {
    g_data &data = req.data;
    Buffer &out = req.out;
    out_arr(out, static_cast<uint32_t>(hm_size(&data.db)));
    hm_foreach(&data.db, &cb_keys, reinterpret_cast<void *>(&out));
}

void do_zquery(Request &req) {
    g_data &data = req.data;
    std::vector<std::string> &cmd = req.cmd;
    Buffer &out = req.out;

    ZSet *zset = expect_zset(data.db, cmd[1]);
    if (!zset) return out_err(out, err_bad_type, "Expected zset");

    double score = 0;
    if (!str_to_double(cmd[2], score))
        return out_err(out, err_bad_arg, "Expected fp number");

    const std::string name = cmd[3];

    int64_t offset = 0, limit = 0;
    if (!str_to_int(cmd[4], offset) || !str_to_int(cmd[5], limit))
        return out_err(out, err_bad_arg, "Expected int");
    
    ZNode *znode = zset_seekge(zset, score, name.data(), name.size());
    znode = znode_offset(znode, offset);

    size_t ctx = out_begin_arr(out);
    int64_t n = 0;
    while (znode && n < limit) {
        out_str(out, znode->name, znode->len, 0);
        out_dbl(out, znode->score);
        znode = znode_offset(znode, +1);
        n += 2;
    }
    out_end_arr(out, ctx, static_cast<uint32_t>(n));
}

void do_zadd(Request &req) {
    g_data &data = req.data;
    std::vector<std::string> &cmd = req.cmd;
    Buffer &out = req.out;

    double score = 0;
    if (!str_to_double(cmd[2], score)) out_err(out, err_bad_arg, "Expected fp number");

    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = hash(reinterpret_cast<uint8_t *>(key.key.data()), key.key.size());
    HNode *node = hm_lookup(&data.db, &key.node, &entry_eq);

    Entry *ent = nullptr;
    if (!node) {
        ent = entry_new(t_zset);
        ent->key.swap(key.key);
        ent->node.hcode = key.node.hcode;
        hm_insert(&data.db, &ent->node);
    }
    else {
        ent = container_of(node, Entry, node);
        if (ent->type != t_zset) out_err(out, err_bad_type, "Expected zset");
    }

    const std::string &name = cmd[3];
    bool added = zset_insert(&ent->zset, name.data(), name.size(), score);
    return out_int(out, static_cast<int64_t>(added));
}

void do_zrem(Request &req) {
    g_data &data = req.data;
    std::vector<std::string> &cmd = req.cmd;
    Buffer &out = req.out;

    ZSet *zset = expect_zset(data.db, cmd[1]);
    if (!zset) out_err(out, err_bad_type, "Expected zset");

    const std::string &name = cmd[2];
    ZNode *znode = zset_lookup(zset, name.data(), name.size());

    if (znode) zset_delete(zset, znode);
    return out_int(out, znode ? 1 : 0);
}

void do_zscore(Request &req) {
    g_data &data = req.data;
    std::vector<std::string> &cmd = req.cmd;
    Buffer &out = req.out;

    ZSet *zset = expect_zset(data.db, cmd[1]);
    if (!zset) out_err(out, err_bad_type, "Expected zset");
    
    const std::string &name = cmd[2];
    ZNode *znode = zset_lookup(zset, name.data(), name.size());
    return znode ? out_dbl(out, znode->score) : out_nil(out);
}

void do_expire(Request &req) {
    g_data &data = req.data;
    std::vector<std::string> &cmd = req.cmd;
    Buffer &out = req.out;

    int64_t ttl_ms = 0;
    if (!str_to_int(cmd[2], ttl_ms)) return out_err(out, err_bad_type, "Expected int");

    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = hash(reinterpret_cast<uint8_t *>(key.key.data()), key.key.size());
    HNode *node = hm_lookup(&data.db, &key.node, &entry_eq);

    if (node) {
        Entry *ent = container_of(node, Entry, node);
        entry_set_ttl(data.heap, ent, ttl_ms);
    }
    
    return out_int(out, node ? 1 : 0);
}

void do_quit(Request &req) {
    g_data &data = req.data;

    for (Conn *c : *(data.connections)) {
        if (c) {
            buf_reset(c->out);
            std::string bye_msg = "Closing server\n";
            size_t header_pos = 0;
            
            response_begin(c->out, &header_pos);
            out_close(c->out, bye_msg.data(), bye_msg.size());
            response_end(c->out, header_pos);

            write(c->fd, c->out.data_begin, buf_size(c->out));
        }
    }
    data.close_server = true;
}