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
    ERR_INTERNAL,
    ERR_VARSIZE_UNEXP_END,
    ERR_ST_LIST_SHORT,
    ERR_STM_LIST_SHORT,
    ERR_STM_LIST_HDR_UNEXP_END,
    ERR_TMPLT_NOTFOUND,
    ERR_READ_RECORD
};

/** Corresponding error messages */
static const char *err_msg[] = {
    [ERR_OK]                            = "No error.",
    [ERR_BLIST_SHORT]                   = "Size of the field is smaller than the minimal size of the Basic list.",
    [ERR_BLIST_UNEXP_END]               = "Unexpected end of the list while reading its members.",
    [ERR_VARSIZE_UNEXP_END]             = "Unexpected end of the list while reading size of the member.",
    [ERR_ITER]                          = "Iterator error.",
    [ERR_INTERNAL]                      = "Internal error.",
    [ERR_ST_LIST_SHORT]                 = "Field is too small for subTemplateList to fit in.",
    [ERR_TMPLT_NOTFOUND]                = "Template ID was not found in a snapshot.",
    [ERR_STM_LIST_SHORT]                = "Field is too small for subTemplateMultiList to fit in.",
    [ERR_STM_LIST_HDR_UNEXP_END]        = "Unexpected end of record while reading header of the data record.",
    [ERR_READ_RECORD]                   = "Error while reading data record. Data is not in correct format."
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
    if ((it->_private.info.id & (1U<<15)) == 0 ) {
        hdr_size = FDS_IPFIX_BLIST_HDR_SHORT;
    }
    //Enterprise number present
    else if ( field->size >= FDS_IPFIX_BLIST_HDR_LONG) {
        hdr_size = FDS_IPFIX_BLIST_HDR_LONG;
        it->_private.info.id = it->_private.info.id & ~(1U<<15);
        it->_private.info.en = ntohl(it->_private.blist->enterprise_number);
    }
    // Error handling
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
    if (it->_private.field_next >= it->_private.blist_end){
        it->_private.err_code = FDS_EOC;
        return it->_private.err_code;
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
            if (it->_private.field_next + 3U > it->_private.blist_end){
                it->_private.err_msg = err_msg[ERR_VARSIZE_UNEXP_END];
                it->_private.err_code = FDS_ERR_FORMAT;
                return it->_private.err_code;
            }
            // Real size is on next 2 bytes
            elem_length = ntohs(*(uint16_t *) &rec_start[data_offset]);
            data_offset += 2U;
        }
    }

    // Check if we are not reading beyond the end of the list
    if (it->_private.field_next + elem_length + data_offset > it->_private.blist_end){
        it->_private.err_msg = err_msg[ERR_BLIST_UNEXP_END];
        it->_private.err_code = FDS_ERR_FORMAT;
        return it->_private.err_code;
    }

    // Filling the structure, setting private properties,
    it->field.size = elem_length;
    it->field.data = &rec_start[data_offset];
    it->field.info = &(it->_private.info);

    // Setting the next-pointer to the next record
    it->_private.info.offset += (uint16_t) (data_offset + elem_length);
    it->_private.field_next = &rec_start[data_offset + elem_length];
    it->_private.err_code = FDS_OK;
    return it->_private.err_code;
}

const char *
fds_blist_iter_err(const struct fds_blist_iter *it)
{
    return it->_private.err_msg;
}

//////////////////////////////////////////////////////////////

void
fds_stlist_iter_init(struct fds_stlist_iter *it, struct fds_drec_field *field, const fds_tsnapshot_t *snap, uint16_t flags)
{
    // subTemplateList part
    if (field->info->id == SUB_TMPLT_LIST_ID){
        //Check if the list can fit in the field
        if (field->size < FDS_IPFIX_STLIST_HDR) {
            it->_private.err_code = FDS_ERR_FORMAT;
            it->_private.err_msg = err_msg[ERR_ST_LIST_SHORT];
            return;
        }
        it->_private.next_rec = field->data + FDS_IPFIX_STLIST_HDR;
    }
    // subTemplateMultiList part
    else if (field->info->id == SUB_TMPLT_MULTI_LIST_ID){
        // Header is same structure as previous + 2U for data length field which is compulsory
        if (field->size < FDS_IPFIX_STLIST_HDR + 2U){
            it->_private.err_code = FDS_ERR_FORMAT;
            it->_private.err_msg = err_msg[ERR_STM_LIST_SHORT];
            return;
        }
        // Skipping only semantic field
        it->_private.next_rec = field->data + 1U;
    }
    // Common part for both types
    it->_private.field_id = field->info->id;
    it->_private.stlist = (struct fds_ipfix_stlist *) field->data;
    it->_private.snap = snap;
    it->_private.stlist_end = field->data + field->size;
    it->_private.next_offset = 0;
    it->_private.flags = flags;
    it->_private.err_msg = err_msg[ERR_OK];
    it->_private.err_code = FDS_OK;

    it->semantic = (enum fds_ipfix_list_semantics) it->_private.stlist->semantic;
}

