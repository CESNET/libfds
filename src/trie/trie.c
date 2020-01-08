#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <libfds.h>

struct bit_array {
    uint32_t *value;
    int bit_offset;
    int bit_length;
};

static inline struct bit_array
bit_array_create(uint32_t *value, int bit_length)
{
    struct bit_array bit_array;
    bit_array.value = value;
    bit_array.bit_length = bit_length;
    bit_array.bit_offset = 0;
	return bit_array;
}

static inline void
bit_array_advance(struct bit_array *bit_array, int n)
{
    bit_array->bit_offset += n;
    if (bit_array->bit_offset == 32) {
        bit_array->bit_offset = 0;
        bit_array->bit_length -= 32;
        bit_array->value++;
    }
}

static inline int
bit_array_is_last_word(struct bit_array *bit_array)
{
    return bit_array->bit_length <= 32;
}

static inline int
bit_array_bits_remaining(struct bit_array *bit_array)
{
    return bit_array->bit_length > 32 ? 32 - bit_array->bit_offset
                                      : bit_array->bit_length - bit_array->bit_offset;
}

struct trie_node {
    uint32_t prefix;
    int prefix_length;
    int is_intermediate;
    struct trie_node *children[2];
};

struct fds_trie {
    struct trie_node *ipv4_root;
    struct trie_node *ipv6_root;
};

static void
dump_n_bits(uint32_t value, int n)
{
    for (int i = 1; i <= n; i++) {
        printf("%s", value & (1U << (32 - i)) ? "1" : "0");
        if (i % 4 == 0) {
            printf(" ");
        }
    }
}

static inline uint32_t
extract_n_bits(uint32_t value, int from, int length)
{
    if (length == 0) {
        return 0;
    }
    value = (value >> (32 - length - from));
    value = value << (32 - length);
    return value;
}

static inline uint32_t
extract_bit(uint32_t value, int index)
{
    return value & (1 << (31 - index));
}

static inline uint32_t
bit_scan_right(uint32_t value)
{
    int i = 0;
    while (i < 32) {
        if (value & (1 << (31 - i))) {
            break;
        }
        i++;
    }
    return i;
}

static inline int
find_differing_bit(uint32_t a, uint32_t b)
{
    return bit_scan_right(a ^ b);
}

static void
ip_address_bytes_to_words(int ip_version, uint8_t *bytes, uint32_t *words)
{
    words[0] = bytes[0] << 24 | bytes[1] << 16 | bytes[2] << 8 | bytes[3];
    if (ip_version == 6) {
        words[1] = bytes[4] << 24 | bytes[5] << 16 | bytes[6] << 8 | bytes[7];
        words[2] = bytes[8] << 24 | bytes[9] << 16 | bytes[10] << 8 | bytes[11];
        words[3] = bytes[12] << 24 | bytes[13] << 16 | bytes[14] << 8 | bytes[15];
    }
}

void
dump_trie_node(struct trie_node *node, int level, const char *name, int limit)
{
    for (int i = 0; i < level; i++) {
        printf("  ");
    }
    if (limit == 0) {
        printf("...\n");
        return;
    }

    printf("%s -> ", name);
    if (!node) {
        printf("NULL\n");
        return;
    }
    printf("prefix: ");
    dump_n_bits(node->prefix, node->prefix_length);
    printf(" length: %d", node->prefix_length);
    printf(" intermediate: %d", node->is_intermediate);
    printf("\n");
    dump_trie_node(node->children[0], level + 1, "0", limit-1);
    dump_trie_node(node->children[1], level + 1, "1", limit-1);
}

/**
 * Create a trie.
 */
struct fds_trie *
fds_trie_create()
{
    struct fds_trie *trie = malloc(sizeof(struct fds_trie));
    if (trie == NULL) {
        return NULL;
    }
    trie->ipv4_root = NULL;
    trie->ipv6_root = NULL;
    return trie;
}

/**
 * Destroy a trie node.
 */
static void
trie_node_destroy(struct trie_node *node)
{
    if (node == NULL) {
        return;
    }
    trie_node_destroy(node->children[0]);
    trie_node_destroy(node->children[1]);
    free(node);
}

/**
 * Destroy a trie.
 */
void
fds_trie_destroy(struct fds_trie *trie)
{
    trie_node_destroy(trie->ipv4_root);
    trie_node_destroy(trie->ipv6_root);
    free(trie);
}

/**
 * Create an empty trie node.
 */
