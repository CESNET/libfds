/**
 * @file   src/file/Block_data_reader.hpp
 * @brief  Data Block reader (header file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   April 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBFDS_BLOCK_DATA_READER_HPP
#define LIBFDS_BLOCK_DATA_READER_HPP

#include <libfds/file.h>
#include <memory>

#include "structure.h"
#include "Io_request.hpp"
#include "Block_templates.hpp"

namespace fds_file {

/**
 * @brief The Data Block reader
 *
 * An instance of the class is able to load a Data Block from a file and iterate over the stored
 * IPFIX Data Records. IPFIX (Options) Template provided by a appropriate Template Block are
 * necessary for record decoding. Keep on mind that all Data Records in one Data Block
 * belong to exactly one combination of Transport Session ID and ODID.
 *
 * To provide optimal condition for asynchronous I/O, the instance also loads the Common Block
 * header of the following block in the order, so a user can determined its type and size to
 * initialize asynchronous read with sufficient time reserve.
 *
 */
class Block_data_reader {
public:
    /**
     * @brief Class constructor
     * @param[in] comp_alg Compression algorithm
     */
    Block_data_reader(enum fds_file_alg comp_alg);
    /**
     * @brief Class destructor
     * @note If the async. I/O is in progress, the destructor waits until it's complete or canceled.
     */
    ~Block_data_reader();

    // Disable copy constructors
    Block_data_reader(const Block_data_reader &other) = delete;
    Block_data_reader &operator=(const Block_data_reader &other) = delete;

    /**
     * @brief Load a Data Block from a file
     *
     * The Data Block will be loaded and decompressed (if necessary) into an internal buffer.
     * If the hint (@p size_hint) is not defied (i.e. the value is zero), synchronous I/O is
     * immediately used to determine real size of the block from its header.
     *
     * If synchronous I/O operation is selected, the content of the Data Block will be loaded
     * when the content is first accessed. In other words, synchronous load operation uses lazy
     * evaluation strategy.
     *
     * If asynchronous I/O operation is selected, background load is immediately started. When the
     * content of the block is first accessed, it might block until the I/O is complete.
     *
     * @warning
     *   If the hint size is specified but doesn't match the real size of the block, load will fail.
     *
     * @param[in] fd        File descriptor (must be opened for reading)
     * @param[in] offset    Offset in the file where the start of the Data Block is placed
     * @param[in] size_hint Size of the Data Block to load (if unknown, set to 0)
     * @param[in] type      Type of I/O operation used for reading (sync./async./default)
     */
    void
    load_from_file(int fd, off_t offset, size_t size_hint = 0,
        Io_factory::Type type = Io_factory::Type::IO_DEFAULT);

    /**
     * @brief Get the header of the loaded Data Block
     *
     * The structure can be used to determine Transport Session ID, ODID and the offset of the
     * Template block before start of Data Record parsing.
     * @warning
     *   All values of the returned structure are in little-endian. Therefore, proper conversion
     *   functions MUST be used!
     * @return Pointer to the header
     */
    const fds_file_bdata *
    get_block_header();

    /**
     * @brief Set a Template Snapshot for decoding IPFIX Data Records
     *
     * Templates from the snapshot are used to determined structure and size of IPFIX Data Records.
     * Without appropriate IPFIX (Options) Templates corresponding IPFIX Data Records cannot be
     * decoded.
     *
     * @note
     *   After change of the snapshot, rewind() is automatically called.
     * @warning
     *   The snapshot MUST exists as long as this Data Reader instance.
     * @param[in] snap Template snapshot
     */
    void
    set_templates(const fds_tsnapshot_t *snap);

    /**
     * @brief Set position indicators to the beginning of the Data Block
     *
     * It will also cause that the next call of next_rec() will return the first IPFIX Data Record
     * in the buffer.
     */
    void
    rewind();

