#ifndef __YY_PROF_ARGS_H__
#define __YY_PROF_ARGS_H__

#include <stdio.h>
#include "Zend/zend_generators.h"
#include "include/yy_structs.h"

static zend_always_inline zend_string *
yy_prof_get_base_filename(const zend_string *filename);
static zend_always_inline zend_string *
yy_prof_get_func_name_by_zval(zval * z TSRMLS_DC);
static zend_always_inline zend_string * 
yy_prof_get_func_name(zend_execute_data * execute_data TSRMLS_DC);
static zend_always_inline zend_string * 
yy_prof_func_get_first_arg(TSRMLS_D);

/**
 * Takes an input of the form /a/b/c/d/foo.php and returns
 * a pointer to one-level directory and basefile name
 * (d/foo.php) in the same string.
 */
static zend_always_inline zend_string *
yy_prof_get_base_filename(const zend_string *filename) {
    int found = 0;
    size_t len;
    const char * str;
    zend_string * ret;

    if(filename == NULL) {
        return NULL;
    }

    for(str = ZSTR_VAL(filename) + ZSTR_LEN(filename) - 1; 
            str >= ZSTR_VAL(filename); str --) {
        if(*str == '/') {
            found ++;
            if(found == 3){
                len = ZSTR_VAL(filename) + ZSTR_LEN(filename) - (str + 1);
                ret = zend_string_alloc(len, 0);
                memcpy(ZSTR_VAL(ret), str + 1, len);
                ZSTR_VAL(ret)[len] = '\0';
                ZSTR_LEN(ret) = len;
                return ret;
            }
        }
    }
    return zend_string_copy((zend_string*)filename);
}

static zend_always_inline zend_string *
yy_prof_get_func_name_by_zval(zval * z TSRMLS_DC) {
    size_t len;
    zend_function * function = NULL;
    zend_string * function_name = NULL, * filename;
    if(z == NULL) {
        return NULL;
    }
    if(Z_TYPE_P(z) == IS_STRING) {
        return zend_string_copy(Z_STR_P(z));
    }
    if(zend_is_callable(z, IS_CALLABLE_CHECK_SILENT, &function_name)) {
        if(zend_binary_strcmp(ZSTR_VAL(function_name), ZSTR_LEN(function_name),
                    "Closure::__invoke", sizeof("Closure::__invoke") - 1) == 0) {
            zend_string_release(function_name);
            function = (zend_function*) (Z_OBJ_P(z) + 1);
            if(function && function->type == ZEND_USER_FUNCTION 
                    && function->op_array.filename != NULL
                    && function->op_array.line_start > 0) {
                filename = yy_prof_get_base_filename(function->op_array.filename);
                len = ZSTR_LEN(filename) + 64;
                function_name = zend_string_alloc(len, 0);
                snprintf(ZSTR_VAL(function_name), ZSTR_LEN(function_name), "%s:%s:%u:%u",
                    "Closure", ZSTR_VAL(filename),
                    function->op_array.line_start, function->op_array.line_end);
                ZSTR_LEN(function_name) = strlen(ZSTR_VAL(function_name));

                zend_string_release(filename);
                return function_name;
            }
            
            return NULL;
        } else {
            return function_name; 
        }
    }
    return NULL;
}

/**
 * Get the name of the current function. The name is qualified with
 * the class name if the function is in a class.
 *
 * @author kannan, hzhao
 */
static zend_always_inline zend_string * 
yy_prof_get_func_name(zend_execute_data * execute_data TSRMLS_DC) {
    int len;
    zend_string *ret = NULL;
    const char * class_name = NULL;
    const char * function_name = NULL;
    zend_object * object = NULL;
    zend_function * func = NULL;
    zend_execute_data * call = NULL, * ptr = NULL;

    call = ptr = execute_data;

    if(call == NULL) {
        return NULL;
    }

    ptr = zend_generator_check_placeholder_frame(ptr);

    object = Z_OBJ(call->This);

    if(call->func) {
        func = call->func;
        function_name = (func->common.scope &&
                         func->common.scope->trait_aliases) ?
            ZSTR_VAL(zend_resolve_method_name(
                (object ? object->ce : func->common.scope), func)) :
            (func->common.function_name ?
                ZSTR_VAL(func->common.function_name) : NULL);
    } else {
        func = NULL;
        function_name = NULL;
    }

    if(function_name) {
        if (object) {
            if (func->common.scope) {
                class_name = ZSTR_VAL(func->common.scope->name);
            } else {
                class_name = ZSTR_VAL(object->ce->name);
            }
        } else if (func->common.scope) {
            class_name = ZSTR_VAL(func->common.scope->name);
        } else {
            class_name = NULL;
        }
    } else {
        if (!ptr->func || !ZEND_USER_CODE(ptr->func->common.type) 
                || ptr->opline->opcode != ZEND_INCLUDE_OR_EVAL) {
            function_name = NULL;
        } else {
            switch (call->opline->extended_value) {
                case ZEND_EVAL:
                    function_name = "eval";
                    break;
                case ZEND_INCLUDE:
                    function_name = "include";
                    break;
                case ZEND_REQUIRE:
                    function_name = "require";
                    break;
                case ZEND_INCLUDE_ONCE:
                    function_name = "include_once";
                    break;
                case ZEND_REQUIRE_ONCE:
                    function_name = "require_once";
                    break;
                default:
                    function_name = NULL;
                    break;
            }
        }
    }

    len = 0;
    if(class_name != NULL) {
        len += strlen(class_name) + 2;
    }
    if(function_name != NULL) {
        len += strlen(function_name);
    }
    if(len == 0) {
        return NULL;
    }


    if(class_name == NULL) {
        ret = zend_string_init(function_name, len, 0);
    } else {
        ret = zend_string_alloc(len, 0);
        ZSTR_H(ret) = 0;
        snprintf(ZSTR_VAL(ret), len + 1, "%s::%s", 
                class_name, function_name);
    }

    return ret;
}

static zend_always_inline zend_string * 
yy_prof_func_get_first_arg(TSRMLS_D) {
    zval * z;
	zend_execute_data * call;

	call = EG(current_execute_data);

    if(!call || ZEND_CALL_NUM_ARGS(call) < 1) {
        return NULL;
    }

    z = ZEND_CALL_ARG(call, 1);

    if(Z_TYPE_P(z) == IS_STRING) {
        return zend_string_dup(Z_STR_P(z), 0);
    }

    return NULL;
}

#endif
