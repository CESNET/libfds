#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include "error.h"
#include "debug.h"

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

static const struct fds_filter_location NO_LOCATION = { 0, 0, 0, 0 };
static const struct error OUT_OF_MEMORY_ERROR = { "Out of memory!", { 0, 0, 0, 0 } };

struct error_list *
create_error_list()
{
    struct error_list *error_list = calloc(1, sizeof(struct error_list));
    if (error_list == NULL) {
        return NULL;
    }
    return error_list;
}

static void
destroy_errors(struct error_list *error_list)
{
    if (error_list->errors != &OUT_OF_MEMORY_ERROR) {
        for (int i = 0; i < error_list->count; i++) {
            free(error_list->errors[i].message);
        }
        free(error_list->errors);
    }
}

void
destroy_error_list(struct error_list *error_list)
{
    destroy_errors(error_list);
    free(error_list);
}

static struct error *
alloc_error_list_entry(struct error_list *error_list)
{
    int new_count = error_list->count + 1;
    struct error *new_errors = realloc(error_list->errors, sizeof(struct error) * new_count);
    if (new_errors == NULL) {
        no_memory_error(error_list);
        return NULL;
    }
    error_list->count = new_count;
    error_list->errors = new_errors;
    return &error_list->errors[error_list->count - 1];
}

void
add_error_location_message(struct error_list *error_list, struct fds_filter_location location, const char *format, ...)
{
    struct error *error = alloc_error_list_entry(error_list);
    if (error == NULL) {
        return;
    }
    error->location = location;

    va_list args;
    va_start(args, format);
    error->message = str_vprintf(NULL, format, args);
    if (error->message == NULL) {
        no_memory_error(error_list);
        return;
    }
    va_end(args);
}

void
add_error_message(struct error_list *error_list, const char *format, ...)
{

    struct error *error = alloc_error_list_entry(error_list);
    if (error == NULL) {
        return;
    }
    error->location = NO_LOCATION;

    va_list args;
    va_start(args, format);
    error->message = str_vprintf(NULL, format, args);
    if (error->message == NULL) {
        no_memory_error(error_list);
        return;
    }
    va_end(args);
}

void
no_memory_error(struct error_list *error_list)
{
    destroy_errors(error_list);
    error_list->count = 1;
    error_list->errors = (struct error *)&OUT_OF_MEMORY_ERROR;
}




