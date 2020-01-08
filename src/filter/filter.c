#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libfds.h>

#include "filter.h"
#include "ast_common.h"
#include "eval_common.h"
#include "opts.h"
#include "error.h"
#include "opts.h"
#include "parser.h"
#include "scanner.h"

int
fds_filter_create(fds_filter_t **filter, const char *expr, fds_filter_opts_t *opts)
{
    *filter = calloc(1, sizeof(fds_filter_t));
    if (!*filter) {
        return FDS_ERR_NOMEM;
    }

    struct scanner_s scanner;
    init_scanner(&scanner, expr);

    printf("----- parse -----\n");
    (*filter)->error = parse_filter(&scanner, &(*filter)->ast);
    if ((*filter)->error != NO_ERROR) {
        destroy_ast((*filter)->ast);
        (*filter)->ast = NULL;
        return (*filter)->error->code;
    }
    print_ast(stdout, (*filter)->ast);

    printf("----- semantic -----\n");
    (*filter)->error = resolve_types((*filter)->ast, opts);
    if ((*filter)->error != NO_ERROR) {
        destroy_ast((*filter)->ast);
        (*filter)->ast = NULL;
        return (*filter)->error->code;
    }
    print_ast(stdout, (*filter)->ast);

    printf("----- generator -----\n");
    (*filter)->error = generate_eval_tree((*filter)->ast, opts, &(*filter)->eval_root);
    if ((*filter)->error != NO_ERROR) {
        destroy_ast((*filter)->ast);
        (*filter)->ast = NULL;
        return (*filter)->error->code;
    }
    (*filter)->eval_runtime.data_cb = opts->data_cb;
    print_eval_tree(stdout, (*filter)->eval_root);

    return FDS_OK;
}

bool
fds_filter_eval(fds_filter_t *filter, void *data)
{
    printf("----- evaluate -----\n");
    filter->eval_runtime.data = data;
    evaluate_eval_tree(filter->eval_root, &filter->eval_runtime);
    print_eval_tree(stdout, filter->eval_root);
    return filter->eval_root->value.b;
}

void
fds_filter_destroy(fds_filter_t *filter)
{
    if (!filter) {
        return;
    }
    destroy_eval_tree(filter->eval_root);
    destroy_ast(filter->ast);
    destroy_error(filter->error);
    free(filter);
}

void
fds_filter_set_user_ctx(fds_filter_t *filter, void *user_ctx)
{
    filter->eval_runtime.user_ctx = user_ctx;
}

void *
fds_filter_get_user_ctx(fds_filter_t *filter)
{
    return filter->eval_runtime.user_ctx;
}

error_t
fds_filter_get_error(fds_filter_t *filter)
{
    if (!filter) {
        return MEMORY_ERROR;
    }
    return filter->error;
}