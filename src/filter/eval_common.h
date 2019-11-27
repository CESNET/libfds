#ifndef FDS_FILTER_EVAL_COMMON_H
#define FDS_FILTER_EVAL_COMMON_H

#include <stdlib.h>

#include <libfds.h>

#include "array.h"
#include "common.h"
#include "values.h"
#include "operations.h"

enum eval_opcode_e {
    EVAL_OP_NONE,
    EVAL_OP_AND,
    EVAL_OP_OR,
    EVAL_OP_NOT,
    EVAL_OP_CALL,
    EVAL_OP_LOOKUP,
    EVAL_OP_ANY,
    EVAL_OP_EXISTS
};

struct eval_node_s {
    enum eval_opcode_e opcode;
    int flags;
    #ifndef NDEBUG
    int data_type;
    operation_s *operation;
    #endif
    value_t value;
    union {
        fds_filter_eval_func_t *eval_function;
        int lookup_id;
    };
    struct eval_node_s *parent;
    union {
        struct {
            struct eval_node_s *left;
            struct eval_node_s *right;
        };
        struct eval_node_s *child;
    };
};

struct eval_context_s {
    fds_filter_field_callback_t *field_callback;
    struct eval_node_s *reevaluate_node;
    bool reset_lookup;
    void *data;
    void *user_ctx;
};

struct destroy_pair_s {
    fds_filter_eval_func_t *destroy_func;
    value_t *value;
};

struct fds_filter_eval_s {
    struct eval_node_s *root;
    struct eval_context_s ctx;
    struct array_s destructors;
};



static inline struct eval_node_s *
create_eval_node()
{
    struct eval_node_s *en = calloc(1, sizeof(struct eval_node_s));
    return en;
}

static inline void
destroy_eval_node(struct eval_node_s *en)
{
    if (!en) {
        return;
    }
    // TODO: destroy value?
    free(en);
}

static inline void
destroy_eval_tree(struct eval_node_s *root)
{
    if (!root) {
        return;
    }
    destroy_eval_tree(root->left);
    destroy_eval_tree(root->right);
    destroy_eval_node(root);
}

static inline void
destroy_eval(struct fds_filter_eval_s *eval)
{
    if (!eval) {
        return;
    }
    ARRAY_FOR_EACH(&eval->destructors, struct destroy_pair_s, dp) {
        dp->destroy_func(dp->value, NULL, NULL);
    }
    array_destroy(&eval->destructors);
    destroy_eval_tree(eval->root);
    free(eval);
}

static inline const char *
eval_opcode_to_str(int opcode)
{
    switch (opcode) {
    case EVAL_OP_NONE:   return "none";
    case EVAL_OP_AND:    return "and";
    case EVAL_OP_OR:     return "or";
    case EVAL_OP_NOT:    return "not";
    case EVAL_OP_CALL:   return "call";
    case EVAL_OP_LOOKUP: return "lookup";
    case EVAL_OP_ANY:    return "any";
    case EVAL_OP_EXISTS: return "exists";
    default:             return "unknown";
    }
}

static inline void
print_eval_tree_one(FILE *out, struct eval_node_s *node, int indent_level)
{
    #define PRINT_INDENT()    for (int i = 0; i < indent_level; i++) { fprintf(out, "  "); }

    if (!node) {
        return;
    }

    PRINT_INDENT();

    fprintf(out, "(%s, ", eval_opcode_to_str(node->opcode));
    fprintf(out, "data type: %s, value: ", data_type_to_str(node->data_type));
    #ifndef NDEBUG
    // if (node->data_type == DT_BOOL)
    print_value(out, node->data_type, &node->value);
    if (node->opcode == EVAL_OP_CALL) {
        fprintf(out, ", ");
        print_operation(out, node->operation);
    }
    #else
    fprintf(out, "N/A");
    #endif

    if (node->left || node->right) {
        fprintf(out, "\n");
    }
    print_eval_tree_one(out, node->left, indent_level + 1);
    print_eval_tree_one(out, node->right, indent_level + 1);

    if (node->left || node->right) {
        PRINT_INDENT();
    }

    fprintf(out, ")\n");

    #undef PRINT_INDENT
}

static inline void
print_eval_tree(FILE *out, struct eval_node_s *root)
{
    print_eval_tree_one(out, root, 0);
}

#endif // FDS_FILTER_EVAL_COMMON_H
