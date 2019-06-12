/**
 * @file   src/file/Block_templates.cpp
 * @brief  Template manager (source file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   April 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <cstring>
#include <libfds.h>

#include "Block_templates.hpp"
#include "File_exception.hpp"
#include "Io_sync.hpp"
#include "structure.h"

using namespace fds_file;

Block_templates::Block_templates() : m_tmgr(nullptr, &fds_tmgr_destroy)
{
    // By default, we want the least restricted version of the Template manager
    m_tmgr.reset(fds_tmgr_create(FDS_SESSION_FILE));
    if (!m_tmgr) {
        throw std::bad_alloc();
    }

    // All Templates will be defined with the same Export time
    int rc = fds_tmgr_set_time(m_tmgr.get(), 0);
    if (rc != FDS_OK) {
        if (rc == FDS_ERR_NOMEM) {
            throw std::bad_alloc();
        } else {
            throw File_exception(FDS_ERR_INTERNAL, "Unable to configure a Template manager");
        }
    }
}

uint64_t
Block_templates::load_from_file(int fd, off_t offset, uint16_t *sid, uint32_t *odid)
{
    // Remove all IPFIX (Options) Templates
    clear();

    // Determine size of the block
    struct fds_file_bhdr block_hdr;
    constexpr size_t block_hdr_size = sizeof(block_hdr);

    Io_sync hdr_reader(fd, &block_hdr, block_hdr_size);
    hdr_reader.read(offset, block_hdr_size);
    if (hdr_reader.wait() != block_hdr_size) {
        throw File_exception(FDS_ERR_INTERNAL, "Failed to load the block header");
    }

    // Check the common block header
    if (le16toh(block_hdr.type) != FDS_FILE_BTYPE_TMPLTS) {
        throw File_exception(FDS_ERR_INTERNAL, "The block type doesn't match");
    }

    const size_t msg_hdr_size = offsetof(struct fds_file_btmplt, recs);
    const size_t rec_hdr_size = offsetof(struct fds_file_trec, data);
    uint64_t bsize = le64toh(block_hdr.length);
    if (bsize < msg_hdr_size) {
        throw File_exception(FDS_ERR_INTERNAL, "The block size is too small");
    }

    // Read the whole Template block into a buffer
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[bsize]);
    Io_sync block_reader(fd, buffer.get(), bsize);
    block_reader.read(offset, bsize);
    if (block_reader.wait() != bsize) {
        throw File_exception(FDS_ERR_INTERNAL, "read() failed to load the whole block");
    }

    // Fill ODID and Transport Session ID
    auto block_ptr = reinterpret_cast<fds_file_btmplt *>(buffer.get());
    if (sid) {
        *sid = le16toh(block_ptr->session_id);
    }
    if (odid) {
        *odid = le32toh(block_ptr->odid);
    }

    // Process all IPFIX (Options) Templates
    uint8_t *block_rec_pos = &buffer[msg_hdr_size];
    const uint8_t *block_end = &buffer[bsize];

    while (block_rec_pos + rec_hdr_size <= block_end) {
        // Parse the record header
        auto rec_ptr = reinterpret_cast<fds_file_trec *>(block_rec_pos);
        uint16_t trec_type = le16toh(rec_ptr->type);
        uint16_t trec_size = le16toh(rec_ptr->length);

        if (block_rec_pos + trec_size > block_end) {
            throw File_exception(FDS_ERR_INTERNAL, "Unexpected end of the block");
        }

        // Add IPFIX (Options) Template to the manager
        auto enum_type = static_cast<enum fds_template_type>(trec_type);
        add(enum_type, rec_ptr->data, trec_size - rec_hdr_size);

        // Move the pointer to the next record
        block_rec_pos += trec_size;
    }

    return bsize;
}

uint64_t
Block_templates::write_to_file(int fd, off_t offset, uint16_t sid, uint32_t odid)
{
    // First, calculate size of the buffer of the block
    const fds_tsnapshot_t *snap = snapshot();
    size_t tdata_size = 0;

    for (const uint16_t tid : m_ids) {
        const fds_template *tmplt = fds_tsnapshot_template_get(snap, tid);
        tdata_size += tmplt->raw.length;
    }

    // Allocate buffer for the Template block (+ padding)
    const size_t msg_hdr_size = offsetof(struct fds_file_btmplt, recs);
    const size_t rec_hdr_size = offsetof(struct fds_file_trec, data);
    size_t bsize = msg_hdr_size + (rec_hdr_size * m_ids.size()) + tdata_size;
    size_t padding = 0;
    if (bsize % 4U != 0) {
        const size_t bsize_cpy = bsize;
        bsize /= 4U;
        bsize++;
        bsize *= 4U;
        padding = bsize - bsize_cpy;
    }

    // Fill the header(s)
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[bsize]);
    auto *block = reinterpret_cast<struct fds_file_btmplt *>(buffer.get());
    block->hdr.type = htole16(FDS_FILE_BTYPE_TMPLTS);
    block->hdr.flags = htole16(0);
    block->hdr.length = htole64(bsize);

    block->odid = htole32(odid);
    block->session_id = htole16(sid);

    // Add all IPFIX (Options) Templates as records
    uint8_t *rec_pos = reinterpret_cast<uint8_t *>(&block->recs[0]);

    for (const uint16_t tid : m_ids) {
        const fds_template *tmplt = fds_tsnapshot_template_get(snap, tid);
        uint16_t rec_size = rec_hdr_size + tmplt->raw.length;
        assert(rec_pos + rec_size <= &buffer[bsize] && "writing behind the end of the buffer");

        auto rec = reinterpret_cast<struct fds_file_trec *>(rec_pos);
        rec->type = htole16(tmplt->type);
        rec->length = htole16(rec_size);
        std::memcpy(&rec->data[0], tmplt->raw.data, tmplt->raw.length);
        // Move pointer to the next record
        rec_pos += rec_size;
    }

    // Fill the padding with zeros
    if (padding > 0) {
        memset(&buffer[bsize - padding], 0, padding);
    }

    // Write to the file
    Io_sync io_writer(fd, buffer.get(), bsize);
    io_writer.write(offset, bsize);
    if (io_writer.wait() != bsize) {
        throw File_exception(FDS_ERR_INTERNAL, "Failed to write a Table block");
    }
    return bsize;
}

void
Block_templates::ie_source(const fds_iemgr_t *mgr)
{
    // Assign the IE manager to the Template Manager
    int rc = fds_tmgr_set_iemgr(m_tmgr.get(), mgr);
    if (rc != FDS_OK) {
        throw std::bad_alloc();
    }

    // Destroy garbage (old templates and snapshots)
    fds_tgarbage_t *garbage = nullptr;
    if (fds_tmgr_garbage_get(m_tmgr.get(), &garbage) == FDS_ERR_NOMEM) {
        throw std::bad_alloc();
    }
    if (garbage != nullptr) {
        fds_tmgr_garbage_destroy(garbage);
    }

    // Configure the Export time (lost during possible update of internal snapshots)
    rc = fds_tmgr_set_time(m_tmgr.get(), 0);
    switch (rc) {
    case FDS_OK:
        return;
    case FDS_ERR_NOMEM:
        throw std::bad_alloc();
    default:
        throw File_exception(FDS_ERR_INTERNAL, "Failed to assign an IE manager to the Template "
            "manager");
    }
}

void
Block_templates::add(enum fds_template_type type, const uint8_t *tdata, uint16_t tsize)
{
    if (type != FDS_TYPE_TEMPLATE && type != FDS_TYPE_TEMPLATE_OPTS) {
        throw File_exception(FDS_ERR_FORMAT, "Unable to parse unknown type of the Template");
    }

    struct fds_template *tmplt_ptr;
    uint16_t size_real = tsize;

    // First, parse the IPFIX Template
    int rc = fds_template_parse(type, tdata, &size_real, &tmplt_ptr);
    if (rc != FDS_OK) {
        // Failed
        if (rc == FDS_ERR_NOMEM) {
            throw std::bad_alloc();
        } else {
            throw File_exception(FDS_ERR_FORMAT, "Invalid definition of IPFIX (Options) Template");
        }
    }

    // Create a wrapper that will free the parsed Template in case of an exception
    std::unique_ptr<struct fds_template, decltype(&fds_template_destroy)>
        tmplt(tmplt_ptr, &fds_template_destroy);

    if (size_real != tsize) {
        throw File_exception(FDS_ERR_FORMAT, "Size of the parsed IPFIX (Options) Template "
            "doesn't match the given size.");
    }
    if (tmplt->fields_cnt_total == 0) {
        throw File_exception(FDS_ERR_FORMAT, "Templates Withdrawal cannot be added!");
    }

    // Add the Template to the manager or replace the current one
    uint16_t tid = tmplt->id;
    rc = fds_tmgr_template_add(m_tmgr.get(), tmplt.get());
    if (rc != FDS_OK) {
        if (rc == FDS_ERR_NOMEM) {
            throw std::bad_alloc();
        } else {
            throw File_exception(FDS_ERR_INTERNAL, "Failed to add the IPFIX (Options) Template "
                "definition");
        }
    }

    // The ownership now belongs to the Template manager
    (void) tmplt.release();
    m_ids.emplace(tid);
}

const struct fds_template *
Block_templates::get(uint16_t tid)
{
    if (m_ids.count(tid) == 0) {
        /*
         * This check should be faster because it will not potentially create a snapshot inside the
         * the internal Template manager if the Template is not present.
         */
        return nullptr;
    }

    const struct fds_template *tmplt_ptr;
    int rc = fds_tmgr_template_get(m_tmgr.get(), tid, &tmplt_ptr);
    switch (rc) {
    case FDS_OK:
        return tmplt_ptr;
    case FDS_ERR_NOTFOUND:
        return nullptr;
    case FDS_ERR_NOMEM:
        throw std::bad_alloc();
    default:
        throw File_exception(FDS_ERR_INTERNAL, "Unable to get an IPFIX (Options) Template");
    }
}

