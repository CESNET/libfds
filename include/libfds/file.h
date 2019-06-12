/**
 * @file   libfds/file.h
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @brief  Data record in the IPFIX message (header file)
 * @date   June 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBFDS_FILE_H
#define LIBFDS_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include <libfds/api.h>
#include "drec.h"

/// Internal instance of the file object
typedef struct fds_file_s fds_file_t;
/// Internal Transport Session ID identifier
typedef uint16_t fds_file_sid_t;

/**
 * @defgroup fds_file FDS File
 * @brief API for reading and writing flow records into libfds file
 *
 * Flow Data Storage (FDS) file allows you to write/read IPFIX Data Records.
 *
 * Writer example:
 * @code{.c}
 *   fds_file_t *file;
 *   int rc;
 *
 *   // Create a new file for writing with enabled LZ4 compression
 *   file = fds_file_init();
 *   if (!file) { _error_handling_ };
 *   rc = fds_file_open(file, "path/to/file", FDS_FILE_WRITE | FDS_FILE_LZ4);
 *   if (rc != FDS_OK) { _error_handling_ };
 *
 *   // Defined at least one Transport Session (i.e. description of flow source)
 *   struct fds_file_session session;
 *   memset(&session, 0, sizeof(session));
 *   // ... fill the session structure ...
 *
 *   fds_file_sid_t sid;
 *   rc = fds_file_session_add(file, &session, &sid);
 *   if (rc != FDS_OK) { _error_handling_ };
 *
 *   // Set the current writer context (Transport Session, ODID, and export time)
 *   uint32_t odid = ...;
 *   uint32_t exp_time = ...;
 *
 *   rc = fds_file_write_ctx(file, sid, odid, exp_time);
 *   if (rc != FDS_OK) { _error_handling_ };
 *
 *   // Add one or more (Options) Templates
 *   const uint8_t *tmplt_rec = ...;
 *   uint16_t tmplt_size = ...;
 *   uint16_t tmplt_type = ...;
 *
 *   rc = fds_file_write_tmplt_add(file, tmplt_type, tmplt_rec, tmplt_size);
 *   if (rc != FDS_OK) { _error_handling_ };
 *
 *   // Write one or more Data Records based on the previously configured Templates
 *   const uint8_t rec_data = ...;
 *   uint16_t rec_size = ...;
 *   uint16_t rec_tmplt_id = ...;
 *
 *   rc = fds_file_write_rec(file, rec_tmplt_id, rec_data, rec_size);
 *   if (rc != FDS_OK) { _error_handling_ };
 *
 *   // Flush buffers, free necessary resources and close the file
 *   fds_file_close(file);
 * @endcode
 *
 * READER example:
 * @code{.c}
 *   fds_iemgr_t *iemgr;
 *   fds_file_t *file;
 *   int rc;
 *
 *   // Load definitions of IPFIX Information Elements (IEs)
 *   iemgr = fds_iemgr_create();
 *   if (!iemgr || fds_iemgr_read_dir(iemgr, fds_api_cfg_dir()) != FDS_OK) { _error_handling_ };
 *
 *   // Open a file for reading
 *   file = fds_file_init();
 *   if (!file) { _error_handling_ };
 *   rc = fds_file_open(file, "path/to/file", FDS_FILE_READ);
 *   if (rc != FDS_OK) { _error_handling_ };
 *
 *   // Bind definitions of IPFIX IEs
 *   if (fds_file_set_iemgr(file, iemgr) != FDS_OK) { _error_handling_ };
 *
 *   // (Optionally) configure reader filters using any combination of:
 *   // - fds_file_read_efilter(...)
 *   // - fds_file_read_sfilter(...)
 *
 *   // Read records from the file
 *   struct fds_drec rec;
 *   struct fds_read_ctx ctx;
 *
 *   while ((rc = fds_file_read_rec(file, &rec, &ctx)) == FDS_OK) {
 *       // The record is now ready. For example, you can get number of flow packets:
 *       struct fds_drec_field field;
 *       rc = fds_drec_find(&rec, 0U, 2U, &field); // 2 = packetDeltaCount (see IANA IPFIX Entities)
 *       if (rc == FDS_EOC) { _not_found_in_record_ };
 *
 *       // Convert it (to host byte order) and print it
 *       uint64_t pkts_cnt;
 *       rc = fds_get_uint_be(field.data, field.size, &pkts_cnt);
 *       if (rc != FDS_OK) { _error_handling_ };
 *       printf("- packets: %"PRIu64"\n", pkts_cnt);
 *
 *       //...
 *   }
 *
 *   if (rc != FDS_EOC) { _error_handling_ };
 *   // Close the file and destroy IE manager
 *   fds_file_close(file);
 *   fds_iemgr_destroy(iemgr);
 * @endcode
 *
 */

