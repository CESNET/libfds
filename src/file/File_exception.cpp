/**
 * @file   src/file/File_exception.cpp
 * @brief  Common exception for all file manipulation classes(header file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   April 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Make sure that XSI-version of strerror is used
#undef _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <cstring>
#include "File_exception.hpp"

using namespace fds_file;

void
File_exception::throw_errno(int errno_code, const std::string &msg, int ecode)
{
    // Store the message to a local buffer
    constexpr size_t buffer_size = 64;
    char buffer[buffer_size];
    if (strerror_r(errno_code, buffer, buffer_size) != 0) {
        snprintf(buffer, buffer_size, "Unknown error (errno: %d)", errno_code);
    }

    if (msg.empty()) {
        throw File_exception(ecode, buffer);
    } else {
        throw File_exception(ecode, msg + ": " + buffer);
    }
}
