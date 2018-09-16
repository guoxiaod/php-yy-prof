#include <stdlib.h>
#include <curl/curl.h>
#include "php.h"
#include <ext/http/php_http.h>
#include <ext/http/php_http_api.h>
#include <ext/http/php_http_message.h>
#include <ext/http/php_http_client.h>

#include "include/yy_adapter.h"
#include "include/yy_func.h"

#define PHP_CURL_PADDING_LEN 296

#define is_empty_char(c) ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n' || (c) == '\0')
#define is_invalid_name_char(c) ((c) == '=' || (c) == '\'' || (c) == '\"' || (c) == '(' || (c) == ')')

#define YY_PROF_EXTRACT_ARGS() \
    uint32_t argc = 0; \
    zval * args = NULL; \
    zend_execute_data *execute_data; \
    execute_data = EG(current_execute_data); \
    if(!execute_data) { \
        return ; \
    } \
    (argc) = ZEND_CALL_NUM_ARGS(execute_data); \
    if((argc) > 0) { \
        (args) = ZEND_CALL_ARG(execute_data, 1); \
    }

#define YY_PROF_EXTRACT_ARGS_WITH_RETURN(ret) \
    uint32_t argc = 0; \
    zval * args = NULL; \
    zend_execute_data *execute_data; \
    execute_data = EG(current_execute_data); \
    if(!execute_data) { \
        return (ret); \
    } \
    (argc) = ZEND_CALL_NUM_ARGS(execute_data); \
    if((argc) > 0) { \
        (args) = ZEND_CALL_ARG(execute_data, 1); \
    }

static yy_prof_adapter_t * yy_prof_adapter_list = NULL;
static int yy_prof_adapter_list_len = 0;

static zend_always_inline zend_string *
yy_prof__url_simplify_key(char * key, int key_len);
static zend_always_inline zend_string *
yy_prof__sql_simplify_key(char * key, int key_len);
static zend_always_inline zend_string *
yy_prof__default_simplify_key(char * key, int key_len, char * prefix, int prefix_len);

static zend_always_inline zend_string **
yy_prof_zval_to_string_arr(zval *values, size_t *len);
extern int  le_curl;
#ifndef le_curl_name
#define le_curl_name "cURL handle"
#endif
extern int  le_curl_multi_handle;
#ifndef le_curl_multi_handle_name
#define le_curl_multi_handle_name "cURL Multi Handle"
#endif
extern int  le_curl_share_handle;
#ifndef le_curl_share_handle
#define le_curl_share_handle_name "cURL Share Handle"
#endif

static zend_always_inline void
yy_prof__curl_ex(yy_prof_entry_t * entry, int idx) {
    zval *z;
    char * uri;
    void * ch;
    CURL * curl;

    double total_time = 0;
    
    entry->type = IO_TYPE_URL;

    YY_PROF_EXTRACT_ARGS();

    z = args + idx - 1;
    if(argc < idx || Z_TYPE_P(z) != IS_RESOURCE) {
        return ;
    }   

    ch = Z_RES_P(z)->ptr;

    curl = *(CURL**) (ch);
    if(curl == NULL) {
        return ;
    }
    entry->ptr = (void*) curl;
    if(curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &uri) == CURLE_OK) {
        entry->key = yy_prof__url_simplify_key(uri, strlen(uri));
    }

    if(curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time) == CURLE_OK) {
        entry->time = total_time * SEC_TO_USEC;
    }
}

static void 
yy_prof__curl(yy_prof_entry_t * entry) {
    yy_prof__curl_ex(entry, 1);
}

static void 
yy_prof__curl_2(yy_prof_entry_t * entry) {
    yy_prof__curl_ex(entry, 2);
}


static void
yy_prof__yar(yy_prof_entry_t * entry) {
    int len;
    int prefix_len = sizeof("Yar_client::") - 1;
    char * method, *str;
    zend_string * func;
    zval * z_uri, rv;
	
    entry->type = IO_TYPE_URL;

    YY_PROF_EXTRACT_ARGS();

    if(argc == 0) {
        return ;
    }

    func = entry->func;
    method = ZSTR_VAL(func) + prefix_len;
    // direct return 0 if method is prefixed by "__" and not __call
    if(*method == '_' && *(method + 1) == '_' && strcmp(method, "__call") != 0) {
        return ;
    }
    if(strcmp(method, "setOpt") == 0) {
        return ;
    }

#if PHP_VERSION_ID < 70100
    zend_class_entry *scope = EG(scope);
#else
    zend_class_entry *scope = zend_get_executed_scope();
#endif
    z_uri = zend_read_property(scope, &EX(This), ZEND_STRL("_uri"), 0, &rv);
    if(z_uri != NULL && Z_STRLEN_P(z_uri) > 0) {
        len = Z_STRLEN_P(z_uri) + ZSTR_LEN(func) - prefix_len + 1;
        str = emalloc(len + 1);
        snprintf(str, len + 1, "%s/%s", Z_STRVAL_P(z_uri), method);
        entry->key = yy_prof__url_simplify_key(str, len);
        efree(str);
    }
}

static void
yy_prof__http(yy_prof_entry_t * entry) {
    void * last;
    char * uri;
    double total_time;
    php_http_client_object_t * client_obj;
    php_http_client_t * client;
    php_http_message_t * message = NULL;
    php_http_message_object_t * request, * response;
    php_http_client_enqueue_t * enqueue;
    zend_llist_position pos;

    zval *zrequest;

    CURL * curl = NULL;

    YY_PROF_EXTRACT_ARGS();

    entry->type = IO_TYPE_URL;

    zrequest = args;

    client_obj = PHP_HTTP_OBJ(NULL, getThis());
    if(client_obj == NULL) {
        return ;
    }
    client = client_obj->client;

    if(argc > 0 && zrequest != NULL) { 
        request = PHP_HTTP_OBJ(NULL, zrequest);
        if(request != NULL) {
            message = request->message;
        }
    } else {
        last = zend_llist_get_last(&client->responses);
        if(last == NULL) {
            return ;
        }
        response = *(php_http_message_object_t **) last;
        message = response == NULL ? NULL : response->message->parent;
    }

    for(enqueue = zend_llist_get_first_ex(&client->requests, &pos); enqueue; 
            enqueue = zend_llist_get_next_ex(&client->requests, &pos)) {
        if(enqueue->request == message) {
            curl = *(CURL**) (enqueue->opaque);
            break;
        }
    }

    if(curl == NULL) {
        return ;
    }

    entry->ptr = (void*) curl;
    if(curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time) == CURLE_OK) {
        entry->time = total_time * SEC_TO_USEC;
    }

    if(curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &uri) == CURLE_OK) {
        entry->key = yy_prof__url_simplify_key(uri, strlen(uri));
    }
}

