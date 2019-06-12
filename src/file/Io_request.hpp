/**
 * @file   src/file/Io_request.hpp
 * @brief  Base class for I/O request (header file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   April 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBFDS_IO_REQUEST_HPP
#define LIBFDS_IO_REQUEST_HPP

#include <stdexcept>
#include <string>
#include <memory>

#include <cstdbool>
#include <sys/types.h>  // off_t
#include <stddef.h>     // size_t

namespace fds_file {

/// Base class for I/0 operations
class Io_request {
public:
    /**
     * @brief Base class constructor
     * @param[in] fd     File descriptor (for reading/writing)
     * @param[in] buffer Input/output buffer for I/0 request
     * @param[in] size   Size of the buffer
     */
    Io_request(int fd, void *buffer, size_t size)
        : m_fd(fd), m_buffer(buffer), m_size(size), m_status(Status::IO_IDLE) {};
    /**
     * @brief Base class destructor
     *
     * Terminates a pending request (if any) and performs necessary cleanup.
     */
    virtual ~Io_request() = default;

    // Disable copy constructors
    Io_request(const Io_request &other) = delete;
    Io_request &operator=(const Io_request &other) = delete;

    /**
     * @brief Move constructor
     * @param[in] other Source object
     */
    Io_request(Io_request &&other) = default;
    /**
     * @brief Move assign constructor
     * @param[in] other Source object
     */
    Io_request &operator=(Io_request &&other) = default;

    /**
     * @brief Initialize read operation
     *
     * After calling this function read operation is started in background. To wait until the
     * operation is complete, you have to call wait() or cancel(). Keep on mind that you are
     * also not able to start another I/O using this handler until the previous operation has
     * been completed or canceled!
     *
     * @note The file (i.e. its file descriptor) MUST be opened for reading.
     * @warning
     *   Content of the buffer passed to the constructor MUST NOT be changed until the operation
     *   is complete!
     *
     * @param[in] offset Offset (from the start of the file)
     * @param[in] size   Read up to N bytes from the file to the buffer (MUST be always smaller
     *   or equal to the size of the output buffer)
     * @throw Io_exception if another operation is already in progress or initialization fails
     */
    virtual void
    read(off_t offset, size_t size) = 0;

    /**
     * @brief Initialize write operation
     *
     * After calling this function write operation is started in background. To wait until the
     * operation is complete, you have to call wait() or cancel(). Keep on mind that you are also
     * not able to start another I/O using this handler until the previous operation has been
     * completed or canceled!
     *
     * @warning
     *   Content of the buffer passed to the constructor MUST NOT be changed until the operation
     *   is complete!
     * @param[in] offset Offset (from the start of the file)
     * @param[in] size   Write N bytes from the buffer to file (the size MUST be always smaller
     *   or equal to the size of the input buffer)
     * @throw Io_exception if another operation is already in progress or initialization fails
     */
    virtual void
    write(off_t offset, size_t size) = 0;

    /**
     * @brief Wait until the I/0 operation is complete
     *
     * @note
     *   In case of read operation, a return of zero indicates end of file. It is also not an error
     *   if the number is smaller than the number of bytes requested. This may happen for example
     *   because fewer bytes are actually available right (maybe because we were close to the
     *   end-of-file), or the operation was interrupted by a signal.
     * @note
     *   In case of write operation, if the number is smaller than the number of bytes requested,
     *   there is insufficient space on the medium, or resource limit is encountered, or the call
     *   was interrupted by a signal handler.
     * @return Number of bytes read/written
     * @throw Io_exception if an error has occurred during read/write operation or if no operation
     *   has been configured.
     */
    virtual size_t
    wait() = 0;

    /**
     * @brief Cancel an outstanding I/O request
     *
     * If there is not operation in progress, the function does nothing. However, if there is
     * running read/write I/O, the content of the buffer (in case of read operation) or the file
     * (in case of write operation) is not defined!
     *
     * @note
     *   If the operation cannot be terminated, it waits until the operation is complete!
     */
    virtual void
    cancel() = 0;

protected:
    /// Internal status of the operation
    enum class Status {
        /// No operation in progress
        IO_IDLE,
        /// Operation currently in progress
        IO_IN_PROGRESS,
    };

    /// File descriptor
    int m_fd;
    /// Input/output buffer
    void *m_buffer;
    /// Size of the buffer
    size_t m_size;
    /// Status of the operation
    Status m_status;

    void
    io_precond(size_t io_size);
};

/// Auxiliary factory for creating disk I/O requests
class Io_factory {
public:
    /// Type of I/O request to receive
    enum class Type {
        /// Default type of request (based on the library configuration)
        IO_DEFAULT,
        /// Synchronous requests
        IO_SYNC,
        /// Asynchronous requests
        IO_ASYNC
    };

    /**
     * @brief Create a new I/O request
     * @param[in] fd          File descriptor (for reading/writing)
     * @param[in] buffer      Input/output buffer for I/0 request
     * @param[in] buffer_size Size of the buffer
     * @param[in] io_type     Type of the request to create
     * @return Pointer to the new request
     */
    static std::unique_ptr<Io_request>
    new_request(int fd, void *buffer, size_t buffer_size, Type io_type = Type::IO_DEFAULT);
};

} // namespace

#endif //LIBFDS_IO_REQUEST_HPP
