#include "int.h"
#include "../common.h"
#include "../values.h"

void
add_int(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->i = left->i + right->i;
}

void
sub_int(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->i = left->i - right->i;
}

void
mul_int(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->i = left->i * right->i;
}

void
div_int(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->i = left->i / right->i;
}

void
neg_int(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->i = -operand->i;
}

void
mod_int(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->i = left->i % right->i;
}

void
bitnot_int(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->i = ~operand->i;
}

void
bitor_int(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->i = left->i | right->i;
}

void
bitand_int(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->i = left->i & right->i;
}

void
bitxor_int(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->i = left->i ^ right->i;
}

void
cast_float_to_int(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->i = operand->f;
}

void
cast_uint_to_int(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->i = operand->u;
}

void
cast_int_to_bool(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->b = operand->i != 0;
}

void
eq_int(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = left->i == right->i;
}

void
ne_int(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = left->i != right->i;
}

void
lt_int(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = left->i < right->i;
}

void
gt_int(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = left->i > right->i;
}

void
le_int(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = left->i <= right->i;
}

void
ge_int(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = left->i >= right->i;
}

void
int_in_list(fds_filter_value_u *item, fds_filter_value_u *list, fds_filter_value_u *result)
{
    result->b = false;
    for (int i = 0; i < list->list.len; i++) {
        if (list->list.items[i].i == item->i) {
            result->b = true;
            return;
        }
    }
}

void
cast_int_list_to_bool(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->b = operand->list.len > 0;
}

void
cast_empty_list_to_int_list(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->list = operand->list;
}

void
destroy_int_list(fds_filter_value_u *operand)
{
    free(operand->list.items);
}

extern const fds_filter_op_s int_operations[] = {
    FDS_FILTER_DEF_UNARY_OP("-", DT_INT, neg_int, DT_INT),
    FDS_FILTER_DEF_BINARY_OP(DT_INT, "+", DT_INT, add_int, DT_INT),
    FDS_FILTER_DEF_BINARY_OP(DT_INT, "-", DT_INT, sub_int, DT_INT),
    FDS_FILTER_DEF_BINARY_OP(DT_INT, "*", DT_INT, mul_int, DT_INT),
    FDS_FILTER_DEF_BINARY_OP(DT_INT, "/", DT_INT, div_int, DT_INT),
    FDS_FILTER_DEF_BINARY_OP(DT_INT, "%", DT_INT, mod_int, DT_INT),

    FDS_FILTER_DEF_UNARY_OP("~", DT_INT, bitnot_int, DT_INT),
    FDS_FILTER_DEF_BINARY_OP(DT_INT, "|", DT_INT, bitor_int, DT_INT),
    FDS_FILTER_DEF_BINARY_OP(DT_INT, "&", DT_INT, bitand_int, DT_INT),
    FDS_FILTER_DEF_BINARY_OP(DT_INT, "^", DT_INT, bitxor_int, DT_INT),

    FDS_FILTER_DEF_BINARY_OP(DT_INT, "", DT_INT, eq_int, DT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(DT_INT, "==", DT_INT, eq_int, DT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(DT_INT, "!=", DT_INT, ne_int, DT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(DT_INT, "<", DT_INT, lt_int, DT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(DT_INT, ">", DT_INT, gt_int, DT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(DT_INT, "<=", DT_INT, le_int, DT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(DT_INT, ">=", DT_INT, ge_int, DT_BOOL),

    FDS_FILTER_DEF_BINARY_OP(DT_INT, "in", DT_LIST | DT_INT, int_in_list, DT_BOOL),

    FDS_FILTER_DEF_CAST(DT_FLOAT, cast_float_to_int, DT_INT),
    FDS_FILTER_DEF_CAST(DT_UINT, cast_uint_to_int, DT_INT),
    FDS_FILTER_DEF_CAST(DT_INT, cast_int_to_bool, DT_BOOL),
    FDS_FILTER_DEF_CAST(DT_INT | DT_LIST, cast_int_list_to_bool, DT_BOOL),
    FDS_FILTER_DEF_CAST(DT_NONE | DT_LIST, cast_empty_list_to_int_list, DT_INT | DT_LIST),

    FDS_FILTER_DEF_DESTRUCTOR(DT_INT | DT_LIST, destroy_int_list),

};
const int num_int_operations = CONST_ARR_SIZE(int_operations);