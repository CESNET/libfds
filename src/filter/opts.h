#ifndef FDS_FILTER_OPTS_H
#define FDS_FILTER_OPTS_H

#include <libfds.h>

#include "array.h"

typedef struct fds_filter_opts {
    fds_filter_lookup_cb_t *lookup_cb;
    fds_filter_const_cb_t *const_cb;
    fds_filter_data_cb_t *data_cb;

    array_s op_list;
} fds_filter_opts_t;

#endif // FDS_FILTER_OPTS_H
