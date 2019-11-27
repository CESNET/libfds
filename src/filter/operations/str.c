#include "str.h"
#include "../common.h"
#include "../values.h"

static inline bool
str_equals(fds_filter_str_t *left, fds_filter_str_t *right)
{
    return left->len == right->len && memcmp(left->chars, right->chars, right->len) == 0;
}

void
eq_str(value_t *left, value_t *right, value_t *result)
{
    result->b = str_equals(&left->str, &right->str);
}

void
ne_str(value_t *left, value_t *right, value_t *result)
{
    result->b = !str_equals(&left->str, &right->str);
}

void
contains_str(value_t *big, value_t *little, value_t *result)
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
str_in_list(value_t *item, value_t *list, value_t *result)
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
destroy_str_(value_t *operand, value_t *_1, value_t *_2)
{
    UNUSED(_1);
    UNUSED(_2);
    free(operand->str.chars);
}

void
destroy_list_of_str(value_t *operand, value_t *_1, value_t *_2)
{
    UNUSED(_1);
    UNUSED(_2);
    for (int i = 0; i < operand->list.len; i++) {
        free(operand->list.items[i].str.chars);
    }
    free(operand->list.items);
}

void
cast_str_list_to_bool(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->b = operand->list.len > 0;
}

void
cast_str_to_bool(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->b = operand->str.len > 0;
}

void
cast_empty_list_to_str_list(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->list = operand->list;
}


const operation_s str_operations[] = {
    DEFINE_INFIX_OP(DT_STR, "", DT_STR, eq_str, DT_BOOL),
    DEFINE_INFIX_OP(DT_STR, "==", DT_STR, eq_str, DT_BOOL),
    DEFINE_INFIX_OP(DT_STR, "!=", DT_STR, ne_str, DT_BOOL),

    DEFINE_INFIX_OP(DT_STR, "contains", DT_STR, contains_str, DT_BOOL),

    DEFINE_INFIX_OP(DT_STR, "in", DT_LIST | DT_STR, str_in_list, DT_BOOL),

    DEFINE_CAST_OP(DT_STR, cast_str_to_bool, DT_BOOL),
    DEFINE_CAST_OP(DT_LIST | DT_STR, cast_str_list_to_bool, DT_BOOL),
    DEFINE_CAST_OP(DT_LIST | DT_NONE, cast_empty_list_to_str_list, DT_LIST | DT_STR),

    DEFINE_DESTRUCTOR(DT_STR, destroy_str_),
    DEFINE_DESTRUCTOR(DT_STR | DT_LIST, destroy_list_of_str),
};

const int num_str_operations = CONST_ARR_SIZE(str_operations);