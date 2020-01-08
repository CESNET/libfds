#include "uint.h"
#include "../common.h"
#include "../values.h"

void
add_uint(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->u = left->u + right->u;
}

void
sub_uint(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->u = left->u - right->u;
}

void
mul_uint(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->u = left->u * right->u;
}

void
div_uint(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->u = left->u / right->u;
}

void
neg_uint(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->i = -operand->u;
}

void
mod_uint(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->u = left->u % right->u;
}

void
bitnot_uint(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->u = ~operand->u;
}

void
bitor_uint(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->u = left->u | right->u;
}

void
bitand_uint(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->u = left->u & right->u;
}

void
bitxor_uint(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->u = left->u ^ right->u;
}

void
cast_float_to_uint(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->u = operand->f;
}

void
eq_uint(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = left->u == right->u;
}

void
ne_uint(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = left->u != right->u;
}

void
lt_uint(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = left->u < right->u;
}

void
gt_uint(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = left->u > right->u;
}

void
le_uint(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = left->u <= right->u;
}

void
ge_uint(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = left->u >= right->u;
}

void
uint_in_list(fds_filter_value_u *item, fds_filter_value_u *list, fds_filter_value_u *result)
{
    result->b = false;
    for (int i = 0; i < list->list.len; i++) {
        if (list->list.items[i].u == item->u) {
            result->b = true;
            return;
        }
    }
}

void
cast_uint_to_bool(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->b = operand->u != 0;
}

void
cast_uint_list_to_bool(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->b = operand->list.len > 0;
}

void
cast_empty_list_to_uint_list(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->list = operand->list;
}

void
destroy_uint_list(fds_filter_value_u *operand)
{
    free(operand->list.items);
}

const fds_filter_op_s uint_operations[] = {
    FDS_FILTER_DEF_UNARY_OP("-", DT_UINT, neg_uint, DT_INT),
    FDS_FILTER_DEF_BINARY_OP(DT_UINT, "+", DT_UINT, add_uint, DT_UINT),
    FDS_FILTER_DEF_BINARY_OP(DT_UINT, "-", DT_UINT, sub_uint, DT_UINT),
    FDS_FILTER_DEF_BINARY_OP(DT_UINT, "*", DT_UINT, mul_uint, DT_UINT),
    FDS_FILTER_DEF_BINARY_OP(DT_UINT, "/", DT_UINT, div_uint, DT_UINT),
    FDS_FILTER_DEF_BINARY_OP(DT_UINT, "%", DT_UINT, mod_uint, DT_UINT),

    FDS_FILTER_DEF_UNARY_OP("~", DT_UINT, bitnot_uint, DT_UINT),
    FDS_FILTER_DEF_BINARY_OP(DT_UINT, "|", DT_UINT, bitor_uint, DT_UINT),
    FDS_FILTER_DEF_BINARY_OP(DT_UINT, "&", DT_UINT, bitand_uint, DT_UINT),
    FDS_FILTER_DEF_BINARY_OP(DT_UINT, "^", DT_UINT, bitxor_uint, DT_UINT),

    FDS_FILTER_DEF_BINARY_OP(DT_UINT, "", DT_UINT, eq_uint, DT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(DT_UINT, "==", DT_UINT, eq_uint, DT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(DT_UINT, "!=", DT_UINT, ne_uint, DT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(DT_UINT, "<", DT_UINT, lt_uint, DT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(DT_UINT, ">", DT_UINT, gt_uint, DT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(DT_UINT, "<=", DT_UINT, le_uint, DT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(DT_UINT, ">=", DT_UINT, ge_uint, DT_BOOL),

    FDS_FILTER_DEF_BINARY_OP(DT_UINT, "in", DT_LIST | DT_UINT, uint_in_list, DT_BOOL),

    FDS_FILTER_DEF_CAST(DT_FLOAT, cast_float_to_uint, DT_UINT),
    FDS_FILTER_DEF_CAST(DT_UINT, cast_uint_to_bool, DT_BOOL),
    FDS_FILTER_DEF_CAST(DT_LIST | DT_NONE, cast_empty_list_to_uint_list, DT_LIST | DT_UINT),

    FDS_FILTER_DEF_DESTRUCTOR(DT_UINT | DT_LIST, destroy_uint_list),
};
const int num_uint_operations = CONST_ARR_SIZE(uint_operations);