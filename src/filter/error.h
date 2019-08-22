#ifndef FDS_FILTER_ERROR_H
#define FDS_FILTER_ERROR_H

#include <libfds.h>

struct error {
    char *message;
    struct fds_filter_location location;
};

struct error_list {
    int count;
    struct error *errors;
};

struct error_list *
create_error_list();

void
no_memory_error(struct error_list *error_list);

void
add_error_message(struct error_list *error_list, const char *format, ...);

void
add_error_location_message(struct error_list *error_list, struct fds_filter_location location, const char *format, ...);

void
destroy_error_list(struct error_list *error_list);

#endif // FDS_FILTER_ERROR_H