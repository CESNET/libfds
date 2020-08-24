/**
 * \file src/filter/operations/trie.c
 * \author Michal Sedlak <xsedla0v@stud.fit.vutbr.cz>
 * \brief Trie operations implementation file
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

#ifndef FDS_FILTER_DISABLE_TRIE

#include <libfds.h>
#include "trie.h"

int
ip_list_to_trie(fds_filter_value_u *val, fds_filter_value_u *res)
{
    struct fds_trie *trie = fds_trie_create();
    if (!trie) {
        return FDS_ERR_NOMEM;
    }
    for (int i = 0; i < val->list.len; i++) {
        int ret = fds_trie_add(trie, val->list.items[i].ip.version, val->list.items[i].ip.addr, val->list.items[i].ip.prefix);
        if (!ret) {
            return FDS_ERR_NOMEM;
        }
    }
    // free(val->list.items);
    res->p = trie;
    return FDS_OK;
}

void
destroy_trie(fds_filter_value_u *val)
{
    fds_trie_destroy(val->p);
}

void
ip_in_trie(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = fds_trie_find(right->p, left->ip.version, left->ip.addr, left->ip.prefix);
}

const fds_filter_op_s trie_operations[] = {
    FDS_FILTER_DEF_CONSTRUCTOR(FDS_FDT_IP | FDS_FDT_LIST, ip_list_to_trie, FDS_FDT_TRIE),
    FDS_FILTER_DEF_DESTRUCTOR(FDS_FDT_TRIE, destroy_trie),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_IP, "in", FDS_FDT_TRIE, ip_in_trie, FDS_FDT_BOOL),
    FDS_FILTER_END_OP_LIST
};

#endif // FDS_FILTER_ENABLE_TRIE