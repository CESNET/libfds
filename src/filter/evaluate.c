#include <libfds.h>
#include "filter.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define BINARY_NODE_FUNC(FUNC_NAME, ACTION) \
    static void FUNC_NAME(struct fds_filter *filter, struct eval_node *node) { \
        node->left->evaluate(filter, node->left); \
        if (!node->left->is_defined) { \
            node->is_defined = 0; \
            return; \
        } \
        node->right->evaluate(filter, node->right); \
        if (!node->right->is_defined) { \
            node->is_defined = 0; \
            return; \
        } \
        node->is_defined = 1; \
        node->is_more = node->left->is_more || node->right->is_more; \
        ACTION; \
    }

#define UNARY_NODE_FUNC(FUNC_NAME, ACTION) \
    static void FUNC_NAME(struct fds_filter *filter, struct eval_node *node) { \
        node->left->evaluate(filter, node->left); \
        if (!node->left->is_defined) { \
            node->is_defined = 0; \
            return; \
        } \
        node->is_defined = 1; \
        node->is_more = node->left->is_more; \
        ACTION; \
    }

BINARY_NODE_FUNC(f_add_uint, node->value.uint_ = node->left->value.uint_ + node->right->value.uint_)
BINARY_NODE_FUNC(f_sub_uint, node->value.uint_ = node->left->value.uint_ - node->right->value.uint_)
BINARY_NODE_FUNC(f_mul_uint, node->value.uint_ = node->left->value.uint_ * node->right->value.uint_)
BINARY_NODE_FUNC(f_div_uint, node->value.uint_ = node->left->value.uint_ / node->right->value.uint_)
BINARY_NODE_FUNC(f_eq_uint, node->value.uint_ = node->left->value.uint_ == node->right->value.uint_)
BINARY_NODE_FUNC(f_ne_uint, node->value.uint_ = node->left->value.uint_ != node->right->value.uint_)
BINARY_NODE_FUNC(f_lt_uint, node->value.uint_ = node->left->value.uint_ < node->right->value.uint_)
BINARY_NODE_FUNC(f_gt_uint, node->value.uint_ = node->left->value.uint_ > node->right->value.uint_)
BINARY_NODE_FUNC(f_le_uint, node->value.uint_ = node->left->value.uint_ <= node->right->value.uint_)
BINARY_NODE_FUNC(f_ge_uint, node->value.uint_ = node->left->value.uint_ >= node->right->value.uint_)
UNARY_NODE_FUNC(f_cast_uint_to_float, node->value.float_ = node->left->value.uint_)
UNARY_NODE_FUNC(f_cast_uint_to_bool, node->value.uint_ = node->left->value.uint_ != 0)

BINARY_NODE_FUNC(f_add_int, node->value.int_ = node->left->value.int_ + node->right->value.int_)
BINARY_NODE_FUNC(f_sub_int, node->value.int_ = node->left->value.int_ - node->right->value.int_)
BINARY_NODE_FUNC(f_mul_int, node->value.int_ = node->left->value.int_ * node->right->value.int_)
BINARY_NODE_FUNC(f_div_int, node->value.int_ = node->left->value.int_ / node->right->value.int_)
BINARY_NODE_FUNC(f_eq_int, node->value.uint_ = node->left->value.int_ == node->right->value.int_)
BINARY_NODE_FUNC(f_ne_int, node->value.uint_ = node->left->value.int_ != node->right->value.int_)
BINARY_NODE_FUNC(f_lt_int, node->value.uint_ = node->left->value.int_ < node->right->value.int_)
BINARY_NODE_FUNC(f_gt_int, node->value.uint_ = node->left->value.int_ > node->right->value.int_)
BINARY_NODE_FUNC(f_le_int, node->value.uint_ = node->left->value.int_ <= node->right->value.int_)
BINARY_NODE_FUNC(f_ge_int, node->value.uint_ = node->left->value.int_ >= node->right->value.int_)
UNARY_NODE_FUNC(f_minus_int, node->value.int_ = -node->value.int_)
UNARY_NODE_FUNC(f_cast_int_to_uint, node->value.uint_ = node->left->value.int_)
UNARY_NODE_FUNC(f_cast_int_to_float, node->value.float_ = node->left->value.int_)
UNARY_NODE_FUNC(f_cast_int_to_bool, node->value.uint_ = node->left->value.int_ != 0)

