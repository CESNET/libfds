/**
 * @file   src/file/Io_sync.hpp
 * @brief  Synchronous I/O request (source file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   April 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cerrno>
#include <unistd.h>
#include "Io_sync.hpp"
#include "File_exception.hpp"

namespace fds_file {

Io_sync::Io_sync(Io_sync &&other) noexcept
    : Io_request(std::move(other)), m_type(other.m_type), m_offset(other.m_offset),
      m_count(other.m_count)
{
    // Invalidate the source
    other.m_status = Status::IO_IDLE;
    other.m_fd = -1;
}

Io_sync &
Io_sync::operator=(Io_sync &&other) noexcept
{
    if (this == &other) {
        return *this;
    }

    // Cancel the current synchronous I/O (if any)
    cancel();

    // Call the base move
    Io_request::operator=(std::move(other));
    // Finally move other parameters
    m_type = other.m_type;
    m_offset = other.m_offset;
    m_count = other.m_count;

    // Invalidate the source
    other.m_status = Status::IO_IDLE;
    other.m_fd = -1;
    return *this;
}

void
Io_sync::read(off_t offset, size_t size)
{
    // I/O preconditions check
    io_precond(size);

    // Postpone the operation unit the result is required
    m_status = Status::IO_IN_PROGRESS;
    m_type = IO_Type::IO_READ;
    m_offset = offset;
    m_count = size;
}

void
Io_sync::write(off_t offset, size_t size)
{
    // I/O preconditions check
    io_precond(size);

    // Postpone the operation unit the result is required
    m_status = Status::IO_IN_PROGRESS;
    m_type = IO_Type::IO_WRITE;
    m_offset = offset;
    m_count = size;
}

size_t Io_sync::wait()
{
    if (m_status != Status::IO_IN_PROGRESS) {
        throw File_exception(FDS_ERR_INTERNAL, "No synchronous I/O operation has been configured "
            "but wait() was called!");
    }

    // Start the I/O operation
    ssize_t result;
    if (m_type == IO_Type::IO_READ) {
        result = pread(m_fd, m_buffer, m_count, m_offset);
    } else {
        result = pwrite(m_fd, m_buffer, m_count, m_offset);
    }

    m_status = Status::IO_IDLE;
    if (result >= 0) {
        // Success
        return static_cast<size_t>(result);
    }

    // Something bad happened
    File_exception::throw_errno(errno, "Synchronous I/O operation failed");
}

void
Io_sync::cancel()
{
    // Nothing to do
    m_status = Status::IO_IDLE;
    return;
}

} // namespace
