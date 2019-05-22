#ifndef FDS_FILTER_AST_H
#define FDS_FILTER_AST_H
#include <libfds.h>

fds_filter_ast_node_t *ast_create(fds_filter_op_t op, fds_filter_ast_node_t *left, fds_filter_ast_node_t *right);

void ast_destroy(fds_filter_ast_node_t *node);

#endif // FDS_FILTER_AST_H