static struct trie_node *
trie_node_create()
{
    struct trie_node *node = malloc(sizeof(struct trie_node));
    if (node == NULL) {
        return NULL;
    }
    node->prefix = 0;
    node->prefix_length = 0;
    node->is_intermediate = 0;
    node->children[0] = NULL;
    node->children[1] = NULL;
    return node;
}

/**
 * Split a node on a particular bit index.
 *
 * @return Pointer to where the new child node should be created, or NULL on failure.
 */
static struct trie_node **
trie_node_split_on_bit(struct trie_node *node, int bit_index)
{
    struct trie_node *child_node = trie_node_create();
    if (child_node == NULL) {
        return NULL;
    }
    child_node->prefix = extract_n_bits(node->prefix, bit_index + 1, node->prefix_length - bit_index - 1);
    child_node->prefix_length = node->prefix_length - bit_index - 1;
    assert(child_node->prefix_length >= 0);
    child_node->children[0] = node->children[0];
    child_node->children[1] = node->children[1];
    child_node->is_intermediate = node->is_intermediate;

    int split_bit = extract_bit(node->prefix, bit_index);

    node->prefix = extract_n_bits(node->prefix, 0, bit_index);
    node->prefix_length = bit_index;
    assert(node->prefix_length >= 0);
    node->is_intermediate = 1;

    node->children[split_bit ? 1 : 0] = child_node;
    node->children[split_bit ? 0 : 1] = NULL;

    return &node->children[split_bit ? 0 : 1];
}

/**
 * Create a sequence of trie nodes for the remainder of the address.
 */
static int
fds_trie_create_nodes(struct trie_node **node, uint32_t *address, int bit_offset, int bit_length)
{
    // Create the nodes leading to the final node
    while (bit_length > 32) {
        *node = trie_node_create();
        if (*node == NULL) {
            return 0;
        }
        (*node)->prefix = extract_n_bits(*address, bit_offset, 31 - bit_offset);
        (*node)->prefix_length = 31 - bit_offset;
        assert((*node)->prefix_length >= 0);
        (*node)->is_intermediate = 1;
        node = &(*node)->children[extract_bit(*address, 31) ? 1 : 0];
        bit_offset = 0;
        bit_length -= 32;
        address++;
    }

    // Create the final node
    *node = trie_node_create();
    if (*node == NULL) {
        return 0;
    }
    (*node)->prefix = extract_n_bits(*address, bit_offset, bit_length - bit_offset);
    (*node)->prefix_length = bit_length - bit_offset;
    (*node)->is_intermediate = 0;

    return 1;
}

/**
 * Find a place in the trie where a new address will be added
 */
static struct trie_node **
trie_node_find_add(struct trie_node **node, struct bit_array *address)
{
    while (*node != NULL) {
        // Not enough remaining bits to compare prefix and advance
        if (bit_array_bits_remaining(address) <= (*node)->prefix_length) {
            return node;
        }

        // Prefix does not match
        if (extract_n_bits(*address->value, address->bit_offset, (*node)->prefix_length) != (*node)->prefix) {
            return node;
        }

        // Walk the trie
        int trailing_bit = extract_bit(*address->value, address->bit_offset + (*node)->prefix_length);
        bit_array_advance(address, (*node)->prefix_length + 1);
        node = &(*node)->children[trailing_bit ? 1 : 0];
    }

    return node;
}

/**
 * Find where a new node should be added added, split nodes along the way.
 */