/// List of flags for fds_file_open()
enum fds_file_flags
{
    /// Open file for reading
    FDS_FILE_READ =   (1U << 0),
    /// Open file for writing (if the file exists, it is truncated)
    FDS_FILE_WRITE =  (1U << 1),
    /// Open file for appending (if the file doesn't exists, create a new one)
    FDS_FILE_APPEND = (1U << 2),

    /// Enable LZ4 compression (only for write mode and append mode of a new file)
    FDS_FILE_LZ4 =    (1U << 3),
    /// Enable ZSTD compression (only for write mode and append mode of a new file)
    FDS_FILE_ZSTD =   (1U << 4),

    /// Disable asynchronous I/O (i.e. use only synchronous)
    FDS_FILE_NOASYNC = (1U << 5),
};

/**
 * @brief Initialize file handler
 * @return Pointer to the handler or NULL (memory allocation error)
 */
FDS_API fds_file_t *
fds_file_init();

/**
 * @brief Close a file
 *
 * Close the previously opened file, flush buffers and release all relevant resources.
 * @param[in] file File handler to close
 */
FDS_API void
fds_file_close(fds_file_t *file);

/**
 * @brief Get the last error message
 * @param[in] file File handler
 * @return Error message
 */
FDS_API const char *
fds_file_error(const fds_file_t *file);

/**
 * @brief Open a file for reading/writing/appending
 *
 * The user MUST specify one of the access modes. Otherwise error code #FDS_ERR_ARG is returned.
 * Combination of multiple modes is not allowed!
 *
 * If a new file should be compressed, one of the compression methods (#FDS_FILE_LZ4 or
 * #FDS_FILE_ZSTD) can be selected. Combination of the both flags is not allowed! If the file
 * is opened in append mode and the file already exists, the compression flags are ignored.
 *
 * @note
 *   If any file was previously opened it will be closed.
 * @note
 *   If the file is opened in append mode, the previously defined Templates are not loaded and
 *   must be defined again!
 *
 * @param[in] file     File handler
 * @param[in] path     Path to the file to create/open.
 * @param[in] flags    Flags (see ::fds_file_flags)
 *
 * @return #FDS_OK if the file is open and ready to read/write
 * @return #FDS_ERR_DENIED if the file should be opened for appending but it's being written
 * @return #FDS_ERR_ARG if the combination of arguments is not valid
 * @return #FDS_ERR_INTERNAL if the handler failed to open the file (e.g. access rights, memory
 *   allocation error, malformed file, etc)
 */
FDS_API int
fds_file_open(fds_file_t *file, const char *path, uint32_t flags);

/**
 * \brief Add a reference to an IE manager and redefine all fields
 *
 * Reference to the manager of IE allows the file handler to enrich internally parsed IPFIX
 * (Options) Templates with information about data types, names, etc. of Template Fields.
 * This can be useful especially during record reading where the parsed (Options) Templates
 * are available as the part of Data Record and allow user to directly see all field attributes
 * without lookup.
 *
 * @warning
 *   In the reader mode, the file is rewind after the call (see fds_file_read_rewind())
 * @warning
 *   The manager MUST exists until it is redefined by another manager, removed by passing NULL
 *   pointer or the file handler is closed,
 * @warning
 *   If a reference to any Data Record or Template has been retried from the file (for example
 *   using fds_file_read_rec() or fds_file_write_tmplt_get()), these references are NOT valid
 *   anymore!
 * @note
 *   If the manager has been already configured before, all references to definitions will be
 *   overwritten with new ones. If a definition of an IE was previously available in the older
 *   manager and the new manager doesn't include the definition, the definition will be removed.
 * @note If the manager @p iemgr is undefined (i.e. NULL), all references will be removed.
 * @note Keep in mind this operation can be VERY expensive if file has been at least partly
 *   processed.
 *
 * @param[in] file  File handler
 * @param[in] iemgr Manager of Information Elements (IEs) (can be NULL)
 * @return #FDS_OK on success
 * @return #FDS_ERR_INTERNAL if a memory allocation error has occurred
 */
