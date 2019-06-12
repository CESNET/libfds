/**
 * @file   src/file/File_reader.cpp
 * @brief  File reader (source file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   May 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <algorithm>
#include <set>
#include <string>

#include <sys/types.h>
#include <unistd.h>  // lseek

#include "File_reader.hpp"
#include "File_exception.hpp"
#include "Io_sync.hpp"

using namespace fds_file;

File_reader::File_reader(const char *path, Io_factory::Type io_type)
    : File_base(path, File_base::CF_READ), m_io_type(io_type)
{
    // Try to load the file header
    file_hdr_load();

    // Load the Content Table
    uint64_t ctable_offset = file_hdr_get_ctable();
    if (ctable_offset != 0) {
        // Position of the Table is known
        m_ctable.load_from_file(m_fd, ctable_offset);
    } else {
        // Position is not defined, create it manually (very expensive)
        ctable_rebuild();
    }

    /* Prepare 2 Data Block readers
     * Why? For asynchronous I/O read we need 2 readers. The first one is used to return Data
     * Records from the current Data Block while the latter is asynchronously loading the next
     * Data Block in background.
     */
    enum fds_file_alg alg = file_hdr_get_calg();
    m_db_idles.emplace_back(new Block_data_reader(alg));
    m_db_idles.emplace_back(new Block_data_reader(alg));

    // Rewind
    read_rewind();
}

void
File_reader::iemgr_set(const fds_iemgr_t *iemgr)
{
    m_iemgr = iemgr;

    // Update each already loaded Template Block
    for (auto &tblock : m_tmplts) {
        tblock.second.block.ie_source(iemgr);
    }

    // Templates and template snapshots used by Data Block readers are not valid anymore!
    // We have to start over!
    read_rewind();
}


const struct fds_file_session *
File_reader::session_get(fds_file_sid_t sid)
{
    const Block_session *session = get_sblock(sid);
    if (!session) {
        return nullptr;
    }

    return &session->get_struct();
}

void
File_reader::session_list(fds_file_sid_t **arr, size_t *size)
{
    /* All Transport Sessions should be in the Content Table, therefore, we can use the default
     * implementation.
     */
    File_base::session_list_from_ctable(m_ctable, arr, size);
}

void
File_reader::session_odids(fds_file_sid_t sid, uint32_t **arr, size_t *size)
{
    /* All Data Blocks should be in the Content Table, therefore, we can use the default
     * implementation.
     */
    File_base::session_odids_from_ctable(m_ctable, sid, arr, size);
}

void
File_reader::read_sfilter_conf(const fds_file_sid_t *sid, const uint32_t *odid)
{
    read_rewind();

    if (!sid && !odid) {
        // Cleanup
        m_sfilter.enabled = false;
        m_sfilter.odids_all.clear();
        m_sfilter.sid_all.clear();
        m_sfilter.combi.clear();
        return;
    }

    if (!sid && odid) {
        // An ODID from all Transport Sessions
        m_sfilter.odids_all.emplace(*odid);
        m_sfilter.enabled = true;
        return;
    }

    // Find the Transport Session
    assert(sid != nullptr && "Transport Session must be defined for other use cases");
    if (get_sblock(*sid) == nullptr) {
        throw File_exception(FDS_ERR_NOTFOUND, "Transport Session ID " + std::to_string(*sid)
            + " doesn't exist");
    }

    if (!odid) {
        // All ODIDs from the given Transport Session
        m_sfilter.sid_all.emplace(*sid);
    } else {
        // Specific combination of Transport Session ID and ODID
        m_sfilter.combi[*sid].emplace(*odid);
    }

    m_sfilter.enabled = true;
}

void
File_reader::read_rewind()
{
    // Move active Data Block readers to the list of inactive Data Block readers
    if (m_db_current) {
        m_db_idles.emplace_back(std::move(m_db_current));
        m_db_current = nullptr;
    }

    if (m_db_next) {
        m_db_idles.emplace_back(std::move(m_db_next));
        m_db_next = nullptr;
    }

    // The next Data Block start from the beginning of the file
    m_db_next_idx = 0;
}

