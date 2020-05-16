#include "eval_common.h"
#include "ast_common.h"
#include "error.h"
#include "opts.h"
#include "values.h"
#include "operations.h"

/**
 * Deletes all references to a value from a eval tree
 * 
 * \param value the value
 * \param tree  the eval tree
 */
static void
delete_value_from_et(fds_filter_value_u *value, eval_node_s *tree)
{
    if (!tree) {
        return;
    }
    delete_value_from_et(value, tree->left);
    delete_value_from_et(value, tree->right);
    
    // if the value matches by value zero it out
    if (memcmp(&tree->value, value, sizeof(fds_filter_value_u)) == 0) {
        tree->value = (fds_filter_value_u){};
    }
}

/**
 * Generate and evaluate an AST once and return the resulting value of its root
 * 
 * \param[in] ast       the AST
 * \param[in] opts      the filter opts
 * \param[out] out_val  the resulting value
 * \return error type
 */ 
static error_t
ast_to_literal(fds_filter_ast_node_s *ast, fds_filter_opts_t *opts, fds_filter_value_u *out_val)
{
    eval_node_s *root;
    error_t err = generate_eval_tree(ast, opts, &root);
    if (err != NO_ERROR) {
        return err;
    }

    eval_runtime_s runtime = {};
    evaluate_eval_tree(root, &runtime); // Evaluation cannot fail

    *out_val = root->value; // Copy the value 
    delete_value_from_et(out_val, root); // Delete it so it doesn't get destructed

    destroy_eval_tree(root);

    return NO_ERROR;
}

/**
 * Destroy a value by calling its destructor function if it has any
 * 
 * \param op_list   the list of operations
 * \param datatype  the data type of the value
 * \param value     pointer to the value to destruct
 */
static void
call_destructor_for_value(fds_filter_op_s *op_list, int datatype, fds_filter_value_u *value)
{
    fds_filter_op_s *destructor = find_destructor(op_list, datatype);
    if (destructor) {
        destructor->destructor_fn(value);
    }
}

/**
 * Properly destruct a partially constructed list that hasn't been finished due to an error
 * 
 * \param op_list    the list of operations
 * \param item_dt    the datatype of the list items
 * \param items      the list items
 * \param num_items  the number of properly constructed items
 */
static void
call_destructor_for_list_items(fds_filter_op_s *op_list, int item_dt, fds_filter_value_u *items, int num_items)
{
    fds_filter_op_s *destructor = find_destructor(op_list, item_dt);
    if (destructor) {
        for (int i = 0; i < num_items; i++) {
            destructor->destructor_fn(&items[i]);
        }
    }
    free(items);
}

/**
 * Convert an AST linked list to a real list
 * 
 * \param[in]  ast       the ast node of the linked list
 * \param[in]  opts      the filter options
 * \param[in]  env       the eval env
 * \param[out] out_list  the resulting list
 * \return error type
 */  
static error_t
list_to_literal(fds_filter_ast_node_s *ast, fds_filter_opts_t *opts, fds_filter_list_t *out_list)
{
    // The ast node must be a list
    assert(ast_node_symbol_is(ast, "__list__"));

    out_list->len = 0;
    out_list->items = NULL;

    // Count the number of items
    for (fds_filter_ast_node_s *li = ast->child; li != NULL; li = li->next) {
        out_list->len++;
    }

    // In case the list is empty there is nothing to do...
    if (out_list == 0) {
        return NO_ERROR;
    }

    // Allocate space for the items
    out_list->items = calloc(out_list->len, sizeof(value_u));
    if (!out_list->items) {
        return MEMORY_ERROR;
    }

    // Evaluate each list item node and populate the list
    int idx = 0;
    for (fds_filter_ast_node_s *li = ast->child; li != NULL; li = li->next) {
        assert(idx < out_list->len);

        error_t err = ast_to_literal(li->item, opts, &out_list->items[idx]);
        if (err != NO_ERROR) {
            call_destructor_for_list_items(opts->op_list, ast->child->datatype, out_list->items, idx);
            return err;
        }
        idx++;
    }
    return NO_ERROR;
}

