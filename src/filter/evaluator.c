#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <assert.h>
#include <libfds.h>
#include "filter.h"
#include "evaluator_functions.h"

static int
eval_tree_node_generate(struct fds_filter *filter, struct fds_filter_ast_node *ast_node,
                        struct eval_node **out_eval_node)
{
    struct eval_node *eval_node = calloc(1, sizeof(struct eval_node));
    if (eval_node == NULL) {
        pdebug("returning FAIL because no memory");
        error_no_memory(filter);
        return FDS_FILTER_FAIL;
    }
    *out_eval_node = eval_node;
    eval_node->type = ast_node->type;
    eval_node->is_trie = ast_node->is_trie;
    eval_node->subtype = ast_node->subtype;
    eval_node->value = ast_node->value;

    switch (ast_node->op) {
    case FDS_FILTER_AST_ADD: {
        assert(ast_node->left->type == ast_node->right->type && ast_node->type == ast_node->left->type);
        switch (ast_node->left->type) {
        case FDS_FILTER_TYPE_INT:
            eval_node->evaluate = f_add_int;
            break;
        case FDS_FILTER_TYPE_UINT:
            eval_node->evaluate = f_add_uint;
            break;
        case FDS_FILTER_TYPE_FLOAT:
            eval_node->evaluate = f_add_float;
            break;
        case FDS_FILTER_TYPE_STR:
            eval_node->evaluate = f_concat_str;
            eval_node->is_alloc = true;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FILTER_AST_SUB: {
        assert(ast_node->left->type == ast_node->right->type && ast_node->type == ast_node->left->type);
        switch (ast_node->left->type) {
        case FDS_FILTER_TYPE_INT:
            eval_node->evaluate = f_sub_int;
            break;
        case FDS_FILTER_TYPE_UINT:
            eval_node->evaluate = f_sub_uint;
            break;
        case FDS_FILTER_TYPE_FLOAT:
            eval_node->evaluate = f_sub_float;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FILTER_AST_MUL: {
        assert(ast_node->left->type == ast_node->right->type && ast_node->type == ast_node->left->type);
        switch (ast_node->left->type) {
        case FDS_FILTER_TYPE_INT:
            eval_node->evaluate = f_mul_int;
            break;
        case FDS_FILTER_TYPE_UINT:
            eval_node->evaluate = f_mul_uint;
            break;
        case FDS_FILTER_TYPE_FLOAT:
            eval_node->evaluate = f_mul_float;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FILTER_AST_DIV: {
        assert(ast_node->left->type == ast_node->right->type && ast_node->type == ast_node->left->type);
        switch (ast_node->left->type) {
        case FDS_FILTER_TYPE_INT:
            eval_node->evaluate = f_div_int;
            break;
        case FDS_FILTER_TYPE_UINT:
            eval_node->evaluate = f_div_uint;
            break;
        case FDS_FILTER_TYPE_FLOAT:
            eval_node->evaluate = f_div_float;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FILTER_AST_EQ: {
        assert(ast_node->left->type == ast_node->right->type && ast_node->type == FDS_FILTER_TYPE_BOOL);
        switch (ast_node->left->type) {
        case FDS_FILTER_TYPE_INT:
            eval_node->evaluate = f_eq_int;
            break;
        case FDS_FILTER_TYPE_UINT:
            eval_node->evaluate = f_eq_uint;
            break;
        case FDS_FILTER_TYPE_FLOAT:
            eval_node->evaluate = f_eq_float;
            break;
        case FDS_FILTER_TYPE_STR:
            eval_node->evaluate = f_eq_str;
            break;
        case FDS_FILTER_TYPE_IP_ADDRESS:
            eval_node->evaluate = f_eq_ip_address;
            break;
        case FDS_FILTER_TYPE_MAC_ADDRESS:
            eval_node->evaluate = f_eq_mac_address;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FILTER_AST_NE: {
        assert(ast_node->left->type == ast_node->right->type && ast_node->type == FDS_FILTER_TYPE_BOOL);
        switch (ast_node->left->type) {
        case FDS_FILTER_TYPE_INT:
            eval_node->evaluate = f_ne_int;
            break;
        case FDS_FILTER_TYPE_UINT:
            eval_node->evaluate = f_ne_uint;
            break;
        case FDS_FILTER_TYPE_FLOAT:
            eval_node->evaluate = f_ne_float;
            break;
        case FDS_FILTER_TYPE_STR:
            eval_node->evaluate = f_ne_str;
            break;
        case FDS_FILTER_TYPE_IP_ADDRESS:
            eval_node->evaluate = f_ne_ip_address;
            break;
        case FDS_FILTER_TYPE_MAC_ADDRESS:
            eval_node->evaluate = f_ne_mac_address;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FILTER_AST_LT: {
        assert(ast_node->left->type == ast_node->right->type && ast_node->type == FDS_FILTER_TYPE_BOOL);
        switch (ast_node->left->type) {
        case FDS_FILTER_TYPE_INT:
            eval_node->evaluate = f_lt_int;
            break;
        case FDS_FILTER_TYPE_UINT:
            eval_node->evaluate = f_lt_uint;
            break;
        case FDS_FILTER_TYPE_FLOAT:
            eval_node->evaluate = f_lt_float;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FILTER_AST_GT: {
        assert(ast_node->left->type == ast_node->right->type && ast_node->type == FDS_FILTER_TYPE_BOOL);
        switch (ast_node->left->type) {
        case FDS_FILTER_TYPE_INT:
            eval_node->evaluate = f_gt_int;
            break;
        case FDS_FILTER_TYPE_UINT:
            eval_node->evaluate = f_gt_uint;
            break;
        case FDS_FILTER_TYPE_FLOAT:
            eval_node->evaluate = f_gt_float;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FILTER_AST_LE: {
        assert(ast_node->left->type == ast_node->right->type && ast_node->type == FDS_FILTER_TYPE_BOOL);
        switch (ast_node->left->type) {
        case FDS_FILTER_TYPE_INT:
            eval_node->evaluate = f_le_int;
            break;
        case FDS_FILTER_TYPE_UINT:
            eval_node->evaluate = f_le_uint;
            break;
        case FDS_FILTER_TYPE_FLOAT:
            eval_node->evaluate = f_le_float;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FILTER_AST_GE: {
        assert(ast_node->left->type == ast_node->right->type && ast_node->type == FDS_FILTER_TYPE_BOOL);
        switch (ast_node->left->type) {
        case FDS_FILTER_TYPE_INT:
            eval_node->evaluate = f_ge_int;
            break;
        case FDS_FILTER_TYPE_UINT:
            eval_node->evaluate = f_ge_uint;
            break;
        case FDS_FILTER_TYPE_FLOAT:
            eval_node->evaluate = f_ge_float;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FILTER_AST_AND: {
        assert(ast_node->left->type == FDS_FILTER_TYPE_BOOL
               && ast_node->right->type == FDS_FILTER_TYPE_BOOL
               && ast_node->type == FDS_FILTER_TYPE_BOOL);
        eval_node->evaluate = f_and;
        eval_node->is_defined = true; // Always defined because of ANY node
        eval_node->is_more = false; // Can never have more because of ANY node
    } break;

    case FDS_FILTER_AST_OR: {
        assert(ast_node->left->type == FDS_FILTER_TYPE_BOOL
               && ast_node->right->type == FDS_FILTER_TYPE_BOOL
               && ast_node->type == FDS_FILTER_TYPE_BOOL);
        eval_node->evaluate = f_or;
        eval_node->is_defined = true; // Always defined because of ANY node
        eval_node->is_more = false; // Can never have more because of ANY node
    } break;

    case FDS_FILTER_AST_NOT: {
        assert(ast_node->left->type == FDS_FILTER_TYPE_BOOL
               && ast_node->right == NULL
               && ast_node->type == FDS_FILTER_TYPE_BOOL);
        eval_node->evaluate = f_not;
        eval_node->is_defined = true; // Always defined because of ANY node
        eval_node->is_more = false; // Can never have more because of ANY node
    } break;

    case FDS_FILTER_AST_CONST: {
        assert(ast_node->left == NULL && ast_node->right == NULL);
        eval_node->evaluate = f_const;
        eval_node->is_defined = true;
    } break;

    case FDS_FILTER_AST_IDENTIFIER: {
        assert(ast_node->left == NULL && ast_node->right == NULL);
        eval_node->identifier_id = ast_node->identifier_id;
        if (ast_node->identifier_type == FDS_FILTER_IDENTIFIER_CONST) {
            eval_node->is_defined = true;
            eval_node->is_more = false;
            eval_node->value = ast_node->value;
            eval_node->evaluate = f_const;
        } else {
            eval_node->evaluate = f_identifier;
        }
    } break;

    case FDS_FILTER_AST_CAST: {
        assert(ast_node->right == NULL);
        if (ast_node->left->type == FDS_FILTER_TYPE_INT && ast_node->type == FDS_FILTER_TYPE_FLOAT) {
            eval_node->evaluate = f_cast_int_to_float;
        } else if (ast_node->left->type == FDS_FILTER_TYPE_INT && ast_node->type == FDS_FILTER_TYPE_UINT) {
            eval_node->evaluate = f_cast_int_to_uint;
        } else if (ast_node->left->type == FDS_FILTER_TYPE_UINT && ast_node->type == FDS_FILTER_TYPE_FLOAT) {
            eval_node->evaluate = f_cast_uint_to_float;
        } else if (ast_node->left->type == FDS_FILTER_TYPE_UINT && ast_node->type == FDS_FILTER_TYPE_BOOL) {
            eval_node->evaluate = f_cast_uint_to_bool;
        } else if (ast_node->left->type == FDS_FILTER_TYPE_INT && ast_node->type == FDS_FILTER_TYPE_BOOL) {
            eval_node->evaluate = f_cast_int_to_bool;
        } else if (ast_node->left->type == FDS_FILTER_TYPE_FLOAT && ast_node->type == FDS_FILTER_TYPE_BOOL) {
            eval_node->evaluate = f_cast_float_to_bool;
        } else if (ast_node->left->type == FDS_FILTER_TYPE_STR && ast_node->type == FDS_FILTER_TYPE_BOOL) {
            eval_node->evaluate = f_cast_str_to_bool;
        } else if (ast_node->left->type == FDS_FILTER_TYPE_IP_ADDRESS && ast_node->type == FDS_FILTER_TYPE_BOOL) {
            eval_node->evaluate = f_exists;
        } else if (ast_node->left->type == FDS_FILTER_TYPE_MAC_ADDRESS && ast_node->type == FDS_FILTER_TYPE_BOOL) {
            eval_node->evaluate = f_exists;
        } else if (ast_node->left->type == FDS_FILTER_TYPE_LIST && ast_node->type == FDS_FILTER_TYPE_LIST) {
            if (ast_node->left->subtype == FDS_FILTER_TYPE_INT && ast_node->subtype == FDS_FILTER_TYPE_UINT) {
                eval_node->evaluate = f_cast_list_int_to_uint;
            } else if (ast_node->left->subtype == FDS_FILTER_TYPE_INT && ast_node->subtype == FDS_FILTER_TYPE_FLOAT) {
                eval_node->evaluate = f_cast_list_int_to_float;
            } else if (ast_node->left->subtype == FDS_FILTER_TYPE_UINT && ast_node->subtype == FDS_FILTER_TYPE_FLOAT) {
                eval_node->evaluate = f_cast_list_uint_to_float;
            } else {
                assert(0 && "Unhandled list cast");
            }
        } else {
            assert(0 && "Unhandled cast");
        }
    } break;

    case FDS_FILTER_AST_UMINUS: {
        assert(ast_node->right == NULL && ast_node->type == ast_node->left->type);
        switch (ast_node->left->type) {
        case FDS_FILTER_TYPE_INT:
            eval_node->evaluate = f_minus_int;
            break;
        case FDS_FILTER_TYPE_FLOAT:
            eval_node->evaluate = f_minus_float;
            break;
        default: assert(0);
        }
    } break;

    case FDS_FILTER_AST_ANY: {
        assert(ast_node->left->type == FDS_FILTER_TYPE_BOOL
               && ast_node->left->type == FDS_FILTER_TYPE_BOOL
               && ast_node->right == NULL);
        eval_node->evaluate = f_any;
        eval_node->is_defined = true;
    } break;

    case FDS_FILTER_AST_ROOT: {
        // TODO: Solve this a better way that doesn't unnecessarily allocate only to free it right away.
        free(eval_node);
        *out_eval_node = NULL;
    } break;

    case FDS_FILTER_AST_IN: {
        assert((ast_node->left->type == ast_node->right->subtype || ast_node->right->subtype == 0)
               && ast_node->right->type == FDS_FILTER_TYPE_LIST);
        switch (ast_node->left->type) {
        case FDS_FILTER_TYPE_UINT:
            eval_node->evaluate = f_in_uint;
            break;
        case FDS_FILTER_TYPE_INT:
            eval_node->evaluate = f_in_int;
            break;
        case FDS_FILTER_TYPE_FLOAT:
            eval_node->evaluate = f_in_float;
            break;
        case FDS_FILTER_TYPE_STR:
            eval_node->evaluate = f_in_str;
            break;
        case FDS_FILTER_TYPE_IP_ADDRESS:
            eval_node->evaluate = ast_node->right->is_trie ? f_ip_address_in_trie : f_in_ip_address;
            break;
        case FDS_FILTER_TYPE_MAC_ADDRESS:
            eval_node->evaluate = f_in_mac_address;
            break;
        case FDS_FILTER_TYPE_NONE: // List with no values
            eval_node->evaluate = f_const;
            eval_node->value.uint_ = 0;
            eval_node->is_defined = true;
            break;
        default: assert(0 && "Unhandled type for IN");
        }
    } break;

    default: assert(0 && "Unhandled ast node operation");
    }

    return FDS_FILTER_OK;
}

struct eval_node *
eval_tree_generate(struct fds_filter *filter, struct fds_filter_ast_node *ast_node)
{
    if (ast_node == NULL) {
        return NULL;
    }

    struct eval_node *left = NULL;
    if (ast_node->left != NULL) {
        left = eval_tree_generate(filter, ast_node->left);
        if (left == NULL) {
            return NULL;
        }
    }

    struct eval_node *right = NULL;
    if (ast_node->right != NULL) {
        right = eval_tree_generate(filter, ast_node->right);
        if (right == NULL) {
            return NULL;
        }
    }

    if (ast_node->op == FDS_FILTER_AST_ROOT) {
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
eval_tree_evaluate(struct fds_filter *filter, struct eval_node *eval_tree)
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
eval_tree_destroy(struct eval_node *node)
{
    if (node == NULL) {
        return;
    }
    if (node->is_alloc) {
        // The only alloc a eval function can do so far is for strings.
        free(node->value.string.chars);
    }
    eval_tree_destroy(node->left);
    eval_tree_destroy(node->right);
    free(node);
}

static void
print_value(FILE *outstream, enum fds_filter_data_type type,
            enum fds_filter_data_type subtype, int is_trie, const union fds_filter_value value)
{
    switch (type) {
    case FDS_FILTER_TYPE_BOOL:
        fprintf(outstream, "BOOL %s", value.int_ != 0 ? "true" : "false");
        break;
    case FDS_FILTER_TYPE_STR:
        fprintf(outstream, "STR %*s", value.string.length, value.string.chars);
        break;
    case FDS_FILTER_TYPE_INT:
        fprintf(outstream, "INT %li", value.int_);
        break;
    case FDS_FILTER_TYPE_UINT:
        fprintf(outstream, "UINT %lu", value.uint_);
        break;
    case FDS_FILTER_TYPE_IP_ADDRESS:
        if (value.ip_address.version == 4) {
            fprintf(outstream, "IPv4 %d.%d.%d.%d",
                    value.ip_address.bytes[0], value.ip_address.bytes[1],
                    value.ip_address.bytes[2], value.ip_address.bytes[3]);
        } else if (value.ip_address.version == 6) {
            fprintf(outstream,
                    "IPv6 %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                    value.ip_address.bytes[0], value.ip_address.bytes[1],
                    value.ip_address.bytes[2], value.ip_address.bytes[3],
                    value.ip_address.bytes[4], value.ip_address.bytes[5],
                    value.ip_address.bytes[6], value.ip_address.bytes[7],
                    value.ip_address.bytes[8], value.ip_address.bytes[9],
                    value.ip_address.bytes[10], value.ip_address.bytes[11],
                    value.ip_address.bytes[12], value.ip_address.bytes[13],
                    value.ip_address.bytes[14], value.ip_address.bytes[15]);
        } else {
            fprintf(outstream, "<invalid ip address value>");
        }
        break;
    case FDS_FILTER_TYPE_MAC_ADDRESS:
        fprintf(outstream, "MAC %02x:%02x:%02x:%02x:%02x:%02x",
                value.mac_address[0], value.mac_address[1],
                value.mac_address[2], value.mac_address[3],
                value.mac_address[4], value.mac_address[5]);
        break;
    case FDS_FILTER_TYPE_LIST:
        if (is_trie) {
            fprintf(outstream, "TRIE");
        } else {
            fprintf(outstream, "LIST (%lu)", value.list.length);
            for (uint64_t i = 0; i < value.list.length; i++) {
                fprintf(outstream, " ");
                print_value(outstream, subtype, FDS_FILTER_TYPE_NONE, 0, value.list.items[i]);
            }
        }
        break;
    default: fprintf(outstream, "<invalid value>");
    };
}

void
eval_tree_print(FILE *outstream, struct eval_node *node)
{
    static int level = 0;
    if (node == NULL) {
        return;
    }
    for (int i = 0; i < level; i++) {
        fprintf(outstream, "    ");
    }
    fprintf(outstream, "(%s) more:%d defined:%d value:",
            eval_func_to_str(node->evaluate), node->is_more, node->is_defined);
    print_value(outstream, node->type, node->subtype, node->is_trie, node->value);
    fprintf(outstream, "\n");
    level++;
    eval_tree_print(outstream, node->left);
    eval_tree_print(outstream, node->right);
    level--;
}