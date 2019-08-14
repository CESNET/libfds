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

BINARY_NODE_FUNC(f_add_uint, {
    node->value.uint_ = node->left->value.uint_ + node->right->value.uint_;
})
BINARY_NODE_FUNC(f_sub_uint, {
    node->value.uint_ = node->left->value.uint_ - node->right->value.uint_;
})
BINARY_NODE_FUNC(f_mul_uint, {
    node->value.uint_ = node->left->value.uint_ * node->right->value.uint_;
})
BINARY_NODE_FUNC(f_div_uint, {
    node->value.uint_ = node->left->value.uint_ / node->right->value.uint_;
})
BINARY_NODE_FUNC(f_eq_uint, {
    node->value.bool_ = node->left->value.uint_ == node->right->value.uint_;
})
BINARY_NODE_FUNC(f_ne_uint, {
    node->value.bool_ = node->left->value.uint_ != node->right->value.uint_;
})
BINARY_NODE_FUNC(f_lt_uint, {
    node->value.bool_ = node->left->value.uint_ < node->right->value.uint_;
})
BINARY_NODE_FUNC(f_gt_uint, {
    node->value.bool_ = node->left->value.uint_ > node->right->value.uint_;
})
BINARY_NODE_FUNC(f_le_uint, {
    node->value.bool_ = node->left->value.uint_ <= node->right->value.uint_;
})
BINARY_NODE_FUNC(f_ge_uint, {
    node->value.bool_ = node->left->value.uint_ >= node->right->value.uint_;
})
UNARY_NODE_FUNC(f_cast_uint_to_float, {
    node->value.float_ = node->left->value.uint_;
})
UNARY_NODE_FUNC(f_cast_uint_to_bool, {
    node->value.bool_ = node->left->value.uint_ != 0;
})
BINARY_NODE_FUNC(f_add_int, {
    node->value.int_ = node->left->value.int_ + node->right->value.int_;
})
BINARY_NODE_FUNC(f_sub_int, {
    node->value.int_ = node->left->value.int_ - node->right->value.int_;
})
BINARY_NODE_FUNC(f_mul_int, {
    node->value.int_ = node->left->value.int_ * node->right->value.int_;
})
BINARY_NODE_FUNC(f_div_int, {
    node->value.int_ = node->left->value.int_ / node->right->value.int_;
})
BINARY_NODE_FUNC(f_eq_int, {
    node->value.bool_ = node->left->value.int_ == node->right->value.int_;
})
BINARY_NODE_FUNC(f_ne_int, {
    node->value.bool_ = node->left->value.int_ != node->right->value.int_;
})
BINARY_NODE_FUNC(f_lt_int, {
    node->value.bool_ = node->left->value.int_ < node->right->value.int_;
})
BINARY_NODE_FUNC(f_gt_int, {
    node->value.bool_ = node->left->value.int_ > node->right->value.int_;
})
BINARY_NODE_FUNC(f_le_int, {
    node->value.bool_ = node->left->value.int_ <= node->right->value.int_;
})
BINARY_NODE_FUNC(f_ge_int, {
    node->value.bool_ = node->left->value.int_ >= node->right->value.int_;
})
UNARY_NODE_FUNC(f_minus_int, {
    node->value.int_ = -node->value.int_;
})
UNARY_NODE_FUNC(f_cast_int_to_uint, {
    node->value.uint_ = node->left->value.int_;
})
UNARY_NODE_FUNC(f_cast_int_to_float, {
    node->value.float_ = node->left->value.int_;
})
UNARY_NODE_FUNC(f_cast_int_to_bool, {
    node->value.bool_ = node->left->value.int_ != 0;
})
BINARY_NODE_FUNC(f_add_float, {
    node->value.float_ = node->left->value.float_ + node->right->value.float_;
})
BINARY_NODE_FUNC(f_sub_float, {
    node->value.float_ = node->left->value.float_ - node->right->value.float_;
})
BINARY_NODE_FUNC(f_mul_float, {
    node->value.float_ = node->left->value.float_ * node->right->value.float_;
})
BINARY_NODE_FUNC(f_div_float, {
    node->value.float_ = node->left->value.float_ / node->right->value.float_;
})
BINARY_NODE_FUNC(f_eq_float, {
    node->value.bool_ = node->left->value.float_ == node->right->value.float_;
})
BINARY_NODE_FUNC(f_ne_float, {
    node->value.bool_ = node->left->value.float_ != node->right->value.float_;
})
BINARY_NODE_FUNC(f_lt_float, {
    node->value.bool_ = node->left->value.float_ < node->right->value.float_;
})
BINARY_NODE_FUNC(f_gt_float, {
    node->value.bool_ = node->left->value.float_ > node->right->value.float_;
})
BINARY_NODE_FUNC(f_le_float, {
    node->value.bool_ = node->left->value.float_ <= node->right->value.float_;
})
BINARY_NODE_FUNC(f_ge_float, {
    node->value.bool_ = node->left->value.float_ >= node->right->value.float_;
})
UNARY_NODE_FUNC(f_minus_float, {
    node->value.float_ = -node->value.float_;
})
UNARY_NODE_FUNC(f_cast_float_to_bool, {
    node->value.uint_ = node->left->value.float_ != 0.0f; // TODO: ?
})

