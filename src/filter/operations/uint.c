/**
 * \file src/filter/operations/uint.c
 * \author Michal Sedlak <xsedla0v@stud.fit.vutbr.cz>
 * \brief Unsigned integer operations implementation file
 * \date 2020
 */

/* 
 * Copyright (C) 2020 CESNET, z.s.p.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is'', and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */

#include "uint.h"
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
destroy_uint_list(fds_filter_value_u *operand)
{
    free(operand->list.items);
}

const fds_filter_op_s uint_operations[] = {
    FDS_FILTER_DEF_UNARY_OP("-", FDS_FDT_UINT, neg_uint, FDS_FDT_INT),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_UINT, "+", FDS_FDT_UINT, add_uint, FDS_FDT_UINT),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_UINT, "-", FDS_FDT_UINT, sub_uint, FDS_FDT_UINT),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_UINT, "*", FDS_FDT_UINT, mul_uint, FDS_FDT_UINT),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_UINT, "/", FDS_FDT_UINT, div_uint, FDS_FDT_UINT),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_UINT, "%", FDS_FDT_UINT, mod_uint, FDS_FDT_UINT),

    FDS_FILTER_DEF_UNARY_OP("~", FDS_FDT_UINT, bitnot_uint, FDS_FDT_UINT),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_UINT, "|", FDS_FDT_UINT, bitor_uint, FDS_FDT_UINT),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_UINT, "&", FDS_FDT_UINT, bitand_uint, FDS_FDT_UINT),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_UINT, "^", FDS_FDT_UINT, bitxor_uint, FDS_FDT_UINT),

    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_UINT, "", FDS_FDT_UINT, eq_uint, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_UINT, "==", FDS_FDT_UINT, eq_uint, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_UINT, "!=", FDS_FDT_UINT, ne_uint, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_UINT, "<", FDS_FDT_UINT, lt_uint, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_UINT, ">", FDS_FDT_UINT, gt_uint, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_UINT, "<=", FDS_FDT_UINT, le_uint, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_UINT, ">=", FDS_FDT_UINT, ge_uint, FDS_FDT_BOOL),

    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_UINT, "in", FDS_FDT_LIST | FDS_FDT_UINT, uint_in_list, FDS_FDT_BOOL),

    FDS_FILTER_DEF_CAST(FDS_FDT_FLOAT, cast_float_to_uint, FDS_FDT_UINT),
    FDS_FILTER_DEF_CAST(FDS_FDT_UINT, cast_uint_to_bool, FDS_FDT_BOOL),

    FDS_FILTER_DEF_DESTRUCTOR(FDS_FDT_UINT | FDS_FDT_LIST, destroy_uint_list),

    FDS_FILTER_END_OP_LIST
};
