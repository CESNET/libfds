/**
 * @file   src/file/Block_data_reader.cpp
 * @brief  Data Block reader (source file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   April 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <libfds.h>
#include <fds_lz4.h>
#include <fds_zstd.h>

#include "Block_data_reader.hpp"
#include "File_exception.hpp"
#include "Io_sync.hpp"

using namespace fds_file;

Block_data_reader::Block_data_reader(enum fds_file_alg comp_alg)
    : m_calg(comp_alg)
{
    // Determine size buffers (able to hold Data Block + following Common Header block)
    size_t alloc_size = FDS_FILE_BDATA_HDR_SIZE + FDS_FILE_BHDR_SIZE;

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
        m_buffer_aux.reset(new uint8_t[m_alloc]);
    }
}

Block_data_reader::~Block_data_reader()
{
    if (!m_io_request) {
        return;
    }

    m_io_request->cancel();
    m_io_request.reset();

    // Buffers can be now automatically destroyed
}

void
Block_data_reader::load_from_file(int fd, off_t offset, size_t size_hint, Io_factory::Type type)
{
    if (size_hint > m_alloc) {
        throw File_exception(FDS_ERR_INTERNAL, "Invalid hint size of a Data Block to read");
    }

    size_t size2load = FDS_FILE_BHDR_SIZE; // The following Common Block header size
    if (size_hint == 0) {
        // Determine the real size of the block
        struct fds_file_bdata hdr;
        const size_t hdr_size = sizeof(hdr);
        Io_sync io_req(fd, reinterpret_cast<void *>(&hdr), hdr_size);
        io_req.read(offset, hdr_size);
        if (io_req.wait() != hdr_size) {
            throw File_exception(FDS_ERR_INTERNAL, "Failed to load the Data Block header");
        }

        if (le16toh(hdr.hdr.type) != FDS_FILE_BTYPE_DATA) {
            throw File_exception(FDS_ERR_INTERNAL, "The Data Block type doesn't match");
        }

        size2load += le64toh(hdr.hdr.length);
    } else {
        // Use the user provided hint
        size2load += size_hint;
    }

    if (size2load > m_alloc) {
        throw File_exception(FDS_ERR_INTERNAL, "The Data Block to load exceed maximum allowed size");
    }

    // Make sure that any previous I/O is not running (the buffer cannot be used by multiple I/Os)
    m_io_request.reset();

    // Create a new I/O request and initialize read
    auto new_io = Io_factory::new_request(fd, m_buffer_main.get(), m_alloc, type);
    new_io->read(offset, size2load);

    m_io_request = std::move(new_io);
    m_io_size = size2load;
    m_read = 0; // Nothing is ready yet
}

void
Block_data_reader::set_templates(const fds_tsnapshot_t *snap)
{
    assert(snap != nullptr && "Snapshot cannot be nullptr");
    m_tsnap = snap;
    rewind();
}

const struct fds_file_bhdr *
Block_data_reader::next_block_hdr()
{
    // Make sure that the Data Block is ready
    data_ready();

    if (m_next_hdr_valid) {
        return &m_next_hdr;
    }

    return nullptr;
}

const fds_file_bdata *
Block_data_reader::get_block_header()
{
    // Make sure that the Data Block is ready
    data_ready();

    assert(m_read >= FDS_FILE_BDATA_HDR_SIZE && "At least the header must be available");
    return reinterpret_cast<struct fds_file_bdata *>(m_buffer_main.get());
}

void
Block_data_reader::rewind()
{
    if (m_read == 0) {
        // Nothing to do
        return;
    }

    assert(m_read >= FDS_FILE_BDATA_HDR_SIZE && "Data Block must contain at least header!");

    m_iters_ready = false;
    // The first message after the Data Block header
    m_msg_next = &m_buffer_main[FDS_FILE_BDATA_HDR_SIZE];
    // The next byte after the end of the main (uncompressed) buffer
    m_msg_end = &m_buffer_main[m_read];
}

int
Block_data_reader::next_rec(struct fds_drec *rec, struct fds_file_read_ctx *ctx)
{
    // Make sure that the Data Block is ready
    data_ready();

    // Template manager MUST be defined!
    if (m_tsnap == nullptr) {
        throw File_exception(FDS_ERR_INTERNAL, "Unable to decode Data Block due to an undefined "
            "Template snapshot");
    }

    while (true) {
        // Return the next Data Record from the current IPFIX Data Set
        if (m_iters_ready && prepare_record(rec) == FDS_OK) {
            // The record is ready
            if (ctx) {
                *ctx = m_ctx;
            }
            return FDS_OK;
        }

        while (true) {
            // No more Data Records in the current Data Set -> try the next one
            if (m_iters_ready && prepare_set() == FDS_OK) {
                // Try to prepare the next Data Record again...
                break;
            }

            // No more IPFIX Sets in the current IPFIX Message -> try to load the next IPFIX Message
            if (prepare_message() == FDS_OK) {
                // Try to prepare the next IPFIX Data Set
                continue;
            }

            // No more IPFIX Messages
            return FDS_EOC;
        }

        // Do NOT add anything here!
    }
}

/**
 * @brief Make sure that Data Block is loaded
 *
 * The function checks if there is a pending I/O request and waits until it's complete and
 * the Data Block is prepared to be parsed. If there is no I/O request and the buffer is empty
 * (i.e. load_from_file() hasn't been called yet), it throws an exceptions.
 */