FDS_API int
fds_file_set_iemgr(fds_file_t *file, const fds_iemgr_t *iemgr);

/// Transport protocol (L4) used to send flow records
enum fds_file_session_proto
{
    /// Unknown protocol (for example, flow records loaded from a file)
    FDS_FILE_SESSION_UNKNOWN = 0,
    /// UDP protocol
    FDS_FILE_SESSION_UDP,
    /// TCP protocol
    FDS_FILE_SESSION_TCP,
    /// SCTP protocol
    FDS_FILE_SESSION_SCTP
};

/**
 * @brief Description of a Transport Session between an exporter and collector
 *
 * @note The structure should be initialized using memset() function
 */
struct fds_file_session
{
    /**
     * Exporter (i.e. Exporting Process) IPv4/IPv6 address
     *
     * @note IPv4 address MUST be encoded as IPv4-Mapped IPv6 Address, see RFC 4291, 2.5.5.2.
     * @note If the address is unknown, set it to zeros.
     */
    uint8_t ip_src[16];
    /**
     * Collector (i.e. Collecting Process) IPv4/IPv6 address
     *
     * @note IPv4 address MUST be encoded as IPv4-Mapped IPv6 Address, see RFC 4291, 2.5.5.2.
     * @note If the address is unknown, set it to zeros.
     */
    uint8_t ip_dst[16];

    /// Exporter port (0 == unknown)
    uint16_t port_src;
    /// Collector port (0 == unknown)
    uint16_t port_dst;
    /// Transport protocol (see ::fds_file_session_proto)
    uint16_t proto;
};

/**
 * @brief Add a new Transport Session of flow records (a.k.a. IPFIX Transport Session)
 *
 * The session helps to distinguish flow records from multiple flow exporters.
 *
 * @note
 *   If there is already a Transport Session with these parameters, the internal identification
 *   of the original Transport Session is returned.
 * @note
 *   At most 65535 Transport Sessions can be added!
 *
 * @param[in]  file File handler
 * @param[in]  info Description of the Session
 * @param[out] sid  Assigned internal identification of the Session
 *
 * @return #FDS_OK on success and the \p sid is filled
 * @return #FDS_ERR_ARG if @p info or @p sid is null.
 * @return #FDS_ERR_DENIED if the file is opened in the reader mode or the maximum number of
 *   Transport Session has been reached.
 * @return #FDS_ERR_FORMAT if the @p info contains invalid values (such as IP, protocol type, etc.)
 * @return #FDS_ERR_INTERNAL usually if a memory allocation error has occurred
 */
FDS_API int
fds_file_session_add(fds_file_t *file, const struct fds_file_session *info, fds_file_sid_t *sid);

/**
 * @brief Get a description of a Transport Session
 *
 * @param[in]  file File handler
 * @param[in]  sid  Assigned internal identification of the Session
 * @param[out] info Description of the Session
 *
 * @return #FDS_OK on success
 * @return #FDS_ERR_ARG if the @p info is NULL.
 * @return #FDS_ERR_NOTFOUND if the internal identification of the Session is unknown
 * @return #FDS_ERR_INTERNAL if a fatal error has occurred (e.g. memory allocation error,
 *   malformed file, etc.)
 */
FDS_API int
fds_file_session_get(fds_file_t *file, fds_file_sid_t sid, const struct fds_file_session **info);