static error_t
generate_children(fds_filter_ast_node_s *ast, fds_filter_opts_t *opts, eval_node_s **out_left, eval_node_s **out_right);

/**
 * Process `__root__` ast node
 */
static error_t
process_root_node(fds_filter_ast_node_s *ast, fds_filter_opts_t *opts, eval_node_s **out_eval_node)
{
    eval_node_s *child;
    error_t err = generate_children(ast, opts, &child, NULL);
    if (err != NO_ERROR) {
        return err;
    }

    // Just propagate the node if the ANY node is not needed
    if (!(ast->child->flags & FDS_FAF_MULTIPLE_EVAL_SUBTREE)) {
        *out_eval_node = child;
        return NO_ERROR;
    }

    // Can be evaluated multiple times so the ANY node is needed
    eval_node_s *en = create_eval_node();
    if (!en) {
        destroy_eval_tree(child);
        return MEMORY_ERROR;
    }
    en->opcode = EVAL_OP_ANY;   
    IF_DEBUG(en->datatype = FDS_FDT_BOOL;)
    en->child = child;
    child->parent = en;
    *out_eval_node = en;
    return NO_ERROR;
}

/**
 * Process `__constructor__` ast node
 */
static error_t
process_constructor_node(fds_filter_ast_node_s *ast, fds_filter_opts_t *opts, eval_node_s **out_eval_node)
{
    // Must be const, should be assured by semantic analysis
    assert(ast->flags & FDS_FAF_CONST_SUBTREE);

    fds_filter_value_u orig_val, constructed_val;

    // Evaluate the subtree under the constructor node and get its value
    error_t err = ast_to_literal(ast->child, opts, &orig_val);
    if (err != NO_ERROR) {
        return err;
    }

    // Find the constructor and try to call it
    fds_filter_op_s *constructor = find_constructor(opts->op_list, ast->child->datatype, ast->datatype);
    int rc = constructor->constructor_fn(&orig_val, &constructed_val);
    // ??? Should the original value be destructed every time or only if the constructor failed?
    call_destructor_for_value(opts->op_list, ast->child->datatype, &orig_val);
    if (rc != FDS_OK) {
        return SEMANTIC_ERROR(ast, "value could not be constructed");
    }

    // Create node for the constructed value
    eval_node_s *en = create_eval_node();
    if (!en) {
        call_destructor_for_value(opts->op_list, ast->datatype, &constructed_val);
        return MEMORY_ERROR;
    }
    en->opcode = EVAL_OP_NONE;
    IF_DEBUG(en->datatype = ast->datatype;)
    en->value = constructed_val;
    fds_filter_op_s *destructor = find_destructor(opts->op_list, ast->datatype);
    if (destructor) {
        en->destructor_fn = destructor->destructor_fn;
    }
    *out_eval_node = en;
    return NO_ERROR;
}

/**
 * Process `exists` ast node
 */
static error_t
process_exists_node(fds_filter_ast_node_s *ast, fds_filter_opts_t *opts, eval_node_s **out_eval_node)
{
    // Must have a __name__ node as its only child
    assert(ast_node_symbol_is(ast->child, "__name__"));

    // Create exists node
    eval_node_s *en = create_eval_node();
    if (!en) {
        return MEMORY_ERROR;
    }
    en->opcode = EVAL_OP_EXISTS;
    en->lookup_id = ast->child->id;
    IF_DEBUG(en->datatype = ast->datatype;)
    *out_eval_node = en;
    return NO_ERROR;
}

/**
 * Process `__name__` node
 */
static error_t
process_name_node(fds_filter_ast_node_s *ast, fds_filter_opts_t *opts, eval_node_s **out_eval_node)
{
    eval_node_s *en = create_eval_node();
    if (!en) {
        return MEMORY_ERROR;
    }
    // Is the name a constant or a variable?
    if (ast->flags & FDS_FAF_CONST_SUBTREE) {
        en->opcode = EVAL_OP_NONE;
        opts->const_cb(opts->user_ctx, ast->id, &en->value);
        // TODO: destructor?
    } else {
        en->opcode = EVAL_OP_DATA_CALL;
        en->lookup_id = ast->id;
    }
    IF_DEBUG(en->datatype = ast->datatype;)
    *out_eval_node = en;
    return NO_ERROR;
}

