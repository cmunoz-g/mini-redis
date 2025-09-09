#pragma once

#include <cstddef>

#define container_of(ptr, T, member) \ 
    ((T *)((char *)(ptr) - offsetof(T, member))) // research