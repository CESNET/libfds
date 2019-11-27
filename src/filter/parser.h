#ifndef FDS_FILTER_PARSER_H
#define FDS_FILTER_PARSER_H

#include "ast_common.h"
#include "error.h"
#include "scanner.h"

error_t
parse_filter(struct scanner_s *scanner, ast_node_s **ast);

#endif