/**
 * Process `__literal__` node
 */
static error_t
process_literal_node(fds_filter_ast_node_s *ast, fds_filter_opts_t *opts, eval_node_s **out_eval_node)
{
    eval_node_s *en = create_eval_node();
    if (!en) {
        return MEMORY_ERROR;
    }
    en->opcode = EVAL_OP_NONE;
    en->value = ast->value;
    fds_filter_op_s *destructor = find_destructor(opts->op_list, ast->datatype);
    if (destructor) {
        en->destructor_fn = destructor->destructor_fn;
    }
    // The destruction of the value is now handled by the eval tree instead of AST
    ast->flags &= ~FDS_FAF_DESTROY_VAL;
    IF_DEBUG(en->datatype = ast->datatype;)
    *out_eval_node = en;
    return NO_ERROR;
}

/**
 * Process `__list__` node
 */
static error_t
process_list_node(fds_filter_ast_node_s *ast, fds_filter_opts_t *opts, eval_node_s **out_eval_node)
{   
    eval_node_s *en = create_eval_node();
    if (!en) {
        return MEMORY_ERROR;
    }

    // Transform the abstract list to real list
    error_t err = list_to_literal(ast, opts, &en->value.list);
    if (err != NO_ERROR) {
        destroy_eval_node(en);
        return err;
    }
    en->opcode = EVAL_OP_NONE;
    fds_filter_op_s *destructor = find_destructor(opts->op_list, ast->datatype);
    if (destructor) {
        en->destructor_fn = destructor->destructor_fn;
    }
    IF_DEBUG(en->datatype = ast->datatype;)
    *out_eval_node = en;
    return NO_ERROR;
}

/**
 * Process logical operation node
 */
static error_t
process_logical_node(fds_filter_ast_node_s *ast, fds_filter_opts_t *opts, eval_node_s **out_eval_node)
{
    eval_node_s *en = create_eval_node();
    if (!en) {
        return MEMORY_ERROR;
    }

    if (ast_node_symbol_is(ast, "not")) {
        en->opcode = EVAL_OP_NOT;
        error_t err = generate_children(ast, opts, &en->child, NULL);
        if (err != NO_ERROR) {
            destroy_eval_node(en);
            return err;
        }
        en->child->parent = en;
        IF_DEBUG(en->datatype = FDS_FDT_BOOL;)
    } else if (ast_node_symbol_is(ast, "and")) {
        en->opcode = EVAL_OP_AND;
        error_t err = generate_children(ast, opts, &en->left, &en->right);
        if (err != NO_ERROR) {
            destroy_eval_node(en);
            return err;
        }
        en->left->parent = en;
        en->right->parent = en;
        IF_DEBUG(en->datatype = FDS_FDT_BOOL;)
    } else if (ast_node_symbol_is(ast, "or")) {
        en->opcode = EVAL_OP_OR;
        error_t err = generate_children(ast, opts, &en->left, &en->right);
        if (err != NO_ERROR) {
            destroy_eval_node(en);
            return err;
        }
        en->left->parent = en;
        en->right->parent = en;
        IF_DEBUG(en->datatype = FDS_FDT_BOOL;)
    }

    *out_eval_node = en;
    return NO_ERROR;
}

/**
 * Process function call node
 */
