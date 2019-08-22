#ifndef FDS_FANT_UTILS_H
#define FDS_FANT_UTILS_H

#include <libfds.h>
#include "filter.h"

struct fds_filter_ast_node *
create_ast_node();

void
destroy_ast(struct fds_filter_ast_node *node);

int
apply_to_all_ast_nodes(int (*function)(struct fds_filter *, struct fds_filter_ast_node **),
                   struct fds_filter *filter, struct fds_filter_ast_node **node_ptr);

static inline bool
is_binary_ast_node(struct fds_filter_ast_node *node)
{
    return node->left != NULL && node->right != NULL;
}

static inline bool
is_unary_ast_node(struct fds_filter_ast_node *node)
{
    return node->left != NULL && node->right == NULL;
}

static inline bool
is_leaf_ast_node(struct fds_filter_ast_node *node)
{
    return node->left == NULL && node->right == NULL;
}

static inline bool
is_constant_ast_node(struct fds_filter_ast_node *node)
{
    return node->node_type == FDS_FANT_CONST
        || (node->node_type == FDS_FANT_IDENTIFIER && node->identifier_type == FDS_FIT_CONST);
}

static inline bool
is_list_of_type(struct fds_filter_ast_node *node, enum fds_filter_data_type type)
{
    return node->data_type == FDS_FDT_LIST && node->data_subtype == type;
}

static inline bool
is_number_type(enum fds_filter_data_type type)
{
    return type == FDS_FDT_INT || type == FDS_FDT_UINT || type == FDS_FDT_FLOAT;
}

static inline bool
is_integer_number_type(enum fds_filter_data_type type)
{
    return type == FDS_FDT_INT || type == FDS_FDT_UINT;
}

static inline bool
both_children_of_type(struct fds_filter_ast_node *node, enum fds_filter_data_type type)
{
    return node->left->data_type == type && node->right->data_type == type;
}

#endif // FDS_FANT_UTILS_H