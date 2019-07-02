#include "semantic.h"
#include "ast.h"
#include "error.h"

static int
is_binary_node(struct fds_filter_ast_node *node) 
{
    return node->left != NULL && node->right != NULL;
}

static int
is_unary_node(struct fds_filter_ast_node *node) 
{
    return node->left != NULL && node->right == NULL;
}

static int
is_leaf_node(struct fds_filter_ast_node *node) 
{
    return node->left == NULL && node->right == NULL;
}

static int
is_number_type(enum fds_filter_type type)
{
    return type == FDS_FILTER_TYPE_INT || type == FDS_FILTER_TYPE_UINT || type == FDS_FILTER_TYPE_FLOAT;
}

static enum fds_filter_type
get_common_number_type(enum fds_filter_type left, enum fds_filter_type right)
{
	if      (left == right)                                                   { return left;                  }
    else if (left == FDS_FILTER_TYPE_FLOAT && right == FDS_FILTER_TYPE_INT  ) { return FDS_FILTER_TYPE_FLOAT; }
    else if (left == FDS_FILTER_TYPE_FLOAT && right == FDS_FILTER_TYPE_UINT ) { return FDS_FILTER_TYPE_FLOAT; }
    else if (left == FDS_FILTER_TYPE_INT   && right == FDS_FILTER_TYPE_FLOAT) { return FDS_FILTER_TYPE_FLOAT; }
    else if (left == FDS_FILTER_TYPE_UINT  && right == FDS_FILTER_TYPE_FLOAT) { return FDS_FILTER_TYPE_FLOAT; }
    else if (left == FDS_FILTER_TYPE_INT   && right == FDS_FILTER_TYPE_UINT ) { return FDS_FILTER_TYPE_INT;   }
    else if (left == FDS_FILTER_TYPE_UINT  && right == FDS_FILTER_TYPE_INT  ) { return FDS_FILTER_TYPE_INT;   }
	else                                                                      { return FDS_FILTER_TYPE_NONE;  }
}

static int
cast_node(struct fds_filter *filter, struct fds_filter_ast_node **node, enum fds_filter_type to)
{
    if (*node->type == to) {
        return 1;
    }

    struct fds_filter_ast_node *new_node = ast_new();
    if (new_node == NULL) {
        error_no_memory(filter);
        return 0;
    }
    new_node->type = to;
    new_node->op = FDS_FILTER_AST_CAST;
    new_node->left = *node;
    *node = new_node;
    return 0;
}

static int
resolve_number_type_left_right(struct fds_filter *filter, struct fds_filter_ast_node *node)
{
    enum fds_filter_type type = get_common_number_type(node->left->type, node->right->type);
    if (type == FDS_FILTER_TYPE_NONE) {
        error_location_message(filter, node, "Cannot cast numbers of type %s and %s to a common type",
                               type_to_str(node->left->type), type_to_str(node->right->type));
        return 0;
    }
    if (!cast_node(filter, &node->left, type)) {
        return 0;
    }
    if (!cast_node(filter, &node->left, type)) {
        return 0;
    }
    assert(node->left->type == node->right->type);
    return 1;
}

static int
resolve_list_types(struct fds_filter *filter, struct fds_filter_node *node)
{
    enum fds_filter_type final_type = node->left->type;
    struct fds_filter_node *list_item = node;
    
    while ((list_item = list_item->right) != NULL) {
        if (is_number_type(list_item->left->type)) {
            enum fds_filter_type type = get_common_number_type(final_type, list_item->left->type);
            if (type == FDS_FILTER_AST_NONE) {
                error_location_message(filter, node, "Cannot cast items of list to the same type - no common type for values of type %s and %s",
                                       type_to_str(type), type_to_str(list_item->left->type));
                return 0;
            }
            final_type = type;

        } else if (list_item->left->type != final_type) {
            error_location_message(filter, list_item, "Cannot cast items of list to the same type - no common type for values of type %s and %s",
                                   type_to_str(final_type), type_to_str(list_item->left->type));
            return 0;
        }
    }

    list_item = node;
    while (list_item != NULL) {
        if (!cast_node(filter, &list_item->left, final_type)) {
            return 0;
        }
        list_item = list_item->right;
    }

    return 1;
}

