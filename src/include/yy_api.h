#ifndef __YY_PROF_API_H__
#define __YY_PROF_API_H__

#include "include/yy_structs.h"

#define YY_PROF_FREE_ENTRY_PTR(entry) \
    if((entry)->ptr != NULL) { \
        if((entry)->type == IO_TYPE_SQL) { \
            zend_string * _s = (entry)->ptr; \
            ZSTR_RELEASE(_s); \
        } \
        (entry)->ptr = NULL; \
    }

void yy_prof_free_free_entries(yy_prof_entry_t **entries);

void yy_prof_free_request_entries(
    yy_prof_entry_t ** request_entries,
    yy_prof_entry_t ** entries);

static zend_always_inline yy_prof_entry_t * 
yy_prof_fast_alloc_entry(yy_prof_entry_t ** free_entries TSRMLS_DC) {
    yy_prof_entry_t * p = *free_entries;
    if(p) {
        *free_entries = p->prev;
    } else {
        p = pemalloc(sizeof(yy_prof_entry_t), 1); 
    }
    return p;
}
static zend_always_inline void
yy_prof_fast_free_entry(yy_prof_entry_t ** free_entries, yy_prof_entry_t * p) {
    p->prev = *free_entries;
    YY_PROF_FREE_ENTRY_PTR(p);
    ZSTR_RELEASE(p->key);
    ZSTR_RELEASE(p->func);
    *free_entries = p;
}

static zend_always_inline void
yy_prof_release_entry(yy_prof_entry_t ** request_entries, yy_prof_entry_t *p) {
    p->prev = *request_entries;
    YY_PROF_FREE_ENTRY_PTR(p);
    *request_entries = p;
}

#endif
