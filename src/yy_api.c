#include "include/yy_api.h"

/**
 * Free any entries in the free list.
 */
void yy_prof_free_free_entries(yy_prof_entry_t ** entries) {
    yy_prof_entry_t *p = *entries;
    yy_prof_entry_t * cur;

    while (p) {
        cur = p;
        p = p->prev;
        pefree(cur, 1);
    }
    *entries = NULL;
}

void yy_prof_free_request_entries(
        yy_prof_entry_t ** request_entries, 
        yy_prof_entry_t ** free_entries) {
    yy_prof_entry_t * p = *request_entries;
    yy_prof_entry_t * cur = p;

    while (p) {
        cur = p->prev;
        p->prev = *free_entries;
        ZSTR_RELEASE(p->key);
        ZSTR_RELEASE(p->func);
        *free_entries = p;
        p = cur;
    }
    *request_entries = NULL;
}