static void
yy_prof__pdo_ex(yy_prof_entry_t * entry, int idx) {
    zval *z;

    entry->type = IO_TYPE_SQL;

    YY_PROF_EXTRACT_ARGS();

    z = args + idx - 1;
    if(argc >= idx && Z_TYPE_P(z) == IS_STRING) {
        entry->ptr = zend_string_copy(Z_STR_P(z));
        entry->key = yy_prof__sql_simplify_key(Z_STRVAL_P(z),
            Z_STRLEN_P(z));
    }
}

static void
yy_prof__pdo(yy_prof_entry_t * entry) {
    yy_prof__pdo_ex(entry, 1);
}

static void
yy_prof__pdo_2(yy_prof_entry_t * entry) {
    yy_prof__pdo_ex(entry, 2);
}

static void
yy_prof__stmt_execute(yy_prof_entry_t * entry) {
    entry->type = IO_TYPE_SQL;
    if(entry->request->query != NULL) {
        entry->ptr = zend_string_copy(entry->request->query);
        entry->key = yy_prof__sql_simplify_key(ZSTR_VAL(entry->request->query),
            ZSTR_LEN(entry->request->query));
    }
}
static void 
yy_prof__mysqli_connect_func_ex(yy_prof_entry_t * entry, int idx) {
    zval *z, *z2;

    entry->type = IO_TYPE_SQL;

    YY_PROF_EXTRACT_ARGS();

    z = args;
    z2 = (args + idx - 1);
    if(z && z2 && Z_TYPE_P(z2) == IS_STRING) {
        entry->key = yy_prof__default_simplify_key(ZSTR_VAL(entry->func),
            ZSTR_LEN(entry->func), NULL, 0);
    }
}


static void 
yy_prof__mysqli_connect_func_1(yy_prof_entry_t * entry) {
    yy_prof__mysqli_connect_func_ex(entry, 1); 
}

static void 
yy_prof__mysqli_connect_func_2(yy_prof_entry_t * entry) {
    yy_prof__mysqli_connect_func_ex(entry, 2); 
}

static void
yy_prof__amqp(yy_prof_entry_t * entry) {
    entry->type = IO_TYPE_QUEUE;
    entry->key = yy_prof__default_simplify_key(ZSTR_VAL(entry->func), 
        ZSTR_LEN(entry->func), NULL, 0);
}

static zend_string * 
yy_prof__amqpqueue_consume_get_key() {
    zval *z, ret, func;
    YY_PROF_EXTRACT_ARGS_WITH_RETURN(NULL);

    z = args + 1;
    if(argc < 2 || z == NULL || Z_TYPE_P(z) != IS_OBJECT) {
        return NULL;
    }

    size_t len;
    zend_string * key;
    ZVAL_STRING(&func, "getName");
    if(call_user_function(NULL, z, &func, &ret, 0, NULL) == SUCCESS) {
        len = sizeof("AMQPQueue::consume::") - 1 + Z_STRLEN(ret);
        key = zend_string_alloc(len, 0);
        snprintf(ZSTR_VAL(key), ZSTR_LEN(key) + 1, "AMQPQueue::consume::%s", Z_STRVAL(ret));
        return key;
    }

    return NULL;
}

static void
yy_prof__amqpqueue_consume(yy_prof_request_detector_t * detector) {
    zval *z;
    zend_string * function_name = NULL;

    YY_PROF_EXTRACT_ARGS();

    if(argc < 1) {
        return;
    }
	
    z = args;

    function_name = yy_prof_get_func_name_by_zval(z);
    if(function_name != NULL) {
        detector->func = function_name;
        detector->get_key = yy_prof__amqpqueue_consume_get_key;
        detector->flag = FLAG_REPLACE_URI;
    }
}

static void
yy_prof__redis(yy_prof_entry_t * entry) {
    entry->type = IO_TYPE_CACHE;
    entry->key = yy_prof__default_simplify_key(ZSTR_VAL(entry->func), 
        ZSTR_LEN(entry->func), NULL, 0);
}

static void
yy_prof__aerospike(yy_prof_entry_t * entry) {
    entry->type = IO_TYPE_CACHE;
    entry->key = yy_prof__default_simplify_key(ZSTR_VAL(entry->func), 
        ZSTR_LEN(entry->func), NULL, 0);
}

static void
yy_prof__soap(yy_prof_entry_t * entry) {
    zval *z;

    entry->type = IO_TYPE_URL;

    YY_PROF_EXTRACT_ARGS();
    
    z = args;
    if(argc > 0 && Z_TYPE_P(z) == IS_STRING) {
        entry->key = yy_prof__default_simplify_key(Z_STRVAL_P(z),
            Z_STRLEN_P(z), "SOAP:", sizeof("SOAP:") - 1);
    }
}

/*
static void
yy_prof__mongo(yy_prof_entry_t * entry) {
    void * ch;
    zval **z;
	
    entry->type = IO_TYPE_MONGODB;

    YY_PROF_EXTRACT_ARGS();

    if(argc == 0) {
        return 0;
    }

    ch = zend_object_store_get_object(ptr->object); 
    z = (zval **) (ch + sizeof(zend_object) + sizeof(zval *) * 2);
    if(z && *z && Z_TYPE_P(*z) == IS_STRING) {
        entry->key = yy_prof__default_simplify_key(Z_STRVAL_P(*z),
            Z_STRLEN_P(*z), "MDB:", sizeof("MDB:") - 1);
    }
}

static void
yy_prof__mongodb(yy_prof_entry_t * entry) {
    void * ch;
    zval **z;
	
    entry->type = IO_TYPE_MONGODB;

    YY_PROF_EXTRACT_ARGS();

    if(argc == 0) {
        return ;
    }

    ch = zend_object_store_get_object(ptr->object); 
    z = (zval **) (ch + sizeof(zend_object) + sizeof(zval *));
    if(z && *z && Z_TYPE_P(*z) == IS_STRING) {
        entry->key = yy_prof__default_simplify_key(Z_STRVAL_P(*z), 
            Z_STRLEN_P(*z), "MDB:", sizeof("MDB:") - 1);
    }
}
*/

static void
yy_prof__mongodb_execute(yy_prof_entry_t * entry) {
    zval *z;
    
    entry->type = IO_TYPE_MONGODB;

    YY_PROF_EXTRACT_ARGS();
    
    z = (zval *) args;
    if(z && Z_TYPE_P(z) == IS_STRING) {
        entry->key = yy_prof__default_simplify_key(Z_STRVAL_P(z), Z_STRLEN_P(z),
                "MDB2:", sizeof("MDB2:") - 1);
    }
}

