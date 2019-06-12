/**
 * @file   src/file/File_writer.cpp
 * @brief  File writer (source file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   May 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <endian.h>
#include <algorithm>

#include <sys/types.h> // lseek
#include <unistd.h>    // lseek, lockf

#include "File_exception.hpp"
#include "File_writer.hpp"

using namespace fds_file;

File_writer::File_writer(const char *path, fds_file_alg calg, bool append, Io_factory::Type io_type)
    : File_base(path, append ? File_base::CF_APPEND : File_base::CF_TRUNC, File_base::DEF_MODE, calg),
      m_io_type(io_type)
{
    /*
     * Lock the whole file for writing (only this process must be able to write to the file)
     * Note: Unlocking is performed automatically when the file descriptor is closed. Therefore,
     *   if this constructor fails, the base class destructor will close it.
     */
    if (lseek(m_fd, 0, SEEK_SET) == -1) {
        File_exception::throw_errno(errno, "lseek() failed");
    }
    if (lockf(m_fd, F_TLOCK, 0) != 0) {
        // Failed
        File_exception::throw_errno(errno, "Unable to lock the file (it's probably being written "
            "by another process)", FDS_ERR_DENIED);
    }

    // If append mode is enabled, try to load the file header (if exists) and initialize internals
    if (append) {
        // Check the file size
        off_t size = lseek(m_fd, 0, SEEK_END);
        if (size == -1) {
            File_exception::throw_errno(errno, "lseek() failed");
        }

        if (size != 0) {
            // The file is not empty, try to prepare for append
            append_prepare();
            return;
        }

        // Fall through i.e. create a new file content
    }

    // Write the default file header to the file (prepared by the base class)
    file_hdr_store();
    // Set the offset of the next block to add right behind the file header
    const size_t hdr_size = sizeof(struct fds_file_hdr);
    m_offset = hdr_size;
}

File_writer::~File_writer()
{
    try {
        // Store all Data Blocks (if not empty) and their Template Blocks (if modified)
        flush_all();
        // Store the Content Table to the file
        m_ctable.write_to_file(m_fd, m_offset);
        // Update the file header and statistics
        file_hdr_set_ctable(m_offset);
        file_hdr_store();
    } catch (File_exception &ex) {
        // TODO: report it somehow
    }
}

/**
 * @brief Initialize internals for append mode
 *
 * First of all, the file header is loaded and parsed. Next, the Content Table (the last block
 * in the file) is loaded and prepared for use. Using the Content Table, all Transport Session
 * descriptions are extracted.
 *
 * Newly appended file blocks will be placed starting from the Content Table position. In other
 * words, the table will be overwritten and written back to the file when the file is closed.
 *
 * @throw File_exception if the initialization fails and file cannot be appended
 */
void
File_writer::append_prepare()
{
    assert(m_sessions.empty() && "The internal structures must be empty!");

    // Try to load the file header (and statistics)
    file_hdr_load();

    if (file_hdr_get_version() != FDS_FILE_VERSION) {
        throw File_exception(FDS_ERR_INTERNAL, "Unable to append a newer version of the file");
    }

    // Try to load the Content table
    uint64_t ctable_offset = file_hdr_get_ctable();
    if (ctable_offset == 0) {
        throw File_exception(FDS_ERR_DENIED, "File is corrupted or already opened for writing "
            "(Content Table position is undefined)");
    }
    m_ctable.load_from_file(m_fd, ctable_offset);
    // Set the offset of the next block to overwrite the Content Table
    m_offset = ctable_offset;

    // Load all Transport Sessions
    for (const struct Block_content::info_session &rec : m_ctable.get_sessions()) {
        // First, load the Session description
        Block_session sloader(m_fd, rec.offset);
        if (sloader.get_sid() != rec.session_id) {
            throw File_exception(FDS_ERR_INTERNAL, "Content Table record of a Transport Session "
                "description doesn't match its parameters (different internal IDs)");
        }

        // Second, create a new record and add it to the map
        const uint16_t sid = rec.session_id;
        const struct fds_file_session *session_dsc = &sloader.get_struct();
        std::unique_ptr<struct session_info> info(new session_info(sid, session_dsc));
        info->m_sblock_offset = rec.offset; // Offset must be defined
        m_session2id[&info->m_sblock_data] = sid;
        m_sessions[sid] = std::move(info);

        assert(m_session2id.size() == m_sessions.size() && "Number of records must be the same!");
    }

    // Remove information about the Content Table because it will be overwritten
    file_hdr_set_ctable(0);
    file_hdr_store();
}