BINARY_NODE_FUNC(f_add_float, node->value.float_ = node->left->value.float_ + node->right->value.float_)
BINARY_NODE_FUNC(f_sub_float, node->value.float_ = node->left->value.float_ - node->right->value.float_)
BINARY_NODE_FUNC(f_mul_float, node->value.float_ = node->left->value.float_ * node->right->value.float_)
BINARY_NODE_FUNC(f_div_float, node->value.float_ = node->left->value.float_ / node->right->value.float_)
BINARY_NODE_FUNC(f_eq_float, node->value.uint_ = node->left->value.float_ == node->right->value.float_)
BINARY_NODE_FUNC(f_ne_float, node->value.uint_ = node->left->value.float_ != node->right->value.float_)
BINARY_NODE_FUNC(f_lt_float, node->value.uint_ = node->left->value.float_ < node->right->value.float_)
BINARY_NODE_FUNC(f_gt_float, node->value.uint_ = node->left->value.float_ > node->right->value.float_)
BINARY_NODE_FUNC(f_le_float, node->value.uint_ = node->left->value.float_ <= node->right->value.float_)
BINARY_NODE_FUNC(f_ge_float, node->value.uint_ = node->left->value.float_ >= node->right->value.float_)
UNARY_NODE_FUNC(f_minus_float, node->value.float_ = -node->value.float_)
UNARY_NODE_FUNC(f_cast_float_to_bool, node->value.uint_ = node->left->value.float_ != 0.0f)

BINARY_NODE_FUNC(f_concat_str, {
    unsigned char *new_string = realloc(node->value.string.chars, node->left->value.string.length + node->right->value.string.length);
    if (new_string == NULL) {
        // TODO: handle error
    }
    strncpy(new_string, node->left->value.string.chars, node->left->value.string.length);
    strncpy(new_string + node->left->value.string.length, node->left->value.string.chars, node->left->value.string.length);
    node->value.string.length = node->left->value.string.length + node->right->value.string.length;
    node->value.string.chars = new_string;
})
BINARY_NODE_FUNC(f_eq_str, node->value.uint_ = node->left->value.string.length == node->right->value.string.length &&
    strncmp(node->left->value.string.chars, node->right->value.string.chars, node->left->value.string.length) == 0)
BINARY_NODE_FUNC(f_ne_str, node->value.uint_ = node->left->value.string.length != node->right->value.string.length ||
    strncmp(node->left->value.string.chars, node->right->value.string.chars, node->left->value.string.length) != 0)
UNARY_NODE_FUNC(f_cast_str_to_bool, node->value.uint_ = node->left->value.string.length > 0) // TODO: what to do with defined

// TODO: compare based on mask
BINARY_NODE_FUNC(f_eq_ip_address, node->value.uint_ = node->left->value.ip_address.version == node->right->value.ip_address.version &&
    memcmp(node->left->value.ip_address.bytes, node->right->value.ip_address.bytes, node->left->value.ip_address.version == 4 ? 4 : 16) == 0)
BINARY_NODE_FUNC(f_ne_ip_address, node->value.uint_ = node->left->value.ip_address.version == node->right->value.ip_address.version &&
    memcmp(node->left->value.ip_address.bytes, node->right->value.ip_address.bytes, node->left->value.ip_address.version == 4 ? 4 : 16) != 0)