int
fds_trie_add(struct fds_trie *trie, int ip_version, uint8_t *address_bytes, int bit_length)
{
    assert((ip_version == 4 && bit_length <= 32) || (ip_version == 6 && bit_length <= 128));
    assert(bit_length > 0);

    uint32_t address_words[4] = { 0 };
    ip_address_bytes_to_words(ip_version, address_bytes, address_words);

    struct trie_node **node = ip_version == 4 ? &trie->ipv4_root : &trie->ipv6_root;
    struct bit_array address = bit_array_create(address_words, bit_length);

	node = trie_node_find_add(node, &address);

    // The result is a missing node - just create the nodes at the place
    if (*node == NULL) {
        if (!fds_trie_create_nodes(node, address.value, address.bit_offset, address.bit_length)) {
            return 0;
        }
        return 1;
    }

    // Prefix is longer than the bits remaining in address
    if (bit_array_bits_remaining(&address) < (*node)->prefix_length) {
        int differing_bit = find_differing_bit(*address.value << address.bit_offset, (*node)->prefix);
        // All the bits in the prefix match the bits of the address, split past the end
        if (differing_bit >= bit_array_bits_remaining(&address)) {
            if (trie_node_split_on_bit(*node, bit_array_bits_remaining(&address)) == NULL) {
                return 0;
            }
            (*node)->is_intermediate = 0;
            return 1;
        } else {
            if ((node = trie_node_split_on_bit(*node, differing_bit)) == NULL) {
                return 0;
            }
            bit_array_advance(&address, differing_bit + 1);
            if (!fds_trie_create_nodes(node, address.value, address.bit_offset, address.bit_length)) {
                return 0;
            }
            return 1;
        }
    // Prefix is the same length as the bits remaining in address
    } else if (bit_array_bits_remaining(&address) == (*node)->prefix_length) {
        int differing_bit = find_differing_bit(*address.value << address.bit_offset, (*node)->prefix);
        // All the prefix bits match
        if (differing_bit >= (*node)->prefix_length) {
            // If there are more words to come we have to split at the last bit
            if (bit_array_is_last_word(&address)) {
                (*node)->is_intermediate = 0;
                return 1;
            } else {
                if ((node = trie_node_split_on_bit(*node, (*node)->prefix_length - 1)) == NULL) {
                    return 0;
                }
                bit_array_advance(&address, (*node)->prefix_length);
                if (!fds_trie_create_nodes(node, address.value, address.bit_offset, address.bit_length)) {
                    return 0;
                }
                return 1;
            }
        } else {
            if ((node = trie_node_split_on_bit(*node, differing_bit)) == NULL) {
                return 0;
            }
            bit_array_advance(&address, differing_bit + 1);
            if (!fds_trie_create_nodes(node, address.value, address.bit_offset, address.bit_length)) {
                return 0;
            }
            return 1;
        }
    // Prefix is shorter than the bits remaining in address,
    } else { // bit_array_bits_remaining(&address) > (*node)->prefix_length
        int differing_bit = find_differing_bit(*address.value << address.bit_offset, (*node)->prefix);
        // We know a bit has to differ else we would have advanced further
        assert(differing_bit < (*node)->prefix_length);
        if ((node = trie_node_split_on_bit(*node, differing_bit)) == NULL) {
            return 0;
        }
        bit_array_advance(&address, differing_bit + 1);
        if (!fds_trie_create_nodes(node, address.value, address.bit_offset, address.bit_length)) {
            return 0;
        }
        return 1;
    }
}

/**
 * Find if an ip address is in a trie
 */
bool
fds_trie_find(struct fds_trie *trie, int ip_version, uint8_t *address_bytes, int bit_length)
{
    assert((ip_version == 4 && bit_length <= 32) || (ip_version == 6 && bit_length <= 128));
    assert(bit_length > 0);

    uint32_t address_words[4] = { 0 };
    ip_address_bytes_to_words(ip_version, address_bytes, address_words);
    uint32_t *address = address_words;

    struct trie_node *node = ip_version == 4 ? trie->ipv4_root : trie->ipv6_root;
    int bit_offset = 0;

    while (bit_length > 32 && node != NULL) {
        if (32 - bit_offset <= node->prefix_length
            || extract_n_bits(*address, bit_offset, node->prefix_length) != node->prefix) {
            return 0;
        }
        if (!node->is_intermediate) {
            return 1;
        }
        bit_offset += node->prefix_length;
        node = node->children[extract_bit(*address, bit_offset) ? 1 : 0];
        bit_offset += 1;
        if (bit_offset == 32) {
            bit_offset = 0;
            bit_length -= 32;
            address++;
        }
    }

    while (node != NULL) {
        if (bit_length - bit_offset < node->prefix_length
            || extract_n_bits(*address, bit_offset, node->prefix_length) != node->prefix) {
            return 0;
        }
        if (!node->is_intermediate) {
            return 1;
        }
        bit_offset += node->prefix_length;
        if (bit_offset == bit_length) {
            break;
        }
        node = node->children[extract_bit(*address, bit_offset) ? 1 : 0];
        bit_offset += 1;
    }

    return node != NULL && !node->is_intermediate;
}

/**
 * Print a trie for debug purposes.
 */
void
fds_trie_print(struct fds_trie *trie)
{
    dump_trie_node(trie->ipv4_root, 0, "ipv4 root", 10000);
    dump_trie_node(trie->ipv6_root, 0, "ipv6 root", 10000);
}
