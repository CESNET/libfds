#include "float.h"
#include "../values.h"

#define FLOAT_EQUALS_EPSILON   0.001 // Precision to 3 decimal point places when comparing floats for equality

static inline bool
float_equals(fds_filter_float_t a, fds_filter_float_t b)
{
    return fabs(a - b) < FLOAT_EQUALS_EPSILON;
}

void
add_float(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->f = left->f + right->f;
}

void
sub_float(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->f = left->f - right->f;
}

void
mul_float(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->f = left->f * right->f;
}

void
div_float(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->f = left->f / right->f;
}

void
neg_float(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->f = -operand->f;
}

void
cast_int_to_float(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->f = operand->i;
}

void
cast_uint_to_float(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->f = operand->u;
}

void
cast_float_to_bool(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->b = operand->f != 0.0; // ???
}

void
eq_float(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = float_equals(left->f, right->f);
}

void
ne_float(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = !float_equals(left->f, right->f);
}

void
lt_float(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = left->f < right->f;
}

void
gt_float(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = left->f > right->f;
}

void
le_float(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = left->f <= right->f;
}

void
ge_float(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = left->f >= right->f;
}

void
float_in_list(fds_filter_value_u *item, fds_filter_value_u *list, fds_filter_value_u *result)
{
    result->b = false;
    for (int i = 0; i < list->list.len; i++) {
        if (list->list.items[i].f == item->f) {
            result->b = true;
            return;
        }
    }
}

void
cast_float_list_to_bool(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->b = operand->list.len > 0;
}

void
destroy_float_list(fds_filter_value_u *operand)
{
    free(operand->list.items);
}

const fds_filter_op_s float_operations[] = {
    FDS_FILTER_DEF_UNARY_OP("-", DT_FLOAT, neg_float, DT_FLOAT),
    FDS_FILTER_DEF_BINARY_OP(DT_FLOAT, "+", DT_FLOAT, add_float, DT_FLOAT),
    FDS_FILTER_DEF_BINARY_OP(DT_FLOAT, "-", DT_FLOAT, sub_float, DT_FLOAT),
    FDS_FILTER_DEF_BINARY_OP(DT_FLOAT, "*", DT_FLOAT, mul_float, DT_FLOAT),
    FDS_FILTER_DEF_BINARY_OP(DT_FLOAT, "/", DT_FLOAT, div_float, DT_FLOAT),

    FDS_FILTER_DEF_BINARY_OP(DT_FLOAT, "", DT_FLOAT, eq_float, DT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(DT_FLOAT, "==", DT_FLOAT, eq_float, DT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(DT_FLOAT, "!=", DT_FLOAT, ne_float, DT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(DT_FLOAT, "<", DT_FLOAT, lt_float, DT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(DT_FLOAT, ">", DT_FLOAT, gt_float, DT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(DT_FLOAT, "<=", DT_FLOAT, le_float, DT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(DT_FLOAT, ">=", DT_FLOAT, ge_float, DT_BOOL),

    FDS_FILTER_DEF_BINARY_OP(DT_FLOAT, "in", DT_LIST | DT_FLOAT, float_in_list, DT_BOOL),

    FDS_FILTER_DEF_CAST(DT_INT, cast_int_to_float, DT_FLOAT),
    FDS_FILTER_DEF_CAST(DT_FLOAT, cast_float_to_bool, DT_BOOL),
    FDS_FILTER_DEF_CAST(DT_FLOAT | DT_LIST, cast_float_list_to_bool, DT_BOOL),

    FDS_FILTER_DEF_DESTRUCTOR(DT_FLOAT | DT_LIST, destroy_float_list),

    FDS_FILTER_END_OP_LIST
};

