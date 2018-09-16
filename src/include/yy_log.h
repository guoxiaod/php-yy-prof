#ifndef __YY_PROF_LOG_H__
#define __YY_PROF_LOG_H__

#include "include/yy_structs.h"

#define YY_PROF_SLOW_LOG_FILE LOCALSTATEDIR "/log/yy_prof/slow.log-%d"

void yy_prof_open_slow_log(yy_prof_slow_log_t * log, time_t cur_time);
void yy_prof_close_slow_log(yy_prof_slow_log_t * log);

void yy_prof_print_backtrace(
    yy_prof_request_t * request, yy_prof_slow_log_t * log, 
    yy_prof_entry_t * entry TSRMLS_DC);

#endif
