/**
 * @file   src/3rd_party/fds_lz4.h
 * @brief  Include of a proper LZ4 header file
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   June 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBFDS_FDS_LZ4_H
#define LIBFDS_FDS_LZ4_H

#include <build_config.h>

// Use LZ4 library configured by CMake
#ifdef USE_SYSTEM_LZ4
#include <lz4.h>
#else
#include "lz4/lz4.h"
#endif

#endif // LIBFDS_FDS_LZ4_H
