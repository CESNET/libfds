#include "flags.h"

void
cast_flags_to_uint(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->u = operand->u;
}

void
cmp_flags(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = (left->u & right->u) == right->u; 
}

const fds_filter_op_s flags_operations[] = {
    FDS_FILTER_DEF_CAST(FDS_FDT_FLAGS, cast_flags_to_uint, FDS_FDT_UINT),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_FLAGS, "", FDS_FDT_UINT, cmp_flags, FDS_FDT_BOOL),
    FDS_FILTER_END_OP_LIST
};
