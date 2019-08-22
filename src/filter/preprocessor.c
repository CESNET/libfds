#include <stdbool.h>
#include <libfds.h>
#include "filter.h"
#include "debug.h"
#include "ast.h"

static bool
is_subtree_flags(struct fds_filter_ast_node *node)
{
    if (node == NULL) {
        return NULL;
    }
    if (is_leaf_ast_node(node)) {
        return node->is_flags;
    }
    return is_subtree_flags(node->left) || is_subtree_flags(node->right);
}

/**
 * Adds a special ANY node at certain places in the AST as a hint for evaluator
 */
static int
add_any_node(struct fds_filter *filter, struct fds_filter_ast_node **node_ptr)
{
    struct fds_filter_ast_node *node = *node_ptr;
    if (node->node_type == FDS_FANT_NOT || node->node_type == FDS_FANT_ROOT) {
        struct fds_filter_ast_node *new_node = create_ast_node();
        if (new_node == NULL) {
            no_memory_error(filter->error_list);
            return FDS_FILTER_FAIL;
        }
        new_node->node_type = FDS_FANT_ANY;
        new_node->left = node->left;
        node->left = new_node;

    } else if (node->node_type == FDS_FANT_AND || node->node_type == FDS_FANT_OR) {
        struct fds_filter_ast_node *new_node_left = create_ast_node();
        if (new_node_left == NULL) {
            no_memory_error(filter->error_list);
            return FDS_FILTER_FAIL;
        }
        new_node_left->node_type = FDS_FANT_ANY;
        new_node_left->left = node->left;
        node->left = new_node_left;

        struct fds_filter_ast_node *new_node_right = create_ast_node();
        if (new_node_right == NULL) {
            no_memory_error(filter->error_list);
            return FDS_FILTER_FAIL;
        }
        new_node_right->node_type = FDS_FANT_ANY;
        new_node_right->left = node->right;
        node->right = new_node_right;
    }

    return FDS_FILTER_OK;
}

/**
 * Looks up identifiers in the AST, gets their type, and their value if they are const
 */
static int
lookup_identifier(struct fds_filter *filter, struct fds_filter_ast_node **node_ptr)
{
    struct fds_filter_ast_node *node = *node_ptr;
    if (node->node_type != FDS_FANT_IDENTIFIER) {
        return FDS_FILTER_OK;
    }

    PDEBUG("Looking up identifier '%s'", node->identifier_name);

    struct fds_filter_identifier_attributes attributes;
    attributes.id = 0;
    attributes.is_flags = false;
    attributes.data_type = FDS_FDT_NONE;
    attributes.data_subtype = FDS_FDT_NONE;

    // Return value of 1 is expected on successful lookup
    int return_code = filter->lookup_callback(node->identifier_name, filter->user_context, &attributes);
    if (return_code != FDS_FILTER_OK) {
        PDEBUG("ERROR: Identifier lookup failed!");
        add_error_location_message(filter->error_list, node->location, "Unknown identifier '%s'", node->identifier_name);
        return FDS_FILTER_FAIL;
    }

    // Check missing datatype
    if (attributes.data_type == FDS_FDT_NONE
        || (attributes.data_type == FDS_FDT_LIST && attributes.data_subtype == FDS_FDT_NONE)) {
        PDEBUG("ERROR: Identifier type is missing!");
        add_error_location_message(filter->error_list, node->location, "Type of identifier '%s' missing", node->identifier_name);
        return FDS_FILTER_FAIL;
    }

    node->identifier_id = attributes.id;
    node->identifier_type = attributes.identifier_type;
    node->is_flags = attributes.is_flags;
    node->data_type = attributes.data_type;
    node->data_subtype = attributes.data_subtype;

    if (attributes.identifier_type == FDS_FIT_CONST) {
        filter->const_callback(attributes.id, filter->user_context, &node->value);
    }

    PDEBUG("Identifier lookup results -> id: %d, type: %s, data type: %s:%s, flags: %s",
        attributes.id,
        attributes.identifier_type == FDS_FIT_CONST ? "CONST" : "FIELD",
        data_type_to_str(attributes.data_type), data_type_to_str(attributes.data_subtype),
        attributes.is_flags ? "YES" : "NO"
    );

    return FDS_FILTER_OK;
}

static int
transform_implicit_node(struct fds_filter *filter, struct fds_filter_ast_node **node_ptr)
{
    struct fds_filter_ast_node *node = *node_ptr;
    if (node->node_type != FDS_FANT_IMPLICIT) {
        return FDS_FILTER_OK;
    }

    bool is_left_flags = is_subtree_flags(node->left);
    bool is_right_flags = is_subtree_flags(node->left);
    // TODO: Warning or error if not both?
    if (is_left_flags || is_right_flags) {
        node->node_type = FDS_FANT_FLAGCMP;
    } else {
        node->node_type = FDS_FANT_EQ;
    }
    return FDS_FILTER_OK;
}

int
preprocess(struct fds_filter *filter)
{
    struct fds_filter_ast_node **root_node_ptr = &filter->ast;

    int return_code;

    return_code = apply_to_all_ast_nodes(lookup_identifier, filter, root_node_ptr);
    if (return_code != FDS_FILTER_OK) {
        return return_code;
    }

    return_code = apply_to_all_ast_nodes(add_any_node, filter, root_node_ptr);
    if (return_code != FDS_FILTER_OK) {
        return return_code;
    }

    return_code = apply_to_all_ast_nodes(transform_implicit_node, filter, root_node_ptr);
    if (return_code != FDS_FILTER_OK) {
        return return_code;
    }

    return FDS_FILTER_OK;
}