BINARY_NODE_FUNC(f_eq_mac_address, node->value.uint_ = memcmp(node->left->value.mac_address, node->right->value.ip_address.bytes, 6) == 0)
BINARY_NODE_FUNC(f_ne_mac_address, node->value.uint_ = memcmp(node->left->value.mac_address, node->right->value.ip_address.bytes, 6) != 0)

static void f_and(struct fds_filter *filter, struct eval_node *node) {
    node->left->evaluate(filter, node->left);
    if (node->left->value.uint_ == 0) {
        node->value.uint_ = 0;
        return;
    }
    node->right->evaluate(filter, node->right);
    node->value.uint_ = node->right->value.uint_;
}

static void f_or(struct fds_filter *filter, struct eval_node *node) {
    node->left->evaluate(filter, node->left);
    if (node->left->value.uint_ != 0) {
        node->value.uint_ = 1;
        return;
    }
    node->right->evaluate(filter, node->right);
    node->value.uint_ = node->right->value.uint_;
}

static void f_not(struct fds_filter *filter, struct eval_node *node) {
    node->left->evaluate(filter, node->left);
    node->value.uint_ = node->left->value.uint_;
}

static void f_const(struct fds_filter *filter, struct eval_node *node) {
    // Do nothing
}

static void f_identifier(struct fds_filter *filter, struct eval_node *node) {
    int rc = filter->data_callback(node->identifier_id, filter->data_context, 1, filter->data, &node->value);
    if (rc == FDS_FILTER_MORE) {
        node->is_defined = 1;
        node->is_more = 1;
    } else if (rc == FDS_FILTER_END) {
        node->is_defined = 1;
        node->is_more = 0;
    } else if (rc == FDS_FILTER_NOT_FOUND) {
        node->is_defined = 0;
        node->is_more = 0;
    } else {
        assert(0);
    }
}

struct eval_node *
eval_node_from_ast_node(struct fds_filter *filter, struct fds_filter_ast_node *ast_node)
{
    struct eval_node *eval_node = malloc(sizeof(struct eval_node));
    if (eval_node == NULL) {
        error_no_memory(filter);
        return NULL;
    }

    switch (ast_node->op) {
    case FDS_FILTER_AST_ADD:
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
            break;
        default: assert(0);
        }
        break;

    case FDS_FILTER_AST_SUB:
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
        break;

    case FDS_FILTER_AST_MUL:
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
        break;

    case FDS_FILTER_AST_DIV:
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
        break;

    case FDS_FILTER_AST_EQ:
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
        break;

    case FDS_FILTER_AST_NE:
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
        break;

    case FDS_FILTER_AST_LT:
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
        break;

    case FDS_FILTER_AST_GT:
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
        break;

    case FDS_FILTER_AST_LE:
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
        break;

    case FDS_FILTER_AST_GE:
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
        break;

    case FDS_FILTER_AST_AND:
        assert(ast_node->left->type == FDS_FILTER_TYPE_BOOL && ast_node->right->type == FDS_FILTER_TYPE_BOOL && ast_node->type == FDS_FILTER_TYPE_BOOL);
        eval_node->evaluate = f_and;
        break;

    case FDS_FILTER_AST_OR:
        assert(ast_node->left->type == FDS_FILTER_TYPE_BOOL && ast_node->right->type == FDS_FILTER_TYPE_BOOL && ast_node->type == FDS_FILTER_TYPE_BOOL);
        eval_node->evaluate = f_or;
        break;

    case FDS_FILTER_AST_NOT:
        assert(ast_node->left->type == FDS_FILTER_TYPE_BOOL && ast_node->right == NULL && ast_node->type == FDS_FILTER_TYPE_BOOL);
        eval_node->evaluate = f_not;
        break;

    case FDS_FILTER_AST_CONST:
        assert(ast_node->left == NULL && ast_node->right == NULL);
        eval_node->evaluate = f_const;
        break;

    case FDS_FILTER_AST_IDENTIFIER:
        assert(ast_node->left == NULL && ast_node->right == NULL);
        eval_node->evaluate = f_identifier;
        eval_node->identifier_id = ast_node->identifier_id;
        break;

    case FDS_FILTER_AST_CAST:
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
        } else {
            assert(0);
        }
        break;

    case FDS_FILTER_AST_UMINUS:
        assert(ast_node->right == NULL && ast_node->left->type == ast_node->right->type);
        switch (ast_node->left->type) {
        case FDS_FILTER_TYPE_INT:
            eval_node->evaluate = f_minus_int;
            break;
        case FDS_FILTER_TYPE_FLOAT:
            eval_node->evaluate = f_minus_float;
            break;
        default: assert(0);
        }
        break;

    default: assert(0);
    }
}

