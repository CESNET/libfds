#include <stdlib.h>
#include "evaluator.h"
#include "evaluator_functions.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

static inline int
bit_compare(uint8_t *a, uint8_t *b, int n_bits)
{
    int n_bytes = n_bits / 8;
    int n_remaining_bits = n_bits % 8;
    int result = memcmp(a, b, n_bytes) == 0;
    result = result && a[n_bytes] >> (8 - n_remaining_bits) == b[n_bytes] >> (8 - n_remaining_bits);
    return result;
}

#define DEFINE_FUNC(NAME, BODY) \
void \
NAME(struct fds_filter *filter, struct eval_node *node) \
{ \
    BODY \
}

#define DEFINE_BINARY_FUNC(NAME, BODY) \
DEFINE_FUNC(NAME, { \
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
    BODY; \
})

#define DEFINE_UNARY_FUNC(NAME, BODY) \
DEFINE_FUNC(NAME, { \
    node->left->evaluate(filter, node->left); \
    if (!node->left->is_defined) { \
        node->is_defined = 0; \
        return; \
    } \
    node->is_defined = true; \
    node->is_more = node->left->is_more; \
    BODY; \
})

DEFINE_BINARY_FUNC(f_add_uint, {
    node->value.uint_ = node->left->value.uint_ + node->right->value.uint_;
})

DEFINE_BINARY_FUNC(f_sub_uint, {
    node->value.uint_ = node->left->value.uint_ - node->right->value.uint_;
})

DEFINE_BINARY_FUNC(f_mul_uint, {
    node->value.uint_ = node->left->value.uint_ * node->right->value.uint_;
})

DEFINE_BINARY_FUNC(f_div_uint, {
    node->value.uint_ = node->left->value.uint_ / node->right->value.uint_;
})

DEFINE_BINARY_FUNC(f_eq_uint, {
    node->value.bool_ = node->left->value.uint_ == node->right->value.uint_;
})

DEFINE_BINARY_FUNC(f_ne_uint, {
    node->value.bool_ = node->left->value.uint_ != node->right->value.uint_;
})

DEFINE_BINARY_FUNC(f_lt_uint, {
    node->value.bool_ = node->left->value.uint_ < node->right->value.uint_;
})

DEFINE_BINARY_FUNC(f_gt_uint, {
    node->value.bool_ = node->left->value.uint_ > node->right->value.uint_;
})

DEFINE_BINARY_FUNC(f_le_uint, {
    node->value.bool_ = node->left->value.uint_ <= node->right->value.uint_;
})

DEFINE_BINARY_FUNC(f_ge_uint, {
    node->value.bool_ = node->left->value.uint_ >= node->right->value.uint_;
})

DEFINE_UNARY_FUNC(f_cast_uint_to_float, {
    node->value.float_ = node->left->value.uint_;
})

DEFINE_UNARY_FUNC(f_cast_uint_to_bool, {
    node->value.bool_ = node->left->value.uint_ != 0;
})

DEFINE_BINARY_FUNC(f_add_int, {
    node->value.int_ = node->left->value.int_ + node->right->value.int_;
})

DEFINE_BINARY_FUNC(f_sub_int, {
    node->value.int_ = node->left->value.int_ - node->right->value.int_;
})

DEFINE_BINARY_FUNC(f_mul_int, {
    node->value.int_ = node->left->value.int_ * node->right->value.int_;
})

DEFINE_BINARY_FUNC(f_div_int, {
    node->value.int_ = node->left->value.int_ / node->right->value.int_;
})

DEFINE_BINARY_FUNC(f_eq_int, {
    node->value.bool_ = node->left->value.int_ == node->right->value.int_;
})

DEFINE_BINARY_FUNC(f_ne_int, {
    node->value.bool_ = node->left->value.int_ != node->right->value.int_;
})

DEFINE_BINARY_FUNC(f_lt_int, {
    node->value.bool_ = node->left->value.int_ < node->right->value.int_;
})

DEFINE_BINARY_FUNC(f_gt_int, {
    node->value.bool_ = node->left->value.int_ > node->right->value.int_;
})

DEFINE_BINARY_FUNC(f_le_int, {
    node->value.bool_ = node->left->value.int_ <= node->right->value.int_;
})

DEFINE_BINARY_FUNC(f_ge_int, {
    node->value.bool_ = node->left->value.int_ >= node->right->value.int_;
})

DEFINE_UNARY_FUNC(f_minus_int, {
    node->value.int_ = -node->left->value.int_;
})

