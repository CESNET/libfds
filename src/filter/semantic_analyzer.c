#include <stdbool.h>
#include <libfds.h>
#include "filter.h"
#include "ast_utils.h"

static enum fds_filter_data_type
get_common_number_type(enum fds_filter_data_type left, enum fds_filter_data_type right)
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
cast_node(struct fds_filter *filter, struct fds_filter_ast_node **node,
          enum fds_filter_data_type to_type, enum fds_filter_data_type to_subtype)
{
    if ((*node)->type == to_type && (*node)->subtype == to_subtype) {
        return FDS_FILTER_OK;
    }

    struct fds_filter_ast_node *new_node = ast_node_create();
    if (new_node == NULL) {
        pdebug("returning FAIL because no memory");
        error_no_memory(filter);
        return FDS_FILTER_FAIL;
    }

    new_node->type = to_type;
    new_node->subtype = to_subtype;
    new_node->op = FDS_FILTER_AST_CAST;
    new_node->left = *node;

    *node = new_node;

    return FDS_FILTER_OK;
}

static int
cast_children_to_common_number_type(struct fds_filter *filter, struct fds_filter_ast_node *node)
{
    enum fds_filter_data_type type = get_common_number_type(node->left->type, node->right->type);

    if (type == FDS_FILTER_TYPE_NONE) {
        pdebug("returning FAIL because cannot cast");
        error_location_message(filter, node->location,
            "Cannot cast numbers of type %s and %s to a common type",
            type_to_str(node->left->type), type_to_str(node->right->type));
        return FDS_FILTER_OK;
    }

    int return_code;

    return_code = cast_node(filter, &node->left, type, FDS_FILTER_TYPE_NONE);
    if (cast_node(filter, &node->left, type, FDS_FILTER_TYPE_NONE) != FDS_FILTER_OK) {
        pdebug("propagating return code");
        return return_code;
    }

    return_code = cast_node(filter, &node->left, type, FDS_FILTER_TYPE_NONE);
    if (cast_node(filter, &node->right, type, FDS_FILTER_TYPE_NONE) != FDS_FILTER_OK) {
        pdebug("propagating return code");
        return return_code;
    }

    assert(node->left->type == node->right->type);

    return FDS_FILTER_OK;
}

static int
cast_all_list_items_to_type(struct fds_filter *filter, struct fds_filter_ast_node *list_node,
                            enum fds_filter_data_type type)
{
    assert(list_node->op == FDS_FILTER_AST_LIST);

    int return_code;
    struct fds_filter_ast_node *list_item = list_node->left;
    while (list_item != NULL) {
        assert(list_item->op == FDS_FILTER_AST_LIST_ITEM);
        return_code = cast_node(filter, &list_item->right, type, FDS_FILTER_TYPE_NONE);
        if (return_code != FDS_FILTER_OK) {
            pdebug("propagating return code");
            return return_code;
        }
        list_item->type = list_item->right->type;
        list_item = list_item->left;
    }
    list_node->subtype = type;
    return FDS_FILTER_OK;
}

static int
cast_list_to_same_type(struct fds_filter *filter, struct fds_filter_ast_node *node)
{
    struct fds_filter_ast_node *list_item = node->left;
    if (list_item == NULL) {
        node->subtype = FDS_FILTER_TYPE_NONE;
        return FDS_FILTER_OK;
    }
    enum fds_filter_data_type final_type = list_item->right->type;

    while ((list_item = list_item->left) != NULL) {
        if (type_is_number(list_item->right->type)) {
            enum fds_filter_data_type type = get_common_number_type(final_type, list_item->right->type);
            if (type == FDS_FILTER_AST_NONE) {
                pdebug("returning FAIL because cannot cast");
                error_location_message(filter, node->location,
                    "Cannot cast items of list to the same type - no common type for values of type %s and %s",
                    type_to_str(type), type_to_str(list_item->right->type));
                return FDS_FILTER_FAIL;
            }
            final_type = type;

        } else if (list_item->right->type != final_type) {
            pdebug("returning FAIL because cannot cast");
            error_location_message(filter, list_item->location,
                "Cannot cast items of list to the same type - no common type for values of type %s and %s",
                type_to_str(final_type), type_to_str(list_item->right->type));
            return FDS_FILTER_FAIL;
        }
    }

    int return_code = cast_all_list_items_to_type(filter, node, final_type);
    if (return_code != FDS_FILTER_OK) {
        pdebug("propagating return code");
        return return_code;
    }

    return FDS_FILTER_OK;
}

static int
cast_to_bool(struct fds_filter *filter, struct fds_filter_ast_node **node)
{
    // TODO: Check type? Now we assume any type can be converted to bool
    pdebug("propagating return code");
    return cast_node(filter, node, FDS_FILTER_TYPE_BOOL, FDS_FILTER_TYPE_NONE);
}

