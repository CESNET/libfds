#include "eval_generator.h"

#include "common.h"
#include "error.h"
#include "opts.h"
#include "values.h"
#include "operations.h"

#define OPTIMIZE_CONSTANT_SUBTREES  0

static void
unref_value_from_eval_tree(value_t *value, struct eval_node_s *tree)
{
    if (!tree) {
        return;
    }
    unref_value_from_eval_tree(value, tree->left);
    unref_value_from_eval_tree(value, tree->right);
    // if the value matches by value zero it out
    if (memcmp(&tree->value, value, sizeof(value_t)) == 0) {
        tree->value = (value_t){};
    }
}

static void
destruct_unfinished_list(opts_s *opts, int data_type, value_t *items, int num_items)
{
    operation_s *destructor = find_destructor(&opts->operations, data_type);
    if (destructor) {
        for (int i = 0; i < num_items; i++) {
            destructor->eval_func(&items[i], NULL, NULL);
        }
    }
    free(items);
}

static error_t
transform_ast_list_to_literal_list(ast_node_s *ast_node, opts_s *opts, struct fds_filter_eval_s *eval, fds_filter_list_t *out_list)
{
    // the ast node must be a list
    assert(is_ast_node_of_symbol(ast_node, "__list__"));

    // count the number of items
    int n_items = 0;
    for (ast_node_s *li = ast_node->operand; li != NULL; li = li->next) {
        n_items++;
    }

    // if empty list
    if (n_items == 0) {
        out_list->items = NULL;
        out_list->len = 0;
        return NO_ERROR;
    }

    // allocate space for the items
    value_u *items = calloc(n_items, sizeof(value_u));
    if (!items) {
        return MEMORY_ERROR;
    }

    // evaluate each list item node and populate the list
    int idx = 0;
    for (ast_node_s *li = ast_node->operand; li != NULL; li = li->next) {
        struct fds_filter_eval_s *item_eval;
        error_t err = generate_eval_tree(li->item, opts, &item_eval);
        if (err != NO_ERROR) {
            destruct_unfinished_list(opts, ast_node->operand->data_type, items, n_items);
            return err;
        }

        evaluate(item_eval, NULL);
        // TODO: error check?
        items[idx] = item_eval->root->value;
        // delete references from the children subtrees to the value
        unref_value_from_eval_tree(&items[idx], item_eval->root);
        idx++;

        destroy_eval(item_eval);
    }

    // assign the eval node
    out_list->items = items;
    out_list->len = n_items;

    return NO_ERROR;
}

static void
call_constructor_if_any(opts_s *opts, int data_type, value_t *value)
{
    operation_s *constructor = find_constructor(&opts->operations, data_type);
    if (!constructor) {
        return;
    }
    constructor->eval_func(value, NULL, NULL);
}

static error_t
push_destructor_if_any(opts_s *opts, struct fds_filter_eval_s *eval, int data_type, value_t *value)
{
    operation_s *destructor = find_destructor(&opts->operations, data_type);
    if (!destructor) {
        return NO_ERROR;
    }
    struct destroy_pair_s dp = { .destroy_func = destructor->eval_func, .value = value };
    if (!array_push_front(&eval->destructors, &dp)) {
        return MEMORY_ERROR;
    }
    return NO_ERROR;
}

