#include "common.h"
#include "ast_common.h"
#include "operations.h"

static error_t
invalid_op_err(fds_filter_ast_node_s *node)
{
    if (is_binary_ast_node(node)) {
        return SEMANTIC_ERROR(
            node, 
            "invalid operation '%s' for values of type '%s' and '%s'", 
            node->symbol,
            data_type_to_str(node->left->datatype), 
            data_type_to_str(node->right->datatype)
        );
    } else if (is_unary_ast_node(node)) {
        return SEMANTIC_ERROR(
            node,
            "invalid operation '%s' for value of type '%s'", 
            node->symbol,
            data_type_to_str(node->child->datatype)
        );
    } else {
        return SEMANTIC_ERROR(
            node,
            "invalid operation '%s'",
            node->symbol
        );
    }
}

/**
 * Propagate flags to the provided node from its children
 */
static void
propagate_flags(fds_filter_ast_node_s *ast)
{
    // If any of the children are multiple eval, the parent is also multiple eval
    if ((ast->left && ast->left->flags & AST_FLAG_MULTIPLE_EVAL_SUBTREE) 
            || (ast->right && ast->right->flags & AST_FLAG_MULTIPLE_EVAL_SUBTREE)) {
        ast->flags |= AST_FLAG_MULTIPLE_EVAL_SUBTREE;
    }
    // If both children are constant, parent is also constant
    if (ast->left                                                          // Has atleast one child
            && ast->left->flags & AST_FLAG_CONST_SUBTREE                   // The child is constant subtree 
            && (!ast->right || ast->right->flags & AST_FLAG_CONST_SUBTREE) // Right child is also constant subtree if there is one
        ) { 
        ast->flags |= AST_FLAG_CONST_SUBTREE;
    }
    
}

/**
 * Inserts a new empty AST node above in place of the provided node, the old node is the child of the new node
 */ 
static fds_filter_ast_node_s *
insert_ast_node(fds_filter_ast_node_s **node_ptr)
{
    fds_filter_ast_node_s *node = calloc(1, sizeof(fds_filter_ast_node_s));
    if (!node) {
        return NULL;
    }
    node->child = *node_ptr;
    *node_ptr = node;
    return node;
}

/**
 * Inserts a constructor node in place of the provided node
 */
static fds_filter_ast_node_s *
insert_constructor_node(fds_filter_ast_node_s **node_ptr, int to_dt)
{
    fds_filter_ast_node_s *node = insert_ast_node(node_ptr);
    if (!node) {
        return NULL;
    }
    node->symbol = "__constructor__";
    node->datatype = to_dt;
    return node;
}

/**
 * Inserts a cast node in place of the provided node
 */
static fds_filter_ast_node_s *
insert_cast_node(fds_filter_ast_node_s **node_ptr, int to_dt)
{
    fds_filter_ast_node_s *node = insert_ast_node(node_ptr);
    if (!node) {
        return NULL;
    }
    node->symbol = "__cast__";
    node->datatype = to_dt;
    return node;
}

static inline bool
op_has_symbol(fds_filter_op_s *op, const char *symbol)
{
    return strcmp(op->symbol, symbol) == 0;
}

static bool
try_match(array_s *op_list, fds_filter_ast_node_s *ast, int dt, bool cast_ok, 
    int *out_dt, int *out_sub_dt);

/**
 * Attempts to match all list items to the specified data type
 */
static bool
try_match_list(array_s *op_list, fds_filter_ast_node_s *ast, bool cast_ok, int dt, int *out_sub_dt)
{
    if (!(dt & DT_LIST)) {
        return false;
    }
    int sub_dt = dt & ~DT_LIST;
    *out_sub_dt = DT_NONE;
    fds_filter_ast_node_s *li = ast->child;
    while (li) {
        int unused;
        if (!try_match(op_list, li->item, sub_dt, cast_ok, out_sub_dt, &unused)) {
            return false;
        }
        assert(unused == DT_NONE);
        li = li->next;
    }
    return true;
}

/**
 * Attempts to match AST node to the specified data type
 */
static bool
try_match(array_s *op_list, fds_filter_ast_node_s *ast, int dt, bool cast_ok, 
    int *out_dt, int *out_sub_dt)
{
    // dt is the datatype we want to achieve
    // We can get to it by direct matching, constructing, or casting if ok

    // We are matching an AST list, which can be tricky
    if (ast_node_symbol_is(ast, "__list__")) {
        // Exact match list items
        if (try_match_list(op_list, ast, cast_ok, dt, out_sub_dt)) {
            *out_dt = dt;
            return true;
        }
        // Check if there is a constructor or cast that has the output type of our desired type
        // and we can match all the list items to the input type
        ARRAY_FOR_EACH(op_list, fds_filter_op_s, op) {
            if ((op_has_symbol(op, "__constructor__") || (cast_ok && op_has_symbol(op, "__cast__")))
                    && op->out_dt == dt
                    && op->arg1_dt & DT_LIST) {
                if (try_match_list(op_list, ast, cast_ok, op->arg1_dt, out_sub_dt)) {
                    *out_dt = op->out_dt;
                    return true;
                }
            }
        }
        return false;

    } else {
        // Exact match
        if (ast->datatype == dt) {
            *out_dt = dt;
            *out_sub_dt = DT_NONE;
            return true;
        }
        // Constructor match
        fds_filter_op_s *op;
        if ((op = find_constructor(op_list, ast->datatype, dt))) {
            *out_dt = op->out_dt;
            *out_sub_dt = DT_NONE;
            return true;
        }
        // Cast match if ok
        if (cast_ok && (op = find_cast(op_list, ast->datatype, dt))) {
            *out_dt = op->out_dt;
            *out_sub_dt = DT_NONE;
            return true;
        }
        return false;
    }
}

