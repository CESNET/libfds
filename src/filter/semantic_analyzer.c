#include <stdbool.h>
#include <libfds.h>
#include "filter.h"
#include "debug.h"
#include "ast.h"

static enum fds_filter_data_type
get_common_number_type(enum fds_filter_data_type left, enum fds_filter_data_type right)
{
	if      (left == right)                                   { return left;          }
    else if (left == FDS_FDT_FLOAT && right == FDS_FDT_INT  ) { return FDS_FDT_FLOAT; }
    else if (left == FDS_FDT_FLOAT && right == FDS_FDT_UINT ) { return FDS_FDT_FLOAT; }
    else if (left == FDS_FDT_INT   && right == FDS_FDT_FLOAT) { return FDS_FDT_FLOAT; }
    else if (left == FDS_FDT_UINT  && right == FDS_FDT_FLOAT) { return FDS_FDT_FLOAT; }
    else if (left == FDS_FDT_INT   && right == FDS_FDT_UINT ) { return FDS_FDT_UINT;  }
    else if (left == FDS_FDT_UINT  && right == FDS_FDT_INT  ) { return FDS_FDT_UINT;  }
	else                                                      { return FDS_FDT_NONE;  }
}

static int
cast_node(struct fds_filter *filter, struct fds_filter_ast_node **node,
          enum fds_filter_data_type to_type, enum fds_filter_data_type to_subtype)
{
    if ((*node)->data_type == to_type && (*node)->data_subtype == to_subtype) {
        return FDS_FILTER_OK;
    }

    struct fds_filter_ast_node *new_node = create_ast_node();
    if (new_node == NULL) {
        PTRACE("returning FAIL because no memory");
        no_memory_error(filter->error_list);
        return FDS_FILTER_FAIL;
    }

    new_node->data_type = to_type;
    new_node->data_subtype = to_subtype;
    new_node->node_type = FDS_FANT_CAST;
    new_node->left = *node;

    *node = new_node;

    return FDS_FILTER_OK;
}

static int
cast_children_to_common_number_type(struct fds_filter *filter, struct fds_filter_ast_node *node)
{
    enum fds_filter_data_type type = get_common_number_type(node->left->data_type, node->right->data_type);

    if (type == FDS_FDT_NONE) {
        PTRACE("returning FAIL because cannot cast");
        add_error_location_message(filter->error_list, node->location,
            "Cannot cast numbers of type %s and %s to a common type",
            data_type_to_str(node->left->data_type), data_type_to_str(node->right->data_type));
        return FDS_FILTER_OK;
    }

    int return_code;

    return_code = cast_node(filter, &node->left, type, FDS_FDT_NONE);
    if (cast_node(filter, &node->left, type, FDS_FDT_NONE) != FDS_FILTER_OK) {
        PTRACE("propagating return code");
        return return_code;
    }

    return_code = cast_node(filter, &node->left, type, FDS_FDT_NONE);
    if (cast_node(filter, &node->right, type, FDS_FDT_NONE) != FDS_FILTER_OK) {
        PTRACE("propagating return code");
        return return_code;
    }

    assert(node->left->data_type == node->right->data_type);

    return FDS_FILTER_OK;
}

static int
cast_all_list_items_to_type(struct fds_filter *filter, struct fds_filter_ast_node *list_node,
                            enum fds_filter_data_type type)
{
    assert(list_node->node_type == FDS_FANT_LIST);

    int return_code;
    struct fds_filter_ast_node *list_item = list_node->left;
    while (list_item != NULL) {
        assert(list_item->node_type == FDS_FANT_LIST_ITEM);
        return_code = cast_node(filter, &list_item->right, type, FDS_FDT_NONE);
        if (return_code != FDS_FILTER_OK) {
            PTRACE("propagating return code");
            return return_code;
        }
        list_item->data_type = list_item->right->data_type;
        list_item = list_item->left;
    }
    list_node->data_subtype = type;
    return FDS_FILTER_OK;
}

