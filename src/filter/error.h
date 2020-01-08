#ifndef FDS_FILTER_ERROR_H
#define FDS_FILTER_ERROR_H

#include <stdio.h>
#include <stdarg.h>

#include <libfds.h>

#define ERR_OK       FDS_OK
#define ERR_NOMEM    FDS_FILTER_ERR_NOMEM
#define ERR_LEXICAL  FDS_FILTER_ERR_LEXICAL
#define ERR_SYNTAX   FDS_FILTER_ERR_SYNTAX
#define ERR_SEMANTIC FDS_FILTER_ERR_SEMANTIC

typedef fds_filter_error_s *error_t;

extern const error_t NO_ERROR;
extern const error_t MEMORY_ERROR;

error_t
create_error_v(int code, const char *fmt, va_list args);

error_t
create_error(int code, const char *fmt, ...);

void
destroy_error(error_t error);

error_t
create_error_with_location(int code, const char *begin, const char *end, const char *fmt, ...);

#define LEXICAL_ERROR(CURSOR, ...) \
    create_error_with_location(FDS_ERR_SYNTAX, (CURSOR), (CURSOR) + 1, "lexical error: " __VA_ARGS__)

#define SYNTAX_ERROR(TOKEN, ...) \
    create_error_with_location(FDS_ERR_SYNTAX, (TOKEN)->cursor_begin, (TOKEN)->cursor_end, "syntax error: " __VA_ARGS__)

#define SEMANTIC_ERROR(AST, ...) \
    create_error_with_location(FDS_ERR_SEMANTIC, (AST)->cursor_begin, (AST)->cursor_end, "semantic error: " __VA_ARGS__)


#endif // FDS_FILTER_ERROR_H