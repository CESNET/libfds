#ifndef FDS_FILTER_SCANNER_H
#define FDS_FILTER_SCANNER_H

#include <stdio.h>
#include <stdbool.h>

#include "util.h"
#include "values.h"
#include "error.h"

enum token_kind_e {
    TK_NONE = 0,
    TK_LITERAL,
    TK_SYMBOL,
    TK_NAME,
    TK_END
};

struct token_s {
    enum token_kind_e kind;
    union {
        struct {
            int data_type;
            fds_filter_value_u value;
        } literal;
        const char *symbol;
        char *name;
    };
    const char *cursor_begin;
    const char *cursor_end;
};

struct scanner_s {
    const char *input;
    const char *cursor;
    struct token_s token;
    bool token_ready;
};

void
init_scanner(struct scanner_s *scanner, const char *input);

error_t
next_token(struct scanner_s *scanner, struct token_s *token);

void
consume_token(struct scanner_s *scanner);

bool
token_is(struct token_s token, enum token_kind_e kind);

bool
token_is_symbol(struct token_s token, const char *symbol);

void
destroy_token(struct token_s token);

void
print_token(FILE *out, struct token_s *token);

#endif