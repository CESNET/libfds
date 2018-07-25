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
    ERR_BLIST_SHORT,
    ERR_BLIST_UNEXP_END,
    ERR_ITER,
    ERR_VARSIZE_UNEXP_END,
};

/** Corresponding error messages */
static const char *err_msg[] = {
    [ERR_OK]                = "No error.",
    [ERR_BLIST_SHORT]       = "Size of the field is smaller than the minimal size of the Basic list.",
    [ERR_BLIST_UNEXP_END]   = "Unexpected end of the list while reading its members.",
    [ERR_VARSIZE_UNEXP_END] = "Unexpected end of the list while reading size of the member.",
    [ERR_ITER]              = "Iterator error.",
};

void
fds_blist_iter_init(struct fds_blist_iter *it, struct fds_drec_field *field,  fds_iemgr_t *ie_mgr)
{
    // Check if the Basic list can fit into the field
    if ( field->size < FDS_IPFIX_BLIST_HDR_SHORT){
        it->_private.err_msg = err_msg[ERR_BLIST_SHORT];
        it->_private.err_code = FDS_ERR_FORMAT;
        return;
    }
    // Point to the start and end of the basic list
    it->_private.blist = ((struct fds_ipfix_blist *)field->data);
    it->_private.blist_end = field->data + field->size;

    it->semantic = (enum fds_ipfix_list_semantics) it->_private.blist->semantic;

    // Filling the structure tfield
    it->_private.info.def = NULL;
    it->_private.info.id = ntohs(it->_private.blist->field_id);
    it->_private.info.length = ntohs(it->_private.blist->element_length);
    it->_private.info.en = 0;
    it->_private.info.flags = 0; // Do I need any flags?
    it->_private.info.offset = 0;

    uint32_t hdr_size;

    // Enterprise number NOT present
    if ((it->_private.info.id & (1u<<15)) == 0 ) {
        hdr_size = FDS_IPFIX_BLIST_HDR_SHORT;
    //Enterprise number present
    }
    else if ( field->size >= FDS_IPFIX_BLIST_HDR_LONG) {
        hdr_size = FDS_IPFIX_BLIST_HDR_LONG;
        it->_private.info.id = it->_private.info.id & ~(1u<<15);
        it->_private.info.en = ntohl(it->_private.blist->enterprise_number);
    }
    else {
        it->_private.err_msg = err_msg[ERR_BLIST_SHORT];
        it->_private.err_code = FDS_ERR_FORMAT;
        return;
    }
    it->_private.field_next = field->data + hdr_size;

    //If IEManager present, we will fill the definition of the field
    if (ie_mgr != NULL){
        it->_private.info.def = fds_iemgr_elem_find_id(ie_mgr, it->_private.info.en, it->_private.info.id);
    }

    //Error message OK
    it->_private.err_msg = err_msg[ERR_OK];
    it->_private.err_code = FDS_OK;
}

int
fds_blist_iter_next(struct fds_blist_iter *it)
{
    // Check if the iterator is without errors
    if (it->_private.err_code != FDS_OK){
        return it->_private.err_code;
    }

    // Check if there is another field in list to read
    if (it->_private.field_next == it->_private.blist_end){
        it->_private.err_code = FDS_EOC;
        return FDS_EOC;
    }

    // resolving the element length
    uint16_t elem_length = it->_private.info.length;
    uint8_t *rec_start = it->_private.field_next;
    uint32_t data_offset = 0;

    if (elem_length == FDS_IPFIX_VAR_IE_LEN) {
        // This is field with variable length encoding -> read size from data
        elem_length = rec_start[data_offset];
        data_offset++;

        if (elem_length == 255U) {
            // Check if we are not reading beyond the end of the list
            if (it->_private.field_next + 2U > it->_private.blist_end){
                it->_private.err_msg = err_msg[ERR_VARSIZE_UNEXP_END];
                it->_private.err_code = FDS_ERR_FORMAT;
                return FDS_ERR_FORMAT;
            }
            // Real size is on next 2 bytes
            elem_length = ntohs(*(uint16_t *) &rec_start[data_offset]);
            data_offset += 2U;
        }
    }

    // Check if we are not reading beyond the end of the list
    if (it->_private.field_next + elem_length > it->_private.blist_end){
        it->_private.err_msg = err_msg[ERR_BLIST_UNEXP_END];
        return FDS_ERR_FORMAT;
    }

    // Filling the structure, setting private properties,
    it->field.size = elem_length;
    it->field.data = &rec_start[data_offset];
    it->field.info = &(it->_private.info);

    // Setting the next-pointer to the next record
    it->_private.info.offset += (uint16_t) (data_offset + elem_length);
    it->_private.field_next = &rec_start[data_offset + elem_length];
    return FDS_OK;
}

const char *
fds_blist_iter_err(const struct fds_blist_iter *it)
{
    return it->_private.err_msg;
}

