
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdint.h>
#include <limits.h>
#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "main/SAPI.h"
#include "php_yy_prof.h"

#include "include/yy_structs.h"
#include "include/yy_storage.h"
#include "include/yy_time.h"
#include "include/yy_api.h"
#include "include/yy_func.h"
#include "include/yy_log.h"
#include "include/yy_stat.h"
#include "include/yy_adapter.h"

/* If you declare any globals in php_yy_prof.h uncomment this: */
ZEND_DECLARE_MODULE_GLOBALS(yy_prof)

#define CHECK_TIME_UNIT(tu) \
	if((tu) != TIME_UNIT_US && (tu) != TIME_UNIT_MS && (tu) != TIME_UNIT_SEC) { \
		php_error(E_WARNING, "Invalid time_unit %d, change to default value %d", (tu), TIME_UNIT_US); \
		(tu) = TIME_UNIT_US; \
	}

/* True global resources - no need for thread safety here */
static int le_yy_prof;

/* {{{ yy_prof_functions[]
 *
 * Every user visible function must have an entry in yy_prof_functions[].
 */
const zend_function_entry yy_prof_functions[] = {
    PHP_FE(confirm_yy_prof_compiled,  NULL)        /* For testing, remove later. */
    PHP_FE(yy_prof_set_funcs,         NULL)
    PHP_FE(yy_prof_reset_funcs,       NULL)
    PHP_FE(yy_prof_get_funcs,         NULL)
    PHP_FE(yy_prof_enable,            NULL)
    PHP_FE(yy_prof_disable,           NULL)
    PHP_FE(yy_prof_is_profiling,      NULL)
    PHP_FE(yy_prof_clear_request_detector,NULL)
    PHP_FE(yy_prof_get_request_detector,NULL)
    PHP_FE(yy_prof_get_page_list,     NULL)
    PHP_FE(yy_prof_get_page_stat,     NULL)
    PHP_FE(yy_prof_get_page_func_stat,NULL)
    PHP_FE(yy_prof_get_all_page_stat, NULL)
    PHP_FE(yy_prof_get_page_result,   NULL)
    PHP_FE(yy_prof_get_func_list,     NULL)
    PHP_FE(yy_prof_get_func_stat,     NULL)
    PHP_FE(yy_prof_get_all_func_stat, NULL)
    PHP_FE(yy_prof_clear_stats,       NULL)
    PHP_FE_END    /* Must be the last line in yy_prof_functions[] */
};
/* }}} */

static const zend_module_dep yy_prof_deps[] = {
    ZEND_MOD_END
};

