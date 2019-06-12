/**
 * @file   src/file/File_reader.hpp
 * @brief  File reader (header file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   May 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBFDS_FILE_READER_HPP
#define LIBFDS_FILE_READER_HPP

#include <map>
#include <memory>
#include <list>
#include <set>

#include <libfds.h>
#include "Block_content.hpp"
#include "Block_data_reader.hpp"
#include "Block_session.hpp"
#include "Block_templates.hpp"
#include "File_base.hpp"

namespace fds_file {

/**
 * @brief File reader
 *
 * Class implements the interface for reading Data Records stored in a file.
 */
class File_reader : public File_base {
public:
    /**
     * @brief Class constructor
     *
     * Load and parse the file header and Content Table. If the Content Table is not available
     * (usually while the file is being written, the Content Table is rebuild).
     *
     * @note
     *   For I/O parameter @p io_type has impact only on loading of large file blocks. For (usually)
     *   small blocks (such as the Content Block, Template Block, etc.) synchronous I/O is always
     *   used.
     * @param[in] path    File to be opened for reading
     * @param[in] io_type I/0 method used for loading large blocks (i.e. Data Blocks, etc.)
     */
    File_reader(const char *path, Io_factory::Type io_type = Io_factory::Type::IO_DEFAULT);
    /**
     * @brief Class destructor
     *
     * Free all allocated resources and close the file.
     */
    ~File_reader() = default;

    // Disable copy constructors
    File_reader(const File_reader &other) = delete;
    File_reader &operator=(const File_reader &other) = delete;

    // ---- Implementation of the base class functions ----
    void
    iemgr_set(const fds_iemgr_t *iemgr) override;

    const struct fds_file_session *
    session_get(fds_file_sid_t sid) override;
    void
    session_list(fds_file_sid_t **arr, size_t *size) override;
    void
    session_odids(fds_file_sid_t sid, uint32_t **arr, size_t *size) override;

    void
    read_sfilter_conf(const fds_file_sid_t *sid, const uint32_t *odid) override;
    void
    read_rewind() override;
    virtual int
    read_rec(struct fds_drec *rec, struct fds_file_read_ctx *ctx) override;

private:
    /// Auxiliary structure with information about a loaded Template Block
    struct tblock_info {
        /// Internal Transport Session ID
        uint16_t sid;
        /// Observation Domain ID
        uint32_t odid;
        /// Template Block
        Block_templates block;
    };

    /// Manager of Information Elements
    const fds_iemgr_t *m_iemgr = nullptr;
    /// Type of I/O used for loading large file blocks
    Io_factory::Type m_io_type;

    /// Content Table (can be nullptr if the table is not available)
    Block_content m_ctable;
    /// Loaded Template Blocks identified by their offset in the file
    std::map<uint64_t, struct tblock_info> m_tmplts;
    /// Loaded Session Blocks identified by internal Transport Session ID
    std::map<uint16_t, std::unique_ptr<Block_session>> m_sessions;

    /// List of idle Data Block readers
    std::list<std::unique_ptr<Block_data_reader>> m_db_idles;
    /// The current Data Block reader from which the next Data Record will be returned
    std::unique_ptr<Block_data_reader> m_db_current = nullptr;
    /// The next Data Block reader (usually used for asynchronous loading of the next Data Block)
    std::unique_ptr<Block_data_reader> m_db_next = nullptr;
    /// Index of the next Data Block in the Content Table
    size_t m_db_next_idx = 0;

    struct {
        /// Status of the Transport Session and ODID filter
        bool enabled = false;
        /// Accepted Observation Domain IDs (no mather from which Transport Session)
        std::set<uint32_t>odids_all;
        /// Accepted Transport Session IDs (no mather which ODID)
        std::set<uint16_t>sid_all;
        /// Accepted specific combinations of Transport Sessions and ODIDs
        std::map<uint16_t, std::set<uint32_t>> combi;
    } m_sfilter;


    struct tblock_info &
    get_tblock(uint64_t offset);
    const Block_session *
    get_sblock(uint16_t sid);

    void
    ctable_rebuild();
    void
    ctable_process_sblock(off_t offset, const struct fds_file_bsession *block);
    void
    ctable_process_dblock(off_t offset, const struct fds_file_bdata *block);

    void
    scheduler();
    void
    scheduler_next2current();
    void
    scheduler_prepare_next();

    bool
    sfilter_match(uint16_t sid, uint32_t odid);

};

} // namespace

#endif //LIBFDS_FILE_READER_HPP
