/**
 * \file src/parsers/list_parser.c
 * \brief Simple parsers of a structured data types in IPFIX Message (source file)
 * \author Jan Kala <xkalaj01@stud.fit.vutbr.cz>
 * \date 2018
 */

/* Copyright (C) 2018 CESNET, z.s.p.o.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in
*    the documentation and/or other materials provided with the
*    distribution.
* 3. Neither the name of the Company nor the names of its contributors
*    may be used to endorse or promote products derived from this
*    software without specific prior written permission.
*
* ALTERNATIVELY, provided that this notice is retained in full, this
* product may be distributed under the terms of the GNU General Public
* License (GPL) version 2 or later, in which case the provisions
* of the GPL apply INSTEAD OF those given above.
*
* This software is provided ``as is'', and any express or implied
* warranties, including, but not limited to, the implied warranties of
* merchantability and fitness for a particular purpose are disclaimed.
* In no event shall the company or contributors be liable for any
* direct, indirect, incidental, special, exemplary, or consequential
* damages (including, but not limited to, procurement of substitute
* goods or services; loss of use, data, or profits; or business
* interruption) however caused and on any theory of liability, whether
* in contract, strict liability, or tort (including negligence or
* otherwise) arising in any way out of the use of this software, even
* if advised of the possibility of such damage.
*
*/

#include <libfds.h>

/** Error code of IPFIX list parsers */
enum error_codes {
    // No error
            ERR_OK,
};

/** Corresponding error messages */
static const char *err_msg[] = {
        [ERR_OK]             = "No error.",
};

void
fds_blist_iter_init(struct fds_blist_iter *it, struct fds_ipfix_msg_hdr *msg)
{
    // Initialization of the iterator
    // set the pointer for the next field in the list to the right place in the memmory (consider EN)
    // Do not fill the structure already, _next() function is there for that case
}

int
fds_blist_iter_next(struct fds_sets_iter *it)
{
    // Check if there is another field in list to read

    // Check if we are not reading beyond the end of the list

    // Check if we are not reading beyond the message

    // Filling the structure, setting private properties,


    return FDS_OK;
}

const char *
fds_blist_iter_err(const struct fds_dset_iter *it)
{
    return it->_private.err_msg;
}

