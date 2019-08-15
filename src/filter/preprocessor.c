#include <libfds.h>
#include "filter.h"
#include "ast_utils.h"

/**
 * Adds a special ANY node at certain places in the AST as a hint for evaluator
 */
static int
add_any_node(struct fds_filter *filter, struct fds_filter_ast_node **node_ptr)
{
    struct fds_filter_ast_node *node = *node_ptr;
    if (node->op == FDS_FILTER_AST_NOT || node->op == FDS_FILTER_AST_ROOT) {
        struct fds_filter_ast_node *new_node = ast_node_create();
        if (new_node == NULL) {
            error_no_memory(filter);
            return FDS_FILTER_FAIL;
        }
        new_node->op = FDS_FILTER_AST_ANY;
        new_node->left = node->left;
        node->left = new_node;

    } else if (node->op == FDS_FILTER_AST_AND || node->op == FDS_FILTER_AST_OR) {
        struct fds_filter_ast_node *new_node_left = ast_node_create();
        if (new_node_left == NULL) {
            error_no_memory(filter);
            return FDS_FILTER_FAIL;
        }
        new_node_left->op = FDS_FILTER_AST_ANY;
        new_node_left->left = node->left;
        node->left = new_node_left;

        struct fds_filter_ast_node *new_node_right = ast_node_create();
        if (new_node_right == NULL) {
            error_no_memory(filter);
            return FDS_FILTER_FAIL;
        }
        new_node_right->op = FDS_FILTER_AST_ANY;
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
    if (node->op != FDS_FILTER_AST_IDENTIFIER) {
        return FDS_FILTER_OK;
    }

    pdebug("Looking up identifier '%s'", node->identifier_name);

    struct fds_filter_identifier_attributes attributes;
    attributes.id = 0;
    attributes.match_mode = FDS_FILTER_MATCH_MODE_NONE;
    attributes.type = FDS_FILTER_TYPE_NONE;
    attributes.subtype = FDS_FILTER_TYPE_NONE;

    // Return value of 1 is expected on successful lookup
    int return_code = filter->lookup_callback(node->identifier_name, filter->user_context, &attributes);
    if (return_code != FDS_FILTER_OK) {
        pdebug("Identifier lookup failed!");
        error_location_message(filter, node->location, "Unknown identifier '%s'", node->identifier_name);
        return FDS_FILTER_FAIL;
    }

    // Check missing datatype
    if (attributes.type == FDS_FILTER_TYPE_NONE
        || (attributes.type == FDS_FILTER_TYPE_LIST && attributes.subtype == FDS_FILTER_TYPE_NONE)) {
        pdebug("Identifier type is missing!");
        error_location_message(filter, node->location, "Type of identifier '%s' missing", node->identifier_name);
        return FDS_FILTER_FAIL;
    }

    // Set default match mode if missing
    if (attributes.match_mode == FDS_FILTER_MATCH_MODE_NONE) {
        if (attributes.type == FDS_FILTER_TYPE_IP_ADDRESS) {
            attributes.match_mode = FDS_FILTER_MATCH_MODE_PARTIAL;
        } else {
            attributes.match_mode = FDS_FILTER_MATCH_MODE_FULL;
        }
    }

    node->identifier_id = attributes.id;
    node->identifier_type = attributes.identifier_type;
    node->match_mode = attributes.match_mode;
    node->type = attributes.type;
    node->subtype = attributes.subtype;

    if (attributes.identifier_type == FDS_FILTER_IDENTIFIER_CONST) {
        filter->const_callback(attributes.id, filter->user_context, &node->value);
    }

    pdebug("Identifier lookup results: id=%d, type=%s, datatype=%s:%s, matchmode=%s",
        attributes.id,
        attributes.identifier_type == FDS_FILTER_IDENTIFIER_CONST ? "CONST" : "FIELD",
        type_to_str(attributes.type), type_to_str(attributes.subtype),
        attributes.match_mode == FDS_FILTER_MATCH_MODE_PARTIAL ? "PARTIAL" : "FULL"
    );

    return FDS_FILTER_OK;
}

int
preprocess(struct fds_filter *filter)
{
    struct fds_filter_ast_node *root_node_ptr = &filter->ast;

    int return_code;

    return_code = ast_apply_to_all_nodes(add_any_node, filter, root_node_ptr);
    if (return_code != FDS_FILTER_OK) {
        return return_code;
    }

    return_code = ast_apply_to_all_nodes(lookup_identifier, filter, root_node_ptr);
    if (return_code != FDS_FILTER_OK) {
        return return_code;
    }

    return FDS_FILTER_OK;
}