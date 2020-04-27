#include "ip.h"
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
eq_ip(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = ip_prefix_equals(&left->ip, &right->ip);
}

void
ne_ip(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = !ip_prefix_equals(&left->ip, &right->ip);
}

void
ip_in_list(fds_filter_value_u *item, fds_filter_value_u *list, fds_filter_value_u *result)
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
cast_ip_to_bool(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->b = operand->ip.version != 0; // ???
}

void
cast_ip_list_to_bool(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->b = operand->list.len > 0;
}

void
destroy_ip_list(fds_filter_value_u *operand)
{
    free(operand->list.items);
}

const fds_filter_op_s ip_operations[] = {
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_IP, "", FDS_FDT_IP, eq_ip, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_IP, "==", FDS_FDT_IP, eq_ip, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_IP, "!=", FDS_FDT_IP, ne_ip, FDS_FDT_BOOL),

    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_IP, "in", FDS_FDT_LIST | FDS_FDT_IP, ip_in_list, FDS_FDT_BOOL),

    FDS_FILTER_DEF_CAST(FDS_FDT_IP, cast_ip_to_bool, FDS_FDT_BOOL),
    FDS_FILTER_DEF_CAST(FDS_FDT_IP | FDS_FDT_LIST, cast_ip_list_to_bool, FDS_FDT_BOOL),

    FDS_FILTER_DEF_DESTRUCTOR(FDS_FDT_IP | FDS_FDT_LIST, destroy_ip_list),

    FDS_FILTER_END_OP_LIST
};
