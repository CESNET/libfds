/**
 * @file   src/file/Block_data_writer.cpp
 * @brief  Data Block writer (source file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   April 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cassert>

#include <libfds.h>
#include <fds_lz4.h>
#include <fds_zstd.h>

#include "Block_data_writer.hpp"
#include "File_exception.hpp"
#include "Io_sync.hpp"

using namespace fds_file;

Block_data_writer::Block_data_writer(uint32_t odid, enum fds_file_alg comp_alg, uint16_t msg_size)
    : m_odid(odid), m_calg(comp_alg), m_size_max(msg_size)
{
    // Calculate buffer size big enough for compression of not compressible data
    size_t alloc_size = FDS_FILE_BDATA_HDR_SIZE;

    switch (comp_alg) {
    case FDS_FILE_CALG_NONE:
        alloc_size += m_capacity;
        break;
    case FDS_FILE_CALG_LZ4:
        alloc_size += LZ4_compressBound(m_capacity);
        break;
    case FDS_FILE_CALG_ZSTD:
        alloc_size += ZSTD_compressBound(m_capacity);
        break;
    default:
        throw File_exception(FDS_ERR_INTERNAL, "Unknown type of compression algorithm");
    }

    // Allocate buffers
    m_alloc = alloc_size;
    m_buffer_main.reset(new uint8_t[m_alloc]);
    if (comp_alg != FDS_FILE_CALG_NONE) {
        m_buffer_comp.reset(new uint8_t[m_alloc]);
    }

    // Initialize Data Block header on the top of the buffer
    reset_buffer();
}

Block_data_writer::~Block_data_writer()
{
    try {
        write_wait();
    } catch (File_exception &ex) {
        // Do nothing... unfortunately destructors cannot throw exceptions!
    }

    // Buffers can be now automatically destroyed
}

void
Block_data_writer::add(const uint8_t *data, uint16_t size, const struct fds_template *tmplt)
{
    // Check if the Data Record is valid based on the IPFIX (Options) Template
    uint16_t real_size = size;
    if (rec_length(data, &real_size, tmplt) != FDS_OK || real_size != size) {
        throw File_exception(FDS_ERR_FORMAT, "Size of the Data Record doesn't match its Template");
    }

    // Check if there is enough space in the buffer (for the worst case scenario)
    if (size > remains()) {
        throw File_exception(FDS_ERR_INTERNAL, "Unable to store the Data Record due to full buffer");
    }

    // Check if the record can at least fit into a new (i.e. empty) IPFIX Message
    if (size > UINT16_MAX - FDS_IPFIX_MSG_HDR_LEN - FDS_IPFIX_SET_HDR_LEN) {
        throw File_exception(FDS_ERR_FORMAT, "The Data Record exceeds the maximum allowed size");
    }

    /*
     * Check if a new IPFIX Message should be create (and the old one closed).
     * - Export time has been changed
     * - Size of the Message with the Data Record to add exceeds the maximum IPFIX Message size
     * - The is no IPFIX Message in the buffer yet
     */
    uint32_t size2add = (m_tid_now == tmplt->id) ? size : (size + FDS_IPFIX_SET_HDR_LEN);
    uint32_t msg_size = m_written - m_pos_msg;
    assert(msg_size <= UINT16_MAX && "Maximum Message size exceeded!");

    if (msg_size != 0 && (msg_size + size2add > m_size_max || m_etime_now != m_etime_set)) {
        // Close the current IPFIX Message (update the IPFIX Message and Set lengths)
        const uint32_t set_size = m_written - m_pos_set;
        assert(set_size <= UINT16_MAX - FDS_IPFIX_MSG_HDR_LEN && "Maximum Set size exceeded!");

        auto msg_ptr = reinterpret_cast<struct fds_ipfix_msg_hdr *>(&m_buffer_main[m_pos_msg]);
        auto set_ptr = reinterpret_cast<struct fds_ipfix_set_hdr *>(&m_buffer_main[m_pos_set]);
        msg_ptr->length = htons(msg_size);
        set_ptr->length = htons(set_size);
        msg_size = 0;
    }

    if (msg_size == 0) {
        // Create a new IPFIX Message header + IPFIX Set header
        m_pos_msg = m_written;
        m_pos_set = m_written + FDS_IPFIX_MSG_HDR_LEN;
        m_written = m_pos_set + FDS_IPFIX_SET_HDR_LEN;

        m_etime_now = m_etime_set; // Update the Export Time (could be changed)
        auto msg_ptr = reinterpret_cast<struct fds_ipfix_msg_hdr *>(&m_buffer_main[m_pos_msg]);
        msg_ptr->version = htons(FDS_IPFIX_VERSION);
        msg_ptr->export_time = htonl(m_etime_now);
        msg_ptr->seq_num = htonl(m_seq_next);
        msg_ptr->odid = htonl(m_odid);

        m_tid_now = tmplt->id;  // Update the Template ID (could be changed)
        auto set_ptr = reinterpret_cast<struct fds_ipfix_set_hdr *>(&m_buffer_main[m_pos_set]);
        set_ptr->flowset_id = htons(m_tid_now);
    } else if (m_tid_now != tmplt->id) {
        // Template has been changed -> update the length of the old Data Set and create a new one
        const uint32_t set_size = m_written - m_pos_set;
        assert(set_size <= UINT16_MAX - FDS_IPFIX_MSG_HDR_LEN && "Maximum Set size exceeded!");
        auto old_ptr = reinterpret_cast<struct fds_ipfix_set_hdr *>(&m_buffer_main[m_pos_set]);
        old_ptr->length = htons(set_size);

        m_tid_now = tmplt->id;
        m_pos_set = m_written;
        m_written += FDS_IPFIX_SET_HDR_LEN;

        auto new_ptr = reinterpret_cast<struct fds_ipfix_set_hdr *>(&m_buffer_main[m_pos_set]);
        new_ptr->flowset_id = htons(m_tid_now);
    }

    // Copy the Data Record
    memcpy(&m_buffer_main[m_written], data, size);
    m_written += size;

    // Update the Sequence number and number of Data Records
    ++m_seq_next;
    ++m_rec_cnt;
}

