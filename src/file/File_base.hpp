/**
 * @file   src/file/File_base.hpp
 * @brief  Base class for file manipulation (header file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   April 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBFDS_FILE_BASE_HPP
#define LIBFDS_FILE_BASE_HPP

#include <cstddef>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libfds/api.h>
#include <libfds/file.h>
#include "structure.h"
#include "Block_content.hpp"

namespace fds_file {

/**
 * @brief Base class for manipulation with the libfds file
 *
 * The class represent interface for file manipulation and provides components common for
 * the file writer and reader.
 */
class File_base {
public:
    /// Default read/write rights for user/group/others (equals to 666)
    static const mode_t DEF_MODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

    /// File creation flags for appending
    static const int CF_APPEND = O_RDWR | O_CREAT; // Note: no O_APPEND due to pwrite() bug
    /// File creation flags for truncation
    static const int CF_TRUNC = O_WRONLY | O_CREAT | O_TRUNC;
    /// File creation flags for read only operation
    static const int CF_READ = O_RDONLY;

    /**
     * @brief Base class constructor
     *
     * The base class will try to open or create a new file based on the configured parameters.
     * Internal statistics are cleared and the file header parameters are set to default value.
     *
     * @param[in] path  Path to the file
     * @param[in] oflag Creation flags (read/write/append - see man 3 open)
     * @param[in] mode  File mode bits applied when a new file is created (e.g. read/write access
     *   rights for user/group/others, etc.)
     * @param[in] calg  Selected compression algorithm
     * @throw File_exception if the file cannot be opened or created.
     */
    File_base(const char *path, int oflag, mode_t mode = DEF_MODE, fds_file_alg calg = FDS_FILE_CALG_NONE);
    /**
     * @brief Class destructor
     *
     * Closes the file descriptor of the file and free memory.
     */
    virtual ~File_base();

    // Copy constructors are disabled (two handlers cannot manipulate with the same file)
    File_base(const File_base &other) = delete;
    File_base &operator=(const File_base &other) = delete;

    // File manipulation functions -----------------------------------------------------------------

    /**
     * @brief Get statistics about records in the file
     * @return Statistics
     */
    virtual const struct fds_file_stats *
    stats_get() {return &m_stats;};
    /**
     * @brief Set definitions of Information Elements
     * @see fds_file_set_iemgr()
     * @throw File_exception if any Template manager fails to update references
     */
    virtual void
    iemgr_set(const fds_iemgr_t *iemgr) = 0;

    /**
     * @brief Add a new Transport Session
     *
     * If a Transport Session with the given description already exists, its ID will be returned.
     * @see fds_file_session_add()
     * @param[in] info Description of the Transport Session to add
     * @return Internal Transport Session ID
     * @throw File_exception if the maximum number of sessions has been reached or the description
     *   is invalid.
     */
    virtual fds_file_sid_t
    session_add(const struct fds_file_session *info);
    /**
     * @brief Get a description of a Transport Session based with a given internal ID
     *
     * @see fds_file_session_get()
     * @param[in] sid Internal Transport Session ID
     * @return Pointer or nullptr (if not defined)
     */
    virtual const struct fds_file_session *
    session_get(fds_file_sid_t sid) = 0;
    /**
     * @brief Get list of Transport Sessions
     *
     * @see fds_file_session_list()
     * @note The @p sid is array that must be later freed (using free) by the user.
     * @note If there are no Transport Sessions, @p size is set to 0, and @p arr is set to nullptr.
     * @param[out] arr  Array of all Transport Session IDs
     * @param[out] size Size of the array
     */
    virtual void
    session_list(fds_file_sid_t **arr, size_t *size) = 0;

