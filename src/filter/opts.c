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
    UNUSED(name);
    UNUSED(out_id);
    UNUSED(out_datatype);
    UNUSED(out_flags);
    return FDS_FILTER_ERROR;
}

static void
dummy_const_callback(int id, value_t *out_value)
{
    UNUSED(id);
    UNUSED(out_value);
}

static int
dummy_field_callback(void *user_ctx, bool reset_ctx, int id, value_t *out_value)
{
    UNUSED(user_ctx);
    UNUSED(reset_ctx);
    UNUSED(id);
    UNUSED(out_value);
    return FDS_FILTER_NOT_FOUND;
}

struct fds_filter_opts_s *
fds_filter_create_default_opts()
{
    struct fds_filter_opts_s *opts = malloc(sizeof(struct fds_filter_opts_s));
    if (!opts) {
        return NULL;
    }

    opts->lookup_callback = dummy_lookup_callback;
    opts->const_callback = dummy_const_callback;
    opts->field_callback = dummy_field_callback;

    opts->operations = array_make(sizeof(struct fds_filter_operation_s));

    if (!array_extend_front(&opts->operations, (void *)other_operations, num_other_operations)) {
        goto error;    
    }
    if (!array_extend_front(&opts->operations, (void *)int_operations, num_int_operations)) {
        goto error;    
    }
    if (!array_extend_front(&opts->operations, (void *)uint_operations, num_uint_operations)) {
        goto error;    
    }
    if (!array_extend_front(&opts->operations, (void *)float_operations, num_float_operations)) {
        goto error;    
    }
    if (!array_extend_front(&opts->operations, (void *)str_operations, num_str_operations)) {
        goto error;    
    }
    if (!array_extend_front(&opts->operations, (void *)ip_operations, num_ip_operations)) {
        goto error;    
    }
    if (!array_extend_front(&opts->operations, (void *)mac_operations, num_mac_operations)) {
        goto error;    
    }

    return opts;

error:
    fds_filter_destroy_opts(opts);
    return NULL;
}

void
fds_filter_opts_set_lookup_callback(fds_filter_opts_t *opts, fds_filter_lookup_callback_t *lookup_callback)
{
    opts->lookup_callback = lookup_callback;
}

void
fds_filter_opts_set_const_callback(fds_filter_opts_t *opts, fds_filter_const_callback_t *const_callback)
{
    opts->const_callback = const_callback;
}

void
fds_filter_opts_set_field_callback(fds_filter_opts_t *opts, fds_filter_field_callback_t *field_callback)
{
    opts->field_callback = field_callback;
}

int
fds_filter_opts_add_operation(fds_filter_opts_t *opts, struct fds_filter_operation_s operation)
{
    return array_push_front(&opts->operations, &operation) ? FDS_FILTER_OK : FDS_FILTER_ERR_NOMEM;
}

int
fds_filter_opts_extend_operations(fds_filter_opts_t *opts, struct fds_filter_operation_s *operations, size_t num_operations)
{
    return array_extend_front(&opts->operations, operations, num_operations) ? FDS_FILTER_OK : FDS_FILTER_ERR_NOMEM;
}

void
fds_filter_destroy_opts(fds_filter_opts_t *opts)
{
    array_destroy(&opts->operations);
    free(opts);
}
