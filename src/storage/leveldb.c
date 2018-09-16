#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_yy_prof.h"
#include <leveldb/c.h>

#include "include/yy_storage.h"

yy_prof_storage_t storage_leveldb;

const size_t WRITE_BUFFER_SIZE = 1024 * 1024 * 16;

yy_prof_db_t yy_prof_storage_leveldb_open(const char * file, char ** errptr) {
    leveldb_options_t * options = leveldb_options_create();
    leveldb_options_set_create_if_missing(options, 1);
    leveldb_options_set_write_buffer_size(options, WRITE_BUFFER_SIZE); 
    leveldb_options_set_block_size(options, 1024 * 16);
    leveldb_options_set_compression(options, leveldb_no_compression);

    leveldb_t * db = leveldb_open(options, file, errptr);
    leveldb_options_destroy(options);
    return db;
}

void yy_prof_storage_leveldb_close(yy_prof_db_t db) {
    leveldb_close(db);
}

void yy_prof_storage_leveldb_clear(yy_prof_db_t db) {
    // TODO
}

void yy_prof_storage_leveldb_set(
        yy_prof_db_t db, 
        const char *key, 
        size_t klen,
        const char *val, 
        size_t vlen, 
        char **errptr) {
    leveldb_writeoptions_t * options = leveldb_writeoptions_create();
    leveldb_writeoptions_set_sync(options, 1);
    leveldb_put(db, options, key, klen, val, vlen, errptr);
    leveldb_writeoptions_destroy(options);
}

void yy_prof_storage_leveldb_get(
        yy_prof_db_t db, 
        const char *key, 
        size_t klen, 
        char **val, 
        size_t *vlen, 
        char **errptr) {
    leveldb_readoptions_t *options = leveldb_readoptions_create();
    *val = leveldb_get(db, options, key, klen, vlen, errptr);
    leveldb_readoptions_destroy(options);
}
void yy_prof_storage_leveldb_free(void *ptr) {
    leveldb_free(ptr);
}

void yy_prof_storage_leveldb_try_free(void *ptr) {
    if (ptr != NULL) {
        leveldb_free(ptr);
    }
}

void yy_prof_storage_leveldb_sets(yy_prof_db_t db) {

}

yy_prof_db_iter_t yy_prof_storage_leveldb_iter_create(yy_prof_db_t db) {
    leveldb_readoptions_t *options = leveldb_readoptions_create();
    leveldb_iterator_t * iter = leveldb_create_iterator(db, options);
    leveldb_readoptions_destroy(options);
    leveldb_iter_seek_to_first(iter);
    return iter;
}

void yy_prof_storage_leveldb_iter_destroy(yy_prof_db_iter_t iter) {
    leveldb_iter_destroy(iter);
}

void yy_prof_storage_leveldb_iter_reset(yy_prof_db_iter_t iter) {
    leveldb_iter_seek_to_first(iter);
}

unsigned char yy_prof_storage_leveldb_iter_valid(yy_prof_db_iter_t iter) {
    return leveldb_iter_valid(iter);
}

void yy_prof_storage_leveldb_iter_next(yy_prof_db_iter_t iter) {
    leveldb_iter_next(iter);
}

const char *yy_prof_storage_leveldb_iter_key(yy_prof_db_iter_t iter, size_t *klen) {
    return leveldb_iter_key(iter, klen);
}

const char *yy_prof_storage_leveldb_iter_value(yy_prof_db_iter_t iter, size_t *vlen) {
    return leveldb_iter_value(iter, vlen);
}

yy_prof_storage_t storage_leveldb = {
    "leveldb",
    FLAG_COPY,
    yy_prof_storage_leveldb_open,
    yy_prof_storage_leveldb_close,
    yy_prof_storage_leveldb_clear,

    yy_prof_storage_leveldb_set,
    yy_prof_storage_leveldb_get,
    yy_prof_storage_leveldb_free,
    yy_prof_storage_leveldb_try_free,
    yy_prof_storage_leveldb_sets,

    yy_prof_storage_leveldb_iter_create,
    yy_prof_storage_leveldb_iter_destroy,
    yy_prof_storage_leveldb_iter_reset,
    yy_prof_storage_leveldb_iter_valid,
    yy_prof_storage_leveldb_iter_next,
    yy_prof_storage_leveldb_iter_key,
    yy_prof_storage_leveldb_iter_value
};
