#ifndef PHP_YY_PROF_H
#define PHP_YY_PROF_H

extern zend_module_entry yy_prof_module_entry;
#define phpext_yy_prof_ptr &yy_prof_module_entry

#define PHP_YY_PROF_VERSION "0.1.0" /* Replace with version number for your extension */

#ifdef PHP_WIN32
#	define PHP_YY_PROF_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_YY_PROF_API __attribute__ ((visibility("default")))
#else
#	define PHP_YY_PROF_API
#endif

#include "include/yy_structs.h"

#ifdef ZTS
#include "TSRM.h"
#endif

#define SCRATCH_BUF_LEN                512
#define SLOW_RUN_TIME              1000000
#define DEFAULT_PROTECT_LOAD             3
#define DEFAULT_PROTECT_RESTORE_LOAD     2
#define DEFAULT_PROTECT_CHECK_INTERVAL  10

#define STATE_OK 0
#define STATE_PROTECT 1

/* 
   Declare any global variables you may need between the BEGIN
   and END macros here:     
   */
ZEND_BEGIN_MODULE_GLOBALS(yy_prof)
    // global configuration
    zend_bool auto_enable;
    zend_bool enable_trace;
    zend_bool enable_trace_cli;
    zend_bool enable_request_detector;
    long slow_run_time; // ms
    long log_level;

    double protect_load;
    double protect_restore_load;
    long protect_check_interval;

    // runtime data
    long protect_last_state;
    zend_bool enabled;
    zend_bool profiling;
    zend_bool ever_enabled;
    zend_bool enabled_by_user;
    zend_bool started_by_detector;
    zend_bool started_by_user;

    // module lifecycle
    yy_prof_cpu_t cpu;
    yy_prof_funcs_t default_funcs;
    yy_prof_stat_db_t db;

    char *default_storage;
    struct _yy_prof_storage *storage;

    yy_prof_entry_t * free_entries;
    yy_prof_request_detector_t detector;
    yy_prof_slow_log_t log;

    // request lifecycle
    yy_prof_entry_t * request_entries;
    yy_prof_entry_t * entries;
    yy_prof_request_t request;
    yy_prof_page_stat_t stat;
    yy_prof_funcs_t funcs;
ZEND_END_MODULE_GLOBALS(yy_prof)

/* Always refer to the globals in your function as YY_PROF_G(variable).
   You are encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/
#define YY_PROF_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(yy_prof, v)

#if defined(ZTS) && defined(COMPILE_DL_YY_PROF)
ZEND_TSRMLS_CACHE_EXTERN()
#endif


PHP_MINIT_FUNCTION(yy_prof);
PHP_MSHUTDOWN_FUNCTION(yy_prof);
PHP_RINIT_FUNCTION(yy_prof);
PHP_RSHUTDOWN_FUNCTION(yy_prof);
PHP_MINFO_FUNCTION(yy_prof);

PHP_FUNCTION(confirm_yy_prof_compiled);    /* For testing, remove later. */

PHP_FUNCTION(yy_prof_set_funcs);
PHP_FUNCTION(yy_prof_reset_funcs);
PHP_FUNCTION(yy_prof_get_funcs);

PHP_FUNCTION(yy_prof_enable);
PHP_FUNCTION(yy_prof_disable);
PHP_FUNCTION(yy_prof_is_profiling);
PHP_FUNCTION(yy_prof_clear_request_detector);
PHP_FUNCTION(yy_prof_get_request_detector);
PHP_FUNCTION(yy_prof_get_page_result);

PHP_FUNCTION(yy_prof_get_page_list);
PHP_FUNCTION(yy_prof_get_page_stat);
PHP_FUNCTION(yy_prof_get_all_page_stat);

PHP_FUNCTION(yy_prof_get_page_func_stat);

PHP_FUNCTION(yy_prof_get_func_list);
PHP_FUNCTION(yy_prof_get_func_stat);
PHP_FUNCTION(yy_prof_get_all_func_stat);

PHP_FUNCTION(yy_prof_clear_stats);

ZEND_EXTERN_MODULE_GLOBALS(yy_prof);


#define YY_PROF_STARTUP_FUNCTION(module)    ZEND_MINIT_FUNCTION(yy_prof_##module)
#define YY_PROF_STARTUP(module)             ZEND_MODULE_STARTUP_N(yy_prof_##module)(INIT_FUNC_ARGS_PASSTHRU)
#define YY_PROF_SHUTDOWN_FUNCTION(module)   ZEND_MSHUTDOWN_FUNCTION(yy_prof_##module)
#define YY_PROF_SHUTDOWN(module)            ZEND_MODULE_SHUTDOWN_N(yy_prof_##module)(SHUTDOWN_FUNC_ARGS_PASSTHRU)
#define YY_PROF_ACTIVATE_FUNCTION(module)   ZEND_MODULE_ACTIVATE_D(yy_prof_##module)
#define YY_PROF_ACTIVATE(module)            ZEND_MODULE_ACTIVATE_N(yy_prof_##module)(INIT_FUNC_ARGS_PASSTHRU)
#define YY_PROF_DEACTIVATE_FUNCTION(module) ZEND_MODULE_DEACTIVATE_D(yy_prof_##module)
#define YY_PROF_DEACTIVATE(module)          ZEND_MODULE_DEACTIVATE_N(yy_prof_##module)(SHUTDOWN_FUNC_ARGS_PASSTHRU)


#endif    /* PHP_YY_PROF_H */
