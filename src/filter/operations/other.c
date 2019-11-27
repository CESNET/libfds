#include "mac.h"
#include "../common.h"
#include "../values.h"

void
cast_empty_list_to_bool(value_t *_1, value_t *_2, value_t *result)
{
    UNUSED(_1);
    UNUSED(_2);
    result->b = false;
}

const operation_s other_operations[] = {
    DEFINE_CAST_OP(DT_LIST | DT_NONE, cast_empty_list_to_bool, DT_BOOL),
};

const int num_other_operations = CONST_ARR_SIZE(other_operations);