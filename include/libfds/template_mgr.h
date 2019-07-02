/**
 * \file   include/libfds/template_mgr.h
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \brief  Template manager (header file)
 * \date   October 2017
 */

/*
 * Copyright (C) 2017 CESNET, z.s.p.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is'', and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */

#ifndef FDS_TEMPLATE_MANAGER_H
#define FDS_TEMPLATE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <libfds/api.h>
#include "template.h"
#include "iemgr.h"

/**
 * \defgroup fds_template_mgr IPFIX Template manager
 * \ingroup publicAPIs
 * \brief Template management
 *
 * Main goal of the Template manager is to make ensure, to the extend possible, that Exporting and
 * Collecting process have a consistent view of the Template and Options Template used to encode
 * and decode IPFIX records sent from the Exporting Process to the Collecting Process.
 * Template manager supports IPFIX template sent over SCTP, UDP, TCP and FILE. Based on selected
 * export method, internal rules are set to comply with required behavior. For example, in case of
 * export over UDP, withdrawal requests are not accepted etc.
 *
 * Template management supports standard operations such as adding/redefining/withdrawing templates.
 * Since there is no guarantee of ordering of exported IPFIX Messages across SCTP streams and
 * over UDP, all template management actions (i.e. defining new Templates, withdrawing them, etc.)
 * are sequenced using Export Time field in the IPFIX Message header. Therefore, all function that
 * directly manipulate with Template definitions must know recent Export Time.
 *
 * Template manager allows to create consistent snapshots of Templates valid at a certain time
 * of processing. This can be useful if it is necessary to access/find templates later. Snapshot
 * is not affected by later modification of Templates (adding/redefining/etc.).
 *
 * Example usage of the manager:
 * \code{.c}
 *   // Initialize
 *   fds_tmgr_create(...);
 *   fds_tmgr_set_udp_timeouts(...); // optional, UDP only
 *   fds_tmgr_set_iemgr(...);        // optional
 *
 *   while (until end) {
 *     fds_tmgr_set_time(...);       // MUST be called BEFORE processing each packet !!!
 *
 *     // multiple times any combination of:
 *       fds_tmgr_snapshot_get(...);
 *       fds_tmgr_template_get(...);
 *       fds_tmgr_template_add(...);
 *       fds_tmgr_template_withdraw(...);
 *       fds_tmgr_template_withdraw_all(...);
 *       fds_tmgr_template_remove(...);
 *
 *     // Cleanup of old snapshots/templates (usually after modification add/withdraw/remove)
 *     fds_tmgr_garbage_get(...);
 *   }
 *
 *   // Destruction
 *   fds_tmgr_destroy(...);
 * \endcode
 *
 * \warning
 *   Keep on mind that after any memory allocation error, consistency of the templates
 *   cannot be guaranteed! We strongly recommend to delete the manager and close a transport
 *   session..
 * \remark Based on RFC 7011 (see https://tools.ietf.org/html/rfc7011)
 * @{
 */

/** \brief Session type of a flow source                                     */
enum fds_session_type {
    FDS_SESSION_UDP,        /**< IPFIX over UDP transfer protocol            */
    FDS_SESSION_TCP,        /**< IPFIX over TCP transfer protocol            */
    FDS_SESSION_SCTP,       /**< IPFIX over SCTP transfer protocol           */
    FDS_SESSION_FILE        /**< IPFIX from IPFIX File format                */
};

/** Internal template manager declaration   */
typedef struct fds_tmgr fds_tmgr_t;
/** Internal template snapshot declaration  */
typedef struct fds_tsnapshot fds_tsnapshot_t;
/** Internal template garbage declaration   */
typedef struct fds_tgarbage fds_tgarbage_t;


/**
 * \brief Create a new template manager
 *
 * This manager is able to handle templates that belong to a combination of the Transport Session
 * and Observation Domain. To configure allowed and prohibited behavior of template (re)definition
 * and withdrawing, the type of session must be configured.
 * \note In case of UDP session, Template and Option Template timeouts should be configured using
 *   fds_tmgr_set_udp_timeouts() function. By default, all timeouts are disabled.
 * \param[in] type Session type
 * \return Pointer to the manager or NULL (usually memory allocation error)
 */
FDS_API fds_tmgr_t *
fds_tmgr_create(enum fds_session_type type);

/**
 * \brief Destroy a template manager
 * \warning The function, among the other, will immediately destroy all templates and snapshots
 *   that are stored inside the manager. If there are any references to these templates/snapshot,
 *   you must wait until you can guarantee that no one is referencing them OR you can move them
 *   into garbage (see fds_tmgr_clear()) and then generate a garbage message that must be later
 *   destroyed (see fds_tmgr_garbage_get()). In the latter case, you can safely destroy the
 *   manager, but the garbage must remain until references exist.
 * \param[in] tmgr Template manager
 */