/**
 * @brief Get a list of all Transport Sessions in the file
 *
 * @param[in]  file     File handler
 * @param[out] sid_arr  Array of internal Transport Session identifiers
 * @param[out] sid_size Size of the array
 *
 * @note
 *   User MUST destroy the array by free() when not required anymore.
 * @return #FDS_OK on success
 * @return #FDS_ERR_ARG if @p sid_arr or @p sid_size are NULL pointers.
 * @return #FDS_ERR_INTERNAL if a fatal error has occurred (e.g. memory allocation error)
 */
FDS_API int
fds_file_session_list(fds_file_t *file, fds_file_sid_t **sid_arr, size_t *sid_size);

/**
 * @brief Get a list of all Observation Domain IDs of a given Transport Session
 *
 * @param[in]  file      File handler
 * @param[in]  sid       Internal Transport Session identifier
 * @param[out] odid_arr  Array of ODIDs for the given Transport Session
 * @param[out] odid_size Size of the array
 *
 * @note
 *   User MUST destroy the array by free() when not required anymore.
 * @return #FDS_OK on success
 * @return #FDS_ERR_NOTFOUND if the @p sid doesn't exist
 * @return #FDS_ERR_ARG if @p odid_arr or @p odid_size are NULL pointers
 * @return #FDS_ERR_INTERNAL if a fatal error has occurred (e.g. memory allocation error)
 */
FDS_API int
fds_file_session_odids(fds_file_t *file, fds_file_sid_t sid, uint32_t **odid_arr, size_t *odid_size);

/**
 * @brief Basic statistics about stored Data Records
 * @note The structure MUST be packed because it's also written into the file.
 */
struct __attribute__((packed)) fds_file_stats {
    // Total counters
    uint64_t recs_total;      ///< Total number of records (unidirectional + Biflow + Options based)
    uint64_t recs_bf_total;   ///< Total number of Biflow records (i.e. recs_bf_total <= recs_total)
    uint64_t recs_opts_total; ///< Total number of records based on any IPFIX Options Templates
    uint64_t bytes_total;     ///< Total number of bytes in all flow records
    uint64_t pkts_total;      ///< Total number of packets in all flow records
    // Basic flow stats
    uint64_t recs_tcp;        ///< Total number of TCP flow records (unidirectional + Biflow)
    uint64_t recs_udp;        ///< Total number of UDP flow records (unidirectional + Biflow)
    uint64_t recs_icmp;       ///< Total number of ICMP flow records (unidirectional + Biflow)
    uint64_t recs_other;      ///< Total number of other flow records (unidirectional + Biflow)
    uint64_t recs_bf_tcp;     ///< Total number of TCP Biflow records (recs_bf_tcp <= recs_tcp)
    uint64_t recs_bf_udp;     ///< Total number of UDP Biflow records (recs_bf_udp <= recs_udp)
    uint64_t recs_bf_icmp;    ///< Total number of ICMP Biflow records (recs_bf_icmp <= recs_icmp)
    uint64_t recs_bf_other;   ///< Total number of other Biflow records (recs_bf_other <= recs_other)
    // Basic bytes stats (note: for Biflow records both directions are added)
    uint64_t bytes_tcp;       ///< Total number of bytes in all TCP flow records
    uint64_t bytes_udp;       ///< Total number of bytes in all UDP flow records
    uint64_t bytes_icmp;      ///< Total number of bytes in all ICMP flow records
    uint64_t bytes_other;     ///< Total number of bytes in all other flow records
    // Basic packet stats (note: for Biflow records both directions are added)
    uint64_t pkts_tcp;        ///< Total number of packets in all TCP flow records
    uint64_t pkts_udp;        ///< Total number of packets in all UDP flow records
    uint64_t pkts_icmp;       ///< Total number of packets in all ICMP flow records
    uint64_t pkts_other;      ///< Total number of packets in all other flow records
};

/**
 * @brief Get total statistics of Data Records in the file
 *
 * The statistics contains information about total number of records, sum of bytes and packets
 * over different protocols, etc. mentioned in added Data Records etc.
 * @param[in] file File handler
 * @return Pointer to statistics or NULL (no file is opened)
 */
