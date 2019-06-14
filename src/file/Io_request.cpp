/**
 * @file   src/file/Io_request.cpp
 * @brief  Base class for I/O request (source file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   April 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */



#include <cerrno>
#include <cstring>
#include "Io_request.hpp"
#include "Io_async.hpp"
#include "Io_sync.hpp"
#include "File_exception.hpp"

namespace fds_file {

/**
 * @brief Check precondition for I/0 operations
 *
 * The function checks if the previous operation has been completed and that the
 * input/output buffer is sufficient for the operation.
 * @param[in] io_size Number of bytes to read/write
 */
void Io_request::io_precond(size_t io_size)
{
    // Check if the previous operation results has been returned
    if (m_status == Status::IO_IN_PROGRESS) {
        throw File_exception(FDS_ERR_INTERNAL, "Previous I/O operation is in progress");
    }

    if (io_size > m_size) {
        throw File_exception(FDS_ERR_INTERNAL, "Insufficient buffer size for I/O operation");
    }
}

std::unique_ptr<Io_request>
Io_factory::new_request(int fd, void *buffer, size_t buffer_size, Io_factory::Type io_type)
{
    switch (io_type) {
    case Type::IO_DEFAULT:  // By default, create a asynchronous I/O
    case Type::IO_ASYNC:
        return std::unique_ptr<Io_request>(new Io_async(fd, buffer, buffer_size));
    case Type::IO_SYNC:
        return std::unique_ptr<Io_request>(new Io_sync(fd, buffer, buffer_size));
    }

    throw File_exception(FDS_ERR_INTERNAL, "Unsupported type of I/O request to create!");
}

} // namespace