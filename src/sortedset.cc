#include "sortedset.hh"
#include "hashtable.hh"
#include "avltree.hh"
#include <cstdlib>
#include <cstring>
#include <string>

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

static ZNode *znode_new(const char *name, size_t len, double score) {
    size_t bytes = offsetof(struct ZNode, name) + len;
    ZNode *node = static_cast<ZNode *>(std::malloc(bytes)); // should it be a C style cast ?
    if (!node) return nullptr;

    avl_init(&node->tree);
    node->map.next = nullptr;
    node->map.hcode = hash(reinterpret_cast<const uint8_t *>(name), len);
    node->score = score;
    node->len = len;
    std::memcpy(node->name, name, len);
    return node;
}

static void znode_del(ZNode *node) {
    free(node);
}

