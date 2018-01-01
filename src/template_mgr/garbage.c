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

#include <stddef.h>
#include <stdlib.h>
#include "garbage.h"

/** Garbage record                  */
struct rec {
    /** Delete callback             */
    garbage_fn_t fn;
    /** Data for delete callback    */
    void *data;
};

/**
 * \brief Internal garbage structure
 */
struct fds_tgarbage {
    /** Number of valid records         */
    size_t cnt_used;
    /** Number of pre-allocated records */
    size_t cnt_alloc;
    /** Array of records                */
    struct rec *array;
};


fds_tgarbage_t *
garbage_create()
{
    // Create the structure and an array of garbage
    fds_tgarbage_t *result = calloc(1, sizeof(*result));
    if (!result) {
        return NULL;
    }

    const size_t def_size = 16;
    result->array = malloc(def_size * sizeof(*(result->array)));
    if (!result->array) {
        free(result);
        return NULL;
    }

    // Set default values
    result->cnt_alloc = def_size;
    result->cnt_used = 0;
    return result;
}


void
garbage_destroy(fds_tgarbage_t *gc)
{
    if (!gc) {
        return;
    }

    garbage_remove(gc);
    free(gc->array);
    free(gc);
}


int
garbage_append(fds_tgarbage_t *gc, void *data, garbage_fn_t fn)
{
    if (!data) {
        return FDS_OK;
    }

    if (!fn) {
        return FDS_ERR_ARG;
    }

    // Make sure that the array is large enough
    if (gc->cnt_used == gc->cnt_alloc) {
        // Resize the garbage array
        size_t new_size = 2 * gc->cnt_alloc;
        struct rec *new_array = realloc(gc->array, new_size * sizeof(*new_array));
        if (!new_array) {
            return FDS_ERR_NOMEM;
        }

        gc->cnt_alloc = new_size;
        gc->array = new_array;
    }

    struct rec val = (struct rec) {fn, data};
    gc->array[gc->cnt_used++] = val;
    return FDS_OK;
}

bool
garbage_empty(const fds_tgarbage_t *gc)
{
    return (gc->cnt_used == 0);
}

void
garbage_remove(fds_tgarbage_t *gc)
{
    for (size_t i = 0; i < gc->cnt_used; ++i) {
        struct rec *ptr = &gc->array[i];
        ptr->fn(ptr->data);
    }
    gc->cnt_used = 0;
}