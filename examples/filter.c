#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <libfds.h>

// int
// trie_from_ip_list(fds_filter_value_u *val, fds_filter_value_u *res)
// {
//     struct fds_trie *trie = fds_trie_create();
//     if (!trie) {
//         return FDS_ERR_NOMEM;
//     }
//     for (int i = 0; i < val->list.len; i++) {
//         int ret = fds_trie_add(trie, val->list.items[i].ip.version, val->list.items[i].ip.addr, val->list.items[i].ip.prefix);
//         if (!ret) {
//             return FDS_ERR_NOMEM;
//         }
//     }
//     // free(val->list.items);
//     val->p = trie;
//     return FDS_OK;
// }

// void
// ip_in_trie(fds_filter_value_u *lval, fds_filter_value_u *rval, fds_filter_value_u *res)
// {
//     res->b = fds_trie_find(rval->p, lval->ip.version, lval->ip.addr, lval->ip.prefix);
// }

// void
// destroy_trie(fds_filter_value_u *val)
// {
//     fds_trie_destroy(val->p);
// }

// void
// cast_trie_to_bool(fds_filter_value_u *val)
// {
//     val->b = true;
// }

// const int DT_TRIE = FDS_FILTER_DT_CUSTOM | 1;

// fds_filter_op_s ip_trie_ops[] = {
//     FDS_FILTER_DEF_CONSTRUCTOR(FDS_FILTER_DT_LIST | FDS_FILTER_DT_IP, trie_from_ip_list, DT_TRIE),
//     FDS_FILTER_DEF_DESTRUCTOR(DT_TRIE, destroy_trie),
//     FDS_FILTER_DEF_BINARY_OP(FDS_FILTER_DT_IP, "in", DT_TRIE, ip_in_trie, FDS_FILTER_DT_BOOL)
// };

int main(int argc, char *argv[]) {
    if (argc < 0) {
        fprintf(stderr, "Usage: filter <expr>\n");
        return 1;
    }

    fds_filter_opts_t *opts = fds_filter_create_default_opts();
    assert(opts);

    // int ret; 
    // ret = fds_filter_opts_extend_operations(opts, ip_trie_ops, 5);
    // assert(ret == FDS_OK);

    fds_filter_t *filter;
    int ret = fds_filter_create(&filter, argv[1], opts);
    if (ret != FDS_OK) {
        fds_filter_error_s *err = fds_filter_get_error(filter);
        printf("(%d) %s\n", err->code, err->msg);
        printf("%s\n", argv[1]);
        const char *p = argv[1];
        while (p < err->cursor_begin) { printf(" "); p++; }
        while (p < err->cursor_end) { printf("^"); p++; }
        printf("\n");
        fds_filter_destroy(filter);
        fds_filter_destroy_opts(opts);
        return 1;
    }
    fds_filter_eval(filter, NULL);
    fds_filter_destroy(filter);
    fds_filter_destroy_opts(opts);
    return 0;
}
