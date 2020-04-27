#include "mac.h"
#include "../values.h"

static inline bool
mac_equals(fds_filter_mac_t *left, fds_filter_mac_t *right)
{
    return memcmp(left->addr, right->addr, 6) == 0;
}

void
eq_mac(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = mac_equals(&left->mac, &right->mac);
}

void
ne_mac(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = !mac_equals(&left->mac, &right->mac);
}

void
mac_in_list(fds_filter_value_u *item, fds_filter_value_u *list, fds_filter_value_u *result)
{
    result->b = false;
    for (int i = 0; i < list->list.len; i++) {
        if (mac_equals(&list->list.items[i].mac, &item->mac)) {
            result->b = true;
            return;
        }
    }
}

void
cast_mac_list_to_bool(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->b = operand->list.len > 0;
}

void
cast_mac_to_bool(fds_filter_value_u *_1, fds_filter_value_u *_2, fds_filter_value_u *result)
{
    result->b = true; // ??
}

void
destroy_mac_list(fds_filter_value_u *operand)
{
    free(operand->list.items);
}

const fds_filter_op_s mac_operations[] = {
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_MAC, "", FDS_FDT_MAC, eq_mac, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_MAC, "==", FDS_FDT_MAC, eq_mac, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_MAC, "!=", FDS_FDT_MAC, ne_mac, FDS_FDT_BOOL),

    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_MAC, "in", FDS_FDT_LIST | FDS_FDT_MAC, mac_in_list, FDS_FDT_BOOL),

    FDS_FILTER_DEF_CAST(FDS_FDT_MAC, cast_mac_to_bool, FDS_FDT_BOOL),
    FDS_FILTER_DEF_CAST(FDS_FDT_LIST | FDS_FDT_MAC, cast_mac_list_to_bool, FDS_FDT_BOOL),

    FDS_FILTER_DEF_DESTRUCTOR(FDS_FDT_MAC | FDS_FDT_LIST, destroy_mac_list),

    FDS_FILTER_END_OP_LIST
};
