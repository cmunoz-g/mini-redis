#pragma once

struct ZSet {
    AVLNode *root = nullptr; // index by score, name
    HMap hmap; // index by name
};

struct ZNode {
    AVLNode tree;
    HNode map;
    double score = 0;
    size_t len = 0;
    char name[];
};

struct HKey {
    HNode node;
    const char *name = nullptr;
    size_t len = 0;
};