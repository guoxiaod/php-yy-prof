#ifndef __YY_PROF_STORAGE_H__
#define __YY_PROF_STORAGE_H__

#include "include/yy_structs.h"

typedef struct _yy_prof_storage {
    const char * name;
    yy_prof_storage_flag_t flags;
    yy_prof_db_t (*open)(const char* file, char ** errptr);
    void (*close)(yy_prof_db_t db);
    void (*clear)(yy_prof_db_t db);

    void (*set)(yy_prof_db_t db, const char *key, size_t klen, const char *val, size_t vlen, char ** errptr);
    void (*get)(yy_prof_db_t db, const char *key, size_t klen, char **val, size_t *vlen, char ** errptr);
    void (*free)(void *);
    void (*try_free)(void *);

    void (*sets)(yy_prof_db_t db);

    yy_prof_db_iter_t (*iter_create)(yy_prof_db_t db);
    void (*iter_destroy)(yy_prof_db_iter_t iter);
    void (*iter_reset)(yy_prof_db_iter_t iter);
    unsigned char (*iter_valid)(yy_prof_db_iter_t iter);
    void (*iter_next)(yy_prof_db_iter_t iter);
    const char *(*iter_key)(yy_prof_db_iter_t iter, size_t *klen);
    const char *(*iter_value)(yy_prof_db_iter_t iter, size_t *vlen);
} yy_prof_storage_t; 

extern yy_prof_storage_t storage_leveldb;
extern yy_prof_storage_t storage_mdbm;

YY_PROF_STARTUP_FUNCTION(storage);
YY_PROF_ACTIVATE_FUNCTION(storage);
YY_PROF_SHUTDOWN_FUNCTION(storage);

PHP_YY_PROF_API yy_prof_storage_t * php_yy_prof_storage_get(char *name, size_t len);
PHP_YY_PROF_API int php_yy_prof_storage_register(yy_prof_storage_t * storage);

#endif
