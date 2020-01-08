#include "mac.h"
#include "../common.h"
#include "../values.h"

void
cast_empty_list_to_bool(fds_filter_value_u *_1, fds_filter_value_u *_2, fds_filter_value_u *result)
{
    result->b = false;
}

const fds_filter_op_s other_operations[] = {
    FDS_FILTER_DEF_CAST(DT_LIST | DT_NONE, cast_empty_list_to_bool, DT_BOOL),
};

const int num_other_operations = CONST_ARR_SIZE(other_operations);