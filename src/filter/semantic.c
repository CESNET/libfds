#include "semantic.h"
#include "ast.h"
#include "error.h"
#include <assert.h>

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
    else if (left == FDS_FILTER_TYPE_INT   && right == FDS_FILTER_TYPE_UINT ) { return FDS_FILTER_TYPE_UINT;  }
    else if (left == FDS_FILTER_TYPE_UINT  && right == FDS_FILTER_TYPE_INT  ) { return FDS_FILTER_TYPE_UINT;  }
	else                                                                      { return FDS_FILTER_TYPE_NONE;  }
}

static int
cast_node(struct fds_filter *filter, struct fds_filter_ast_node **node, enum fds_filter_type to)
{
    if ((*node)->type == to) {
        return 1;
    }

    struct fds_filter_ast_node *new_node = ast_node_create();
    if (new_node == NULL) {
        error_no_memory(filter);
        return 0;
    }
    new_node->type = to;
    new_node->op = FDS_FILTER_AST_CAST;
    new_node->left = *node;
    *node = new_node;
    return 1;
}

static int
cast_children_to_common_number_type(struct fds_filter *filter, struct fds_filter_ast_node *node)
{
    enum fds_filter_type type = get_common_number_type(node->left->type, node->right->type);
    if (type == FDS_FILTER_TYPE_NONE) {
        error_location_message(filter, node->location, "Cannot cast numbers of type %s and %s to a common type",
                               type_to_str(node->left->type), type_to_str(node->right->type));
        return 0;
    }
    if (!cast_node(filter, &node->left, type)) {
        return 0;
    }
    if (!cast_node(filter, &node->right, type)) {
        return 0;
    }
    assert(node->left->type == node->right->type);
    return 1;
}

static int
cast_list_to_same_type(struct fds_filter *filter, struct fds_filter_ast_node *node)
{
    enum fds_filter_type final_type = node->right->type;
    struct fds_filter_ast_node *list_item = node;
    
    while ((list_item = list_item->left) != NULL) {
        if (is_number_type(list_item->right->type)) {
            enum fds_filter_type type = get_common_number_type(final_type, list_item->right->type);
            if (type == FDS_FILTER_AST_NONE) {
                error_location_message(filter, node->location, "Cannot cast items of list to the same type - no common type for values of type %s and %s",
                                       type_to_str(type), type_to_str(list_item->right->type));
                return 0;
            }
            final_type = type;

        } else if (list_item->right->type != final_type) {
            error_location_message(filter, list_item->location, "Cannot cast items of list to the same type - no common type for values of type %s and %s",
                                   type_to_str(final_type), type_to_str(list_item->right->type));
            return 0;
        }
    }

    list_item = node;
    while (list_item != NULL) {
        if (!cast_node(filter, &list_item->right, final_type)) {
            return 0;
        }
        list_item = list_item->left;
    }

    return 1;
}

static int 
cast_to_bool(struct fds_filter *filter, struct fds_filter_ast_node **node)
{
    // TODO: Check type? Now we assume any type can be converted to bool
    return cast_node(filter, node, FDS_FILTER_TYPE_BOOL);
}

static int
lookup_identifier(struct fds_filter *filter, struct fds_filter_ast_node *node)
{
    int ok = filter->lookup_callback(node->identifier_name, &node->identifier_id, &node->type, 
                                     &node->identifier_is_constant, &node->value);
    if (!ok) {
        error_location_message(filter, node->location, "Lookup callback for identifier %s failed", node->identifier_name);
    }
    return ok;
}

