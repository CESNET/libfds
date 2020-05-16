#ifndef FDS_FILTER_DISABLE_TRIE

#include <libfds.h>
#include "trie.h"

int
ip_list_to_trie(fds_filter_value_u *val, fds_filter_value_u *res)
{
    struct fds_trie *trie = fds_trie_create();
    if (!trie) {
        return FDS_ERR_NOMEM;
    }
    for (int i = 0; i < val->list.len; i++) {
        int ret = fds_trie_add(trie, val->list.items[i].ip.version, val->list.items[i].ip.addr, val->list.items[i].ip.prefix);
        if (!ret) {
            return FDS_ERR_NOMEM;
        }
    }
    // free(val->list.items);
    res->p = trie;
    return FDS_OK;
}

void
destroy_trie(fds_filter_value_u *val)
{
    fds_trie_destroy(val->p);
}

void
ip_in_trie(fds_filter_value_u *left, fds_filter_value_u *right, fds_filter_value_u *result)
{
    result->b = fds_trie_find(right->p, left->ip.version, left->ip.addr, left->ip.prefix);
}

const fds_filter_op_s trie_operations[] = {
    FDS_FILTER_DEF_CONSTRUCTOR(FDS_FDT_IP | FDS_FDT_LIST, ip_list_to_trie, FDS_FDT_TRIE),
    FDS_FILTER_DEF_DESTRUCTOR(FDS_FDT_TRIE, destroy_trie),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_IP, "in", FDS_FDT_TRIE, ip_in_trie, FDS_FDT_BOOL),
    FDS_FILTER_END_OP_LIST
};

#endif // FDS_FILTER_ENABLE_TRIE