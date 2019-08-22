#include <stdbool.h>
#include <libfds.h>
#include "debug.h"
#include "filter.h"
#include "ast.h"

static bool
is_constant_subtree(struct fds_filter_ast_node *node)
{
    if (is_leaf_ast_node(node)) {
        return is_constant_ast_node(node);
    } else if (is_binary_ast_node(node)) {
        return is_constant_subtree(node->left) && is_constant_subtree(node->right);
    } else if (is_unary_ast_node(node)) {
        return is_constant_subtree(node->left);
    }
    assert(!"unhandled node");
    return false;
}

static int
optimize_constant_subtree(struct fds_filter *filter, struct fds_filter_ast_node **node_ptr)
{
    struct fds_filter_ast_node *node = *node_ptr;

    if (node->node_type == FDS_FANT_ROOT
        || node->node_type == FDS_FANT_LIST_ITEM
        || is_leaf_ast_node(node)
        || !is_constant_subtree(node)) {
        return FDS_FILTER_OK;
    }

    struct eval_node *eval_tree = generate_eval_tree(filter, node);
    if (eval_tree == NULL) {
        PTRACE("returning FAIL because eval_tree is null");
        return FDS_FILTER_FAIL;
    }
    evaluate_eval_tree(filter, eval_tree);
    destroy_ast(node->left);
    destroy_ast(node->right);
    node->node_type = FDS_FANT_CONST;
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
    if (list_node->node_type != FDS_FANT_LIST) {
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
        PTRACE("returning FAIL because no memory for list");
        no_memory_error(filter->error_list);
        return FDS_FILTER_FAIL;
    }

    // Evaluate all the list item nodes to allow constant expressions and populate the actual list
    // with their values.
    int item_index = 0;
    while (list_item->node_type != FDS_FANT_LIST) { // Loop until we reach the root of the list
        if (!is_constant_subtree(list_item->right)) {
            PTRACE("returning FAIL because non const list");
            add_error_location_message(filter->error_list, list_item->location,
                "List items must be constant expressions");
            return FDS_FILTER_FAIL;
        }
        if ((return_code = optimize_constant_subtree(filter, &list_item->right)) != FDS_FILTER_OK) {
            PTRACE("propagating return code");
            return return_code;
        }
        list[item_index] = list_item->right->value;
        item_index++;
        list_item = list_item->parent;
    }
    assert(item_index == n_list_items);

    // Transform the AST_LIST node to AST_CONST node with a value of list.
    destroy_ast(list_node->left);
    assert(list_node->right == NULL);
    list_node->node_type = FDS_FANT_CONST;
    list_node->left = NULL;
    list_node->value.list.items = list;
    list_node->value.list.length = n_list_items;

    return FDS_FILTER_OK;
}

static int
convert_ip_address_list_to_trie(struct fds_filter *filter, struct fds_filter_ast_node **node_ptr)
{
    struct fds_filter_ast_node *node = *node_ptr;
    if (!is_constant_ast_node(node) || !is_list_of_type(node, FDS_FDT_IP_ADDRESS)) {
        return FDS_FILTER_OK;
    }

    assert(!node->is_trie);
    fds_trie_t *trie = fds_trie_create();
    if (trie == NULL) {
        PTRACE("returning FAIL because cannot alloc trie");
        no_memory_error(filter->error_list);
        return FDS_FILTER_FAIL;
    }
    for (uint64_t i = 0; i < node->value.list.length; i++) {
        union fds_filter_value value = node->value.list.items[i];
        int return_code = fds_trie_add(trie,
            value.ip_address.version, value.ip_address.bytes, value.ip_address.prefix_length);
        if (!return_code) {
            PTRACE("returning FAIL because cannot add to trie - no memory");
            fds_trie_destroy(trie);
            no_memory_error(filter->error_list);
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
    struct fds_filter_ast_node **root_node_ptr = &filter->ast;

    int return_code;

    // This has to be done before the optimize constant subtree is applied to the whole tree,
    // because the evaluator assumes the special AST_LIST_ITEMS are gone as only const lists
    // are supported right now.
    return_code = apply_to_all_ast_nodes(convert_ast_list_to_actual_list, filter, root_node_ptr);
    if (return_code != FDS_FILTER_OK) {
        PTRACE("propagating return code");
        return return_code;
    }

    return_code = apply_to_all_ast_nodes(optimize_constant_subtree, filter, root_node_ptr);
    if (return_code != FDS_FILTER_OK) {
        PTRACE("propagating return code");
        return return_code;
    }

    // This has to be done after the AST lists are converted to actual lists.
    return_code = apply_to_all_ast_nodes(convert_ip_address_list_to_trie, filter, root_node_ptr);
    if (return_code != FDS_FILTER_OK) {
        PTRACE("propagating return code");
        return return_code;
    }

    return FDS_FILTER_OK;
}