    /**
     * @brief Get list of ODIDs of a given Transport Session
     *
     * The function doesn't check if the Transport Session ID is defined. If the ID doesn't exist
     * at all, the result is the same as for a Transport Session without any ODID.
     * @see fds_file_session_odids()
     * @note The @p sid is array that must be later freed (using free) by the user.
     * @note If there are no ODIDs, @p size is set to 0, and @p arr is set to nullptr.
     * @param[in]  sid  Internal Transport Session ID
     * @param[out] arr  Array of Observation Domain IDs
     * @param[out] size Size of the array
     */
    virtual void
    session_odids(fds_file_sid_t sid, uint32_t **arr, size_t *size) = 0;

    /**
     * @brief Transport Session and ODID filter configuration
     *
     * Implements configuration interface of the filter. For more information see
     * fds_file_read_sfilter() function.
     * @param[in] sid  Internal Transport Session ID (can be nullptr)
     * @param[in] odid Observation Domain ID (can be nullptr)
     */
    virtual void
    read_sfilter_conf(const fds_file_sid_t *sid, const uint32_t *odid);

    /**
     * @brief Set internal position of the reader to the beginning of the file
     * @see fds_file_read_rewind()
     */
    virtual void
    read_rewind();

    /**
     * @brief Get the next Data Record from the file
     *
     * @see fds_file_read_rec()
     * @param[out] rec Data Record to be filled
     * @param[out] ctx Data Record context to be filled (can be nullptr)
     * @return #FDS_OK on success (both structures are filled
     * @return #FDS_EOC if there are no more Data Records
     * @throw File_exception if the file malformed or any parser fails
     */
    virtual int
    read_rec(struct fds_drec *rec, struct fds_file_read_ctx *ctx);

    /**
     * @brief Select context of writer operations (Transport Session, ODID, Export Time)
     * @see fds_file_write_ctx()
     * @param[in] sid      Internal Transport Session ID
     * @param[in] odid     Observation Domain ID
     * @param[in] exp_time Export Time
     * @throw File_exception if the Transport Session doesn't exist
     */
    virtual void
    select_ctx(fds_file_sid_t sid, uint32_t odid, uint32_t exp_time);
    /**
     * @brief Add an IPFIX Data Record formatted by an IPFIX (Options) Template with a given ID
     * @see fds_file_write_rec()
     * @note Context must be defined, see select_ctx()
     * @param[in] tid      Template ID
     * @param[in] rec_data IPFIX Data Record
     * @param[in] rec_size Size of the Data Record
     * @throw File_exception If the Data Record is malformed or the Template hasn't been defined yet
     */
    virtual void
    write_rec(uint16_t tid, const uint8_t *rec_data, uint16_t rec_size);
    /**
     * @brief Add a definition of an IPFIX (Options) Template
     * @see fds_file_write_tmplt_add()
     * @note Context must be defined, see select_ctx()
     * @note Template ID is a part of its definition
     * @param[in] t_type Type of the Template
     * @param[in] t_data Definition of the Template (based on RFC 7011)
     * @param[in] t_size Size of the definition
     * @throw File_exception if the Template is malformed
     */
    virtual void
    tmplt_add(enum fds_template_type t_type, const uint8_t *t_data, uint16_t t_size);
    /**
     * @brief Remove a definition of an IPFIX (Options) Template with a given ID
     *
     * @see fds_file_write_tmplt_remove()
     * @note Context must be defined, see select_ctx()
     * @param[in] tid Template ID
     * @throw File_exception if a Template with the given ID is not defined
     */
    virtual void
    tmplt_remove(uint16_t tid);
    /**
     * @brief Get an IPFIX (Options) Template with a given ID
     *
     * @see fds_file_write_tmplt_get()
     * @note Context must be defined, see select_ctx()
     * @param[in]  tid    Template ID
     * @param[out] t_type Type of the Template
     * @param[out] t_data Pointer to the definition of the Template
     * @param[out] t_size Size of the Template definition
     */
    virtual void
    tmplt_get(uint16_t tid, enum fds_template_type *t_type, const uint8_t **t_data, uint16_t *t_size);

protected:
    /// File descriptor of the file
    int m_fd;

