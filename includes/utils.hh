#pragma once
#include <cstddef>

// research
#define container_of(ptr, T, member) \
    ((T *)((char *)(ptr) - offsetof(T, member)))
    