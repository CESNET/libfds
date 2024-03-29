/**
 * \file src/filter/eval_common.h
 * \author Michal Sedlak <xsedla0v@stud.fit.vutbr.cz>
 * \brief Common evaluation definitions file
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

#ifndef FDS_FILTER_EVAL_COMMON_H
#define FDS_FILTER_EVAL_COMMON_H

#include <stdlib.h>

#include <libfds.h>

#include "common.h"
#include "error.h"
#include "values.h"
#include "operations.h"

typedef enum eval_opcode {
    EVAL_OP_NONE,
    EVAL_OP_VALUE,
    EVAL_OP_AND,
    EVAL_OP_OR,
    EVAL_OP_NOT,
    EVAL_OP_CAST_CALL,
    EVAL_OP_UNARY_CALL,
    EVAL_OP_BINARY_CALL,
    EVAL_OP_DATA_CALL,
    EVAL_OP_ANY,
    EVAL_OP_EXISTS
} eval_opcode_e;

typedef struct eval_node {
    eval_opcode_e opcode;

    IF_DEBUG(
    int datatype;
    fds_filter_op_s *operation;
    )

    fds_filter_value_u value;
    union {
        fds_filter_unary_fn_t *unary_fn;
        fds_filter_binary_fn_t *binary_fn;
        fds_filter_cast_fn_t *cast_fn;
        fds_filter_destructor_fn_t *destructor_fn;
        int lookup_id;
    };
    struct eval_node *parent;
    union {
        struct {
            struct eval_node *left;
            struct eval_node *right;
        };
        struct eval_node *child;
    };
} eval_node_s;

typedef struct eval_runtime {
    fds_filter_data_cb_t *data_cb;
    eval_node_s *reevaluate_node;
    bool reset_lookup;
    void *data;
    void *user_ctx;
} eval_runtime_s;


static inline eval_node_s *
create_eval_node()
{
    eval_node_s *en = calloc(1, sizeof(eval_node_s));
    return en;
}

static inline void
destroy_eval_node(eval_node_s *en)
{
    if (!en) {
        return;
    }
    if (en->opcode == EVAL_OP_NONE && en->destructor_fn) {
        en->destructor_fn(&en->value);
        memset(&en->value, 0, sizeof(fds_filter_value_u));
    }
    free(en);
}

static inline void
destroy_eval_tree(eval_node_s *root)
{
    if (!root) {
        return;
    }
    destroy_eval_tree(root->left);
    destroy_eval_tree(root->right);
    destroy_eval_node(root);
}

static inline const char *
eval_opcode_to_str(int opcode)
{
    switch (opcode) {
    case EVAL_OP_NONE:         return "none";
    case EVAL_OP_AND:          return "and";
    case EVAL_OP_OR:           return "or";
    case EVAL_OP_NOT:          return "not";
    case EVAL_OP_UNARY_CALL:   return "unary_call";
    case EVAL_OP_BINARY_CALL:  return "binary_call";
    case EVAL_OP_CAST_CALL:    return "cast_call";
    case EVAL_OP_DATA_CALL:    return "data_call";
    case EVAL_OP_ANY:          return "any";
    case EVAL_OP_EXISTS:       return "exists";
    default:                   return "unknown";
    }
}

#ifdef FDS_FILTER_DEBUG
static inline void
print_eval_tree_rec(FILE *out, eval_node_s *node, int indent_level)
{

    #define PRINT_INDENT()    for (int i = 0; i < indent_level; i++) { fprintf(out, "  "); }
    if (!node) {
        return;
    }

    PRINT_INDENT();

    fprintf(out, "data type: %s, value: ", data_type_to_str(node->datatype));

    print_value(out, node->datatype, &node->value);
    if (node->opcode == EVAL_OP_UNARY_CALL
        || node->opcode == EVAL_OP_BINARY_CALL
        || node->opcode == EVAL_OP_CAST_CALL) {
        fprintf(out, ", ");
        print_operation(out, node->operation);
    }


    if (node->left || node->right) {
        fprintf(out, "\n");
    }
    print_eval_tree_rec(out, node->left, indent_level + 1);
    print_eval_tree_rec(out, node->right, indent_level + 1);

    if (node->left || node->right) {
        PRINT_INDENT();
    }

    fprintf(out, ")\n");

    #undef PRINT_INDENT
}


static inline void
print_eval_tree(FILE *out, eval_node_s *root)
{
    print_eval_tree_rec(out, root, 0);
}
#endif

error_t
generate_eval_tree(fds_filter_ast_node_s *ast, fds_filter_opts_t *opts, bool second_run, eval_node_s **out_eval_tree);

void
evaluate_eval_tree(eval_node_s *root, eval_runtime_s *runtime);


#endif // FDS_FILTER_EVAL_COMMON_H
