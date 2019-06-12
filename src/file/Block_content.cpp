/**
 * @file   src/file/Block_content.hpp
 * @brief  Content table (header file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   May 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <bitset>
#include <cassert>
#include <cstddef>
#include <memory>

#include "Block_content.hpp"
#include "File_exception.hpp"
#include "Io_sync.hpp"
#include "structure.h"

using namespace fds_file;

void
Block_content::add_session(uint64_t offset, uint64_t len, uint16_t sid)
{
    assert(offset != 0 && "Offset of the block cannot be zero");
    assert(len != 0 && "Size of the block cannot be zero");

    if (m_sessions.size() >= size_t(UINT16_MAX) + 1) {
        throw File_exception(FDS_ERR_INTERNAL, "Too many Transport Sessions (over limit)");
    }

    struct info_session session = {offset, len, sid};
    m_sessions.emplace_back(session);
}

void
Block_content::add_data_block(uint64_t offset, uint64_t len, uint64_t tmplt_offset, uint32_t odid,
    uint16_t sid)
{
    assert(offset != 0 && "Offset of the block cannot be zero");
    assert(len != 0 && "Size of the block cannot be zero");
    assert(tmplt_offset != 0 && "Template Block offset cannot be zero");
    assert(tmplt_offset < offset && "Template Block must be placed before Data Block");

    if (m_dblocks.size() + 1 > UINT32_MAX) {
        throw File_exception(FDS_ERR_INTERNAL, "Too many Data Blocks (over limit)");
    }

    struct info_data_block dblock = {offset, len, tmplt_offset, odid, sid};
    m_dblocks.emplace_back(dblock);
}

void
Block_content::clear()
{
    m_sessions.clear();
    m_dblocks.clear();
}

uint64_t
Block_content::write_to_file(int fd, off_t offset)
{
    // Determine number of sections
    uint32_t flags = 0;
    unsigned int sections = 0;

    if (!m_sessions.empty()) {
        sections++;
        flags |= FDS_FILE_CTB_SESSION;
    }
    if (!m_dblocks.empty()) {
        sections++;
        flags |= FDS_FILE_CTB_DATA;
    }

    // Write all sections
    size_t hdr_size = offsetof(struct fds_file_bctable, offsets) + (sections * sizeof(uint64_t));
    std::unique_ptr<uint8_t[]> hdr_memory(new uint8_t[hdr_size]);
    fds_file_bctable *hdr_ptr = reinterpret_cast<struct fds_file_bctable *>(hdr_memory.get());

    unsigned int idx = 0;
    off_t rel_offset = hdr_size; // Relative offset from the start of the block
    if (!m_sessions.empty()) {
        hdr_ptr->offsets[idx++] = htole64(rel_offset);
        rel_offset += write_sessions(fd, offset + rel_offset);
    }

    if (!m_dblocks.empty()) {
        hdr_ptr->offsets[idx++] = htole64(rel_offset);
        rel_offset += write_data_blocks(fd, offset + rel_offset);
    }

    // Fill and write the header of the Content Table block
    hdr_ptr->hdr.type = htole16(FDS_FILE_BTYPE_TABLE);
    hdr_ptr->hdr.flags = htole16(0);
    hdr_ptr->hdr.length = htole64(rel_offset);
    hdr_ptr->block_flags = htole32(flags);

    // Write the header
    Io_sync req(fd, hdr_ptr, hdr_size);
    req.write(offset, hdr_size);
    if (req.wait() != hdr_size) {
        throw File_exception(FDS_ERR_INTERNAL, "Failed to write Content Table header");
    }

    return rel_offset;
}

/**
 * @brief Write a section with all Transport Sessions
 * @param[in] fd     File descriptor
 * @param[in] offset Offset of the section from the start of the file
 * @return Size of the section (in bytes)
 */
