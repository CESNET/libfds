/**
 * \file src/filter/filter.c
 * \author Michal Sedlak <xsedla0v@stud.fit.vutbr.cz>
 * \brief Filter interface implementation file
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libfds.h>

#include "common.h"
#include "filter.h"
#include "ast_common.h"
#include "eval_common.h"
#include "opts.h"
#include "error.h"
#include "opts.h"
#include "parser.h"
#include "scanner.h"

int
fds_filter_create(fds_filter_t **out_filter, const char *expr, const fds_filter_opts_t *opts)
{
    // Create empty filter struct 
    fds_filter_t *filter = calloc(1, sizeof(fds_filter_t));
    *out_filter = filter;
    if (filter == NULL) {
        return FDS_ERR_NOMEM;
    }

    // Copy the opts so we can access them later after they are freed by the user  
    //filter->opts = opts;
    filter->opts = fds_filter_opts_copy(opts);
    if (filter->opts == NULL) {
        filter->error = MEMORY_ERROR;
        return FDS_ERR_NOMEM;
    }

    // Set up scanner
    struct scanner_s scanner;
    init_scanner(&scanner, expr);

    // Parse the expression and build AST
    IF_DEBUG( fprintf(stderr, "----- parse -----\n"); )
    filter->error = parse_filter(&scanner, &filter->ast);
    if (filter->error != NO_ERROR) {
        return filter->error->code;
    }
    IF_DEBUG( print_ast(stderr, filter->ast); )

    // Run semantic analysis -> determine types of the AST nodes
    IF_DEBUG( fprintf(stderr, "----- semantic -----\n"); )
    filter->error = resolve_types(filter->ast, filter->opts);
    if (filter->error != NO_ERROR) {
        return filter->error->code;
    }
    IF_DEBUG( print_ast(stderr, filter->ast); )
    
    // Generate eval nodes from the AST and set up the eval runtime
    IF_DEBUG( fprintf(stderr, "----- generator -----\n"); )
    filter->error = generate_eval_tree(filter->ast, filter->opts, false, &filter->eval_root);
    if (filter->error != NO_ERROR) {
        return filter->error->code;
    }
    filter->eval_runtime.data_cb = filter->opts->data_cb;
    filter->eval_runtime.user_ctx = filter->opts->user_ctx;
    IF_DEBUG( print_eval_tree(stderr, filter->eval_root); )

    return FDS_OK;
}

bool
fds_filter_eval(fds_filter_t *filter, void *data)
{
    // Evaluate the filter with the provided data and return the value of the root node
    IF_DEBUG( fprintf(stderr, "----- evaluate -----\n"); )
    filter->eval_runtime.data = data;
    evaluate_eval_tree(filter->eval_root, &filter->eval_runtime);
    IF_DEBUG( print_eval_tree(stderr, filter->eval_root); )
    return filter->eval_root->value.b;
}

void
fds_filter_destroy(fds_filter_t *filter)
{
    if (filter == NULL) {
        return;
    }
    destroy_eval_tree(filter->eval_root);
    destroy_ast(filter->ast);
    error_destroy(filter->error);
    fds_filter_destroy_opts(filter->opts);
    free(filter);
}

error_t
fds_filter_get_error(fds_filter_t *filter)
{
    // If the filter is NULL assume memory error
    if (filter == NULL) {
        return MEMORY_ERROR;
    }
    return filter->error;
}