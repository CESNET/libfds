#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <libfds.h>

int main(int argc, char *argv[]) {
    if (argc < 0) {
        fprintf(stderr, "Usage: filter <expr>\n");
        return 1;
    }

    fds_filter_opts_t *opts = fds_filter_create_default_opts();
    assert(opts);

    fds_filter_t *filter;
    int ret = fds_filter_create(argv[1], opts, &filter);
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
