#ifndef FDS_FILTER_OPTS_H
#define FDS_FILTER_OPTS_H

#include <libfds.h>

#include "array.h"

struct fds_filter_opts_s {
    fds_filter_lookup_callback_t *lookup_callback;
    fds_filter_const_callback_t *const_callback;
    fds_filter_field_callback_t *field_callback;

    struct array_s operations;
};

typedef struct fds_filter_opts_s opts_s;

#endif // FDS_FILTER_OPTS_H