static zend_string *
yy_prof__swoole_http_server_request_get_key() {
    size_t len;

    zval *z;
    zend_string * key;
    char port_str[10] = {0};
    zval *z_header, * z_server, ret_header, ret_server;
    zval * host = NULL, * port = NULL, *request_uri = NULL;
    YY_PROF_EXTRACT_ARGS_WITH_RETURN(NULL);

    z = args;
    if(argc == 0 || z == NULL || Z_TYPE_P(z) != IS_OBJECT) {
        return NULL;
    }

    z_header = zend_read_property(Z_OBJCE_P(z), z, ZEND_STRL("header"), 0, &ret_header);
    z_server = zend_read_property(Z_OBJCE_P(z), z, ZEND_STRL("server"), 0, &ret_server);

    if(z_header != NULL) {
        host = zend_hash_str_find(Z_ARR_P(z_header), ZEND_STRL("host"));
    }
    if(host == NULL && z_server != NULL) {
        host = zend_hash_str_find(Z_ARR_P(z_server), ZEND_STRL("server_addr"));
        port = zend_hash_str_find(Z_ARR_P(z_server), ZEND_STRL("server_port"));

        if(port) {
            snprintf(port_str, sizeof(port_str), "%s", Z_LVAL_P(port));
        }
    }
    if(z_server != NULL) {
        request_uri = zend_hash_str_find(Z_ARR_P(z_server), ZEND_STRL("request_uri"));
    }

    len = host ? Z_STRLEN_P(host) : 0;
    len += request_uri ? Z_STRLEN_P(request_uri) : 0;
    len += strlen(port_str) ? strlen(port_str)  + 1 : 0;
    len += sizeof("http://") - 1;
    key = zend_string_alloc(len, 0);
    snprintf(ZSTR_VAL(key), ZSTR_LEN(key) + 1, 
        "http://%s%s%s%s", host ? Z_STRVAL_P(host) : "", 
        port ? ":" : "", port_str,
        request_uri ? Z_STRVAL_P(request_uri) : "");
    ZSTR_H(key) = 0;

    return key;
}
static void 
yy_prof__swoole_http_server(yy_prof_request_detector_t * detector) {
    zval *z,*z2;

    YY_PROF_EXTRACT_ARGS();

    if(argc < 2) {
        return;
    }

    z = args;
    z2 = args + 1;
    if(z == NULL || z2 == NULL || Z_TYPE_P(z) != IS_STRING) {
        return;
    }
    if(zend_binary_strcasecmp(Z_STRVAL_P(z), Z_STRLEN_P(z),
            "Request", sizeof("Request") - 1) != 0) {
        return;
    }
	
    zend_string * function_name = yy_prof_get_func_name_by_zval(z2);
    if(function_name != NULL) {
        detector->func = function_name;
        detector->get_key = yy_prof__swoole_http_server_request_get_key;
        detector->flag = FLAG_REPLACE_URI;
    }
}

static void
yy_prof__default(yy_prof_entry_t * entry) {
    entry->type = IO_TYPE_DEFAULT;

    entry->key = yy_prof__default_simplify_key(ZSTR_VAL(entry->func), 
        ZSTR_LEN(entry->func), "FUNC:", sizeof("FUNC:") - 1);
}

static void
yy_prof__sql_func(yy_prof_entry_t * entry) {
    entry->type = IO_TYPE_SQL;

    entry->key = yy_prof__default_simplify_key(ZSTR_VAL(entry->func),
        ZSTR_LEN(entry->func), NULL, 0);
}

static zend_always_inline int
normalize_table_name(char * str, int len) {
    char * src, * dst, * end;

    src = dst = str;
    end = str + len;
    
    while (src < end) {
        if(* src >= '0' && * src <= '9') {
            * dst ++ = 'N';
        } else if(*src != '`' && *src != '"') {
            * dst ++ = *src;
        }
        src ++;
    }
    return dst - str;
}

static zend_always_inline char * 
str_copy_and_upper(char * dest, char * src, int maxsize) {
    int last_is_empty = 0;
    // skip empty chars
    while(is_empty_char(*src)) {
        src ++;
    }
    while(-- maxsize && *src) {
        if(*src >= 'a' && *src <= 'z') {
            if(last_is_empty) {
                * dest ++ = ' ';
                last_is_empty = 0;
            }
            * dest ++ = * src - 32;
        } else if(!is_empty_char(*src)) {
            if(last_is_empty) {
                * dest ++ = ' ';
                last_is_empty = 0;
            }
            * dest ++ = * src;
        } else {
            last_is_empty = 1;
        }
        src ++;
    }
    *dest = '\0';
    return dest;
}

typedef struct yy_prof_sql_syntax_t {
    char * first_word;
    char * tag;
    char * result;
    unsigned char first_word_len;
    unsigned char tag_offset;
    unsigned char result_len;
} yy_prof_sql_syntax_t;