/** 
 * Converts AST node to specified type by adding a constructor or cast node, 
 * or does nothing if the node is already the correct type
 */
static error_t
typeconv_node(fds_filter_ast_node_s **ast_ptr, array_s *op_list, int to_dt, bool cast_ok)
{

    // Empty list literal can be matched to any type, because there is no way to determine the type without any elements
    if ((*ast_ptr)->datatype == DT_NONE && ast_node_symbol_is(*ast_ptr, "__list__")) {
        (*ast_ptr)->datatype = to_dt;
        return NO_ERROR;
    }

    if ((*ast_ptr)->datatype == to_dt) {
        return NO_ERROR;
    }

    fds_filter_op_s *op = find_constructor(op_list, (*ast_ptr)->datatype, to_dt);
    assert(op || cast_ok);
    if (op) {
        fds_filter_ast_node_s *cnode = insert_constructor_node(ast_ptr, to_dt);
        if (!cnode) {
            return MEMORY_ERROR;
        }
        propagate_flags(cnode);
    }
    if (cast_ok) {
        fds_filter_op_s *op = find_cast(op_list, (*ast_ptr)->datatype, to_dt);
        assert(op);
        fds_filter_ast_node_s *cnode = insert_cast_node(ast_ptr, to_dt);
        if (!cnode) {
            return MEMORY_ERROR;
        }
        propagate_flags(cnode);
    }
    return NO_ERROR;
}

/**
 * Loops through AST list items and converts each item to the specified type 
 */
static error_t
typeconv_list_items(fds_filter_ast_node_s *ast, array_s *op_list, int to_dt, bool cast_ok)
{
    assert(ast_node_symbol_is(ast, "__list__"));
    fds_filter_ast_node_s *li = ast->item; 
    while (li) {
        error_t err = typeconv_node(&li->item, op_list, to_dt, cast_ok);
        if (err != NO_ERROR) {
            return err;
        }
        li = li->next;
    }
    ast->datatype = DT_LIST | to_dt;
    return NO_ERROR;
}

static bool
match_unary_node(fds_filter_ast_node_s *ast, array_s *op_list, int dt, int child_dt, bool cast_ok, error_t *out_error)
{
    int dt1, sub_dt1;
    if (!try_match(op_list, ast->child, child_dt, cast_ok, &dt1, &sub_dt1)) {
        return false;
    }

    ast->datatype = dt;

    if (sub_dt1 != DT_NONE) {
        error_t err = typeconv_list_items(ast->child, op_list, sub_dt1, cast_ok);
        if (err != NO_ERROR) {
            *out_error = err;
            return true;
        }
    }

    if (dt1 != DT_NONE) {
        error_t err = typeconv_node(&ast->child, op_list, dt1, cast_ok);
        if (err != NO_ERROR) {
            *out_error = err;
            return true;
        }
    }

    *out_error = NO_ERROR;
    return true;
}

static bool
match_binary_node(fds_filter_ast_node_s *ast, array_s *op_list, int dt, int to_dt1, int to_dt2, bool cast_ok, error_t *out_error)
{
    int dt1, sub_dt1;
    if (!try_match(op_list, ast->left, to_dt1, cast_ok, &dt1, &sub_dt1)) {
        return false;
    }

    int dt2, sub_dt2;
    if (!try_match(op_list, ast->right, to_dt2, cast_ok, &dt2, &sub_dt2)) {
        return false;
    }

    ast->datatype = dt;
    
    if (sub_dt1 != DT_NONE) {
        error_t err = typeconv_list_items(ast->left, op_list, sub_dt1, cast_ok);
        if (err != NO_ERROR) {
            *out_error = err;
            return true;
        }
    }
    if (sub_dt2 != DT_NONE) {
        error_t err = typeconv_list_items(ast->right, op_list, sub_dt2, cast_ok);
        if (err != NO_ERROR) {
            *out_error = err;
            return true;
        }
    }
    if (dt1 != DT_NONE) {
        error_t err = typeconv_node(&ast->left, op_list, dt1, cast_ok);
        if (err != NO_ERROR) {
            *out_error = err;
            return true;
        }
    }
    if (dt2 != DT_NONE) {
        error_t err = typeconv_node(&ast->right, op_list, dt2, cast_ok);
        if (err != NO_ERROR) {
            *out_error = err;
            return true;
        }
    }

    *out_error = NO_ERROR;
    return true;
}

