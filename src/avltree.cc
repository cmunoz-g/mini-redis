#include "avltree.hh"
#include <cstdint>
#include <algorithm>

struct AVLNode {
    AVLNode *parent;
    AVLNode *r;
    AVLNode *l;
    uint32_t height = 0;
};

inline void avl_init(AVLNode *node) {
    node->l = node->r = node->parent = nullptr;
    node->height = 1;
}

static uint32_t avl_height(AVLNode *node) {
    return node ? node->height : 0;
}

static void avl_update(AVLNode *node) {
    node->height = 1 + std::max(avl_height(node->l), avl_height(node->r));
}

static uint8_t avl_get_height_diff(AVLNode *node) {
    uintptr_t p = reinterpret_cast<uintptr_t>(node->parent);
    return p & 0b11;
}

static AVLNode *avl_get_parent(AVLNode *node) {
    uintptr_t p = reinterpret_cast<uintptr_t>(node->parent);
    return reinterpret_cast<AVLNode *>(p & (~0b11));
}

static AVLNode *rot_left(AVLNode *node) {
    AVLNode *parent = node->parent;
    AVLNode *new_node = node->r;
    AVLNode *inner = new_node->l;

    node->r = inner;
    if (inner) inner->parent = node; 

    new_node->parent = parent;
    new_node->l = node;
    node->parent = new_node;

    avl_update(node);
    avl_update(new_node);

    return new_node;
}

static AVLNode *rot_right(AVLNode *node) {
    AVLNode *parent = node->parent;
    AVLNode *new_node = node->l;
    AVLNode *inner = new_node->r;

    node->l = inner;
    if (inner) inner->parent = node;

    new_node->parent = parent;
    new_node->r = node;
    node->parent = new_node;

    avl_update(node);
    avl_update(new_node);

    return new_node;
}

static AVLNode *avl_fix_left(AVLNode *node) {
    if (avl_height(node->l->l) < avl_height(node->l->r))
        node->l = rot_left(node->l);
    return rot_right(node);
}

static AVLNode *avl_fix_right(AVLNode *node) {
    if (avl_height(node->r->r) < avl_height(node->r->l))
        node->r = rot_right(node->r);
    return rot_left(node);
} 

AVLNode *avl_fix(AVLNode *node) {
    
}