BINARY_NODE_FUNC(f_concat_str, {
    char *new_string = realloc(node->value.string.chars,
                               node->left->value.string.length + node->right->value.string.length);
    if (new_string == NULL) {
        // TODO: handle error. longjmp?
        assert(0);
    }
    strncpy(new_string, node->left->value.string.chars, node->left->value.string.length);
    strncpy(new_string + node->left->value.string.length, node->right->value.string.chars,
            node->right->value.string.length);
    node->value.string.length = node->left->value.string.length + node->right->value.string.length;
    node->value.string.chars = new_string;
})

BINARY_NODE_FUNC(f_eq_str, {
    node->value.bool_ = (node->left->value.string.length == node->right->value.string.length
                         && strncmp(node->left->value.string.chars,
                                    node->right->value.string.chars,
                                    node->left->value.string.length) == 0);
})

BINARY_NODE_FUNC(f_ne_str, {
    node->value.bool_ = (node->left->value.string.length != node->right->value.string.length
                         || strncmp(node->left->value.string.chars,
                                    node->right->value.string.chars,
                                    node->left->value.string.length) != 0);
})

UNARY_NODE_FUNC(f_cast_str_to_bool, { node->value.uint_ = node->left->value.string.length > 0; })
// TODO: what to do with defined

// TODO: compare based on mask
BINARY_NODE_FUNC(f_eq_ip_address, {
    node->value.bool_ = (node->left->value.ip_address.version == node->right->value.ip_address.version
                         && memcmp(node->left->value.ip_address.bytes,
                                   node->right->value.ip_address.bytes,
                                   node->left->value.ip_address.version == 4 ? 4 : 16) == 0);
})
BINARY_NODE_FUNC(f_ne_ip_address, {
    node->value.bool_ = (node->left->value.ip_address.version != node->right->value.ip_address.version
                         || memcmp(node->left->value.ip_address.bytes,
                                   node->right->value.ip_address.bytes,
                                   node->left->value.ip_address.version == 4 ? 4 : 16) != 0);
})
BINARY_NODE_FUNC(f_eq_mac_address, {
    node->value.bool_ = memcmp(node->left->value.mac_address, node->right->value.mac_address, 6) == 0;
})

BINARY_NODE_FUNC(f_ne_mac_address, {
    node->value.bool_ = memcmp(node->left->value.mac_address, node->right->value.mac_address, 6) != 0;
})

#define LIST_IN_FUNC(FUNC_NAME, COMPARE_STATEMENT) \
    BINARY_NODE_FUNC(FUNC_NAME, { \
        for (uint64_t i = 0; i < node->right->value.list.length; i++) { \
            if (COMPARE_STATEMENT) { \
                node->value.uint_ = 1; \
                return; \
            } \
        } \
        node->value.uint_ = 0; \
    })

LIST_IN_FUNC(f_in_uint,
    node->left->value.uint_ == node->right->value.list.items[i].uint_
)
LIST_IN_FUNC(f_in_int,
    node->left->value.int_ == node->right->value.list.items[i].int_
)
LIST_IN_FUNC(f_in_float,
    node->left->value.float_ == node->right->value.list.items[i].float_
)
LIST_IN_FUNC(f_in_str,
    node->left->value.string.length == node->right->value.list.items[i].string.length
    && strncmp(node->left->value.string.chars,
               node->right->value.list.items[i].string.chars,
               node->left->value.string.length) == 0
)
LIST_IN_FUNC(f_in_mac_address,
    memcmp(node->left->value.mac_address, node->right->value.list.items[i].mac_address, 6) == 0
)
LIST_IN_FUNC(f_in_ip_address,
    node->left->value.ip_address.version == node->right->value.list.items[i].ip_address.version
    && memcmp(node->left->value.ip_address.bytes,
              node->right->value.list.items[i].ip_address.bytes,
              node->left->value.ip_address.version == 4 ? 4 : 16) == 0
) // TODO: use trie