FDS_API const struct fds_file_stats *
fds_file_stats_get(fds_file_t *file);

// Reader only API ---------------------------------------------------------------------------------

/**
 * @brief Transport Session and ODID filter
 *
 * By default, fds_file_read_rec() returns all Data Records written into the file. This behaviour
 * can be change to return only Data Records that match user defined combinations of Transport
 * Sessions and ODIDs. The filter can be also combined with the expression filter (see
 * fds_file_read_efilter()).
 *
 * Subsequent calls of this function will add the specified combinations to the filter.
 * In other words, the filter is union of all previously passed parameter combinations.
 *
 * - To add a specific combination of a Transport Session and ODID, both @p sid and @p odid must
 *   be specified.
 * - To all ODIDs from a given Transport Session, @p sid must be specified and @p odid must be NULL.
 * - To add a given ODID from all Transport Sessions, @p sid must be NULL and @p odid must be
 *   specified.
 *
 * @note
 *   List of available Transport Session IDs can be obtained using fds_file_session_list().
 *   Available Observation Domain IDs of a given Transport Session ID are provided
 *   by fds_file_session_odids().
 * @note
 *   The file is automatically rewind after the call (see fds_file_read_rewind())
 * @note
 *   To disable/reset this filter, you can call this function with both parameters set to NULL.
 * @note
 *   Not available ODID is not reported as an error. Only invalidity of the Transport Session ID
 *   is checked and reported.
 *
 * @param[in] file File handler
 * @param[in] sid  Internal Transport Session ID (or null for all Transport Sessions)
 * @param[in] odid Observation Domain ID (or null for all ODIDs)
 * @return #FDS_OK on success
 * @return #FDS_ERR_NOTFOUND if the Transport Session doesn't exist
 * @return #FDS_ERR_DENIED if the file is not opened in reader mode
 * @return #FDS_ERR_INTERNAL if a fatal error has occurred (e.g. memory allocation error)
 */
FDS_API int
fds_file_read_sfilter(fds_file_t *file, const fds_file_sid_t *sid, const uint32_t *odid);

/**
 * @brief Set internal position indicator to the beginning of the file
 * @param[in] file File handler
 * @return #FDS_OK on success
 * @return #FDS_ERR_DENIED if the file is not opened in the reader mode
 */
FDS_API int
fds_file_read_rewind(fds_file_t *file);

/// Description of a Data Record context
struct fds_file_read_ctx
{
    /// Export time
    uint32_t exp_time;
    /// Observation Domain ID
    uint32_t odid;
    /// Internal Transport Session identifier
    fds_file_sid_t sid;

    // TODO: add reference to the structure?
};

/**
 * @brief Get the next Data Record from the file
 *
 * Return the next Data Record in the order. If the fds_file_read_filter() and/or
 * fds_file_read_sessions() were configured before, non-matching Data Records are automatically
 * skipped.
 *
 * @note
 *   Keep on mind that all fields in the Data Record are encoded in network byte order (a.k.a.
 *   big endian) and MUST be converted into host byte order using converters distributed with
 *   this library (i.e. fds_get_*_be functions in libfds/converters.h)
 *
 * @warning
 *   Returned pointers represent a Data Record and its IPFIX Template stored withing the file
 *   handler instance. Therefore, subsequent calls of this function may overwrite internal buffers
 *   where the Data Records are stored. If a user wants to preserve the Data Record, a deep copy
 *   of the record and its IPFIX Template MUST be made.
 *
 * @param[in]  file File handle
 * @param[out] rec  Data Record (pointer to the Data Record, IPFIX Template, etc.)
 * @param[out] ctx  Data Record context i.e. Transport Session, ODID, Export Time (can be NULL)
 *
 * @return #FDS_OK on success and structures \p rec and \p ctx are filled
 * @return #FDS_EOC if the end of the file was reached (i.e. no more records available)
 * @return #FDS_ERR_DENIED if the file is not opened in the reader mode
 * @return #FDS_ERR_INTERNAL if a fatal error has occurred (e.g. memory allocation error)
 */
