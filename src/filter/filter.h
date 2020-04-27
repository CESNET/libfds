#ifndef FDS_FILTER_H
#define FDS_FILTER_H

#include <libfds.h>

#include "ast_common.h"
#include "eval_common.h"

struct fds_filter {
    fds_filter_ast_node_s *ast;
    fds_filter_opts_t *opts;
    eval_node_s *eval_root;
    eval_runtime_s eval_runtime;
    fds_filter_error_s *error;
};

#endif