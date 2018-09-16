#ifndef __YY_PROF_STAT_H__
#define __YY_PROF_STAT_H__

#include "include/yy_structs.h"

#define YY_PROF_STAT_VERSION      1
#define YY_PROF_DB_PAGE_STAT_FILE LOCALSTATEDIR "/cache/yy_prof/page_stat.%d.%s"
#define YY_PROF_DB_FUNC_STAT_FILE LOCALSTATEDIR "/cache/yy_prof/func_stat.%d.%s"
#define YY_PROF_DB_TRACE_FILE     LOCALSTATEDIR "/cache/yy_prof/trace.%d.%s"

void yy_prof_record_page_stat(yy_prof_page_stat_t * stat, yy_prof_entry_t * entry);

void * yy_prof_open_stat_db(char * file);

void yy_prof_close_stat_db(void *db);

void yy_prof_save_page_stat_to_db(void *db, yy_prof_request_t * request, yy_prof_page_stat_t * stat);

void yy_prof_save_func_stat_to_db(void *db, yy_prof_entry_t * entries);

void yy_prof_save_page_func_stat_to_db_ex(void *db, yy_prof_request_t * request, yy_prof_entry_t * entries);

#endif
