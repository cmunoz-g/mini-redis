#include "heap.hh"
#include "entry.hh"
#include <stddef.h>
#include <algorithm>

/* Internal helpers*/

static size_t heap_left_index(size_t i) {
    return i * 2 + 1;
}

static size_t heap_right_index(size_t i) {
    return i * 2 + 2;
}

static size_t heap_parent_index(size_t i) {
    return (i + 2) / 2 - 1;
}

static void heap_up(HeapItem *a, size_t pos) {
    HeapItem t = a[pos];
    while (pos > 0 && a[heap_parent_index(pos)].val > t.val) {
        a[pos] = a[heap_parent_index(pos)];
        *a[pos].ref = pos;
        pos = heap_parent_index(pos);
    }
    a[pos] = t;
    *a[pos].ref = pos;
}

static void heap_down(HeapItem *a, size_t pos, size_t len) {
    HeapItem t = a[pos];

    for (;;) {
        size_t l_index = heap_left_index(pos);
        size_t r_index = heap_right_index(pos);
        
        size_t min_pos = pos;

        if (l_index < len && a[l_index].val < t.val) min_pos = l_index;
        if (r_index < len && a[r_index].val < a[min_pos].val) min_pos = r_index;

        if (min_pos == pos) break;
        a[pos] = a[min_pos];
        *a[pos].ref = pos;
        pos = min_pos;
    }
    
    a[pos] = t;
    *a[pos].ref = pos;
}

/* API */

void heap_update(HeapItem *a, size_t pos, size_t len) {
    if (pos > 0 && a[heap_parent_index(pos)].val > a[pos].val)
        heap_up(a, pos);
    else
        heap_down(a, pos, len);
}

void heap_delete(std::vector<HeapItem> &a, size_t pos) {
    a[pos] = a.back();
    a.pop_back();
    if (pos < a.size()) heap_update(a.data(), pos, a.size());
}

void heap_upsert(std::vector<HeapItem> &a, size_t pos, HeapItem item) {
    if (pos < a.size()) a[pos] = item;
    else {
        pos = a.size();
        a.push_back(item);
    }
    heap_update(a.data(), pos, a.size());
}