#pragma once

struct AVLNode {
    AVLNode *parent;
    AVLNode *r;
    AVLNode *l;
    uint32_t height = 0;
    uint32_t count = 0;
};

void avl_init(AVLNode *node);
AVLNode *avl_fix(AVLNode *node);
AVLNode *avl_del(AVLNode *node);
AVLNode *avl_offset(AVLNode *node, int64_t offset);