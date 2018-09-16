#ifndef __YY_PROF_STRUCTS_H__
#define __YY_PROF_STRUCTS_H__

#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"


#define FUNC_HASH_COUNTERS_SIZE         256 
// include last null byte
#define YY_PROF_MAX_KEY_LEN             128
#define YY_PROF_MAX_FINGERPRINT_LEN    1024
#define YY_PROF_MAX_PROF_FUNC           256
// ((YY_PROF_MAX_PROF_FUNC + 7) / 8)
#define YY_PROF_PROF_FUNC_SIZE           32 
#define ROOT_SYMBOL                "main()"
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))


#ifdef __FreeBSD__
# if __FreeBSD_version >= 700110
#   include <sys/resource.h>
#   include <sys/cpuset.h>
#   define cpu_set_t cpuset_t
#   define SET_AFFINITY(pid, size, mask) \
    cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_TID, -1, size, mask)
#   define GET_AFFINITY(pid, size, mask) \
    cpuset_getaffinity(CPU_LEVEL_WHICH, CPU_WHICH_TID, -1, size, mask)
# else
#   error "This version of FreeBSD does not support cpusets"
# endif /* __FreeBSD_version */
#elif __APPLE__
/*
 * Patch for compiling in Mac OS X Leopard
 * @author Svilen Spasov <s.spasov@gmail.com>
 */
#    include <mach/mach_init.h>
#    include <mach/thread_policy.h>
#    define cpu_set_t thread_affinity_policy_data_t
#    define CPU_SET(cpu_id, new_mask) \
    (*(new_mask)).affinity_tag = (cpu_id + 1)
#    define CPU_ZERO(new_mask)                 \
    (*(new_mask)).affinity_tag = THREAD_AFFINITY_TAG_NULL
#   define SET_AFFINITY(pid, size, mask)       \
    thread_policy_set(mach_thread_self(), THREAD_AFFINITY_POLICY, mask, \
            THREAD_AFFINITY_POLICY_COUNT)
#else
/* For sched_getaffinity, sched_setaffinity */
# include <sched.h>
# define SET_AFFINITY(pid, size, mask) sched_setaffinity(0, size, mask)
# define GET_AFFINITY(pid, size, mask) sched_getaffinity(0, size, mask)
#endif /* __FreeBSD__ */

typedef enum {
    IO_TYPE_DEFAULT = 0,    // default
    IO_TYPE_URL,            // url, yar
    IO_TYPE_SQL,            // mysqli, PDO
    IO_TYPE_QUEUE,          // rabbitmq
    IO_TYPE_CACHE,          // Redis
    IO_TYPE_MONGODB,        // MongoDB
    IO_TYPE_SELF_CLI = 98,  // script
    IO_TYPE_SELF_URL = 99   // url
} yy_prof_io_type_t;

typedef enum {
    ADAPTER_TYPE_EQUALS = 1,
    ADAPTER_TYPE_STARTS_WITH = 2,
    ADAPTER_TYPE_REQUEST_DETECTOR = 3
} yy_prof_adapter_type_t;

typedef enum {
    FLAG_SQL_PREPARE = 0x01,
    FLAG_SKIP_STAT   = 0x02,
    FLAG_CURL        = 0x04,
    FLAG_USER_DEFINE = 0x08
} yy_prof_flag_t;

typedef enum {
    FLAG_COPY = 1,
    FLAG_INPLACE = 2
} yy_prof_storage_flag_t;
/*
typedef enum {
    LOG_EMERG = 0,
    LOG_ALERT = 1,
    LOG_CRIT = 2,
    LOG_ERR = 3,
    LOG_WARNING = 4,
    LOG_NOTICE = 5,
    LOG_INFO = 6, 
    LOG_DEBUG = 7
} yy_prof_log_level_t;
*/

// @deprected
static const long FLAG_FORCE_BEGIN = 0x01;
static const long FLAG_FORCE_END   = 0x02;

static const long FLAG_APPEND = 0x04;
static const long FLAG_REPLACE = 0x08;

static const long FLAG_REPLACE_URI = 0x01;

static const long TIME_UNIT_SEC = 1000000;
static const long TIME_UNIT_MS  = 1000;
static const long TIME_UNIT_US  = 1;

static const double SEC_TO_USEC = 1 / 1000000;
static const double USEC_TO_SEC = 1000000;
static const double MSEC_TO_USEC = 1000;


#define ZSTR_FREE(str) \
    if((str) != NULL) { \
        zend_string_free(str); \
        (str) = NULL; \
    }

#define ZSTR_RELEASE(str) \
    if((str) != NULL) { \
        zend_string_release(str); \
        (str) = NULL; \
    }