inline void
Block_data_reader::data_ready()
{
    if (m_io_request != nullptr) {
        // Wait for Data Block to be loaded and prepare it
        data_loader();
    }

    if (m_read == 0) {
        throw File_exception(FDS_ERR_INTERNAL, "No Data Block is loaded");
    }
}

/**
 * @brief Wait for an I/O request to complete and process the Data Block
 *
 * The type and size of the loaded Data Block is checked. If the content is compressed,
 * it will be decompressed so it can be easily processed. The Common Block header of the following
 * block is also extracted (if it is available at all).
 *
 * @note
 *   The function resets position pointers.
 */
void
Block_data_reader::data_loader()
{
    assert(m_io_request != nullptr && "I/O request must exist");
    assert(m_io_size >= FDS_FILE_BDATA_HDR_SIZE && "At least Data Block header was requested");

    /*
     * Wait for I/O to complete
     * Returned size should be equal to the loaded Data Block and can contain also size of the
     * next Common Block header. However, if this is the last Data Block in the file, the next
     * Common Block header might be missing. Different size than mentioned means that something
     * is not loaded properly.
     */
    size_t ret_size = m_io_request->wait();
    m_io_request.reset(); // Remove the request

    if (ret_size != m_io_size && ret_size != m_io_size - FDS_FILE_BHDR_SIZE) {
        throw File_exception(FDS_ERR_INTERNAL, "read() failed to load a Data Block");
    }

    // Check the type and size of the loaded block
    auto hdr_ptr = reinterpret_cast<struct fds_file_bdata *>(m_buffer_main.get());
    if (le16toh(hdr_ptr->hdr.type) != FDS_FILE_BTYPE_DATA) {
        throw File_exception(FDS_ERR_INTERNAL, "The Data Block type doesn't match");
    }

    uint64_t real_size = le64toh(hdr_ptr->hdr.length);
    if (ret_size < real_size) {
        throw File_exception(FDS_ERR_INTERNAL, "The Data Block is not loaded properly");
    }

    // Extract the next Common Block header (if it was loaded at all)
    if (real_size + FDS_FILE_BHDR_SIZE == ret_size) {
        // The next header was loaded
        memcpy(&m_next_hdr, &m_buffer_main[real_size], FDS_FILE_BHDR_SIZE);
        m_next_hdr_valid = true;
    } else if (real_size == ret_size) {
        // The next header is not available (probably end of the file)
        m_next_hdr_valid = false;
    } else {
        throw File_exception(FDS_ERR_INTERNAL, "The Data Block is not loaded properly (probably"
            "invalid size hint)");
    }

    // For further processing, only the Data Block (without the next header) is considered as valid
    m_read = real_size;

    // Decompress if necessary
    uint16_t flags = le16toh(hdr_ptr->hdr.flags);
    if ((flags & FDS_FILE_CFLGS_COMP) != 0) {
        // Decompress to auxiliary buffer and swap it with the main one
        if (m_calg == FDS_FILE_CALG_NONE) {
            throw File_exception(FDS_ERR_INTERNAL, "Data Block is compressed but decompression "
            "algorithm is not selected");
        }

        decompress();

        // Remove the compression flag (note: the buffers have been swapped = we need a new pointer)
        hdr_ptr = reinterpret_cast<struct fds_file_bdata *>(m_buffer_main.get());
        hdr_ptr->flags = htole16(flags &= ~FDS_FILE_CFLGS_COMP);
    }

    // Update context of the Data Records
    m_ctx.sid = le16toh(hdr_ptr->session_id);
    m_ctx.odid = le32toh(hdr_ptr->odid);
    m_ctx.exp_time = 0;

    // Reset position pointers to the beginning of the buffer
    rewind();
}