uint64_t
Block_data_writer::write_to_file(int fd, off_t offset, uint16_t sid, uint64_t off_btmplt,
    Io_factory::Type type)
{
    uint64_t result;
    if (m_written <= FDS_FILE_BDATA_HDR_SIZE) {
        // Nothing to do
        return 0;
    }

    // Close the current IPFIX Message and IPFIX Set (i.e. update length fields)
    const uint32_t msg_size = m_written - m_pos_msg;
    const uint32_t set_size = m_written - m_pos_set;
    assert(msg_size <= UINT16_MAX && "Maximum Message size exceeded!");
    assert(set_size <= UINT16_MAX - FDS_IPFIX_MSG_HDR_LEN && "Maximum Set size exceeded!");
    auto msg_ptr = reinterpret_cast<struct fds_ipfix_msg_hdr *>(&m_buffer_main[m_pos_msg]);
    auto set_ptr = reinterpret_cast<struct fds_ipfix_set_hdr *>(&m_buffer_main[m_pos_set]);
    msg_ptr->length = htons(msg_size);
    set_ptr->length = htons(set_size);

    // Update the Data Block header (only previously undefined values)
    auto block_ptr = reinterpret_cast<struct fds_file_bdata *>(m_buffer_main.get());
    block_ptr->hdr.length = htole64(m_written);
    block_ptr->session_id = htole16(sid);
    block_ptr->offset_tmptls = htole64(off_btmplt);

    if (m_calg != FDS_FILE_CALG_NONE) {
        // Compress the Data Block
        size_t comp_size = compress();
        // Update the Data Block header (in the compression buffer) to contain correct block size
        auto block_ptr = reinterpret_cast<struct fds_file_bdata *>(m_buffer_comp.get());
        block_ptr->hdr.length = htole64(comp_size);
        // Store the block
        store(fd, offset, m_buffer_comp, comp_size, type);
        result = comp_size;
    } else {
        // Just store the content of the main buffer to the file
        store(fd, offset, m_buffer_main, m_written, type);
        result = m_written;
    }

    // Initialize Data Block header on the top of the main buffer (and reset position variables)
    reset_buffer();
    return result;
}

