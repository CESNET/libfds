#ifndef FDS_FILTER_EVAL_GENERATOR_H
#define FDS_FILTER_EVAL_GENERATOR_H

#include "ast_common.h"
#include "eval_common.h"
#include "opts.h"
#include "error.h"

error_t
generate_eval_tree(ast_node_s *ast, opts_s *opts, struct fds_filter_eval_s **out_eval);

#endif // FDS_FILTER_EVAL_GENERATOR_H
