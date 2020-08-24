/**
 * \file src/filter/opts.c
 * \author Michal Sedlak <xsedla0v@stud.fit.vutbr.cz>
 * \brief Filter opts
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

#include "opts.h"

#include "operations/float.h"
#include "operations/int.h"
#include "operations/ip.h"
#include "operations/mac.h"
#include "operations/str.h"
#include "operations/uint.h"
#include "operations/flags.h"
#ifndef FDS_FILTER_DISABLE_TRIE
#include "operations/trie.h"
#endif

static int
dummy_lookup_callback(void *user_ctx, const char *name, const char *other_name, int *out_id, int *out_datatype, int *out_flags)
{
    (void)(user_ctx);
    (void)(name);
    (void)(other_name);
    (void)(out_id);
    (void)(out_datatype);
    (void)(out_flags);
    return FDS_ERR_NOTFOUND;
}

static void
dummy_const_callback(void *user_ctx, int id, fds_filter_value_u *out_value)
{
    (void)(user_ctx);
    (void)(id);
    (void)(out_value);
}

static int
dummy_data_callback(void *user_ctx, bool reset_ctx, int id, void *data, fds_filter_value_u *out_value)
{
    (void)(user_ctx);
    (void)(reset_ctx);
    (void)(id);
    (void)(data);
    (void)(out_value);
    return FDS_ERR_NOTFOUND;
}

static void
print_op_list(FILE *out, fds_filter_op_s *op_list)
{
    for (fds_filter_op_s *op = op_list; op->symbol != NULL; op++) {
        print_operation(out, op);
        fprintf(out, "\n");
    }
}

static inline int
op_list_count(fds_filter_op_s *op_list)
{
    if (!op_list) {
        return 0;
    }
    int cnt = 0;
    while (op_list[cnt].symbol != NULL) {
        cnt++;
    }
    return cnt;
}

fds_filter_op_s *
fds_filter_opts_add_op(fds_filter_opts_t *opts, fds_filter_op_s op)
{
    int cnt = op_list_count(opts->op_list) + 1;
    void *tmp = realloc(opts->op_list, (cnt + 1) * sizeof(fds_filter_op_s));
    if (!tmp) {
        return NULL;
    }
    opts->op_list = tmp;
    memmove(opts->op_list + 1, opts->op_list, cnt * sizeof(fds_filter_op_s));
    opts->op_list[0] = op;
    return opts->op_list; 
}

fds_filter_op_s *
fds_filter_opts_extend_ops(fds_filter_opts_t *opts, const fds_filter_op_s *op_list)
{
    int cnt = op_list_count(opts->op_list) + 1; // + 1 for the trailing null op
    int extend_cnt = op_list_count(op_list);
    void *tmp = realloc(opts->op_list, (cnt + extend_cnt) * sizeof(fds_filter_op_s));
    if (!tmp) {
        return NULL;
    }
    opts->op_list = tmp;
    memmove(opts->op_list + extend_cnt, opts->op_list, cnt * sizeof(fds_filter_op_s));
    memcpy(opts->op_list, op_list, extend_cnt * sizeof(fds_filter_op_s));
    return opts->op_list;
}

fds_filter_opts_t *
fds_filter_create_default_opts()
{
    fds_filter_opts_t *opts = malloc(sizeof(fds_filter_opts_t));
    if (!opts) {
        return NULL;
    }

    opts->lookup_cb = dummy_lookup_callback;
    opts->const_cb = dummy_const_callback;
    opts->data_cb = dummy_data_callback;

    opts->op_list = malloc(sizeof(fds_filter_op_s));
    if (!opts->op_list) {
        goto error;
    }
    opts->op_list[0] = (fds_filter_op_s) FDS_FILTER_END_OP_LIST;

    if (!fds_filter_opts_extend_ops(opts, int_operations)) {
        goto error;    
    }
    if (!fds_filter_opts_extend_ops(opts, uint_operations)) {
        goto error;    
    }
    if (!fds_filter_opts_extend_ops(opts, float_operations)) {
        goto error;    
    }
    if (!fds_filter_opts_extend_ops(opts, str_operations)) {
        goto error;    
    }
    if (!fds_filter_opts_extend_ops(opts, ip_operations)) {
        goto error;    
    }
    if (!fds_filter_opts_extend_ops(opts, mac_operations)) {
        goto error;    
    }
    if (!fds_filter_opts_extend_ops(opts, flags_operations)) {
        goto error;    
    }
#if 0
    if (!fds_filter_opts_extend_ops(opts, trie_operations)) {
        goto error;    
    }
#endif

    return opts;

error:
    fds_filter_destroy_opts(opts);
    return NULL;
}

fds_filter_opts_t *
fds_filter_opts_copy(fds_filter_opts_t *original_opts)
{
    fds_filter_opts_t *opts = malloc(sizeof(fds_filter_opts_t));
    *opts = *original_opts;
    int cnt = op_list_count(original_opts->op_list);
    // + 1 for the end op
    opts->op_list = malloc((cnt + 1) * sizeof(fds_filter_op_s));
    if (!opts->op_list) {
        free(opts);
        return NULL;
    }
    memcpy(opts->op_list, original_opts->op_list, (cnt + 1) * sizeof(fds_filter_op_s));
    opts->const_cb = original_opts->const_cb;
    opts->data_cb = original_opts->data_cb;
    opts->lookup_cb = original_opts->lookup_cb;
    opts->user_ctx = original_opts->user_ctx;
    return opts;
}

void
fds_filter_opts_set_lookup_cb(fds_filter_opts_t *opts, fds_filter_lookup_cb_t *cb)
{
    opts->lookup_cb = cb;
}

void
fds_filter_opts_set_const_cb(fds_filter_opts_t *opts, fds_filter_const_cb_t *cb)
{
    opts->const_cb = cb;
}

void
fds_filter_opts_set_data_cb(fds_filter_opts_t *opts, fds_filter_data_cb_t *cb)
{
    opts->data_cb = cb;
}

void
fds_filter_opts_set_user_ctx(fds_filter_opts_t *opts, void *user_ctx)
{
    opts->user_ctx = user_ctx;
}

void *
fds_filter_opts_get_user_ctx(const fds_filter_opts_t *opts)
{
    return opts->user_ctx;
}

void
fds_filter_destroy_opts(fds_filter_opts_t *opts)
{
    free(opts->op_list);
    free(opts);
}
