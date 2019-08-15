#ifndef FDS_FILTER_AST_UTILS_H
#define FDS_FILTER_AST_UTILS_H

#include <libfds.h>

static inline bool
ast_is_binary_node(struct fds_filter_ast_node *node)
{
    return node->left != NULL && node->right != NULL;
}

static inline bool
ast_is_unary_node(struct fds_filter_ast_node *node)
{
    return node->left != NULL && node->right == NULL;
}

static inline bool
ast_is_leaf_node(struct fds_filter_ast_node *node)
{
    return node->left == NULL && node->right == NULL;
}

static inline bool
ast_is_constant_node(struct fds_filter_ast_node *node)
{
    return node->op == FDS_FILTER_AST_CONST
        || (node->op == FDS_FILTER_AST_IDENTIFIER && node->identifier_type == FDS_FILTER_IDENTIFIER_CONST);
}

static inline bool
ast_has_list_of_type(struct fds_filter_ast_node *node, enum fds_filter_data_type type)
{
    return node->type == FDS_FILTER_TYPE_LIST && node->subtype == type;
}

static inline bool
type_is_number(enum fds_filter_data_type type)
{
    return type == FDS_FILTER_TYPE_INT || type == FDS_FILTER_TYPE_UINT || type == FDS_FILTER_TYPE_FLOAT;
}

static inline bool
type_of_both_children(struct fds_filter_ast_node *node, enum fds_filter_data_type type)
{
    return node->left->type == type && node->right->type == type;
}

static inline int
ast_apply_to_all_nodes(int (*function)(struct fds_filter *, struct fds_filter_ast_node **),
                   struct fds_filter *filter, struct fds_filter_ast_node **node_ptr)
{
    if (*node_ptr == NULL) {
        return FDS_FILTER_OK;
    }

    int return_code;

    return_code = ast_apply_to_all_nodes(function, filter, &(*node_ptr)->left);
    if (return_code != FDS_FILTER_OK) {
        return return_code;
    }

    return_code = ast_apply_to_all_nodes(function, filter, &(*node_ptr)->right);
    if (return_code != FDS_FILTER_OK) {
        return return_code;
    }

    return_code = function(filter, node_ptr);
    if (return_code != FDS_FILTER_OK) {
        return return_code;
    }

    return FDS_FILTER_OK;
}

#endif // FDS_FILTER_AST_UTILS_H