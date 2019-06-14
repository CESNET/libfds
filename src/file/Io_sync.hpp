/**
 * @file   src/file/Io_sync.hpp
 * @brief  Synchronous I/O request (header file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   April 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBFDS_IO_SYNC_HPP
#define LIBFDS_IO_SYNC_HPP

#include <unistd.h>
#include <cerrno>
#include "Io_request.hpp"

namespace fds_file {

/**
 * @brief Synchronous read/write I/O
 *
 * An instance of this class performs lazy evaluation of an I/O request. In other words, no
 * operations are performed until wait() function is called. Functions read() and write() are
 * used just to prepare for the future operation.
 */
class Io_sync : public Io_request {
public:
    /**
     * @brief Synchronous I/0 request constructor
     * @param[in] fd     File descriptor (for reading/writing)
     * @param[in] buffer Input/output buffer for I/0 request
     * @param[in] size   Size of the buffer
     */
    Io_sync(int fd, void *buffer, size_t size)
        : Io_request(fd, buffer, size) {};
    /**
     * @brief Class destructor
     */
    virtual ~Io_sync() = default;

    /**
     * @brief Move constructor
     * @param[in] other Source object
     */
    Io_sync(Io_sync &&other) noexcept;
    /**
     * @brief Move assign constructor
     * @param[in] other Source object
     */
    Io_sync &operator=(Io_sync &&other) noexcept;

    // Implemented functions
    void
    read(off_t offset, size_t size) override;
    void
    write(off_t offset, size_t size) override;
    size_t
    wait() override;
    void
    cancel() override;

private:
    /// Type of operation to perform
    enum class IO_Type {
        /// Read from a file
        IO_READ,
        /// Write to a file
        IO_WRITE
    } m_type;

    /// Offset from the start of the file
    off_t m_offset;
    /// Number of bytes to write/read
    size_t m_count;
};

} // Namespace

#endif //LIBFDS_IO_SYNC_HPP
