#include "error.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static char *
str_vprintf(char *str, const char *format, va_list args) 
{
	va_list args_;
	va_copy(args_, args);
    int size_needed = vsnprintf(NULL, 0, format, args_) + 1; // + 1 for the terminating null byte
    int size_now = str != NULL ? strlen(str) : 0; 
    char *new_str = realloc(str, size_now + size_needed);
    if (new_str == NULL) { 
        free(str);
        return NULL;
    }
    vsnprintf(&new_str[size_now], size_needed, format, args);
	va_end(args);
    return new_str;
}

#if 0
static char *
str_printf(char *str, const char *format, ...) 
{
    va_list args;
    va_start(args, format);
    char *new_str = str_vprintf(str, format, args);
    va_end(args);
    return new_str;
}
#endif


static const struct error OUT_OF_MEMORY_ERROR = { "out of memory", { -1, -1, -1, -1 } };

void
error_init(struct fds_filter *filter)
{
    filter->error_count = 0;
    filter->errors = NULL;
}

void
error_location_message(struct fds_filter *filter, struct fds_filter_location location, const char *format, ...)
{
    struct error *new_errors = realloc(filter->errors, (filter->error_count + 1) * sizeof(*new_errors));
    if (new_errors == NULL) {
        error_no_memory(filter);
        return;
    }
    filter->error_count++;
    filter->errors = new_errors;
    struct error *error = &filter->errors[filter->error_count - 1];
    error->location = location;

    va_list args;
    va_start(args, format);
    error->message = str_vprintf(NULL, format, args);
    if (error->message == NULL) {
        error_no_memory(filter);
        return;
    }
    va_end(args);
}   

void
error_message(struct fds_filter *filter, const char *format, ...)
{
    struct error *new_errors = realloc(filter->errors, (filter->error_count + 1) * sizeof(new_errors));
    if (new_errors == NULL) {
        error_no_memory(filter);
        return;
    }
    filter->error_count++;
    filter->errors = new_errors;
    struct error *error = &filter->errors[filter->error_count - 1];
    error->location = (struct fds_filter_location) { -1, -1, -1, -1 };

    va_list args;
    va_start(args, format);
    error->message = str_vprintf(NULL, format, args);
    if (error->message == NULL) {
        error_no_memory(filter);
        return;
    }
    va_end(args);
}   

void
error_no_memory(struct fds_filter *filter)
{
    error_destroy(filter);
    filter->error_count = 1;
    filter->errors = &OUT_OF_MEMORY_ERROR;
}

void 
error_destroy(struct fds_filter *filter)
{   
    if (filter->errors != &OUT_OF_MEMORY_ERROR) {
        for (int i = 0; i < filter->error_count; i++) {
            free(filter->errors[i].message);
        }
        free(filter->errors);
    }
    filter->errors = NULL;
    filter->error_count = 0;
}

const char *
type_to_str(int type)
{
    switch (type) {
    case FDS_FILTER_TYPE_NONE:        return "NONE";
    case FDS_FILTER_TYPE_STR:         return "STR";
    case FDS_FILTER_TYPE_UINT:        return "UINT";
    case FDS_FILTER_TYPE_INT:         return "INT";
    case FDS_FILTER_TYPE_FLOAT:       return "FLOAT";
    case FDS_FILTER_TYPE_BOOL:        return "BOOL";
    case FDS_FILTER_TYPE_IP_ADDRESS:  return "IP_ADDRESS";
    case FDS_FILTER_TYPE_MAC_ADDRESS: return "MAC_ADDRESS";
    default:                          assert(0);
    }
}

const char *
ast_op_to_str(int op)
{
    switch (op) {
    case FDS_FILTER_AST_NONE:       return "NONE";
    case FDS_FILTER_AST_ADD:        return "ADD";
    case FDS_FILTER_AST_MUL:        return "MUL";
    case FDS_FILTER_AST_SUB:        return "SUB";
    case FDS_FILTER_AST_DIV:        return "DIV";
    case FDS_FILTER_AST_MOD:        return "MOD";
    case FDS_FILTER_AST_UMINUS:     return "UMINUS";
    case FDS_FILTER_AST_BITNOT:     return "BITNOT";
    case FDS_FILTER_AST_BITAND:     return "BITAND";
    case FDS_FILTER_AST_BITOR:      return "BITOR";
    case FDS_FILTER_AST_BITXOR:     return "BITXOR";
    case FDS_FILTER_AST_NOT:        return "NOT";
    case FDS_FILTER_AST_AND:        return "AND";
    case FDS_FILTER_AST_OR:         return "OR";
    case FDS_FILTER_AST_EQ:         return "EQ";
    case FDS_FILTER_AST_NE:         return "NE";
    case FDS_FILTER_AST_GT:         return "GT";
    case FDS_FILTER_AST_LT:         return "LT";
    case FDS_FILTER_AST_GE:         return "GE";
    case FDS_FILTER_AST_LE:         return "LE";
    case FDS_FILTER_AST_CONST:      return "CONST";
    case FDS_FILTER_AST_IDENTIFIER: return "IDENTIFIER";
    case FDS_FILTER_AST_LIST:       return "LIST";
    case FDS_FILTER_AST_LIST_ITEM:  return "LIST_ITEM";
    case FDS_FILTER_AST_IN:         return "IN";
    case FDS_FILTER_AST_CONTAINS:   return "CONTAINS";
    case FDS_FILTER_AST_CAST:       return "CAST";
    default:                        assert(0);
    }
}