/**
 * @brief Compress Data Block in the main buffer and write it to the compression buffer
 *
 * First, the original Data Block header is copied to the compression buffer. Remaining content
 * (with IPFIX Messages) is compressed and appended.
 * @return Size of the compressed Data Block (i.e. valid length of the compression buffer)
 */
size_t
Block_data_writer::compress()
{
    assert(m_written > FDS_FILE_BDATA_HDR_SIZE && "The block must contain useful data");
    assert(m_calg != FDS_FILE_CALG_NONE && "Compression algorithm must be selected");
    assert(m_buffer_comp != nullptr && "Compression buffer must exist");
    size_t ret_val = FDS_FILE_BDATA_HDR_SIZE; // Uncompressed Data Block header

    // First, copy the Data Block header (always uncompressed)
    memcpy(m_buffer_comp.get(), m_buffer_main.get(), FDS_FILE_BDATA_HDR_SIZE);

    // Prepare pointers to the buffers (behind uncompressed Data Block headers)
    uint8_t *ptr_in  = &m_buffer_main[FDS_FILE_BDATA_HDR_SIZE];
    uint8_t *ptr_out = &m_buffer_comp[FDS_FILE_BDATA_HDR_SIZE];
    size_t size_in = m_written - FDS_FILE_BDATA_HDR_SIZE;
    size_t size_out = m_alloc - FDS_FILE_BDATA_HDR_SIZE;

    if (m_calg == FDS_FILE_CALG_LZ4) {
        // LZ4 compression
        auto src_ptr = reinterpret_cast<const char *>(ptr_in);
        auto dst_ptr = reinterpret_cast<char *>(ptr_out);
        int src_size = static_cast<int>(size_in);
        int dst_cap = static_cast<int>(size_out);

        assert(dst_cap >= LZ4_compressBound(src_size) && "Non optimal output buffer size");
        int rc = LZ4_compress_default(src_ptr, dst_ptr, src_size, dst_cap);
        if (rc == 0) {
            throw File_exception(FDS_ERR_INTERNAL, "LZ4 failed to compress a Data Block");
        }
        assert(rc > 0 && "On success the size must be always positive");
        ret_val += static_cast<size_t>(rc);
    } else if (m_calg == FDS_FILE_CALG_ZSTD) {
        // ZSTD compression
        assert(size_out >= ZSTD_compressBound(size_in) && "Non optimal output buffer size");
        size_t rc = ZSTD_compress(ptr_out, size_out, ptr_in, size_in, 1); // Fastest possible level
        if (ZSTD_isError(rc)) {
            const char *err_msg = ZSTD_getErrorName(rc);
            throw File_exception(FDS_ERR_INTERNAL, "ZSTD failed to compress a Data Block ("
                + std::string(err_msg) + ")");
        }
        assert(rc > 0 && "On success the size must be always positive");
        ret_val += rc;
    } else {
        throw File_exception(FDS_ERR_INTERNAL, "Selected compression algorithm is not implemented");
    }

    return ret_val;
}

/**
 * @brief Write a prepared Data Block to a file
 *
 * If there is already a previous asynchronous I/O operation in progress, the function waits
 * until its complete. After return from this function, the user can use the source buffer
 * (@p src_buffer) without any limitation (i.e. it can be overwritten with new content).
 *
 * @note
 *   If asynchronous I/O operation has been started, then source buffer (@p src_buffer) and
 *   internal asynchronous buffer are transparently swapped.
 * @note
 *   Synchronous I/O operations are performed immediately without any delay.
 * @param[in] fd           File descriptor
 * @param[in] offset       Offset in the file where the Data block will be placed
 * @param[in] src_buffer   Source buffer with Data Block to write to the file
 * @param[in] src_size     Size of the content in the source buffer (in bytes)
 * @param[in] io_type      Type of I/O operation to use (synchronous vs. asynchronous)
 */