static error_t
match_unary_op(fds_filter_ast_node_s *ast, array_s *op_list)
{

    // First iteration through the operations list - exact match or constructable
    ARRAY_FOR_EACH(op_list, fds_filter_op_s, op) {
        if (op_has_symbol(op, ast->symbol)) {
            error_t err;
            if (match_unary_node(ast, op_list, op->out_dt, op->arg1_dt, false, &err)) {
                return err;
            }
        }
    }

    // Second iteration - also try casts
    ARRAY_FOR_EACH(op_list, fds_filter_op_s, op) {
        if (op_has_symbol(op, ast->symbol)) {
            error_t err;
            if (match_unary_node(ast, op_list, op->out_dt, op->arg1_dt, true, &err)) {
                return err;
            }
        }
    }

    // If none of the above succeeded
    return invalid_op_err(ast);
}

static error_t
match_binary_op(fds_filter_ast_node_s *ast, array_s *op_list)
{
    ARRAY_FOR_EACH(op_list, fds_filter_op_s, op) {
        if (op_has_symbol(op, ast->symbol)) {
            error_t err;
            if (match_binary_node(ast, op_list, op->out_dt, op->arg1_dt, op->arg2_dt, false, &err)) {
                return err;
            }
        }
    }

    ARRAY_FOR_EACH(op_list, fds_filter_op_s, op) {
        if (op_has_symbol(op, ast->symbol)) {
            error_t err;
            if (match_binary_node(ast, op_list, op->out_dt, op->arg1_dt, op->arg2_dt, true, &err)) {
                return err;
            }
        }
    }

    return invalid_op_err(ast);
}




error_t
resolve_types(fds_filter_ast_node_s *ast, fds_filter_opts_t *opts)
{
    if (!ast) {
        return NO_ERROR;
    }

    // Resolve children first
    error_t err;
    err = resolve_types(ast->left, opts);
    if (err != NO_ERROR) {
        return err;
    }
    err = resolve_types(ast->right, opts);
    if (err != NO_ERROR) {
        return err;
    }

    propagate_flags(ast);
    
    if (ast_node_symbol_is(ast, "__listitem__")) {
        if (!(ast->flags & AST_FLAG_CONST_SUBTREE)) {
            return SEMANTIC_ERROR(ast, "list items must be const");
        }
        ast->datatype = ast->item->datatype;
        return NO_ERROR;
    }

    if (ast_node_symbol_is(ast, "__list__")) {
        return NO_ERROR;
    }

    if (ast_node_symbol_is(ast, "__literal__")) {
        ast->flags |= AST_FLAG_CONST_SUBTREE;
        return NO_ERROR;
    }

    if (ast_node_symbol_is(ast, "and") || ast_node_symbol_is(ast, "or")) {
        error_t err;
        if (match_binary_node(ast, &opts->op_list, DT_BOOL, DT_BOOL, DT_BOOL, false, &err)
                || match_binary_node(ast, &opts->op_list, DT_BOOL, DT_BOOL, DT_BOOL, true, &err)) {
            if (err != NO_ERROR) {
                return err;
            }
            ast->flags &= ~AST_FLAG_MULTIPLE_EVAL_SUBTREE;
            return NO_ERROR;
        } else {
            return invalid_op_err(ast);
        }
    }

    if (ast_node_symbol_is(ast, "not") || ast_node_symbol_is(ast, "__root__")) {        
        error_t err;
        if (match_unary_node(ast, &opts->op_list, DT_BOOL, DT_BOOL, false, &err)
                || match_unary_node(ast, &opts->op_list, DT_BOOL, DT_BOOL, true, &err)) {
            if (err != NO_ERROR) {
                return err;
            }
            ast->flags &= ~AST_FLAG_MULTIPLE_EVAL_SUBTREE;
            return NO_ERROR;
        } else {
            return invalid_op_err(ast);
        }
    }

    if (ast_node_symbol_is(ast, "exists")) {
        if (!ast_node_symbol_is(ast->child, "__name__")) {
            return SEMANTIC_ERROR(ast, "expected field name for exists");
        } else if (ast->child->flags & AST_FLAG_CONST_SUBTREE) {
            return SEMANTIC_ERROR(ast, "expected non-const field name for exists");
        }
        ast->datatype = DT_BOOL;
        return NO_ERROR;
    }

    if (ast_node_symbol_is(ast, "__name__")) {
        int flags = 0;
        int rv = opts->lookup_cb(ast->name, &ast->id, &ast->datatype, &flags);
        if (rv != FDS_OK) {
            return SEMANTIC_ERROR(ast, "invalid name");
        }
        if (flags & FDS_FILTER_FLAG_CONST) {
            ast->flags |= AST_FLAG_CONST_SUBTREE;
        } else {
            ast->flags |= AST_FLAG_MULTIPLE_EVAL_SUBTREE;
        }
        return NO_ERROR;
    }

    if (ast->left && !ast->right) {
        return match_unary_op(ast, &opts->op_list);
    } else if (ast->left && ast->right) {
        return match_binary_op(ast, &opts->op_list);
    } else {
        return NO_ERROR;
    }
}
