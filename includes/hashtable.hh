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
//void hm_init(HMap*); // do i need these ?
//void hm_destroy(HMap*); // 
void hm_insert(HMap*, HNode*);
HNode *hm_lookup(HMap*, HNode*, bool (*eq)(HNode*, HNode*));
HNode *hm_delete(HMap*, HNode*, bool (*eq)(HNode *, HNode *));