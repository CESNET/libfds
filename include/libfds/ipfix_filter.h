#ifndef LIBFDS_IPFIX_FILTER_H
#define LIBFDS_IPFIX_FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libfds/api.h>
#include <libfds/iemgr.h>
#include <stdbool.h>

typedef struct fds_ipfix_filter fds_ipfix_filter_t;

/**
 * 
 * Creates an IPFIX filter from an expression.
 *
 * \param[out] ipfil    Pointer to where to allocate and construct the filter
 * \param[in]  iemgr    The information elements manager
 * \param[in]  expr     The filter expression
 * 
 * On error, more detailed information about the error can be obtained using fds_filter_ipfix_get_error.
 * After the filter structure is destroyed the error is destroyed along with it!
 *
 * \return FDS_OK on success, else one of FDS_ERR_NOMEM, FDS_ERR_SYNTAX, FDS_ERR_SEMANTIC.
 */
FDS_API int 
fds_ipfix_filter_create(fds_ipfix_filter_t **ipxfil, fds_iemgr_t *iemgr, const char *expr);

/**
 * Evaluates the IPFIX filter using the provided data record.
 * 
 * \param[in] ipxfil  The IPFIX filter
 * \param[in] record  A data record 
 *
 * \return true if the data record passes the filter, false if not 
 */
FDS_API bool
fds_ipfix_filter_eval(struct fds_ipfix_filter *ipxfil, struct fds_drec *record);

/**
 * Destroys the IPFIX filter.
 * 
 * \param[in] ipxfil  The filter to be destroyed.
 */
FDS_API void
fds_ipfix_filter_destroy(fds_ipfix_filter_t *ipxfil);

/**
 * Gets the error from an IPFIX filter.
 * 
 * If the filter parameter is NULL then memory error is assumed.
 * 
 * \param[in] ipxfil  The IPFIX filter
 * 
 * \return Pointer to the error, or NULL if there is no error.
 */
FDS_API const char *
fds_ipfix_filter_get_error(fds_ipfix_filter_t *ipxfil);

#ifdef __cplusplus
}
#endif

#endif