#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filter.h"
#include "debug.h"

struct fds_filter_ast_node *
create_ast_node()
{
    struct fds_filter_ast_node *node = calloc(1, sizeof(struct fds_filter_ast_node));
    if (node == NULL) {
        return NULL;
    }
    return node;
}

void
destroy_ast(struct fds_filter_ast_node *node)
{
    if (node == NULL) {
        return;
    }

    if (node->node_type == FDS_FANT_CONST) {
        if (node->data_type == FDS_FDT_STR) {
            free(node->value.string.chars);
        } else if (node->data_type == FDS_FDT_LIST && !node->is_trie) {
            if (node->data_subtype == FDS_FDT_STR) {
                for (uint64_t i = 0; i < node->value.list.length; i++) {
                    free(node->value.list.items[i].string.chars);
                }
            }
            free(node->value.list.items);
        } else if (node->is_trie) {
            fds_trie_destroy(node->value.pointer);
        }
    } else if (node->node_type == FDS_FANT_IDENTIFIER) {
        free(node->identifier_name);
        if (node->is_trie) {
            fds_trie_destroy(node->value.pointer);
        }
    }

    destroy_ast(node->left);
    destroy_ast(node->right);
    free(node);
}

int
apply_to_all_ast_nodes(int (*function)(struct fds_filter *, struct fds_filter_ast_node **),
                   struct fds_filter *filter, struct fds_filter_ast_node **node_ptr)
{
    if (*node_ptr == NULL) {
        return FDS_FILTER_OK;
    }

    int return_code;

    return_code = apply_to_all_ast_nodes(function, filter, &(*node_ptr)->left);
    if (return_code != FDS_FILTER_OK) {
        return return_code;
    }

    return_code = apply_to_all_ast_nodes(function, filter, &(*node_ptr)->right);
    if (return_code != FDS_FILTER_OK) {
        return return_code;
    }

    return_code = function(filter, node_ptr);
    if (return_code != FDS_FILTER_OK) {
        return return_code;
    }

    return FDS_FILTER_OK;
}
