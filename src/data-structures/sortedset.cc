#include "sortedset.hh"
#include "utils.hh"
#include <cstdlib>
#include <cstring>
#include <string>
#include <assert.h>

static bool hcmp(HNode *node, HNode *key) {
    ZNode *znode = container_of(node, ZNode, map);
    HKey *hkey = container_of(key, HKey, node);
    if (znode->len != hkey->len) return false;
    return memcmp(znode->name, hkey->name, znode->len) == 0;
}

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

ZNode *zset_lookup(ZSet *zset, const char *name, size_t len) {
    if (!zset->root) return nullptr;

    HKey key;
    key.node.hcode = hash(reinterpret_cast<const uint8_t *>(name), len);
    key.name = name;
    key.len = len;

    HNode *found = hm_lookup(&zset->hmap, &key.node, &hcmp);
    return found ? container_of(found, ZNode, map) : nullptr;
}

static bool zless(AVLNode *lhs, AVLNode *rhs) {
    ZNode *zl = container_of(lhs, ZNode, tree);
    ZNode *zr = container_of(rhs, ZNode, tree);
    if (zl->score != zr->score) return zl->score < zr->score;

    int rv = memcmp(zl->name, zr->name, std::min(zl->len, zr->len));
    return (rv != 0) ? (rv < 0) : (zl->len < zr->len);
}

static bool zless(AVLNode *lhs, double score, const char *name, size_t len) { // change name ? or signature can stay zless ?
    ZNode *zl = container_of(lhs, ZNode, tree);
    if (zl->score != score) return zl->score < score;
    
    int rv = memcmp(zl->name, name, std::min(zl->len, len));
    return (rv != 0) ? (rv < 0) : (zl->len < len);
}

static void tree_insert(ZSet *zset, ZNode *node) {
    AVLNode *parent = nullptr;
    AVLNode **from = &zset->root;

    while (*from) {
        parent = *from;
        from = zless(&node->tree, parent) ? &parent->l : &parent->r;
    }

    *from = &node->tree;
    node->tree.parent = parent;
    zset->root = avl_fix(&node->tree);
}

static void zset_update(ZSet *zset, ZNode *node, double score) {
    zset->root = avl_del(&node->tree);
    avl_init(&node->tree);
    node->score = score;
    tree_insert(zset, node);
}

bool zset_insert(ZSet *zset, const char *name, size_t len, double score) {
    if (ZNode *node = zset_lookup(zset, name, len)) {
        zset_update(zset, node, score);
        return false;
    }
    ZNode *node = znode_new(name, len, score);
    hm_insert(&zset->hmap, &node->map);
    tree_insert(zset, node);
    return true;
}

void zset_delete(ZSet *zset, ZNode *node) {
    HKey key;
    key.node.hcode = node->map.hcode;
    key.name = node->name;
    key.len = node->len;

    HNode *found = hm_delete(&zset->hmap, &node->map, &hcmp);
    assert(found);

    zset->root = avl_del(&node->tree);
    znode_del(node);
}

// ranged queries

ZNode *zset_seekge(ZSet *zset, double score, const char *name, size_t len) {
    AVLNode *found = nullptr;
    AVLNode *node = zset->root;
    while (node) {
        if (zless(node, score, name, len)) node = node->r;
        else {
            found = node;
            node = node->l;
        }
    }
    return found ? container_of(found, ZNode, tree) : nullptr;
}

ZNode *znode_offset(ZNode *node, int64_t offset) {
    AVLNode *tnode = node ? avl_offset(&node->tree, offset) : nullptr;
    return tnode ? container_of(tnode, ZNode, tree) : nullptr;
}

static void tree_destroy(AVLNode *node) {
    if (node->l) tree_destroy(node->l);
    if (node->r) tree_destroy(node->r);
    znode_del(container_of(node, ZNode, tree));
}

void zset_destroy(ZSet *zset) {
    hm_destroy(&zset->hmap);
    tree_destroy(zset->root);
    zset->root = nullptr;
}
