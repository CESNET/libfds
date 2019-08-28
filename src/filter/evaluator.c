#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <assert.h>
#include <libfds.h>
#include "ast.h"
#include "filter.h"
#include "debug.h"
#include "evaluator_functions.h"

static int
eval_tree_node_generate(struct fds_filter *filter, struct fds_filter_ast_node *ast_node,
                        struct eval_node **out_eval_node)
{
    struct eval_node *eval_node = calloc(1, sizeof(struct eval_node));
    if (eval_node == NULL) {
        PTRACE("returning FAIL because no memory");
        no_memory_error(filter->error_list);
        return FDS_FILTER_FAIL;
    }
    *out_eval_node = eval_node;
    eval_node->data_type = ast_node->data_type;
    eval_node->is_trie = ast_node->is_trie;
    eval_node->data_subtype = ast_node->data_subtype;
    eval_node->value = ast_node->value;

    switch (ast_node->node_type) {
    case FDS_FANT_ADD: {
        assert(ast_node->left->data_type == ast_node->right->data_type
            && ast_node->data_type == ast_node->left->data_type);
        switch (ast_node->left->data_type) {
        case FDS_FDT_INT:
            eval_node->evaluate = f_add_int;
            break;
        case FDS_FDT_UINT:
            eval_node->evaluate = f_add_uint;
            break;
        case FDS_FDT_FLOAT:
            eval_node->evaluate = f_add_float;
            break;
        case FDS_FDT_STR:
            eval_node->evaluate = f_concat_str;
            eval_node->is_alloc = true;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FANT_SUB: {
        assert(ast_node->left->data_type == ast_node->right->data_type
            && ast_node->data_type == ast_node->left->data_type);
        switch (ast_node->left->data_type) {
        case FDS_FDT_INT:
            eval_node->evaluate = f_sub_int;
            break;
        case FDS_FDT_UINT:
            eval_node->evaluate = f_sub_uint;
            break;
        case FDS_FDT_FLOAT:
            eval_node->evaluate = f_sub_float;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FANT_MUL: {
        assert(ast_node->left->data_type == ast_node->right->data_type
            && ast_node->data_type == ast_node->left->data_type);
        switch (ast_node->left->data_type) {
        case FDS_FDT_INT:
            eval_node->evaluate = f_mul_int;
            break;
        case FDS_FDT_UINT:
            eval_node->evaluate = f_mul_uint;
            break;
        case FDS_FDT_FLOAT:
            eval_node->evaluate = f_mul_float;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FANT_DIV: {
        assert(ast_node->left->data_type == ast_node->right->data_type
            && ast_node->data_type == ast_node->left->data_type);
        switch (ast_node->left->data_type) {
        case FDS_FDT_INT:
            eval_node->evaluate = f_div_int;
            break;
        case FDS_FDT_UINT:
            eval_node->evaluate = f_div_uint;
            break;
        case FDS_FDT_FLOAT:
            eval_node->evaluate = f_div_float;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FANT_MOD: {
        assert(ast_node->left->data_type == ast_node->right->data_type
            && ast_node->data_type == ast_node->left->data_type);
        switch (ast_node->left->data_type) {
        case FDS_FDT_INT:
            eval_node->evaluate = f_mod_int;
            break;
        case FDS_FDT_UINT:
            eval_node->evaluate = f_mod_uint;
            break;
        case FDS_FDT_FLOAT:
            eval_node->evaluate = f_mod_float;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FANT_EQ: {
        assert(ast_node->left->data_type == ast_node->right->data_type
            && ast_node->data_type == FDS_FDT_BOOL);
        switch (ast_node->left->data_type) {
        case FDS_FDT_INT:
            eval_node->evaluate = f_eq_int;
            break;
        case FDS_FDT_UINT:
            eval_node->evaluate = f_eq_uint;
            break;
        case FDS_FDT_FLOAT:
            eval_node->evaluate = f_eq_float;
            break;
        case FDS_FDT_STR:
            eval_node->evaluate = f_eq_str;
            break;
        case FDS_FDT_IP_ADDRESS:
            eval_node->evaluate = f_eq_ip_address;
            break;
        case FDS_FDT_MAC_ADDRESS:
            eval_node->evaluate = f_eq_mac_address;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FANT_NE: {
        assert(ast_node->left->data_type == ast_node->right->data_type
            && ast_node->data_type == FDS_FDT_BOOL);
        switch (ast_node->left->data_type) {
        case FDS_FDT_INT:
            eval_node->evaluate = f_ne_int;
            break;
        case FDS_FDT_UINT:
            eval_node->evaluate = f_ne_uint;
            break;
        case FDS_FDT_FLOAT:
            eval_node->evaluate = f_ne_float;
            break;
        case FDS_FDT_STR:
            eval_node->evaluate = f_ne_str;
            break;
        case FDS_FDT_IP_ADDRESS:
            eval_node->evaluate = f_ne_ip_address;
            break;
        case FDS_FDT_MAC_ADDRESS:
            eval_node->evaluate = f_ne_mac_address;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FANT_LT: {
        assert(ast_node->left->data_type == ast_node->right->data_type
            && ast_node->data_type == FDS_FDT_BOOL);
        switch (ast_node->left->data_type) {
        case FDS_FDT_INT:
            eval_node->evaluate = f_lt_int;
            break;
        case FDS_FDT_UINT:
            eval_node->evaluate = f_lt_uint;
            break;
        case FDS_FDT_FLOAT:
            eval_node->evaluate = f_lt_float;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FANT_GT: {
        assert(ast_node->left->data_type == ast_node->right->data_type
            && ast_node->data_type == FDS_FDT_BOOL);
        switch (ast_node->left->data_type) {
        case FDS_FDT_INT:
            eval_node->evaluate = f_gt_int;
            break;
        case FDS_FDT_UINT:
            eval_node->evaluate = f_gt_uint;
            break;
        case FDS_FDT_FLOAT:
            eval_node->evaluate = f_gt_float;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FANT_LE: {
        assert(ast_node->left->data_type == ast_node->right->data_type
            && ast_node->data_type == FDS_FDT_BOOL);
        switch (ast_node->left->data_type) {
        case FDS_FDT_INT:
            eval_node->evaluate = f_le_int;
            break;
        case FDS_FDT_UINT:
            eval_node->evaluate = f_le_uint;
            break;
        case FDS_FDT_FLOAT:
            eval_node->evaluate = f_le_float;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FANT_GE: {
        assert(ast_node->left->data_type == ast_node->right->data_type
            && ast_node->data_type == FDS_FDT_BOOL);
        switch (ast_node->left->data_type) {
        case FDS_FDT_INT:
            eval_node->evaluate = f_ge_int;
            break;
        case FDS_FDT_UINT:
            eval_node->evaluate = f_ge_uint;
            break;
        case FDS_FDT_FLOAT:
            eval_node->evaluate = f_ge_float;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FANT_AND: {
        assert(ast_node->left->data_type == FDS_FDT_BOOL
               && ast_node->right->data_type == FDS_FDT_BOOL
               && ast_node->data_type == FDS_FDT_BOOL);
        eval_node->evaluate = f_and;
        eval_node->is_defined = true; // Always defined because of ANY node
        eval_node->is_more = false; // Can never have more because of ANY node
    } break;

    case FDS_FANT_OR: {
        assert(ast_node->left->data_type == FDS_FDT_BOOL
               && ast_node->right->data_type == FDS_FDT_BOOL
               && ast_node->data_type == FDS_FDT_BOOL);
        eval_node->evaluate = f_or;
        eval_node->is_defined = true; // Always defined because of ANY node
        eval_node->is_more = false; // Can never have more because of ANY node
    } break;

    case FDS_FANT_NOT: {
        assert(ast_node->left->data_type == FDS_FDT_BOOL
               && ast_node->right == NULL
               && ast_node->data_type == FDS_FDT_BOOL);
        eval_node->evaluate = f_not;
        eval_node->is_defined = true; // Always defined because of ANY node
        eval_node->is_more = false; // Can never have more because of ANY node
    } break;

    case FDS_FANT_CONST: {
        assert(ast_node->left == NULL && ast_node->right == NULL);
        eval_node->evaluate = f_const;
        eval_node->is_defined = true;
    } break;

    case FDS_FANT_IDENTIFIER: {
        assert(ast_node->left == NULL && ast_node->right == NULL);
        eval_node->identifier_id = ast_node->identifier_id;
        if (ast_node->identifier_type == FDS_FIT_CONST) {
            eval_node->is_defined = true;
            eval_node->is_more = false;
            eval_node->value = ast_node->value;
            eval_node->evaluate = f_const;
        } else {
            eval_node->evaluate = f_identifier;
        }
    } break;

    case FDS_FANT_CAST: {
        assert(ast_node->right == NULL);
        if (ast_node->left->data_type == FDS_FDT_INT && ast_node->data_type == FDS_FDT_FLOAT) {
            eval_node->evaluate = f_cast_int_to_float;
        } else if (ast_node->left->data_type == FDS_FDT_INT && ast_node->data_type == FDS_FDT_UINT) {
            eval_node->evaluate = f_cast_int_to_uint;
        } else if (ast_node->left->data_type == FDS_FDT_UINT && ast_node->data_type == FDS_FDT_INT) {
            eval_node->evaluate = f_cast_uint_to_int;
        } else if (ast_node->left->data_type == FDS_FDT_UINT && ast_node->data_type == FDS_FDT_FLOAT) {
            eval_node->evaluate = f_cast_uint_to_float;
        } else if (ast_node->left->data_type == FDS_FDT_UINT && ast_node->data_type == FDS_FDT_BOOL) {
            eval_node->evaluate = f_cast_uint_to_bool;
        } else if (ast_node->left->data_type == FDS_FDT_INT && ast_node->data_type == FDS_FDT_BOOL) {
            eval_node->evaluate = f_cast_int_to_bool;
        } else if (ast_node->left->data_type == FDS_FDT_FLOAT && ast_node->data_type == FDS_FDT_BOOL) {
            eval_node->evaluate = f_cast_float_to_bool;
        } else if (ast_node->left->data_type == FDS_FDT_STR && ast_node->data_type == FDS_FDT_BOOL) {
            eval_node->evaluate = f_cast_str_to_bool;
        } else if (ast_node->left->data_type == FDS_FDT_IP_ADDRESS && ast_node->data_type == FDS_FDT_BOOL) {
            eval_node->evaluate = f_exists;
        } else if (ast_node->left->data_type == FDS_FDT_MAC_ADDRESS && ast_node->data_type == FDS_FDT_BOOL) {
            eval_node->evaluate = f_exists;
        } else if (ast_node->left->data_type == FDS_FDT_LIST && ast_node->data_type == FDS_FDT_LIST) {
            if (ast_node->left->data_subtype == FDS_FDT_INT && ast_node->data_subtype == FDS_FDT_UINT) {
                eval_node->evaluate = f_cast_list_int_to_uint;
            } else if (ast_node->left->data_subtype == FDS_FDT_INT && ast_node->data_subtype == FDS_FDT_FLOAT) {
                eval_node->evaluate = f_cast_list_int_to_float;
            } else if (ast_node->left->data_subtype == FDS_FDT_UINT && ast_node->data_subtype == FDS_FDT_FLOAT) {
                eval_node->evaluate = f_cast_list_uint_to_float;
            } else {
                assert(0 && "Unhandled list cast");
            }
        } else {
            assert(0 && "Unhandled cast");
        }
    } break;

    case FDS_FANT_UMINUS: {
        assert(ast_node->right == NULL && ast_node->data_type == ast_node->left->data_type);
        switch (ast_node->left->data_type) {
        case FDS_FDT_INT:
            eval_node->evaluate = f_minus_int;
            break;
        case FDS_FDT_FLOAT:
            eval_node->evaluate = f_minus_float;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FANT_ANY: {
        assert(ast_node->left->data_type == FDS_FDT_BOOL
               && ast_node->left->data_type == FDS_FDT_BOOL
               && ast_node->right == NULL);
        eval_node->evaluate = f_any;
        eval_node->is_defined = true;
    } break;

    case FDS_FANT_ROOT: {
        // TODO: Solve this a better way that doesn't unnecessarily allocate only to free it right away.
        free(eval_node);
        *out_eval_node = NULL;
    } break;

    case FDS_FANT_FLAGCMP: {
        assert(is_integer_number_type(ast_node->left->data_type)
            && is_integer_number_type(ast_node->right->data_type));
        eval_node->evaluate = f_flagcmp;
    } break;

    case FDS_FANT_BITOR: {
        assert(is_integer_number_type(ast_node->left->data_type)
            && is_integer_number_type(ast_node->right->data_type));
        eval_node->evaluate = f_bitor;
    } break;

    case FDS_FANT_BITAND: {
        assert(is_integer_number_type(ast_node->left->data_type)
            && is_integer_number_type(ast_node->right->data_type));
        eval_node->evaluate = f_bitand;
    } break;

    case FDS_FANT_BITXOR: {
        assert(is_integer_number_type(ast_node->left->data_type)
            && is_integer_number_type(ast_node->right->data_type));
        eval_node->evaluate = f_bitxor;
    } break;

    case FDS_FANT_BITNOT: {
        assert(is_integer_number_type(ast_node->left->data_type)
            && ast_node->right == NULL);
        eval_node->evaluate = f_bitnot;
    } break;

    case FDS_FANT_IN: {
        assert((ast_node->left->data_type == ast_node->right->data_subtype
                || ast_node->right->data_subtype == FDS_FDT_NONE)
            && ast_node->right->data_type == FDS_FDT_LIST);
        switch (ast_node->left->data_type) {
        case FDS_FDT_UINT:
            eval_node->evaluate = f_in_uint;
            break;
        case FDS_FDT_INT:
            eval_node->evaluate = f_in_int;
            break;
        case FDS_FDT_FLOAT:
            eval_node->evaluate = f_in_float;
            break;
        case FDS_FDT_STR:
            eval_node->evaluate = f_in_str;
            break;
        case FDS_FDT_IP_ADDRESS:
            eval_node->evaluate = ast_node->right->is_trie ? f_ip_address_in_trie : f_in_ip_address;
            break;
        case FDS_FDT_MAC_ADDRESS:
            eval_node->evaluate = f_in_mac_address;
            break;
        case FDS_FDT_NONE: // List with no values
            eval_node->evaluate = f_const;
            eval_node->value.uint_ = 0;
            eval_node->is_defined = true;
            break;
        default: assert(0 && "Unhandled type for IN");
        }
    } break;

    case FDS_FANT_CONTAINS: {
        assert(ast_node->left->data_type == FDS_FDT_STR && ast_node->right->data_type == FDS_FDT_STR);
        eval_node->evaluate = f_contains_str;
    } break;


    default: assert(0 && "Unhandled ast node operation");
    }

    return FDS_FILTER_OK;
}

struct eval_node *
generate_eval_tree(struct fds_filter *filter, struct fds_filter_ast_node *ast_node)
{
    if (ast_node == NULL) {
        return NULL;
    }

    struct eval_node *left = NULL;
    if (ast_node->left != NULL) {
        left = generate_eval_tree(filter, ast_node->left);
        if (left == NULL) {
            return NULL;
        }
    }

    struct eval_node *right = NULL;
    if (ast_node->right != NULL) {
        right = generate_eval_tree(filter, ast_node->right);
        if (right == NULL) {
            return NULL;
        }
    }

    if (ast_node->node_type == FDS_FANT_ROOT) {
        // Ignore this node for the eval tree and just propagate the child
        assert(right == NULL);
        return left;
    }

    struct eval_node *parent;
    if (eval_tree_node_generate(filter, ast_node, &parent) != FDS_FILTER_OK) {
        return NULL;
    }
    parent->left = left;
    parent->right = right;
    return parent;
}

int
evaluate_eval_tree(struct fds_filter *filter, struct eval_node *eval_tree)
{
    filter->reset_context = 1;
    assert(FDS_FILTER_FAIL != 0);
    assert(FDS_FILTER_OK == 0);
    int return_code;
    if ((return_code = setjmp(filter->eval_jmp_buf)) == FDS_FILTER_OK) {
        eval_tree->evaluate(filter, eval_tree);
        return FDS_FILTER_OK;
    } else {
        return return_code;
    }
}

void
destroy_eval_tree(struct eval_node *node)
{
    if (node == NULL) {
        return;
    }
    if (node->is_alloc) {
        // The only alloc a eval function can do so far is for strings.
        free(node->value.string.chars);
    }
    destroy_eval_tree(node->left);
    destroy_eval_tree(node->right);
    free(node);
}
