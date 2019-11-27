#include "int.h"
#include "../common.h"
#include "../values.h"

void
add_int(value_t *left, value_t *right, value_t *result)
{
    result->i = left->i + right->i;
}

void
sub_int(value_t *left, value_t *right, value_t *result)
{
    result->i = left->i - right->i;
}

void
mul_int(value_t *left, value_t *right, value_t *result)
{
    result->i = left->i * right->i;
}

void
div_int(value_t *left, value_t *right, value_t *result)
{
    result->i = left->i / right->i;
}

void
neg_int(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->i = -operand->i;
}

void
mod_int(value_t *left, value_t *right, value_t *result)
{
    result->i = left->i % right->i;
}

void
bitnot_int(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->i = ~operand->i;
}

void
bitor_int(value_t *left, value_t *right, value_t *result)
{
    result->i = left->i | right->i;
}

void
bitand_int(value_t *left, value_t *right, value_t *result)
{
    result->i = left->i & right->i;
}

void
bitxor_int(value_t *left, value_t *right, value_t *result)
{
    result->i = left->i ^ right->i;
}

void
cast_float_to_int(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->i = operand->f;
}

void
cast_uint_to_int(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->i = operand->u;
}

void
cast_int_to_bool(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->b = operand->i != 0;
}

void
eq_int(value_t *left, value_t *right, value_t *result)
{
    result->b = left->i == right->i;
}

void
ne_int(value_t *left, value_t *right, value_t *result)
{
    result->b = left->i != right->i;
}

void
lt_int(value_t *left, value_t *right, value_t *result)
{
    result->b = left->i < right->i;
}

void
gt_int(value_t *left, value_t *right, value_t *result)
{
    result->b = left->i > right->i;
}

void
le_int(value_t *left, value_t *right, value_t *result)
{
    result->b = left->i <= right->i;
}

void
ge_int(value_t *left, value_t *right, value_t *result)
{
    result->b = left->i >= right->i;
}

void
int_in_list(value_t *item, value_t *list, value_t *result)
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
cast_int_list_to_bool(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->b = operand->list.len > 0;
}

void
cast_empty_list_to_int_list(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->list = operand->list;
}

void
destroy_int_list(value_t *operand, value_t *_1, value_t *_2)
{
    UNUSED(_1);
    UNUSED(_2);
    free(operand->list.items);
}

extern const operation_s int_operations[] = {
    DEFINE_PREFIX_OP("-", DT_INT, neg_int, DT_INT),
    DEFINE_INFIX_OP(DT_INT, "+", DT_INT, add_int, DT_INT),
    DEFINE_INFIX_OP(DT_INT, "-", DT_INT, sub_int, DT_INT),
    DEFINE_INFIX_OP(DT_INT, "*", DT_INT, mul_int, DT_INT),
    DEFINE_INFIX_OP(DT_INT, "/", DT_INT, div_int, DT_INT),
    DEFINE_INFIX_OP(DT_INT, "%", DT_INT, mod_int, DT_INT),

    DEFINE_PREFIX_OP("~", DT_INT, bitnot_int, DT_INT),
    DEFINE_INFIX_OP(DT_INT, "|", DT_INT, bitor_int, DT_INT),
    DEFINE_INFIX_OP(DT_INT, "&", DT_INT, bitand_int, DT_INT),
    DEFINE_INFIX_OP(DT_INT, "^", DT_INT, bitxor_int, DT_INT),

    DEFINE_INFIX_OP(DT_INT, "", DT_INT, eq_int, DT_BOOL),
    DEFINE_INFIX_OP(DT_INT, "==", DT_INT, eq_int, DT_BOOL),
    DEFINE_INFIX_OP(DT_INT, "!=", DT_INT, ne_int, DT_BOOL),
    DEFINE_INFIX_OP(DT_INT, "<", DT_INT, lt_int, DT_BOOL),
    DEFINE_INFIX_OP(DT_INT, ">", DT_INT, gt_int, DT_BOOL),
    DEFINE_INFIX_OP(DT_INT, "<=", DT_INT, le_int, DT_BOOL),
    DEFINE_INFIX_OP(DT_INT, ">=", DT_INT, ge_int, DT_BOOL),

    DEFINE_INFIX_OP(DT_INT, "in", DT_LIST | DT_INT, int_in_list, DT_BOOL),

    DEFINE_CAST_OP(DT_FLOAT, cast_float_to_int, DT_INT),
    DEFINE_CAST_OP(DT_UINT, cast_uint_to_int, DT_INT),
    DEFINE_CAST_OP(DT_INT, cast_int_to_bool, DT_BOOL),
    DEFINE_CAST_OP(DT_INT | DT_LIST, cast_int_list_to_bool, DT_BOOL),
    DEFINE_CAST_OP(DT_NONE | DT_LIST, cast_empty_list_to_int_list, DT_INT | DT_LIST),

    DEFINE_DESTRUCTOR(DT_INT | DT_LIST, destroy_int_list),

};
const int num_int_operations = CONST_ARR_SIZE(int_operations);