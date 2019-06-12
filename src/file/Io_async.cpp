/**
 * @file   src/file/Io_async.cpp
 * @brief  Asynchronous I/O request (source file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   April 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cerrno>
#include <cstring>
#include "Io_async.hpp"
#include "File_exception.hpp"

namespace fds_file {

Io_async::Io_async(int fd, void *buffer, size_t size)
    : Io_request(fd, buffer, size), m_ctx(new struct aiocb)
{
}

Io_async::~Io_async()
{
    if (m_status == Status::IO_IDLE) {
        return;
    }

    // Cancel the current asynchronous I/O (if any)
    cancel();
}

Io_async::Io_async(Io_async &&other) noexcept
    : Io_request(std::move(other)), m_ctx(std::move(other.m_ctx))
{
    // Invalidate the source
    other.m_status = Status::IO_IDLE;
    other.m_fd = -1;
}

Io_async &
Io_async::operator=(Io_async &&other) noexcept
{
    if (this == &other) {
        return *this;
    }

    // First, cancel the current asynchronous I/0 (if any)
    cancel();
    // Call the base move assign constructor and move the rest
    Io_request::operator=(std::move(other));
    m_ctx = std::move(other.m_ctx);
    // Invalidate the source
    other.m_status = Status::IO_IDLE;
    other.m_fd = -1;
    return *this;
}

void
Io_async::read(off_t offset, size_t size)
{
    io_start(IO_type::IO_READ, offset, size);
}

void
Io_async::write(off_t offset, size_t size)
{
    io_start(IO_type::IO_WRITE, offset, size);
}

size_t
Io_async::wait()
{
    if (m_status != Status::IO_IN_PROGRESS) {
        throw File_exception(FDS_ERR_INTERNAL, "No asynchronous I/O operation has been configured "
            "but wait() was called!");
    }

    // Check if the operation was completed
    struct aiocb *ctx_ptr = m_ctx.get();
    int rc = aio_error(ctx_ptr);
    if (rc == EINPROGRESS) {
        // Wait for the operation to complete
        while (true) {
            rc = aio_suspend(&ctx_ptr, 1, NULL);
            if (rc == 0) {
                // Result is ready
                break;
            }

            if (rc == -1 && errno == EINTR) {
                // Interrupted, try again...
                continue;
            }

            // Something bad happened
            File_exception::throw_errno(errno, "aio_suspend() failed");
        }

        rc = aio_error(ctx_ptr);
        if (rc == EINPROGRESS) {
            throw File_exception(FDS_ERR_INTERNAL, "Unable to get status of asynchronous I/O");
        }
    }

    if (rc != 0) {
        // I/O failed
        File_exception::throw_errno(rc, "Asynchronous I/O operation failed");
    }

    // Operation complete
    m_status = Status::IO_IDLE;
    ssize_t val = aio_return(ctx_ptr);
    if (val >= 0) {
        // Success
        return static_cast<size_t>(val);
    }

    File_exception::throw_errno(errno, "Asynchronous I/O operation failed");
}

void
Io_async::cancel()
{
    if (m_status == Status::IO_IDLE) {
        // Nothing to do
        return;
    }

    int rc = aio_cancel(m_fd, m_ctx.get());
    if (rc == AIO_CANCELED || rc == AIO_ALLDONE) {
        // Canceled or completed
        m_status = Status::IO_IDLE;
        return;
    }

    // Unable to cancel -> wait for the operation to complete
    try {
        wait();
    } catch (File_exception &) {
        // Ignore...
    }

    // Complete
    m_status = Status::IO_IDLE;
}

/**
 * @brief Internal function starting I/O operation
 * @param[in] type   Type of the operation (read/write)
 * @param[in] offset Offset from the start of the file
 * @param[in] size   Number of bytes to read/write
 * @throw Io_exception if start of the operation fails
 */
void
Io_async::io_start(IO_type type, off_t offset, size_t size)
{
    // I/O precondition check
    io_precond(size);

    // Fill the operation structure
    struct aiocb *ctx_ptr = m_ctx.get();
    std::memset(ctx_ptr, 0, sizeof(*ctx_ptr));
    ctx_ptr->aio_fildes = m_fd;
    ctx_ptr->aio_offset = offset;
    ctx_ptr->aio_buf = m_buffer;
    ctx_ptr->aio_nbytes = size;
    ctx_ptr->aio_sigevent.sigev_notify = SIGEV_NONE;

    // Start the I/O
    int res;
    if (type == IO_type::IO_READ) {
        res = aio_read(ctx_ptr);
    } else {
        res = aio_write(ctx_ptr);
    }

    if (res == 0) {
        // No error
        m_status = Status::IO_IN_PROGRESS;
        return;
    }

    // Something bad happened
    File_exception::throw_errno(errno, "Failed to start asynchronous I/O");
}

} // namespace
