/**
 * @file   src/file/Block_content.hpp
 * @brief  Content table (header file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   May 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBFDS_BLOCK_CONTENT_HPP
#define LIBFDS_BLOCK_CONTENT_HPP

#include <vector>
#include <cstdint>
#include <stdio.h>

namespace fds_file {

/**
 * @brief Content table
 *
 * The content table is able to hold meta information about selected blocks of FDS file.
 * It provides simple way how to easily locate their start position and determine their size
 * for faster access time. All occurrences of supported block types MUST be present in the
 * Content table and sorted by the offset in ascending order.
 *
 * The Content Table block is ALWAYS places as the last block in the file, so it can be
 * overwritten when the file is opened for appending.
 */
class Block_content {
public:
    /// Class constructor
    Block_content() = default;
    /// Class destructor
    ~Block_content() = default;

    // Disable copy constructors
    Block_content(const Block_content &other) = delete;
    Block_content &operator=(const Block_content &other) = delete;

    /// Information about a Transport Session block
    struct info_session {
        uint64_t offset;        ///< Offset of the block from the start of the file
        uint64_t len;           ///< Length of the block
        uint16_t session_id;    ///< Internal Transport Session ID
    };

    /// Information about a Data block
    struct info_data_block {
        uint64_t offset;        ///< Offset of the block from the start of the file
        uint64_t len;           ///< Length of the block
        uint64_t tmplt_offset;  ///< Offset of the Template block used to interpret Data Records
        uint32_t odid;          ///< Observation Domain ID
        uint16_t session_id;    ///< Internal Transport Session ID
    };

    /**
     * @brief Load Content Table from a file
     *
     * @note
     *   To preserve future backwards compatibility If the content table in the file contains
     *   newer record types, these records are silently ignored.
     * @warning
     *   All information stored in the object will be replaced or removed.
     * @param[in] fd File descriptor (must be opened for reading)
     * @param[in] offset Offset in the file where the start of the Template Block is placed
     * @return Size of the block (in bytes)
     * @throw File_exception if the loading operation fails and the object is in an undefined state
     */
    uint64_t
    load_from_file(int fd, off_t offset);

    /**
     * @brief Write Content Table to a file
     * @param[in] fd     File descriptor (must be opened for writing)
     * @param[in] offset Offset in the file where the start of the Content Table will be placed
     * @return Size of the written block (in bytes)
     * @throw File_exception if the writing operation fails
     */
    uint64_t
    write_to_file(int fd, off_t offset);

    /**
     * @brief Remove all records from the Content Table
     */
    void
    clear();

    /**
     * @brief Add information about a new Transport Session Block
     * @param[in] offset Offset of the block
     * @param[in] len    Length of the block
     * @param[in] sid    Internal Transport Session ID
     */
    void
    add_session(uint64_t offset, uint64_t len, uint16_t sid);

    /**
     * @brief Add information about a new Data Block
     * @param[in] offset       Offset of the block
     * @param[in] len          Length of the block
     * @param[in] tmplt_offset Offset of the Template Block used to interpret Data Records
     * @param[in] odid         Observation Domain ID of Data Records in the block
     * @param[in] sid          Internal Transport Session ID
     */
    void
    add_data_block(uint64_t offset, uint64_t len, uint64_t tmplt_offset, uint32_t odid, uint16_t sid);

    /**
     * @brief Get list of all Transport Session positions
     * @return List
     */
    const std::vector<struct info_session> &
    get_sessions() const {return m_sessions;};

    /**
     * @brief Get list of all Data Blocks in the file
     * @return List
     */
    const std::vector<struct info_data_block> &
    get_data_blocks() const {return m_dblocks;};

private:
    /// List of all Transport Sessions
    std::vector<struct info_session> m_sessions;
    /// List of all Data Blocks
    std::vector<struct info_data_block> m_dblocks;

    size_t
    write_sessions(int fd, off_t offset);
    size_t
    write_data_blocks(int fd, off_t offset);

    size_t
    read_sessions(const uint8_t *bdata, size_t bsize, uint64_t rel_offset);
    size_t
    read_data_blocks(const uint8_t *bdata, size_t bsize, uint64_t rel_offset);
};

} // namespace

#endif //LIBFDS_BLOCK_CONTENT_HPP
