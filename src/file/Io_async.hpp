/**
 * @file   src/file/Io_async.hpp
 * @brief  Asynchronous I/O request (header file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   April 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBFDS_IO_ASYNC_HPP
#define LIBFDS_IO_ASYNC_HPP

#include <aio.h>
#include <memory>
#include "Io_request.hpp"

namespace fds_file {

/**
 * @brief  Asynchronous I/O request
 *
 * A requested I/O operation is performed in the background using POSIX AIO (usually implemented
 * as a pool of threads that performs synchronous read/write operation instead of the caller.
 *
 * Asynchronous read/write operations usually make sense only for bigger chunks of memory,
 * for example, at least hundreds of kilobytes. For smaller chunks, overhead is usually too high
 * and it's better to use the synchronous version of the I/O requests.
 */
class Io_async : public Io_request {
public:
    /**
     * @brief Asynchronous I/O request constructor
     * @param[in] fd     File descriptor (for reading/writing)
     * @param[in] buffer Input/output buffer for I/O request
     * @param[in] size   Size of the buffer
     */
    Io_async(int fd, void *buffer, size_t size);
    /**
     * @brief Class destructor
     * @note Stops a pending I/O and performs cleanup
     */
    ~Io_async() override;

    /**
     * @brief Move constructor
     * @param[in] other Source object
     */
    Io_async(Io_async &&other) noexcept;
    /**
     * @brief Move assign constructor
     * @param[in] other Source object
     */
    Io_async &operator=(Io_async &&other) noexcept;

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
    /// Internal operation type
    enum class IO_type {
        /// Read request
        IO_READ,
        /// Write request
        IO_WRITE
    };

    /// POSIX AIO structure
    std::unique_ptr<struct aiocb> m_ctx;

    void
    io_start(IO_type type, off_t offset, size_t size);
};

} // namespace

#endif //LIBFDS_IO_ASYNC_HPP
