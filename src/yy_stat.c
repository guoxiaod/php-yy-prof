#include <errno.h>

#include "php.h"
#include "php_yy_prof.h"

#include "include/yy_stat.h"
#include "include/yy_storage.h"
#include "php_yy_prof.h"

#define YY_PROF_MAX_TRACE_SIZE 100

static zend_always_inline zend_string *
yy_prof_get_stat_key(yy_prof_request_t * request) {
    size_t len, total_len;
    char *str, *mark;
    zend_string * stat_key, * uri;

    if(request->stat_key && ZSTR_LEN(request->stat_key) > 0) {
        return request->stat_key;
    }

    uri = request->uri2 ? request->uri2 : request->uri;
    mark = strchr(ZSTR_VAL(uri), '?');
    len = mark ? mark - ZSTR_VAL(uri) : ZSTR_LEN(uri);
    total_len = len + (request->tag ? ZSTR_LEN(request->tag) + 1 : 0);

    stat_key = zend_string_alloc(total_len, 0);
    memcpy(ZSTR_VAL(stat_key), ZSTR_VAL(uri), len);
    if(request->tag) {
        ZSTR_VAL(stat_key)[len] = '#';
        memcpy(ZSTR_VAL(stat_key) + len + 1, ZSTR_VAL(request->tag), ZSTR_LEN(request->tag));
    }
    ZSTR_VAL(stat_key)[total_len] = '\0';

    // fix restful uri
    str = ZSTR_VAL(stat_key) + len - 1;
    while(str >= ZSTR_VAL(stat_key) && *str >= '0' && *str <= '9') {
        *str -- = 'N';
    }

    // strip to YY_PROF_MAX_KEY_LEN
    ZSTR_LEN(stat_key) = min(total_len, YY_PROF_MAX_KEY_LEN - 1);
    ZSTR_VAL(stat_key)[ZSTR_LEN(stat_key)] = '\0';

    request->stat_key = stat_key;

    return stat_key;
}

static zend_always_inline void 
yy_prof_set_stat_2_status(yy_prof_func_stat_t * stat, int status) {
    if (status < 300) {
        stat->status_200 ++; 
    } else if (status < 400) {
        stat->status_300 ++;
    } else if (status < 500) {
        stat->status_400 ++;
    } else if (status == 500) {
        stat->status_500 ++;
    } else {
        stat->status_501 ++;
    }
}


void 
yy_prof_record_page_stat(yy_prof_page_stat_t * stat, yy_prof_entry_t * entry) {
    switch(entry->type) {
        case IO_TYPE_URL:
            stat->url_time += entry->time;
            stat->url_count ++;
            break;
        case IO_TYPE_SQL:
            stat->sql_time += entry->time;
            stat->sql_count ++;
            break;
        case IO_TYPE_QUEUE:
            stat->queue_time += entry->time;
            stat->queue_count ++;
            break;
        case IO_TYPE_CACHE:
            stat->cache_time += entry->time;
            stat->cache_count ++;
            break;
        case IO_TYPE_MONGODB:
            stat->mongodb_time += entry->time;
            stat->mongodb_count ++;
            break;
        case IO_TYPE_DEFAULT:
        default:
            stat->default_time += entry->time;
            stat->default_count ++;
            break;
    }
}

yy_prof_db_t
yy_prof_open_stat_db(char *file) {
    mode_t mode;
    char real_file[PATH_MAX] = {0};
    yy_prof_storage_t *storage = YY_PROF_G(storage);

    char *errptr = NULL;
    mode = umask(0);
    snprintf(real_file, sizeof(real_file), file, YY_PROF_STAT_VERSION, storage->name);
    yy_prof_db_t * db = storage->open(real_file, &errptr);
    umask(mode);

    if (db == NULL) {
        php_error(E_ERROR, "YY_PROF: Can not open file %s : %s", real_file, errptr);
        storage->free(errptr);
    }
    return db;
}

void 
yy_prof_close_stat_db(yy_prof_db_t db) {
    yy_prof_storage_t *storage = YY_PROF_G(storage);

    storage->close(db);
}

