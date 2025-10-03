#pragma once

struct DList {
    DList *prev = nullptr;
    DList *next = nullptr;
};

void dlist_init(DList *node);
void dlist_insert_before(DList *target, DList *node);
void dlist_detach(DList *node);
bool dlist_empty(DList *node);
