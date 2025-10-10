#pragma once
#include <cstddef>
#include <cstdint>

/* Structs */
struct HNode {
    HNode *next = nullptr;
    uint64_t hcode = 0;
};

struct HTab {
    HNode **tab = nullptr;
    size_t mask = 0;
    size_t size = 0;
    size_t threshold;
};

struct HMap {
    HTab newer;
    HTab older;
    size_t migrate_pos = 0;
};

/* API */
//void hm_init(HMap*); // do i need this ?
void hm_destroy(HMap *hmap);
uint64_t hash(const uint8_t *data, size_t len); // should it be moved elsewhere ?
bool hnode_same(HNode *node, HNode *key);
void hm_insert(HMap *hmap, HNode *node);
HNode *hm_lookup(HMap *hmap, HNode *key, bool (*eq)(HNode*, HNode*));
HNode *hm_delete(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *));
size_t hm_size(HMap *hmap);
void hm_foreach(HMap *hmap, void (*f)(HNode *, void *), void *arg);