int
File_reader::read_rec(struct fds_drec *rec, struct fds_file_read_ctx *ctx)
{
    // Get the Data Record from the currently available Data Block
    if (m_db_current && m_db_current->next_rec(rec, ctx) == FDS_OK) {
        // The record is ready
        return FDS_OK;
    }

    // The current Data Block has been completely processed, prepare the next one...
    while (true) {
        scheduler();

        if (!m_db_current) {
            // End of the file has been reached
            return FDS_EOC;
        }

        if (m_db_current->next_rec(rec, ctx) != FDS_OK) {
            // The loaded Data Block is probably empty, try to load the next one...
            continue;
        }

        // The Data Record is filled and the Data Block is ready
        return FDS_OK;
    }
}

/**
 * @brief Get a Template Block with a given offset
 *
 * If the Template Block hasn't been previously loaded, the block is located in the file, parsed,
 * and stored into the internal list and returned.
 *
 * @param[in] offset Offset of the Template Block from the start of the file
 * @return Reference of the Template Block
 * @throw File_exception if a valid Template Block is not available at the offset in the file
 */
struct File_reader::tblock_info &
File_reader::get_tblock(uint64_t offset)
{
    const auto it = m_tmplts.find(offset);
    if (it != m_tmplts.end()) {
        // Found
        return it->second;
    }

    // Not found -> try to load it
    struct tblock_info &tblock = m_tmplts[offset];
    tblock.block.ie_source(m_iemgr);
    tblock.block.load_from_file(m_fd, offset, &tblock.sid, &tblock.odid);
    return tblock;
}

/**
 * Get a Transport Session with a given ID
 *
 * If the block hasn't been loaded earlier, the particular Content Table record of the Transport
 * Session in the Content Table is used to locate it in the file. The description is loaded,
 * parsed, and stored into the internal list and returned.
 *
 * @note
 *   Content Table is used to determine location of the block is it hasn't been loaded yet.
 * @param[in] sid Internal Transport Session ID of the block to load
 * @return Reference to the Transport Session or nullptr (not present)
 * @throw File_exception if the Session description is malformed
 */
const Block_session *
File_reader::get_sblock(uint16_t sid)
{
    // Check if the block has been already loaded
    const auto it = m_sessions.find(sid);
    if (it != m_sessions.end()) {
        return it->second.get();
    }

    // Find the position of the Transport Session block in the Content Table
    auto lambda = [sid](const struct Block_content::info_session &item) -> bool {
        return item.session_id == sid;
    };

    const std::vector<Block_content::info_session> &list = m_ctable.get_sessions();
    const auto it_sinfo = std::find_if(list.begin(), list.end(), lambda);
    if (it_sinfo == list.end()) {
        // Not found
        return nullptr;
    }

    // Parse the Session and store it
    auto sptr = std::unique_ptr<Block_session>(new Block_session(m_fd, it_sinfo->offset));
    if (sptr->get_sid() != sid) {
        throw File_exception(FDS_ERR_INTERNAL, "Failed to load a Transport Session (ID: "
            + std::to_string(sid) + ") based on the Content Table from the file (ID mismatch)");
    }

    m_sessions[sid] = std::move(sptr);
    return m_sessions[sid].get();
}

/**
 * @brief Rebuilt the Content Table from the file
 *
 * The function iterates through all blocks in the file and adds information about occurrences
 * of important blocks to the Content Table. For each Transport Session block its definition is
 * also immediately parsed and added to the list of Transport Sessions.
 *
 * @note
 *   This operation can be very expensive in case of large files. It is always preferred to load
 *   the prebuild table from the file (if available).
 * @note
 *   Previous content of the Content Table is cleared.
 * @throw File_exception if the file is malformed
 */