FDS_API void
fds_tmgr_destroy(fds_tmgr_t *tmgr);

/**
 * \brief Move all templates and snapshots to garbage
 *
 * After cleaning, the template manager will be the same as a newly create manager, except the
 * configuration parameters (timeouts, IE manager, etc.) that will be preserved.
 * \note Garbage should be retrieved using fds_tmgr_garbage_get() function.
 * \warning Time context will be lost.
 * \param[in] tmgr Template manager
 */
FDS_API void
fds_tmgr_clear(fds_tmgr_t *tmgr);

/**
 * \brief Collect internal garbage
 *
 * All unreachable (or old) templates and snapshots will be moved an internal garbage structure
 * that will be returned. If no garbage is available, the pointer will be set to NULL.
 * \param[in]  tmgr Template manager
 * \param[out] gc   Garbage structure
 * \return On success returns #FDS_OK and the garbage pointer is set. The pointer will be set to
 *   NULL, if no garbage is available. Otherwise returns #FDS_ERR_NOMEM and the pointer is
 *   undefined.
 */
FDS_API int
fds_tmgr_garbage_get(fds_tmgr_t *tmgr, fds_tgarbage_t **gc);

/**
 * \brief Destroy all snapshots and templates in a garbage
 *
 * The garbage structure will be destroyed too.
 * \param[in] gc Garbage
 */
FDS_API void
fds_tmgr_garbage_destroy(fds_tgarbage_t *gc);

/**
 * \brief Set (Options) Template lifetime (UDP session only)
 *
 * (Options) Templates that are not received again (i.e. not refreshed by the Exporting Process
 * or someone else) within the configured lifetime become invalid and then automatically
 * discarded (moved to garbage) by the manager. All timeout are related to Export Time
 * (see fds_tmgr_set_time()).
 *
 * \warning Only newly added/refreshed/redefined templates will have new timeout. Already present
 * templates will be unaffected.
 * \note To disable timeout, use value 0. In this case, templates exists throughout the whole
 *   existence of the manager or until they are redefined/updated by another template with the
 *   same ID.
 * \param[in] tmgr    Template manager
 * \param[in] tl_data Timeout of Data Templates (in seconds)
 * \param[in] tl_opts Timeout of Optional Templates (in seconds)
 * \return On success returns #FDS_OK. Otherwise (invalid session type) returns #FDS_ERR_ARG.
 */
FDS_API int
fds_tmgr_set_udp_timeouts(fds_tmgr_t *tmgr, uint16_t tl_data, uint16_t tl_opts);

/**
 * \brief Set timeout of template snapshots
 *
 * This parameter defines how many seconds of template history will be available.
 * \note To disable timeout, use value 0. In this case, no historical snapshots are accessible
 *   (SCTP, UDP and FILE). In case of TCP session, this configuration doesn't have any impact
 *   because the history is always disabled. In other words, if the timeout is disabled, export
 *   time for fds_tmgr_set_time() function can be only increasing.
 * \warning High values have a significant impact on performance and memory consumption.
 *   The recommended range of the timeout value is 0 - 30.
 * \param[in] tmgr    Template manager
 * \param[in] timeout Timeout (in seconds)
 */
FDS_API void
fds_tmgr_set_snapshot_timeout(fds_tmgr_t *tmgr, uint16_t timeout);

/**
 * \brief Add a reference to a IE manager and redefine all fields
 *
 * All templates require the manager to determine a definition (a type, semantic, etc.) of
 * each template fields. If the manager is not defined or a definition of a field is missing,
 * the field cannot be properly interpreted and some information about the template are unknown.
 *
 * \warning Time context will be lost.
 * \warning If the manager already contains another manager, all references to definitions will be
 *   overwritten with new ones. If a definition of an IE was previously available in the older
 *   manager and the new manager doesn't include the definition, the definition will be removed
 *   and corresponding fields will not be interpretable.
 *   Technically, all templates and snapshots are replaces with new copies and old copies are moved
 *   to the garbage that should be retrieved using fds_tmgr_garbage_get().
 * \warning If the manager \p iemgr is undefined (i.e. NULL), all references will be removed.
 * \warning Keep in mind this operation is VERY expensive if the manager already includes templates
 *   and snapshots!
 * \param[in] tmgr  Template manager
 * \param[in] iemgr Manager of Information Elements (IEs) (can be NULL)
 * \return On success returns #FDS_OK. Otherwise returns #FDS_ERR_NOMEM and references are still
 *   the same.
 */
FDS_API int
fds_tmgr_set_iemgr(fds_tmgr_t *tmgr, const fds_iemgr_t *iemgr);