static yy_prof_sql_syntax_t
sql_syntax_list[] = {
    {"ALTER",     NULL,            "SQL::ALTER",      5, 0, 10},
    {"ANALYZE",   NULL,            "SQL::ANALYZE",    7, 0, 12},
    {"BEGIN",     NULL,            "SQL::BEGIN",      5, 6, 10},
    {"BINLOG",    NULL,            "SQL::BINLOG",     6, 0, 11},
    {"CACHE",     NULL,            "SQL::CACHE",      5, 0, 10},
    {"CALL",      NULL,            NULL,              4, 5, 0},
    {"CHANGE",    NULL,            "SQL::CHANGE",     6, 0, 11},
    {"CHECK",     NULL,            "SQL::CHECK",      5, 0, 10},
    {"CHECKSUM",  NULL,            "SQL::CHECKSUM",   8, 0, 13},
    {"COMMIT",    NULL,            "SQL::COMMIT",     6, 7, 11},
    {"CREATE",    NULL,            "SQL::CREATE",     6, 0, 11},
    {"DEALLOCATE",NULL,            "SQL::DEALLOCATE",10, 0, 15},
    {"DELETE",    "FROM",          "SQL::DELETE",     6, 7, 11},
    {"DESC",      NULL,            "SQL::DESCRIBE",   4, 0, 13},
    {"DESCRIBE",  NULL,            "SQL::DESCRIBE",   8, 0, 13},
    {"DO",        NULL,            "SQL::DO",         2, 0, 7},
    {"DROP",      NULL,            "SQL::DROP",       4, 0, 9},
    {"EXECUTE",   NULL,            "SQL::EXECUTE",    7, 0, 12},
    {"EXPLAIN",   NULL,            "SQL::EXPLAIN",    7, 0, 12},
    {"FLUSH",     NULL,            "SQL::FLUSH",      5, 0, 10},
    {"GRANT",     NULL,            "SQL::GRANT",      5, 0, 10},
    {"HANDLER",   NULL,            NULL,              7, 0, 0},
    {"HELP",      NULL,            "SQL::HELP",       4, 0, 9},
    {"INSERT",    "INTO",          "SQL::INSERT",     6, 7, 11},
    {"INSTALL",   NULL,            "SQL::INSTALL",    7, 0, 12},
    {"KILL",      NULL,            "SQL::KILL",       4, 0, 9},
    {"LOAD DATA", "INTO TABLE",    NULL,              9,10, 0},
    {"LOAD INDEX",NULL,            "SQL::LOADINDEX", 10, 0, 14},
    {"LOAD XML",  "INTO TABLE",    NULL,              8, 9, 0},
    {"LOCK",      "TABLES",        NULL,              4, 5, 0},
    {"OPTIMIZE",  NULL,            "SQL::OPTIMIZE",   8, 0, 13},
    {"PREPARE",   NULL,            "SQL::PREPARE",    7, 0, 12},
    {"PURGE",     NULL,            "SQL::PURGE",      5, 0, 10},
    {"RELEASE",   NULL,            "SQL::RELEASE",    7, 8, 12},
    {"RENAME",    NULL,            "SQL::RENAME",     6, 0, 11},
    {"REPAIR",    NULL,            "SQL::REPAIR",     6, 0, 11},
    {"REPLACE",   "INTO",          "SQL::REPLACE",    7, 8, 12},
    {"RESET",     NULL,            "SQL::RESET",      5, 0, 10},
    {"REVOKE",    NULL,            "SQL::REVOKE",     6, 0, 11},
    {"ROLLBACK",  NULL,            "SQL::ROLLBACK",   8, 9, 13},
    {"SAVEPOINT", NULL,            "SQL::SAVEPOINT",  9,10, 14},
    {"SELECT",    "FROM",          "SQL::SELECT",     6, 7, 11},
    {"SET",       NULL,            "SQL::SET",        3, 0, 8},
    {"SHOW",      NULL,            "SQL::SHOW",       4, 0, 9},
    {"SHUTDOWN",  NULL,            "SQL::SHUTDOWN",   8, 0, 13},
    {"START",     NULL,            "SQL::START",      5, 6, 10},
    {"STOP",      NULL,            "SQL::STOP",       4, 0, 9},
    {"TRUNCATE",  NULL,            "SQL::TRUNCATE",   8, 0, 13},
    {"UPDATE",    NULL,            "SQL::UPDATE",     6, 7, 11},
    {"UNLOCK",    NULL,            "SQL::UNLOCK",     6, 0, 11},
    {"USE",       NULL,            "SQL::USE",        3, 0, 8},
    {"UNINSTALL", NULL,            "SQL::UNINSTALL",  9, 0, 14},
    {"XA",        NULL,            "SQL::XA",         2, 0, 7},
    {NULL,        NULL,            NULL,              0, 0, 0}
};

static int 
sql_syntax_list_len = sizeof(sql_syntax_list) / sizeof(yy_prof_sql_syntax_t) - 1;

static zend_string *
yy_prof__sql_simplify_key(char * key, int key_len) {
    int min, max, mid, cmp_ret, found = 0;
    int tag_len;
	char c, c1;
	char * from = NULL;
	char * tag;
	char * search;
    char tmp_key[YY_PROF_MAX_FINGERPRINT_LEN] = {0};
    yy_prof_sql_syntax_t * sql, * tmp_sql;

    str_copy_and_upper(tmp_key, key, sizeof(tmp_key));

    min = 0; 
    max = sql_syntax_list_len;
    do {
        mid = (min + max) / 2;
        sql = &sql_syntax_list[mid];
        cmp_ret = strncmp(tmp_key, sql->first_word, sql->first_word_len);
        if(cmp_ret > 0) {
            min = mid + 1; 
        } else if(cmp_ret < 0) {
            max = mid - 1;
        } else {
            found = 1;
            break;
        }
    } while (min <= max);

    if(found == 0) {
        return yy_prof__default_simplify_key("SQL::NOTFOUND",
            sizeof("SQL::NOTFOUND") - 1, NULL, 0);
    }

    if(tmp_key[sql->first_word_len] != ' ') {
        if(mid > 0) {
            tmp_sql = sql - 1;
            if(strncmp(tmp_sql->first_word, tmp_key, tmp_sql->first_word_len) == 0
                    && tmp_key[tmp_sql->first_word_len] == ' ') {
                sql --; 
                found = 1;
            }
        }
        if(mid < sql_syntax_list_len - 1) {
            tmp_sql = sql + 1;
            if(strncmp(tmp_sql->first_word, tmp_key, tmp_sql->first_word_len) == 0
                    && tmp_key[tmp_sql->first_word_len] == ' ') {
                sql ++;
                found = 1;
            }
        }
    }

    if(sql->tag_offset == 0 && sql->result != NULL) {
        return yy_prof__default_simplify_key(sql->result,
            sql->result_len, NULL, 0);
    }

    search = tmp_key + sql->tag_offset;
    tag = sql->tag;

    tag_len = tag == NULL ? 0 : strlen(tag);
    do {
        if(tag_len == 0) {
            from = search;
            break;
        }
        if((from = strstr(search, tag)) == NULL) {
            break;
        }
        c = * (from + tag_len);
        if(!is_empty_char(c)) {
            search += tag_len;
            continue;
        }
        if(from == search) {
            from += tag_len;
            break;
        }
        c1 = * (from - 1);
        if(is_empty_char(c1)) {
            from += tag_len;
            break;
        }
        search += tag_len;
    } while(1);

    if(from != NULL) {
        // skip empty chars
        while (is_empty_char(*from) || is_invalid_name_char(*from)) {
            from ++;
        }

        key_len = 0;
        search = from;
        while(!is_empty_char(*search) && !is_invalid_name_char(*search)) {
            search ++;
        }
        key_len = search - from;

        key_len = normalize_table_name(from, key_len);

        return yy_prof__default_simplify_key(from, key_len,
            "SQL:", sizeof("SQL:") - 1);
    } else if(sql->result != NULL) {
        return yy_prof__default_simplify_key(sql->result,
            sql->result_len, NULL, 0);
    }
    return yy_prof__default_simplify_key(key, key_len, NULL, 0);
}

static zend_always_inline zend_string *
yy_prof__url_simplify_key(char * key, int key_len) {
    char * search;
    
    search = strchr(key, '?');
    key_len = search == NULL ? key_len : (search - key);

    return yy_prof__default_simplify_key(key, key_len, NULL, 0);
}

static zend_always_inline zend_string *
yy_prof__default_simplify_key(char * key, int key_len, char * prefix, int prefix_len) {
    int len = 0;
    zend_string * s = NULL;

    len = key_len + prefix_len;
    len = min(len, YY_PROF_MAX_KEY_LEN - 1);

    s = zend_string_alloc(len, 0);
    if(prefix != NULL) {
        memcpy(ZSTR_VAL(s), prefix, prefix_len);
    }
    memcpy(ZSTR_VAL(s) + prefix_len, key, len - prefix_len);
    ZSTR_VAL(s)[len] = '\0';

    return s;
}