// page
void 
yy_prof_save_page_stat_to_db(
        yy_prof_db_t db, 
        yy_prof_request_t * request,
        yy_prof_page_stat_t * stat) {
    yy_prof_storage_t *storage = YY_PROF_G(storage);

    size_t vlen = 0;
    char * val = NULL, *errptr = NULL;
    zend_string * stat_key;
    yy_prof_page_stat_t * old_stat, new_stat = {0};

    if(db == NULL || ZSTR_LEN(request->uri) == 0) {
        return;
    }
    stat_key = yy_prof_get_stat_key(request);

    storage->get(db, ZSTR_VAL(stat_key), ZSTR_LEN(stat_key), &val, &vlen, &errptr);
    if (errptr != NULL) {
        php_error(E_WARNING, "YY_PROF: get page key '%s' fail: %s", ZSTR_VAL(stat_key), errptr);
        storage->free(errptr);
    }

    if (vlen == 0) {
        old_stat = &new_stat;
    } else {
        old_stat = (yy_prof_page_stat_t *) val; 
    }

    old_stat->request_count ++;
    old_stat->request_time += stat->request_time;
    old_stat->url_count += stat->url_count;
    old_stat->url_time += stat->url_time;
    old_stat->sql_count += stat->sql_count;
    old_stat->sql_time += stat->sql_time;
    old_stat->cache_count += stat->cache_count;
    old_stat->cache_time += stat->cache_time;
    old_stat->queue_count += stat->queue_count;
    old_stat->queue_time += stat->queue_time;
    old_stat->mongodb_count += stat->mongodb_count;
    old_stat->mongodb_time += stat->mongodb_time;
    old_stat->default_time += stat->default_time;

    // need not set for replace mode (mdbm)
    if (vlen == 0 || (storage->flags & FLAG_COPY)) {
        storage->set(db, ZSTR_VAL(stat_key), ZSTR_LEN(stat_key),
            (char*)old_stat, sizeof(yy_prof_page_stat_t), &errptr);
        if (errptr != NULL) {
            php_error(E_WARNING, "YY_PROF: save page key '%s' fail: %s", ZSTR_VAL(stat_key), errptr); 
            storage->free(errptr);
        }
    }
    storage->try_free(val);
}

// func
void 
yy_prof_save_func_stat_to_db(yy_prof_db_t db, yy_prof_entry_t * entries) {
    yy_prof_storage_t *storage = YY_PROF_G(storage);
    yy_prof_entry_t * entry = entries;
    yy_prof_func_stat_t * old_stat, new_stat = {0};

    size_t vlen;
    char *val = NULL, *errptr = NULL;

    if(db == NULL || entry == NULL) {
        return;
    }

    while(entry != NULL) {
        memset(&new_stat, 0, sizeof(new_stat));
        if(ZSTR_LEN(entry->key) == 0) {
            entry = entry->prev;
            continue;
        }
        if(entry->adapter != NULL && (entry->adapter->flag & FLAG_SKIP_STAT)) {
            entry = entry->prev;
            continue;
        }
        
        storage->get(db, ZSTR_VAL(entry->key), ZSTR_LEN(entry->key), &val, &vlen, &errptr);
        if (errptr != NULL) {
            php_error(E_WARNING, "YY_PROF: get func key '%s' fail: %s", ZSTR_VAL(entry->key), errptr);
            storage->free(errptr);
        }
        if (vlen == 0) {
            old_stat = &new_stat;
        } else {
            old_stat = (yy_prof_func_stat_t *) val; 
        }

        old_stat->type = entry->type;
        old_stat->count ++;
        old_stat->time += entry->time;
        yy_prof_set_stat_2_status(old_stat, entry->status);
        old_stat->request_bytes += entry->request_bytes;
        old_stat->response_bytes += entry->response_bytes;

        // need not save for inplace mode(mdbm)
        if (vlen == 0 || (storage->flags == FLAG_COPY)) {
            storage->set(db, ZSTR_VAL(entry->key), ZSTR_LEN(entry->key),
                (char*)old_stat, sizeof(yy_prof_func_stat_t), &errptr);
            if (errptr != NULL) {
                php_error(E_WARNING, "YY_PROF: save func key '%s' fail: %s", ZSTR_VAL(entry->key), errptr);
                storage->free(errptr);
            }
        }

        storage->try_free(val);

        entry = entry->prev;
    }
}

static void pfs_swap(void * a, void * b) {
    yy_prof_page_func_stat_t s;
    yy_prof_page_func_stat_t *f = a;
    yy_prof_page_func_stat_t *t = b;
    s = *(yy_prof_page_func_stat_t *) f;
    *f = *(yy_prof_page_func_stat_t *) t;
    *t = s;
}

static int pfs_compare(const void *a, const void *b)
{
    const yy_prof_page_func_stat_t *f; 
    const yy_prof_page_func_stat_t *s; 

    f = ((const yy_prof_page_func_stat_t *) a); 
    s = ((const yy_prof_page_func_stat_t *) b); 

    if (f->key.s.len == 0 && s->key.s.len == 0) { /* both numeric */
        return ZEND_NORMALIZE_BOOL(f->key.s.len - s->key.s.len);
    } else if (f->key.s.len == 0) { /* f is numeric, s is not */
        return -1; 
    } else if (s->key.s.len == 0) { /* s is numeric, f is not */
        return 1;
    } else { /* both strings */
        return zend_binary_strcmp(f->key.s.val, f->key.s.len, s->key.s.val, s->key.s.len);
    }   
}


