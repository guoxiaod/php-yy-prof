#ifndef __YY_PROF_ADAPTER__H__
#define __YY_PROF_ADAPTER__H__

#include "include/yy_structs.h"
#include "Zend/zend_smart_str.h"

zend_string * yy_prof_get_current_uri(yy_prof_request_t *request);
smart_str * yy_prof_func_get_time_stats(yy_prof_entry_t *entry, smart_str *str);
smart_str * yy_prof_func_get_origin_key(yy_prof_entry_t *entry, smart_str *str);
void yy_prof_set_entry_status(yy_prof_entry_t *entry);
static zend_always_inline zend_ulong yy_prof_compute_hash(zend_string *str);
static zend_always_inline yy_prof_adapter_t *yy_prof_is_tracking_func(yy_prof_funcs_t *fcs, zend_string *func);

void yy_prof_init_default_funcs_ex(yy_prof_funcs_t * funcs);
void yy_prof_clear_default_funcs_ex(yy_prof_funcs_t * funcs);

void yy_prof_free_request_detector(yy_prof_request_detector_t * detector);
void yy_prof_set_funcs_ex(yy_prof_funcs_t * funcs,
        yy_prof_funcs_t * default_funcs, zval * values, long flag);
void yy_prof_reset_funcs_ex(yy_prof_funcs_t * funcs, yy_prof_funcs_t * default_funcs);
void yy_prof_clear_funcs_ex(yy_prof_funcs_t * funcs);
static zend_always_inline yy_prof_adapter_t *
yy_prof_is_tracking_func_ex(yy_prof_funcs_t * fcs, zend_string* func);
    
#define YY_PROF_IS_YAR(str) \
    (strncmp((str), "Yar_Client::", sizeof("Yar_Client::") - 1) == 0)

#define YY_PROF_IS_REDIS(str) \
    (strncmp((str), "Redis::", sizeof("Redis::") - 1) == 0)

#define YY_PROF_IS_AEROSPIKE(str) \
    (strncmp((str), "Aerospike::", sizeof("Aerospike::") - 1) == 0)

#define YY_PROF_IS_PREDEFINE_FUNC(str) \
    (YY_PROF_IS_YAR(str) || YY_PROF_IS_REDIS(str) || YY_PROF_IS_AEROSPIKE(str))

#define YY_PROF_IS_SKIP_FUNC(str) \
    ( \
        strncmp((str), "Yar_Client::__", sizeof("Yar_Client::__") - 1) == 0 \
        || strncmp((str), "Yar_Client::setOpt", sizeof("Yar_Client::setOpt")) == 0 \
        || strncmp((str), "Redis::__", sizeof("Redis::__") - 1) == 0 \
        || strncmp((str), "Aerospike::__", sizeof("Aerospike::__") - 1) == 0 \
        || strncmp((str), "Aerospike::error", sizeof("Aerospike::error") - 1) == 0 \
        || strncmp((str), "Aerospike::setLog", sizeof("Aerospike::setLog") - 1) == 0 \
        || strncmp((str), "Aerospike::isConnected", sizeof("Aerospike::isConnected") - 1) == 0 \
        || strncmp((str), "Aerospike::initKey", sizeof("Aerospike::initKey") - 1) == 0 \
    )

/* Bloom filter for function names to be ignored */
#define INDEX_2_BYTE(index)  (index >> 3)
#define INDEX_2_BIT(index)   (1 << (index & 0x7))


static zend_always_inline zend_ulong 
yy_prof_compute_hash(zend_string* str) {
    if(YY_PROF_IS_PREDEFINE_FUNC(ZSTR_VAL(str))) {
        ZSTR_H(str) = 1;
    }
    return ZSTR_HASH(str);
}

// @param ret -1: not exists, >= 0: adapter index
static zend_always_inline yy_prof_adapter_t *
yy_prof_is_tracking_func(yy_prof_funcs_t * fcs, zend_string* func) {
    int ret = -1, res; 
    uint8_t mask; 
    uint8_t idx; 
    uint8_t hash = (uint8_t) yy_prof_compute_hash(func);
    int min, max, mid; 
    yy_prof_adapter_t * adapter; 
    mask = INDEX_2_BIT(hash); 
    idx = INDEX_2_BYTE(hash); 
    if(fcs->filter[idx] & mask) { 
        min = 1; 
        max = fcs->adapters_len - 1; 
        while (min <= max) { 
            mid = (min + max) / 2; 
            adapter = fcs->adapters + mid; 
            switch(adapter->type) { 
                case ADAPTER_TYPE_EQUALS: 
                    res = strcmp(ZSTR_VAL(adapter->func), ZSTR_VAL(func)); 
                    break; 
                case ADAPTER_TYPE_STARTS_WITH: 
                default: 
                    if(YY_PROF_IS_SKIP_FUNC(ZSTR_VAL(func))) { 
                        ret = -1; 
                        res = 1; 
                        mid = 0; 
                    } else { 
                        res = strncmp(ZSTR_VAL(adapter->func), ZSTR_VAL(func), ZSTR_LEN(adapter->func)); 
                    } 
                    break;  
            } 
            if(res > 0) { 
                max = mid - 1; 
            } else if(res < 0) {
                min = mid + 1; 
            } else {
                ret = mid; 
                break; 
            } 
        }; 
    } 
    return ret == -1 ? NULL : fcs->adapters + ret;
}

static zend_always_inline yy_prof_adapter_t *
yy_prof_is_tracking_func_ex(yy_prof_funcs_t * fcs, zend_string * func) {
    int len = 0, origin_len = 0;
    int is_pre_defined = 0;
    zend_ulong hash = 0, origin_hash = 0;
    yy_prof_adapter_t * adapter = NULL;

    is_pre_defined = YY_PROF_IS_PREDEFINE_FUNC(ZSTR_VAL(func));
    if(is_pre_defined && YY_PROF_IS_SKIP_FUNC(ZSTR_VAL(func))) {
        return NULL;
    }
    if(is_pre_defined) {
        len = strchr(ZSTR_VAL(func), ':') - ZSTR_VAL(func) + 2;
        hash = zend_inline_hash_func(ZSTR_VAL(func), len);
    } else {
        len = ZSTR_LEN(func);
        hash = ZSTR_HASH(func);
    }

    origin_len = ZSTR_LEN(func);
    origin_hash = ZSTR_H(func);
    ZSTR_H(func) = hash;
    ZSTR_LEN(func) = len;
    adapter = zend_hash_find_ptr(&fcs->ht, (zend_string*) func);
    ZSTR_H(func) = origin_hash;
    ZSTR_LEN(func) = origin_len;

    return adapter;
}
#endif
