#ifndef FDS_FILTER_AST_COMMON_H
#define FDS_FILTER_AST_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <libfds.h>

#include "values.h"

typedef enum fds_filter_ast_flags_e ast_flags_e;

#define AST_FLAG_NONE FDS_FILTER_AST_FLAG_NONE
#define AST_FLAG_CONST_SUBTREE FDS_FILTER_AST_FLAG_CONST_SUBTREE
#define AST_FLAG_MULTIPLE_EVAL_SUBTREE FDS_FILTER_AST_FLAG_MULTIPLE_EVAL_SUBTREE
#define AST_FLAG_DESTROY_VAL FDS_FILTER_AST_FLAG_DESTROY_VAL

typedef struct fds_filter_ast_node_s ast_node_s;

// typedef fds_filter_eval_func_t eval_func_t;

static inline bool
is_unary_ast_node(ast_node_s *ast_node)
{
    return ast_node->left != NULL && ast_node->right == NULL;
}

static inline bool
is_binary_ast_node(ast_node_s *ast_node)
{
    return ast_node->left != NULL && ast_node->right != NULL;
}

static inline bool
is_leaf_ast_node(ast_node_s *ast_node)
{
    return ast_node->left != NULL && ast_node->right == NULL;
}

static inline bool
is_ast_node_of_symbol(ast_node_s *ast_node, const char *symbol)
{
    return strcmp(ast_node->symbol, symbol) == 0;
}

static inline ast_node_s *
create_ast_node(const char *symbol)
{
    ast_node_s *ast = calloc(1, sizeof(ast_node_s));
    if (!ast) {
        return NULL;
    }
    ast->symbol = symbol;
    return ast;
}

static inline ast_node_s *
create_binary_ast_node(const char *symbol, ast_node_s *left, ast_node_s *right)
{
    ast_node_s *ast = create_ast_node(symbol);
    if (!ast) {
        return NULL;
    }
    ast->left = left;
    ast->right = right;
}

static inline ast_node_s *
create_unary_ast_node(const char *symbol, ast_node_s *operand)
{
    ast_node_s *ast = create_ast_node(symbol);
    if (!ast) {
        return NULL;
    }
    ast->operand = operand;
}

static inline void
destroy_ast(ast_node_s *ast)
{
    if (!ast) {
        return;
    }
    destroy_ast(ast->left);
    destroy_ast(ast->right);
    if (ast->flags & AST_FLAG_DESTROY_VAL) {
        destroy_value(ast->data_type, &ast->value);
    }
    free(ast->name);
    free(ast);
}

static inline void
print_ast_one(FILE *out, ast_node_s *ast, int depth)
{
    if (ast == NULL) {
        return;
    }

    for (int i = 0; i < depth; i++) {
        fprintf(out, "  ");
    }

    fprintf(out, "(%s", ast->symbol);
    fprintf(out, ", data type: %s", data_type_to_str(ast->data_type));
    fprintf(out, ", value: ");
    print_value(out, ast->data_type, &ast->value);

    if (strcmp(ast->symbol, "__name__") == 0) {
        fprintf(out, ", name: %s", ast->name);
    }
    if (ast->flags) {
        fprintf(out, ", flags:");
    }
    if (ast->flags & AST_FLAG_CONST_SUBTREE) {
        fprintf(out, " (const subtree)");
    }
    if (ast->flags & AST_FLAG_MULTIPLE_EVAL_SUBTREE) {
        fprintf(out, " (multi eval subtree)");
    }
    if (ast->flags & AST_FLAG_DESTROY_VAL) {
        fprintf(out, " (destroy value)");
    }

    if (ast->left || ast->right) {
        fprintf(out, "\n");

        print_ast_one(out, ast->left, depth + 1);
        print_ast_one(out, ast->right, depth + 1);

        for (int i = 0; i < depth; i++) {
            fprintf(out, "  ");
        }
    }
    fprintf(out, ")\n");
}

static inline void
print_ast(FILE *out, ast_node_s *ast)
{
    print_ast_one(out, ast, 0);
}

#endif // FDS_FILTER_AST_COMMON_H