/**
 * \file src/converters/converters.c
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \author Pavel Yadlouski <xyadlo00@stud.fit.vutbr.cz>
 * \brief Conversion of IPFIX Data Record to JSON  (source code)
 * \date May 2019
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2019 CESNET z.s.p.o.
 */

#include <libfds.h>

int
fds_drec2json(const struct fds_drec *rec, uint32_t flags, char **str,
    size_t *str_size)
{
    // TODO: implement me!
    return FDS_ERR_ARG;
}