/**
 * \brief Set current time context of a processed packet
 *
 * In a header of each IPFIX message is present so called "Export time" that helps to determine
 * a context in which it should be processed.
 *
 * In case of unreliable transmission (such as UDP, SCTP-PR), an IPFIX packet could be received
 * out of order i.e. it may be delayed. Because a scope of validity of template definitions is
 * directly connected with Export Time and the definitions can change from time to time,
 * the Export Time of the processing packet is necessary to determine to which template flow
 * records belong.
 *
 * In case of reliable transmission (such as TCP, SCTP), the Export time helps to detect wrong
 * behavior of an exporting process. For example, a TCP connection must be always reliable, thus
 * Export Time must be only increasing (i.e. the same or greater).
 *
 * \param[in] tmgr     Template manager
 * \param[in] exp_time Export time
 * \return On success returns #FDS_OK.
 *   In case of TCP session and setting time in history, the function will return #FDS_ERR_DENIED.
 *   If the \p exp_time is in history and the difference between the newest time and the export
 *   time (\p exp_time) is greater that the snapshot timeout, the function will not change context
 *   and return #FDS_ERR_NOTFOUND (In other words, such old data is no longer available).
 *   On memory allocation error returns #FDS_ERR_NOMEM.
 */
FDS_API int
fds_tmgr_set_time(fds_tmgr_t *tmgr, uint32_t exp_time);

/**
 * \brief Get a reference to a template
 *
 * \warning This operation is related to a context (determined by the current Export Time).
 *   For more information see: fds_tmgr_set_time().
 * \param[in]  tmgr  Template manager
 * \param[in]  id    Identification number of the template
 * \param[out] tmplt Pointer to the template
 * \return #FDS_OK on success and the pointer \p tmplt is filled.
 *   #FDS_ERR_NOTFOUND, if the template is not present in the manager.
 *   #FDS_ERR_NOMEM, if a memory allocation error has occurred.
 *   #FDS_ERR_ARG, if the time context is not defined.
 */
FDS_API int
fds_tmgr_template_get(fds_tmgr_t *tmgr, uint16_t id, const struct fds_template **tmplt);

/**
 * \brief Add a template
 *
 * Based on the session type, try to add the template in the snapshot. References to
 * IE definitions will be added to template's elements and the lifetime of the template will
 * be configured appropriately to the manager configuration.
 *
 * \warning Templates withdrawals cannot be added!
 * \warning This operation is related to a context (determined by the current Export Time).
 *   For more information see: fds_tmgr_set_time().
 * \param[in] tmgr   Template manager
 * \param[in] tmplt  Pointer to the new template
 * \return On success returns #FDS_OK and the manager will take responsibility for the template.
 *   In case of any following error, the template will not be part of the manager and it must
 *   be freed by the user. On memory allocation error returns #FDS_ERR_NOMEM.
 *   If the operation is not allowed for this template and session combination
 *     returns #FDS_ERR_DENIED.
 *   If the template is template withdrawal or the time context is not defined
 *     returns #FDS_ERR_ARG.
 */
FDS_API int
fds_tmgr_template_add(fds_tmgr_t *tmgr, struct fds_template *tmplt);

/**
 * \brief Withdraw a template
 *
 * The function will try to withdraw the template with a given ID and an expected template \p type.
 * If \p type is ::FDS_TYPE_TEMPLATE_UNDEF, type check is skipped.
 *
 * \warning This operation is related to a context (determined by the current Export Time).
 *   For more information see: fds_tmgr_set_time().
 * \param[in] tmgr Template manager
 * \param[in] id   Template ID
 * \param[in] type Expected type of the template
 * \return On success returns #FDS_OK.
 *   If a memory allocation error has occurred returns #FDS_ERR_NOMEM.
 *   If the time context is not defined returns #FDS_ERR_ARG.
 *   If the operation is not allowed for this template and session combination (for example,
 *     prohibited history modification or disabled withdrawal messages) returns #FDS_ERR_DENIED.
 *   If the template is not defined returns #FDS_ERR_NOTFOUND.
 *   If the template exists in the manager, but the \p type of template doesn't match,
 *     returns #FDS_ERR_ARG and the template stays in the manager.
 */
FDS_API int
fds_tmgr_template_withdraw(fds_tmgr_t *tmgr, uint16_t id, enum fds_template_type type);

