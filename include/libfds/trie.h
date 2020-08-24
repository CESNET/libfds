/**
 * \file include/libfds/trie.h
 * \author Michal Sedlak <xsedla0v@stud.fit.vutbr.cz>
 * \brief IP address trie interface header file
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

#ifndef FDS_TRIE_H
#define FDS_TRIE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct fds_trie fds_trie_t;

/**
 * Create a trie.
 */
FDS_API struct fds_trie *
fds_trie_create();

/**
 * Destroy a trie.
 */
FDS_API void
fds_trie_destroy(struct fds_trie *trie);

/**
 * Add an address record to the trie.
 *
 * @param   trie            The trie.
 * @param   ip_version      The IP version (4 or 6).
 * @param   address_bytes   The address.
 * @param   bit_length      The length of the address in bits.
 * @return  1 on success, 0 on failure.
 */
FDS_API int
fds_trie_add(struct fds_trie *trie, int ip_version, uint8_t *address, int bit_length);

/**
 * Try to find an address record in a trie.
 *
 * @param   trie            The trie.
 * @param   ip_version      The IP version (4 or 6).
 * @param   address_bytes   The address.
 * @param   bit_length      The length of the address in bits.
 * @return  true if the address was found, false otherwise.
 */
FDS_API bool
fds_trie_find(struct fds_trie *trie, int ip_version, uint8_t *address_bytes, int bit_length);

/**
 *  Print the trie to stdout (for debugging).
 */
FDS_API void
fds_trie_print(struct fds_trie *trie);

#ifdef __cplusplus
}
#endif

#endif // FDS_TRIE_H