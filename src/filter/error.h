#ifndef LIBFDS_FILTER_ERROR_H
#define LIBFDS_FILTER_ERROR_H

#include <stdarg.h>
#include "api.h"
#include "filter.h"

struct error {
    const char *message;
    struct fds_filter_location location;
};

void
error_message(struct fds_filter *filter, const char *format, ...);

void
error_location_message(struct fds_filter *filter, struct fds_filter_ast_node *node, const char *format, ...);

void
error_no_memory(struct fds_filter *filter);

void 
error_destroy(struct fds_filter *filter);

const char *
type_to_str(int type);

const char *
ast_op_to_str(int op);

#endif // LIBFDS_FILTER_ERROR_H