static error_t
generate_eval_tree_recursively(ast_node_s *ast_node, opts_s *opts, struct fds_filter_eval_s *eval, struct eval_node_s **out_eval_node)
{
    if (!ast_node) {
        *out_eval_node = NULL;
        return NO_ERROR;
    }

    if (is_ast_node_of_symbol(ast_node, "__root__")) {
        struct eval_node_s *child_node;
        error_t err = generate_eval_tree_recursively(ast_node->operand, opts, eval, &child_node);
        if (err != NO_ERROR) {
            return err;
        }
        if (!(ast_node->operand->flags & AST_FLAG_MULTIPLE_EVAL_SUBTREE)) {
            *out_eval_node = child_node;
            return NO_ERROR;
        }

        // If the tree needs multiple evaluations and it's not handled by any of the child nodes
        // we have to provide a special node for the root
        struct eval_node_s *eval_node = create_eval_node();
        if (!eval_node) {
            destroy_eval_tree(child_node);
            // TODO: destructors
            return MEMORY_ERROR;
        }
        eval_node->opcode = EVAL_OP_ANY;
        eval_node->child = child_node;
        #ifndef NDEBUG
        eval_node->data_type = DT_BOOL;
        #endif
        *out_eval_node = eval_node;
        return NO_ERROR;
    }

    if (is_ast_node_of_symbol(ast_node, "exists")) {
        assert(is_ast_node_of_symbol(ast_node->operand, "__name__"));
        struct eval_node_s *eval_node = create_eval_node();
        if (!eval_node) {
            return MEMORY_ERROR;
        }
        eval_node->opcode = EVAL_OP_EXISTS;
        eval_node->lookup_id = ast_node->operand->id;
        #ifndef NDEBUG
        eval_node->data_type = ast_node->data_type;
        #endif
        *out_eval_node = eval_node;
        return NO_ERROR;
    }

    if (is_ast_node_of_symbol(ast_node, "__literal__")) {
        struct eval_node_s *eval_node = create_eval_node();
        if (!eval_node) {
            return MEMORY_ERROR;
        }
        eval_node->opcode = EVAL_OP_NONE;
        eval_node->value = ast_node->value;
        // The destruction of the value is now handled by the destructor rather than the AST
        ast_node->flags &= ~AST_FLAG_DESTROY_VAL; 
        call_constructor_if_any(opts, ast_node->data_type, &eval_node->value);
        error_t err = push_destructor_if_any(opts, eval, ast_node->data_type, &eval_node->value);
        if (err != NO_ERROR) {
            destroy_eval_node(eval_node);
            return err;
        }
        #ifndef NDEBUG
        eval_node->data_type = ast_node->data_type;
        #endif
        *out_eval_node = eval_node;
        return NO_ERROR;
    }

    if (is_ast_node_of_symbol(ast_node, "__name__")) {
        struct eval_node_s *eval_node = create_eval_node();
        if (!eval_node) {
            return MEMORY_ERROR;
        }
        eval_node->opcode = EVAL_OP_LOOKUP;
        eval_node->lookup_id = ast_node->id;
        #ifndef NDEBUG
        eval_node->data_type = ast_node->data_type;
        #endif
        *out_eval_node = eval_node;
        return NO_ERROR;
    }

    if (is_ast_node_of_symbol(ast_node, "__list__")) {
        struct eval_node_s *eval_node = create_eval_node();
        if (!eval_node) {
            return MEMORY_ERROR;
        }

        error_t err;
        err = transform_ast_list_to_literal_list(ast_node, opts, eval, &eval_node->value.list);
        if (err != NO_ERROR) {
            destroy_eval_node(eval_node);
            return err;
        }

        err = push_destructor_if_any(opts, eval, ast_node->data_type, &eval_node->value);
        if (err != NO_ERROR) {
            destruct_unfinished_list(opts, ast_node->operand->data_type, eval_node->value.list.items, eval_node->value.list.len);
            destroy_eval_node(eval_node);
            return err;
        }

        call_constructor_if_any(opts, ast_node->data_type, &eval_node->value);
        #ifndef NDEBUG
        eval_node->data_type = ast_node->data_type;
        #endif
        *out_eval_node = eval_node;
        return NO_ERROR;
    }

    struct eval_node_s *left = NULL;
    struct eval_node_s *right = NULL;
    // process children if any
    error_t err;
    err = generate_eval_tree_recursively(ast_node->left, opts, eval, &left);
    if (err != NO_ERROR) {
        // TODO: free
        PTRACE("propagating error");
        return err;
    }
    err = generate_eval_tree_recursively(ast_node->right, opts, eval, &right);
    if (err != NO_ERROR) {
        destroy_eval_tree(left);
        // TODO: destructors
        return err;
    }

    struct eval_node_s *eval_node = create_eval_node();
    if (!eval_node) {
        destroy_eval_tree(left);
        destroy_eval_tree(right);
        return MEMORY_ERROR;
    }

    if (is_ast_node_of_symbol(ast_node, "and")) {
        eval_node->opcode = EVAL_OP_AND;
    } else if (is_ast_node_of_symbol(ast_node, "or")) {
        eval_node->opcode = EVAL_OP_OR;
    } else if (is_ast_node_of_symbol(ast_node, "not")) {
        eval_node->opcode = EVAL_OP_NOT;
    } else {
        eval_node->opcode = EVAL_OP_CALL;
        operation_s *op = find_operation(&opts->operations, ast_node->symbol, ast_node->data_type,
            ast_node->left ? ast_node->left->data_type : DT_NONE, ast_node->right ? ast_node->right->data_type : DT_NONE);
        #ifndef NDEBUG
        eval_node->operation = op;
        #endif
        assert(op);
        eval_node->eval_function = op->eval_func;
        // plan destructor calling if the op needs it
        if (op->flags & OP_FLAG_DESTROY) {
            error_t err = push_destructor_if_any(opts, eval, ast_node->data_type, &eval_node->value);
            if (err != NO_ERROR) {
                destroy_eval_node(eval_node);
                PTRACE("propagating error");
                return err;
            }
        }
    }

    #ifndef NDEBUG
    eval_node->data_type = ast_node->data_type;
    #endif

    eval_node->left = left;
    eval_node->right = right;
    if (left) {
        left->parent = eval_node;
    }
    if (right) {
        right->parent = eval_node;
    }
    *out_eval_node = eval_node;

    return NO_ERROR;
}

error_t
generate_eval_tree(ast_node_s *ast, opts_s *opts, struct fds_filter_eval_s **out_eval)
{
    struct fds_filter_eval_s *eval = calloc(1, sizeof(struct fds_filter_eval_s));
    if (!eval) {
        PTRACE("memory error");
        return MEMORY_ERROR;
    }
    eval->ctx.field_callback = opts->field_callback;
    eval->destructors = array_make(sizeof(struct destroy_pair_s));

    error_t err = generate_eval_tree_recursively(ast, opts, eval, &eval->root);
    if (err != NO_ERROR) {
        // TODO: free
        PTRACE("propagating error");
        return err;
    }

    *out_eval = eval;
    return NO_ERROR;
}
