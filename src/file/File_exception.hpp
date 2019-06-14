/**
 * @file   src/file/File_exception.cpp
 * @brief  Common exception for all file manipulation classes(header file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   April 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBFDS_FILE_EXCEPTION_HPP
#define LIBFDS_FILE_EXCEPTION_HPP

#include <stdexcept>
#include <string>
#include <libfds/api.h>

namespace fds_file {

/// Custom File exception (common for all file I/O requests)
class File_exception : public std::runtime_error {
public:
    /**
     * @brief Class constructor
     * @param[in] ecode libfds specific error code
     * @param[in] msg   Error message
     */
    explicit File_exception(int ecode, const char *msg)
        : std::runtime_error(msg), m_code(ecode) {};
    /**
     * @brief Class constructor
     * @param[in] ecode libfds specific error code
     * @param[in] msg   Error message
     */
    explicit File_exception(int ecode, const std::string &msg)
        : std::runtime_error(msg), m_code(ecode) {};
    /**
     * @brief Get the libfds specific error code
     * @return Error code
     */
    int code() {return m_code;};

    /**
     * @brief Internal function for throwing an File_exception with errno message
     *
     * If the @p msg is not empty, the user specified message is placed before the errno message
     * and they are colon separated. Otherwise, only the errno message is used.
     * @param[in] msg        Optional prepend message (can be empty)
     * @param[in] errno_code errno code
     * @param[in] ecode      libfds error code
     * @throw File_exception with errno message and libfds specific error code
     */
    [[noreturn]] static void
    throw_errno(int errno_code, const std::string &msg = "", int ecode = FDS_ERR_INTERNAL);
private:
    /// Libfds error code
    int m_code;
};

} // namespace

#endif //LIBFDS_FILE_EXCEPTION_HPP