#define ZSTR_FROM_ZSTR(dest, src) \
    GC_REFCOUNT(dest) = 1; \
    GC_TYPE_INFO(dest) = IS_STRING; \
    ZSTR_LEN(dest) = ZSTR_LEN(src); \
    memcpy(ZSTR_VAL(dest), ZSTR_VAL(src), ZSTR_LEN(src)); \
    ZSTR_VAL(dest)[ZSTR_LEN(src)] = '\0'; \
    ZSTR_H(dest) = ZSTR_HASH(src);

#define ZSTR_TRUNC(str, len) \
    (ZSTR_LEN(str) > (len)) ? zend_string_init(ZSTR_VAL(str), (len), 0) : zend_string_copy(str)
    

typedef struct yy_prof_entry_t yy_prof_entry_t;
typedef struct yy_prof_funcs_t yy_prof_funcs_t;
typedef void* yy_prof_db_t;
typedef void* yy_prof_db_iter_t;

typedef struct yy_prof_adapter_define_t {
    char * func;
    uint8_t type;
    uint8_t flag;
    void * callback;
} yy_prof_adapter_define_t;

typedef struct yy_prof_adapter_t {
    zend_string *func;
    uint8_t type;
    uint8_t flag;
    void (* callback)(yy_prof_entry_t * TSRMLS_DC); 
} yy_prof_adapter_t;

typedef struct yy_prof_cpu_t {
    uint32_t cur_cpu_id;
    uint32_t cpu_num;
    uint32_t av_cpu_num;
    double * cpu_frequencies;
    short  * av_cpus;

    cpu_set_t prev_mask;
} yy_prof_cpu_t;

typedef struct yy_prof_slow_log_t {
    int day;
    zend_string * file;
    FILE *fd;
} yy_prof_slow_log_t;

// 单个页面汇总信息
typedef struct yy_prof_page_stat_t {
    uint64_t request_count;
    uint64_t request_time;   // ms
    uint64_t url_count;
    uint64_t url_time;       // ms
    uint64_t sql_count;
    uint64_t sql_time;       // ms
    uint64_t queue_count;
    uint64_t queue_time;     // ms
    uint64_t cache_count;
    uint64_t cache_time;     // ms
    uint64_t mongodb_count;
    uint64_t mongodb_time;   // ms
    uint64_t default_count;
    uint64_t default_time;   // ms
} yy_prof_page_stat_t;

// 单次函数调用汇总信息
typedef struct yy_prof_func_stat_t {
    yy_prof_io_type_t type;
    uint64_t count;
    uint64_t time;
    uint64_t request_bytes;
    uint64_t response_bytes;
    uint64_t status_200;
    uint64_t status_300;
    uint64_t status_400;
    uint64_t status_500;
    uint64_t status_501;
} yy_prof_func_stat_t;

typedef struct yy_prof_request_t {
    uint8_t is_cli;
    zend_string *stat_key;  // "uri#tag"
    zend_string *uri;
    zend_string *uri2; // request detector
    zend_string *tag;
    zend_string *query; // last mysqli::prepare
    zend_string *uuid;
    uint64_t tsc_start;
    uint64_t tsc_end;
    uint64_t time;
} yy_prof_request_t;

typedef struct yy_prof_request_detector_t {
    zend_string * func;
    uint64_t flag;
    void * get_key;
} yy_prof_request_detector_t;

typedef struct yy_prof_stat_db_t {
    void * page_db;
    void * func_db;
    void * trace_db;
} yy_prof_stat_db_t;

typedef struct yy_prof_page_func_stat_t {
    union {
        zend_string s;
        struct {
#if PHP_VERSION_ID >= 70000
	        zend_refcounted_h gc; 
#endif
            uint64_t h;
            size_t   len;
            char     val[YY_PROF_MAX_KEY_LEN];
	    } m;
    } key;
    yy_prof_io_type_t type;
    uint64_t count;
    uint64_t time;
} yy_prof_page_func_stat_t;

struct yy_prof_entry_t {
    zend_string *func;
    zend_string *key;
    yy_prof_request_t *request;
    yy_prof_adapter_t *adapter;
    long status;
    long request_bytes;
    long response_bytes;
    yy_prof_io_type_t type;
    uint64_t tsc_start;
    uint64_t tsc_end;
    uint64_t time; // ms
    struct yy_prof_entry_t * prev;
    void * ptr;  // 保存临时数据
};

struct yy_prof_funcs_t {
    uint8_t filter[YY_PROF_PROF_FUNC_SIZE];
    yy_prof_adapter_t * adapters;
    size_t adapters_len;
    HashTable ht;
    zend_bool is_changed;
};

#endif