static error_t
process_fcall_node(fds_filter_ast_node_s *ast, fds_filter_opts_t *opts, eval_node_s **out_eval_node)
{
    eval_node_s *en = create_eval_node();
    if (!en) {
        return MEMORY_ERROR;
    }

    if (ast_node_symbol_is(ast, "__cast__")) {
        en->opcode = EVAL_OP_CAST_CALL;
        error_t err = generate_children(ast, opts, &en->child, NULL);
        if (err != NO_ERROR) {
            destroy_eval_node(en);
            return err;
        }
        en->child->parent = en;
        fds_filter_op_s *op = find_op(opts->op_list, "__cast__", ast->datatype, ast->child->datatype, FDS_FDT_NONE); 
        en->cast_fn = op->cast_fn;
        IF_DEBUG(
        en->operation = op;
        en->datatype = ast->datatype;
        )
    } else if (is_unary_ast_node(ast)) {
        en->opcode = EVAL_OP_UNARY_CALL;
        error_t err = generate_children(ast, opts, &en->child, NULL);
        if (err != NO_ERROR) {
            destroy_eval_node(en);
            return err;
        }
        en->child->parent = en;
        fds_filter_op_s *op = find_op(opts->op_list, ast->symbol, ast->datatype, ast->child->datatype, FDS_FDT_NONE);
        en->unary_fn = op->unary_fn;
        IF_DEBUG(
        en->operation = op;
        en->datatype = ast->datatype;
        )
    } else if (is_binary_ast_node(ast)) {
        en->opcode = EVAL_OP_BINARY_CALL;
        error_t err = generate_children(ast, opts, &en->left, &en->right);
        if (err != NO_ERROR) {
            destroy_eval_node(en);
            return err;
        }
        en->left->parent = en;
        en->right->parent = en;
        fds_filter_op_s *op = find_op(opts->op_list, ast->symbol, ast->datatype, ast->left->datatype, ast->right->datatype);
        en->binary_fn = op->binary_fn;
        IF_DEBUG(
        en->operation = op;
        en->datatype = ast->datatype;
        )
    }
    
    *out_eval_node = en;
    return NO_ERROR;
}

error_t
generate_eval_tree(fds_filter_ast_node_s *ast, fds_filter_opts_t *opts, eval_node_s **out_eval_node)
{
#if 0
    // Optimize away constant subtrees into a literal
    if (ast->flags & FDS_FAF_CONST_SUBTREE 
            && (ast->parent == NULL || !(ast->parent->flags & FDS_FAF_CONST_SUBTREE))) {
        eval_node_s *en = create_eval_node();
        if (!en) {
            return MEMORY_ERROR;
        }
        en->opcode = EVAL_OP_NONE;
        
        error_t err = ast_to_literal(ast, opts, &en->value);
        *out_eval_node = en;

        // ast->flags &= ~FDS_FAF_DESTROY_VAL;

        return NO_ERROR;
    }
#endif

    if (ast_node_symbol_is(ast, "__root__")) {
        return process_root_node(ast, opts, out_eval_node);
    } else if (ast_node_symbol_is(ast, "exists")) {
        return process_exists_node(ast, opts, out_eval_node);
    } else if (ast_node_symbol_is(ast, "__literal__")) {
        return process_literal_node(ast, opts, out_eval_node);
    } else if (ast_node_symbol_is(ast, "__name__")) {
        return process_name_node(ast, opts, out_eval_node);
    } else if (ast_node_symbol_is(ast, "__list__")) {
        return process_list_node(ast, opts, out_eval_node);
    } else if (ast_node_symbol_is(ast, "__constructor__")) {
        return process_constructor_node(ast, opts, out_eval_node);
    } else if (ast_node_symbol_is(ast, "and") || ast_node_symbol_is(ast, "or") || ast_node_symbol_is(ast, "not")) {
        return process_logical_node(ast, opts, out_eval_node);
    } else {
        return process_fcall_node(ast, opts, out_eval_node);
    }
}

static error_t
generate_children(fds_filter_ast_node_s *ast, fds_filter_opts_t *opts, eval_node_s **out_left, eval_node_s **out_right)
{
    if (!out_left) {
        assert(!out_right);
        return NO_ERROR;
    }

    error_t err = generate_eval_tree(ast->left, opts, out_left);
    if (err != NO_ERROR) {
        return err;
    }

    if (!out_right) {
        return NO_ERROR;
    }

    err = generate_eval_tree(ast->right, opts, out_right);
    if (err != NO_ERROR) {
        destroy_eval_tree(*out_left);
        return err;
    }

    return NO_ERROR;
}

