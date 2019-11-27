#include <assert.h>

#include "common.h"
#include "eval_evaluator.h"

// Reevaluate the tree from the bottom node to top node (excluding the top node)
static void
reevaluate_upwards(struct eval_context_s *ctx, struct eval_node_s *bottom_node, struct eval_node_s *top_node)
{
    assert(bottom_node->opcode == EVAL_OP_LOOKUP);

    struct eval_node_s *node = bottom_node;
    for (;;) {
        switch (node->opcode) {
        case EVAL_OP_LOOKUP: {
            int rc = ctx->field_callback(ctx->user_ctx, false, node->lookup_id, &node->value);
            switch (rc) {
            case FDS_FILTER_OK: {
                // The last item
                ctx->reevaluate_node = NULL;
                break;
            }
            case FDS_FILTER_NOT_FOUND: {
                // Stop the reevaluation entirely, we don't use default value in this case
                ctx->reevaluate_node = NULL;
                return;
            }
            case FDS_FILTER_OK_AND_MORE: {
                // Reevaluate node is already set properly
                // ctx->reevaluate_node = node;
                break;
            }
            default: ASSERT_UNREACHABLE();
            }
            break;
        }
        case EVAL_OP_CALL: {
            node->eval_function(&node->left->value, &node->right->value, &node->value);
            break;
        }
        default: ASSERT_UNREACHABLE();
        }

        // Reached the top node
        if (node == top_node) {
            break;
        }

        node = node->parent;
    }
}

static void
evaluate_recursively(struct eval_context_s *ctx, struct eval_node_s *node);

static inline void
evaluate_while_not_true(struct eval_context_s *ctx, struct eval_node_s *node)
{
    evaluate_recursively(ctx, node);
    printf("--- evaluate_while_not_true: 1 ---\n");
    print_eval_tree(stdout, node);
    while (!node->value.b && ctx->reevaluate_node) {
        reevaluate_upwards(ctx, ctx->reevaluate_node, node);
        printf("--- evaluate_while_not_true: 2 ---\n");
        print_eval_tree(stdout, node);
    }
}

static void
evaluate_recursively(struct eval_context_s *ctx, struct eval_node_s *node)
{
    if (!node) {
        return;
    }

    switch (node->opcode) {
    case EVAL_OP_NONE: {
        break;
    }
    case EVAL_OP_LOOKUP: {
        int rc = ctx->field_callback(ctx->user_ctx, true, node->lookup_id, &node->value);
        switch (rc) {
        case FDS_FILTER_OK:
        case FDS_FILTER_NOT_FOUND: { // The callback should set a default value in this case
            assert(!ctx->reevaluate_node);
            ctx->reset_lookup = true;
            break;
        }
        case FDS_FILTER_OK_AND_MORE: {
            ctx->reevaluate_node = node;
            ctx->reset_lookup = false;
            break;
        }
        default: ASSERT_UNREACHABLE();
        }
        break;
    }
    case EVAL_OP_CALL: {
        // Recursively evaluate children and call the eval func
        evaluate_recursively(ctx, node->left);
        evaluate_recursively(ctx, node->right);
        node->eval_function(&node->left->value, &node->right->value, &node->value);
        break;
    }
    case EVAL_OP_EXISTS: {
        int rc = ctx->field_callback(ctx->user_ctx, true, node->lookup_id, &node->value);
        switch (rc) {
        case FDS_FILTER_OK:
        case FDS_FILTER_OK_AND_MORE: {
            node->value.b = true;
            break;
        }
        case FDS_FILTER_NOT_FOUND: {
            node->value.b = false;
            break;
        }
        default: ASSERT_UNREACHABLE();
        }
        break;
    }
    case EVAL_OP_ANY: {
        evaluate_while_not_true(ctx, node->child);
        node->value.b = node->child->value.b;
        break;
    }
    case EVAL_OP_AND: {
        evaluate_while_not_true(ctx, node->left);
        if (!node->left->value.b) {
            node->value.b = false;
            break;
        }
        evaluate_while_not_true(ctx, node->right);
        node->value.b = node->right->value.b;
        break;
    }
    case EVAL_OP_OR: {
        evaluate_while_not_true(ctx, node->left);
        if (node->left->value.b) {
            node->value.b = true;
            break;
        }
        evaluate_while_not_true(ctx, node->right);
        node->value.b = node->right->value.b;
        break;
    }
    case EVAL_OP_NOT: {
        evaluate_while_not_true(ctx, node->child);
        node->value.b = !node->child->value.b;
        break;
    }
    default: ASSERT_UNREACHABLE();
    }
}

void
evaluate(struct fds_filter_eval_s *eval)
{
    evaluate_recursively(&eval->ctx, eval->root);
}