DEFINE_UNARY_FUNC(f_cast_int_to_uint, {
    node->value.uint_ = node->left->value.int_;
})

DEFINE_UNARY_FUNC(f_cast_int_to_float, {
    node->value.float_ = node->left->value.int_;
})

DEFINE_UNARY_FUNC(f_cast_int_to_bool, {
    node->value.bool_ = node->left->value.int_ != 0;
})

DEFINE_BINARY_FUNC(f_add_float, {
    node->value.float_ = node->left->value.float_ + node->right->value.float_;
})

DEFINE_BINARY_FUNC(f_sub_float, {
    node->value.float_ = node->left->value.float_ - node->right->value.float_;
})

DEFINE_BINARY_FUNC(f_mul_float, {
    node->value.float_ = node->left->value.float_ * node->right->value.float_;
})

DEFINE_BINARY_FUNC(f_div_float, {
    node->value.float_ = node->left->value.float_ / node->right->value.float_;
})

DEFINE_BINARY_FUNC(f_eq_float, {
    node->value.bool_ = node->left->value.float_ == node->right->value.float_;
})

DEFINE_BINARY_FUNC(f_ne_float, {
    node->value.bool_ = node->left->value.float_ != node->right->value.float_;
})

DEFINE_BINARY_FUNC(f_lt_float, {
    node->value.bool_ = node->left->value.float_ < node->right->value.float_;
})

DEFINE_BINARY_FUNC(f_gt_float, {
    node->value.bool_ = node->left->value.float_ > node->right->value.float_;
})

DEFINE_BINARY_FUNC(f_le_float, {
    node->value.bool_ = node->left->value.float_ <= node->right->value.float_;
})

DEFINE_BINARY_FUNC(f_ge_float, {
    node->value.bool_ = node->left->value.float_ >= node->right->value.float_;
})

DEFINE_UNARY_FUNC(f_minus_float, {
    node->value.float_ = -node->left->value.float_;
})

DEFINE_UNARY_FUNC(f_cast_float_to_bool, {
    node->value.uint_ = node->left->value.float_ != 0.0f; // TODO: ?
})

DEFINE_BINARY_FUNC(f_concat_str, {
    char *new_string = realloc(node->value.string.chars,
        node->left->value.string.length + node->right->value.string.length);
    if (new_string == NULL) {
        no_memory_error(filter->error_list);
        longjmp(filter->eval_jmp_buf, FDS_FILTER_FAIL);
    }
    strncpy(new_string, node->left->value.string.chars, node->left->value.string.length);
    strncpy(new_string + node->left->value.string.length, node->right->value.string.chars,
        node->right->value.string.length);
    node->value.string.length = node->left->value.string.length + node->right->value.string.length;
    node->value.string.chars = new_string;
})

DEFINE_BINARY_FUNC(f_eq_str, {
    node->value.bool_ = (node->left->value.string.length == node->right->value.string.length
                         && strncmp(node->left->value.string.chars,
                                    node->right->value.string.chars,
                                    node->left->value.string.length) == 0);
})

DEFINE_BINARY_FUNC(f_ne_str, {
    node->value.bool_ = (node->left->value.string.length != node->right->value.string.length
                         || strncmp(node->left->value.string.chars,
                                    node->right->value.string.chars,
                                    node->left->value.string.length) != 0);
})

DEFINE_UNARY_FUNC(f_cast_str_to_bool, { node->value.uint_ = node->left->value.string.length > 0; })
// TODO: what to do with defined

// TODO: compare based on prefix_length
DEFINE_BINARY_FUNC(f_eq_ip_address, {
    node->value.bool_ = node->left->value.ip_address.version == node->right->value.ip_address.version
                        && bit_compare(node->left->value.ip_address.bytes,
                                node->right->value.ip_address.bytes,
                                MIN(node->left->value.ip_address.prefix_length,
                                    node->right->value.ip_address.prefix_length)
                            );
})

DEFINE_BINARY_FUNC(f_ne_ip_address, {
    node->value.bool_ = node->left->value.ip_address.version != node->right->value.ip_address.version
                        || !bit_compare(node->left->value.ip_address.bytes,
                                node->right->value.ip_address.bytes,
                                MIN(node->left->value.ip_address.prefix_length,
                                    node->right->value.ip_address.prefix_length)
                            );
})

DEFINE_BINARY_FUNC(f_eq_mac_address, {
    node->value.bool_ = memcmp(node->left->value.mac_address, node->right->value.mac_address, 6) == 0;
})