static int
cast_list_to_same_type(struct fds_filter *filter, struct fds_filter_ast_node *node)
{
    struct fds_filter_ast_node *list_item = node->left;
    if (list_item == NULL) {
        node->data_subtype = FDS_FDT_NONE;
        return FDS_FILTER_OK;
    }
    enum fds_filter_data_type final_type = list_item->right->data_type;

    while ((list_item = list_item->left) != NULL) {
        if (is_number_type(list_item->right->data_type)) {
            enum fds_filter_data_type type = get_common_number_type(final_type, list_item->right->data_type);
            if (type == FDS_FDT_NONE) {
                PTRACE("returning FAIL because cannot cast");
                add_error_location_message(filter->error_list, node->location,
                    "Cannot cast items of list to the same type - no common type for values of type %s and %s",
                    data_type_to_str(type), data_type_to_str(list_item->right->data_type));
                return FDS_FILTER_FAIL;
            }
            final_type = type;

        } else if (list_item->right->data_type != final_type) {
            PTRACE("returning FAIL because cannot cast");
            add_error_location_message(filter->error_list, list_item->location,
                "Cannot cast items of list to the same type - no common type for values of type %s and %s",
                data_type_to_str(final_type), data_type_to_str(list_item->right->data_type));
            return FDS_FILTER_FAIL;
        }
    }

    int return_code = cast_all_list_items_to_type(filter, node, final_type);
    if (return_code != FDS_FILTER_OK) {
        PTRACE("propagating return code");
        return return_code;
    }

    return FDS_FILTER_OK;
}

static int
cast_to_bool(struct fds_filter *filter, struct fds_filter_ast_node **node)
{
    // TODO: Check type? Now we assume any type can be converted to bool
    int return_code;
    return_code = cast_node(filter, node, FDS_FDT_BOOL, FDS_FDT_NONE);
    if (return_code != FDS_FILTER_OK) {
        PTRACE("propagating error code");
        return return_code;
    }
    return FDS_FILTER_OK;
}

