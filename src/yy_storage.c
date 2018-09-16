#ifndef HAVE_CONFIG_H
#include "config.h"
#endif


#include "php.h"
#include "php_yy_prof.h"
#include "include/yy_storage.h"

struct _yy_prof_storage_list {
    unsigned int size;
    unsigned int capacity;
    yy_prof_storage_t ** data;
} yy_prof_storage_list;

extern yy_prof_storage_t storage_leveldb;
extern yy_prof_storage_t storage_mdbm;

PHP_YY_PROF_API yy_prof_storage_t * php_yy_prof_storage_get(char *name, size_t len) {
    int i = 0;
    for (i = 0; i < yy_prof_storage_list.size; i ++) {
        if (strncmp(yy_prof_storage_list.data[i]->name, name, len) == 0) {
            return yy_prof_storage_list.data[i];
        }
    }
    return NULL;
}
PHP_YY_PROF_API int php_yy_prof_storage_register(yy_prof_storage_t * storage) {
    if (yy_prof_storage_list.capacity == 0) {
        yy_prof_storage_list.capacity += 5;
        yy_prof_storage_list.data = 
            (yy_prof_storage_t **) malloc(sizeof(yy_prof_storage_t*) * yy_prof_storage_list.capacity);
    } else if (yy_prof_storage_list.capacity == yy_prof_storage_list.size) {
        yy_prof_storage_list.capacity += 5;
        yy_prof_storage_list.data = 
            (yy_prof_storage_t **) realloc(yy_prof_storage_list.data, sizeof(yy_prof_storage_t*) * yy_prof_storage_list.capacity);
    }
    yy_prof_storage_list.data[yy_prof_storage_list.size] = storage;

    return yy_prof_storage_list.size ++;
}

YY_PROF_STARTUP_FUNCTION(storage) {
    php_yy_prof_storage_register(&storage_leveldb);
    php_yy_prof_storage_register(&storage_mdbm);
    return SUCCESS;
}
YY_PROF_ACTIVATE_FUNCTION(storage) {
    yy_prof_storage_t * storage = php_yy_prof_storage_get(YY_PROF_G(default_storage), strlen(YY_PROF_G(default_storage)));
    if (storage) {
        YY_PROF_G(storage) = storage;
        return SUCCESS;
    }
    YY_PROF_G(storage) = &storage_leveldb; 
    return SUCCESS;
}
YY_PROF_SHUTDOWN_FUNCTION(storage) {
    if(yy_prof_storage_list.capacity) {
        free(yy_prof_storage_list.data);
    }
    return SUCCESS;
}

