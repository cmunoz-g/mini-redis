#include "dlist.hh"

#include <stdio.h>
#include <stdlib.h>

void dlist_detach(DList *node) {
    DList *prev = node->prev;
    DList *next = node->next;
    prev->next = next;
    next->prev = prev;
}

void dlist_init(DList *node) {
    node->prev = node->next = node;
}

bool dlist_empty(DList *node) {
    return node->next == node;
}

void dlist_insert_before(DList *target, DList *node) {
    DList *prev = target->prev;
    prev->next = node;
    node->prev = prev;
    node->next = target;
    target->prev = node;
}