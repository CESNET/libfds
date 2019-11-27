#include "mac.h"
#include "../common.h"
#include "../values.h"

static inline bool
mac_equals(fds_filter_mac_t *left, fds_filter_mac_t *right)
{
    return memcmp(left->addr, right->addr, 6) == 0;
}

void
eq_mac(value_t *left, value_t *right, value_t *result)
{
    result->b = mac_equals(&left->mac, &right->mac);
}

void
ne_mac(value_t *left, value_t *right, value_t *result)
{
    result->b = !mac_equals(&left->mac, &right->mac);
}

void
mac_in_list(value_t *item, value_t *list, value_t *result)
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
cast_mac_list_to_bool(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->b = operand->list.len > 0;
}

void
cast_mac_to_bool(value_t *_1, value_t *_2, value_t *result)
{
    UNUSED(_1);
    UNUSED(_2);
    result->b = true; // ??
}

void
cast_empty_list_to_mac_list(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->list = operand->list;
}

void
destroy_mac_list(value_t *operand, value_t *_1, value_t *_2)
{
    UNUSED(_1);
    UNUSED(_2);
    free(operand->list.items);
}

const operation_s mac_operations[] = {
    DEFINE_INFIX_OP(DT_MAC, "", DT_MAC, eq_mac, DT_BOOL),
    DEFINE_INFIX_OP(DT_MAC, "==", DT_MAC, eq_mac, DT_BOOL),
    DEFINE_INFIX_OP(DT_MAC, "!=", DT_MAC, ne_mac, DT_BOOL),

    DEFINE_INFIX_OP(DT_MAC, "in", DT_LIST | DT_MAC, mac_in_list, DT_BOOL),

    DEFINE_CAST_OP(DT_MAC, cast_mac_to_bool, DT_BOOL),
    DEFINE_CAST_OP(DT_LIST | DT_MAC, cast_mac_list_to_bool, DT_BOOL),
    DEFINE_CAST_OP(DT_LIST | DT_NONE, cast_empty_list_to_mac_list, DT_LIST | DT_MAC),

    DEFINE_DESTRUCTOR(DT_MAC | DT_LIST, destroy_mac_list),
};

const int num_mac_operations = CONST_ARR_SIZE(mac_operations);