void
File_reader::ctable_rebuild()
{
    // Remove any previous content
    m_ctable.clear();

    // First, determine the end of the file
    off_t offset_eof = lseek(m_fd, 0, SEEK_END);
    if (offset_eof < 0) {
        File_exception::throw_errno(errno, std::string(__PRETTY_FUNCTION__) + ": lseek() failed");
    }

    // Start right after the file header
    off_t offset = sizeof(struct fds_file_hdr);

    constexpr size_t size_session = sizeof(struct fds_file_bsession);
    constexpr size_t size_dblock_hdr = offsetof(struct fds_file_bdata, data);
    constexpr size_t buffer_size = (size_session > size_dblock_hdr) ? size_session : size_dblock_hdr;
    uint8_t buffer[buffer_size];
    Io_sync io_req(m_fd, buffer, buffer_size);

    while (offset + static_cast<off_t>(buffer_size) <= offset_eof) {
        // Load the content to the buffer
        io_req.read(offset, buffer_size);
        if (io_req.wait() != buffer_size) {
            throw File_exception(FDS_ERR_INTERNAL, "Failed to load a Common Block header (offset: "
                + std::to_string(offset) + ") while rebuilding the Content Table");
        }

        const auto *block_hdr = reinterpret_cast<struct fds_file_bhdr *>(buffer);
        uint16_t block_type = le16toh(block_hdr->type);
        uint64_t block_len = le64toh(block_hdr->length);

        if (block_len == 0) {
            throw File_exception(FDS_ERR_INTERNAL, "Zero length Common Block header (offset: "
                + std::to_string(offset) + ") found while rebuilding the Content Table");
        }

        if (offset + static_cast<off_t>(block_len) > offset_eof) {
            // The block not complete (probably it is still being written)
            break;
        }

        // Process important blocks
        if (block_type == FDS_FILE_BTYPE_SESSION) {
            // Process the Transport Session block (whole structure is available)
            const auto *sblock = reinterpret_cast<const struct fds_file_bsession *>(buffer);
            ctable_process_sblock(offset, sblock);
        } else if (block_type == FDS_FILE_BTYPE_DATA) {
            // Process the Data block (only the block header is available)
            const auto *dblock = reinterpret_cast<const struct fds_file_bdata *>(buffer);
            ctable_process_dblock(offset, dblock);
        }

        offset += block_len;
    }
}

/**
 * @brief Add Transport Session block metadata into the Content Table (auxiliary function)
 *
 * @note To improve performance, the definition is also parsed and added to the internal list.
 * @param[in] offset Offset of the block from the start of the file
 * @param[in] block  Session block
 * @throw File_exception if the block is malformed or already loaded but different
 */
void
File_reader::ctable_process_sblock(off_t offset, const struct fds_file_bsession *block)
{
    assert(le16toh(block->hdr.type) == FDS_FILE_BTYPE_SESSION && "Session block expected!");

    // Parse the block
    auto session = std::unique_ptr<Block_session>(new Block_session(m_fd, offset));
    uint16_t sid = session->get_sid();
    assert(le16toh(block->session_id) == sid && "Transport Session ID doesn't match");

    // Add it to the Content Table
    uint64_t block_len = le64toh(block->hdr.length);
    m_ctable.add_session(offset, block_len, sid);

    // Check if the Transport Session definition hasn't been already loaded
    auto it = m_sessions.find(sid);
    if (it != m_sessions.end()) {
        // Already loaded
        if (session->get_struct() == *it->second) {
            return;
        }

        throw File_exception(FDS_ERR_INTERNAL, "Failed to load Transport Session block (offset: "
            + std::to_string(offset) + ") - a different Transport Session with the same ID has "
            "been already defined");
    }

    // Store the Transport Session description
    m_sessions[sid] = std::move(session);
}

/**
 * @brief Add Data block metadata into the Content Table (auxiliary function)
 * @param[in] offset Offset of the block from the start of the file
 * @param[in] block  Data block header (without content)
 */
void
File_reader::ctable_process_dblock(off_t offset, const struct fds_file_bdata *block)
{
    assert(le16toh(block->hdr.type) == FDS_FILE_BTYPE_DATA && "Data block expected!");

    uint64_t block_len = le64toh(block->hdr.length);
    uint64_t tmplt_offset = le64toh(block->offset_tmptls);
    uint32_t odid = le32toh(block->odid);
    uint16_t sid = le16toh(block->session_id);

    m_ctable.add_data_block(offset, block_len, tmplt_offset, odid, sid);
}