/**
 * \brief Withdraw all templates
 *
 * The function will try to withdraw all templates with a given \p type.
 * If \p type is ::FDS_TYPE_TEMPLATE_UNDEF, all templates will be withdrawn.
 *
 * \warning This operation is related to a context (determined by the current Export Time).
 *   For more information see: fds_tmgr_set_time().
 * \param[in] tmgr Template manager
 * \param[in] type Type of the templates to withdraw
 * \return On success returns #FDS_OK.
 *   If a memory allocation error has occurred returns #FDS_ERR_NOMEM.
 *   If the time context is not defined returns #FDS_ERR_ARG.
 *   If the operation is not for this session returns #FDS_ERR_DENIED.
 */
FDS_API int
fds_tmgr_template_withdraw_all(fds_tmgr_t *tmgr, enum fds_template_type type);

/**
 * \brief Remove a template
 *
 * If you want to withdraw a template, this is not the function you are looking for. See functions
 * fds_tmgr_template_withdraw() and fds_tmgr_template_withdraw_all().
 *
 * This function will remove a specific template from _whole_ history of the template manager with
 * a given \p type. This can be useful in case of UDP communication when an invalid record of the
 * specific is received. The template will be removed without impact on the other templates.
 * If \p type is ::FDS_TYPE_TEMPLATE_UNDEF, type check is skipped.

 * \note The function ignores default history modification rules.
 * \warning Keep in mind that removing a template from history is VERY expensive operation. If it
 *   is not necessary, try to avoid calling this function.
 * \param[in] tmgr Template manager
 * \param[in] id   Template ID to remove
 * \param[in] type Expected type of the template
 * \return On success returns #FDS_OK. Otherwise returns #FDS_ERR_NOMEM.
 */
FDS_API int
fds_tmgr_template_remove(fds_tmgr_t *tmgr, uint16_t id, enum fds_template_type type);

/**
 * \brief Assign a flow key to a template
 *
 * In case of modification of history, the flow key will be propagated to the same templates
 * (based on Template ID) of newer snapshots if the templates are not redefined.
 *
 * \warning This operation is related to a context (determined by the current Export Time).
 *   For more information see: fds_tmgr_set_time().
 * \param[in] tmgr Template manager
 * \param[in] id   Template ID
 * \param[in] key  Flow key
 * \return On success returns #FDS_OK.
 *   If the template is not present in the manager returns #FDS_ERR_NOTFOUND.
 *   If the template is present in the manager but the flow key is too long, returns #FDS_ERR_ARG
 *     and the flow key is not applied.
 *   If a memory allocation error has occurred returns #FDS_ERR_NOMEM.
 *   If the time context is not defined returns #FDS_ERR_ARG.
 */
FDS_API int
fds_tmgr_template_set_fkey(fds_tmgr_t *tmgr, uint16_t id, uint64_t key);

/**
 * \brief Get a snapshot of valid templates
 *
 * \warning This operation is related to a context (determined by the current Export Time).
 *   For more information see: fds_tmgr_set_time().
 * \param[in]  tmgr Template manager
 * \param[out] snap Snapshot
 * \return On success returns #FDS_OK and sets the pointer \p snap to the snapshot.
 *   If a memory allocation error has occurred returns #FDS_ERR_NOMEM.
 *   If the time context is not defined returns #FDS_ERR_ARG.
 */
FDS_API int
fds_tmgr_snapshot_get(const fds_tmgr_t *tmgr, const fds_tsnapshot_t **snap);

/**
 * \brief Find a template in a snapshot
 * \param[in] snap Snapshot
 * \param[in] id   Template ID
 * \return Pointer or NULL, if not present.
 */
FDS_API const struct fds_template *
fds_tsnapshot_template_get(const fds_tsnapshot_t *snap, uint16_t id);


/**
 * \brief Function callback for processing an IPFIX (Options) Template in a snapshot
 *
 * \param[in] tmplt Template to process
 * \param[in] data  User defined data for the callback (optional, can be NULL)
 *
 * \warning
 *   Propagation of C++ exceptions is not supported! In other words, no exceptions must be
 *   thrown in the callback function!
 * \return True if the iteration should continue.
 * \return False if the iteration should be immediately terminated.
 */
typedef bool (*fds_tsnapshot_for_cb)(const struct fds_template *tmplt, void *data);

/**
 * \brief Call a function on each IPFIX (Options) Template in a snapshot
 *
 * This function allows you to effectively iterate over all IPFIX (Options) Templates defined
 * in the snapshot. It is guaranteed that the templates will be processed in the order given by
 * their Template ID in ascending order.
 * \param[in] snap Snapshot to iterate over
 * \param[in] cb   Callback function to be called for each IPFIX (Options) Template
 * \param[in] data User defined data that will be passed to the callback function (can be NULL)
 */
FDS_API void
fds_tsnapshot_for(const fds_tsnapshot_t *snap, fds_tsnapshot_for_cb cb, void *data);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif
#endif // FDS_TEMPLATE_MANAGER_H
