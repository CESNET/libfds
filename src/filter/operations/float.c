#include "float.h"
#include "../common.h"
#include "../values.h"

#define FLOAT_EQUALS_EPSILON   0.001 // Precision to 3 decimal point places when comparing floats for equality

static inline bool
float_equals(fds_filter_float_t a, fds_filter_float_t b)
{
    return fabs(a - b) < FLOAT_EQUALS_EPSILON;
}

void
add_float(value_t *left, value_t *right, value_t *result)
{
    result->f = left->f + right->f;
}

void
sub_float(value_t *left, value_t *right, value_t *result)
{
    result->f = left->f - right->f;
}

void
mul_float(value_t *left, value_t *right, value_t *result)
{
    result->f = left->f * right->f;
}

void
div_float(value_t *left, value_t *right, value_t *result)
{
    result->f = left->f / right->f;
}

void
neg_float(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->f = -operand->f;
}

void
cast_int_to_float(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->f = operand->i;
}

void
cast_uint_to_float(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->f = operand->u;
}

void
cast_float_to_bool(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->b = operand->f != 0.0; // ???
}

void
eq_float(value_t *left, value_t *right, value_t *result)
{
    result->b = float_equals(left->f, right->f);
}

void
ne_float(value_t *left, value_t *right, value_t *result)
{
    result->b = !float_equals(left->f, right->f);
}

void
lt_float(value_t *left, value_t *right, value_t *result)
{
    result->b = left->f < right->f;
}

void
gt_float(value_t *left, value_t *right, value_t *result)
{
    result->b = left->f > right->f;
}

void
le_float(value_t *left, value_t *right, value_t *result)
{
    result->b = left->f <= right->f;
}

void
ge_float(value_t *left, value_t *right, value_t *result)
{
    result->b = left->f >= right->f;
}

void
float_in_list(value_t *item, value_t *list, value_t *result)
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
cast_float_list_to_bool(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->b = operand->list.len > 0;
}

void
cast_empty_list_to_float_list(value_t *operand, value_t *_, value_t *result)
{
    UNUSED(_);
    result->list = operand->list;
}

void
destroy_float_list(value_t *operand, value_t *_1, value_t *_2)
{
    UNUSED(_1);
    UNUSED(_2);
    free(operand->list.items);
}

const operation_s float_operations[] = {
    DEFINE_PREFIX_OP("-", DT_FLOAT, neg_float, DT_FLOAT),
    DEFINE_INFIX_OP(DT_FLOAT, "+", DT_FLOAT, add_float, DT_FLOAT),
    DEFINE_INFIX_OP(DT_FLOAT, "-", DT_FLOAT, sub_float, DT_FLOAT),
    DEFINE_INFIX_OP(DT_FLOAT, "*", DT_FLOAT, mul_float, DT_FLOAT),
    DEFINE_INFIX_OP(DT_FLOAT, "/", DT_FLOAT, div_float, DT_FLOAT),

    DEFINE_INFIX_OP(DT_FLOAT, "", DT_FLOAT, eq_float, DT_BOOL),
    DEFINE_INFIX_OP(DT_FLOAT, "==", DT_FLOAT, eq_float, DT_BOOL),
    DEFINE_INFIX_OP(DT_FLOAT, "!=", DT_FLOAT, ne_float, DT_BOOL),
    DEFINE_INFIX_OP(DT_FLOAT, "<", DT_FLOAT, lt_float, DT_BOOL),
    DEFINE_INFIX_OP(DT_FLOAT, ">", DT_FLOAT, gt_float, DT_BOOL),
    DEFINE_INFIX_OP(DT_FLOAT, "<=", DT_FLOAT, le_float, DT_BOOL),
    DEFINE_INFIX_OP(DT_FLOAT, ">=", DT_FLOAT, ge_float, DT_BOOL),

    DEFINE_INFIX_OP(DT_FLOAT, "in", DT_LIST | DT_FLOAT, float_in_list, DT_BOOL),

    DEFINE_CAST_OP(DT_INT, cast_int_to_float, DT_FLOAT),
    DEFINE_CAST_OP(DT_FLOAT, cast_float_to_bool, DT_BOOL),
    DEFINE_CAST_OP(DT_FLOAT | DT_LIST, cast_float_list_to_bool, DT_BOOL),
    DEFINE_CAST_OP(DT_NONE | DT_LIST, cast_empty_list_to_float_list, DT_FLOAT | DT_LIST),

    DEFINE_DESTRUCTOR(DT_FLOAT | DT_LIST, destroy_float_list),
};
const int num_float_operations = CONST_ARR_SIZE(float_operations);