    /**
     * @brief Update global statistics about Data Records in the file
     *
     * Common parameters such as number of packets and bytes, protocol etc. are extracted and
     * used to update common statistic table(s).
     * @param[in] rec_data Data Record
     * @param[in] rec_size Data Record size
     * @param[in] tmplt    IPFIX (Options) Template
     */
    void
    stats_update(const uint8_t *rec_data, uint16_t rec_size, const struct fds_template *tmplt);

    /**
     * @brief Load the content of the file header and global statistics from the file
     *
     * @warning
     *   Only file "magic" code and compression algorithm support are checked and if any of them
     *   doesn't match, an exception is thrown. However, the file version identification is ignored
     *   and the check is up to the subclass. The reason is that a reader subclass should be able
     *   to read file content while ignoring unsupported changes. On the other hand, writing in
     *   append mode should not be allowed to avoid making incompatible changes.
     *
     * @note Current statistics and parameters are overwritten.
     * @note If the exception is thrown, internal structures (i.e. current internal version of
     *   the header and statistics) are not modified.
     * @throw File_exception if the header cannot be loaded or it is malformed
     */
    void
    file_hdr_load();

    /**
     * @brief Write the content of the file header and global statistics to the file
     * @throw File_exception if the header cannot be stored
     */
    void
    file_hdr_store();

    /**
     * @brief Get the file version
     * @return Version code
     */
    uint8_t
    file_hdr_get_version() {return m_file_hdr.version;};

    /**
     * @brief Get the compression/decompression method of the file
     * @return Compression method
     */
    enum fds_file_alg
    file_hdr_get_calg() {return static_cast<enum fds_file_alg>(m_file_hdr.comp_method);};

    /**
     * @brief Define position of the content table in the file
     * @param[in] offset Offset from the start of the file
     */
    void
    file_hdr_set_ctable(uint64_t offset) {m_file_hdr.table_offset = htole64(offset);};
    /**
     * @brief Get position of the content table in the file
     * @note
     *   If the position is zero, it usually means that someone is editing the file (i.e. it
     *   hasn't been written yet) or the file is corrupted.
     * @return Offset from the start of the file
     */
    uint64_t
    file_hdr_get_ctable() {return le64toh(m_file_hdr.table_offset);};

    /**
     * @brief Get list of Transport Sessions
     *
     * The function implements session_list() functionality based on extraction from a Content
     * Table. Subclasses can use this function if their can provide the Content Table.
     *
     * @param[in]  cblock Content Table (all Session Blocks in the file MUST be defined here)
     * @param[out] arr    Array of all Transport Session IDs
     * @param[out] size   Size of the array
     */
    static void
    session_list_from_ctable(const Block_content &cblock, fds_file_sid_t **arr, size_t *size);

    /**
     * @brief Get list of ODIDs of a given Transport Sessions
     *
     * The function implements session_odids() functionality based on extraction from a Content
     * Table. Subclasses can use this function if their can provide the Content Table.
     *
     * @param[in]  cblock Content Table (all Data Blocks in the file MUST be defined here)
     * @param[in]  sid    Internal Transport Session ID
     * @param[out] arr    Array of Observation Domain IDs
     * @param[out] size   Size of the array
     */
    static void
    session_odids_from_ctable(const Block_content &cblock, fds_file_sid_t sid, uint32_t **arr,
        size_t *size);

private:
    /// File header (in little endian - proper conversion functions MUST be used!)
    struct fds_file_hdr m_file_hdr;
    /// Statistics about flows in the file (in host byte order - NO conversion required)
    struct fds_file_stats m_stats;

    /**
     * @brief Default handler for not implemented functions by derived classes
     * @throw File_exception Always throw the exception with #FDS_ERR_DENIED code
     */
    [[noreturn]] void
    not_impl_handler();

    /**
     * @brief Copy global statistics to the file header structure (in proper byte order)
     */
    void
    stats_to_hdr();
    /**
     * @brief Copy global statistics from the file header structure (in proper byte order)
     */
    void
    stats_from_hdr();
};

} // namespace

#endif //LIBFDS_FILE_BASE_HPP