int
fds_stlist_iter_next(struct fds_stlist_iter *it)
{
    // Check if iterator is without errors
    if (it->_private.err_code != FDS_OK && it->_private.err_code != FDS_ERR_NOTFOUND){
        return it->_private.err_code;
    }
    // Check if we are not reading beyond end of the list
    if (it->_private.next_rec >= it->_private.stlist_end){
        it->_private.err_code = FDS_EOC;
        return it->_private.err_code;
    }

    // Set the error code in advance
    it->_private.err_code = FDS_OK;

    uint16_t rec_size = 0;
    uint16_t tmplt_id;

    // subTemplateList part
    if (it->_private.field_id == SUB_TMPLT_LIST_ID){
        // Template ID is in the header
        tmplt_id = ntohs(it->_private.stlist->template_id);
        // Setting the record size to the size of whole data.
        // If the template will be found, the record size will change to the real size
        rec_size = (uint16_t) (it->_private.stlist_end - it->_private.next_rec);
    }

    // subTemplateMultiList part
    else if (it->_private.field_id == SUB_TMPLT_MULTI_LIST_ID){
        // Check if we are not reading beyond end of the message
        if (it->_private.next_rec + 4U > it->_private.stlist_end){
            it->_private.err_code = FDS_ERR_FORMAT;
            it->_private.err_msg = err_msg[ERR_STM_LIST_HDR_UNEXP_END];
            return it->_private.err_code;
        }
        // Get the template ID from data
        tmplt_id = ntohs((uint16_t) it->_private.next_rec);
        it->_private.next_rec += 2U;

        // Record size is also included in data
        rec_size = (uint16_t) it->_private.next_rec;
        it->_private.next_rec += 2U;
        it->_private.next_offset += 4U;
    }

    // Get the template from the snapshot
    const struct fds_template *tmplt = fds_tsnapshot_template_get(it->_private.snap,tmplt_id);
    if ((tmplt == NULL) && (it->_private.flags == FDS_STL_REPORT)){
        // Not a fatal error
        it->_private.err_msg = err_msg[ERR_TMPLT_NOTFOUND];
        it->_private.err_code = FDS_ERR_NOTFOUND;
    }

    //TODO EOC in subTemplate MultiLIst means next record but in subTmpltLst means end of list
    if (tmplt != NULL){
        int rc = get_record_size(tmplt, it->_private.next_rec, it->_private.stlist_end, &rec_size);
        if (rc != FDS_OK){
            // In case of FDS_EOC length in template needs more size than it's available
            // no padding is used in lists, so there will be error
            it->_private.err_code = FDS_ERR_FORMAT;
            it->_private.err_msg = err_msg[ERR_READ_RECORD];
            return it->_private.err_code;
        }
    }

    // Setting up the public part
    it->tid = tmplt_id;
    it->rec.size = rec_size;
    it->rec.snap = it->_private.snap;
    it->rec.data = it->_private.next_rec;
    it->rec.tmplt = tmplt;
    // Setting up the private part
    it->_private.next_rec += rec_size;
    it->_private.next_offset += rec_size;
    return it->_private.err_code;
}

int
get_record_size(const struct fds_template *tmplt, uint8_t* data_start, uint8_t* data_end, uint16_t *record_size)
{
    uint32_t size = tmplt->data_length;
    if (data_start + size > data_end) {
        // The rest of the Data Set is padding
        return FDS_EOC;
    }

    if ((tmplt->flags & FDS_TEMPLATE_DYNAMIC) == 0) {
        // Processing a static record
        *record_size = (uint16_t) size;
        return FDS_OK;
    }

    // Processing a dynamic record
    size = 0;
    uint16_t idx;

    for (idx = 0; idx < tmplt->fields_cnt_total; ++idx) {
        uint16_t field_size = tmplt->fields[idx].length;
        if (field_size != FDS_IPFIX_VAR_IE_LEN) {
            size += field_size;
            continue;
        }

        // This is a field with variable-length encoding
        if (&data_start[size + 1] > data_end) {
            // The memory is beyond the end of the Data Set
            break;
        }


        field_size = data_start[size];
        size += 1U;
        if (field_size != 255U) {
            size += field_size;
            continue;
        }

        if (&data_start[size + 2] > data_end) {
            // The memory is beyond the end of the Data Set
            return FDS_ERR_FORMAT;
        }

        field_size = ntohs(*(uint16_t *) &data_start[size]);
        size += 2U + field_size;
    }
    *record_size = (uint16_t) size;
    return FDS_OK;
}

const char *
fds_stlist_iter_err(struct fds_stlist_iter *it)
{
    return it->_private.err_msg;
}
