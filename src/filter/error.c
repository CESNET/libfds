/**
 * \file src/filter/scanner.h
 * \author Michal Sedlak <xsedla0v@stud.fit.vutbr.cz>
 * \brief Error functions implementation file
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

#include "error.h"

const fds_filter_error_s memory_error = { .code = FDS_ERR_NOMEM, .msg = "out of memory" };

const error_t NO_ERROR = NULL;
const error_t MEMORY_ERROR = &memory_error;

error_t
error_create_variadic(int code, const char *fmt, va_list args)
{
    va_list args_;
    va_copy(args_, args);
    size_t msg_len = vsnprintf(NULL, 0, fmt, args_);
    error_t err = malloc(sizeof(fds_filter_error_s));
    if (!err) { 
        va_end(args_);
        return MEMORY_ERROR;
    }
    char *msg = malloc(msg_len + 1);
    if (!msg) {
        va_end(args_);
        free(err);
        return MEMORY_ERROR;
    }
    va_end(args_);
    va_copy(args_, args);
    vsnprintf(msg, msg_len + 1, fmt, args_);
    msg[msg_len] = '\0';
    err->msg = msg;
    err->code = code;
    va_end(args_);
    return err;
}

error_t
error_create(int code, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    error_t err = error_create_variadic(code, fmt, args);
    va_end(args);
    return err;
}

error_t
error_create_with_location(int code, const char *begin, const char *end, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    error_t err = error_create_variadic(code, fmt, args);
    va_end(args);
    if (err == MEMORY_ERROR) {
        return err;
    }
    err->cursor_begin = begin;
    err->cursor_end = end;
    return err;
}

void
error_destroy(error_t error)
{
    if (error != NULL && error != NO_ERROR && error != MEMORY_ERROR) {
        free(error->msg);
        free(error);
    }
}
