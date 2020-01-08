# Filter

## Example usage
```
const char *expr = "ip in [127.0.0.1, 192.168.1.1] and port in [80, 443]"

// The opts structure holds information about callbacks and filter operations.
// The default opts structure contains dummy callbacks and the basic filter operations.
// One opts structure can be used with multiple filters and is not mutated by the filter in any way.
// It is dynamically allocated and has to be destroyed by the call to fds_filter_destroy_opts.
fds_filter_opts_t *opts = fds_filter_create_default_opts();

// The callbacks can be (and are expected to be) set using the following functions.
fds_filter_opts_set_lookup_cb(opts, my_lookup_cb); 
fds_filter_opts_set_const_cb(opts, my_const_cb);
fds_filter_opts_set_data_cb(opts, my_data_cb);

// Creating the filter from an expression 
fds_filter_t *filter;
int ret = fds_filter_create(&filter, expr, opts);

if (ret != FDS_OK) {
    // Filter creation failed.
    // More detailed information about the failure can be obtained using the fds_filter_get_error 
    // function, and inspecting the fds_filter_error_s structure.
    fds_filter_error_s *err = fds_filter_get_error(filter);
    printf("Error code: %d, messsage: %s\n", err->code, err->msg);

    // The error structure also contains field cursor_begin and cursor_end which are pointers 
    // to the input expressions indicating where exactly the error occured, if it's relevant 
    // to the error.
    if (err->code == FDS_ERR_SYNTAX || err->code == FDS_ERR_SEMANTIC) {
        printf("Error occured at position %d:%d\n", err->cursor_begin - expr, err->cursor_end - expr);
    }

    // The fds_filter_t has to be destroyed even if the creation failed using fds_filter_destroy.
    // The fds_filter_error_s is a part of the fds_filter_t structure and is destroyed with 
    // the destruction of the filter.
    fds_filter_destroy(filter);

    // The opts have to be destroyed separately as multiple filter instances can use the same opts.    
    fds_filter_destroy_opts(opts);

    exit(1);
}

// Filter created successfully.
fds_filter_eval(filter, NULL);

// Clean-up
fds_filter_destroy(filter);
fds_filter_destroy_opts(opts);
```