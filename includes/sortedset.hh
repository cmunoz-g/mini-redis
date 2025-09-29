#pragma once

struct ZSet {
    AVLNode *root = nullptr; // index by score, name
    HMap hmap; // index by name
};

struct ZNode {
    AVLNode tree;
    HNode map;
    double score = 0;
    size_t len = 0;
    char name[];
};

struct HKey {
    HNode node;
    const char *name = nullptr;
    size_t len = 0;
};

// review what should be API
ZNode *zset_seekge(ZSet *zset, double score, const char *name, size_t len);
ZNode *znode_offset(ZNode *node, int64_t offset);
void zset_delete(ZSet *zset, ZNode *node);
bool zset_insert(ZSet *zset, const char *name, size_t len, double score);
ZNode *zset_lookup(ZSet *zset, const char *name, size_t len);