    /**
     * @brief Get the next Data Record in the loaded Data Block
     *
     * The function will parse the next IPFIX Data Record from the buffer and update data
     * structures about to the record and its context.
     *
     * @warning
     *   The Template manager MUST be configured before the first use of this function using
     *   set_tmanager(). Otherwise, the Data Reader is not able to parse any IPFIX Data Record
     *   in the block.
     *
     * @param[out] rec  Data Record (pointer to the Data Record, IPFIX Template, etc.)
     * @param[out] ctx  Data Record context i.e. Transport Session, ODID, Export Time (can be NULL)
     * @return #FDS_OK on success and the @p rec and @p ctx are filled.
     * @return #FDS_EOC if there are no more IPFIX Data Records.
     * @throw File_exception if no Data Block has been previously loaded, a Template manager
     *   is not configured or the Block is internally malformed.
     */
    int
    next_rec(struct fds_drec *rec, struct fds_file_read_ctx *ctx);

    /**
     * @brief Get the Common block header placed right after the current Data Block
     *
     * The main purpose of this function is to provide necessary information for start of the
     * next asynchronous I/O operation. If there are NO more blocks behind the currently
     * loaded Data Block (i.e. the end-of-file has been reached), nullptr will be returned.
     *
     * @warning
     *   All values of the returned structure are in little-endian. Therefore, proper conversion
     *   functions MUST be used!
     * @return Pointer to the next header or nullptr
     */
    const struct fds_file_bhdr *
    next_block_hdr();

private:
    /// Capacity of the buffers (raw uncompressed data without Data Block header)
    static constexpr uint32_t m_capacity = FDS_FILE_DBLOCK_SIZE;
    /// Selected decompression algorithm
    enum fds_file_alg m_calg;
    /// Pointer to the Template snapshot (common for all Data Records in this Data Block)
    const fds_tsnapshot_t *m_tsnap = nullptr;
    /// Allocated size of the internal buffers
    size_t m_alloc;

    /// Context of the current IPFIX message (i.e. Transport Session, ODID, Export Time)
    struct fds_file_read_ctx m_ctx;
    /// Number of bytes that be been read from a file (i.e. valid size of the main buffer)
    size_t m_read = 0;
    /// Buffer with loaded (uncompressed) Data Block
    std::unique_ptr<uint8_t[]> m_buffer_main = nullptr;
    /// Auxiliary buffer for decompression
    std::unique_ptr<uint8_t[]> m_buffer_aux = nullptr;

    /// Pointer to the next IPFIX Message in the order
    uint8_t *m_msg_next = nullptr;
    /// The byte after the end of the main (uncompressed) buffer
    const uint8_t *m_msg_end = nullptr;

    /// Indicator of iterator initialization
    bool m_iters_ready = false;
    /// IPFIX Sets iterator in the current IPFIX Message
    struct fds_sets_iter m_iter_sets;
    /// IPFIX Data Set iterator in the current IPFIX Data Set
    struct fds_dset_iter m_iter_dset;
    /// Pointer to the IPFIX (Options) Template used in the current IPFIX Data Set
    const struct fds_template *m_iter_tmplt = nullptr;


    /// Synchronous/asynchronous read I/O request
    std::unique_ptr<Io_request> m_io_request = nullptr;
    /// Size of the requested block (valid only if m_io_request is not nullptr)
    size_t m_io_size;

    /// Common Block header of the following block
    struct fds_file_bhdr m_next_hdr;
    /// Status of the following Common Block header
    bool m_next_hdr_valid = false;

    // Check if Data Block is ready (i.e. I/O request is complete) or wait for data to be prepared
    inline void
    data_ready();
    // Wait for I/O to complete and prepare it for parsing
    void
    data_loader();
    // Perform decompression of a Data Block in the main buffer
    void
    decompress();

    // Prepare the next IPFIX Data Record in the current IPFIX Data Set
    inline int
    prepare_record(struct fds_drec *rec);
    // Prepare the next IPFIX Data Set in the current IPFIX Message
    inline int
    prepare_set();
    // Prepare the next IPFIX Message in the buffer
    inline int
    prepare_message();
};

} // namespace

#endif //LIBFDS_BLOCK_DATA_READER_HPP
