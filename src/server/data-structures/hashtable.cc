#include "hashtable.hh"
#include <limits>
#include <assert.h>
#include <stdlib.h>

static constexpr size_t max_load_factor = 8;
static constexpr size_t initial_table_size = 4;
static constexpr size_t rehashing_work = 128;

/* Internal helpers*/

// Tab functions
static void h_init(HTab *htab, size_t n) {
    assert(n > 0 && ((n - 1) & n) == 0);
    htab->tab = reinterpret_cast<HNode **>(calloc(n, sizeof(HNode *)));
    htab->mask = n - 1;
    htab->size = 0;
    htab->threshold = (htab->mask + 1) * max_load_factor;
}

static void h_insert(HTab *htab, HNode *node) {
    size_t pos = node->hcode & htab->mask;
    HNode *next = htab->tab[pos];
    node->next = next;
    htab->tab[pos] = node;
    htab->size++;
}

static HNode **h_lookup(HTab *htab, HNode *key, bool (*eq)(HNode *, HNode *)) {
    if (!htab->tab) return nullptr;

    size_t pos = key->hcode & htab->mask;
    HNode **from = &htab->tab[pos];
    for (HNode *cur; (cur = *from) != nullptr; from = &cur->next) {
        if (cur->hcode == key->hcode && eq(cur, key)) return from;
    }
    return nullptr;
}

static HNode *h_detach(HTab *htab, HNode **from) {
    HNode *node = *from;
    *from = node->next;
    htab->size--;
    return node;
}

static void h_foreach(HTab *htab, void (*f)(HNode *, void *), void *arg) {
    if (htab->mask == 0) return;

    const size_t buckets = htab->mask + 1;
    for (size_t i = 0; i < buckets; ++i) {
        HNode *node = htab->tab[i];
        while (node) {
            HNode *next = node->next;
            f(node, arg);
            node = next;
        }
    }
}

// HMap rehashing functions
static void hm_help_rehashing(HMap *hmap) {
    size_t nwork = 0;
    while (nwork < rehashing_work && hmap->older.size > 0 && hmap->migrate_pos <= hmap->older.mask) {
        HNode **from = &hmap->older.tab[hmap->migrate_pos];
        if (!*from) {
            hmap->migrate_pos++;
            continue;
        }
        HNode *moved = h_detach(&hmap->older, from);
        h_insert(&hmap->newer, moved);
        nwork++;
    }

    if (hmap->older.tab && hmap->older.size == 0) {
        free(hmap->older.tab);
        hmap->older = HTab{};
    }
}

static void hm_trigger_rehashing(HMap *hmap) {
    assert(hmap->older.tab == nullptr);

    const size_t curr_mask = hmap->newer.mask + 1;
    if (curr_mask > (std::numeric_limits<size_t>::max() >> 1)) return;
    hmap->older = hmap->newer; 
    h_init(&hmap->newer, curr_mask * 2);
    hmap->migrate_pos = 0;
}

/* API */

bool hnode_same(HNode *node, HNode *key) {
    return node == key;
}

uint64_t hash(const uint8_t *data, size_t len) { 
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; ++i) {
        h = (h + data[i]) * 0x01000193;
    }
    return h;
}

HNode *hm_lookup(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *)) {
    hm_help_rehashing(hmap);
    HNode **from = h_lookup(&hmap->newer, key, eq);
    if (!from)
    from = h_lookup(&hmap->older, key, eq);
    return from ? *from : nullptr;
}

HNode *hm_delete(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *)) {
    hm_help_rehashing(hmap);
    if (HNode **from = h_lookup(&hmap->newer, key, eq))
        return h_detach(&hmap->newer, from);
    if (HNode **from = h_lookup(&hmap->older, key, eq))
        return h_detach(&hmap->older, from);
    return nullptr;
}

void hm_insert(HMap *hmap, HNode *node) {
    if (!hmap->newer.tab) 
        h_init(&hmap->newer, initial_table_size);
    h_insert(&hmap->newer, node);
    if (!hmap->older.tab && hmap->newer.size >= hmap->newer.threshold)
        hm_trigger_rehashing(hmap);
    hm_help_rehashing(hmap);
}

size_t hm_size(HMap *hmap) {
    return (hmap->newer.size + hmap->older.size);
}

void hm_foreach(HMap *hmap, void (*f)(HNode *, void *), void *arg) {
    h_foreach(&hmap->newer, f, arg);
    h_foreach(&hmap->older, f, arg);
}

void hm_destroy(HMap *hmap) {
    free(hmap->newer.tab);
    free(hmap->older.tab);
    *hmap = HMap{};
}