#include <stdio.h>
#include <libfds.h>
#include <assert.h>


int
ip_list_to_trie(fds_filter_value_u *val, fds_filter_value_u *res)
{
    printf("hello from trie constructor\n");

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

#define FDS_FDT_TRIE (FDS_FDT_CUSTOM | 1) // const int doesn't work here?

const fds_filter_op_s trie_ops[] = {
    FDS_FILTER_DEF_CONSTRUCTOR(FDS_FDT_IP | FDS_FDT_LIST, ip_list_to_trie, FDS_FDT_TRIE),
    FDS_FILTER_DEF_DESTRUCTOR(FDS_FDT_TRIE, destroy_trie),
    FDS_FILTER_DEF_BINARY_OP(FDS_FDT_IP, "in", FDS_FDT_TRIE, ip_in_trie, FDS_FDT_BOOL),
    FDS_FILTER_END_OP_LIST
};

int
main(int argc, char *argv[])
{
    int exit_code = EXIT_SUCCESS;
    int res;
    fds_filter_opts_t *opts = NULL;
    fds_filter_t *filter = NULL;
    
    opts = fds_filter_create_default_opts();
    if (!opts) {
        printf("error: create default opts failed\n");
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

    if (!fds_filter_opts_extend_ops(opts, trie_ops)) {
        printf("error: extend ops failed\n");
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

    const char *expr = "127.0.0.1 in [127.0.0.1, 127.0.0.2, 192.168.1.21, 1.1.1.1, 8.8.8.8, 4.4.4.4]";
    res = fds_filter_create(&filter, expr, opts);
    if (res != FDS_OK) {
        fds_filter_error_s *err = fds_filter_get_error(filter);
        printf("error creating filter: %d: %s\n", err->code, err->msg);
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

    res = fds_filter_eval(filter, NULL);
    if (res) {
        printf("filter passed\n");
    } else {
        printf("filter didn't pass\n");
    }

cleanup:
    fds_filter_destroy(filter);
    fds_filter_destroy_opts(opts);
    return exit_code;
}