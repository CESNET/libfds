#include "common.h"
#include "ast_semantic.h"
#include "operations.h"
#include "type_set.h"

static error_t
invalid_operation_error(ast_node_s *node)
{
    if (is_binary_ast_node(node)) {
        return SEMANTIC_ERROR(node, "invalid operation '%s' for values of type '%s' and '%s'", node->symbol,
                    data_type_to_str(node->left->data_type), data_type_to_str(node->right->data_type));
    } else if (is_unary_ast_node(node)) {
        return SEMANTIC_ERROR(node, "invalid operation '%s' for value of type '%s'", node->symbol,
                data_type_to_str(node->operand->data_type));
    } else {
        return SEMANTIC_ERROR(node, "invalid operation '%s'", node->symbol);
    }
}

static error_t
create_cast_for_node(ast_node_s **node_ptr, int to_datatype)
{
    if (to_datatype == (*node_ptr)->data_type) {
        return NO_ERROR;
    }

    ast_node_s *cast_node = create_unary_ast_node("__cast__", *node_ptr);
    if (!cast_node) {
        PTRACE("memory error");
        return MEMORY_ERROR;
    }

    cast_node->data_type = to_datatype;
    *node_ptr = cast_node;
    return NO_ERROR;
}

static error_t
resolve_node(ast_node_s *ast, opts_s *opts)
{
    int arg1_dt = ast->left ? ast->left->data_type : DT_NONE;
    int arg2_dt = ast->right ? ast->right->data_type : DT_NONE;

    operation_s *exact_op = find_operation(&opts->operations, ast->symbol, DT_ANY, arg1_dt, arg2_dt);
    if (exact_op) {
        ast->data_type = exact_op->out_data_type;
        return NO_ERROR;
    }

    operation_s *op = NULL;
    for (;;) {
        op = find_next_operation(&opts->operations, op, ast->symbol, DT_ANY, DT_ANY, DT_ANY);
        if (!op) {
            return invalid_operation_error(ast);
        }

        if (!can_cast(&opts->operations, arg1_dt, op->arg1_data_type)) {
            continue;
        }
        if (!can_cast(&opts->operations, arg2_dt, op->arg2_data_type)) {
            continue;
        }

        if (ast->left) {
            error_t err = create_cast_for_node(&ast->left, op->arg1_data_type);
            if (err != NO_ERROR) {
                PTRACE("propagating error");
                return err;
            }
        }
        if (ast->right) {
            error_t err = create_cast_for_node(&ast->right, op->arg2_data_type);
            if (err != NO_ERROR) {
                PTRACE("propagating error");
                return err;
            }
        }

        ast->data_type = op->out_data_type;
        return NO_ERROR;
    }

    ASSERT_UNREACHABLE();
}

static error_t
resolve_list(ast_node_s *ast, opts_s *opts)
{
    assert(strcmp("__list__", ast->symbol) == 0);

    struct type_set_s types_in_list = {};
    struct type_set_s potential_types = {};

    ast_node_s *li = ast->operand;
    if (!li) {
        // Empty list
        ast->data_type = DT_LIST | DT_NONE;
        return NO_ERROR;
    }

    add_type_and_all_casts(&opts->operations, &potential_types, li->data_type);
    add_type(&types_in_list, li->data_type);

    while (li = li->next) {
        struct type_set_s possible_casts = {};
        add_type_and_all_casts(&opts->operations, &possible_casts, li->data_type);
        struct type_set_s tmp = {};
        set_intersect(&potential_types, &possible_casts, &tmp);
        potential_types = tmp;

        add_type(&types_in_list, li->data_type);
    }

    if (potential_types.n_types == 0) {
        PTRACE("semantic error");
        return SEMANTIC_ERROR(ast, "cannot cast all list items to same type");
    }

    struct type_set_s priority_types = {};
    set_intersect(&types_in_list, &potential_types, &priority_types);
    if (priority_types.n_types > 0) {
        potential_types = priority_types;
    }

    int best_type = choose_best_type(&opts->operations, &priority_types);
    for (li = ast->operand; li; li = li->next) {
        error_t err = create_cast_for_node(&li->item, best_type);
        if (err != NO_ERROR) {
            // TODO: error handling
            PTRACE("propagating error");
            return err;
        }
    }

    ast->data_type = DT_LIST | best_type;
    ast->flags |= AST_FLAG_CONST_SUBTREE;

    return NO_ERROR;
}