static yy_prof_adapter_define_t
yy_prof_adapter_define_list[] =
{
    {""                                 , ADAPTER_TYPE_EQUALS, 0, yy_prof__default},

    {"AMQPConnection::connect"          , ADAPTER_TYPE_EQUALS, 0, yy_prof__amqp},
    {"AMQPConnection::pconnect"         , ADAPTER_TYPE_EQUALS, 0, yy_prof__amqp},
    {"AMQPConnection::preconnect"       , ADAPTER_TYPE_EQUALS, 0, yy_prof__amqp},
    {"AMQPConnection::reconnect"        , ADAPTER_TYPE_EQUALS, 0, yy_prof__amqp},
    {"AMQPExchange::publish"            , ADAPTER_TYPE_EQUALS, 0, yy_prof__amqp},
    {"AMQPQueue::ack"                   , ADAPTER_TYPE_EQUALS, 0, yy_prof__amqp},
    {"AMQPQueue::cancel"                , ADAPTER_TYPE_EQUALS, 0, yy_prof__amqp},
    {"AMQPQueue::consume"               , ADAPTER_TYPE_REQUEST_DETECTOR, 0, yy_prof__amqpqueue_consume},
    {"AMQPQueue::delete"                , ADAPTER_TYPE_EQUALS, 0, yy_prof__amqp},
    {"AMQPQueue::get"                   , ADAPTER_TYPE_EQUALS, 0, yy_prof__amqp},
    {"AMQPQueue::nack"                  , ADAPTER_TYPE_EQUALS, 0, yy_prof__amqp},
    {"AMQPQueue::purge"                 , ADAPTER_TYPE_EQUALS, 0, yy_prof__amqp},
    {"AMQPQueue::reject"                , ADAPTER_TYPE_EQUALS, 0, yy_prof__amqp},

    //{"MongoCollection::aggregate"       , ADAPTER_TYPE_EQUALS, 0, yy_prof__mongo},
    //{"MongoCollection::batchInsert"     , ADAPTER_TYPE_EQUALS, 0, yy_prof__mongo},
    //{"MongoCollection::count"           , ADAPTER_TYPE_EQUALS, 0, yy_prof__mongo},
    //{"MongoCollection::distinct"        , ADAPTER_TYPE_EQUALS, 0, yy_prof__mongo},
    //{"MongoCollection::find"            , ADAPTER_TYPE_EQUALS, 0, yy_prof__mongo},
    //{"MongoCollection::findAndModify"   , ADAPTER_TYPE_EQUALS, 0, yy_prof__mongo},
    //{"MongoCollection::findOne"         , ADAPTER_TYPE_EQUALS, 0, yy_prof__mongo},
    //{"MongoCollection::group"           , ADAPTER_TYPE_EQUALS, 0, yy_prof__mongo},
    //{"MongoCollection::insert"          , ADAPTER_TYPE_EQUALS, 0, yy_prof__mongo},
    //{"MongoCollection::remove"          , ADAPTER_TYPE_EQUALS, 0, yy_prof__mongo},
    //{"MongoCollection::save"            , ADAPTER_TYPE_EQUALS, 0, yy_prof__mongo},
    //{"MongoCollection::update"          , ADAPTER_TYPE_EQUALS, 0, yy_prof__mongo},
    //{"MongoDB::command"                 , ADAPTER_TYPE_EQUALS, 0, yy_prof__mongodb},

    {"MongoDB\\Driver\\Manager::executeBulkWrite", ADAPTER_TYPE_EQUALS, 0, yy_prof__mongodb_execute},
    {"MongoDB\\Driver\\Manager::executeCommand"  , ADAPTER_TYPE_EQUALS, 0, yy_prof__mongodb_execute},
    {"MongoDB\\Driver\\Manager::executeQuery"    , ADAPTER_TYPE_EQUALS, 0, yy_prof__mongodb_execute},
    {"MongoDB\\Driver\\Server::executeBulkWrite" , ADAPTER_TYPE_EQUALS, 0, yy_prof__mongodb_execute},
    {"MongoDB\\Driver\\Server::executeCommand"   , ADAPTER_TYPE_EQUALS, 0, yy_prof__mongodb_execute},
    {"MongoDB\\Driver\\Server::executeQuery"     , ADAPTER_TYPE_EQUALS, 0, yy_prof__mongodb_execute},

    {"PDO::__construct"           , ADAPTER_TYPE_EQUALS, 0, yy_prof__sql_func},
    {"PDO::beginTransaction"           , ADAPTER_TYPE_EQUALS, 0, yy_prof__sql_func},
    {"PDO::commit"                     , ADAPTER_TYPE_EQUALS, 0, yy_prof__sql_func},
    {"PDO::exec"                       , ADAPTER_TYPE_EQUALS, 0, yy_prof__pdo},
    {"PDO::prepare"                    , ADAPTER_TYPE_EQUALS, 1, yy_prof__pdo},
    {"PDO::query"                      , ADAPTER_TYPE_EQUALS, 0, yy_prof__pdo},
    {"PDO::rollBack"                   , ADAPTER_TYPE_EQUALS, 0, yy_prof__sql_func},
    {"PDOStatement::execute"           , ADAPTER_TYPE_EQUALS, 0, yy_prof__stmt_execute},

    
    {"Redis::"                         , ADAPTER_TYPE_STARTS_WITH, 0, yy_prof__redis},
    {"SoapClient::__soapCall"          , ADAPTER_TYPE_EQUALS, 0, yy_prof__soap},
    
    {"Swoole\\Http\\Server::on"        , ADAPTER_TYPE_REQUEST_DETECTOR, 0, yy_prof__swoole_http_server},

    {"Yar_Client::"                    , ADAPTER_TYPE_STARTS_WITH, 0, yy_prof__yar},

    {"Aerospike::"                     , ADAPTER_TYPE_STARTS_WITH, 0, yy_prof__aerospike},

    {"curl_exec"                       , ADAPTER_TYPE_EQUALS, 4, yy_prof__curl},
    {"curl_multi_remove_handle"        , ADAPTER_TYPE_EQUALS, 4, yy_prof__curl_2},
                                                            
    //{"http\\Client::send"              }, ADAPTER_TYPE_EQUALS, 0, yy_prof__http},
    {"http\\Client::getResponse"       , ADAPTER_TYPE_EQUALS, 4, yy_prof__http},

    {"mysqli::__construct"             , ADAPTER_TYPE_EQUALS, 0, yy_prof__mysqli_connect_func_1},
    {"mysqli::autocommit"              , ADAPTER_TYPE_EQUALS, 0, yy_prof__sql_func},
    {"mysqli::begin_transaction"       , ADAPTER_TYPE_EQUALS, 0, yy_prof__sql_func},
    {"mysqli::commit"                  , ADAPTER_TYPE_EQUALS, 0, yy_prof__sql_func},
    {"mysqli::multi_query"             , ADAPTER_TYPE_EQUALS, 0, yy_prof__pdo},
    {"mysqli::prepare"                 , ADAPTER_TYPE_EQUALS, 1, yy_prof__pdo},
    {"mysqli::query"                   , ADAPTER_TYPE_EQUALS, 0, yy_prof__pdo},
    {"mysqli::real_connect"            , ADAPTER_TYPE_EQUALS, 0, yy_prof__mysqli_connect_func_1},
    {"mysqli::real_query"              , ADAPTER_TYPE_EQUALS, 0, yy_prof__pdo},
    {"mysqli::rollback"                , ADAPTER_TYPE_EQUALS, 0, yy_prof__sql_func},

    {"mysqli_autocommit"               , ADAPTER_TYPE_EQUALS, 0, yy_prof__sql_func},
    {"mysqli_begin_transaction"        , ADAPTER_TYPE_EQUALS, 0, yy_prof__sql_func},
    {"mysqli_commit"                   , ADAPTER_TYPE_EQUALS, 0, yy_prof__sql_func},
    {"mysqli_connect"                  , ADAPTER_TYPE_EQUALS, 0, yy_prof__mysqli_connect_func_2},
    {"mysqli_multi_query"              , ADAPTER_TYPE_EQUALS, 0, yy_prof__pdo_2},
    {"mysqli_prepare"                  , ADAPTER_TYPE_EQUALS, 1, yy_prof__pdo_2},
    {"mysqli_query"                    , ADAPTER_TYPE_EQUALS, 0, yy_prof__pdo_2},
    {"mysqli_real_connect"             , ADAPTER_TYPE_EQUALS, 0, yy_prof__mysqli_connect_func_2},
    {"mysqli_real_query"               , ADAPTER_TYPE_EQUALS, 0, yy_prof__pdo_2},
    {"mysqli_rollback"                 , ADAPTER_TYPE_EQUALS, 0, yy_prof__sql_func},

    {"mysqli_stmt::execute"            , ADAPTER_TYPE_EQUALS, 0, yy_prof__stmt_execute},

    {"swoole_http_server::on"          , ADAPTER_TYPE_REQUEST_DETECTOR, 0, yy_prof__swoole_http_server},

    {NULL, 0, 0, NULL}
};

