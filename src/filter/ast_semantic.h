#ifndef FDS_FILTER_SEMANTIC_H
#define FDS_FILTER_SEMANTIC_H

#include "ast_common.h"
#include "opts.h"
#include "error.h"

error_t
resolve_types(ast_node_s *ast, opts_s *opts);

#endif FDS_FILTER_SEMANTIC_H