DEFINE_BINARY_FUNC(f_ne_mac_address, {
    node->value.bool_ = memcmp(node->left->value.mac_address, node->right->value.mac_address, 6) != 0;
})

#define LIST_IN_FUNC(FUNC_NAME, COMPARE_STATEMENT) \
    DEFINE_BINARY_FUNC(FUNC_NAME, { \
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
    && bit_compare(
        node->left->value.ip_address.bytes,
        node->right->value.list.items[i].ip_address.bytes,
        MIN(
            node->left->value.ip_address.prefix_length,
            node->right->value.list.items[i].ip_address.prefix_length
        )
    )
)

#define LIST_CAST_FUNC(FUNC_NAME, CAST_STATEMENT) \
    DEFINE_UNARY_FUNC(FUNC_NAME, { \
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

DEFINE_BINARY_FUNC(f_ip_address_in_trie, {
    node->value.bool_ =
        fds_trie_find(
            (fds_trie_t *)node->right->value.pointer,
            node->left->value.ip_address.version,
            node->left->value.ip_address.bytes,
            node->left->value.ip_address.prefix_length
        );
})

DEFINE_BINARY_FUNC(f_ip_address_not_in_trie, {
    node->value.bool_ =
        !fds_trie_find(
            (fds_trie_t *)node->right->value.pointer,
            node->left->value.ip_address.version,
            node->left->value.ip_address.bytes,
            node->left->value.ip_address.prefix_length
        );
})

DEFINE_FUNC(f_and, {
    node->left->evaluate(filter, node->left);
    if (node->left->value.uint_ == 0) {
        node->value.uint_ = 0;
        return;
    }
    node->right->evaluate(filter, node->right);
    node->value.uint_ = node->right->value.uint_;
})

DEFINE_FUNC(f_or, {
    node->left->evaluate(filter, node->left);
    if (node->left->value.uint_ != 0) {
        node->value.uint_ = 1;
        return;
    }
    node->right->evaluate(filter, node->right);
    node->value.uint_ = node->right->value.uint_;
})

DEFINE_FUNC(f_not, {
    node->left->evaluate(filter, node->left);
    node->value.uint_ = node->left->value.uint_ == 0;
})

DEFINE_FUNC(f_const, {
    // Do nothing
    (void)filter;
    (void)node;
})

DEFINE_FUNC(f_identifier, {
    int return_code = filter->field_callback(node->identifier_id, filter->user_context,
        filter->reset_context, filter->data, &node->value);
    assert(return_code == FDS_FILTER_OK || return_code == FDS_FILTER_OK_MORE
        || return_code == FDS_FILTER_FAIL);
    node->is_defined = (return_code == FDS_FILTER_OK || return_code == FDS_FILTER_OK_MORE);
    node->is_more = (return_code == FDS_FILTER_OK_MORE);
    filter->reset_context = 0; // If the context should reset this is set to 1 by one of the parent nodes.
})

DEFINE_FUNC(f_any, {
    do {
        node->left->evaluate(filter, node->left);
    } while (node->left->is_defined && node->left->value.uint_ == 0 && node->left->is_more);
    node->value.uint_ = node->left->is_defined && node->left->value.uint_;
    node->is_more = false;
    node->is_defined = true;
    filter->reset_context = 1;
})

DEFINE_FUNC(f_exists, {
    node->left->evaluate(filter, node->left);
    node->is_defined = true;
    node->is_more = false;
    node->value.uint_ = node->is_defined;
})

// int or uint should not matter in case of bit operations (?)
DEFINE_BINARY_FUNC(f_bitand, {
    node->value.uint_ = node->left->value.uint_ & node->right->value.uint_;
})
DEFINE_BINARY_FUNC(f_bitor, {
    node->value.uint_ = node->left->value.uint_ | node->right->value.uint_;
})
DEFINE_BINARY_FUNC(f_bitxor, {
    node->value.uint_ = node->left->value.uint_ ^ node->right->value.uint_;
})
DEFINE_UNARY_FUNC(f_bitnot, {
    node->value.uint_ = ~node->left->value.uint_;
})
DEFINE_UNARY_FUNC(f_flagcmp, {
    node->value.bool_ = (node->left->value.uint_ & node->right->value.uint_) ? 1 : 0;
})


#undef DEFINE_FUNC
#undef DEFINE_BINARY_FUNC
#undef DEFINE_UNARY_FUNC
#undef MIN