/**
 * @file   src/file/Block_session.hpp
 * @brief  Session identification (header file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   May 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBFDS_BLOCK_SESSION_HPP
#define LIBFDS_BLOCK_SESSION_HPP

#include <libfds/file.h>

namespace fds_file {

/**
 * @brief Transport Session
 *
 * The structure handles description about a Transport Session and its internal Session ID.
 * The description of the session can be stored or loaded from a file.
 */
class Block_session {
public:
    /**
     * @brief Create a Transport Session description
     * @param[in] sid     Internal session ID (should be unique for each Session)
     * @param[in] session Description of a Transport Session
     * @throw File_exception if the structure contains invalid parameters
     */
    Block_session(uint16_t sid, const struct fds_file_session *session);
    /**
     * @brief Load Session description from a file
     *
     * The block is loaded from the file using synchronous I/O and parsed to internal data
     * structure.
     *
     * @param[in] fd     File descriptor (must be opened for reading)
     * @param[in] offset Offset in the file where the start of the Data Block is placed
     * @throw File_exception if loading process fails
     */
    Block_session(int fd, off_t offset);
    /**
     * @brief Destructor
     */
    ~Block_session() = default;

    /**
     * @brief Load Transport Session stored as a Session Block from a file
     *
     * @warning
     *   Currently loaded parameters will be overwritten.
     * @param[in]  fd     File descriptor (must be opened for reading)
     * @param[in]  offset Offset in the file where the start of the Session Block is placed
     * @return Size of the block (in bytes)
     * @throw File_exception if the loading operation fails and the object is in an undefined state.
     */
    uint64_t
    load_from_file(int fd, off_t offset);

    /**
     * @brief Write the description as Session block to a file
     *
     * @param[in] fd     File descriptor (must be opened for writing)
     * @param[in] offset Offset in the file where the Session block will be placed
     * @return Size of the written block (in bytes)
     */
    uint64_t
    write_to_file(int fd, off_t offset);

    /**
     * @brief Get the Transport Session ID
     * @return ID
     */
    uint16_t
    get_sid() const {return m_sid;};
    /**
     * @brief Get the description of Transport Session
     * @return Structure
     */
    const struct fds_file_session &
    get_struct() const {return m_struct;};

private:
    // Allow comparison functions access to internals
    friend bool operator==(const Block_session &lhs, const struct fds_file_session &rhs);
    friend bool operator==(const struct fds_file_session &lhs, const Block_session &rhs);

    /// Get the Transport Session identification
    uint16_t m_sid;
    /// Internal structure
    struct fds_file_session m_struct;

    // Check validity of description structure
    void
    check_validity(const struct fds_file_session &session);
};

/**
 * @brief Compare Transport Session object with a Session description
 *
 * @param[in] lhs Left side
 * @param[in] rhs Right side
 * @return True or false
 */
bool operator==(const Block_session &lhs, const struct fds_file_session &rhs);

/**
 * @brief Compare Transport Session object with a Session description
 *
 * @param[in] lhs Left side
 * @param[in] rhs Right side
 * @return True or false
 */
bool operator==(const struct fds_file_session &lhs, const Block_session &rhs);

/**
 * @brief Compare Transport Session objects
 *
 * For sorting by std::map etc. In other words, it's a binary predicate that, taking
 * two element keys as argument, returns true if the first argument goes before the
 * second argument in the strict weak ordering it defines, and false otherwise.
 * @param[in] lhs Left side
 * @param[in] rhs Right side
 * @return True or false
 */
bool
operator<(const Block_session &lhs, const Block_session &rhs);

} // namespace

#endif //LIBFDS_BLOCK_SESSION_HPP
