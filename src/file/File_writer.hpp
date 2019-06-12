/**
 * @file   src/file/File_writer.hpp
 * @brief  File writer (header file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   May 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBFDS_WRITER_HPP
#define LIBFDS_WRITER_HPP

#include <map>

#include "File_base.hpp"
#include "Block_templates.hpp"
#include "Block_data_writer.hpp"
#include "Block_session.hpp"
#include "Block_content.hpp"

namespace fds_file {

/**
 * @brief Auxiliary structure for Block_session comparsion used in std::map
 */
struct block_session_cmp {
    /**
     * @brief Compare two Transport Sessions
     *
     * @note Objects are compare only by Transport Session definition (Internal ID is ignored!)
     * @param[in] lhs Left side
     * @param[in] rhs Right side
     * @return True if the first argument goes before the second argument in the strict
     *   weak ordering it defines, and false otherwise.
     */
    bool
    operator()(const Block_session *lhs, const Block_session *rhs) const {
        return (*lhs) < (*rhs);
    }
};

/**
 * @brief File writer
 *
 * Class implements the interface for writing Data Records to a file.
 */
class File_writer : public File_base {
public:
    /**
     * @brief Class constructor
     *
     * @note
     *   The compression algorithm is ignored if the file already exists and the @p append mode is
     *   enabled.
     * @note
     *   For I/O parameter @p io_type has impact only on writing of large file blocks. For (usually)
     *   small blocks (such as the Content Block, Template Block, etc.) synchronous I/O is always
     *   used.
     * @param[in] path    File path
     * @param[in] calg    Selected compression algorithm
     * @param[in] append  Open in append mode (do not overwrite if the file already exists)
     * @param[in] io_type I/0 method used for writing large blocks (i.e. Data Blocks, etc.)
     */
    File_writer(const char *path, fds_file_alg calg, bool append = false,
        Io_factory::Type io_type = Io_factory::Type::IO_DEFAULT);
    /**
     * @brief Class destructor
     *
     * Flush all Data Record buffers and their unwritten/modified Template Managers to the file
     * and free all resources.
     */
    ~File_writer();

    // Disable copy constructors
    File_writer(const File_writer &other) = delete;
    File_writer &operator=(const File_writer &other) = delete;

    // ---- Implementation of base functions ----
    void
    iemgr_set(const fds_iemgr_t *iemgr) override;

    fds_file_sid_t
    session_add(const struct fds_file_session *info) override;
    const struct fds_file_session *
    session_get(fds_file_sid_t sid) override;
    void
    session_list(fds_file_sid_t **arr, size_t *size) override;
    void
    session_odids(fds_file_sid_t sid, uint32_t **arr, size_t *size) override;

    void
    select_ctx(fds_file_sid_t sid, uint32_t odid, uint32_t exp_time) override;
    void
    write_rec(uint16_t tid, const uint8_t *rec_data, uint16_t rec_size) override;
    void
    tmplt_add(enum fds_template_type t_type, const uint8_t *t_data, uint16_t t_size) override;
    void
    tmplt_remove(uint16_t tid) override;
    void
    tmplt_get(uint16_t tid, enum fds_template_type *t_type, const uint8_t **t_data, uint16_t *t_size) override;

private:
    /// Auxiliary structure that is unique for a combination of Transport Session and ODID
    struct odid_info {
        /// Template manager of the Data Records (will be stored as a Template Block)
        Block_templates m_tblock_data;
        /// Offset of Template Block in the file (0 == not written yet)
        uint64_t m_tblock_offset;
        /// Buffer of IPFIX Data Records (will be stored as a Data Block)
        Block_data_writer m_data;

        /// Observation Domain ID
        uint32_t m_odid;
        /// Transport Session ID
        uint16_t m_sid;
        /// IPFIX (Options) Template used during adding of the latest Data Record (can be nullptr)
        const fds_template *m_tmplt_last;

        /**
         * @brief Constructor
         * @param[in] sid  Transport Session ID
         * @param[in] odid Observation Domain ID of IPFIX Data Records and IPFIX (Options) Templates
         * @param[in] calg Selected compression algorithm
         */
        odid_info(uint16_t sid, uint32_t odid, enum fds_file_alg calg)
            : m_tblock_data(), m_tblock_offset(0),  m_data(odid, calg), m_odid(odid), m_sid(sid),
              m_tmplt_last(nullptr) {}
    };

    /// Transport Session description
    struct session_info {
        /// Session description (will be stored as a Session Block)
        Block_session m_sblock_data;
        /// Offset of the Session Block in the file (0 == not written yet)
        uint64_t m_sblock_offset;
        /// List of Observation Domain IDs of the Transport Session
        std::map<uint32_t, std::unique_ptr<struct odid_info>> m_odids;

        /**
         * @brief Constructor
         * @param[in] sid     Internal Transport Session ID
         * @param[in] session Transport Session description
         */
        session_info(uint16_t sid, const fds_file_session *session)
            : m_sblock_data(sid, session), m_sblock_offset(0), m_odids() {}
    };

    /// Type of I/O used for writing large file blocks
    Io_factory::Type m_io_type;

    /// List of all Transport Sessions (identified by internal Transport Session ID)
    std::map<uint16_t, std::unique_ptr<struct session_info>> m_sessions;
    /// Mapping of Transport Sessions to internal IDs (just for faster Transport Session lookup)
    std::map<const Block_session *, uint16_t, block_session_cmp> m_session2id;

    /// Content Table
    Block_content m_ctable;

    /// Selected combination of Transport Session + ODID (can be nullptr if not selected)
    struct odid_info *m_selected = nullptr;
    /// File offset where the next block should be placed
    uint64_t m_offset = 0;
    /// Reference to the IE manager (can be nullptr)
    const fds_iemgr_t *m_iemgr = nullptr;

    void
    append_prepare();
    void
    flush_all();
    void
    flush(odid_info *oinfo);
};

} // namespace

#endif //LIBFDS_WRITER_HPP