// page -> func
void yy_prof_save_page_func_stat_to_db_ex(
        yy_prof_db_t db, 
        yy_prof_request_t *request, 
        yy_prof_entry_t * entries) {
    yy_prof_storage_t *storage = YY_PROF_G(storage);

    zend_string * stat_key;
    int len = 0;
    int count = 0, old_count = 0, old_index = 0, new_index = 0, left_count = 0;

    size_t vlen;
    char * val = NULL, *errptr = NULL;

    HashTable ht = {{0}};
    HashPosition pos = {0};
    yy_prof_entry_t * entry;
    yy_prof_page_func_stat_t * old_stat, *tmp_stat = NULL, *stat, *stat2;
    yy_prof_page_func_stat_t new_stat[YY_PROF_MAX_TRACE_SIZE] = {{{{{0}}}}};

    if(db == NULL || request->uri == NULL || entries == NULL) {
        return;
    }

    stat_key = yy_prof_get_stat_key(request);

    // fetch old stat from mdbm
    zend_hash_init(&ht, YY_PROF_MAX_TRACE_SIZE, NULL, NULL, 0);

    storage->get(db, ZSTR_VAL(stat_key), ZSTR_LEN(stat_key), &val, &vlen, &errptr);
    if (errptr != NULL) {
        php_error(E_WARNING, "YY_PROF: get trace key '%s' fail: %s", ZSTR_VAL(stat_key), errptr);
        storage->free(errptr);
    }
    if (vlen > 0) {
        old_count = vlen / sizeof(yy_prof_page_func_stat_t); 
        // TODO check the size is valid
        old_stat = (yy_prof_page_func_stat_t*) val;

        old_index = 0;
        while (old_index ++ < old_count) {
            // @NOTE: just a tricky, give old_stat->key a big refcount, and will not free in zend_string_release
            GC_REFCOUNT(&old_stat->key.s) = 100;
            zend_hash_add_ptr(&ht, &old_stat->key.s, old_stat);
            old_stat ++;
        }
    }

    // copy new stat to hash 
    new_index = 0;
    entry = entries;
    left_count = YY_PROF_MAX_TRACE_SIZE - old_count;
    while(entry != NULL && new_index < YY_PROF_MAX_TRACE_SIZE) {
        if(entry->key != NULL && entry->adapter != NULL && !(entry->adapter->flag & FLAG_SKIP_STAT)) {
            if((tmp_stat = zend_hash_find_ptr(&ht, entry->key)) != NULL) {
                tmp_stat->count ++;
                tmp_stat->time += entry->time;
            } else if(new_index < left_count) {
                stat2 = &new_stat[new_index];
                ZSTR_FROM_ZSTR(&stat2->key.s, entry->key);

                stat2->type = entry->type;
                stat2->count = 1;
                stat2->time = entry->time;
                
                GC_REFCOUNT(&stat2->key.s) = 100;
                zend_hash_add_ptr(&ht, &stat2->key.s, stat2);
                new_index ++;
            }
        }
        entry = entry->prev;
    }

    // inplace mode and not modify keys, directly return
    if (new_index == 0 && (storage->flags & FLAG_INPLACE)) {
        storage->try_free(val);
        zend_hash_destroy(&ht);
        return ;
    }

    // save all stat to db by the keys order
    count = zend_hash_num_elements(&ht);
    len = count * sizeof(yy_prof_page_func_stat_t);
    stat = stat2 = (yy_prof_page_func_stat_t *) emalloc(len);
    memset(stat, 0, len);

    for(zend_hash_internal_pointer_reset_ex(&ht, &pos);
        zend_hash_has_more_elements_ex(&ht, &pos) == SUCCESS;
        zend_hash_move_forward_ex(&ht, &pos)) {
        if((tmp_stat = zend_hash_get_current_data_ptr_ex(&ht, &pos)) != NULL) {
            memcpy(stat2++, tmp_stat, sizeof(yy_prof_page_func_stat_t)); 
        }
    }
    if(count > 1) {
        zend_sort(stat, count, sizeof(yy_prof_page_func_stat_t), pfs_compare, pfs_swap);
    }

    storage->set(db, ZSTR_VAL(stat_key), ZSTR_LEN(stat_key), (char*) stat, len, &errptr);
    if (errptr != NULL) {
        php_error(E_WARNING, "YY_PROF: save key '%s' fail: %s", ZSTR_VAL(stat_key), errptr);
        storage->free(errptr);
    }
    zend_hash_destroy(&ht);
    storage->try_free(val);
    efree(stat);
}