/**
 * @brief Schedule load of Data Blocks
 *
 * Replace the current Data Block reader with next one and optionally initialize (asynchronous)
 * read of the following Data Block.
 *
 * @note
 *   If the current Data Block is not defined (i.e. nullptr) after calling this function, no more
 *   Data Blocks are available.
 */
void
File_reader::scheduler()
{
#ifndef NDEBUG
    // The current Data Block must be completely processed before scheduling
    struct fds_drec aux_rec;
    assert(!m_db_current || m_db_current->next_rec(&aux_rec, nullptr) == FDS_EOC);
#endif

    if (m_db_current) {
        // Move the current reader to the list of idle Data Block readers
        m_db_idles.emplace_back(std::move(m_db_current));
        m_db_current = nullptr;
    }

    if (m_db_next) {
        // The next Data Block reader is ready to be used
        scheduler_next2current();
    }

    // Prepare the next Data Block to read
    assert(m_db_next == nullptr && "The next Data Block must be undefined!");
    scheduler_prepare_next();

    if (!m_db_current && m_db_next) {
        // The current block is not available but the next is (after initialization or rewind)
        scheduler_next2current();
        scheduler_prepare_next();
    }
}

/**
 * @brief
 *   Replace the current Data Block reader with the next Data Block reader using Content Table
 *   assistance (auxiliary function)
 *
 * The associated Transport Session Block and Template Block of the next Data Block are loaded from
 * the file (if they haven't been loaded earlier) and stored into internal lists and bind to the
 * next Data Block.
 *
 * As the result, the current Data Block reader (if not nullptr) is moved to the list of idle
 * readers and the next Data Block reader is marked as current.
 * @throw File_exception if loading of any Block failed
 */
void
File_reader::scheduler_next2current()
{
    assert(m_db_next != nullptr && "The next Data Block must be defined!");
    assert(m_db_next_idx < m_ctable.get_data_blocks().size() && "Index out of range!");

    if (m_db_current) {
        // Move the current reader to the list of idle Data Block readers
        m_db_idles.emplace_back(std::move(m_db_current));
        m_db_current = nullptr;
    }

    /*
     * Order of the following operations is important!
     * Usually, blocks (on which the next Data Block depends) are ordered in the file as:
     *   Transport Session -> Template Block -> Data Block
     * Therefore, we should preserve order of loading blocks in this order.
     *
     * Moreover, if asynchronous read is used for loading of the Data Block, we give it enough time
     * to complete before we need to access its Data Records.
     */

    // Extract information about of the next Data Block to read from the Content Table
    const auto &dblock_info = m_ctable.get_data_blocks()[m_db_next_idx];

    // Check if the definition of its Transport Session has been already loaded
    const Block_session *sblock_info = get_sblock(dblock_info.session_id);
    if (!sblock_info) {
        throw File_exception(FDS_ERR_INTERNAL, "Unable to find a definition of Transport Session "
            "ID " + std::to_string(dblock_info.session_id));
    }
    assert(sblock_info->get_sid() == dblock_info.session_id && "Sesssion ID mismatch!");

    // Check if its Template Block has been already loaded
    struct tblock_info &tblock_info = get_tblock(dblock_info.tmplt_offset);
    if (tblock_info.sid != dblock_info.session_id || tblock_info.odid != dblock_info.odid) {
        // The reference in the Content Table contains unexpected Transport Session ID or ODID
        throw File_exception(FDS_ERR_INTERNAL, "Failed to load a Template Block for the next "
            "Data Block based on the Content Table (Transport Session ID or ODID mismatch)");
    }

    /*
     * FIRST TOUCH (if the Data Block hasn't been loaded yet, it will be now!)
     * Note: Accessing any structure of the next Data Block (m_db_next) earlier would cause
     *   blocking until (asynchronous) read of the Block is complete, therefore, no data access
     *   is made until this point.
     */
    const struct fds_file_bdata *dblock_hdr = m_db_next->get_block_header();
    const uint64_t dblock_toff = le64toh(dblock_hdr->offset_tmptls);
    const uint32_t dblock_odid = le32toh(dblock_hdr->odid);
    const uint16_t dblock_sid = le16toh(dblock_hdr->session_id);

    if (dblock_sid != dblock_info.session_id || dblock_odid != dblock_info.odid) {
        // Unexpected Session ID or ODID was loaded due to the invalid record in the Content Table
        throw File_exception(FDS_ERR_INTERNAL, "Failed to load a Data Block based on the Content "
            "Table (Transport Session ID or ODID mismatch)");
    }

    if (dblock_toff != dblock_info.tmplt_offset) {
        // Invalid Template Block was prepared due to the invalid record in the Content Table
        throw File_exception(FDS_ERR_INTERNAL, "Failed to load a proper Template Block for the "
            "next Data Block due to invalid record in the Content Table");
    }

    m_db_next->set_templates(tblock_info.block.snapshot());
    m_db_current = std::move(m_db_next);
    m_db_next = nullptr;

    // Change index of the next Data Block to prepare
    m_db_next_idx++;
}