/**
 * @brief Decompress Data Block in the main buffer
 *
 * The Data Block in the main buffer is decompressed and replaced. Its size is also updated to
 * reflect the change.
 *
 * @note
 *   For better performance, the decompressed version of the Data Block is placed into the
 *   internal auxiliary buffer and then it's swapped with the main buffer.
 */
void
Block_data_reader::decompress()
{
    assert(m_read >= FDS_FILE_BDATA_HDR_SIZE && "The main buffer must not be empty");
    assert(m_calg != FDS_FILE_CALG_NONE && "Compression algorithm must be selected");
    assert(m_buffer_aux != nullptr && "Decompression buffer must exist");
    size_t ret_val = FDS_FILE_BDATA_HDR_SIZE; // Uncompressed Data Block header

    // First, copy the Data Block header (always uncompressed)
    memcpy(m_buffer_aux.get(), m_buffer_main.get(), FDS_FILE_BDATA_HDR_SIZE);

    // Prepare pointers to the buffers (behind the uncompressed Data Block headers)
    uint8_t *ptr_in = &m_buffer_main[FDS_FILE_BDATA_HDR_SIZE];
    uint8_t *ptr_out = &m_buffer_aux[FDS_FILE_BDATA_HDR_SIZE];
    size_t size_in = m_read - FDS_FILE_BDATA_HDR_SIZE;
    size_t size_out = m_alloc - FDS_FILE_BDATA_HDR_SIZE;

    if (m_calg == FDS_FILE_CALG_LZ4) {
        // LZ4 decompression
        auto src_ptr = reinterpret_cast<const char *>(ptr_in);
        auto dst_ptr = reinterpret_cast<char *>(ptr_out);
        int src_size = static_cast<int>(size_in);
        int dst_cap = static_cast<int>(size_out);

        int rc = LZ4_decompress_safe(src_ptr, dst_ptr, src_size, dst_cap);
        if (rc < 0) {
            throw File_exception(FDS_ERR_INTERNAL, "LZ4 failed to decompress a Data Block");
        }
        ret_val += static_cast<size_t>(rc);
    } else if (m_calg == FDS_FILE_CALG_ZSTD) {
        // ZSTD decompression
        size_t rc = ZSTD_decompress(ptr_out, size_out, ptr_in, size_in);
        if (ZSTD_isError(rc)) {
            const char *err_msg = ZSTD_getErrorName(rc);
            throw File_exception(FDS_ERR_INTERNAL, "ZSTD failed to decompress a Data Block ("
                + std::string(err_msg) + ")");
        }
        ret_val += rc;
    } else {
        throw File_exception(FDS_ERR_INTERNAL, "Selected compression algorithm is not implemented");
    }

    m_buffer_main.swap(m_buffer_aux); // Swap buffers
    m_read = ret_val;
}

/**
 * @brief Prepare the next IPFIX Data Record in the current IPFIX Data Set
 *
 * In the current IPFIX Data Set (prepared by prepare_set()) extract description of the next
 * Data Record in the order.
 *
 * @warning
 *   The IPFIX Data Set iterator must be prepared by previous call of prepare_set().
 *
 * @param[out] rec Description of the extracted Data Record
 * @return #FDS_OK on success
 * @return #FDS_EOC if no more IPFIX Data Records are available in the current IPFIX Data Set
 * @throw File_exception if the Data Record is malformed
 */
inline int
Block_data_reader::prepare_record(struct fds_drec *rec)
{
    assert(m_iters_ready && "Iterators must be initialized!");
    assert(m_iter_tmplt != nullptr && "IPFIX (Options) Template cannot be nullptr");
    assert(m_tsnap != nullptr && "Template snapshot must be defined");

    switch(fds_dset_iter_next(&m_iter_dset)) {
    case FDS_OK:
        // The pointers are prepared
        break;
    case FDS_EOC:
        // No more Data Records in the Data Set
        return FDS_EOC;
    case FDS_ERR_FORMAT: {
        // Something is wrong with current IPFIX Message/Set
        const char *err = fds_dset_iter_err(&m_iter_dset);
        throw File_exception(FDS_ERR_INTERNAL, std::string("Malformed Data Set (") + err + ")");
        }
    default:
        throw File_exception(FDS_ERR_INTERNAL, "fds_dset_iter_next() returned unexpected code");
    }

    // Fill the record parameters
    rec->data = m_iter_dset.rec;
    rec->size = m_iter_dset.size;
    rec->tmplt = m_iter_tmplt;
    rec->snap = m_tsnap;

    return FDS_OK;
}

