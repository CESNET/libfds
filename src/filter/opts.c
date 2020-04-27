#include "opts.h"

#include "operations/float.h"
#include "operations/int.h"
#include "operations/ip.h"
#include "operations/mac.h"
#include "operations/str.h"
#include "operations/uint.h"
#include "operations/flags.h"

static int
dummy_lookup_callback(void *user_ctx, const char *name, const char *other_name, int *out_id, int *out_datatype, int *out_flags)
{
    (void)(user_ctx);
    (void)(name);
    (void)(other_name);
    (void)(out_id);
    (void)(out_datatype);
    (void)(out_flags);
    return FDS_ERR_NOTFOUND;
}

static void
dummy_const_callback(void *user_ctx, int id, fds_filter_value_u *out_value)
{
    (void)(user_ctx);
    (void)(id);
    (void)(out_value);
}

static int
dummy_data_callback(void *user_ctx, bool reset_ctx, int id, void *data, fds_filter_value_u *out_value)
{
    (void)(user_ctx);
    (void)(reset_ctx);
    (void)(id);
    (void)(data);
    (void)(out_value);
    return FDS_ERR_NOTFOUND;
}

static void
print_op_list(FILE *out, fds_filter_op_s *op_list)
{
    for (fds_filter_op_s *op = op_list; op->symbol != NULL; op++) {
        print_operation(out, op);
        fprintf(out, "\n");
    }
}

static inline int
op_list_count(fds_filter_op_s *op_list)
{
    if (!op_list) {
        return 0;
    }
    int cnt = 0;
    while (op_list[cnt].symbol != NULL) {
        cnt++;
    }
    return cnt;
}

fds_filter_op_s *
fds_filter_opts_add_op(fds_filter_opts_t *opts, fds_filter_op_s op)
{
    int cnt = op_list_count(opts->op_list) + 1;
    void *tmp = realloc(opts->op_list, (cnt + 1) * sizeof(fds_filter_op_s));
    if (!tmp) {
        return NULL;
    }
    opts->op_list = tmp;
    memmove(opts->op_list + 1, opts->op_list, cnt * sizeof(fds_filter_op_s));
    opts->op_list[0] = op;
    return opts->op_list; 
}

fds_filter_op_s *
fds_filter_opts_extend_ops(fds_filter_opts_t *opts, const fds_filter_op_s *op_list)
{
    int cnt = op_list_count(opts->op_list) + 1; // + 1 for the trailing null op
    int extend_cnt = op_list_count(op_list);
    void *tmp = realloc(opts->op_list, (cnt + extend_cnt) * sizeof(fds_filter_op_s));
    if (!tmp) {
        return NULL;
    }
    opts->op_list = tmp;
    memmove(opts->op_list + extend_cnt, opts->op_list, cnt * sizeof(fds_filter_op_s));
    memcpy(opts->op_list, op_list, extend_cnt * sizeof(fds_filter_op_s));
    return opts->op_list;
}

fds_filter_opts_t *
fds_filter_create_default_opts()
{
    fds_filter_opts_t *opts = malloc(sizeof(fds_filter_opts_t));
    if (!opts) {
        return NULL;
    }

    opts->lookup_cb = dummy_lookup_callback;
    opts->const_cb = dummy_const_callback;
    opts->data_cb = dummy_data_callback;

    opts->op_list = malloc(sizeof(fds_filter_op_s));
    if (!opts->op_list) {
        goto error;
    }
    opts->op_list[0] = (fds_filter_op_s) FDS_FILTER_END_OP_LIST;

    if (!fds_filter_opts_extend_ops(opts, int_operations)) {
        goto error;    
    }
    if (!fds_filter_opts_extend_ops(opts, uint_operations)) {
        goto error;    
    }
    if (!fds_filter_opts_extend_ops(opts, float_operations)) {
        goto error;    
    }
    if (!fds_filter_opts_extend_ops(opts, str_operations)) {
        goto error;    
    }
    if (!fds_filter_opts_extend_ops(opts, ip_operations)) {
        goto error;    
    }
    if (!fds_filter_opts_extend_ops(opts, mac_operations)) {
        goto error;    
    }
    if (!fds_filter_opts_extend_ops(opts, flags_operations)) {
        goto error;    
    }

    return opts;

error:
    fds_filter_destroy_opts(opts);
    return NULL;
}

fds_filter_opts_t *
fds_filter_opts_copy(fds_filter_opts_t *original_opts)
{
    fds_filter_opts_t *opts = malloc(sizeof(fds_filter_opts_t));
    *opts = *original_opts;
    int cnt = op_list_count(original_opts->op_list);
    // + 1 for the end op
    opts->op_list = malloc((cnt + 1) * sizeof(fds_filter_op_s));
    if (!opts->op_list) {
        free(opts);
        return NULL;
    }
    memcpy(opts->op_list, original_opts->op_list, cnt + 1);
    return opts;
}

void
fds_filter_opts_set_lookup_cb(fds_filter_opts_t *opts, fds_filter_lookup_cb_t *cb)
{
    opts->lookup_cb = cb;
}

void
fds_filter_opts_set_const_cb(fds_filter_opts_t *opts, fds_filter_const_cb_t *cb)
{
    opts->const_cb = cb;
}

void
fds_filter_opts_set_data_cb(fds_filter_opts_t *opts, fds_filter_data_cb_t *cb)
{
    opts->data_cb = cb;
}

void
fds_filter_opts_set_user_ctx(fds_filter_opts_t *opts, void *user_ctx)
{
    opts->user_ctx = user_ctx;
}

void *
fds_filter_opts_get_user_ctx(const fds_filter_opts_t *opts)
{
    return opts->user_ctx;
}

void
fds_filter_destroy_opts(fds_filter_opts_t *opts)
{
    free(opts->op_list);
    free(opts);
}
