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
};

/** Corresponding error messages */
static const char *err_msg[] = {
        [ERR_OK]             = "No error.",
        [ERR_BLIST_SHORT]    = "Size of the field is smaller than the minimal size of the Basic list.",
        [ERR_BLIST_UNEXP_END]= "Unexpected end of the list while reading its members",
};

int
fds_blist_iter_init(struct fds_blist_iter *it, struct fds_drec_field *field,  fds_iemgr_t *ie_mgr)
{
    // Check if the Basic list can fit into the field
    if (ntohs(field->size) < FDS_IPFIX_BLIST_HDR_SHORT){
        it->_private.err_msg = err_msg[ERR_BLIST_SHORT];
        return FDS_ERR_FORMAT;
    }
    // Point to the start and end of the basic list
    it->_private.blist = ((struct fds_ipfix_blist *)field->data);
    it->_private.blist_end = field->data + ntohs(field->size);

    it->_private.info->def = NULL;
    it->_private.info->id = ntohs(it->_private.blist->field_id);
    it->_private.info->length = ntohs(it->_private.blist->element_length);
    it->_private.info->en = 0;
    it->_private.info->flags = 0; // Do I need any flags?
    it->_private.info->offset = 0;

    uint32_t hdr_size;
    uint32_t en;

    // Enterprise number NOT present
    if (ntohs(it->_private.info->id) < FDS_IPFIX_BLIST_ENBIT_ON) {
        hdr_size = FDS_IPFIX_BLIST_HDR_SHORT;
    //Enterprise number present
    } else {
        hdr_size = FDS_IPFIX_BLIST_HDR_LONG;
        it->_private.info->id -= FDS_IPFIX_BLIST_ENBIT_ON;
        en = ntohl(it->_private.blist->enterprise_number);
        //If IEManager present, we will fill the definition of the field
        if (ie_mgr != NULL){
            it->_private.info->def = fds_iemgr_elem_find_id(ie_mgr, en, it->_private.blist->field_id);
        }
    }
    //Setting pointer to first record
    it->_private.field_next = field->data + hdr_size;

    //Error message OK
    it->_private.err_msg = err_msg[ERR_OK];
    return FDS_OK;


}

int
fds_blist_iter_next(struct fds_blist_iter *it)
{
    // Check if there is another field in list to read
    if (it->_private.field_next == it->_private.blist_end){
        return FDS_EOC;
    }
    // resolving the element length
    uint16_t elem_length;
    uint8_t *rec_start = it->_private.field_next;
    uint32_t offset = it->_private.info->offset;

    if (ntohs(it->_private.info->length) == FDS_IPFIX_VAR_IE_LEN) {
        // This is field with variable length encoding -> read size from data
        elem_length = rec_start[offset];
        offset++;

        if (elem_length == 255U) {
            // Real size is on next 2 bytes
            elem_length = ntohs(*(uint16_t *) &rec_start[offset]);
            offset += 2U;
        }
    }
    else{
        elem_length = ntohs(it->_private.blist->element_length);
    }

    // Check if we are not reading beyond the end of the list
    if (it->_private.field_next + elem_length > it->_private.blist_end){
        it->_private.err_msg = err_msg[ERR_BLIST_UNEXP_END];
        return FDS_ERR_FORMAT;
    }

    // Filling the structure, setting private properties,
    it->field.size = elem_length;
    it->field.data = &rec_start[offset];
    it->field.info = it->_private.info;

    // Setting the next-pointer to the next record
    it->_private.next_offset = offset + elem_length;
    it->_private.field_next = &rec_start[offset+elem_length];

    return FDS_OK;
}

const char *
fds_blist_iter_err(const struct fds_blist_iter *it)
{
    return it->_private.err_msg;
}

