/**
 * @file   src/file/Block_data_writer.hpp
 * @brief  Data Block writer (header file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   April 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBFDS_BLOCK_DATA_WRITER_HPP
#define LIBFDS_BLOCK_DATA_WRITER_HPP

#include "Io_request.hpp"
#include "structure.h"

namespace fds_file {

/**
 * @brief Data block writer
 *
 * An instance of the class is able to store IPFIX Data Records that belongs to exactly one
 * combination of Transport Session ID and ODID. In other words, for different combination,
 * different instances should be used.
 *
 * The Data Block can contain Data Records based on different IPFIX (Options) Templates. All of
 * these Templates MUST have a unique Template ID. In other words, there MUST NOT be Data Records
 * based on different Templates with the same Template ID.
 */
class Block_data_writer {
public:
    /// Default maximum IPFIX Message size
    static const uint16_t MSG_DEF_SIZE = 1400;

    /**
     * @brief Class constructor
     *
     * @note
     *   The maximum IPFIX Message size (@p msg_size) is ignored if a size of a Data Record
     *   that is added exceeds the maximum size.
     *
     * @param[in] odid     Observation Domain ID (common for all Data Records to be added)
     * @param[in] comp_alg Compression algorithm (used during writing to the file)
     * @param[in] msg_size Maximum IPFIX message size
     */
    Block_data_writer(uint32_t odid, enum fds_file_alg comp_alg, uint16_t msg_size = MSG_DEF_SIZE);
    /**
     * @brief Class destructor
     *
     * If asynchronous operation is in progress, it blocks until it's complete.
     */
    ~Block_data_writer();

    // Copy constructors are disabled, by default
    Block_data_writer(const Block_data_writer &other) = delete;
    Block_data_writer &operator=(const Block_data_writer &other) = delete;

    /**
     * @brief Write all added IPFIX Data Records as Data block to a file
     *
     * If compression is enabled, the records are compressed first. Then the (potentially
     * compressed) Data block is written to the file using synchronous or asynchronous I/O.
     *
     * @note
     *   If the previous asynchronous write is still in progress, the function waits for the
     *   operation to complete before the next operation is started.
     * @warning
     *   The content (i.e. the added Data Records) is automatically cleared after the records are
     *   written and the buffer can be used to store new Data Records. The current Export Time
     *   is preserved.
     * @param[in] fd         File descriptor (must be opened for writing)
     * @param[in] offset     Offset in the file where the Data block will be placed
     * @param[in] sid        Internal Transport Session ID
     * @param[in] off_btmplt Offset of the Template block in the file used to decode this Data block
     *   (MUST be placed before this block in the file!)
     * @param[in] type       Type of I/O operation used for writing (sync./async./default)
     * @return Size of the written block (in bytes, can be 0 i.e. no records = nothing to write).
     * @throw File_exception if compression or writing operation fails
     */
    uint64_t
    write_to_file(int fd, off_t offset, uint16_t sid, uint64_t off_btmplt,
        Io_factory::Type type = Io_factory::Type::IO_DEFAULT);

    /**
     * @brief Set Export Time
     *
     * All records added using add() will be be part of IPFIX Messages with the given Export Time
     * specified in the Message header. By default, zero is used as Export Time.
     *
     * @param[in] time Export time
     */
    void
    set_etime(uint32_t time) {m_etime_set = time;};

    /**
     * @brief Add a Data Record
     *
     * First, the function checks if the @p size matches the size of Data Records based on
     * the @p tmplt. Then the record is stored into the internal buffer.
     *
     * @note
     *   Before writing the Data Records, a user should always check that there is enough
     *   remaining space in the buffer using remains(). Otherwise an exception is thrown.
     * @warning
     *   The class doesn't check Template ID collisions! Therefore, the user MUST make sure
     *   that different IPFIX (Options) Templates with the same Template ID are NOT used in
     *   the same Data Block. Different IPFIX (Options) Template definition can be used only
     *   after write_to_file(), because content of the buffer is cleared.
     *
     *
     * @param[in] data  Start of the Data Record to add
     * @param[in] size  Size of the Data Record
     * @param[in] tmplt Parse IPFIX (Options) Template that describes the Data Record
     * @throw File_exception if the Data Record doesn't match the IPFIX (Options) Template or
     *   if the internal buffer is full.
     */
    void
    add(const uint8_t *data, uint16_t size, const struct fds_template *tmplt);