void
Block_data_writer::store(int fd, off_t offset, std::unique_ptr<uint8_t[]> &src_buffer, size_t src_size,
    Io_factory::Type io_type)
{
    // First, wait for the previous asynchronous I/O operation to complete
    write_wait();

    // Create a new I/O request
    auto new_req = Io_factory::new_request(fd, src_buffer.get(), m_alloc, io_type);
    new_req->write(offset, src_size);

    // In case of synchronous I/O, perform the operation immediately
    if (Io_sync *sync_req = dynamic_cast<Io_sync *>(new_req.get())) {
        size_t res = sync_req->wait();
        if (res != src_size) {
            throw File_exception(FDS_ERR_INTERNAL, "Synchronous write() failed to write a Data Block");
        }

        // Everything is done
        return;
    }

    // Asynchronous I/O only -> store the I/O instance and swap buffers
    if (m_buffer_async == nullptr) {
        // The asynchronous buffer hasn't been allocated yet
        m_buffer_async.reset(new uint8_t[m_alloc]);
    }

    m_async_io = std::move(new_req);
    m_async_size = src_size;
    src_buffer.swap(m_buffer_async); // Swap buffers now
}

/**
 * @brief Initialize FDS Data block header in the main buffer
 *
 * After calling this function, the block is considered as empty i.e. all position variables
 * are reset to point right behind the block header.
 */
void
Block_data_writer::reset_buffer()
{
    // Common message header (length will be filled before writing to a file)
    auto ptr = reinterpret_cast<struct fds_file_bdata *>(m_buffer_main.get());
    ptr->hdr.type = htole16(FDS_FILE_BTYPE_DATA);
    ptr->hdr.flags = htole16(m_calg != FDS_FILE_CALG_NONE ? FDS_FILE_CFLGS_COMP : 0);

    // Data Block header (Session ID + Template Block offset will be filled during writing)
    ptr->odid = htole32(m_odid);
    ptr->flags = htole16(0);

    // Reset position variables
    m_written = FDS_FILE_BDATA_HDR_SIZE;
    m_pos_msg = m_written;
    m_pos_set = m_written;
    m_tid_now = 0;
    m_rec_cnt = 0;
}

// TODO: move to IPFIX parsers...
/**
 * @brief Get real size of a Data Record
 *
 * @param[in]     data  Pointer to the start of the Data Record
 * @param[in,out] size  [in] Maximum record size / [out] Real size (both in bytes)
 * @param[in]     tmplt Corresponding IPFIX (Options) Template
 * @return #IPX_OK on success and the @p size is updated with the real size of the Data Record.
 * @return #IPX_ERR_FORMAT if the Data Record is longer then @p size (i.e. it's probably malformed)
 */
int
Block_data_writer::rec_length(const uint8_t *data, uint16_t *size, const struct fds_template *tmplt)
{
    if ((tmplt->flags & FDS_TEMPLATE_DYNAMIC) == 0) {
        // Processing a static record
        if (tmplt->data_length > *size) {
            return FDS_ERR_FORMAT;
        }
        *size = tmplt->data_length;
        return FDS_OK;
    }

    // Processing a dynamic record
    const uint8_t *rec_end = data + (*size);
    uint32_t real_size = 0;
    uint16_t idx;

    for (idx = 0; idx < tmplt->fields_cnt_total; ++idx) {
        uint16_t field_size = tmplt->fields[idx].length;
        if (field_size != FDS_IPFIX_VAR_IE_LEN) {
            real_size += field_size;
            continue;
        }

        // This is a field with variable-length encoding
        if (&data[real_size + 1] > rec_end) {
            // The memory is beyond the end of the expected end
            break;
        }

        field_size = data[real_size];
        real_size += 1U;
        if (field_size != 255U) {
            real_size += field_size;
            continue;
        }

        if (&data[real_size + 2] > rec_end) {
            // The memory is beyond the end of the Data Set
            break;
        }

        field_size = ntohs(*(uint16_t *) &data[real_size]);
        real_size += 2U + field_size;
    }

    if (idx != tmplt->fields_cnt_total || &data[real_size] > rec_end) {
        // A variable-length Data Record is longer than its enclosing Data Set.
        return FDS_ERR_FORMAT;
    }

    *size = (uint16_t) real_size;
    return FDS_OK;
}

void
Block_data_writer::write_wait()
{
    if (!m_async_io) {
        return;
    }

    size_t async_res = m_async_io->wait();
    if (async_res != m_async_size) {
        throw File_exception(FDS_ERR_INTERNAL, "Asynchronous write() failed to write a Data Block");
    }

    m_async_io.reset();
}
