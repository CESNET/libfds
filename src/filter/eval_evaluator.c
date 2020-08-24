/**
 * \file src/filter/eval_evaluator.h
 * \author Michal Sedlak <xsedla0v@stud.fit.vutbr.cz>
 * \brief Evaluation tree evaluator implementation file
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

#include <assert.h>

#include "eval_common.h"

// Reevaluate the tree from the bottom node to top node (excluding the top node)
static void
reevaluate_upwards(eval_runtime_s *runtime, eval_node_s *bottom_node, eval_node_s *top_node)
{
    assert(bottom_node->opcode == EVAL_OP_DATA_CALL);

    eval_node_s *node = bottom_node;
    for (;;) {
        switch (node->opcode) {
        case EVAL_OP_DATA_CALL: {
            int rc = runtime->data_cb(runtime->user_ctx, false, node->lookup_id, runtime->data, &node->value);
            switch (rc) {
            case FDS_OK: {
                // The last item
                runtime->reevaluate_node = NULL;
                break;
            }
            case FDS_ERR_NOTFOUND: {
                // Stop the reevaluation entirely, we don't use default value in this case
                runtime->reevaluate_node = NULL;
                return;
            }
            case FDS_OK_MORE: {
                // Reevaluate node is already set properly
                // runtime->reevaluate_node = node;
                break;
            }
            default: assert(0 && "invalid value");
            }
            break;
        }
        case EVAL_OP_UNARY_CALL: {
            node->unary_fn(&node->left->value, &node->value);
            break;
        }
        case EVAL_OP_BINARY_CALL: {
            node->binary_fn(&node->left->value, &node->right->value, &node->value);
            break;
        }
        case EVAL_OP_CAST_CALL: {
            node->cast_fn(&node->left->value, &node->value);
            break;
        }
        default: assert(0 && "invalid value");
        }

        // Reached the top node
        if (node == top_node) {
            break;
        }

        node = node->parent;
    }
}

static void
evaluate_recursively(eval_runtime_s *runtime, eval_node_s *node);

static inline void
evaluate_while_not_true(eval_runtime_s *runtime, eval_node_s *node)
{
    evaluate_recursively(runtime, node);
    //fprintf(stderr, "--- evaluate_while_not_true: 1 ---\n");
    //print_eval_tree(stderr, node);
    while (!node->value.b && runtime->reevaluate_node) {
        reevaluate_upwards(runtime, runtime->reevaluate_node, node);
        //fprintf(stderr, "--- evaluate_while_not_true: 2 ---\n");
        //print_eval_tree(stderr, node);
    }
}

static void
evaluate_recursively(eval_runtime_s *runtime, eval_node_s *node)
{
    if (!node) {
        return;
    }

    switch (node->opcode) {
    case EVAL_OP_NONE: {
        break;
    }
    case EVAL_OP_DATA_CALL: {
        int rc = runtime->data_cb(runtime->user_ctx, true, node->lookup_id, runtime->data, &node->value);
        switch (rc) {
        case FDS_OK:
        case FDS_ERR_NOTFOUND: { // The callback should set a default value in this case
            runtime->reevaluate_node = NULL;
            runtime->reset_lookup = true;
            break;
        }
        case FDS_OK_MORE: {
            runtime->reevaluate_node = node;
            runtime->reset_lookup = false;
            break;
        }
        default: assert(0 && "invalid value");
        }
        break;
    }
    case EVAL_OP_UNARY_CALL: {
        evaluate_recursively(runtime, node->left);
        node->unary_fn(&node->left->value, &node->value);
        break;
    }
    case EVAL_OP_BINARY_CALL: {
        evaluate_recursively(runtime, node->left);
        evaluate_recursively(runtime, node->right);
        node->binary_fn(&node->left->value, &node->right->value, &node->value);
        break;
    }
    case EVAL_OP_CAST_CALL: {
        evaluate_recursively(runtime, node->left);
        node->cast_fn(&node->left->value, &node->value);
        break;
    }
    case EVAL_OP_EXISTS: {
        int rc = runtime->data_cb(runtime->user_ctx, true, node->lookup_id, runtime->data, &node->value);
        switch (rc) {
        case FDS_OK:
        case FDS_OK_MORE: {
            node->value.b = true;
            break;
        }
        case FDS_ERR_NOTFOUND: {
            node->value.b = false;
            break;
        }
        default: assert(0 && "invalid value");
        }
        break;
    }
    case EVAL_OP_ANY: {
        evaluate_while_not_true(runtime, node->child);
        node->value.b = node->child->value.b;
        break;
    }
    case EVAL_OP_AND: {
        evaluate_while_not_true(runtime, node->left);
        if (!node->left->value.b) {
            node->value.b = false;
            break;
        }
        evaluate_while_not_true(runtime, node->right);
        node->value.b = node->right->value.b;
        break;
    }
    case EVAL_OP_OR: {
        evaluate_while_not_true(runtime, node->left);
        if (node->left->value.b) {
            node->value.b = true;
            break;
        }
        evaluate_while_not_true(runtime, node->right);
        node->value.b = node->right->value.b;
        break;
    }
    case EVAL_OP_NOT: {
        evaluate_while_not_true(runtime, node->child);
        node->value.b = !node->child->value.b;
        break;
    }
    default: assert(0 && "invalid value");
    }
}

void
evaluate_eval_tree(eval_node_s *root, eval_runtime_s *runtime)
{
    evaluate_recursively(runtime, root);
}
