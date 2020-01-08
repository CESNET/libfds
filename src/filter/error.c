#include "error.h"

const fds_filter_error_s memory_error = { .code = FDS_ERR_NOMEM, .msg = "out of memory" };

const error_t NO_ERROR = NULL;
const error_t MEMORY_ERROR = &memory_error;

error_t
create_error_v(int code, const char *fmt, va_list args)
{
    va_list args_;
    va_copy(args_, args);
    size_t msg_len = vsnprintf(NULL, 0, fmt, args_);
    error_t err = malloc(sizeof(fds_filter_error_s));
    if (!err) { 
        va_end(args_);
        return MEMORY_ERROR;
    }
    char *msg = malloc(msg_len + 1);
    if (!msg) {
        va_end(args_);
        free(err);
        return MEMORY_ERROR;
    }
    va_end(args_);
    va_copy(args_, args);
    vsnprintf(msg, msg_len + 1, fmt, args_);
    msg[msg_len] = '\0';
    err->msg = msg;
    err->code = code;
    va_end(args_);
    return err;
}

error_t
create_error(int code, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    error_t err = create_error_v(code, fmt, args);
    va_end(args);
    return err;
}

error_t
create_error_with_location(int code, const char *begin, const char *end, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    error_t err = create_error_v(code, fmt, args);
    va_end(args);
    if (err == MEMORY_ERROR) {
        return err;
    }
    err->cursor_begin = begin;
    err->cursor_end = end;
    return err;
}

void
destroy_error(error_t error)
{
    if (error != NULL && error != NO_ERROR && error != MEMORY_ERROR) {
        free(error->msg);
        free(error);
    }
}
