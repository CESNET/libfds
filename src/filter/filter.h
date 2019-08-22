#ifndef FDS_FILTER_FILTER_H
#define FDS_FILTER_FILTER_H

#include <stdbool.h>
#include <setjmp.h>
#include <libfds.h>
#include "error.h"

struct fds_filter {
    struct fds_filter_ast_node *ast;

    fds_filter_lookup_callback_t lookup_callback;
    fds_filter_const_callback_t const_callback;
    fds_filter_field_callback_t field_callback;
    void *data; // The raw data
    void *user_context; // To be filled and operated on by the user
    bool reset_context;

    struct eval_node *eval_tree;
    jmp_buf eval_jmp_buf;

    struct error_list *error_list;
};

int
preprocess(struct fds_filter *filter);

int
semantic_analysis(struct fds_filter *filter);

int
optimize(struct fds_filter *filter);


#endif // FDS_FILTER_FILTER_H
