/**
 * \file src/filter/operations/str.c
 * \author Michal Sedlak <xsedla0v@stud.fit.vutbr.cz>
 * \brief String operations implementation file
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

#include "str.h"
#include "../values.h"

static inline bool
str_equals(fds_filter_str_t *left, fds_filter_str_t *right)
{
    return left->len == right->len && memcmp(left->chars, right->chars, right->len) == 0;
}

void
eq_str(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = str_equals(&left->str, &right->str);
}

void
ne_str(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = !str_equals(&left->str, &right->str);
}

const char *
strnnstr(const char *s, const char *find, size_t slen, size_t findlen)
{
    char c, sc;

    if (findlen != 0) {
        c = *find++;
        findlen--;
        do {
            do {
                if (slen-- < 1)
                    return NULL;
            } while ((sc = *s++) != c);
            if (findlen > slen)
                return NULL;
        } while (strncmp(s, find, findlen) != 0);
        s--;
    }
    return s;
}

void
contains_str(fds_filter_value_u *big, fds_filter_value_u *little, fds_filter_value_u *result)
{
    result->b = strnnstr(big->str.chars, little->str.chars, big->str.len, little->str.len) != NULL;
}




void
str_in_list(fds_filter_value_u *item, fds_filter_value_u *list, fds_filter_value_u *result)
{
    result->b = false;
    for (int i = 0; i < list->list.len; i++) {
        if (str_equals(&item->str, &list->list.items[i].str)) {
            result->b = true;
            return;
        }
    }
}

void
destroy_str_(fds_filter_value_u *operand)
{
    free(operand->str.chars);
}

void
destroy_list_of_str(fds_filter_value_u *operand)
{
    for (int i = 0; i < operand->list.len; i++) {
        free(operand->list.items[i].str.chars);
    }
    free(operand->list.items);
}

void
cast_str_list_to_bool(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->b = operand->list.len > 0;
}

void
cast_str_to_bool(fds_filter_value_u *operand, fds_filter_value_u *result)
{
    result->b = operand->str.len > 0;
}

const fds_filter_op_s str_operations[] = {
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_STR, "", FDS_FDT_STR, eq_str, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_STR, "==", FDS_FDT_STR, eq_str, FDS_FDT_BOOL),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_STR, "!=", FDS_FDT_STR, ne_str, FDS_FDT_BOOL),

    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_STR, "contains", FDS_FDT_STR, contains_str, FDS_FDT_BOOL),

    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_STR, "in", FDS_FDT_LIST | FDS_FDT_STR, str_in_list, FDS_FDT_BOOL),

    FDS_FILTER_DEF_CAST(FDS_FDT_STR, cast_str_to_bool, FDS_FDT_BOOL),
    FDS_FILTER_DEF_CAST(FDS_FDT_LIST | FDS_FDT_STR, cast_str_list_to_bool, FDS_FDT_BOOL),

    FDS_FILTER_DEF_DESTRUCTOR(FDS_FDT_STR, destroy_str_),
    FDS_FILTER_DEF_DESTRUCTOR(FDS_FDT_STR | FDS_FDT_LIST, destroy_list_of_str),

    FDS_FILTER_END_OP_LIST
};
