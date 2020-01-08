#include "common.h"
#include "opts.h"

#include "operations/float.h"
#include "operations/int.h"
#include "operations/ip.h"
#include "operations/mac.h"
#include "operations/str.h"
#include "operations/uint.h"
#include "operations/other.h"

static int
dummy_lookup_callback(const char *name, int *out_id, int *out_datatype, int *out_flags)
{
    (void)(name);
    (void)(out_id);
    (void)(out_datatype);
    (void)(out_flags);
    return FDS_ERR_NOTFOUND;
}

static void
dummy_const_callback(int id, fds_filter_value_u *out_value)
{
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

    opts->op_list = array_create(sizeof(fds_filter_op_s));

    if (!array_extend_front(&opts->op_list, (void *)other_operations, num_other_operations)) {
        goto error;    
    }
    if (!array_extend_front(&opts->op_list, (void *)int_operations, num_int_operations)) {
        goto error;    
    }
    if (!array_extend_front(&opts->op_list, (void *)uint_operations, num_uint_operations)) {
        goto error;    
    }
    if (!array_extend_front(&opts->op_list, (void *)float_operations, num_float_operations)) {
        goto error;    
    }
    if (!array_extend_front(&opts->op_list, (void *)str_operations, num_str_operations)) {
        goto error;    
    }
    if (!array_extend_front(&opts->op_list, (void *)ip_operations, num_ip_operations)) {
        goto error;    
    }
    if (!array_extend_front(&opts->op_list, (void *)mac_operations, num_mac_operations)) {
        goto error;    
    }

    return opts;

error:
    fds_filter_destroy_opts(opts);
    return NULL;
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

int
fds_filter_opts_add_ops(fds_filter_opts_t *opts, fds_filter_op_s operation)
{
    return array_push_front(&opts->op_list, &operation) 
        ? FDS_OK 
        : FDS_ERR_NOMEM;
}

int
fds_filter_opts_extend_ops(fds_filter_opts_t *opts,
    fds_filter_op_s *operations, size_t num_operations)
{
    return array_extend_front(&opts->op_list, operations, num_operations) 
        ? FDS_OK
        : FDS_ERR_NOMEM;
}

void
fds_filter_destroy_opts(fds_filter_opts_t *opts)
{
    array_destroy(&opts->op_list);
    free(opts);
}