static int 
yy_prof_adapter_define_list_len = sizeof(yy_prof_adapter_define_list) / sizeof(yy_prof_adapter_define_t) - 1;

static zend_always_inline void 
yy_prof_check_server_name(char ** server_name, size_t *server_name_len,
        char * host, size_t host_len) {
    
    char *n = *server_name, *s = host; 
    while((*n >= 'a' && *n <= 'z')  
            || (*n >= 'A' && *n <= 'Z')  
            || (*n >= '0' && *n <= '9')  
            || *n == '-' || *n == '.') { 
        n++; 
    } 
    if((n - *server_name < *server_name_len) && host) { 
        while((*s >= 'a' && *s <= 'z') 
                || (*s >= 'A' && *s <= 'Z') 
                || (*s >= '0' && *s <= '9') 
                || *s == '-' || *s == '.') { 
            s++; 
        } 
        if(s - host == host_len) { 
            *server_name = host; 
            *server_name_len = host_len; 
        } 
    } 
}

zend_string * 
yy_prof_get_current_uri(yy_prof_request_t * request) {
    size_t len, server_name_len, request_uri_len, http_len, host_len, ltrim_slash_len;
    zend_string * uri = NULL;
    char * server_name = NULL, * request_uri = NULL, * https = NULL, * host = NULL;
    if(request->is_cli) {
        if(SG(request_info).path_translated) {
            uri = zend_string_init(SG(request_info).path_translated, strlen(SG(request_info).path_translated), 0);
        } else {
            uri = zend_string_init("UNKNOWN", sizeof("UNKNOWN") - 1, 0);
        }
    } else {
        server_name = sapi_getenv("SERVER_NAME", sizeof("SERVER_NAME") - 1);
        request_uri = sapi_getenv("REQUEST_URI", sizeof("REQUEST_URI") - 1);
        host = sapi_getenv("HTTP_HOST", sizeof("HTTP_HOST") - 1);
        https = sapi_getenv("HTTPS", sizeof("HTTPS") - 1);
        
        if(server_name && request_uri) {
            server_name_len = strlen(server_name);
            request_uri_len = strlen(request_uri);
            host_len = strlen(host);
            http_len = https ? sizeof("https://") - 1 : sizeof("http://") - 1;

            yy_prof_check_server_name(&server_name, &server_name_len, host, host_len);

            // skip the first slash if request_uri with double slash 
            ltrim_slash_len = request_uri[0] == '/' && request_uri[1] == '/' ? 1 : 0;
            request_uri_len -= ltrim_slash_len;
            request_uri += ltrim_slash_len;

            len = http_len + server_name_len + request_uri_len;
            uri = zend_string_alloc(len, 0);
            memcpy(ZSTR_VAL(uri), https ? "https://" : "http://", http_len);
            memcpy(ZSTR_VAL(uri) + http_len, server_name, server_name_len);
            memcpy(ZSTR_VAL(uri) + http_len + server_name_len, request_uri, request_uri_len);
            ZSTR_VAL(uri)[len] = '\0';
        } else {
            uri = zend_string_init("UNKNOWN", sizeof("UNKNOWN") - 1, 0);
        }
    }
    request->uri = uri;
    return uri;
}

smart_str *
yy_prof_func_get_time_stats(yy_prof_entry_t * entry, smart_str * str) {
    int len = 0;
    char buffer[256] = {0};
    if(entry->type == IO_TYPE_URL && entry->ptr != NULL) {
        long code;
        double namelookup_time, connect_time, appconnect_time;
        double pretransfer_time, starttransfer_time, redirect_time, total_time;
        CURL * curl = entry->ptr;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        curl_easy_getinfo(curl, CURLINFO_NAMELOOKUP_TIME, &namelookup_time);
        curl_easy_getinfo(curl, CURLINFO_CONNECT_TIME, &connect_time);
        curl_easy_getinfo(curl, CURLINFO_APPCONNECT_TIME, &appconnect_time);
        curl_easy_getinfo(curl, CURLINFO_PRETRANSFER_TIME, &pretransfer_time);
        curl_easy_getinfo(curl, CURLINFO_STARTTRANSFER_TIME, &starttransfer_time);
        curl_easy_getinfo(curl, CURLINFO_REDIRECT_TIME, &redirect_time);
        curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);
                
        len = sprintf(buffer, "DETAIL: code=%ld n=%f c=%f a=%f p=%f s=%f r=%f t=%f\n",
                code, namelookup_time, connect_time, appconnect_time, 
                pretransfer_time, starttransfer_time, redirect_time, total_time);

        smart_str_appendl(str, buffer, len);
    }
    return str;
}