error_t
resolve_types(ast_node_s *ast, opts_s *opts)
{
    // Recursion guard
    if (!ast) {
        return NO_ERROR;
    }
    // Resolve children first
    error_t err;
    err = resolve_types(ast->left, opts);
    if (err != NO_ERROR) {
        PTRACE("propagating error");
        return err;
    }
    err = resolve_types(ast->right, opts);
    if (err != NO_ERROR) {
        PTRACE("propagating error");
        return err;
    }
    
    if (is_ast_node_of_symbol(ast, "__listitem__")) {
        if (!(ast->operand->flags & AST_FLAG_CONST_SUBTREE)) {
            PTRACE("semantic error");
            return SEMANTIC_ERROR(ast, "list items must be const");
        }
        ast->data_type = ast->item->data_type;
        ast->flags |= AST_FLAG_CONST_SUBTREE;
        return NO_ERROR;
    }

    if (is_ast_node_of_symbol(ast, "__list__")) {
        return resolve_list(ast, opts);
    }

    if (is_ast_node_of_symbol(ast, "__literal__")) {
        ast->flags |= AST_FLAG_CONST_SUBTREE;
        return NO_ERROR;
    }

    if (is_ast_node_of_symbol(ast, "and") || is_ast_node_of_symbol(ast, "or")) {
        if (!can_cast(&opts->operations, ast->left->data_type, DT_BOOL)
            || !can_cast(&opts->operations, ast->right->data_type, DT_BOOL)) {
            return invalid_operation_error(ast);
        }
        if (ast->left->flags & AST_FLAG_CONST_SUBTREE && ast->right->flags & AST_FLAG_CONST_SUBTREE) {
            ast->flags |= AST_FLAG_CONST_SUBTREE;
        }
        ast->data_type = DT_BOOL;
        return NO_ERROR;
    }

    if (is_ast_node_of_symbol(ast, "not") || is_ast_node_of_symbol(ast, "__root__")) {
        if (!can_cast(&opts->operations, ast->left->data_type, DT_BOOL)) {
            return invalid_operation_error(ast);
        }
        error_t err = create_cast_for_node(&ast->operand, DT_BOOL);
        if (err != NO_ERROR) {
            PTRACE("propagating error");
            return err; 
        }
        if (ast->left->flags & AST_FLAG_CONST_SUBTREE) {
            ast->flags |= AST_FLAG_CONST_SUBTREE;
        }
        ast->data_type = DT_BOOL;
        return NO_ERROR;
    }

    if (is_ast_node_of_symbol(ast, "exists")) {
        if (!is_ast_node_of_symbol(ast->operand, "__name__")) {
            return SEMANTIC_ERROR(ast, "expected field name for exists");
        } else if (ast->operand->flags & AST_FLAG_CONST_SUBTREE) {
            return SEMANTIC_ERROR(ast, "expected non-const field name for exists");
        }
        ast->data_type = DT_BOOL;
        return NO_ERROR;
    }

    if (is_ast_node_of_symbol(ast, "__name__")) {
        int flags = 0;
        int rv = opts->lookup_callback(ast->name, &ast->id, &ast->data_type, &flags);
        if (rv != FDS_FILTER_OK) {
            return SEMANTIC_ERROR(ast, "invalid name");
        }
        if (flags & FDS_FILTER_FLAG_CONST) {
            ast->flags |= AST_FLAG_CONST_SUBTREE;
        } else {
            ast->flags |= AST_FLAG_MULTIPLE_EVAL_SUBTREE;
        }
        return NO_ERROR;
    }

    // ... If none of the above

    // Propagate flags
    if (ast->left && ast->right) {
        if (ast->left->flags & AST_FLAG_MULTIPLE_EVAL_SUBTREE || ast->right->flags & AST_FLAG_MULTIPLE_EVAL_SUBTREE) {
            ast->flags |= AST_FLAG_MULTIPLE_EVAL_SUBTREE;
        }
        if (ast->left->flags & AST_FLAG_CONST_SUBTREE && ast->right->flags & AST_FLAG_CONST_SUBTREE) {
            ast->flags |= AST_FLAG_CONST_SUBTREE;
        }
    } else if (ast->left) {
        if (ast->left->flags & AST_FLAG_MULTIPLE_EVAL_SUBTREE) {
            ast->flags |= AST_FLAG_MULTIPLE_EVAL_SUBTREE;
        }
        if (ast->left->flags & AST_FLAG_CONST_SUBTREE) {
            ast->flags |= AST_FLAG_CONST_SUBTREE;
        }
    }
    return resolve_node(ast, opts);
}