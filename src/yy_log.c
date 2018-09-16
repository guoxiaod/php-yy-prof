#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "main/SAPI.h"
#include "Zend/zend_builtin_functions.h"

#include "include/yy_log.h"
#include "include/yy_adapter.h"


static zend_always_inline smart_str *
yy_prof_fetch_first_backtrace(zend_string * func, smart_str *str  );

static zend_always_inline void 
yy_prof_print_backtrace_ex(yy_prof_request_t * request, yy_prof_slow_log_t * log, 
        yy_prof_entry_t * entry );

void yy_prof_open_slow_log(yy_prof_slow_log_t *log, time_t cur_time) {
	int day;
	mode_t mode;
	struct tm cur_tm;
	
	localtime_r(&cur_time, &cur_tm);

	day = (cur_tm.tm_year + 1900) * 10000 + (cur_tm.tm_mon + 1) * 100 + cur_tm.tm_mday;
	if(day != log->day) {
        yy_prof_close_slow_log(log);

        log->day = day;
        log->file = zend_string_alloc(sizeof(YY_PROF_SLOW_LOG_FILE) + 6, 1);
        sprintf(ZSTR_VAL(log->file), YY_PROF_SLOW_LOG_FILE, day);
        ZSTR_VAL(log->file)[sizeof(YY_PROF_SLOW_LOG_FILE) + 5] = '\0';
        mode = umask(0111);
        log->fd = fopen(ZSTR_VAL(log->file), "a+");
        umask(mode);
        if(log->fd == NULL) {
            php_error(E_WARNING, "YY_PROF: Cann't open log file for writing %s", ZSTR_VAL(log->file));
        }
	}
}

void yy_prof_close_slow_log(yy_prof_slow_log_t * log) {
    if(log->fd != NULL) {
        fclose(log->fd);
        log->fd = NULL;
    }
    ZSTR_FREE(log->file);
    log->day = 0;
}


static zend_always_inline void 
yy_prof_print_backtrace_ex(yy_prof_request_t * request, yy_prof_slow_log_t * log, 
        yy_prof_entry_t * entry ) {
	int i = 0;
	int len;
    char buffer[1024] = {0};
    smart_str buf = {0};
	zval z, *item = NULL;
	HashTable * ht, *h;
	HashPosition pos;
	zval * file, * line, * class, * type, * function;

    smart_str_appendl(&buf, "KEY: ", sizeof("KEY: ") - 1);
    smart_str_appendl(&buf, ZSTR_VAL(entry->key), ZSTR_LEN(entry->key));
    smart_str_appendc(&buf, '\n');

    yy_prof_func_get_origin_key(entry, &buf);

    yy_prof_func_get_time_stats(entry, &buf);

    // 获取当前函数所在的文件以及行号
    yy_prof_fetch_first_backtrace(entry->func, &buf);

    // 获取函数的调用的堆栈
	zend_fetch_debug_backtrace(&z, 1, DEBUG_BACKTRACE_IGNORE_ARGS, 20);

    i ++;
    // 打印调用堆栈
	if(Z_TYPE_P(&z) == IS_ARRAY) {
		ht = Z_ARRVAL_P(&z);	

		for(zend_hash_internal_pointer_reset_ex(ht, &pos);
				(item = zend_hash_get_current_data_ex(ht, &pos)) != NULL;
				zend_hash_move_forward_ex(ht, &pos)) {
			h = Z_ARRVAL_P(item);
			
			file = line = class = function = type = NULL;
			//zend_hash_find(h, "file", sizeof("file"), (void **) &file); 
			//zend_hash_find(h, "line", sizeof("line"), (void **) &line); 
            //zend_hash_find(h, "class", sizeof("class"), (void **) &class); 
            //zend_hash_find(h, "function", sizeof("function"), (void **) &function); 
            //zend_hash_find(h, "type", sizeof("type"), (void **) &type); 
			file = zend_hash_str_find(h, "file", sizeof("file") - 1); 
			line = zend_hash_str_find(h, "line", sizeof("line") - 1); 
            class = zend_hash_str_find(h, "class", sizeof("class") - 1); 
            function = zend_hash_str_find(h, "function", sizeof("function") - 1); 
            type = zend_hash_str_find(h, "type", sizeof("type") - 1); 
            memset(buffer, 0, sizeof(buffer));
			len = snprintf(buffer, sizeof(buffer), "%d) %s:%d %s%s%s()\n",
				i,
				file ? Z_STRVAL_P(file) : "unknown file",
				line ? Z_LVAL_P(line) : 0,
				class ? Z_STRVAL_P(class) : "",
				type ? Z_STRVAL_P(type) : "",
				function ? Z_STRVAL_P(function) : ""
            );
            smart_str_appendl(&buf, buffer, len);
			i++;
		}
	}
    if(log->fd) {
        //fwrite(buf.c, buf.len, 1, log->fd);
        fwrite(ZSTR_VAL(buf.s), ZSTR_LEN(buf.s), 1, log->fd);
    } else {
        //php_error(E_NOTICE, buf.c);
        php_error(E_NOTICE, ZSTR_VAL(buf.s));
    }
    smart_str_free(&buf);
	zval_dtor(&z);
}

void yy_prof_print_backtrace(
        yy_prof_request_t * request, yy_prof_slow_log_t * log, 
        yy_prof_entry_t * entry ) {
    char timestr[32];
    time_t cur_time;
    struct tm cur_tm;

	cur_time = time(NULL);
    yy_prof_open_slow_log(log, cur_time);

    if(log->fd == NULL) {
        php_error(E_NOTICE,
            "\n============SLOW RUN CODE===========\n"
            "FUNC: <%s> SPENT: %ldms UUID: %s\n"
            "URI: %s",
			ZSTR_VAL(entry->func), 
            entry->time / TIME_UNIT_MS,
            ZSTR_VAL(entry->request->uuid),
            ZSTR_VAL(entry->request->uri));
    } else {
        localtime_r(&cur_time, &cur_tm);
		memset(timestr, 0, sizeof(timestr));
        strftime(timestr, sizeof(timestr), "%Y-%m-%d %T %z", &cur_tm);
        fprintf(log->fd,
            "============SLOW RUN CODE===========\n"
            "TIME: [%s] FUNC: <%s> SPENT: %ldms UUID: %s\n"
            "URI: %s\n",
            timestr, ZSTR_VAL(entry->func), entry->time / TIME_UNIT_MS,
            ZSTR_VAL(entry->request->uuid),
            ZSTR_VAL(entry->request->uri));
		yy_prof_print_backtrace_ex(request, log, entry);
        fflush(log->fd);
    }
}

static zend_always_inline smart_str *
yy_prof_fetch_first_backtrace(zend_string * func, smart_str * str ) {
    int len = 0;
    char buffer[1024] = {0};
    const char * filename = zend_get_executed_filename();
    uint lineno = zend_get_executed_lineno();
  
    len = snprintf(buffer, sizeof(buffer), "0) %s:%d %s()\n", filename, lineno, ZSTR_VAL(func));
    smart_str_appendl(str, buffer, len);

    return str;
}