void 
yy_prof_set_entry_status(yy_prof_entry_t * entry) {
    long header_size = 0, request_size = 0, status = 0;
    if(entry->type == IO_TYPE_URL && entry->ptr != NULL) {
        curl_easy_getinfo(entry->ptr, CURLINFO_RESPONSE_CODE, &status);
        curl_easy_getinfo(entry->ptr, CURLINFO_REQUEST_SIZE, &request_size);
        curl_easy_getinfo(entry->ptr, CURLINFO_HEADER_SIZE, &header_size);
    }
    entry->status = status;
    entry->request_bytes = request_size;
    entry->response_bytes = header_size;
}

static zend_always_inline void 
yy_prof_init_filter(yy_prof_funcs_t * funcs) {
    uint8_t hash_code;
    yy_prof_adapter_t * adapter;

	memset(funcs->filter, 0, sizeof(funcs->filter));
	if(funcs->adapters != NULL) {
		for(adapter = funcs->adapters; adapter->func != NULL; adapter++) {
            hash_code = (uint8_t) ZSTR_HASH(adapter->func);
            uint8_t idx = INDEX_2_BYTE(hash_code);
			funcs->filter[idx] |= INDEX_2_BIT(hash_code);
		}
    }
}


static int
compare_func(const void *p1, const void *p2)
{
    /* The actual arguments to this function are "pointers to
       pointers to char", but strcmp(3) arguments are "pointers
       to char", hence the following cast plus dereference */

    zend_string * a = * (zend_string * const *) p1;
    zend_string * b = * (zend_string * const *) p2;
    return zend_binary_strcmp(ZSTR_VAL(a), ZSTR_LEN(a),
                  ZSTR_VAL(b), ZSTR_LEN(b));
}


/** Convert the PHP array of strings to an emalloced array of strings. Note,
 *  this method duplicates the string data in the PHP array.
 *
 *  @author mpal
 **/
static zend_always_inline zend_string ** 
yy_prof_zval_to_string_arr(zval  *values, size_t *len) {
	size_t count;
	size_t ix = 0;
	zend_string **result = NULL;

    *len = 0;
	if (!values) {
		return NULL;
	}

	if (Z_TYPE_P(values) == IS_ARRAY) {
		HashTable *ht;

		ht = Z_ARRVAL_P(values);
		count = zend_hash_num_elements(ht);

		if((result = (zend_string **)emalloc(sizeof(zend_string *) * (count + 1))) == NULL) {
			return result;
		}

		for (zend_hash_internal_pointer_reset(ht);
				zend_hash_has_more_elements(ht) == SUCCESS;
				zend_hash_move_forward(ht)) {
			int type;
			zval *data;
			type = zend_hash_get_current_key_type(ht);
			if(type == HASH_KEY_IS_LONG) {
				if ((data = zend_hash_get_current_data(ht)) != NULL &&
						Z_TYPE_P(data) == IS_STRING) {
                    result[ix] = zend_string_copy(Z_STR_P(data));
					ix++;
				}
			}
		}
	} else if(Z_TYPE_P(values) == IS_STRING) {
		if((result = (zend_string**)emalloc(sizeof(zend_string*) * 2)) == NULL) {
			return result;
		}
        result[0] = zend_string_copy(Z_STR_P(values));
		ix = 1;
	} else {
		result = NULL;
	}

	/* NULL terminate the array */
	if (result != NULL) {
		result[ix] = NULL;
	}
    *len = ix;

    if(ix > 1) {
        qsort(result, ix, sizeof(zend_string *), compare_func);
    }

	return result;
}


void 
yy_prof_init_default_funcs_ex(yy_prof_funcs_t * funcs) {
    int len = 0;
    yy_prof_adapter_define_t * adapter_define;
    yy_prof_adapter_t * adapter;

    yy_prof_adapter_list_len = yy_prof_adapter_define_list_len;
    
    zend_hash_init(&funcs->ht, yy_prof_adapter_list_len * 1.4, NULL, NULL, 1);

    yy_prof_adapter_list = pemalloc(sizeof(yy_prof_adapter_t) * (yy_prof_adapter_list_len + 1), 1);
    memset(yy_prof_adapter_list, 0, sizeof(yy_prof_adapter_t) * (yy_prof_adapter_list_len + 1));
    for(adapter_define = &yy_prof_adapter_define_list[0],
            adapter = &yy_prof_adapter_list[0];
            adapter_define->func != NULL; 
            adapter_define ++, adapter ++) {
        len = strlen(adapter_define->func);
        adapter->func = zend_string_init(adapter_define->func, len, 1);
        adapter->callback = adapter_define->callback;
        adapter->type = adapter_define->type;
        adapter->flag = adapter_define->flag;

        zend_hash_add_ptr(&funcs->ht, adapter->func, adapter);
    }

    funcs->adapters = yy_prof_adapter_list;
    funcs->adapters_len = yy_prof_adapter_list_len;
    funcs->is_changed = 0;

    yy_prof_init_filter(funcs);
}

void 
yy_prof_clear_default_funcs_ex(yy_prof_funcs_t * funcs) {
    int i, len;
    yy_prof_adapter_t *adapter,  *list;
    
    len = funcs->adapters_len;
    list = funcs->adapters;
    adapter = list;
    for(i = 0; i < len; i ++, adapter ++) {
        ZSTR_RELEASE(adapter->func);
    }
    if(funcs->adapters != NULL) {
        pefree(funcs->adapters, 1);
        funcs->adapters = NULL;
    }

    zend_hash_destroy(&funcs->ht);
    memset(funcs, 0, sizeof(yy_prof_funcs_t));
}


void 
yy_prof_free_request_detector(yy_prof_request_detector_t * detector) {
    if(detector != NULL) {
        if(detector->func != NULL) {
            ZSTR_RELEASE(detector->func);
        } 
        memset(detector, 0, sizeof(yy_prof_request_detector_t));
    }
}