/**
 * @brief Prepare the next IPFIX Data Set in the current IPFIX Message
 *
 * In the current IPFIX Message (prepared by prepare_message()) prepare the next Data Set
 * in the order and initialize IPFIX Data Set iterator (used by prepare_record()).
 *
 * @warning
 *   The IPFIX Data Sets iterator must be prepared by previous call of prepare_message().
 *
 * @return #FDS_OK on success
 * @return #FDS_EOC if no more Data Sets are available in the current IPFIX Message
 * @throw File_exception if the Data Set is malformed
 */
inline int
Block_data_reader::prepare_set()
{
    assert(m_iters_ready && "Iterators must be initialized!");
    struct fds_ipfix_set_hdr *set;
    uint16_t tid;

    while (true) {
        switch (fds_sets_iter_next(&m_iter_sets)) {
        case FDS_OK:
            // The next Data Set is ready
            break;
        case FDS_EOC:
            // No more IPFIX Sets
            return FDS_EOC;
        case FDS_ERR_FORMAT: {
            const char *err = fds_sets_iter_err(&m_iter_sets);
            throw File_exception(FDS_ERR_INTERNAL, std::string("Malformed IPFIX Message (") + err + ")");
        }
        default:
            throw File_exception(FDS_ERR_INTERNAL, "fds_sets_iter_next() returned unexpected code");
        }

        // Check the type of the IPFIX Set
        set = m_iter_sets.set;
        tid = ntohs(set->flowset_id);

        if (tid < FDS_IPFIX_SET_MIN_DSET) {
            /* Skip non-IPFIX Data Set in the IPFIX Message (for the future backwards compatibility)
             * Moreover, this will also skip IPFIX (Options) Templates Set in we are not interested
             * in, because (Options) Templates are defined in special FDS File Template Blocks.
             */
            continue;
        }

        break;
    }

    // Prepare the Data Set iterator
    auto tmplt_ptr = fds_tsnapshot_template_get(m_tsnap, tid);
    if (!tmplt_ptr) {
        // The IPFIX (Options) Template is missing
        const std::string tid_str = std::to_string(tid);
        throw File_exception(FDS_ERR_INTERNAL, "IPFIX (Options) Template (ID: " + tid_str + ") is "
            "not defined");
    }

    m_iter_tmplt = tmplt_ptr;
    fds_dset_iter_init(&m_iter_dset, set, tmplt_ptr);
    return FDS_OK;
}

/**
 * @brief Prepare the next IPFIX Message in the order
 *
 * Check if there is an unprocessed IPFIX Message and its header is valid. If so, initialize
 * the iterator of IPFIX Data Sets (used by prepare_set()) and update Data Record context.
 *
 * @return #FDS_OK on success (the iterator is prepared)
 * @return #FDS_EOC if no more IPFIX Message are available in the buffer
 * @throw File_exception if the next IPFIX Message is malformed
 */
int
Block_data_reader::prepare_message()
{
    assert(m_msg_next != nullptr && "The next IPFIX Message pointer must be defined");
    assert(m_msg_next <= m_msg_end && "Buffer overflow");

    if (m_msg_next == m_msg_end) {
        // No more IPFIX Messages
        return FDS_EOC;
    }

    // Read values from the IPFIX Message header
    if (m_msg_next + FDS_IPFIX_MSG_HDR_LEN > m_msg_end) {
        throw File_exception(FDS_ERR_INTERNAL, "Unexpected end of a Data Block");
    }

    auto msg_hdr = reinterpret_cast<struct fds_ipfix_msg_hdr *>(m_msg_next);
    if (ntohs(msg_hdr->version) != FDS_IPFIX_VERSION) {
        throw File_exception(FDS_ERR_INTERNAL, "Failed to locate the IPFIX Message header");
    }

    uint16_t msg_size = ntohs(msg_hdr->length);
    if (m_msg_next + msg_size > m_msg_end) {
        throw File_exception(FDS_ERR_INTERNAL, "Unexpected end of a Data Block");
    }

    if (msg_size == 0) {
        throw File_exception(FDS_ERR_INTERNAL, "Invalid zero-length IPFIX Message found");
    }

    // Update Data Record Context
    assert(m_ctx.odid == ntohl(msg_hdr->odid) && "The ODID must match the ODID of the Data Block");
    m_ctx.exp_time = ntohl(msg_hdr->export_time);

    // Note: Should we also check sequence numbers? It's probably not necessary...

    // Prepare pointer to the next message
    m_msg_next += msg_size;

    // Initialize Sets iterator
    fds_sets_iter_init(&m_iter_sets, msg_hdr);
    m_iters_ready = true;

    return 0;
}
