#pragma once
#include <cstdint>
#include <vector>

struct HeapItem {
    uint64_t val;
    size_t *ref;
};

void heap_update(HeapItem *a, size_t pos, size_t len);
void heap_delete(std::vector<HeapItem> &a, size_t pos);
void heap_upsert(std::vector<HeapItem> &a, size_t pos, HeapItem item);