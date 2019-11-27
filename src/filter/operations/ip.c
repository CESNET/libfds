#include "ip.h"
#include "../common.h"
#include "../values.h"

static inline bool
ip_prefix_equals(fds_filter_ip_t *left, fds_filter_ip_t *right)
{
    if (left->version != right->version) {
        return false;
    }

    int n_cmp_bits = left->prefix <= right->prefix ? left->prefix : right->prefix;
    if (memcmp(left->addr, right->addr, n_cmp_bits / 8) != 0) {
        return false;
    }
    if (n_cmp_bits % 8 == 0) {
        return true;
    }
    uint8_t last_byte_left = left->addr[n_cmp_bits / 8] >> (8 - (n_cmp_bits % 8));
    uint8_t last_byte_right = right->addr[n_cmp_bits / 8] >> (8 - (n_cmp_bits % 8));
    return last_byte_left == last_byte_right;
}

void
eq_ip(value_t *left, value_t *right, value_t *result)
{
    result->b = ip_prefix_equals(&left->ip, &right->ip);
}

void
ne_ip(value_t *left, value_t *right, value_t *result)
{
    result->b = !ip_prefix_equals(&left->ip, &right->ip);
}

void
ip_in_list(value_t *item, value_t *list, value_t *result)
{
    result->b = false;
    for (int i = 0; i < list->list.len; i++) {
        if (ip_prefix_equals(&list->list.items[i].ip, &item->ip)) {
            result->b = true;
            return;
        }
    }
}

void
cast_ip_to_bool(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->b = true; // ??
}

void
cast_ip_list_to_bool(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->b = operand->list.len > 0;
}

void
cast_empty_list_to_ip_list(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->list = operand->list;
}

void
destroy_ip_list(value_t *operand, value_t *_1, value_t *_2)
{
    UNUSED(_1);
    UNUSED(_2);
    free(operand->list.items);
}

const operation_s ip_operations[] = {
    DEFINE_INFIX_OP(DT_IP, "", DT_IP, eq_ip, DT_BOOL),
    DEFINE_INFIX_OP(DT_IP, "==", DT_IP, eq_ip, DT_BOOL),
    DEFINE_INFIX_OP(DT_IP, "!=", DT_IP, ne_ip, DT_BOOL),

    DEFINE_INFIX_OP(DT_IP, "in", DT_LIST | DT_IP, ip_in_list, DT_BOOL),

    DEFINE_CAST_OP(DT_IP, cast_ip_to_bool, DT_BOOL),
    DEFINE_CAST_OP(DT_IP | DT_LIST, cast_ip_list_to_bool, DT_BOOL),
    DEFINE_CAST_OP(DT_NONE | DT_LIST, cast_empty_list_to_ip_list, DT_IP | DT_LIST),

    DEFINE_DESTRUCTOR(DT_IP | DT_LIST, destroy_ip_list),
};
const int num_ip_operations = CONST_ARR_SIZE(ip_operations);