struct eval_node *
generate_eval_tree_from_ast(struct fds_filter *filter, struct fds_filter_ast_node *ast_node)
{
    if (ast_node == NULL) {
        return NULL;
    }

    struct eval_node *left = NULL;
    if (ast_node->left != NULL) {
        left = generate_eval_tree_from_ast(filter, ast_node->left);
        if (left == NULL) {
            return NULL;
        }
    }

    struct eval_node *right = NULL;
    if (ast_node->right != NULL) {
        right = generate_eval_tree_from_ast(filter, ast_node->right);
        if (right == NULL) {
            return NULL;
        }
    }

    struct eval_node *parent = eval_node_from_ast_node(filter, ast_node);
    if (parent == NULL) {
        return NULL;
    }
    parent->left = left;
    parent->right = right;
    return parent;
}

int
evaluate_eval_tree(struct fds_filter *filter, struct eval_node *eval_tree)
{
    eval_tree->evaluate(filter, eval_tree);
    return 1;
}

int
destroy_eval_tree(struct eval_node *eval_tree)
{
    if (eval_tree == NULL) {
        return;
    }
    destroy_eval_tree(eval_tree->left);
    destroy_eval_tree(eval_tree->right);
    free(eval_tree);
}

static const char *
eval_func_to_str(const eval_func_t *eval_func)
{
    if (eval_func == f_add_uint) { return "f_add_uint"; }
    else if (eval_func == f_sub_uint) { return "f_sub_uint"; }
    else if (eval_func == f_mul_uint) { return "f_mul_uint"; }
    else if (eval_func == f_div_uint) { return "f_div_uint"; }
    else if (eval_func == f_eq_uint) { return "f_eq_uint"; }
    else if (eval_func == f_ne_uint) { return "f_ne_uint"; }
    else if (eval_func == f_lt_uint) { return "f_lt_uint"; }
    else if (eval_func == f_gt_uint) { return "f_gt_uint"; }
    else if (eval_func == f_le_uint) { return "f_le_uint"; }
    else if (eval_func == f_ge_uint) { return "f_ge_uint"; }
    else if (eval_func == f_cast_uint_to_float) { return "f_cast_uint_to_float"; }
    else if (eval_func == f_cast_uint_to_bool) { return "f_cast_uint_to_bool"; }
    else if (eval_func == f_add_int) { return "f_add_int"; }
    else if (eval_func == f_sub_int) { return "f_sub_int"; }
    else if (eval_func == f_mul_int) { return "f_mul_int"; }
    else if (eval_func == f_div_int) { return "f_div_int"; }
    else if (eval_func == f_eq_int) { return "f_eq_int"; }
    else if (eval_func == f_ne_int) { return "f_ne_int"; }
    else if (eval_func == f_lt_int) { return "f_lt_int"; }
    else if (eval_func == f_gt_int) { return "f_gt_int"; }
    else if (eval_func == f_le_int) { return "f_le_int"; }
    else if (eval_func == f_ge_int) { return "f_ge_int"; }
    else if (eval_func == f_minus_int) { return "f_minus_int"; }
    else if (eval_func == f_cast_int_to_uint) { return "f_cast_int_to_uint"; }
    else if (eval_func == f_cast_int_to_float) { return "f_cast_int_to_float"; }
    else if (eval_func == f_cast_int_to_bool) { return "f_cast_int_to_bool"; }
    else if (eval_func == f_add_float) { return "f_add_float"; }
    else if (eval_func == f_sub_float) { return "f_sub_float"; }
    else if (eval_func == f_mul_float) { return "f_mul_float"; }
    else if (eval_func == f_div_float) { return "f_div_float"; }
    else if (eval_func == f_eq_float) { return "f_eq_float"; }
    else if (eval_func == f_ne_float) { return "f_ne_float"; }
    else if (eval_func == f_lt_float) { return "f_lt_float"; }
    else if (eval_func == f_gt_float) { return "f_gt_float"; }
    else if (eval_func == f_le_float) { return "f_le_float"; }
    else if (eval_func == f_ge_float) { return "f_ge_float"; }
    else if (eval_func == f_minus_float) { return "f_minus_float"; }
    else if (eval_func == f_cast_float_to_bool) { return "f_cast_float_to_bool"; }
    else if (eval_func == f_concat_str) { return "f_concat_str"; }
    else if (eval_func == f_eq_str) { return "f_eq_str"; }
    else if (eval_func == f_ne_str) { return "f_ne_str"; }
    else if (eval_func == f_cast_str_to_bool) { return "f_cast_str_to_bool"; }
    else if (eval_func == f_eq_ip_address) { return "f_eq_ip_address"; }
    else if (eval_func == f_ne_ip_address) { return "f_ne_ip_address"; }
    else if (eval_func == f_eq_mac_address) { return "f_eq_mac_address"; }
    else if (eval_func == f_ne_mac_address) { return "f_ne_mac_address"; }
    else if (eval_func == f_and) { return "f_and"; }
    else if (eval_func == f_or) { return "f_or"; }
    else if (eval_func == f_not) { return "f_not"; }
    else if (eval_func == f_const) { return "f_const"; }
    else if (eval_func == f_identifier) { return "f_identifier"; }
    else { assert(0); }
}

