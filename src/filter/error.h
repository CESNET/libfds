#ifndef LIBFDS_FILTER_ERROR_H
#define LIBFDS_FILTER_ERROR_H

#include <libfds.h>
#include "filter.h"
#include <stdarg.h>
#include <stdio.h>

#ifndef NDEBUG
#define pdebug(fmt, ...) \
        do { fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__, __VA_ARGS__); } while (0)
#else
#define pdebug(fmt, ...) /* nothing */
#endif

struct error {
    const char *message;
    struct fds_filter_location location;
};

void
error_message(struct fds_filter *filter, const char *format, ...);

void
error_location_message(struct fds_filter *filter, struct fds_filter_location location, const char *format, ...);

void
error_no_memory(struct fds_filter *filter);

void 
error_destroy(struct fds_filter *filter);

const char *
type_to_str(int type);

const char *
ast_op_to_str(int op);

#endif // LIBFDS_FILTER_ERROR_H
