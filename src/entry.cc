#include "entry.hh"
#include "timer.hh"
#include <utils.hh>
#include <vector>
#include "heap.hh"

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

void entry_del(std::vector<HeapItem> &heap, Entry *ent) {
    if (ent->type == T_ZSET) zset_destroy(&ent->zset);
    entry_set_ttl(heap, ent, -1);
    delete ent;
}

void entry_set_ttl(std::vector<HeapItem> &heap, Entry *ent, int64_t ttl_ms) {
    if (ttl_ms < 0 && ent->heap_idx != HEAP_INVALID) { // negative ttl_ms = remove
        heap_delete(heap, ent->heap_idx);
        ent->heap_idx = HEAP_INVALID;
    }
    else if (ttl_ms >= 0) {
        uint64_t expires_at = get_monotonic_msec() + static_cast<uint64_t>(ttl_ms);
        HeapItem item = {expires_at, &ent->heap_idx};
        heap_upsert(heap, ent->heap_idx, item);
    }
}