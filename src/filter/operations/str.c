#include "str.h"
#include "../values.h"

static inline bool
str_equals(fds_filter_str_t *left, fds_filter_str_t *right)
{
    return left->len == right->len && memcmp(left->chars, right->chars, right->len) == 0;
}

void
eq_str(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = str_equals(&left->str, &right->str);
}

void
ne_str(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = !str_equals(&left->str, &right->str);
}

void
contains_str(fds_filter_value_u *big, fds_filter_value_u *little, fds_filter_value_u *result)
{
    result->b = false;
    for (int i = 0; i <= big->str.len - little->str.len; i++) {
        if (memcmp(big->str.chars + i, little->str.chars, little->str.len) == 0) {
            result->b = true;
            return;
        }
    }
}

void
str_in_list(fds_filter_value_u *item, fds_filter_value_u *list, fds_filter_value_u *result)
{
    result->b = false;
    for (int i = 0; i < list->list.len; i++) {
        if (str_equals(&item->str, &list->list.items[i].str)) {
            result->b = true;
            return;
        }
    }
}

void
destroy_str_(fds_filter_value_u *operand)
{
    free(operand->str.chars);
}

void
destroy_list_of_str(fds_filter_value_u *operand)
{
    for (int i = 0; i < operand->list.len; i++) {
        free(operand->list.items[i].str.chars);
    }
    free(operand->list.items);
}

void
cast_str_list_to_bool(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->b = operand->list.len > 0;
}

void
cast_str_to_bool(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->b = operand->str.len > 0;
}

const fds_filter_op_s str_operations[] = {
    FDS_FILTER_DEF_BINARY_OP(DT_STR, "", DT_STR, eq_str, DT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(DT_STR, "==", DT_STR, eq_str, DT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(DT_STR, "!=", DT_STR, ne_str, DT_BOOL),

    FDS_FILTER_DEF_BINARY_OP(DT_STR, "contains", DT_STR, contains_str, DT_BOOL),

    FDS_FILTER_DEF_BINARY_OP(DT_STR, "in", DT_LIST | DT_STR, str_in_list, DT_BOOL),

    FDS_FILTER_DEF_CAST(DT_STR, cast_str_to_bool, DT_BOOL),
    FDS_FILTER_DEF_CAST(DT_LIST | DT_STR, cast_str_list_to_bool, DT_BOOL),

    FDS_FILTER_DEF_DESTRUCTOR(DT_STR, destroy_str_),
    FDS_FILTER_DEF_DESTRUCTOR(DT_STR | DT_LIST, destroy_list_of_str),

    FDS_FILTER_END_OP_LIST
};
