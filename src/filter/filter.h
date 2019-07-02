#ifndef FDS_FILTER_FILTER_H
#define FDS_FILTER_FILTER_H

#include "error.h"
#include "api.h"
#include "interpreter.h"

struct fds_filter {
    struct fds_filter_ast_node *ast;

    fds_filter_lookup_func_t lookup_callback;
    fds_filter_data_func_t data_callback;
    void *data; // The raw data
    void *data_context; // To be filled and operated on by the user

    int error_count;
    struct error *errors;
};

#endif // FDS_FILTER_FILTER_H