/**
 * @brief Flush all Data Blocks to the file
 *
 * If their Template Blocks haven't been stored yet (or have been modified), the function
 * will stored them into the file too.
 * @throw File_exception if any write operation fails
 */
void
File_writer::flush_all()
{
    // For each Transport Session
    for (auto &session : m_sessions) {
        struct session_info *sinfo = session.second.get();
        assert(sinfo->m_sblock_offset != 0 && "The session block must be already written");

        // For each ODID of the Transport Session
        for (auto &odid : sinfo->m_odids) {
            struct odid_info *oinfo = odid.second.get();
            // Flush Template Block (if modified) and Data Block (if not empty)
            flush(oinfo);
        }
    }
}

/*
 * @brief Flush a Data Block of a specified combination of Transport Session and ODID to the file
 * @param[in] oinfo ODID info
 */
void
File_writer::flush(odid_info *oinfo)
{
    assert(m_sessions[oinfo->m_sid]->m_sblock_offset != 0 && "Session Block must be written!");
    uint64_t bsize;

    // Are there any Data Records that haven't been stored yet?
    if (oinfo->m_data.count() == 0) {
        return;
    }

    // Check if the Template Block has been already written
    if (oinfo->m_tblock_offset == 0) {
        bsize = oinfo->m_tblock_data.write_to_file(m_fd, m_offset, oinfo->m_sid, oinfo->m_odid);
        oinfo->m_tblock_offset = m_offset;
        m_offset += bsize;
    }

    // Write the Data Block and add metadata about the block to the Content Table
    bsize = oinfo->m_data.write_to_file(m_fd, m_offset, oinfo->m_sid, oinfo->m_tblock_offset,
        m_io_type);
    m_ctable.add_data_block(m_offset, bsize, oinfo->m_tblock_offset, oinfo->m_odid, oinfo->m_sid);
    m_offset += bsize;
}

void
File_writer::iemgr_set(const fds_iemgr_t *iemgr)
{
    m_iemgr = iemgr;

    // For each Transport Session and its ODID
    for (auto &session : m_sessions) {
        for (auto &odid : session.second->m_odids) {
            // Update Templates and remove a reference to the last used Template (not valid pointer)
            struct odid_info *info = odid.second.get();
            info->m_tmplt_last = nullptr;
            info->m_tblock_data.ie_source(iemgr);
        }
    }
}

const struct fds_file_session *
File_writer::session_get(fds_file_sid_t sid)
{
    const auto pos = m_sessions.find(sid);
    if (pos == m_sessions.end()) {
        return nullptr;
    }

    struct session_info *sinfo = pos->second.get();
    return &sinfo->m_sblock_data.get_struct();
}

void
File_writer::session_list(fds_file_sid_t **arr, size_t *size)
{
    /* All Transport Sessions are immediately written to the file and Content Table, therefore,
     * the default implementation based on the Content Table can be used.
     */
    File_base::session_list_from_ctable(m_ctable, arr, size);
}

void
File_writer::session_odids(fds_file_sid_t sid, uint32_t **arr, size_t *size)
{
    /* The default implementation cannot be used because Data Blocks are not defined in the Content
     * Table until they are written to the file.
     */

    // Is the Transport Session defined at all?
    const auto session_it = m_sessions.find(sid);
    if (session_it == m_sessions.end()) {
        *arr = nullptr;
        *size = 0;
        return;
    }

    const auto &odid_map = session_it->second->m_odids;
    const size_t cnt = odid_map.size();
    if (cnt == 0) {
        *arr = nullptr;
        *size = 0;
        return;
    }

    uint32_t *array = (uint32_t *) malloc(cnt * sizeof(*array));
    if (!array) {
        throw std::bad_alloc();
    }

    size_t idx = 0;
    for (const auto &odid : odid_map) {
        assert(idx < cnt && "Index out of range!");
        array[idx++] = odid.first;
    }

    *arr = array;
    *size = cnt;
}

