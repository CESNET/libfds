#include "uint.h"
#include "../common.h"
#include "../values.h"

void
add_uint(value_t *left, value_t *right, value_t *result)
{
    result->u = left->u + right->u;
}

void
sub_uint(value_t *left, value_t *right, value_t *result)
{
    result->u = left->u - right->u;
}

void
mul_uint(value_t *left, value_t *right, value_t *result)
{
    result->u = left->u * right->u;
}

void
div_uint(value_t *left, value_t *right, value_t *result)
{
    result->u = left->u / right->u;
}

void
neg_uint(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->i = -operand->u;
}

void
mod_uint(value_t *left, value_t *right, value_t *result)
{
    result->u = left->u % right->u;
}

void
bitnot_uint(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->u = ~operand->u;
}

void
bitor_uint(value_t *left, value_t *right, value_t *result)
{
    result->u = left->u | right->u;
}

void
bitand_uint(value_t *left, value_t *right, value_t *result)
{
    result->u = left->u & right->u;
}

void
bitxor_uint(value_t *left, value_t *right, value_t *result)
{
    result->u = left->u ^ right->u;
}

void
cast_float_to_uint(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->u = operand->f;
}

void
eq_uint(value_t *left, value_t *right, value_t *result)
{
    result->b = left->u == right->u;
}

void
ne_uint(value_t *left, value_t *right, value_t *result)
{
    result->b = left->u != right->u;
}

void
lt_uint(value_t *left, value_t *right, value_t *result)
{
    result->b = left->u < right->u;
}

void
gt_uint(value_t *left, value_t *right, value_t *result)
{
    result->b = left->u > right->u;
}

void
le_uint(value_t *left, value_t *right, value_t *result)
{
    result->b = left->u <= right->u;
}

void
ge_uint(value_t *left, value_t *right, value_t *result)
{
    result->b = left->u >= right->u;
}

void
uint_in_list(value_t *item, value_t *list, value_t *result)
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
cast_uint_to_bool(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->b = operand->u != 0;
}

void
cast_uint_list_to_bool(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->b = operand->list.len > 0;
}

void
cast_empty_list_to_uint_list(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->list = operand->list;
}

void
destroy_uint_list(value_t *operand, value_t *_1, value_t *_2)
{
    UNUSED(_1);
    UNUSED(_2);
    free(operand->list.items);
}

const operation_s uint_operations[] = {
    DEFINE_PREFIX_OP("-", DT_UINT, neg_uint, DT_INT),
    DEFINE_INFIX_OP(DT_UINT, "+", DT_UINT, add_uint, DT_UINT),
    DEFINE_INFIX_OP(DT_UINT, "-", DT_UINT, sub_uint, DT_UINT),
    DEFINE_INFIX_OP(DT_UINT, "*", DT_UINT, mul_uint, DT_UINT),
    DEFINE_INFIX_OP(DT_UINT, "/", DT_UINT, div_uint, DT_UINT),
    DEFINE_INFIX_OP(DT_UINT, "%", DT_UINT, mod_uint, DT_UINT),

    DEFINE_PREFIX_OP("~", DT_UINT, bitnot_uint, DT_UINT),
    DEFINE_INFIX_OP(DT_UINT, "|", DT_UINT, bitor_uint, DT_UINT),
    DEFINE_INFIX_OP(DT_UINT, "&", DT_UINT, bitand_uint, DT_UINT),
    DEFINE_INFIX_OP(DT_UINT, "^", DT_UINT, bitxor_uint, DT_UINT),

    DEFINE_INFIX_OP(DT_UINT, "", DT_UINT, eq_uint, DT_BOOL),
    DEFINE_INFIX_OP(DT_UINT, "==", DT_UINT, eq_uint, DT_BOOL),
    DEFINE_INFIX_OP(DT_UINT, "!=", DT_UINT, ne_uint, DT_BOOL),
    DEFINE_INFIX_OP(DT_UINT, "<", DT_UINT, lt_uint, DT_BOOL),
    DEFINE_INFIX_OP(DT_UINT, ">", DT_UINT, gt_uint, DT_BOOL),
    DEFINE_INFIX_OP(DT_UINT, "<=", DT_UINT, le_uint, DT_BOOL),
    DEFINE_INFIX_OP(DT_UINT, ">=", DT_UINT, ge_uint, DT_BOOL),

    DEFINE_INFIX_OP(DT_UINT, "in", DT_LIST | DT_UINT, uint_in_list, DT_BOOL),

    DEFINE_CAST_OP(DT_FLOAT, cast_float_to_uint, DT_UINT),
    DEFINE_CAST_OP(DT_UINT, cast_uint_to_bool, DT_BOOL),
    DEFINE_CAST_OP(DT_LIST | DT_NONE, cast_empty_list_to_uint_list, DT_LIST | DT_UINT),

    DEFINE_DESTRUCTOR(DT_UINT | DT_LIST, destroy_uint_list),
};
const int num_uint_operations = CONST_ARR_SIZE(uint_operations);