#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libfds.h>

#include "filter.h"
#include "ast_common.h"
#include "ast_semantic.h"
#include "eval_common.h"
#include "eval_generator.h"
#include "eval_evaluator.h"
#include "opts.h"
#include "error.h"
#include "opts.h"
#include "parser.h"
#include "scanner.h"

int
fds_filter_compile(fds_filter_t **filter, const char *input_expr, fds_filter_opts_t *opts)
{
    *filter = calloc(1, sizeof(fds_filter_t));
    if (!*filter) {
        return ERR_NOMEM;
    }

    struct scanner_s scanner;
    init_scanner(&scanner, input_expr);

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
    (*filter)->error = generate_eval_tree((*filter)->ast, opts, &(*filter)->eval);
    if ((*filter)->error != NO_ERROR) {
        destroy_ast((*filter)->ast);
        (*filter)->ast = NULL;
        return (*filter)->error->code;
    }
    print_eval_tree(stdout, (*filter)->eval->root);

    return ERR_OK;
}

int
fds_filter_evaluate(fds_filter_t *filter, void *data)
{
    printf("----- evaluate -----\n");
    filter->eval->ctx.data = data;
    evaluate(filter->eval);
    print_eval_tree(stdout, filter->eval->root);
    return filter->eval->root->value.b ? FDS_FILTER_YES : FDS_FILTER_NO;
}

void
fds_filter_destroy(fds_filter_t *filter)
{
    if (!filter) {
        return;
    }
    destroy_eval(filter->eval);
    destroy_ast(filter->ast);
    destroy_error(filter->error);
    free(filter);
}

void
fds_filter_set_user_ctx(fds_filter_t *filter, void *user_ctx)
{
    filter->eval->ctx.user_ctx = user_ctx;
}

void *
fds_filter_get_user_ctx(fds_filter_t *filter)
{
    return filter->eval->ctx.user_ctx;
}

error_t
fds_filter_get_error(fds_filter_t *filter)
{
    if (!filter) {
        return MEMORY_ERROR;
    }
    return filter->error;
}