FDS_API int
fds_file_read_rec(fds_file_t *file, struct fds_drec *rec, struct fds_file_read_ctx *ctx);

// Writer only API ---------------------------------------------------------------------------------

/**
 * @brief Set the context of the flow records that will be written
 *
 * The context helps to distinguish flow records from different Transport Sessions and Observation
 * Domain IDs (ODIDs).
 *
 * @warning
 *   Keep on mind that Templates (and their IDs) defined in one combination of a Transport Session
 *   and ODID are NOT automatically defined in all other combinations. Moreover, definitions of
 *   the Templates with the same IDs in different ODIDs, Transport Sessions, or their combinations
 *   can differ!
 *
 * @param[in] file     File handler
 * @param[in] sid      Internal identification of the Transport Session (see fds_file_session_add())
 * @param[in] odid     Observation Domain ID (user specific identification of flow monitoring
 *   location, should be usually different from zero)
 * @param[in] exp_time Export time i.e. time at which following records left the corresponding
 *   exporter. If this information is not available or flow records doesn't contain any Information
 *   Elements that refer to this value, it can be set to 0 or the current system time.
 *
 * @return #FDS_OK on success
 * @return #FDS_ERR_NOTFOUND if the internal identification of the Session is unknown
 * @return #FDS_ERR_DENIED if the file is opened in the reader mode
 * @return #FDS_ERR_INTERNAL if a fatal error has occurred (e.g. memory allocation error)
 */
FDS_API int
fds_file_write_ctx(fds_file_t *file, fds_file_sid_t sid, uint32_t odid, uint32_t exp_time);

/**
 * @brief Add a new IPFIX (Options) Template definition or redefine the current one
 *
 * The (Options) Templates defines structure of IPFIX Data Record (i.e. position and type of
 * Information Elements in the Data Record). Each (Options) Template definition contains
 * unique Template ID (>= #FDS_IPFIX_SET_MIN_DSET), number of fields, and field definitions.
 * Moreover, Options Templates contains number of so called Scope fields. For more information
 * see RFC 7011.
 *
 * The user must specify the Template type i.e. whether the passed pointer to a memory points
 * to an IPFIX Template or IPFIX Options Template.
 *
 * @note
 *   Before adding/redefining an (Options) Template make sure that the writer context is set
 *   appropriately. See fds_file_write_ctx(). If the same (Options) Templates should be reused
 *   in different ODIDs or Transport Sessions it must be individually added for each of them
 *   (or their combination).
 * @note
 *   Keep on mind that frequent Template redefinition (i.e. adding Template with the Template ID
 *   that is already used) can cause non-optimal storage performance - writer/reader speed and the
 *   file size are affected. Therefore, if it is possible, avoid Template redefinition.
 *
 * @warning
 *   The (Options) Template MUST be specified in the same format as specified by RFC 7011
 *   i.e. in network byte order!
 *
 * @param[in] file    File handler
 * @param[in] t_type  Template type (#FDS_TYPE_TEMPLATE or #FDS_TYPE_TEMPLATE_OPTS)
 * @param[in] t_data  Pointer to IPFIX (Options) Template definition to add. Expected structure
 *   matches ::fds_ipfix_trec (for #FDS_TYPE_TEMPLATE type) or ::fds_ipfix_opts_trec
 *   (for #FDS_TYPE_TEMPLATE_OPTS type)
 * @param[in] t_size  Size of the (Options) Template definition
 *
 * @return #FDS_OK on success (i.e. the (Options) Template with given ID has been added)
 * @return #FDS_ERR_FORMAT if the (Options) Template is malformed or \p t_type is invalid
 * @return #FDS_ERR_DENIED if the file is opened in the reader mode
 * @return #FDS_ERR_ARG if the context is not specified or arguments are invalid
 * @return #FDS_ERR_INTERNAL if a fatal error has occurred (e.g. memory allocation error)
 */
FDS_API int
fds_file_write_tmplt_add(fds_file_t *file, enum fds_template_type t_type, const uint8_t *t_data,
    uint16_t t_size);