/**
 * @brief Prepare the next Data Block reader using Content Table assistance (auxiliary function)
 *
 * The next Data Block is assigned to an idle Data Block reader and the reader is marked as the
 * next. Moreover, if asynchronous read is used, it will start to load the block in background.
 *
 * The determined where the next block is located, Content Table is used. If the Transport
 * Session/ODID filter is enabled, not required Data Blocks are effectively skipped.
 *
 * @warning
 *   Content Table must be available and loaded!
 */
void
File_reader::scheduler_prepare_next()
{
    assert(m_db_next == nullptr && "The next Data Block must not be defined!");
    assert(!m_db_idles.empty() && "The must be always at least one idle Data Block reader!");

    const struct Block_content::info_data_block *dblock_next = nullptr;
    const std::vector<Block_content::info_data_block> &dblock_list = m_ctable.get_data_blocks();
    while (m_db_next_idx < dblock_list.size()) {
        // Check if the Data Block is required by the user
        const struct Block_content::info_data_block *aux = &dblock_list[m_db_next_idx];
        if (!sfilter_match(aux->session_id, aux->odid)) {
            // User don't want to see data from this block
            m_db_next_idx++;
            continue;
        }

        dblock_next = aux;
        break;
    }

    if (dblock_next == nullptr) {
        // No more Data Blocks to process
        return;
    }

    // Configure the first idle reader to start loading the next Data Block
    m_db_next = std::move(m_db_idles.front());
    m_db_idles.pop_front();

    /*
     * For asynchronous I/O it will start loading of the block in the background immediately.
     * For synchronous I/O it will only initialize the reader but loading is postponed until the
     *   Data Records are not required (yes, that's what we want)
     */
    m_db_next->load_from_file(m_fd, dblock_next->offset, dblock_next->len, m_io_type);
}

/**
 * @brief Transport Session and ODID filter test
 *
 * Check if a Data Block with a given Transport Session ID and ODID should be processed (or
 * not) based on Transport Session and ODID filter.
 *
 * @param[in] sid  Internal Transport Session ID
 * @param[in] odid Observation Domain ID
 * @return True or false
 */
bool
File_reader::sfilter_match(uint16_t sid, uint32_t odid)
{
    if (!m_sfilter.enabled) {
        // If disabled, accept all Transport Sessions and ODIDs
        return true;
    }

    if (m_sfilter.sid_all.count(sid) != 0) {
        // Found on the list of "widely" accepted Transport Sessions
        return true;
    }

    if (m_sfilter.odids_all.count(odid) != 0) {
        // Found on the list of "widely" accepted ODIDs
        return true;
    }

    const auto it = m_sfilter.combi.find(sid);
    if (it != m_sfilter.combi.end()) {
        if (it->second.count(odid) != 0) {
            /// Accepted combination of Transport Session ID and ODID found
            return true;
        }
    }

    // Not found
    return false;
}