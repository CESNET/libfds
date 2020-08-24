/**
 * \file src/filter/ast_common.h
 * \author Michal Sedlak <xsedla0v@stud.fit.vutbr.cz>
 * \brief Common AST functions file
 * \date 2020
 */
 
/* 
 * Copyright (C) 2020 CESNET, z.s.p.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is'', and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */
#ifndef FDS_FILTER_AST_COMMON_H
#define FDS_FILTER_AST_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <libfds.h>

#include "values.h"
#include "error.h"
#include "opts.h"

static inline bool
is_unary_ast_node(fds_filter_ast_node_s *ast)
{
    return ast->left && !ast->right;
}

static inline bool
is_binary_ast_node(fds_filter_ast_node_s *ast)
{
    return ast->left && ast->right;
}

static inline bool
ast_node_symbol_is(fds_filter_ast_node_s *ast_node, const char *symbol)
{
    return strcmp(ast_node->symbol, symbol) == 0;
}

static inline fds_filter_ast_node_s *
create_ast_node(const char *symbol)
{
    fds_filter_ast_node_s *ast = calloc(1, sizeof(fds_filter_ast_node_s));
    if (!ast) {
        return NULL;
    }
    ast->symbol = symbol;
    return ast;
}

static inline fds_filter_ast_node_s *
create_binary_ast_node(const char *symbol, fds_filter_ast_node_s *left, fds_filter_ast_node_s *right)
{
    fds_filter_ast_node_s *ast = create_ast_node(symbol);
    if (!ast) {
        return NULL;
    }
    ast->left = left;
    ast->right = right;
    ast->cursor_begin = left->cursor_begin;
    ast->cursor_end = right->cursor_end;
    left->parent = ast;
    right->parent = ast;
    return ast;
}

static inline fds_filter_ast_node_s *
create_unary_ast_node(const char *symbol, fds_filter_ast_node_s *child)
{
    fds_filter_ast_node_s *ast = create_ast_node(symbol);
    if (!ast) {
        return NULL;
    }
    ast->child = child;
    child->parent = ast;
    ast->cursor_begin = child->cursor_begin;
    ast->cursor_end = child->cursor_end;
    return ast;
}

static inline void
destroy_ast(fds_filter_ast_node_s *ast)
{
    if (!ast) {
        return;
    }
    destroy_ast(ast->left);
    destroy_ast(ast->right);
    if (ast->flags & FDS_FAF_DESTROY_VAL) {
        destroy_value(ast->datatype, &ast->value);
    }
    free(ast->name);
    free(ast);
}

static inline void
print_ast_nodes_recursively(FILE *out, fds_filter_ast_node_s *ast, int depth)
{
    if (ast == NULL) {
        return;
    }

    for (int i = 0; i < depth; i++) {
        fprintf(out, "  ");
    }

    fprintf(out, "(%s", ast->symbol);
    fprintf(out, ", data type: %s", data_type_to_str(ast->datatype));
    fprintf(out, ", value: ");
    print_value(out, ast->datatype, &ast->value);

    if (strcmp(ast->symbol, "__name__") == 0) {
        fprintf(out, ", name: %s", ast->name);
    }
    if (ast->flags) {
        fprintf(out, ", flags:");
    }
    if (ast->flags & FDS_FAF_CONST_SUBTREE) {
        fprintf(out, " (const subtree)");
    }
    if (ast->flags & FDS_FAF_MULTIPLE_EVAL_SUBTREE) {
        fprintf(out, " (multi eval subtree)");
    }
    if (ast->flags & FDS_FAF_DESTROY_VAL) {
        fprintf(out, " (destroy value)");
    }

    fprintf(out, ", ptr: %p, parent: %p", ast, ast->parent);

    if (ast->left || ast->right) {
        fprintf(out, "\n");

        print_ast_nodes_recursively(out, ast->left, depth + 1);
        print_ast_nodes_recursively(out, ast->right, depth + 1);

        for (int i = 0; i < depth; i++) {
            fprintf(out, "  ");
        }
    }
    fprintf(out, ")\n");
}

static inline void
print_ast(FILE *out, fds_filter_ast_node_s *ast)
{
    print_ast_nodes_recursively(out, ast, 0);
}

error_t
resolve_types(fds_filter_ast_node_s *ast, fds_filter_opts_t *opts);

#endif // FDS_FILTER_AST_COMMON_H