static int
semantic_resolve_node(struct fds_filter *filter, struct fds_filter_ast_node **node_ptr)
{
    struct fds_filter_ast_node *node = *node_ptr;
    int return_code;

    switch (node->op) {
    case FDS_FILTER_AST_AND:
    case FDS_FILTER_AST_OR: {
        if ((return_code = cast_to_bool(filter, &node->left)) != FDS_FILTER_OK
            || (return_code = cast_to_bool(filter, &node->right)) != FDS_FILTER_OK) {
            pdebug("propagating return code");
            return return_code;
        }
        node->type = FDS_FILTER_TYPE_BOOL;
    } break;

    case FDS_FILTER_AST_NOT:
    case FDS_FILTER_AST_ROOT:
    case FDS_FILTER_AST_ANY: {
        if ((return_code = cast_to_bool(filter, &node->left)) != FDS_FILTER_OK) {
            pdebug("propagating return code");
            return return_code;
        }
        node->type = FDS_FILTER_TYPE_BOOL;
    } break;

    case FDS_FILTER_AST_ADD: {
        if (type_is_number(node->left->type) && type_is_number(node->right->type)) {
            if ((return_code = cast_children_to_common_number_type(filter, node)) != FDS_FILTER_OK) {
                pdebug("propagating return code");
                return return_code;
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
    case FDS_FILTER_AST_DIV: {
        if (type_is_number(node->left->type) && type_is_number(node->right->type)) {
            if ((return_code = cast_children_to_common_number_type(filter, node)) != FDS_FILTER_OK) {
                pdebug("propagating return code");
                return return_code;
            }
            node->type = node->left->type;
        } else {
            goto invalid_operation;
        }
    } break;

    case FDS_FILTER_AST_UMINUS: {
        if (type_is_number(node->left->type)) {
            if (node->left->type == FDS_FILTER_TYPE_UINT) {
                if (return_code =
                    cast_node(filter, &node->left, FDS_FILTER_TYPE_INT, FDS_FILTER_TYPE_NONE) != FDS_FILTER_OK) {
                    pdebug("propagating return code");
                    return return_code;
                }
            }
            node->type = node->left->type;
        } else {
            goto invalid_operation;
        }
    } break;

    case FDS_FILTER_AST_EQ:
    case FDS_FILTER_AST_NE: {
        if (type_is_number(node->left->type) && type_is_number(node->right->type)) {
            if ((return_code = cast_children_to_common_number_type(filter, node)) != FDS_FILTER_OK) {
                pdebug("propagating return code");
                return return_code;
            }
        } else if (type_of_both_children(node, FDS_FILTER_TYPE_IP_ADDRESS)
                || type_of_both_children(node, FDS_FILTER_TYPE_MAC_ADDRESS)) {
            // Pass
        } else {
            goto invalid_operation;
        }
        node->type = FDS_FILTER_TYPE_BOOL;
    } break;

    case FDS_FILTER_AST_LT:
    case FDS_FILTER_AST_GT:
    case FDS_FILTER_AST_LE:
    case FDS_FILTER_AST_GE: {
        if (type_is_number(node->left->type) && type_is_number(node->right->type)) {
            if ((return_code = cast_children_to_common_number_type(filter, node)) != FDS_FILTER_OK) {
                pdebug("propagating return code");
                return return_code;
            }
        } else {
            goto invalid_operation;
        }
        node->type = FDS_FILTER_TYPE_BOOL;
    } break;

    case FDS_FILTER_AST_CONTAINS: {
        if (type_of_both_children(node, FDS_FILTER_TYPE_STR)) {
            node->type = FDS_FILTER_TYPE_BOOL;
        } else {
            goto invalid_operation;
        }
    } break;

    case FDS_FILTER_AST_IN: {
        node->type = FDS_FILTER_TYPE_BOOL;
        if (node->right->type != FDS_FILTER_TYPE_LIST) {
            goto invalid_operation;
        }

        if (node->left->type == node->right->subtype || node->right->subtype == FDS_FILTER_TYPE_NONE) {
            // No need to do anything
        } else if (type_is_number(node->left->type) && type_is_number(node->right->subtype)) {
            enum fds_filter_data_type common_type =
                get_common_number_type(node->left->type, node->right->subtype);
            if (common_type != FDS_FILTER_TYPE_NONE) {
                if ((return_code =
                    cast_node(filter, &node->left, common_type, FDS_FILTER_TYPE_NONE)) != FDS_FILTER_OK){
                    pdebug("propagating return code");
                    return return_code;
                }
                if ((return_code =
                    cast_node(filter, &node->right, FDS_FILTER_TYPE_LIST, common_type)) != FDS_FILTER_OK){
                    pdebug("propagating return code");
                    return return_code;
                }
            } else {
                goto invalid_operation;
            }
        } else {
            goto invalid_operation;
        }
    } break;

    case FDS_FILTER_AST_LIST: {
        node->type = FDS_FILTER_TYPE_LIST;
        if ((return_code = cast_list_to_same_type(filter, node)) != FDS_FILTER_OK) {
            pdebug("propagating return code");
            return return_code;
        }
    } break;

    case FDS_FILTER_AST_LIST_ITEM: {
        node->type = node->right->type;
    } break;

    case FDS_FILTER_AST_IDENTIFIER:
    case FDS_FILTER_AST_CONST: {
        // Pass
    } break;

    default: assert(0); // We want to explicitly handle all cases
    }

    return FDS_FILTER_OK;


invalid_operation:
    if (ast_is_binary_node(node)) {
        error_location_message(filter, node->location,
            "Invalid operation %s for values of type %s(%s) and %s(%s)",
            ast_op_to_str(node->op), type_to_str(node->left->type), type_to_str(node->left->subtype),
            type_to_str(node->right->type), type_to_str(node->right->subtype));
    } else if (ast_is_unary_node(node)) {
        error_location_message(filter, node->location, "Invalid operation %s for value of type %s",
                               ast_op_to_str(node->op), type_to_str(node->left->type));
    } else if (ast_is_leaf_node(node)) {
        error_location_message(filter, node->location, "Invalid operation %s", ast_op_to_str(node->op));
    }
    pdebug("returning FAIL because invalid operation");
    return FDS_FILTER_FAIL;
}

int
semantic_analysis(struct fds_filter *filter)
{
    struct fds_filter_ast_node **root_node_ptr = &filter->ast;

    int return_code;

    return_code = ast_apply_to_all_nodes(semantic_resolve_node, filter, root_node_ptr);
    if (return_code != FDS_FILTER_OK) {
        pdebug("propagating return code");
        return return_code;
    }

    pdebug("semantic analysis succeeded");
    return FDS_FILTER_OK;
}
