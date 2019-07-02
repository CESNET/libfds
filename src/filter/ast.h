#ifndef FDS_FILTER_AST_H
#define FDS_FILTER_AST_H

#include "api.h"

struct fds_filter_ast_node *
ast_node_create();

void 
ast_destroy(struct fds_filter_ast_node *node);

void 
ast_print(FILE *outstream, struct fds_filter_ast_node *node);


#endif // FDS_FILTER_AST_H