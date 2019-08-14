#ifndef FDS_TRIE_H
#define FDS_TRIE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

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
 * @return  1 if the address was found, 0 otherwise.
 */
FDS_API int
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