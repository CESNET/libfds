/**
 * \file src/filter/operations/float.c
 * \author Michal Sedlak <xsedla0v@stud.fit.vutbr.cz>
 * \brief Float operations implementation file
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
    FDS_FILTER_DEF_UNARY_OP("-", FDS_FDT_FLOAT, neg_float, FDS_FDT_FLOAT),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_FLOAT, "+", FDS_FDT_FLOAT, add_float, FDS_FDT_FLOAT),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_FLOAT, "-", FDS_FDT_FLOAT, sub_float, FDS_FDT_FLOAT),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_FLOAT, "*", FDS_FDT_FLOAT, mul_float, FDS_FDT_FLOAT),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_FLOAT, "/", FDS_FDT_FLOAT, div_float, FDS_FDT_FLOAT),

    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_FLOAT, "", FDS_FDT_FLOAT, eq_float, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_FLOAT, "==", FDS_FDT_FLOAT, eq_float, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_FLOAT, "!=", FDS_FDT_FLOAT, ne_float, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_FLOAT, "<", FDS_FDT_FLOAT, lt_float, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_FLOAT, ">", FDS_FDT_FLOAT, gt_float, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_FLOAT, "<=", FDS_FDT_FLOAT, le_float, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_FLOAT, ">=", FDS_FDT_FLOAT, ge_float, FDS_FDT_BOOL),

    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_FLOAT, "in", FDS_FDT_LIST | FDS_FDT_FLOAT, float_in_list, FDS_FDT_BOOL),

    FDS_FILTER_DEF_CAST(FDS_FDT_INT, cast_int_to_float, FDS_FDT_FLOAT),
    FDS_FILTER_DEF_CAST(FDS_FDT_FLOAT, cast_float_to_bool, FDS_FDT_BOOL),
    FDS_FILTER_DEF_CAST(FDS_FDT_FLOAT | FDS_FDT_LIST, cast_float_list_to_bool, FDS_FDT_BOOL),

    FDS_FILTER_DEF_DESTRUCTOR(FDS_FDT_FLOAT | FDS_FDT_LIST, destroy_float_list),

    FDS_FILTER_END_OP_LIST
};