size_t
Block_content::write_sessions(int fd, off_t offset)
{
    if (m_sessions.empty()) {
        return 0;
    }

    // Prepare memory for the section
    const size_t rsize = sizeof(struct fds_file_ctable_session_rec);
    const size_t sec_size = offsetof(fds_file_ctable_session, recs) + (m_sessions.size() * rsize);
    std::unique_ptr<uint8_t[]> aux_mem(new uint8_t[sec_size]);
    auto *ptr = reinterpret_cast<struct fds_file_ctable_session *>(aux_mem.get());

    // Fill the header and records
    ptr->rec_cnt = htole16(static_cast<uint16_t>(m_sessions.size()));

    uint16_t idx = 0;
    for (const auto &rec_orig : m_sessions) {
        struct fds_file_ctable_session_rec *rec2fill = &ptr->recs[idx++];
        rec2fill->offset = htole64(rec_orig.offset);
        rec2fill->length = htole64(rec_orig.len);
        rec2fill->session_id = htole16(rec_orig.session_id);
        rec2fill->flags = htole16(0);
    }

    // Write the section
    Io_sync req(fd, ptr, sec_size);
    req.write(offset, sec_size);
    if (req.wait() != sec_size) {
        throw File_exception(FDS_ERR_INTERNAL, "Failed to write the Transport Session section of "
            "the Content Table");
    }

    return sec_size;
}

/**
 * @brief Write a section with all Data Blocks
 * @param[in] fd     File descriptor
 * @param[in] offset Offset of the section from the start of the file
 * @return Size of the section (in bytes)
 */
size_t
Block_content::write_data_blocks(int fd, off_t offset)
{
    if (m_dblocks.empty()) {
        return 0;
    }

    // Prepare memory for the section
    const size_t rsize = sizeof(struct fds_file_ctable_data_rec);
    const size_t sec_size  = offsetof(fds_file_ctable_data, recs) + (m_dblocks.size() * rsize);
    std::unique_ptr<uint8_t[]> aux_mem(new uint8_t[sec_size]);
    auto *ptr = reinterpret_cast<struct fds_file_ctable_data *>(aux_mem.get());

    // Fill the header and records
    ptr->rec_cnt = htole32(static_cast<uint32_t>(m_dblocks.size()));

    uint32_t idx = 0;
    for (const auto &rec_orig : m_dblocks) {
        struct fds_file_ctable_data_rec *rec2fill = &ptr->recs[idx++];
        rec2fill->offset = htole64(rec_orig.offset);
        rec2fill->length = htole64(rec_orig.len);
        rec2fill->offset_tmptls = htole64(rec_orig.tmplt_offset);
        rec2fill->odid = htole32(rec_orig.odid);
        rec2fill->session_id = htole16(rec_orig.session_id);
        rec2fill->flags = htole16(0);
    }

    // Write the section
    Io_sync req(fd, ptr, sec_size);
    req.write(offset, sec_size);
    if (req.wait() != sec_size) {
        throw File_exception(FDS_ERR_INTERNAL, "Failed to write the Data Block section of "
            "the Content Table");
    }

    return sec_size;
}

uint64_t
Block_content::load_from_file(int fd, off_t offset)
{
    // Remove all previous definitions
    clear();

    // Determine size of the block
    struct fds_file_bhdr block_hdr;
    constexpr size_t block_hdr_size = sizeof block_hdr;

    Io_sync hdr_reader(fd, &block_hdr, block_hdr_size);
    hdr_reader.read(offset, block_hdr_size);
    if (hdr_reader.wait() != block_hdr_size) {
        throw File_exception(FDS_ERR_INTERNAL, "Failed to load the Content Table header");
    }

    // Check the common block header
    if (le16toh(block_hdr.type) != FDS_FILE_BTYPE_TABLE) {
        throw File_exception(FDS_ERR_INTERNAL, "The block type doesn't match (expected Content"
            "Table)");
    }

    const size_t content_hdr_size = offsetof(struct fds_file_bctable, offsets);
    uint64_t bsize = le64toh(block_hdr.length);
    if (bsize < content_hdr_size) {
        throw File_exception(FDS_ERR_INTERNAL, "The block size of the Content Table is too small");
    }

    // Read the block into a buffer
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[bsize]);
    Io_sync block_reader(fd, buffer.get(), bsize);
    block_reader.read(offset, bsize);
    if (block_reader.wait() != bsize) {
        throw File_exception(FDS_ERR_INTERNAL, "read() failed to load the whole Content Table");
    }
    const auto *block_ptr = reinterpret_cast<struct fds_file_bctable *>(buffer.get());

    // Parse the offset table
    uint32_t block_flags = le32toh(block_ptr->block_flags);
    std::bitset<32> bset(block_flags);
    if (bsize < content_hdr_size + (bset.count() * sizeof(uint64_t))) {
        throw File_exception(FDS_ERR_INTERNAL, "Unexpected end of the Content Table block");
    }

    // Parse sections
    unsigned int idx = 0;
    if ((bset.to_ulong() & FDS_FILE_CTB_SESSION) != 0) {
        read_sessions(buffer.get(), bsize, le64toh(block_ptr->offsets[idx++]));
    }
    if ((bset.to_ulong() & FDS_FILE_CTB_DATA) != 0) {
        read_data_blocks(buffer.get(), bsize, le64toh(block_ptr->offsets[idx++]));
    }

    return bsize;
}

