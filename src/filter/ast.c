#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filter.h"

struct fds_filter_ast_node *
ast_node_create()
{
    struct fds_filter_ast_node *node = calloc(1, sizeof(struct fds_filter_ast_node));
    if (node == NULL) {
        return NULL;
    }
    return node;
}

void
ast_destroy(struct fds_filter_ast_node *node)
{
    if (node == NULL) {
        return;
    }

    if (node->op == FDS_FILTER_AST_CONST) {
        if (node->type == FDS_FILTER_TYPE_STR) {
            free(node->value.string.chars);
        } else if (node->type == FDS_FILTER_AST_LIST) {
            if (node->is_trie) {
                fds_trie_destroy(node->value.pointer);
            } else {
                if (node->subtype == FDS_FILTER_TYPE_STR) {
                    for (int i = 0; i < node->value.list.length; i++) {
                        free(node->value.list.items[i].string.chars);
                    }
                }
                free(node->value.list.items);
            }
        }
    }

    ast_destroy(node->left);
    ast_destroy(node->right);
    free(node);
}
