#pragma once
#include <cstddef>

// research
#define container_of(ptr, T, member) \
    ((T *)((char *)(ptr) - offsetof(T, member)))

int err_msg(const char *msg, int fatal);