/* {{{ yy_prof_module_entry
*/
zend_module_entry yy_prof_module_entry = {
    STANDARD_MODULE_HEADER_EX, NULL,
    yy_prof_deps,
    "yy_prof",
    yy_prof_functions,
    PHP_MINIT(yy_prof),
    PHP_MSHUTDOWN(yy_prof),
    PHP_RINIT(yy_prof),        /* Replace with NULL if there's nothing to do at request start */
    PHP_RSHUTDOWN(yy_prof),    /* Replace with NULL if there's nothing to do at request end */
    PHP_MINFO(yy_prof),
    PHP_YY_PROF_VERSION,
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_YY_PROF
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(yy_prof)
#endif

#if PHP_VERSION_ID < 50500
/* Pointer to the original execute function */
ZEND_DLEXPORT void (*_zend_execute) (zend_op_array *ops);

/* Pointer to the origianl execute_internal function */
ZEND_DLEXPORT void (*_zend_execute_internal) (zend_execute_data *data, int ret);
#else
/* Pointer to the original execute function */
static void (*_zend_execute_ex) (zend_execute_data *execute_data);

/* Pointer to the origianl execute_internal function */
static void (*_zend_execute_internal) (zend_execute_data *data, zval *return_value);
#endif

/* Pointer to the original compile function */
static zend_op_array *(*_zend_compile_file) (zend_file_handle *file_handle, int type);

/* Pointer to the original compile string function (used by eval) */
static zend_op_array * (*_zend_compile_string) (zval *source_string, char *filename);

ZEND_DLEXPORT zend_op_array*
yy_prof_compile_file(zend_file_handle *file_handle, int type) ;

ZEND_DLEXPORT zend_op_array*
yy_prof_compile_string(zval *source_string, char *filename);

static void yy_prof_enable(TSRMLS_D);
static void yy_prof_disable();

static void yy_prof_start();
static void yy_prof_stop();

static void yy_prof_save();

static zend_always_inline void 
yy_prof_mode_common_beginfn(yy_prof_entry_t **entries, yy_prof_entry_t  *current);

static zend_always_inline void 
yy_prof_mode_common_endfn(yy_prof_entry_t **entries, yy_prof_entry_t *current);

static zend_always_inline double yy_prof_get_load(long interval);
static zend_always_inline int yy_prof_check_load();

static ZEND_INI_MH(OnUpdateTime) /* {{{ */
{
    zend_long *p; 
#ifndef ZTS
    char *base = (char *) mh_arg2;
#else
    char *base;

    base = (char *) ts_resource(*((int *) mh_arg2));
#endif

    p = (zend_long *) (base+(size_t) mh_arg1);

    *p = zend_atol(ZSTR_VAL(new_value), (int)ZSTR_LEN(new_value)) * MSEC_TO_USEC;
    return SUCCESS;
}
/* }}} */


/* {{{ PHP_INI
*/
/* Remove comments and fill if you need to have entries in php.ini */
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("yy_prof.auto_enable", "0", PHP_INI_ALL,
        OnUpdateBool, auto_enable, zend_yy_prof_globals, yy_prof_globals)
    STD_PHP_INI_ENTRY("yy_prof.enable_trace", "0", PHP_INI_ALL,
        OnUpdateBool, enable_trace, zend_yy_prof_globals, yy_prof_globals)
    STD_PHP_INI_ENTRY("yy_prof.enable_trace_cli", "0", PHP_INI_ALL,
        OnUpdateBool, enable_trace_cli, zend_yy_prof_globals, yy_prof_globals)
    STD_PHP_INI_ENTRY("yy_prof.enable_request_detector", "0", PHP_INI_ALL,
        OnUpdateBool, enable_request_detector, zend_yy_prof_globals, yy_prof_globals)
    STD_PHP_INI_ENTRY("yy_prof.slow_run_time", "1000000", PHP_INI_ALL,
        OnUpdateTime, slow_run_time, zend_yy_prof_globals, yy_prof_globals)
    STD_PHP_INI_ENTRY("yy_prof.protect_load", "3", PHP_INI_ALL,
        OnUpdateReal, protect_load, zend_yy_prof_globals, yy_prof_globals)
    STD_PHP_INI_ENTRY("yy_prof.protect_restore_load", "2", PHP_INI_ALL,
        OnUpdateReal, protect_restore_load, zend_yy_prof_globals, yy_prof_globals)
    STD_PHP_INI_ENTRY("yy_prof.protect_check_interval", "10", PHP_INI_ALL,
        OnUpdateLong, protect_check_interval, zend_yy_prof_globals, yy_prof_globals)
    STD_PHP_INI_ENTRY("yy_prof.log_level", "0", PHP_INI_ALL,
        OnUpdateLong, log_level, zend_yy_prof_globals, yy_prof_globals)
    STD_PHP_INI_ENTRY("yy_prof.default_storage", "mdbm", PHP_INI_ALL,
        OnUpdateString, default_storage, zend_yy_prof_globals, yy_prof_globals)
PHP_INI_END()
/* }}} */

/* {{{ php_yy_prof_init_globals
*/
/* Uncomment this function if you have INI entries */
static void 
php_yy_prof_init_globals(zend_yy_prof_globals *yy_prof_globals)
{
	memset(yy_prof_globals, 0, sizeof(zend_yy_prof_globals));

    yy_prof_globals->slow_run_time = SLOW_RUN_TIME;

    yy_prof_globals->protect_load = DEFAULT_PROTECT_LOAD;
    yy_prof_globals->protect_restore_load = DEFAULT_PROTECT_RESTORE_LOAD;
    yy_prof_globals->protect_check_interval = DEFAULT_PROTECT_CHECK_INTERVAL;

	yy_prof_globals->entries = NULL;
	yy_prof_globals->free_entries = NULL;
	yy_prof_globals->request_entries = NULL;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
*/
PHP_MINIT_FUNCTION(yy_prof)
{
    /* If you have INI entries, uncomment these lines */
    ZEND_INIT_MODULE_GLOBALS(yy_prof, php_yy_prof_init_globals, NULL);

    REGISTER_INI_ENTRIES();

	// overwrite the ini config
	char * env = getenv("YY_PROF_AUTO_ENABLE");
	if(env && strcmp(env, "0") == 0) {
		YY_PROF_G(auto_enable) = 0;
	}

	// @deprecated
    REGISTER_LONG_CONSTANT("YY_PROF_FLAG_FORCE_BEGIN", FLAG_FORCE_BEGIN, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("YY_PROF_FLAG_FORCE_END",   FLAG_FORCE_END,   CONST_CS | CONST_PERSISTENT);

    REGISTER_LONG_CONSTANT("YY_PROF_FLAG_APPEND",  FLAG_APPEND,  CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("YY_PROF_FLAG_REPLACE", FLAG_REPLACE, CONST_CS | CONST_PERSISTENT);

    REGISTER_LONG_CONSTANT("YY_PROF_TIME_UNIT_MS",  TIME_UNIT_MS,  CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("YY_PROF_TIME_UNIT_US",  TIME_UNIT_US,  CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("YY_PROF_TIME_UNIT_SEC", TIME_UNIT_SEC, CONST_CS | CONST_PERSISTENT);

    REGISTER_LONG_CONSTANT("YY_PROF_STAT_VERSION", YY_PROF_STAT_VERSION, CONST_CS | CONST_PERSISTENT);

	YY_PROF_STARTUP(storage);
	YY_PROF_ACTIVATE(storage);

	// init cpu info
    init_cpu_info(&YY_PROF_G(cpu));

	// init default functions info
    yy_prof_init_default_funcs_ex(&YY_PROF_G(default_funcs));

	// open/create stat db
	yy_prof_stat_db_t * db = &YY_PROF_G(db);
    db->page_db = yy_prof_open_stat_db(YY_PROF_DB_PAGE_STAT_FILE);
    db->func_db = yy_prof_open_stat_db(YY_PROF_DB_FUNC_STAT_FILE);
    db->trace_db = yy_prof_open_stat_db(YY_PROF_DB_TRACE_FILE);

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
*/
PHP_MSHUTDOWN_FUNCTION(yy_prof)
{
    yy_prof_close_slow_log(&YY_PROF_G(log));
    yy_prof_free_free_entries(&YY_PROF_G(free_entries));

	// close stat db
    yy_prof_close_stat_db(YY_PROF_G(db).page_db);
    yy_prof_close_stat_db(YY_PROF_G(db).func_db);
    yy_prof_close_stat_db(YY_PROF_G(db).trace_db);

	// clear default functions info
    yy_prof_clear_default_funcs_ex(&YY_PROF_G(default_funcs));

	// release cpu and clear cpu info
    clear_cpu_info(&YY_PROF_G(cpu));

	YY_PROF_SHUTDOWN(storage);

    /* uncomment this line if you have INI entries */
    UNREGISTER_INI_ENTRIES();

    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
*/
PHP_RINIT_FUNCTION(yy_prof)
{
#if defined(COMPILE_DL_YY_PROF) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
    int len = 0;
    char * uuid = NULL;

    YY_PROF_G(enabled) = 0;
    YY_PROF_G(ever_enabled) = 0;
	YY_PROF_G(profiling) = 0;
	YY_PROF_G(started_by_detector) = 0;
	YY_PROF_G(started_by_user) = 0;

    // init entries
	YY_PROF_G(entries) = NULL;
	YY_PROF_G(request_entries) = NULL;

    // init request
    yy_prof_request_t * request = &YY_PROF_G(request);
    request->is_cli = strcmp(sapi_module.name, "cli") == 0;
    request->query = NULL;
    request->uri = yy_prof_get_current_uri(request);

    uuid = sapi_getenv("UNIQUE_ID", sizeof("UNIQUE_ID") - 1);
    len = uuid == NULL ? 0 : strlen(uuid);
    request->uuid = zend_string_init(uuid, len, 0);

    // init stat
    memset(&YY_PROF_G(stat), 0, sizeof(yy_prof_page_stat_t));

    // init funcs
    yy_prof_reset_funcs_ex(&YY_PROF_G(funcs), &YY_PROF_G(default_funcs));

    if(YY_PROF_G(auto_enable)) {
		yy_prof_enable();

		// protect the system
		if(yy_prof_check_load() == FAILURE) {
			return SUCCESS;
		}

		yy_prof_start();
	}

    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
*/
PHP_RSHUTDOWN_FUNCTION(yy_prof)
{
	yy_prof_stop();
    yy_prof_disable();

    yy_prof_request_t * request = &YY_PROF_G(request);

    ZSTR_RELEASE(request->uri);
    ZSTR_RELEASE(request->uri2);
    ZSTR_RELEASE(request->query);
    ZSTR_RELEASE(request->tag);
    ZSTR_RELEASE(request->stat_key);
    ZSTR_RELEASE(request->uuid);

	yy_prof_free_request_detector(&YY_PROF_G(detector));
    yy_prof_clear_funcs_ex(&YY_PROF_G(funcs));

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
*/
PHP_MINFO_FUNCTION(yy_prof)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "yy_prof support", "enabled");
    php_info_print_table_end();

    /* Remove comments if you have entries in php.ini */
    DISPLAY_INI_ENTRIES();
}
/* }}} */


/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_yy_prof_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_yy_prof_compiled)
{
	char *arg = NULL;
	size_t arg_len;
	zend_string *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	strg = strpprintf(0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "yy_prof", arg);

	RETURN_STR(strg);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and
   unfold functions in source code. See the corresponding marks just before
   function definition, where the functions purpose is also documented. Please
   follow this convention for the convenience of others editing your code.
   */

/* {{{ proto bool yy_prof_set_funcs() */
PHP_FUNCTION(yy_prof_set_funcs)
{
    zend_long  flag = FLAG_REPLACE;
    zval * z_funcs = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|l", &z_funcs, &flag) == FAILURE) {
        return;
    }

    if(z_funcs == NULL || Z_TYPE_P(z_funcs) != IS_ARRAY) {
		php_error(E_WARNING, "yy_prof_set_funcs first parameter must be an array");
		RETURN_FALSE;
	}
	yy_prof_set_funcs_ex(&YY_PROF_G(funcs), &YY_PROF_G(default_funcs), z_funcs, flag);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool yy_prof_reset_funcs() */
PHP_FUNCTION(yy_prof_reset_funcs)
{
    yy_prof_reset_funcs_ex(&YY_PROF_G(funcs), &YY_PROF_G(default_funcs));
}
/* }}} */

/* {{{ proto array yy_prof_get_funcs()
*/
PHP_FUNCTION(yy_prof_get_funcs)
{
    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    array_init(return_value);

    yy_prof_adapter_t * adapters = YY_PROF_G(funcs).adapters + 1;
    while(adapters && adapters->func != NULL) {
        add_next_index_str(return_value, adapters->func);
        adapters ++;
    }
}
/* }}} */



/* {{{ proto bool yy_prof_enable() */
PHP_FUNCTION(yy_prof_enable)
{

    zend_string * tag = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|S", &tag) == FAILURE) {
        return;
    }

	yy_prof_stop();

	if(tag != NULL) {
		YY_PROF_G(request).tag = zend_string_copy(tag);
	}

	if(!YY_PROF_G(enabled)) {
		yy_prof_enable();
		YY_PROF_G(enabled_by_user) = 1;
	}

	YY_PROF_G(started_by_user) = 1;
	yy_prof_start();
}
/* }}} */

/* {{{ proto bool yy_prof_disable() */
PHP_FUNCTION(yy_prof_disable)
{
    zend_long flag = 0;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|l", &flag) == FAILURE) {
        return;
    }

	yy_prof_stop();
	YY_PROF_G(started_by_user) = 0;

	if(YY_PROF_G(enabled_by_user)) {
		yy_prof_disable();
		YY_PROF_G(enabled_by_user) = 0;
	}
}
/* }}} */

/* {{{ proto bool yy_prof_is_profiling() */
PHP_FUNCTION(yy_prof_is_profiling)
{
	if(YY_PROF_G(profiling)) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool yy_prof_clear_request_detector() */
PHP_FUNCTION(yy_prof_clear_request_detector)
{
	yy_prof_free_request_detector(&YY_PROF_G(detector));
}
/* }}} */

/* {{{ proto bool yy_prof_get_request_detector() */
PHP_FUNCTION(yy_prof_get_request_detector)
{
	yy_prof_request_detector_t * detector = &YY_PROF_G(detector);

	if(detector->func != NULL) {
		array_init(return_value);

		add_assoc_str_ex(return_value, "func", sizeof("func") - 1, detector->func);
		add_assoc_long(return_value, "flag", detector->flag);

		return;
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto array yy_prof_get_page_list(string func)
*/
PHP_FUNCTION(yy_prof_get_page_list) {
    yy_prof_db_t db;
    zend_string * prefix  = NULL;

    int argc = ZEND_NUM_ARGS();
    zend_bool no_prefix = 0;

    if (zend_parse_parameters(argc , "|S", &prefix) == FAILURE) {
        return;
    }

    array_init(return_value);

    db = YY_PROF_G(db).page_db;
    if(db == NULL) {
        php_error(E_WARNING, "YY_PROF: can not open page stat db");
        return ;
    }

	size_t klen = 0;
	const char *key = NULL;

	yy_prof_storage_t * storage = YY_PROF_G(storage);
	yy_prof_db_iter_t iter = storage->iter_create(db);

    no_prefix = prefix == NULL || ZSTR_LEN(prefix) == 0;
	storage->iter_reset(iter);
    while(storage->iter_valid(iter)) {
		key = storage->iter_key(iter, &klen);
		if(no_prefix || strncmp(key, ZSTR_VAL(prefix), ZSTR_LEN(prefix)) == 0) {
			add_next_index_stringl(return_value, key, klen);
		}
		// storage->try_free(key);
		storage->iter_next(iter);
    }
	storage->iter_destroy(iter);
}
/* }}} */

/* {{{ proto array yy_prof_get_all_page_stat(string func)
*/
PHP_FUNCTION(yy_prof_get_all_page_stat) {
    yy_prof_db_t * db;

    zval z;
    zend_bool no_prefix = 0;
    zend_string * prefix  = NULL;
    int argc = ZEND_NUM_ARGS();
	zend_long time_unit = TIME_UNIT_US;
    yy_prof_page_stat_t * stat;

    if (zend_parse_parameters(argc , "|Sl", &prefix, &time_unit) == FAILURE) {
        return;
    }
	CHECK_TIME_UNIT(time_unit);

    array_init(return_value);

    db = YY_PROF_G(db).page_db;
    if(db == NULL) {
        php_error(E_WARNING, "YY_PROF: can not open page stat db");
        return ;
    }

	yy_prof_storage_t * storage = YY_PROF_G(storage);
	yy_prof_db_iter_t iter = storage->iter_create(db);

    no_prefix = prefix == NULL || ZSTR_LEN(prefix) == 0;
	storage->iter_reset(iter);
	while (storage->iter_valid(iter)) {
		size_t klen = 0, vlen = 0;
		const char *key = NULL, *val = NULL;
		key = storage->iter_key(iter, &klen);
		if(no_prefix || strncmp(key, ZSTR_VAL(prefix), ZSTR_LEN(prefix)) == 0) {
			array_init(&z);
			val = storage->iter_value(iter, &vlen);

			stat = (yy_prof_page_stat_t *) val;
			add_assoc_long(&z, "request_time", stat->request_time / time_unit);
			add_assoc_long(&z, "request_count", stat->request_count);
			add_assoc_long(&z, "url_count", stat->url_count);
			add_assoc_long(&z, "url_time", stat->url_time / time_unit);
			add_assoc_long(&z, "sql_count", stat->sql_count);
			add_assoc_long(&z, "sql_time", stat->sql_time / time_unit);
			add_assoc_long(&z, "queue_count", stat->queue_count);
			add_assoc_long(&z, "queue_time", stat->queue_time / time_unit);
			add_assoc_long(&z, "cache_count", stat->cache_count);
			add_assoc_long(&z, "cache_time", stat->cache_time / time_unit);
			add_assoc_long(&z, "mongodb_count", stat->mongodb_count);
			add_assoc_long(&z, "mongodb_time", stat->mongodb_time / time_unit);
			add_assoc_long(&z, "default_count", stat->default_count);
			add_assoc_long(&z, "default_time", stat->default_time / time_unit);

			add_assoc_zval_ex(return_value, key, klen, &z);

			// storage->try_free(val);
		}
		// storage->try_free(key);
		storage->iter_next(iter);
    }
	storage->iter_destroy(iter);
}
/* }}} */

/* {{{ proto array yy_prof_get_page_stat(string page)
*/
PHP_FUNCTION(yy_prof_get_page_stat)
{
    yy_prof_db_t db;
    yy_prof_page_stat_t * stat;

    int argc = ZEND_NUM_ARGS();
	zend_long time_unit = TIME_UNIT_US;
    zend_string *page = NULL;

    if (zend_parse_parameters(argc , "S|l", &page, &time_unit) == FAILURE) {
        return;
    }
	CHECK_TIME_UNIT(time_unit);

    array_init(return_value);

    db = YY_PROF_G(db).page_db;
    if(db == NULL) {
        php_error(E_WARNING, "YY_PROF: can not open page stat db");
        return ;
    }

	size_t vlen = 0;
	const char *val = NULL, *errptr = NULL;

	yy_prof_storage_t *storage = YY_PROF_G(storage);
	page = ZSTR_TRUNC(page, YY_PROF_MAX_KEY_LEN - 1);
	storage->get(db, ZSTR_VAL(page), ZSTR_LEN(page), &val, &vlen, &errptr);
	if (errptr != NULL) {
		php_error(E_WARNING, "YY_PROF: get page key '%s' fail: %s", ZSTR_VAL(page), errptr);
		storage->free(errptr);
	}

	zend_string_release(page);
    if(vlen == 0) {
        return;
    }

    stat = (yy_prof_page_stat_t*) val;
    add_assoc_long(return_value, "request_time", stat->request_time / time_unit);
    add_assoc_long(return_value, "request_count", stat->request_count);
    add_assoc_long(return_value, "url_count", stat->url_count);
    add_assoc_long(return_value, "url_time", stat->url_time / time_unit);
    add_assoc_long(return_value, "sql_count", stat->sql_count);
    add_assoc_long(return_value, "sql_time", stat->sql_time / time_unit);
    add_assoc_long(return_value, "queue_count", stat->queue_count);
    add_assoc_long(return_value, "queue_time", stat->queue_time / time_unit);
    add_assoc_long(return_value, "cache_count", stat->cache_count);
    add_assoc_long(return_value, "cache_time", stat->cache_time / time_unit);
    add_assoc_long(return_value, "mongodb_count", stat->mongodb_count);
    add_assoc_long(return_value, "mongodb_time", stat->mongodb_time / time_unit);
    add_assoc_long(return_value, "default_count", stat->default_count);
    add_assoc_long(return_value, "default_time", stat->default_time / time_unit);

	storage->try_free(val);
}
/* }}} */

/* {{{ proto array yy_prof_get_page_func_stat(string page)
*/
PHP_FUNCTION(yy_prof_get_page_func_stat)
{
    yy_prof_db_t db;
    yy_prof_page_func_stat_t * stat;

    zval z;
    int slen, sidx;
    int argc = ZEND_NUM_ARGS();
	zend_long time_unit = TIME_UNIT_US;
    zend_string *page = NULL;

    if (zend_parse_parameters(argc , "S|l", &page, &time_unit) == FAILURE) {
        return;
    }
	CHECK_TIME_UNIT(time_unit);

    array_init(return_value);

    db = YY_PROF_G(db).trace_db;
    if(db == NULL) {
        php_error(E_WARNING, "YY_PROF: can not open page func stat db");
        return ;
    }


	size_t vlen = 0;
	const char *val = NULL, *errptr = NULL;
	yy_prof_storage_t *storage = YY_PROF_G(storage);

	page = ZSTR_TRUNC(page, YY_PROF_MAX_KEY_LEN - 1);
	storage->get(db, ZSTR_VAL(page), ZSTR_LEN(page), &val, &vlen, &errptr);
	if (errptr != NULL) {
		php_error(E_WARNING, "YY_PROF: get page_func key '%s' fail: %s", ZSTR_VAL(page), errptr);
		storage->free(errptr);
	}
	zend_string_release(page);
	if (vlen == 0) {
		return;
	}

    stat = (yy_prof_page_func_stat_t*) val;
    slen = vlen / sizeof(yy_prof_page_func_stat_t);
	for(sidx = 0; sidx < slen; sidx ++) {
		array_init(&z);
		//add_assoc_stringl(z, "key", stat[sidx].key, strlen(stat[sidx].key), 1);
		add_assoc_long(&z, "type", stat[sidx].type);
		add_assoc_long(&z, "count", stat[sidx].count);
		add_assoc_long(&z, "time", stat[sidx].time / time_unit);
		add_assoc_zval_ex(return_value, ZSTR_VAL(&stat[sidx].key.s), ZSTR_LEN(&stat[sidx].key.s), &z);
	}
	storage->try_free(val);	
}
/* }}} */

/* {{{ proto array yy_prof_get_page_result()
    */
PHP_FUNCTION(yy_prof_get_page_result)
{
    zval z;
    int argc = ZEND_NUM_ARGS();
	zend_long time_unit = TIME_UNIT_US;
    yy_prof_entry_t * entry = NULL;
    if (zend_parse_parameters(argc , "|l", &time_unit) == FAILURE) {
        return;
    }
	CHECK_TIME_UNIT(time_unit);

    array_init(return_value);

    entry = YY_PROF_G(request_entries);
    while (entry != NULL) {
        array_init(&z);

        add_assoc_str_ex(&z, "func", sizeof("func") - 1, entry->func);
        add_assoc_str_ex(&z, "key",  sizeof("key") - 1,  entry->key);
        add_assoc_long(&z, "type", entry->type);
        add_assoc_long(&z, "start", entry->tsc_start);
        add_assoc_long(&z, "end", entry->tsc_end);
        add_assoc_long(&z, "time", entry->time / time_unit);

        add_next_index_zval(return_value, &z);

        entry = entry->prev;
    }
}
/* }}} */

/* {{{ proto array yy_prof_get_func_list(string func)
*/
PHP_FUNCTION(yy_prof_get_func_list) {
    yy_prof_db_t db;

    int argc = ZEND_NUM_ARGS();
    zend_bool no_prefix = 0;
    zend_string *prefix = NULL;

    if (zend_parse_parameters(argc , "|S", &prefix) == FAILURE) {
        return;
    }

    array_init(return_value);

    db = YY_PROF_G(db).func_db;

    if(db == NULL) {
        php_error(E_WARNING, "YY_PROF: can not open func stat db");
        return;
    }

	yy_prof_storage_t * storage = YY_PROF_G(storage);
	yy_prof_db_iter_t iter = storage->iter_create(db);

    no_prefix = prefix == NULL || ZSTR_LEN(prefix) == 0;
	storage->iter_reset(iter);
	while (storage->iter_valid(iter)) {
		size_t klen = 0;
		char *key = storage->iter_key(iter, &klen);
		if(no_prefix || strncmp(key, ZSTR_VAL(prefix), ZSTR_LEN(prefix)) == 0) {
			add_next_index_stringl(return_value, key, klen);
		}
		// storage->try_free(key);
		storage->iter_next(iter);
	}
	storage->iter_destroy(iter);
}
/* }}} */


/* {{{ proto array yy_prof_get_func_stat(string func)
*/
PHP_FUNCTION(yy_prof_get_func_stat)
{
    yy_prof_db_t db;
    yy_prof_func_stat_t * stat;

    int argc = ZEND_NUM_ARGS();
	zend_long time_unit = TIME_UNIT_US;
    zend_string *func = NULL;

    if (zend_parse_parameters(argc , "S|l", &func, &time_unit) == FAILURE) {
        return;
    }
	CHECK_TIME_UNIT(time_unit);

    array_init(return_value);

    db = YY_PROF_G(db).func_db;
    if(db == NULL) {
        php_error(E_WARNING, "YY_PROF: can not open func stat db");
        return;
    }


	size_t vlen = 0;
	const char *val = NULL, *errptr = NULL;
	yy_prof_storage_t *storage = YY_PROF_G(storage);

	func = ZSTR_TRUNC(func, YY_PROF_MAX_KEY_LEN - 1);
	storage->get(db, ZSTR_VAL(func), ZSTR_LEN(func), &val, &vlen, &errptr);
	if (errptr != NULL) {
		php_error(E_WARNING, "YY_PROF: get func key '%s' fail: %s", ZSTR_VAL(func), errptr);
		storage->free(errptr);
	}
	zend_string_release(func);
	if (vlen == 0) {
		return ;
	}

    stat = (yy_prof_func_stat_t*) val;
    add_assoc_long(return_value, "type", stat->type);
    add_assoc_long(return_value, "count", stat->count);
    add_assoc_long(return_value, "time", stat->time / time_unit);
    add_assoc_long(return_value, "request_bytes", stat->request_bytes);
    add_assoc_long(return_value, "response_bytes", stat->response_bytes);
    add_assoc_long(return_value, "status_200", stat->status_200);
    add_assoc_long(return_value, "status_300", stat->status_300);
    add_assoc_long(return_value, "status_400", stat->status_400);
    add_assoc_long(return_value, "status_500", stat->status_500);
    add_assoc_long(return_value, "status_501", stat->status_501);

	storage->try_free(val);
}
/* }}} */

/* {{{ proto array yy_prof_get_all_func_stat(string func)
*/
PHP_FUNCTION(yy_prof_get_all_func_stat)
{
    yy_prof_db_t db;
    yy_prof_func_stat_t * stat;

    zval z;
    zend_bool no_prefix;
    int argc = ZEND_NUM_ARGS();
	zend_long time_unit = TIME_UNIT_US;
    zend_string *prefix = NULL;

    if (zend_parse_parameters(argc , "|Sl", &prefix, &time_unit) == FAILURE) {
        return;
    }
	CHECK_TIME_UNIT(time_unit);

    array_init(return_value);

    db = YY_PROF_G(db).func_db;
    if(db == NULL) {
        php_error(E_WARNING, "YY_PROF: can not open func stat db");
        return;
    }

	yy_prof_storage_t *storage = YY_PROF_G(storage);
	yy_prof_db_iter_t iter = storage->iter_create(db);

    no_prefix = prefix == NULL || ZSTR_LEN(prefix) == 0;
	storage->iter_reset(iter);
	while (storage->iter_valid(iter)) {
		size_t klen = 0, vlen = 0;
		const char *key = storage->iter_key(iter, &klen);

		if(no_prefix || strncmp(key, ZSTR_VAL(prefix), ZSTR_LEN(prefix)) == 0) {
			array_init(&z);

			const char *val = storage->iter_value(iter, &vlen);
			stat = (yy_prof_func_stat_t *) val;

			add_assoc_long(&z, "type", stat->type);
			add_assoc_long(&z, "count", stat->count);
			add_assoc_long(&z, "time", stat->time / time_unit);
			add_assoc_long(&z, "request_bytes", stat->request_bytes);
			add_assoc_long(&z, "response_bytes", stat->response_bytes);
			add_assoc_long(&z, "status_200", stat->status_200);
			add_assoc_long(&z, "status_300", stat->status_300);
			add_assoc_long(&z, "status_400", stat->status_400);
			add_assoc_long(&z, "status_500", stat->status_500);
			add_assoc_long(&z, "status_501", stat->status_501);

			add_assoc_zval_ex(return_value, key, klen, &z);

			// storage->try_free(val);
		}
		// storage->try_free(key);
		storage->iter_next(iter);
	}
	storage->iter_destroy(iter);
}
/* }}} */


/* {{{ proto array yy_prof_close_stats(string func)
*/
PHP_FUNCTION(yy_prof_clear_stats)
{
	yy_prof_storage_t * storage = YY_PROF_G(storage);

    storage->clear(YY_PROF_G(db).page_db);
    storage->clear(YY_PROF_G(db).func_db);
    storage->clear(YY_PROF_G(db).trace_db);
}

/**
 * XHPROF universal begin function.
 * This function is called for all modes before the
 * mode's specific begin_function callback is called.
 *
 * @param  hp_entry_t **entries  linked list (stack)
 *                                  of hprof entries
 * @param  hp_entry_t  *current  hprof entry for the current fn
 * @return void
 * @author kannan, veeve
 */
static zend_always_inline void
yy_prof_mode_common_beginfn(yy_prof_entry_t **entries,
        yy_prof_entry_t  *current) {
    SET_TIMESTAMP_COUNTER(current->tsc_start);
}

/**
 * XHPROF universal end function.  This function is called for all modes after
 * the mode's specific end_function callback is called.
 *
 * @param  hp_entry_t **entries  linked list (stack) of hprof entries
 * @return void
 * @author kannan, veeve
 */
static zend_always_inline void
yy_prof_mode_common_endfn(yy_prof_entry_t **entries,
        yy_prof_entry_t *current) {
    /* Get end tsc counter */
    SET_TIMESTAMP_COUNTER(current->tsc_end);

    yy_prof_cpu_t * cpu = &YY_PROF_G(cpu);
    // 特定情况下, 某些函数自己计算消耗的时长
    if(current->time == 0) {
        current->time = GET_US_FROM_TSC(current->tsc_end - current->tsc_start,
            cpu->cpu_frequencies[cpu->cur_cpu_id]);
    }

    yy_prof_set_entry_status(current);

    if((current->adapter->flag & FLAG_SKIP_STAT) == 0) {
        yy_prof_record_page_stat(&YY_PROF_G(stat), current);
    }

    if(current->time >= YY_PROF_G(slow_run_time) && (current->adapter->flag & FLAG_SQL_PREPARE) == 0) {
        yy_prof_print_backtrace(&YY_PROF_G(request), &YY_PROF_G(log), current);
    }

    if(current->adapter->flag & FLAG_SQL_PREPARE) {
        ZSTR_RELEASE(YY_PROF_G(request).query);
        YY_PROF_G(request).query = yy_prof_func_get_first_arg();
    }
}

static zend_always_inline int
begin_request(zend_string * func) {
	yy_prof_request_detector_t * detector = &YY_PROF_G(detector);

    if(YY_PROF_G(enable_request_detector) && 
			detector->func != NULL && zend_string_equals(detector->func, func)) {
		yy_prof_stop();

		zend_string *(*callback)() = detector->get_key;
		if(detector->flag & FLAG_REPLACE_URI) {
			YY_PROF_G(request).uri2 = callback();
		} else {
			YY_PROF_G(request).tag = callback();
		}

		YY_PROF_G(started_by_detector) = 1;
		yy_prof_start();
		return 1;
	}
	return 0;
}

static zend_always_inline void
end_request(zend_string * func) {
	yy_prof_request_detector_t * detector = &YY_PROF_G(detector);

    if(detector->func != NULL && zend_string_equals(detector->func, func)) {
		yy_prof_stop();
		YY_PROF_G(started_by_detector) = 0;
	}
}

static zend_always_inline int
begin_profiling_ex(yy_prof_entry_t **entries, zend_string * symbol) {
    yy_prof_entry_t *entry;
    yy_prof_funcs_t *funcs = &YY_PROF_G(funcs);
    yy_prof_adapter_t * adapter = yy_prof_is_tracking_func_ex(funcs, symbol);

	if(adapter != NULL) {
		if(adapter->type == ADAPTER_TYPE_REQUEST_DETECTOR) {
			if(YY_PROF_G(detector).func != NULL) {
				return 0;
			}
			if(YY_PROF_G(enable_request_detector)) {
				char *(*callback)(yy_prof_request_detector_t *detector);
				yy_prof_request_detector_t * detector = &YY_PROF_G(detector);
				callback = (void *) adapter->callback;
				callback(detector);
				return 2;
			}
			return 0;
		}
		if(!YY_PROF_G(profiling)) {
			return 0;
		}

		entry = yy_prof_fast_alloc_entry(&YY_PROF_G(free_entries));
		void (*callback) (yy_prof_entry_t * entry);
		callback = adapter->callback;
		memset(entry, 0, sizeof(yy_prof_entry_t));
		entry->ptr = NULL;
		entry->request = &YY_PROF_G(request);
		entry->func = zend_string_copy(symbol);
		entry->prev = *entries;
		entry->time = 0;
		entry->adapter = adapter;
		callback(entry);
		if((entry)->key == NULL) {
			entry->key = zend_string_copy(entry->func);
		}
		yy_prof_mode_common_beginfn(entries, entry);
		*entries = entry;
	}
    return adapter != NULL ? 1 : 0;
}

static zend_always_inline void
end_profiling_ex(yy_prof_entry_t **entries, int profile_flag) {
    if(profile_flag == 1) {
        yy_prof_entry_t *entry = *entries;
        yy_prof_mode_common_endfn(entries, entry);
        *entries = (*entries)->prev;
        if(YY_PROF_G(request).is_cli 
				&& YY_PROF_G(enable_trace_cli) == 0
				&& YY_PROF_G(started_by_detector) == 0 
				&& YY_PROF_G(started_by_user) == 0) {
            yy_prof_save_func_stat_to_db(YY_PROF_G(db).func_db, entry);

			if(YY_PROF_G(log_level) == LOG_DEBUG) {
				fprintf(stderr, "DEBUG: ENTRY: %s => %s => %ld\n", ZSTR_VAL(entry->func),
						ZSTR_VAL(entry->key), entry->time);
			}
            yy_prof_fast_free_entry(&YY_PROF_G(free_entries), entry);
        } else {
            yy_prof_release_entry(&YY_PROF_G(request_entries), entry);
        }
    }
}

ZEND_DLEXPORT void
yy_prof_execute_ex (zend_execute_data *execute_data) {
    int profile_flag = 0, is_request_detector = 0;
    zend_string *func = NULL;

	if(YY_PROF_G(profiling) || YY_PROF_G(enable_request_detector)) {
		func = yy_prof_get_func_name(execute_data);
	}

    if (!func) {
        _zend_execute_ex(execute_data);
        return;
    }

    yy_prof_entry_t * entries = YY_PROF_G(entries);
    profile_flag = begin_profiling_ex(&entries, func);
	is_request_detector = begin_request(func);
    _zend_execute_ex(execute_data);
    if (profile_flag == 1) {
        end_profiling_ex(&entries, profile_flag);
    }
	if (is_request_detector) {
		end_request(func);
	}
    ZSTR_RELEASE(func);
}


/**
 * Proxy for zend_compile_file(). Used to profile PHP compilation time.
 *
 * @author kannan, hzhao
 */
ZEND_DLEXPORT zend_op_array* 
yy_prof_compile_file(zend_file_handle *file_handle, int type) {

    zend_op_array  *ret = NULL;

    return ret;
}

/**
 * Proxy for zend_compile_string(). Used to profile PHP eval compilation time.
 */
ZEND_DLEXPORT zend_op_array*
yy_prof_compile_string(zval *source_string, char *filename) {
    zend_op_array *ret = NULL;

    return ret;
}


ZEND_DLEXPORT void 
yy_prof_execute_internal(zend_execute_data *execute_data,
        zval * return_value) {
    zend_string * func = NULL;
    int profile_flag = 0, is_request_detector = 0;
    yy_prof_entry_t * entries = YY_PROF_G(entries);

	if(YY_PROF_G(profiling) || YY_PROF_G(enable_request_detector)) {
		func = yy_prof_get_func_name(EG(current_execute_data));
		if (func) {
			profile_flag = begin_profiling_ex(&entries, func);
			is_request_detector = begin_request(func);
		}
	}

    if(_zend_execute_internal == NULL) {
        execute_internal(execute_data, return_value);
    } else {
        _zend_execute_internal(execute_data, return_value);
    }

    if (func) {
        if (profile_flag == 1) {
            end_profiling_ex(&entries, profile_flag);
		}
		if (is_request_detector) {
			end_request(func);
		}
        ZSTR_RELEASE(func);
    }
}

static void 
yy_prof_enable() {
    if(YY_PROF_G(enabled)) {
        return;
    }
    YY_PROF_G(enabled) = 1;

    /* Replace zend_execute with our proxy */
    _zend_execute_ex = zend_execute_ex;
    zend_execute_ex  = yy_prof_execute_ex;

    _zend_execute_internal = zend_execute_internal;
    zend_execute_internal = yy_prof_execute_internal;

    if(!YY_PROF_G(ever_enabled)) {
        YY_PROF_G(ever_enabled) = 1;
    }
}

/**
 * Called at request shutdown time. Cleans the profiler's global state.
 */
static void 
yy_prof_disable() {
    /* Bail if not ever enabled */
    if (!YY_PROF_G(ever_enabled)) {
        return;
    }

    /* Clean up state */
	YY_PROF_G(enabled) = 0;
    YY_PROF_G(ever_enabled) = 0;


    ZSTR_RELEASE(YY_PROF_G(request).tag);
    ZSTR_RELEASE(YY_PROF_G(request).stat_key);

    /* Remove proxies, restore the originals */
    zend_execute_ex       = _zend_execute_ex;
    zend_execute_internal = _zend_execute_internal;
    //zend_compile_file     = _zend_compile_file;
    //zend_compile_string   = _zend_compile_string;
}

static void 
yy_prof_start()
{
	if(YY_PROF_G(profiling)) {
		return ;
	}
	YY_PROF_G(profiling) = 1;
    bind_rand_cpu(&YY_PROF_G(cpu));
    SET_TIMESTAMP_COUNTER(YY_PROF_G(request).tsc_start);
}

static void 
yy_prof_stop() 
{
	if(YY_PROF_G(profiling) == 0) {
		return ;
	}
	YY_PROF_G(profiling) = 0;

    SET_TIMESTAMP_COUNTER(YY_PROF_G(request).tsc_end);

    // compute request rt
    yy_prof_cpu_t * cpu = &YY_PROF_G(cpu);
    yy_prof_request_t * request = &YY_PROF_G(request);
	request->time = GET_US_FROM_TSC(request->tsc_end - request->tsc_start,
            cpu->cpu_frequencies[cpu->cur_cpu_id]);
    YY_PROF_G(stat).request_time = request->time;

    /* End any unfinished calls */
    int profile_flag = 1;
    yy_prof_entry_t ** entries = &YY_PROF_G(entries);
    while (*entries) {
        end_profiling_ex(entries, profile_flag);
    }

	// save all the stat
	yy_prof_save();

    yy_prof_free_request_entries(&YY_PROF_G(request_entries), &YY_PROF_G(free_entries));

    /* Resore cpu affinity. */
    release_cpu(&YY_PROF_G(cpu));

    memset(&YY_PROF_G(stat), 0, sizeof(yy_prof_page_stat_t));

	ZSTR_RELEASE(YY_PROF_G(request).tag);
	ZSTR_RELEASE(YY_PROF_G(request).uri2);
	ZSTR_RELEASE(YY_PROF_G(request).stat_key);
}

static void
yy_prof_save() 
{
    // save the request entries
    yy_prof_save_func_stat_to_db(YY_PROF_G(db).func_db, YY_PROF_G(request_entries));

	// save the trace entries
    if(YY_PROF_G(enable_trace)) {
        yy_prof_save_page_func_stat_to_db_ex(YY_PROF_G(db).trace_db,
           &YY_PROF_G(request), YY_PROF_G(request_entries));
    }

    // save the page stat
    yy_prof_save_page_stat_to_db(YY_PROF_G(db).page_db,
        &YY_PROF_G(request), &YY_PROF_G(stat));

	// print debug info
	if(YY_PROF_G(log_level) == LOG_DEBUG) {
		yy_prof_request_t * request = &YY_PROF_G(request);
		fprintf(stderr, "DEBUG> REQUEST: %s => %ld\n", 
			ZSTR_VAL(request->uri2 ? request->uri2 : request->uri), request->time);
		yy_prof_entry_t * entry = YY_PROF_G(request_entries);
		while (entry) {
			fprintf(stderr, "DEBUG> ENTRY: %s => %s => %ld\n", ZSTR_VAL(entry->func),
				ZSTR_VAL(entry->key), entry->time);
			entry = entry->prev;
		}
	}
}


static zend_always_inline double
yy_prof_get_load(long interval) {
    static double loadavg[1];
    static time_t last_time = 0;
    time_t cur_time;

    cur_time = time(NULL);
    if(cur_time - last_time <= interval) {
        return loadavg[0];
    }

    last_time = cur_time;
    if(getloadavg(loadavg, sizeof(loadavg) / sizeof(double)) == -1) {
        return 0;
    }

    return loadavg[0];
}

static zend_always_inline int
yy_prof_check_load() {
	int ret = SUCCESS;
    double load = yy_prof_get_load(YY_PROF_G(protect_check_interval));
    switch (YY_PROF_G(protect_last_state)) {
        case STATE_PROTECT:
            if(load >= YY_PROF_G(protect_restore_load)) {
                ret = FAILURE;
				break;
            }
            YY_PROF_G(protect_last_state) = STATE_OK;
            break;
        case STATE_OK:
        default:
            if(load >= YY_PROF_G(protect_load)) {
                YY_PROF_G(protect_last_state) = STATE_PROTECT;
                ret = FAILURE;
				break;
            }
            break;
    }
	if(YY_PROF_G(log_level) == LOG_DEBUG) {
		fprintf(stderr, "DEBUG>	LOAD: current load is %.2f, current state is %d\n", load, ret);
	}
	return ret;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
