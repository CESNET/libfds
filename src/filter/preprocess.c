#include <libfds.h>
#include "filter.h"
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

static int
both_children_of_type(struct fds_filter_ast_node *node, enum fds_filter_type type)
{
    return node->left->type == type && node->right->type == type;
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
    struct fds_filter_lookup_args args;
    args.context = filter->context;
    args.name = node->identifier_name;
    args.id = &node->identifier_id;
    args.type = &node->type;
    args.subtype = &node->subtype;
    args.is_constant = &node->identifier_is_constant;
    args.output_value = &node->value;
    if (!filter->lookup_callback(args)) {
        error_location_message(filter, node->location, "Unknown identifier '%s'", node->identifier_name);
        return 0;
    }
    return 1;
}

static int
resolve_types(struct fds_filter *filter, struct fds_filter_ast_node *node)
{
    switch (node->op) {
    case FDS_FILTER_AST_AND:
    case FDS_FILTER_AST_OR:
    {
        if (!cast_to_bool(filter, &node->left) || !cast_to_bool(filter, &node->right)) {
            return 0;
        }
        node->type = FDS_FILTER_TYPE_BOOL;
    } break;

    case FDS_FILTER_AST_NOT:
    case FDS_FILTER_AST_ROOT:
    case FDS_FILTER_AST_ANY:
    {
        if (!cast_to_bool(filter, &node->left)) {
            return 0;
        }
        node->type = FDS_FILTER_TYPE_BOOL;
    } break;

    case FDS_FILTER_AST_ADD:
    {
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
    } break;

    case FDS_FILTER_AST_SUB:
    case FDS_FILTER_AST_MUL:
    case FDS_FILTER_AST_DIV:
    {
        if (is_number_type(node->left->type) && is_number_type(node->right->type)) {
            if (!cast_children_to_common_number_type(filter, node)) {
                return 0;
            }
            node->type = node->left->type;
        } else {
            goto invalid_operation;
        }
    } break;

    case FDS_FILTER_AST_UMINUS:
    {
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
    } break;

    case FDS_FILTER_AST_EQ:
    case FDS_FILTER_AST_NE:
    {
        if (is_number_type(node->left->type) && is_number_type(node->right->type)) {
            if (!cast_children_to_common_number_type(filter, node)) {
                return 0;
            }
        } else if (both_children_of_type(node, FDS_FILTER_TYPE_IP_ADDRESS)
                || both_children_of_type(node, FDS_FILTER_TYPE_MAC_ADDRESS)) {
            // Pass
        } else {
            goto invalid_operation;
        }
        node->type = FDS_FILTER_TYPE_BOOL;
    } break;

    case FDS_FILTER_AST_LT:
    case FDS_FILTER_AST_GT:
    case FDS_FILTER_AST_LE:
    case FDS_FILTER_AST_GE:
    {
        if (is_number_type(node->left->type) && is_number_type(node->right->type)) {
            if (!cast_children_to_common_number_type(filter, node)) {
                return 0;
            }
        } else {
            goto invalid_operation;
        }
        node->type = FDS_FILTER_TYPE_BOOL;
    } break;

    case FDS_FILTER_AST_CONTAINS:
    {
        if (both_children_of_type(node, FDS_FILTER_TYPE_STR)) {
            node->type = FDS_FILTER_TYPE_BOOL;
        } else {
            goto invalid_operation;
        }
    } break;

    case FDS_FILTER_AST_IN:
    {
        // !!! TODO: Make it work with actual lists, now it assumes AST lists
        // All the values in the list have the same type at this point
        // node->right is the AST_LIST
        // node->right->left is the last AST_LIST_ITEM, if it's null the list is empty
        // node->right->left->right is the value of the last list item
        if (node->right->type == FDS_FILTER_TYPE_LIST
                && (!node->right->left || node->left->type == node->right->left->right->type)) {
           node->type = FDS_FILTER_TYPE_BOOL;
        } else {
            goto invalid_operation;
        }
    } break;

    case FDS_FILTER_AST_LIST:
    {
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
    } break;

    case FDS_FILTER_AST_IDENTIFIER:
    {
        if (!lookup_identifier(filter, node)) {
            return 0;
        }
    } break;

    case FDS_FILTER_AST_LIST_ITEM:
    case FDS_FILTER_AST_CONST:
    {
        // Pass
    } break;

    default: assert(0); // We want to explicitly handle all cases
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

static int
is_constant_subtree(struct fds_filter_ast_node *node)
{
    if (is_leaf_node(node)) {
        assert(node->op == FDS_FILTER_AST_CONST || node->op == FDS_FILTER_AST_IDENTIFIER);
        return node->op == FDS_FILTER_AST_CONST || node->identifier_is_constant;
    } else if (is_binary_node(node)) {
        return is_constant_subtree(node->left) && is_constant_subtree(node->right);
    } else if (is_unary_node(node)) {
        return is_constant_subtree(node->left);
    }
}

static int
optimize_constant_subtree(struct fds_filter *filter, struct fds_filter_ast_node *node)
{
    struct eval_node *eval_tree = generate_eval_tree_from_ast(filter, node);
    if (!eval_tree) {
        return 0;
    }
    evaluate_eval_tree(filter, eval_tree);
    ast_destroy(node->left);
    ast_destroy(node->right);
    node->op = FDS_FILTER_AST_CONST;
    node->identifier_is_constant = 1;
    node->value = eval_tree->value;
    node->left = NULL;
    node->right = NULL;
    return 1;
}

static int
optimize_node(struct fds_filter *filter, struct fds_filter_ast_node **node)
{
    // Skip the special ast nodes
    if ((*node)->op == FDS_FILTER_AST_ROOT || (*node)->op == FDS_FILTER_AST_LIST_ITEM) {
        return 1;
    }

    // TODO: The is_constant_subtree function gets unnecessarily called for each node
    //       and has to recursively go through all the children. This could be optimized
    //       if it's a performance issue.
    if (is_constant_subtree(*node)) {
        if (!optimize_constant_subtree(filter, *node)) {
            return 0;
        }
    }
}

static int
convert_ast_list_to_actual_list(struct fds_filter *filter, struct fds_filter_ast_node *list_node)
{
    assert(list_node->op == FDS_FILTER_AST_LIST);

    // Walk through the list to the end and count the items along the way.
    // The list items are in reverse order in the AST.
    struct fds_filter_ast_node *list_item = list_node;
    int n_list_items = 0;
    while (list_item->left != NULL) {
        n_list_items++;
        list_item = list_item->left;
    }

    union fds_filter_value *list = malloc(n_list_items * sizeof (union fds_filter_value));
    if (list == NULL) {
        error_no_memory(filter);
        return 0;
    }

    // Evaluate all the list item nodes to allow constant expressions and populate the actual list
    // with their values.
    int item_index = 0;
    while (list_item->op != FDS_FILTER_AST_LIST) { // Loop until we reach the root of the list
        if (!is_constant_subtree(list_item->right)) {
            error_location_message(filter, list_item->location, "List items must be constant expressions");
            return 0;
        }
        if (!optimize_constant_subtree(filter, list_item->right)) {
            return 0;
        }
        list[item_index] = list_item->right->value;
        item_index++;
        list_item = list_item->parent;
    }
    assert(item_index == n_list_items);

    // Transform the AST_LIST node to AST_CONST node with a value of list.
    ast_destroy(list_node->left);
    assert(list_node->right == NULL);
    list_node->op = FDS_FILTER_AST_CONST;
    list_node->left = NULL;
    list_node->value.list.items = list;
    list_node->value.list.length = n_list_items;

    return 1;
}

static int
add_any_nodes(struct fds_filter *filter, struct fds_filter_ast_node *node)
{
    if (node->op == FDS_FILTER_AST_NOT || node->op == FDS_FILTER_AST_ROOT) {
        struct fds_filter_ast_node *new_node = ast_node_create();
        if (new_node == NULL) {
            error_no_memory(filter);
            return 0;
        }
        new_node->op = FDS_FILTER_AST_ANY;
        new_node->left = node->left;
        node->left = new_node;

    } else if (node->op == FDS_FILTER_AST_AND || node->op == FDS_FILTER_AST_OR) {
        struct fds_filter_ast_node *new_node_left = ast_node_create();
        if (new_node_left == NULL) {
            error_no_memory(filter);
            return 0;
        }
        new_node_left->op = FDS_FILTER_AST_ANY;
        new_node_left->left = node->left;
        node->left = new_node_left;

        struct fds_filter_ast_node *new_node_right = ast_node_create();
        if (new_node_right == NULL) {
            error_no_memory(filter);
            return 0;
        }
        new_node_right->op = FDS_FILTER_AST_ANY;
        new_node_right->left = node->right;
        node->right = new_node_right;
    }

    return 1;
}

int
prepare_ast_nodes(struct fds_filter *filter, struct fds_filter_ast_node *node)
{
    if (node == NULL) {
        return 1;
    }

    // Attach extra nodes.
    // The nodes have to be added first before the children are processed,
    // else the new nodes don't get processed.
    if (!add_any_nodes(filter, node)) {
        pdebug("ERROR: Add any nodes for %s failed", ast_op_to_str(node->op));
        return 0;
    }

    // Process left subtree
    if (!prepare_ast_nodes(filter, node->left)) {
        pdebug("ERROR: Preparing left subtree of %s failed", ast_op_to_str(node->op));
        return 0;
    }
    // Process right subtree
    if (!prepare_ast_nodes(filter, node->right)) {
        pdebug("ERROR: Preparing right subtree of %s failed", ast_op_to_str(node->op));
        return 0;
    }

    // Process the actual node
    if (!resolve_types(filter, node)) {
        pdebug("ERROR: Resolve types for %s failed", ast_op_to_str(node->op));
        return 0;
    }
    if (node->op == FDS_FILTER_AST_LIST) {
        if (!convert_ast_list_to_actual_list(filter, node)) {
            pdebug("ERROR: Convert AST list to actual list for %s failed", ast_op_to_str(node->op));
            return 0;
        }
        assert(node->op == FDS_FILTER_AST_CONST);
    }
    // Optimize
    if (!optimize_node(filter, &node)) { // &node because we may want to remove the node itself
        pdebug("ERROR: Optimize node for %s failed", ast_op_to_str(node->op));
        return 0;
    }

    return 1;
}
