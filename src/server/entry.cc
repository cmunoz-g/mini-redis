#include "entry.hh"
#include "timer.hh"
#include "threadpool.hh"
#include "server.hh"
#include "heap.hh"
#include "utils.hh"
#include <vector>

static constexpr size_t k_large_container_size = 1000;

/* Internal helpers */
static void entry_del_sync(Entry *ent) {
    if (ent->type == t_zset) zset_destroy(&ent->zset);
    delete ent;
}

static void entry_del_f(void *arg) {
    entry_del_sync(static_cast<Entry *>(arg));
}

/* API */
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

void entry_del(g_data &data, Entry *ent) {
    entry_set_ttl(data.heap, ent, -1);
    size_t set_size = ent->type == t_zset ? hm_size(&ent->zset.hmap) : 0;

    if (set_size > k_large_container_size)
        thread_pool_queue(&data.thread_pool, &entry_del_f, ent);
    else
        entry_del_sync(ent);
}

void entry_set_ttl(std::vector<HeapItem> &heap, Entry *ent, int64_t ttl_ms) {
    if (ttl_ms < 0 && ent->heap_idx != heap_invalid) {
        heap_delete(heap, ent->heap_idx);
        ent->heap_idx = heap_invalid;
    }
    else if (ttl_ms >= 0) {
        uint64_t expires_at = get_monotonic_msec() + static_cast<uint64_t>(ttl_ms);
        HeapItem item = {expires_at, &ent->heap_idx};
        heap_upsert(heap, ent->heap_idx, item);
    }
}