#ifndef FDS_FILTER_FILTER_H
#define FDS_FILTER_FILTER_H

#include <libfds.h>
#include "error.h"
#include "evaluate.h"

#if 0
struct evaluate_context {
    int program_counter;
    
    int stack_top;
    int stack_capacity;
    union fds_filter_value *stack;

    int instructions_count;
    struct instruction *instructions;
};
#endif


struct fds_filter {
    struct fds_filter_ast_node *ast;

    fds_filter_lookup_func_t lookup_callback;
    fds_filter_data_func_t data_callback;
    void *data; // The raw data
    void *data_context; // To be filled and operated on by the user

    struct evaluate_context evaluate_context;

    int error_count;
    struct error *errors;
};

#endif // FDS_FILTER_FILTER_H