/**
 * @brief Remove an IPFIX (Options) Template definition
 *
 * The previously defined (Options) Template is removed from internal structures. After removing
 * the Template, fds_file_write_rec() rejects any attempt to write Data Records with the given
 * Template ID.
 *
 * @note
 *   Before removing the (Options) Template make sure that the writer context is set appropriately.
 *   See fds_file_write_ctx(). If the same (Options) Templates should be removed from different
 *   ODIDs or Transport Sessions it must be individually done for each of them (or their
 *   combination).
 * @note
 *   Keep on mind that frequent Template removing can cause non-optimal storage performance i.e.
 *   writer/reader speed and the file size are affected. Therefore, if it is possible, avoid
 *   Template removing.
 *
 * @param[in] file File handler
 * @param[in] tid  Template ID
 *
 * @return #FDS_OK on success
 * @return #FDS_ERR_NOTFOUND if the (Options) Template is not defined in the current context
 * @return #FDS_ERR_DENIED if the file is opened in the reader mode
 * @return #FDS_ERR_ARG if the context is not specified
 * @return #FDS_ERR_INTERNAL if a fatal error has occurred (e.g. memory allocation error)
 */
FDS_API int
fds_file_write_tmplt_remove(fds_file_t *file, uint16_t tid);

/**
 * @brief Get a definition of a previously defined IPFIX (Options) Template with given Template ID
 *
 * @note
 *   Before getting the (Options) Template make sure that the writer context is set appropriately.
 *   See fds_file_write_ctx().
 *
 * @param[in]  file   File handler
 * @param[in]  tid    Template ID
 * @param[out] t_type Template type (i.e. #FDS_TYPE_TEMPLATE or #FDS_TYPE_TEMPLATE_OPTS) (can be NULL)
 * @param[out] t_data Pointer to the (Options) Template (can be NULL)
 * @param[out] t_size Size of the (Options) Template (can be NULL)
 *
 * @return #FDS_OK on success and non-NULL pointers are filled
 * @return #FDS_ERR_NOTFOUND if the (Options) Template is not defined in the current context
 * @return #FDS_ERR_DENIED if the file is opened in the reader mode
 * @return #FDS_ERR_ARG if the context is not specified or arguments are invalid
 * @return #FDS_ERR_INTERNAL if a fatal error has occurred (e.g. memory allocation error)
 */
FDS_API int
fds_file_write_tmplt_get(fds_file_t *file, uint16_t tid, enum fds_template_type *t_type,
    const uint8_t **t_data, uint16_t *t_size);

/**
 * @brief Write an IPFIX Data Record based on a given IPFIX (Options) Template
 *
 * Check if the Data Record matches the previously defined (Options) Template with the given
 * Template ID and store it into the file.
 *
 * @note
 *   Before writing the Data Record make sure that the writer context is set appropriately.
 *   See fds_file_write_ctx().
 * @note
 *   It is guaranteed that IPFIX Data Records written to the file using the same combination
 *   of a Transport Session and Observation Domain ID (configured by fds_file_write_ctx()))
 *   preserve insertion order.
 * @warning
 *   The Data Record MUST be specified in the same format as specified by RFC 7011
 *   i.e. in network byte order!
 *
 * @param[in] file     File handler
 * @param[in] tid      Template ID
 * @param[in] rec_data Data Record to store
 * @param[in] rec_size Size of the Data Record (must be greater than zero)
 *
 * @return #FDS_OK on success
 * @return #FDS_ERR_NOTFOUND if the (Options) Template is not defined in the current context
 * @return #FDS_ERR_FORMAT if the Data Record is malformed and doesn't match the (Options) Template
 *   (i.e. record length is different than expected)
 * @return #FDS_ERR_DENIED if the file is opened in the reader mode
 * @return #FDS_ERR_ARG if the context is not specified or arguments are invalid
 * @return #FDS_ERR_INTERNAL if a fatal error has occurred (e.g. memory allocation error)
 */
FDS_API int
fds_file_write_rec(fds_file_t *file, uint16_t tid, const uint8_t *rec_data, uint16_t rec_size);

#ifdef __cplusplus
}
#endif

#endif // LIBFDS_FILE_H