/**
 * @brief Read section with information about all Transport Sessions
 *
 * All parsed Transport Session are added to the local vector of Sessions
 * @param[in] bdata      Buffer with the whole Content Table
 * @param[in] bsize      Size of the buffer (in bytes)
 * @param[in] rel_offset Offset of the section from the start of the buffer
 * @return Size of the section
 */
size_t
Block_content::read_sessions(const uint8_t *bdata, size_t bsize, uint64_t rel_offset)
{
    const size_t hdr_size = offsetof(struct fds_file_ctable_session, recs);
    const size_t rec_size = sizeof(struct fds_file_ctable_session_rec);

    if (rel_offset + hdr_size > bsize) {
        throw File_exception(FDS_ERR_INTERNAL, "Unexpected end of the Content Table block");
    }

    const auto *ptr = reinterpret_cast<const struct fds_file_ctable_session *>(bdata + rel_offset);
    uint16_t rec_cnt = le16toh(ptr->rec_cnt);
    const size_t section_size = hdr_size + (rec_cnt * rec_size);
    if (rel_offset + section_size > bsize) {
        throw File_exception(FDS_ERR_INTERNAL, "Unexpected end of the Content Table block");
    }

    for (size_t i = 0; i < rec_cnt; ++i) {
        const struct fds_file_ctable_session_rec *rec_ptr = &ptr->recs[i];
        add_session(
            le64toh(rec_ptr->offset),
            le64toh(rec_ptr->length),
            le16toh(rec_ptr->session_id)
        );
    }

    return section_size;
}

/**
 * @brief Read section with information about all Data Blocks
 *
 * All parsed Data Block descriptions are added to the local vector of Data Blocks
 * @param[in] bdata      Buffer with the whole Content Table
 * @param[in] bsize      Size of the buffer (in bytes)
 * @param[in] rel_offset Offset of the section from the start of the buffer
 * @return Size of the section
 */
size_t
Block_content::read_data_blocks(const uint8_t *bdata, size_t bsize, uint64_t rel_offset)
{
    const size_t hdr_size = offsetof(struct fds_file_ctable_data, recs);
    const size_t rec_size = sizeof(struct fds_file_ctable_data_rec);

    if (rel_offset + hdr_size > bsize) {
        throw File_exception(FDS_ERR_INTERNAL, "Unexpected end of the Content Table block");
    }

    const auto *ptr = reinterpret_cast<const struct fds_file_ctable_data *>(bdata + rel_offset);
    uint32_t rec_cnt = le32toh(ptr->rec_cnt);
    const size_t section_size = hdr_size + (rec_cnt * rec_size);
    if (rel_offset + section_size > bsize) {
        throw File_exception(FDS_ERR_INTERNAL, "Unexpected end of the Content Table block");
    }

    for (size_t i = 0; i < rec_cnt; ++i) {
        const struct fds_file_ctable_data_rec *rec_ptr = &ptr->recs[i];
        add_data_block(
            le64toh(rec_ptr->offset),
            le64toh(rec_ptr->length),
            le64toh(rec_ptr->offset_tmptls),
            le32toh(rec_ptr->odid),
            le16toh(rec_ptr->session_id)
        );
    }

    return section_size;
}