    /**
     * @brief Get number of IPFIX Data Records in the buffer
     *
     * The counter is automatically reset when the buffer is written to a file.
     * @return Number of Data Records
     */
    uint32_t
    count() {return m_rec_cnt;};

    /**
     * @brief Remaining size of the internal buffer
     *
     * Returned value represents the maximum size of a Data Record that can fit into the buffer.
     * The function calculates the worst possible case when a new IPFIX Message must be created
     * (for example, due to exceeding of maximum IPFIX Message size or change of the Export Time)
     * with a new IPFIX Set header.
     *
     * @return Number of bytes
     */
    inline uint32_t
    remains() {
        assert(m_capacity >= m_written && "buffer overflow!");
        uint32_t required = m_written + FDS_IPFIX_MSG_HDR_LEN + FDS_IPFIX_SET_HDR_LEN;
        return (m_capacity > required) ? (m_capacity - required) : 0;
    }

    /**
     * @brief Wait for the current I/O operation to complete
     *
     * The function is useful only when the asynchronous I/O mode is selected. Otherwise, it has
     * not effect at all. If not previous I/O operation is running, returns immediately.
     * @note
     *    It does not make any guarantee that data has been committed to disk.
     * @throw File_exception if the Data Block is not written
     */
    void
    write_wait();

private:
    /// Capacity of the Output buffer (raw uncompressed data without Data Block header)
    static constexpr uint32_t m_capacity = FDS_FILE_DBLOCK_SIZE;

    /// Observation Domain ID of IPFIX Messages
    uint32_t m_odid;
    /// Selected compression algorithm
    enum fds_file_alg m_calg;
    /// Maximum size of an IPFIX Message
    uint16_t m_size_max;

    /// Allocated size of the buffer(s)
    uint32_t m_alloc;
    /// Total number of written bytes into the main buffer
    uint32_t m_written = 0;
    /// Total number of Data Records in the unwritten buffer
    uint32_t m_rec_cnt = 0;

    /// Main buffer used for adding new Data Records
    std::unique_ptr<uint8_t[]> m_buffer_main = nullptr;
    /// Compression buffer (i.e. compressed version of the Data block for writing)
    std::unique_ptr<uint8_t[]> m_buffer_comp = nullptr;
    /// Buffer for asynchronous write operations (cannot be changed when I/O is in progress)
    std::unique_ptr<uint8_t[]> m_buffer_async = nullptr;

    /// Asynchronous write I/O request
    std::unique_ptr<Io_request> m_async_io = nullptr;
    /// Size of asynchronously requested block (valid only if m_async_io is not nullptr)
    size_t m_async_size;

    /// The selected export time (of the next Data Record)
    uint32_t m_etime_set = 0;
    /// The current export time (of the IPFIX Message that is being written)
    uint32_t m_etime_now = 0;
    /// Position of the current IPFIX Message header that is being written in the buffer
    uint32_t m_pos_msg = 0;
    /// Position of the current IPFIX Set header that is being written in the buffer
    uint32_t m_pos_set = 0;
    /// Sequence number of the next IPFIX Message
    uint32_t m_seq_next = 0;
    /// Template ID of the current IPFIX Data Set
    uint16_t m_tid_now = 0;

    // Calculate real length of an IPFIX Data Record
    int
    rec_length(const uint8_t *data, uint16_t *size, const struct fds_template *tmplt);
    // Reset content of the main buffer
    void
    reset_buffer();
    // Compress a content of the main buffer to the compressions buffer
    size_t
    compress();
    // Store a content of a buffer to a file
    void
    store(int fd, off_t offset, std::unique_ptr<uint8_t[]> &src_buffer, size_t src_size,
        Io_factory::Type io_type = Io_factory::Type::IO_DEFAULT);
};

} // namespace

#endif //LIBFDS_BLOCK_DATA_WRITER_HPP
