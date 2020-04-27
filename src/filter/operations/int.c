#include "int.h"
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
cast_int_to_uint(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->u = operand->i;
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
destroy_int_list(fds_filter_value_u *operand)
{
    free(operand->list.items);
}

extern const fds_filter_op_s int_operations[] = {
    FDS_FILTER_DEF_UNARY_OP("-", FDS_FDT_INT, neg_int, FDS_FDT_INT),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_INT, "+", FDS_FDT_INT, add_int, FDS_FDT_INT),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_INT, "-", FDS_FDT_INT, sub_int, FDS_FDT_INT),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_INT, "*", FDS_FDT_INT, mul_int, FDS_FDT_INT),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_INT, "/", FDS_FDT_INT, div_int, FDS_FDT_INT),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_INT, "%", FDS_FDT_INT, mod_int, FDS_FDT_INT),

    FDS_FILTER_DEF_UNARY_OP("~", FDS_FDT_INT, bitnot_int, FDS_FDT_INT),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_INT, "|", FDS_FDT_INT, bitor_int, FDS_FDT_INT),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_INT, "&", FDS_FDT_INT, bitand_int, FDS_FDT_INT),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_INT, "^", FDS_FDT_INT, bitxor_int, FDS_FDT_INT),

    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_INT, "", FDS_FDT_INT, eq_int, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_INT, "==", FDS_FDT_INT, eq_int, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_INT, "!=", FDS_FDT_INT, ne_int, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_INT, "<", FDS_FDT_INT, lt_int, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_INT, ">", FDS_FDT_INT, gt_int, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_INT, "<=", FDS_FDT_INT, le_int, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_INT, ">=", FDS_FDT_INT, ge_int, FDS_FDT_BOOL),

    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_INT, "in", FDS_FDT_LIST | FDS_FDT_INT, int_in_list, FDS_FDT_BOOL),

    FDS_FILTER_DEF_CAST(FDS_FDT_FLOAT, cast_float_to_int, FDS_FDT_INT),
    FDS_FILTER_DEF_CAST(FDS_FDT_UINT, cast_uint_to_int, FDS_FDT_INT),
    FDS_FILTER_DEF_CAST(FDS_FDT_INT, cast_int_to_uint, FDS_FDT_UINT),
    FDS_FILTER_DEF_CAST(FDS_FDT_INT, cast_int_to_bool, FDS_FDT_BOOL),
    FDS_FILTER_DEF_CAST(FDS_FDT_INT | FDS_FDT_LIST, cast_int_list_to_bool, FDS_FDT_BOOL),

    FDS_FILTER_DEF_DESTRUCTOR(FDS_FDT_INT | FDS_FDT_LIST, destroy_int_list),
    
    FDS_FILTER_END_OP_LIST
};