#define LIST_CAST_FUNC(FUNC_NAME, CAST_STATEMENT) \
    UNARY_NODE_FUNC(FUNC_NAME, { \
        node->value.list = node->left->value.list; \
        for (uint64_t i = 0; i < node->right->value.list.length; i++) { \
            CAST_STATEMENT; \
        } \
    })
LIST_CAST_FUNC(f_cast_list_uint_to_float, {
    node->value.list.items[i].float_ = node->value.list.items[i].uint_;
})
LIST_CAST_FUNC(f_cast_list_int_to_uint, {
    node->value.list.items[i].uint_ = node->value.list.items[i].int_;
})
LIST_CAST_FUNC(f_cast_list_int_to_float, {
    node->value.list.items[i].float_ = node->value.list.items[i].int_;
})

BINARY_NODE_FUNC(f_ip_address_in_trie, {
    node->value.bool_ =
        fds_trie_find(
            (fds_trie_t *)node->right->value.pointer,
            node->left->value.ip_address.version,
            node->left->value.ip_address.bytes,
            node->left->value.ip_address.mask
        );
})

BINARY_NODE_FUNC(f_ip_address_not_in_trie, {
    node->value.bool_ =
        !fds_trie_find(
            (fds_trie_t *)node->right->value.pointer,
            node->left->value.ip_address.version,
            node->left->value.ip_address.bytes,
            node->left->value.ip_address.mask
        );
})

static void
f_and(struct fds_filter *filter, struct eval_node *node)
{
    node->left->evaluate(filter, node->left);
    if (node->left->value.uint_ == 0) {
        node->value.uint_ = 0;
        return;
    }
    node->right->evaluate(filter, node->right);
    node->value.uint_ = node->right->value.uint_;
}

static void
f_or(struct fds_filter *filter, struct eval_node *node)
{
    node->left->evaluate(filter, node->left);
    if (node->left->value.uint_ != 0) {
        node->value.uint_ = 1;
        return;
    }
    node->right->evaluate(filter, node->right);
    node->value.uint_ = node->right->value.uint_;
}

static void
f_not(struct fds_filter *filter, struct eval_node *node)
{
    node->left->evaluate(filter, node->left);
    node->value.uint_ = node->left->value.uint_ == 0;
}

static void
f_const(struct fds_filter *filter, struct eval_node *node)
{
    // Do nothing
    (void)filter;
    (void)node;
}

static void
f_identifier(struct fds_filter *filter, struct eval_node *node)
{
    int return_code = filter->field_callback(node->identifier_id, filter->user_context,
                                             filter->reset_context, filter->data, &node->value);
    assert(return_code == FDS_FILTER_OK || return_code == FDS_FILTER_OK_MORE || return_code == FDS_FILTER_FAIL);
    node->is_defined = (return_code == FDS_FILTER_OK || return_code == FDS_FILTER_OK_MORE);
    node->is_more = (return_code == FDS_FILTER_OK_MORE);
    filter->reset_context = 0; // If the context should reset this is set to 1 by one of the parent nodes.
}

static void
f_any(struct fds_filter *filter, struct eval_node *node)
{
    do {
        node->left->evaluate(filter, node->left);
    } while (node->left->is_defined && node->left->value.uint_ == 0 && node->left->is_more);
    node->value.uint_ = node->left->is_defined && node->left->value.uint_;
    node->is_more = 0;
    node->is_defined = 1;
    filter->reset_context = 1;
}

static void
f_exists(struct fds_filter *filter, struct eval_node *node)
{
    node->left->evaluate(filter, node->left);
    node->is_defined = 1;
    node->is_more = 0;
    node->value.uint_ = node->is_defined;
}