fds_file_sid_t
File_writer::session_add(const struct fds_file_session *info)
{
    // First of all, try to find it in the list of already added Transport Sessions
    Block_session tmp_session(0, info); // Create a temporary object
    const auto result_id = m_session2id.find(&tmp_session);
    if (result_id != m_session2id.end()) {
        // Already defined
        const auto result_ref = m_sessions.find(result_id->second);
        if (result_ref == m_sessions.end()) {
            // Something is wrong, it must be already defined
            throw File_exception(FDS_ERR_INTERNAL, "Transport Session already defined, but its "
                "definition cannot be found!");
        }

        assert(result_ref->second->m_sblock_data == *info && "Structures ḾUST match!");
        return result_ref->first;
    }

    // Not found -> try to add a new one
    size_t next_id = m_sessions.size() + 1U; // Skip ID 0 (reserved)
    if (next_id > UINT16_MAX) {
        throw File_exception(FDS_ERR_DENIED, "Maximum number of Transport Sessions has been "
            "reached");
    }

    assert(next_id <= UINT16_MAX && "Transport Session ID must be <= (2^16) - 1");
    uint16_t new_sid = static_cast<uint16_t>(next_id);
    auto ptr = std::unique_ptr<struct session_info>(new session_info(new_sid, info)); // may throw

    // Write the session description to the file and add metadata to the Content Table
    uint64_t wsize = ptr->m_sblock_data.write_to_file(m_fd, m_offset);
    m_ctable.add_session(m_offset, wsize, new_sid);
    ptr->m_sblock_offset = m_offset;
    m_offset += wsize;

    // Store the pointer
    m_session2id[&ptr->m_sblock_data] = new_sid;
    m_sessions[new_sid] = std::move(ptr);
    assert(m_session2id.size() == m_sessions.size() && "Number of records must be the same!");
    return new_sid;
}

void
File_writer::select_ctx(fds_file_sid_t sid, uint32_t odid, uint32_t exp_time)
{
    if (m_selected != nullptr && m_selected->m_sid == sid && m_selected->m_odid == odid) {
        // Just change the Export Time
        m_selected->m_data.set_etime(exp_time);
        return;
    }

    // Find the Transport Session by ID
    auto session_iter = m_sessions.find(sid);
    if (session_iter == m_sessions.end()) {
        // Not found
        throw File_exception(FDS_ERR_NOTFOUND, "Transport Session not found");
    }

    // Find the ODID in the Transport Session
    struct session_info *sinfo = session_iter->second.get();
    auto odid_iter = sinfo->m_odids.find(odid);
    if (odid_iter != sinfo->m_odids.end()) {
        // ODID found
        struct odid_info *oinfo = odid_iter->second.get();
        assert(oinfo->m_sid == sid && oinfo->m_odid == odid && "ODID and Session ID must match!");
        m_selected = oinfo;
        m_selected->m_data.set_etime(exp_time);
        return;
    }

    // Create a new ODID
    auto ptr = std::unique_ptr<struct odid_info>(new odid_info(sid, odid, file_hdr_get_calg()));
    ptr->m_tblock_data.ie_source(m_iemgr);
    sinfo->m_odids[odid] = std::move(ptr);
    m_selected = sinfo->m_odids[odid].get();
    m_selected->m_data.set_etime(exp_time);
}