int 
resolve_types(struct fds_filter *filter, struct fds_filter_ast_node *node)
{
    if (is_binary_node(node) && is_number_type(node->left->type) && is_number_type(node->right->type)) {
        if (!resolve_number_type_left_right(filter, node)) {
            return 1;
        }
        switch (node->op) {
        case FDS_FILTER_AST_ADD:
        case FDS_FILTER_AST_SUB:
        case FDS_FILTER_AST_MUL:
        case FDS_FILTER_AST_DIV:
            node->type = node->left->type;
            break;
        case FDS_FILTER_AST_EQ:
        case FDS_FILTER_AST_NE:
        case FDS_FILTER_AST_LT:
        case FDS_FILTER_AST_GT:
        case FDS_FILTER_AST_LE:
        case FDS_FILTER_AST_GE:
            node->type = FDS_FILTER_TYPE_BOOL;
            break;
        default:
            goto invalid_operation;
        }

    } else if (is_binary_node(node) && node->left->type == FDS_FILTER_TYPE_STR && node->right->type == FDS_FILTER_TYPE_STR) {
        switch (node->op) {
        case FDS_FILTER_AST_ADD:
            node->type = FDS_FILTER_TYPE_STR;
            break;
        case FDS_FILTER_AST_EQ:
        case FDS_FILTER_AST_NE:
            node->type = FDS_FILTER_TYPE_BOOL;
            break;
        default:
            goto invalid_operation;
        }

    } else if (is_binary_node(node) && node->left->type == FDS_FILTER_TYPE_IP_ADDRESS && node->right->type == FDS_FILTER_TYPE_IP_ADDRESS) {
        switch (node->op) {
        case FDS_FILTER_AST_EQ:
        case FDS_FILTER_AST_NE:
            node->type = FDS_FILTER_TYPE_BOOL;
            break;
        default:
            goto invalid_operation;
        }

    } else if (is_binary_node(node) && node->left->type == FDS_FILTER_TYPE_MAC_ADDRESS && node->right->type == FDS_FILTER_TYPE_MAC_ADDRESS) {
        switch (node->op) {
        case FDS_FILTER_AST_EQ:
        case FDS_FILTER_AST_NE:
            node->type = FDS_FILTER_TYPE_BOOL;
            break;
        default:
            goto invalid_operation;
        }
    
    } else if (is_unary_node(node) && is_number_type(node->left->type)) {
        switch (node->op) {
        case FDS_FILTER_AST_UMINUS:
            if (node->left->type == FDS_FILTER_TYPE_UINT) {
                if (!cast_node(filter, &node->left, FDS_FILTER_TYPE_INT)) {
                    return 0;
                }
            }
            node->type = node->left->type;
            break;
        default:
            goto invalid_operation;
        }
    
    } else if ((is_binary_node(node) || is_unary_node(node)) && node->type == FDS_FILTER_AST_LIST) {
        if (!resolve_list_types(filter, node)) {
            return 0;
        }
    
    } else if (is_leaf_node(node)) {
        // Do nothing

    } else {
        goto invalid_operation;
    }

    return 1;

invalid_operation:
    if (is_binary_node(node)) {
        error_location_message(filter, node, "Invalid operation %s for values of type %s and %s", 
                               ast_op_to_str(node->op), type_to_str(node->left->type), type_to_str(node->right->type));
    } else if (is_unary_node(node)) {
        error_location_message(filter, node, "Invalid operation %s for value of type %s", 
                               ast_op_to_str(node->op), type_to_str(node->left->type));
    } else if (is_leaf_node(node)) {
        error_location_message(filter, node, "Invalid operation %s", ast_op_to_str(node->op));
    }
    return 0;
}


int
prepare_node(struct fds_filter *filter, struct fds_filter_node *node)
{
    prepare_node(left);
    prepare_node(right);

    resolve_types(node);
}