static void
yy_prof_strange_merge_funcs(
        yy_prof_funcs_t * funcs, 
        yy_prof_funcs_t * default_funcs, 
        zend_string ** keys,
        size_t keys_count,
        long flag) {

    int diff;
    size_t new_len, origin_adapters_len;
    size_t keys_idx, dest_idx, origin_adapters_idx;
    zend_bool is_changed = funcs->is_changed;
    zend_bool is_append = (flag & FLAG_APPEND) == FLAG_APPEND;
    yy_prof_adapter_t *origin_adapters, *dest_adapters, * src_adapter, * dest_adapter;

    origin_adapters = funcs->adapters;
    origin_adapters_len = funcs->adapters_len;
    new_len = keys_count + 1 + (is_append ? origin_adapters_len - 1 : 0);
    dest_adapters = emalloc(sizeof(yy_prof_adapter_t) * (new_len + 1));

#define YY_PROF_COPY_STRING(is_changed, str, persistent) \
    (is_changed ? zend_string_copy(str) : zend_string_dup(str, persistent))

    // copy the first default function adapter to dest_adapters
    memcpy(dest_adapters, origin_adapters, sizeof(yy_prof_adapter_t));
    dest_adapters->func = YY_PROF_COPY_STRING(is_changed, dest_adapters->func, 0);

    keys_idx = 0; // new functions
    dest_idx = 1; // dest functions
    origin_adapters_idx = 1; // origin functions

    while(keys_idx < keys_count && origin_adapters_idx < origin_adapters_len) {
        diff = 0; 
        src_adapter = origin_adapters + origin_adapters_idx;
        dest_adapter = dest_adapters + dest_idx;

        switch(src_adapter->type) {
            case ADAPTER_TYPE_STARTS_WITH:
                diff = strncmp(ZSTR_VAL(keys[keys_idx]), ZSTR_VAL(src_adapter->func), ZSTR_LEN(src_adapter->func));
                break;
            case ADAPTER_TYPE_EQUALS:
            default:
                diff = strcmp(ZSTR_VAL(keys[keys_idx]), ZSTR_VAL(src_adapter->func));
                break;
        }

        // if find the function adapter in the origin function adapters
        if(diff == 0) {
            memcpy(dest_adapter, src_adapter, sizeof(yy_prof_adapter_t));
            if(is_append) {
                dest_adapter->func = YY_PROF_COPY_STRING(is_changed, dest_adapter->func, 0);
            } else {
                dest_adapter->func = zend_string_copy(keys[keys_idx]);
                dest_adapter->type = ADAPTER_TYPE_EQUALS;
            }
            keys_idx ++;
            dest_idx ++;
        // if not find the function adapter
        } else if(diff < 0) {
            // copy the default function adapter to dest adapter
            memcpy(dest_adapter, dest_adapters, sizeof(yy_prof_adapter_t));
            dest_adapter->func = zend_string_copy(keys[keys_idx]);
            keys_idx ++;
            dest_idx ++;
        } else {
            if(is_append) {
                memcpy(dest_adapter, src_adapter, sizeof(yy_prof_adapter_t));
                dest_adapter->func = YY_PROF_COPY_STRING(is_changed, dest_adapter->func, 0);
                dest_idx ++;
            }
            origin_adapters_idx ++;
        }
    }

    while(keys_idx < keys_count) {
        dest_adapter = dest_adapters + dest_idx;
        memcpy(dest_adapter, dest_adapters, sizeof(yy_prof_adapter_t));
        dest_adapter->func = zend_string_copy(keys[keys_idx]);
        keys_idx ++;
        dest_idx ++;
    }
    if(is_append) {
        while(origin_adapters_idx < origin_adapters_len) {
            dest_adapter = dest_adapters + dest_idx;
            src_adapter = origin_adapters + origin_adapters_idx;
            memcpy(dest_adapter, src_adapter, sizeof(yy_prof_adapter_t));
            dest_adapter->func = YY_PROF_COPY_STRING(is_changed, dest_adapter->func, 0);
            dest_idx ++;
            origin_adapters_idx ++;
        }
    }

    memset(dest_adapters + dest_idx, 0, sizeof(yy_prof_adapter_t));

    yy_prof_clear_funcs_ex(funcs);

    new_len = dest_idx;
    funcs->adapters = dest_adapters;
    funcs->adapters_len = new_len;

    // init hash table
    zend_hash_init(&funcs->ht, (new_len + 1) * 1.4, NULL, NULL, 0);
    for(dest_idx = 1; dest_idx < new_len; dest_idx ++) {
        dest_adapter = funcs->adapters + dest_idx;
        zend_hash_add_ptr(&funcs->ht, dest_adapter->func, dest_adapter);
    }

    funcs->is_changed = 1;
}

void 
yy_prof_set_funcs_ex(yy_prof_funcs_t * funcs,
        yy_prof_funcs_t * default_funcs, zval * values, long flag) {
    size_t len = 0;
    zend_string ** arr = NULL, ** str;

    if(flag & FLAG_REPLACE) {
        yy_prof_reset_funcs_ex(funcs, default_funcs);
    }

    // use user define adapters
    if(values != NULL) {
        arr = yy_prof_zval_to_string_arr(values, &len);
        if(arr != NULL && len > 0) {
            yy_prof_strange_merge_funcs(funcs, default_funcs, arr, len, flag);

            yy_prof_init_filter(funcs);
        }
        if(arr != NULL) {
            str = arr;
            while(*str != NULL) {
                ZSTR_RELEASE(*str);
                str ++;
            }
            efree(arr);
        }
    }
}

void yy_prof_reset_funcs_ex(yy_prof_funcs_t * funcs, yy_prof_funcs_t * default_funcs){
    yy_prof_clear_funcs_ex(funcs);

    memcpy(funcs, default_funcs, sizeof(yy_prof_funcs_t));
}

void yy_prof_clear_funcs_ex(yy_prof_funcs_t * funcs) {
    yy_prof_adapter_t * adapter = NULL;

    if(funcs->is_changed) {
        for(adapter = funcs->adapters; adapter->func != NULL; adapter ++) {
            ZSTR_RELEASE(adapter->func);
        }

        efree(funcs->adapters);
        zend_hash_destroy(&funcs->ht);
    }

    memset(funcs, 0, sizeof(yy_prof_funcs_t));
}


smart_str * yy_prof_func_get_origin_key(yy_prof_entry_t * entry, smart_str *str) {
    char * uri;
    CURL * curl;
    zend_string * s;

    if(entry->ptr == NULL) {
        return str;
    }

    switch(entry->type) {
        case IO_TYPE_SQL:
            s = (zend_string*) entry->ptr;
            smart_str_appendl(str, "SQL: ", sizeof("SQL: ") - 1);
            smart_str_appendl(str, ZSTR_VAL(s), min(YY_PROF_MAX_FINGERPRINT_LEN, ZSTR_LEN(s)));
            smart_str_appendc(str, '\n');
            break;
        case IO_TYPE_URL:
            curl = (CURL *) entry->ptr;
            if(curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &uri) == CURLE_OK) {
                smart_str_appendl(str, "URL: ", sizeof("URL: ") - 1);
                smart_str_appendl(str, uri, strlen(uri));
                smart_str_appendc(str, '\n');
            }
            break;

        default:
            break;
    }

    return str;
}
