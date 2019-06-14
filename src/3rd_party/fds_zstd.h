/**
 * @file   src/3rd_party/fds_zstd.h
 * @brief  Include of a proper ZSTD header file
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   June 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBFDS_FDS_ZSTD_H
#define LIBFDS_FDS_ZSTD_H

#include <build_config.h>

// Use LZ4 library configured by CMake
#ifdef USE_SYSTEM_ZSTD
#include <zstd.h>
#else
#include "zstd/src/lib/zstd.h"
#endif

#endif // LIBFDS_FDS_ZSTD_H
