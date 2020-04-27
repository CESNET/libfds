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
    if ((ast->left && ast->left->flags & FDS_FAF_MULTIPLE_EVAL_SUBTREE) 
            || (ast->right && ast->right->flags & FDS_FAF_MULTIPLE_EVAL_SUBTREE)) {
        ast->flags |= FDS_FAF_MULTIPLE_EVAL_SUBTREE;
    }
    // If both children are constant, parent is also constant
    if (ast->left                                                         // Has atleast one child
            && ast->left->flags & FDS_FAF_CONST_SUBTREE                   // The child is constant subtree 
            && (!ast->right || ast->right->flags & FDS_FAF_CONST_SUBTREE) // Right child is also constant subtree if there is one
        ) { 
        ast->flags |= FDS_FAF_CONST_SUBTREE;
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
    node->parent = (*node_ptr)->parent;
    (*node_ptr)->parent = node;
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
try_match(fds_filter_op_s *op_list, fds_filter_ast_node_s *ast, int dt, bool cast_ok, 
    int *out_dt, int *out_sub_dt);

/**
 * Attempts to match all list items to the specified data type
 */
static bool
try_match_list_items(fds_filter_op_s *op_list, fds_filter_ast_node_s *ast, bool cast_ok, int dt, int *out_sub_dt)
{
    if (!(dt & FDS_FDT_LIST)) {
        return false;
    }
    int sub_dt = dt & ~FDS_FDT_LIST;
    *out_sub_dt = FDS_FDT_NONE;
    fds_filter_ast_node_s *li = ast->child;
    while (li) {
        int unused;
        if (!try_match(op_list, li->item, sub_dt, cast_ok, out_sub_dt, &unused)) {
            return false;
        }
        assert(unused == FDS_FDT_NONE);
        li = li->next;
    }
    return true;
}

/**
 * Attempts to match AST node to the specified data type
 */
static bool
try_match(fds_filter_op_s *op_list, fds_filter_ast_node_s *ast, int dt, bool cast_ok, 
    int *out_dt, int *out_sub_dt)
{
    // dt is the datatype we want to achieve
    // We can get to it by direct matching, constructing, or casting if ok

    // We are matching an AST list, which can be tricky
    if (ast_node_symbol_is(ast, "__list__")) {
        // Exact match list items
        if (try_match_list_items(op_list, ast, cast_ok, dt, out_sub_dt)) {
            *out_dt = dt;
            return true;
        }
        // Check if there is a constructor or cast that has the output type of our desired type
        // and we can match all the list items to the input type
        for (fds_filter_op_s *op = op_list; op->symbol != NULL; op++) {
            if ((op_has_symbol(op, "__constructor__") || (cast_ok && op_has_symbol(op, "__cast__")))
                    && op->out_dt == dt
                    && op->arg1_dt & FDS_FDT_LIST) {
                if (try_match_list_items(op_list, ast, cast_ok, op->arg1_dt, out_sub_dt)) {
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
            *out_sub_dt = FDS_FDT_NONE;
            return true;
        }
        //printf("XXXX:\n");
        //print_ast(stdout, ast);
        //printf("XXXX: no exact match from %s to %s\n", data_type_to_str(ast->datatype), data_type_to_str(dt));
        // Constructor match
        fds_filter_op_s *op;
        if ((op = find_constructor(op_list, ast->datatype, dt))) {
            *out_dt = op->out_dt;
            *out_sub_dt = FDS_FDT_NONE;
            return true;
        }
        //printf("XXXX: no constructor from %s to %s\n", data_type_to_str(ast->datatype), data_type_to_str(dt));
        // Cast match if ok
        if (cast_ok && (op = find_cast(op_list, ast->datatype, dt))) {
            *out_dt = op->out_dt;
            *out_sub_dt = FDS_FDT_NONE;
            return true;
        }
        //if (cast_ok) {
        //    printf("XXXX: no cast from %s to %s\n", data_type_to_str(ast->datatype), data_type_to_str(dt));
        //}
        return false;
    }
}

/** 
 * Converts AST node to specified type by adding a constructor or cast node, 
 * or does nothing if the node is already the correct type
 */
static error_t
typeconv_node(fds_filter_ast_node_s **ast_ptr, fds_filter_op_s *op_list, int to_dt, bool cast_ok)
{

    // Empty list literal can be matched to any type, because there is no way to determine the type without any elements
    if ((*ast_ptr)->datatype == FDS_FDT_NONE && ast_node_symbol_is(*ast_ptr, "__list__")) {
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
typeconv_list_items(fds_filter_ast_node_s *ast, fds_filter_op_s *op_list, int to_dt, bool cast_ok)
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
    ast->datatype = FDS_FDT_LIST | to_dt;
    return NO_ERROR;
}

static bool
match_unary_node(fds_filter_ast_node_s *ast, fds_filter_op_s *op_list, int dt, int child_dt, bool cast_ok, error_t *out_error)
{
    int dt1, sub_dt1;
    if (!try_match(op_list, ast->child, child_dt, cast_ok, &dt1, &sub_dt1)) {
        return false;
    }

    ast->datatype = dt;

    if (sub_dt1 != FDS_FDT_NONE) {
        error_t err = typeconv_list_items(ast->child, op_list, sub_dt1, cast_ok);
        if (err != NO_ERROR) {
            *out_error = err;
            return true;
        }
    }

    if (dt1 != FDS_FDT_NONE) {
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
match_binary_node(fds_filter_ast_node_s *ast, fds_filter_op_s *op_list, int dt, int to_dt1, int to_dt2, bool cast_ok, error_t *out_error)
{
    //printf("XXXX: match_binary_node :: left: %s->%s :: right: %s->%s :: dt: %s\n",
    //    data_type_to_str(ast->left->datatype), data_type_to_str(to_dt1),
    //    data_type_to_str(ast->right->datatype), data_type_to_str(to_dt2),
    //    data_type_to_str(dt));
    int dt1, sub_dt1;
    if (!try_match(op_list, ast->left, to_dt1, cast_ok, &dt1, &sub_dt1)) {
        return false;
    }

    int dt2, sub_dt2;
    if (!try_match(op_list, ast->right, to_dt2, cast_ok, &dt2, &sub_dt2)) {
        return false;
    }

    ast->datatype = dt;
    
    if (sub_dt1 != FDS_FDT_NONE) {
        error_t err = typeconv_list_items(ast->left, op_list, sub_dt1, cast_ok);
        if (err != NO_ERROR) {
            *out_error = err;
            return true;
        }
    }
    if (sub_dt2 != FDS_FDT_NONE) {
        error_t err = typeconv_list_items(ast->right, op_list, sub_dt2, cast_ok);
        if (err != NO_ERROR) {
            *out_error = err;
            return true;
        }
    }
    if (dt1 != FDS_FDT_NONE) {
        error_t err = typeconv_node(&ast->left, op_list, dt1, cast_ok);
        if (err != NO_ERROR) {
            *out_error = err;
            return true;
        }
    }
    if (dt2 != FDS_FDT_NONE) {
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
match_unary_op(fds_filter_ast_node_s *ast, fds_filter_op_s *op_list)
{

    // First iteration through the operations list - exact match or constructable
    for (fds_filter_op_s *op = op_list; op->symbol != NULL; op++) {
        if (op_has_symbol(op, ast->symbol)) {
            error_t err;
            if (match_unary_node(ast, op_list, op->out_dt, op->arg1_dt, false, &err)) {
                return err;
            }
        }
    }

    // Second iteration - also try casts
    for (fds_filter_op_s *op = op_list; op->symbol != NULL; op++) {
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
match_binary_op(fds_filter_ast_node_s *ast, fds_filter_op_s *op_list)
{
    //printf("XXXX: matching binary op for ast:\n");
    //print_ast(stdout, ast);

    for (fds_filter_op_s *op = op_list; op->symbol != NULL; op++) {
        if (op_has_symbol(op, ast->symbol)) {
            error_t err;
            if (match_binary_node(ast, op_list, op->out_dt, op->arg1_dt, op->arg2_dt, false, &err)) {
                return err;
            }
        }
    }

    for (fds_filter_op_s *op = op_list; op->symbol != NULL; op++) {
        if (op_has_symbol(op, ast->symbol)) {
            error_t err;
            if (match_binary_node(ast, op_list, op->out_dt, op->arg1_dt, op->arg2_dt, true, &err)) {
                return err;
            }
        }
    }

    return invalid_op_err(ast);
}

const char *
find_first_name(fds_filter_ast_node_s *ast)
{
    if (ast == NULL) {
        return NULL;
    }
    const char *name;
    name = find_first_name(ast->left);
    if (name != NULL) {
        return name;
    }
    name = find_first_name(ast->right);
    if (name != NULL) {
        return name;
    }
    if (ast_node_symbol_is(ast, "__name__")) {
        return ast->name;
    }
    return NULL;
}

const char *
find_other_name(fds_filter_ast_node_s *ast)
{
    if (ast->parent == NULL) {
        return NULL;
    }
    fds_filter_ast_node_s *this_side = ast;
    ast = ast->parent;
    while (ast != NULL) {
        bool is_cmp = 
            ast_node_symbol_is(ast, "==") || 
            ast_node_symbol_is(ast, "!=") || 
            ast_node_symbol_is(ast, "<") || 
            ast_node_symbol_is(ast, ">") || 
            ast_node_symbol_is(ast, ">=") || 
            ast_node_symbol_is(ast, "<=") || 
            ast_node_symbol_is(ast, "contains") || 
            ast_node_symbol_is(ast, "in") || 
            ast_node_symbol_is(ast, "");
        if (is_cmp) {
            break;
        }
        this_side = ast;
        ast = ast->parent;
    }

    if (ast == NULL) {
        return NULL;
    }

    fds_filter_ast_node_s *other_side = (ast->left == this_side) ? ast->right : ast->left;
    if (other_side == NULL) {
        return NULL;
    }
    return find_first_name(other_side);
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
        if (!(ast->flags & FDS_FAF_CONST_SUBTREE)) {
            return SEMANTIC_ERROR(ast, "list items must be const");
        }
        ast->datatype = ast->item->datatype;
        return NO_ERROR;
    }

    if (ast_node_symbol_is(ast, "__list__")) {
        return NO_ERROR;
    }

    if (ast_node_symbol_is(ast, "__literal__")) {
        ast->flags |= FDS_FAF_CONST_SUBTREE;
        return NO_ERROR;
    }

    if (ast_node_symbol_is(ast, "and") || ast_node_symbol_is(ast, "or")) {
        error_t err;
        if (match_binary_node(ast, opts->op_list, FDS_FDT_BOOL, FDS_FDT_BOOL, FDS_FDT_BOOL, false, &err)
                || match_binary_node(ast, opts->op_list, FDS_FDT_BOOL, FDS_FDT_BOOL, FDS_FDT_BOOL, true, &err)) {
            if (err != NO_ERROR) {
                return err;
            }
            ast->flags &= ~FDS_FAF_MULTIPLE_EVAL_SUBTREE;
            return NO_ERROR;
        } else {
            return invalid_op_err(ast);
        }
    }

    if (ast_node_symbol_is(ast, "not") || ast_node_symbol_is(ast, "__root__")) {        
        error_t err;
        if (match_unary_node(ast, opts->op_list, FDS_FDT_BOOL, FDS_FDT_BOOL, false, &err)
                || match_unary_node(ast, opts->op_list, FDS_FDT_BOOL, FDS_FDT_BOOL, true, &err)) {
            if (err != NO_ERROR) {
                return err;
            }
            ast->flags &= ~FDS_FAF_MULTIPLE_EVAL_SUBTREE;
            return NO_ERROR;
        } else {
            return invalid_op_err(ast);
        }
    }

    if (ast_node_symbol_is(ast, "exists")) {
        if (!ast_node_symbol_is(ast->child, "__name__")) {
            return SEMANTIC_ERROR(ast, "expected field name for exists");
        } else if (ast->child->flags & FDS_FAF_CONST_SUBTREE) {
            return SEMANTIC_ERROR(ast, "expected non-const field name for exists");
        }
        ast->datatype = FDS_FDT_BOOL;
        return NO_ERROR;
    }

    if (ast_node_symbol_is(ast, "__name__")) {
        int flags = 0;
        const char *other_name = find_other_name(ast);
        int rc = opts->lookup_cb(opts->user_ctx, ast->name, other_name, &ast->id, &ast->datatype, &flags);
        if (rc != FDS_OK) {
            return SEMANTIC_ERROR(ast, "invalid name");
        }
        if (flags & FDS_FILTER_FLAG_CONST) {
            ast->flags |= FDS_FAF_CONST_SUBTREE;
        } else {
            ast->flags |= FDS_FAF_MULTIPLE_EVAL_SUBTREE;
        }
        return NO_ERROR;
    }

    if (ast->left && !ast->right) {
        return match_unary_op(ast, opts->op_list);
    } else if (ast->left && ast->right) {
        return match_binary_op(ast, opts->op_list);
    } else {
        return NO_ERROR;
    }
}