static int
eval_node_from_ast_node(struct fds_filter *filter, struct fds_filter_ast_node *ast_node,
                        struct eval_node **out_eval_node)
{
    struct eval_node *eval_node = malloc(sizeof(struct eval_node));
    if (eval_node == NULL) {
        error_no_memory(filter);
        return 0;
    }
    *out_eval_node = eval_node;
    eval_node->is_defined = 0;
    eval_node->is_more = 0;
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
        eval_node->is_defined = 1; // Always defined because of ANY node
        eval_node->is_more = 0; // Can never have more because of ANY node
    } break;

    case FDS_FILTER_AST_OR: {
        assert(ast_node->left->type == FDS_FILTER_TYPE_BOOL
               && ast_node->right->type == FDS_FILTER_TYPE_BOOL
               && ast_node->type == FDS_FILTER_TYPE_BOOL);
        eval_node->evaluate = f_or;
        eval_node->is_defined = 1; // Always defined because of ANY node
        eval_node->is_more = 0; // Can never have more because of ANY node
    } break;

    case FDS_FILTER_AST_NOT: {
        assert(ast_node->left->type == FDS_FILTER_TYPE_BOOL
               && ast_node->right == NULL
               && ast_node->type == FDS_FILTER_TYPE_BOOL);
        eval_node->evaluate = f_not;
        eval_node->is_defined = 1; // Always defined because of ANY node
        eval_node->is_more = 0; // Can never have more because of ANY node
    } break;

    case FDS_FILTER_AST_CONST: {
        assert(ast_node->left == NULL && ast_node->right == NULL);
        eval_node->evaluate = f_const;
        eval_node->is_defined = 1;
    } break;

    case FDS_FILTER_AST_IDENTIFIER: {
        assert(ast_node->left == NULL && ast_node->right == NULL);
        eval_node->identifier_id = ast_node->identifier_id;
        if (ast_node->identifier_type == FDS_FILTER_IDENTIFIER_CONST) {
            eval_node->is_defined = 1;
            eval_node->is_more = 0;
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
        eval_node->is_defined = 1;
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
            eval_node->is_defined = 1;
            break;
        default: assert(0 && "Unhandled type for IN");
        }
    } break;

    default: assert(0 && "Unhandled ast node operation");
    }

    return 1;
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

    if (ast_node->op == FDS_FILTER_AST_ROOT) {
        // Ignore this node for the eval tree and just propagate the child
        assert(right == NULL);
        return left;
    }

    struct eval_node *parent;
    if (!eval_node_from_ast_node(filter, ast_node, &parent)) {
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
    eval_tree->evaluate(filter, eval_tree);
    pdebug("After evaluation");
    print_eval_tree(stderr, eval_tree);
    return 1;
}

void
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
eval_func_to_str(const eval_func_t eval_func)
{
    #define F(FUNC_NAME)    if (eval_func == FUNC_NAME) { return #FUNC_NAME; }
    F(f_add_uint) F(f_sub_uint) F(f_mul_uint) F(f_div_uint)
    F(f_eq_uint) F(f_ne_uint) F(f_lt_uint) F(f_gt_uint) F(f_le_uint) F(f_ge_uint)
    F(f_cast_uint_to_float) F(f_cast_uint_to_bool)

    F(f_add_int) F(f_sub_int) F(f_mul_int) F(f_div_int)
    F(f_eq_int) F(f_ne_int) F(f_lt_int) F(f_gt_int) F(f_le_int) F(f_ge_int)
    F(f_minus_int) F(f_cast_int_to_uint) F(f_cast_int_to_float) F(f_cast_int_to_bool)

    F(f_add_float) F(f_sub_float) F(f_mul_float) F(f_div_float)
    F(f_eq_float) F(f_ne_float) F(f_lt_float) F(f_gt_float) F(f_le_float) F(f_ge_float)
    F(f_minus_float) F(f_cast_float_to_bool)

    F(f_concat_str) F(f_eq_str) F(f_ne_str) F(f_cast_str_to_bool)

    F(f_eq_ip_address) F(f_ne_ip_address)

    F(f_eq_mac_address) F(f_ne_mac_address)

    F(f_and) F(f_or) F(f_not) F(f_const) F(f_identifier) F(f_any) F(f_exists)

    F(f_in_uint) F(f_in_int) F(f_in_float) F(f_in_str) F(f_in_ip_address) F(f_in_mac_address)

    F(f_ip_address_in_trie) F(f_ip_address_not_in_trie)
    #undef F
    assert(0);
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
print_eval_tree(FILE *outstream, struct eval_node *node)
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
    print_eval_tree(outstream, node->left);
    print_eval_tree(outstream, node->right);
    level--;
}