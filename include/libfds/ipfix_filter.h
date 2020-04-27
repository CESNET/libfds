#ifndef LIBFDS_IPFIX_FILTER_H
#define LIBFDS_IPFIX_FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libfds/api.h>
#include <libfds/iemgr.h>
#include <stdbool.h>

typedef struct fds_ipfix_filter fds_ipfix_filter_t;

FDS_API int 
fds_ipfix_filter_create(fds_ipfix_filter_t **ipxfil, fds_iemgr_t *iemgr, const char *expr);

FDS_API bool
fds_ipfix_filter_eval(struct fds_ipfix_filter *ipxfil, struct fds_drec *record);

FDS_API void
fds_ipfix_filter_destroy(fds_ipfix_filter_t *ipxfil);

FDS_API const char *
fds_ipfix_filter_get_error(fds_ipfix_filter_t *ipxfil);

#ifdef __cplusplus
}
#endif

#endif