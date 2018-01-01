/**
 * \file   src/template_mgr/garbage.c
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \brief  Simple garbage collector (source file)
 * \date   November 2017
 */

/*
 * Copyright (C) 2017 CESNET, z.s.p.o.
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

#ifndef IPFIXCOL_GARBAGE_H
#define IPFIXCOL_GARBAGE_H

#include <stdbool.h>
#include <libfds/template_mgr.h>

/**
 * \brief Callback function that will be used to destroy a garbage record
 */
typedef void (*garbage_fn_t)(void *);

/**
 * \brief Create a new garbage collection
 * \return Pointer or NULL (memory allocation error)
 */
fds_tgarbage_t *
garbage_create();

/**
 * \brief Destroy a garbage collection
 *
 * All garbage records will be destroyed too.
 * \param[in] gc Garbage collection
 */
void
garbage_destroy(fds_tgarbage_t *gc);

/**
 * \brief Add a garbage
 * \param[in] gc   Garbage collection
 * \param[in] data Garbage to add
 * \param[in] fn   Function callback that will destroy the garbage
 * \return On success returns #FDS_OK. Otherwise #FDS_ERR_NOMEM or #FDS_ERR_ARG.
 */
int
garbage_append(fds_tgarbage_t *gc, void *data, garbage_fn_t fn);

/**
 * \brief Destroy garbage
 *
 * For all garbage records, run their destroy callback.
 * \param[in] gc Garbage collection
 */
void
garbage_remove(fds_tgarbage_t *gc);

/**
 * \brief Is the collection empty?
 * \param[in] gc Garbage collection
 * \return True or false
 */
bool
garbage_empty(const fds_tgarbage_t *gc);


#endif //IPFIXCOL_GARBAGE_H