static int 
resolve_types(struct fds_filter *filter, struct fds_filter_ast_node *node)
{
    if (node->op == FDS_FILTER_AST_AND || node->op == FDS_FILTER_AST_OR) {
        if (!cast_to_bool(filter, &node->left) || !cast_to_bool(filter, &node->right)) {
            return 0;
        }
        node->type = FDS_FILTER_TYPE_BOOL;
    } else if (node->op == FDS_FILTER_AST_NOT) {
        if (!cast_to_bool(filter, &node->left)) {
            return 0;
        }
        node->type = FDS_FILTER_TYPE_BOOL;
    } else if (node->op == FDS_FILTER_AST_ADD) {
        if (is_number_type(node->left->type) && is_number_type(node->right->type)) {
            if (!cast_children_to_common_number_type(filter, node)) {
                return 0;
            }
            node->type = node->left->type;
        } else if (node->left->type == FDS_FILTER_TYPE_STR && node->right->type == FDS_FILTER_TYPE_STR) {
            node->type = FDS_FILTER_TYPE_STR;
        } else {
            goto invalid_operation;
        }
    } else if (node->op == FDS_FILTER_AST_SUB || node->op == FDS_FILTER_AST_MUL || node->op == FDS_FILTER_AST_DIV) {
        if (is_number_type(node->left->type) && is_number_type(node->right->type)) {
            if (!cast_children_to_common_number_type(filter, node)) {
                return 0;
            }
            node->type = node->left->type;
        } else {
            goto invalid_operation;
        }
    } else if (node->op == FDS_FILTER_AST_UMINUS) {
        if (is_number_type(node->left->type)) {
            if (node->left->type == FDS_FILTER_TYPE_UINT) {
                if (!cast_node(filter, &node->left, FDS_FILTER_TYPE_INT)) {
                    return 0;
                }
            }
            node->type = node->left->type;
        } else {
            goto invalid_operation;
        }
    } else if (node->op == FDS_FILTER_AST_EQ || node->op == FDS_FILTER_AST_NE) {
        if (is_number_type(node->left->type) && is_number_type(node->right->type)) {
            if (!cast_children_to_common_number_type(filter, node)) {
                return 0;
            }
        } else if ((node->left->type == FDS_FILTER_TYPE_IP_ADDRESS && node->right->type == FDS_FILTER_TYPE_IP_ADDRESS)
                   || (node->left->type == FDS_FILTER_TYPE_MAC_ADDRESS && node->right->type == FDS_FILTER_TYPE_MAC_ADDRESS)) {
            // Pass
        } else {
            goto invalid_operation;
        }
        node->type = FDS_FILTER_TYPE_BOOL;
    } else if (node->op == FDS_FILTER_AST_LT || node->op == FDS_FILTER_AST_GT || node->op == FDS_FILTER_AST_LE || node->op == FDS_FILTER_AST_GE) {
        if (is_number_type(node->left->type) && is_number_type(node->right->type)) {
            if (!cast_children_to_common_number_type(filter, node)) {
                return 0;
            }
        } else {
            goto invalid_operation;
        }
        node->type = FDS_FILTER_TYPE_BOOL;
    } else if (node->op == FDS_FILTER_AST_CONTAINS) {
        if (node->left->type == FDS_FILTER_TYPE_STR && node->right->type == FDS_FILTER_TYPE_STR) {
            node->type = FDS_FILTER_TYPE_BOOL;
        } else {
            goto invalid_operation;
        }
    } else if (node->op == FDS_FILTER_AST_IN) {
        // All the values in the list have the same type at this point
        // node->right is the AST_LIST
        // node->right->left is the last AST_LIST_ITEM, if it's null the list is empty
        // node->right->left->right is the value of the last list item
        if (node->right->type == FDS_FILTER_TYPE_LIST && (!node->right->left || node->left->type == node->right->left->right->type)) {
           node->type = FDS_FILTER_TYPE_BOOL;
        } else {
            goto invalid_operation;
        }
    } else if (node->op == FDS_FILTER_AST_LIST) {
    // TODO: list support
        node->type = FDS_FILTER_TYPE_LIST;
        if (node->left) { // Empty list
            if (!cast_list_to_same_type(filter, node->left)) {
                return 0;
            }
            node->subtype = node->left->right->type; // All the list items are the same type at this point 
        } else {
            node->subtype = FDS_FILTER_TYPE_NONE;
        }
    } else if (node->op == FDS_FILTER_AST_IDENTIFIER) {
        if (!lookup_identifier(filter, node)) {
            return 0;
        }
    } else if (node->op == FDS_FILTER_AST_LIST_ITEM) {
        // TODO
    } else {
        goto invalid_operation;
    }
    return 1;

invalid_operation:
    if (is_binary_node(node)) {
        error_location_message(filter, node->location, "Invalid operation %s for values of type %s and %s", 
                               ast_op_to_str(node->op), type_to_str(node->left->type), type_to_str(node->right->type));
    } else if (is_unary_node(node)) {
        error_location_message(filter, node->location, "Invalid operation %s for value of type %s", 
                               ast_op_to_str(node->op), type_to_str(node->left->type));
    } else if (is_leaf_node(node)) {
        error_location_message(filter, node->location, "Invalid operation %s", ast_op_to_str(node->op));
    }
    return 0;
}

int
prepare_nodes(struct fds_filter *filter, struct fds_filter_ast_node *node)
{
    if (node == NULL) {
        return 0;
    }

    // Process children first and then parent
    prepare_nodes(filter, node->left); // TODO: Do something with return code
    prepare_nodes(filter, node->right); // TODO: Do something with return code
    resolve_types(filter, node); // TODO: Do something with return code
    
}