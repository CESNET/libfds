/**
 * \file src/filter/operations/ip.c
 * \author Michal Sedlak <xsedla0v@stud.fit.vutbr.cz>
 * \brief IP address operations implementation file
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

#include "ip.h"
#include "../values.h"

static inline bool
ip_prefix_equals(fds_filter_ip_t *left, fds_filter_ip_t *right)
{
    if (left->version != right->version) {
        return false;
    }

    int n_cmp_bits = left->prefix <= right->prefix ? left->prefix : right->prefix;
    if (memcmp(left->addr, right->addr, n_cmp_bits / 8) != 0) {
        return false;
    }
    if (n_cmp_bits % 8 == 0) {
        return true;
    }
    uint8_t last_byte_left = left->addr[n_cmp_bits / 8] >> (8 - (n_cmp_bits % 8));
    uint8_t last_byte_right = right->addr[n_cmp_bits / 8] >> (8 - (n_cmp_bits % 8));
    return last_byte_left == last_byte_right;
}

void
eq_ip(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = ip_prefix_equals(&left->ip, &right->ip);
}

void
ne_ip(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = !ip_prefix_equals(&left->ip, &right->ip);
}

void
ip_in_list(fds_filter_value_u *item, fds_filter_value_u *list, fds_filter_value_u *result)
{
    result->b = false;
    for (int i = 0; i < list->list.len; i++) {
        if (ip_prefix_equals(&list->list.items[i].ip, &item->ip)) {
            result->b = true;
            return;
        }
    }
}

void
cast_ip_to_bool(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->b = operand->ip.version != 0; // ???
}

void
cast_ip_list_to_bool(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->b = operand->list.len > 0;
}

void
destroy_ip_list(fds_filter_value_u *operand)
{
    free(operand->list.items);
}

const fds_filter_op_s ip_operations[] = {
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_IP, "", FDS_FDT_IP, eq_ip, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_IP, "==", FDS_FDT_IP, eq_ip, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_IP, "!=", FDS_FDT_IP, ne_ip, FDS_FDT_BOOL),

    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_IP, "in", FDS_FDT_LIST | FDS_FDT_IP, ip_in_list, FDS_FDT_BOOL),

    FDS_FILTER_DEF_CAST(FDS_FDT_IP, cast_ip_to_bool, FDS_FDT_BOOL),
    FDS_FILTER_DEF_CAST(FDS_FDT_IP | FDS_FDT_LIST, cast_ip_list_to_bool, FDS_FDT_BOOL),

    FDS_FILTER_DEF_DESTRUCTOR(FDS_FDT_IP | FDS_FDT_LIST, destroy_ip_list),

    FDS_FILTER_END_OP_LIST
};
