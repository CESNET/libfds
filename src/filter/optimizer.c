#include <stdbool.h>
#include <libfds.h>
#include "filter.h"
#include "ast_utils.h"

static bool
ast_is_constant_subtree(struct fds_filter_ast_node *node)
{
    if (ast_is_leaf_node(node)) {
        return ast_is_constant_node(node);
    } else if (ast_is_binary_node(node)) {
        return ast_is_constant_subtree(node->left) && ast_is_constant_subtree(node->right);
    } else if (ast_is_unary_node(node)) {
        return ast_is_constant_subtree(node->left);
    }
}

static int
optimize_constant_subtree(struct fds_filter *filter, struct fds_filter_ast_node **node_ptr)
{
    struct fds_filter_ast_node *node = *node_ptr;

    if (!ast_is_constant_subtree(node)
        || node->op == FDS_FILTER_AST_ROOT || node->op == FDS_FILTER_AST_LIST_ITEM) {
        return FDS_FILTER_OK;
    }

    struct eval_node *eval_tree = eval_tree_generate(filter, node);
    if (eval_tree == NULL) {
        pdebug("returning FAIL because eval_tree is null");
        return FDS_FILTER_FAIL;
    }
    eval_tree_evaluate(filter, eval_tree);
    ast_destroy(node->left);
    ast_destroy(node->right);
    node->op = FDS_FILTER_AST_CONST;
    node->value = eval_tree->value;
    node->left = NULL;
    node->right = NULL;
    return FDS_FILTER_OK;
}

/**
 * Convert the lists made from linked AST nodes to actual value of type list
 */
static int
convert_ast_list_to_actual_list(struct fds_filter *filter, struct fds_filter_ast_node **node_ptr)
{
    int return_code;

    struct fds_filter_ast_node *list_node = *node_ptr;
    if (list_node->op != FDS_FILTER_AST_LIST) {
        return FDS_FILTER_OK;
    }

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
        pdebug("returning FAIL because no memory for list");
        error_no_memory(filter);
        return FDS_FILTER_FAIL;
    }

    // Evaluate all the list item nodes to allow constant expressions and populate the actual list
    // with their values.
    int item_index = 0;
    while (list_item->op != FDS_FILTER_AST_LIST) { // Loop until we reach the root of the list
        if (!ast_is_constant_subtree(list_item->right)) {
            pdebug("returning FAIL because non const list");
            error_location_message(filter, list_item->location,
                "List items must be constant expressions");
            return FDS_FILTER_FAIL;
        }
        if ((return_code = optimize_constant_subtree(filter, &list_item->right)) != FDS_FILTER_OK) {
            pdebug("propagating return code");
            return return_code;
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

    return FDS_FILTER_OK;
}

static int
convert_ip_address_list_to_trie(struct fds_filter *filter, struct fds_filter_ast_node *node)
{
    if (!ast_is_constant_node(node) || !ast_has_list_of_type(node, FDS_FILTER_TYPE_IP_ADDRESS)) {
        return FDS_FILTER_OK;
    }

    assert(!node->is_trie);
    fds_trie_t *trie = fds_trie_create();
    if (trie == NULL) {
        pdebug("returning FAIL because cannot alloc trie");
        error_no_memory(filter);
        return FDS_FILTER_FAIL;
    }
    for (int i = 0; i < node->value.list.length; i++) {
        union fds_filter_value value = node->value.list.items[i];
        int return_code = fds_trie_add(trie,
            value.ip_address.version, value.ip_address.bytes, value.ip_address.mask);
        if (!return_code) {
            pdebug("returning FAIL because cannot add to trie - no memory");
            fds_trie_destroy(trie);
            error_no_memory(filter);
            return FDS_FILTER_FAIL;
        }
    }
    node->is_trie = 1;
    node->value.pointer = trie;
    return FDS_FILTER_OK;
}

int
optimize(struct fds_filter *filter)
{
    struct fds_filter_ast_node *root_node_ptr = &filter->ast;

    int return_code;

    // This has to be done before the optimize constant subtree is applied to the whole tree,
    // because the evaluator assumes the special AST_LIST_ITEMS are gone as only const lists
    // are supported right now.
    return_code = ast_apply_to_all_nodes(convert_ast_list_to_actual_list, filter, root_node_ptr);
    if (return_code != FDS_FILTER_OK) {
        pdebug("propagating return code");
        return return_code;
    }

    return_code = ast_apply_to_all_nodes(optimize_constant_subtree, filter, root_node_ptr);
    if (return_code != FDS_FILTER_OK) {
        pdebug("propagating return code");
        return return_code;
    }

    // This has to be done after the AST lists are converted to actual lists.
    return_code = ast_apply_to_all_nodes(convert_ip_address_list_to_trie, filter, root_node_ptr);
    if (return_code != FDS_FILTER_OK) {
        pdebug("propagating return code");
        return return_code;
    }

    return FDS_FILTER_OK;
}
