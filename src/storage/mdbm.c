#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_yy_prof.h"
#include <mdbm.h>

#include "include/yy_storage.h"

const int YY_PROF_MDBM_PAGE_SIZE = 1024 * 16;
const int YY_PROF_MDBM_INIT_SIZE = 1024 * 1024 * 16;

typedef struct _yy_prof_storage_mdbm_iter {
    MDBM *db;
    MDBM_ITER iter;
    kvpair kv;
} yy_prof_storage_mdbm_iter_t;

yy_prof_db_t yy_prof_storage_mdbm_open(const char * file, char ** errptr) {
    mode_t mode;
    yy_prof_db_t *db = NULL;
    int flags = MDBM_O_RDWR | MDBM_O_CREAT | MDBM_O_ASYNC;

    mode = umask(0111);
    errno = 0;
    db = mdbm_open(file, flags, 0666, YY_PROF_MDBM_PAGE_SIZE, YY_PROF_MDBM_INIT_SIZE);
    umask(mode);
    if (db == NULL) {
        php_error(E_WARNING, "YY_PROF: open mdbm %s failed, errno=%d errstr=%s",
            file, errno, strerror(errno));
        return db;
    }

    return db;
}

void yy_prof_storage_mdbm_close(yy_prof_db_t db) {
    mdbm_fsync(db);
    mdbm_close(db);
}

void yy_prof_storage_mdbm_clear(yy_prof_db_t db) {
    mdbm_purge(db);
    mdbm_fsync(db);
}

void yy_prof_storage_mdbm_set(
        yy_prof_db_t db, 
        const char *key, 
        size_t klen,
        const char *val, 
        size_t vlen, 
        char **errptr) {

    datum key_ = {key, klen}, val_ = {val, vlen};
    errno = 0;
    *errptr = NULL;
    if (mdbm_store_r(db, &key_, &val_, MDBM_REPLACE, NULL) == -1) {
        char buffer[200];
        snprintf(buffer, sizeof(buffer), "store %s fail: errno=%d errstr=%s", key, errno, strerror(errno));
        *errptr = strndup(buffer, strlen(buffer));
    }
}

void yy_prof_storage_mdbm_get(
        yy_prof_db_t db, 
        const char *key, 
        size_t klen, 
        char **val, 
        size_t *vlen, 
        char **errptr) {

    datum key_ = {key, klen}, val_;
    errno = 0;
    *errptr = NULL;
    if (mdbm_fetch_r(db, &key_, &val_, NULL)) {
        if (errno != ENOENT) {
            char buffer[200];
            snprintf(buffer, sizeof(buffer), 
                "get %s fail: errno=%d errstr=%s", key, errno, strerror(errno));
            *errptr = strndup(buffer, strlen(buffer));
        }
        *val = NULL;
        *vlen = 0;
        return ;
    }
    *val = val_.dptr;
    *vlen = val_.dsize;
}

void yy_prof_storage_mdbm_free(void *ptr) {
    free(ptr);
}

void yy_prof_storage_mdbm_try_free(void *ptr) {
    // do nothing
}

void yy_prof_storage_mdbm_sets(yy_prof_db_t db) {

}

yy_prof_db_iter_t yy_prof_storage_mdbm_iter_create(yy_prof_db_t db) {
    yy_prof_storage_mdbm_iter_t *iter = (yy_prof_db_iter_t) emalloc(sizeof(yy_prof_storage_mdbm_iter_t));
    iter->db = db;
    return iter;
}

void yy_prof_storage_mdbm_iter_destroy(yy_prof_db_iter_t iter) {
    efree(iter);
}

void yy_prof_storage_mdbm_iter_reset(yy_prof_db_iter_t iter) {
    yy_prof_storage_mdbm_iter_t *it = iter;
    memset(&it->kv, 0, sizeof(kvpair));
    MDBM_ITER_INIT(&it->iter);

    it->kv = mdbm_first_r(it->db, &it->iter);
}

unsigned char yy_prof_storage_mdbm_iter_valid(yy_prof_db_iter_t iter) {
    yy_prof_storage_mdbm_iter_t *it = iter;
    return it->iter.m_next == -1 ? 0 : 1;
}

void yy_prof_storage_mdbm_iter_next(yy_prof_db_iter_t iter) {
    yy_prof_storage_mdbm_iter_t *it = iter;
    it->kv = mdbm_next_r(it->db, &it->iter);
}

const char *yy_prof_storage_mdbm_iter_key(yy_prof_db_iter_t iter, size_t *klen) {
    yy_prof_storage_mdbm_iter_t *it = iter;
    *klen = it->kv.key.dsize;
    return it->kv.key.dptr;
}

const char *yy_prof_storage_mdbm_iter_value(yy_prof_db_iter_t iter, size_t *vlen) {
    yy_prof_storage_mdbm_iter_t *it = iter;
    *vlen = it->kv.val.dsize;
    return it->kv.val.dptr;
}

yy_prof_storage_t storage_mdbm = {
    "mdbm",
    FLAG_INPLACE,
    yy_prof_storage_mdbm_open,
    yy_prof_storage_mdbm_close,
    yy_prof_storage_mdbm_clear,

    yy_prof_storage_mdbm_set,
    yy_prof_storage_mdbm_get,
    yy_prof_storage_mdbm_free,
    yy_prof_storage_mdbm_try_free,
    yy_prof_storage_mdbm_sets,

    yy_prof_storage_mdbm_iter_create,
    yy_prof_storage_mdbm_iter_destroy,
    yy_prof_storage_mdbm_iter_reset,
    yy_prof_storage_mdbm_iter_valid,
    yy_prof_storage_mdbm_iter_next,
    yy_prof_storage_mdbm_iter_key,
    yy_prof_storage_mdbm_iter_value
};