void
File_writer::write_rec(uint16_t tid, const uint8_t *rec_data, uint16_t rec_size)
{
    if (!m_selected) {
        throw File_exception(FDS_ERR_ARG, "Context (i.e. Session and ODID) is not specified");
    }

    // Obtain IPFIX (Options) Template of the Data Record
    const fds_template *tmplt = m_selected->m_tmplt_last;
    if (tmplt == nullptr || tmplt->id != tid) {
        // Look up the IPFIX (Options) Template
        tmplt = m_selected->m_tblock_data.get(tid);
        if (tmplt == nullptr) {
            throw File_exception(FDS_ERR_NOTFOUND, "IPFIX (Options) Template not defined");
        }

        m_selected->m_tmplt_last = tmplt;
    }

    if (rec_size > m_selected->m_data.remains()) {
        // The buffer is full
        flush(m_selected);
    }

    // Try to add the Data Record
    m_selected->m_data.add(rec_data, rec_size, tmplt);
    // Extract statistics (bytes, packets, proto, etc)
    stats_update(rec_data, rec_size, tmplt);
}

void
File_writer::tmplt_add(enum fds_template_type t_type, const uint8_t *t_data, uint16_t t_size)
{
    if (!m_selected) {
        throw File_exception(FDS_ERR_ARG, "Context (i.e. Session and ODID) is not specified");
    }

    /* Check if the Template is already defined.
     * Note: Template ID definition is at the same position for both types of Templates so
     *  it is ok to cast it to any type...
     */
    const auto tmplt_hdr = reinterpret_cast<const struct fds_ipfix_trec *>(t_data);
    uint16_t tid = ntohs(tmplt_hdr->template_id);

    const fds_template *tmplt_def = m_selected->m_tblock_data.get(tid);
    if (!tmplt_def) {
        // Not defined
        m_selected->m_tblock_data.add(t_type, t_data, t_size);
        m_selected->m_tblock_offset = 0; // Make sure that the Template Block will be written later
        return;
    }

    // An IPFIX (Options) Template with the same ID already exists
    if (tmplt_def->type == t_type
            && tmplt_def->raw.length == t_size
            && memcmp(tmplt_def->raw.data, t_data, t_size) == 0) {
        // Templates are the same ... nothing to do
        return;
    }

    /* The templates are different but the Template ID is the same!
     * To make sure that all previously added Data Records based on current IPFIX (Options)
     * Template are interpretable, we have to flush the Data Block (if not empty) and the current
     * version of Template Block (if not already written) to the file. After that we can override
     * the current template and label the Template Manager as modified!
     */
    flush(m_selected);

    m_selected->m_tblock_data.add(t_type, t_data, t_size);
    m_selected->m_tblock_offset = 0;
    m_selected->m_tmplt_last = nullptr; // Just in case
}

void
File_writer::tmplt_remove(uint16_t tid)
{
    if (!m_selected) {
        throw File_exception(FDS_ERR_ARG, "Context (i.e. Session and ODID) is not specified");
    }

    if (m_selected->m_tblock_data.get(tid) == nullptr) {
        throw File_exception(FDS_ERR_NOTFOUND, "Template to remove is not defined");
    }

    if (m_selected->m_data.count() != 0) {
        // There are already Data Records that could be based on the Template
        flush(m_selected);
    }

    m_selected->m_tblock_data.remove(tid);
    m_selected->m_tblock_offset = 0; // Make sure that the Template Block will be written later
    m_selected->m_tmplt_last = nullptr; // Just in case
}

void
File_writer::tmplt_get(uint16_t tid, enum fds_template_type *t_type, const uint8_t **t_data,
    uint16_t *t_size)
{
    if (!m_selected) {
        throw File_exception(FDS_ERR_ARG, "Context (i.e. Session and ODID) is not specified");
    }

    const struct fds_template *tmplt = m_selected->m_tblock_data.get(tid);
    if (!tmplt) {
        throw File_exception(FDS_ERR_NOTFOUND, "Template with the given ID is not defined");
    }

    // Found
    if (t_type) {
        assert(tmplt->type == FDS_TYPE_TEMPLATE || tmplt->type == FDS_TYPE_TEMPLATE_OPTS);
        *t_type = tmplt->type;
    }

    if (t_data) {
        *t_data = tmplt->raw.data;
    }

    if (t_size) {
        *t_size = tmplt->raw.length;
    }
}