void
Block_templates::remove(uint16_t tid)
{
    // Remove the Template
    int rc = fds_tmgr_template_withdraw(m_tmgr.get(), tid, FDS_TYPE_TEMPLATE_UNDEF);
    switch (rc) {
    case FDS_OK:
        break;
    case FDS_ERR_NOTFOUND:
        throw File_exception(FDS_ERR_NOTFOUND, "IPFIX (Options) Template with the given Template "
            "ID not found.");
    case FDS_ERR_NOMEM:
        throw std::bad_alloc();
    default:
        throw File_exception(FDS_ERR_INTERNAL, "Failed to remove the IPFIX (Options) Template");
    }

    m_ids.erase(tid);
}

const fds_tsnapshot_t *
Block_templates::snapshot()
{
    const fds_tsnapshot_t *snap_ptr;
    int rc = fds_tmgr_snapshot_get(m_tmgr.get(), &snap_ptr);
    switch (rc) {
    case FDS_OK:
        return snap_ptr;
    case FDS_ERR_NOMEM:
        throw std::bad_alloc();
    default:
        throw File_exception(FDS_ERR_INTERNAL, "Failed to get a snapshot from the Template manager");
    }
}

void
Block_templates::clear()
{
    int rc;
    fds_tgarbage_t *garbage = nullptr;

    // Cleanup the manager
    m_ids.clear();
    fds_tmgr_clear(m_tmgr.get());
    if (fds_tmgr_garbage_get(m_tmgr.get(), &garbage) == FDS_OK && garbage != nullptr) {
        fds_tmgr_garbage_destroy(garbage);
    }

    // Set the time context again
    rc = fds_tmgr_set_time(m_tmgr.get(), 0);
    switch (rc) {
    case FDS_OK: // Success
        return;
    case FDS_ERR_NOMEM:
        throw std::bad_alloc();
    default:
        throw File_exception(FDS_ERR_INTERNAL, "Failed to clear a Template manager");
    }
}
