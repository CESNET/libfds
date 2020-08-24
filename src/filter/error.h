/**
 * \file src/filter/error.h
 * \author Michal Sedlak <xsedla0v@stud.fit.vutbr.cz>
 * \brief Error definitions header file
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

#ifndef FDS_FILTER_ERROR_H
#define FDS_FILTER_ERROR_H

#include <stdio.h>
#include <stdarg.h>

#include <libfds.h>

#define ERR_OK       FDS_OK
#define ERR_NOMEM    FDS_FILTER_ERR_NOMEM
#define ERR_LEXICAL  FDS_FILTER_ERR_LEXICAL
#define ERR_SYNTAX   FDS_FILTER_ERR_SYNTAX
#define ERR_SEMANTIC FDS_FILTER_ERR_SEMANTIC

typedef fds_filter_error_s *error_t;

extern const error_t NO_ERROR;
extern const error_t MEMORY_ERROR;

error_t
error_create_variadic(int code, const char *fmt, va_list args);

error_t
error_create(int code, const char *fmt, ...);

void
error_destroy(error_t error);

error_t
error_create_with_location(int code, const char *begin, const char *end, const char *fmt, ...);

#define LEXICAL_ERROR(CURSOR, ...) \
    error_create_with_location(FDS_ERR_SYNTAX, (CURSOR), (CURSOR) + 1, "lexical error: " __VA_ARGS__)

#define SYNTAX_ERROR(TOKEN, ...) \
    error_create_with_location(FDS_ERR_SYNTAX, (TOKEN)->cursor_begin, (TOKEN)->cursor_end, "syntax error: " __VA_ARGS__)

#define SEMANTIC_ERROR(AST, ...) \
    error_create_with_location(FDS_ERR_SEMANTIC, (AST)->cursor_begin, (AST)->cursor_end, "semantic error: " __VA_ARGS__)


#endif // FDS_FILTER_ERROR_H