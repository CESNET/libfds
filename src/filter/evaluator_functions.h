#ifndef FDS_FILTER_EVALUATOR_FUNCTIONS_H
#define FDS_FILTER_EVALUATOR_FUNCTIONS_H

#include <libfds.h>
#include "filter.h"

#define BINARY_NODE_FUNC(FUNC_NAME, ACTION) \
    static inline void \
    FUNC_NAME(struct fds_filter *filter, struct eval_node *node) { \
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
        node->is_defined = true; \
        node->is_more = node->left->is_more || node->right->is_more; \
        ACTION; \
    }

#define UNARY_NODE_FUNC(FUNC_NAME, ACTION) \
    static inline void \
    FUNC_NAME(struct fds_filter *filter, struct eval_node *node) { \
        node->left->evaluate(filter, node->left); \
        if (!node->left->is_defined) { \
            node->is_defined = 0; \
            return; \
        } \
        node->is_defined = true; \
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
        error_no_memory(filter);
        longjmp(filter->eval_jmp_buf, FDS_FILTER_FAIL);
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
)

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

static inline void
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

static inline void
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

static inline void
f_not(struct fds_filter *filter, struct eval_node *node)
{
    node->left->evaluate(filter, node->left);
    node->value.uint_ = node->left->value.uint_ == 0;
}

static inline void
f_const(struct fds_filter *filter, struct eval_node *node)
{
    // Do nothing
    (void)filter;
    (void)node;
}

static inline void
f_identifier(struct fds_filter *filter, struct eval_node *node)
{
    int return_code = filter->field_callback(node->identifier_id, filter->user_context,
        filter->reset_context, filter->data, &node->value);
    assert(return_code == FDS_FILTER_OK || return_code == FDS_FILTER_OK_MORE
        || return_code == FDS_FILTER_FAIL);
    node->is_defined = (return_code == FDS_FILTER_OK || return_code == FDS_FILTER_OK_MORE);
    node->is_more = (return_code == FDS_FILTER_OK_MORE);
    filter->reset_context = 0; // If the context should reset this is set to 1 by one of the parent nodes.
}

static inline void
f_any(struct fds_filter *filter, struct eval_node *node)
{
    do {
        node->left->evaluate(filter, node->left);
    } while (node->left->is_defined && node->left->value.uint_ == 0 && node->left->is_more);
    node->value.uint_ = node->left->is_defined && node->left->value.uint_;
    node->is_more = false;
    node->is_defined = true;
    filter->reset_context = 1;
}

static inline void
f_exists(struct fds_filter *filter, struct eval_node *node)
{
    node->left->evaluate(filter, node->left);
    node->is_defined = true;
    node->is_more = false;
    node->value.uint_ = node->is_defined;
}

static inline const char *
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

#endif // FDS_FILTER_EVALUATOR_FUNCTIONS_H