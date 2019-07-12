#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filter.h"

struct fds_filter_ast_node *
ast_node_create()
{
    struct fds_filter_ast_node *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    node->op = FDS_FILTER_AST_NONE;
    node->parent = NULL;
    node->left = NULL;
    node->right = NULL;
    node->identifier_id = 0;
    node->identifier_name = NULL;
    node->type = FDS_FILTER_TYPE_NONE;
    node->subtype = FDS_FILTER_TYPE_NONE;
	node->identifier_is_constant = 0;
    memset(&node->value, 0, sizeof(node->value));
    node->location.first_line = 0;
    node->location.last_line = 0;
    node->location.first_column = 0;
    node->location.last_column = 0;
    return node;
}

void
ast_destroy(struct fds_filter_ast_node *node)
{
    if (node == NULL) {
        return;
    }
    // TODO: check based on node type
    // TODO: also destroy value etc
    free(node->left);
    free(node->right);
    free(node);
}
