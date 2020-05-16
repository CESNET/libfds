#ifndef FDS_FILTER_DISABLE_TRIE

#ifndef FDS_FILTER_OPERATIONS_TRIE_H
#define FDS_FILTER_OPERATIONS_TRIE_H

#define FDS_FDT_TRIE (FDS_FDT_CUSTOM | 1) // const int doesn't work here?

extern const fds_filter_op_s trie_operations[];

#endif // FDS_FILTER_OPERATIONS_TRIE_H

#endif // FDS_FILTER_ENABLE_TRIE