#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

fds_filter_ast_node_t *
ast_create(fds_filter_op_t op, fds_filter_ast_node_t *left, fds_filter_ast_node_t *right)
{
	fds_filter_ast_node_t *node = malloc(sizeof(fds_filter_ast_node_t *));
	if (node == NULL) {
		return NULL;
	}
	node->op = op;
	node->left = left;
	node->right = right;
	return node;
}

void 
ast_destroy(fds_filter_ast_node_t *node)
{
	if (node == NULL) {
		return;
	}
	// TODO: check based on node type
	// TODO: also destroy value etc
	free(node->left);
	free(node->right);
	free(node);
}

void 
fds_filter_ast_print_tree(fds_filter_ast_node_t *node)
{
	if (node == NULL) {
		return; 
	}
	static int level = 0;
	for (int i = 0; i < level; i++) { 
		printf("    "); 
	}
	printf("(%s)\n", fds_filter_op_str[node->op]);
	level++;
	// TODO: check node type or op and perform action based on it
	fds_filter_ast_print_tree(node->left);
	fds_filter_ast_print_tree(node->right);
	level--;
}
