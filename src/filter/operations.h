/**
 * \file src/filter/operations.h
 * \author Michal Sedlak <xsedla0v@stud.fit.vutbr.cz>
 * \brief Operations utils file
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

#ifndef FDS_FILTER_OPERATION_H
#define FDS_FILTER_OPERATION_H

#include <stdbool.h>

#include "values.h"

static inline void
print_operation(FILE *out, fds_filter_op_s *op)
{
    if (op->arg1_dt != FDS_FDT_NONE && op->arg2_dt != FDS_FDT_NONE && op->out_dt != FDS_FDT_NONE) {
        fprintf(out, 
            "%s (%s, %s) -> %s",
            op->symbol,
            data_type_to_str(op->arg1_dt),
            data_type_to_str(op->arg2_dt),
            data_type_to_str(op->out_dt)
        );
    } else if (op->arg1_dt != FDS_FDT_NONE && op->out_dt != FDS_FDT_NONE) {
        fprintf(out,
            "%s (%s) -> %s",
            op->symbol,
            data_type_to_str(op->arg1_dt),
            data_type_to_str(op->out_dt)
        );
    } else if (op->arg1_dt != FDS_FDT_NONE) {
        fprintf(out,
            "%s (%s)",
            op->symbol,
            data_type_to_str(op->arg1_dt)
        );
    } else {
        // ??
        fprintf(out,
            "%s (%s, %s) -> %s",
            op->symbol,
            data_type_to_str(op->arg1_dt),
            data_type_to_str(op->arg2_dt),
            data_type_to_str(op->out_dt)
        );
    }
}

static inline fds_filter_op_s *
find_next_op(fds_filter_op_s *op_list, fds_filter_op_s *prev, const char *symbol, int out_dt, int arg1_dt, int arg2_dt)
{
    for (fds_filter_op_s *o = prev ? prev : op_list; o->symbol != NULL; o++) {
        if (strcmp(o->symbol, symbol) != 0) {
            continue;
        }
        if (arg1_dt != FDS_FDT_ANY && arg1_dt != o->arg1_dt) {
            continue;
        }
        if (arg2_dt != FDS_FDT_ANY && arg2_dt != o->arg2_dt) {
            continue;
        }
        if (out_dt != FDS_FDT_ANY && out_dt != o->out_dt) {
            continue;
        }
        return o;
    }
    return NULL;
}

static inline fds_filter_op_s *
find_op(fds_filter_op_s *op_list, const char *symbol, int out_dt, int arg1_dt, int arg2_dt)
{
    return find_next_op(op_list, NULL, symbol, out_dt, arg1_dt, arg2_dt);
}

static inline fds_filter_op_s *
find_next_binary_op(fds_filter_op_s *op_list,
    fds_filter_op_s *prev, const char *symbol, int arg1_dt, int arg2_dt)
{
    return find_next_op(op_list, prev, symbol, FDS_FDT_ANY, arg1_dt, arg2_dt);
}

static inline fds_filter_op_s *
find_binary_op(fds_filter_op_s *op_list, const char *symbol, int arg1_dt, int arg2_dt)
{
    return find_next_op(op_list, NULL, symbol, FDS_FDT_ANY, arg1_dt, arg2_dt);
}

static inline fds_filter_op_s *
find_next_unary_op(fds_filter_op_s *op_list, fds_filter_op_s *prev, const char *symbol, int arg1_dt)
{
    return find_next_op(op_list, prev, symbol, FDS_FDT_ANY, arg1_dt, FDS_FDT_NONE);
}

static inline fds_filter_op_s *
find_unary_op(fds_filter_op_s *op_list, const char *symbol, int arg1_dt)
{
    return find_next_op(op_list, NULL, symbol, FDS_FDT_ANY, arg1_dt, FDS_FDT_NONE);
}

static inline fds_filter_op_s *
find_next_cast(fds_filter_op_s *op_list, fds_filter_op_s *prev, int from_dt)
{
    return find_next_op(op_list, prev, "__cast__", FDS_FDT_ANY, from_dt, FDS_FDT_NONE);
}

static inline fds_filter_op_s *
find_cast(fds_filter_op_s *op_list, int from_dt, int to_dt)
{
    return find_next_op(op_list, NULL, "__cast__", to_dt, from_dt, FDS_FDT_NONE);
}

static inline bool
can_cast(fds_filter_op_s *op_list, int from_dt, int to_dt)
{
    return from_dt == to_dt || find_cast(op_list, from_dt, to_dt) != NULL;
}


static inline fds_filter_op_s *
find_constructor(fds_filter_op_s *op_list, int from_dt, int to_dt)
{
    return find_next_op(op_list, NULL, "__constructor__", to_dt, from_dt, FDS_FDT_NONE);
}


static inline fds_filter_op_s *
find_destructor(fds_filter_op_s *op_list, int datatype)
{
    return find_next_op(op_list, NULL, "__destructor__", FDS_FDT_NONE, datatype, FDS_FDT_NONE);
}


#endif