static void
print_value(FILE *outstream, const union fds_filter_value value)
{
    fprintf(outstream, "BOOL %s, ", value.int_ != 0 ? "true" : "false");
    fprintf(outstream, "STR %*s, ", value.string.length, value.string.chars);
    fprintf(outstream, "INT %d, ", value.int_);
    fprintf(outstream, "UINT %u, ", value.uint_);
    fprintf(outstream, "IPv4 %d.%d.%d.%d, ",
            value.ip_address.bytes[0],
            value.ip_address.bytes[1],
            value.ip_address.bytes[2],
            value.ip_address.bytes[3]);
    fprintf(outstream, "IPv6 %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, ",
            value.ip_address.bytes[0],
            value.ip_address.bytes[1],
            value.ip_address.bytes[2],
            value.ip_address.bytes[3],
            value.ip_address.bytes[4],
            value.ip_address.bytes[5],
            value.ip_address.bytes[6],
            value.ip_address.bytes[7],
            value.ip_address.bytes[8],
            value.ip_address.bytes[9],
            value.ip_address.bytes[10],
            value.ip_address.bytes[11],
            value.ip_address.bytes[12],
            value.ip_address.bytes[13],
            value.ip_address.bytes[14],
            value.ip_address.bytes[15]);
    fprintf(outstream, "MAC %02x:%02x:%02x:%02x:%02x:%02x",
            value.mac_address[0],
            value.mac_address[1],
            value.mac_address[2],
            value.mac_address[3],
            value.mac_address[4],
            value.mac_address[5]);
}

void
print_eval_tree(FILE *outstream, struct eval_node *node)
{
    static int level = 0;
    if (node == NULL) {
        return;
    }
    for (int i = 0; i < level; i++) {
        fprintf(outstream, "    ");
    }
    fprintf(outstream, "(%s)\n", eval_func_to_str(node->evaluate));
    level++;
    print_eval_tree(outstream, node->left);
    print_eval_tree(outstream, node->right);
    level--;
}