static int
semantic_resolve_node(struct fds_filter *filter, struct fds_filter_ast_node **node_ptr)
{
    struct fds_filter_ast_node *node = *node_ptr;
    int return_code;

    switch (node->node_type) {
    case FDS_FANT_AND:
    case FDS_FANT_OR: {
        RETURN_IF_ERROR(cast_to_bool(filter, &node->left));
        RETURN_IF_ERROR(cast_to_bool(filter, &node->right));
        node->data_type = FDS_FDT_BOOL;
    } break;

    case FDS_FANT_NOT:
    case FDS_FANT_ROOT:
    case FDS_FANT_ANY: {
        RETURN_IF_ERROR(cast_to_bool(filter, &node->left));
        node->data_type = FDS_FDT_BOOL;
    } break;

    case FDS_FANT_ADD: {
        if (is_number_type(node->left->data_type) && is_number_type(node->right->data_type)) {
            RETURN_IF_ERROR(cast_children_to_common_number_type(filter, node));
            node->data_type = node->left->data_type; // The resulting datatype of the cast
        } else if (node->left->data_type == FDS_FDT_STR && node->right->data_type == FDS_FDT_STR) {
            node->data_type = FDS_FDT_STR; // String concatenation
        } else {
            goto invalid_operation;
        }
    } break;

    case FDS_FANT_SUB:
    case FDS_FANT_MUL:
    case FDS_FANT_DIV:
    case FDS_FANT_MOD: {
        if (is_number_type(node->left->data_type) && is_number_type(node->right->data_type)) {
            RETURN_IF_ERROR(cast_children_to_common_number_type(filter, node));
            node->data_type = node->left->data_type;
        } else {
            goto invalid_operation;
        }
    } break;

    case FDS_FANT_UMINUS: {
        if (is_number_type(node->left->data_type)) {
            if (node->left->data_type == FDS_FDT_UINT) {
                RETURN_IF_ERROR(cast_node(filter, &node->left, FDS_FDT_INT, FDS_FDT_NONE));
            }
            node->data_type = node->left->data_type;
        } else {
            goto invalid_operation;
        }
    } break;

    case FDS_FANT_EQ:
    case FDS_FANT_NE: {
        if (is_number_type(node->left->data_type) && is_number_type(node->right->data_type)) {
            RETURN_IF_ERROR(cast_children_to_common_number_type(filter, node));
        } else if (both_children_of_type(node, FDS_FDT_IP_ADDRESS)
                || both_children_of_type(node, FDS_FDT_MAC_ADDRESS)
                || both_children_of_type(node, FDS_FDT_STR)) {
            // Pass
        } else {
            goto invalid_operation;
        }
        node->data_type = FDS_FDT_BOOL;
    } break;

    case FDS_FANT_LT:
    case FDS_FANT_GT:
    case FDS_FANT_LE:
    case FDS_FANT_GE: {
        if (is_number_type(node->left->data_type) && is_number_type(node->right->data_type)) {
            RETURN_IF_ERROR(cast_children_to_common_number_type(filter, node));
        } else {
            goto invalid_operation;
        }
        node->data_type = FDS_FDT_BOOL;
    } break;

    case FDS_FANT_CONTAINS: {
        if (both_children_of_type(node, FDS_FDT_STR)) {
            node->data_type = FDS_FDT_BOOL;
        } else {
            goto invalid_operation;
        }
    } break;

    case FDS_FANT_IN: {
        node->data_type = FDS_FDT_BOOL;
        if (node->right->data_type != FDS_FDT_LIST) {
            goto invalid_operation;
        }

        if (node->left->data_type == node->right->data_subtype || node->right->data_subtype == FDS_FDT_NONE) {
            // No need to do anything
        } else if (is_number_type(node->left->data_type) && is_number_type(node->right->data_subtype)) {
            enum fds_filter_data_type common_type =
                get_common_number_type(node->left->data_type, node->right->data_subtype);
            if (common_type != FDS_FDT_NONE) {
                RETURN_IF_ERROR(cast_node(filter, &node->left, common_type, FDS_FDT_NONE));
                RETURN_IF_ERROR(cast_node(filter, &node->right, FDS_FDT_LIST, common_type));
            } else {
                goto invalid_operation;
            }
        } else {
            goto invalid_operation;
        }
    } break;

    case FDS_FANT_LIST: {
        node->data_type = FDS_FDT_LIST;
        RETURN_IF_ERROR(cast_list_to_same_type(filter, node));
    } break;

    case FDS_FANT_LIST_ITEM: {
        node->data_type = node->right->data_type;
    } break;

    case FDS_FANT_FLAGCMP: {
        if (is_integer_number_type(node->left->data_type)
                && is_integer_number_type(node->right->data_type)) {
            RETURN_IF_ERROR(cast_children_to_common_number_type(filter, node));
            node->data_type = FDS_FDT_BOOL;
        } else {
            goto invalid_operation;
        }
    } break;

    case FDS_FANT_BITAND:
    case FDS_FANT_BITOR:
    case FDS_FANT_BITXOR: {
        if (is_integer_number_type(node->left->data_type)
                && is_integer_number_type(node->right->data_type)) {
            RETURN_IF_ERROR(cast_children_to_common_number_type(filter, node));
            node->data_type = node->left->data_type;
        } else {
            goto invalid_operation;
        }
    } break;

    case FDS_FANT_BITNOT: {
        if (is_integer_number_type(node->left->data_type)) {
            node->data_type = node->left->data_type;
        }
    } break;

    case FDS_FANT_IDENTIFIER:
    case FDS_FANT_CONST: {
        // Pass
    } break;

    default: assert(0); // We want to explicitly handle all cases
    }

    return FDS_FILTER_OK;


invalid_operation:

    if (is_binary_ast_node(node)) {
        add_error_location_message(filter->error_list, node->location,
            "Invalid operation %s for values of type %s(%s) and %s(%s)",
            ast_node_type_to_str(node->node_type),
            data_type_to_str(node->left->data_type), data_type_to_str(node->left->data_subtype),
            data_type_to_str(node->right->data_type), data_type_to_str(node->right->data_subtype));

    } else if (is_unary_ast_node(node)) {
        add_error_location_message(filter->error_list, node->location,
            "Invalid operation %s for value of type %s",
            ast_node_type_to_str(node->node_type), data_type_to_str(node->left->data_type));

    } else if (is_leaf_ast_node(node)) {
        add_error_location_message(filter->error_list, node->location,
            "Invalid operation %s", ast_node_type_to_str(node->node_type));
    }

    PTRACE("returning FAIL because invalid operation");
    return FDS_FILTER_FAIL;
}

int
semantic_analysis(struct fds_filter *filter)
{
    struct fds_filter_ast_node **root_node_ptr = &filter->ast;

    int return_code;

    return_code = apply_to_all_ast_nodes(semantic_resolve_node, filter, root_node_ptr);
    if (return_code != FDS_FILTER_OK) {
        PTRACE("propagating return code");
        return return_code;
    }

    return FDS_FILTER_OK;
}
