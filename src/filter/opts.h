#ifndef FDS_FILTER_OPTS_H
#define FDS_FILTER_OPTS_H

#include <libfds.h>

typedef struct fds_filter_opts {
    fds_filter_lookup_cb_t *lookup_cb;
    fds_filter_const_cb_t *const_cb;
    fds_filter_data_cb_t *data_cb;
    fds_filter_op_s *op_list;
    void *user_ctx;
} fds_filter_opts_t;

fds_filter_opts_t *
fds_filter_opts_copy(fds_filter_opts_t *original_opts);

#endif // FDS_FILTER_OPTS_H
