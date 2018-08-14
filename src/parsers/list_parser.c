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
    ERR_VARSIZE_UNEXP_END,
    ERR_ST_LIST_SHORT,
    ERR_STM_LIST_SHORT,
    ERR_STM_LIST_HDR_UNEXP_END,
    ERR_TMPLT_NOTFOUND,
    ERR_READ_RECORD,
    ERR_FIELD_TYPE,
    ERR_FIELD_DEF_MISSING,
    ERR_TMPLTID_NOT_VALID
};

/** Corresponding error messages */
static const char *err_msg[] = {
    [ERR_OK]                            = "No error.",
    [ERR_BLIST_SHORT]                   = "Size of the field is smaller than the minimal size of the Basic list.",
    [ERR_BLIST_UNEXP_END]               = "Unexpected end of the list while reading its members.",
    [ERR_VARSIZE_UNEXP_END]             = "Unexpected end of the list while reading size of the member.",
    [ERR_ST_LIST_SHORT]                 = "Field is too small for subTemplateList to fit in.",
    [ERR_TMPLT_NOTFOUND]                = "Template ID was not found in a snapshot.",
    [ERR_STM_LIST_SHORT]                = "Field is too small for subTemplateMultiList to fit in.",
    [ERR_STM_LIST_HDR_UNEXP_END]        = "Unexpected end of record while reading header of the data record.",
    [ERR_READ_RECORD]                   = "Error while reading data record. Data is not in correct format.",
    [ERR_FIELD_TYPE]                    = "Field is not subTemplateList nor subTemplateMultiList.",
    [ERR_FIELD_DEF_MISSING]             = "Field definition is missing therefore it cannot be used for this iterator.",
    [ERR_TMPLTID_NOT_VALID]             = "Template ID is not valid!"
};

void
fds_blist_iter_init(struct fds_blist_iter *it, struct fds_drec_field *field,  fds_iemgr_t *ie_mgr)
{
    if (field->size < FDS_IPFIX_BLIST_SHORT_HDR_LEN) {
        // Check if the Basic list can fit into the field
        it->_private.err_msg = err_msg[ERR_BLIST_SHORT];
        it->_private.err_code = FDS_ERR_FORMAT;
        return;
    }
    // Point to the start and end of the basic list
    it->_private.blist = ((struct fds_ipfix_blist *)field->data);
    it->_private.blist_end = field->data + field->size;

    if (it->_private.blist->semantic > 0 && it->_private.blist->semantic <= 4){
        // Semantic is known
        it->semantic = (enum fds_ipfix_list_semantics) it->_private.blist->semantic;
    } else {
        // Unknown semantic
        it->semantic = FDS_IPFIX_LIST_UNDEFINED;
    }

    // Filling the structure tfield
    it->_private.info.def = NULL;
    it->_private.info.id = ntohs(it->_private.blist->field_id);
    it->_private.info.length = ntohs(it->_private.blist->element_length);
    it->_private.info.en = 0;
    it->_private.info.flags = 0; // Do I need any flags?
    it->_private.info.offset = 0;

    uint32_t hdr_size;

    if ((it->_private.info.id & (1U << 15)) == 0 ) {
        // Enterprise number NOT present
        hdr_size = FDS_IPFIX_BLIST_SHORT_HDR_LEN;
    } else if (field->size >= FDS_IPFIX_BLIST_LONG_HDR_LEN) {
        // Enterprise number present
        hdr_size = FDS_IPFIX_BLIST_LONG_HDR_LEN;
        it->_private.info.id = it->_private.info.id & ~(1U << 15);
        it->_private.info.en = ntohl(it->_private.blist->enterprise_number);
    } else {
        // Error handling
        it->_private.err_msg = err_msg[ERR_BLIST_SHORT];
        it->_private.err_code = FDS_ERR_FORMAT;
        return;
    }
    it->_private.field_next = field->data + hdr_size;

    if (ie_mgr != NULL) {
        // If IEManager present, we will fill the definition of the field
        it->_private.info.def = fds_iemgr_elem_find_id(ie_mgr, it->_private.info.en, it->_private.info.id);
    }

    // Error message OK
    it->_private.err_msg = err_msg[ERR_OK];
    it->_private.err_code = FDS_OK;
}

int
fds_blist_iter_next(struct fds_blist_iter *it)
{
    if (it->_private.err_code != FDS_OK) {
        // Check if the iterator is without errors
        return it->_private.err_code;
    }

    if (it->_private.field_next >= it->_private.blist_end) {
        // Check if there is another field in list to read
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
            if (it->_private.field_next + 3U > it->_private.blist_end) {
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
    if (it->_private.field_next + elem_length + data_offset > it->_private.blist_end) {
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
    // Check the definition of the field
    enum fds_iemgr_element_type type
    if ((flags & FDS_STL_FLAG_AS_SUBTEMPLIST) != 0) {
        // Forced interpretation as subTemplateList
        type = FDS_ET_SUB_TEMPLATE_LIST;
    } else if ((flags & FDS_STL_FLAG_AS_SUBTEMPMULTILIST) != 0) {
        // Forced interpretation as subTemplateMultiList
        type = FDS_ET_SUB_TEMPLATE_MULTILIST;
    } else if (field->info->def == NULL) {
        // Definition of the field is not available - cannot be interpreted
        it->_private.err_code = FDS_ERR_FORMAT;
        it->_private.err_msg = err_msg[ERR_FIELD_DEF_MISSING];
        return;
    } else {
        // Default state - reading type from available definition
        type = field->info->def->data_type;
        if(type != FDS_ET_SUB_TEMPLATE_LIST && type != FDS_ET_SUB_TEMPLATE_MULTILIST) {
            // Is this error in format?
            it->_private.err_code = FDS_ERR_FORMAT;
            it->_private.err_msg = err_msg[ERR_FIELD_TYPE];
            return;
        }
    }

    if (type == FDS_ET_SUB_TEMPLATE_LIST) {
        // subTemplateList part
        if (field->size < FDS_IPFIX_STLIST_HDR_LEN) {
            it->_private.err_code = FDS_ERR_FORMAT;
            it->_private.err_msg = err_msg[ERR_ST_LIST_SHORT];
            return;
        }
        it->_private.next_rec = field->data + FDS_IPFIX_STLIST_HDR_LEN;
    } else {
        // subTemplateMultiList part
        if (field->size < FDS_IPFIX_STMULTILIST_HDR_LEN) {
            it->_private.err_code = FDS_ERR_FORMAT;
            it->_private.err_msg = err_msg[ERR_STM_LIST_SHORT];
            return;
        }
        // Skipping only semantic field
        it->_private.next_rec = field->data + 1U;
    }
    // Common part for both types
    it->_private.type = type;
    it->_private.stlist = (struct fds_ipfix_stlist *) field->data;
    it->_private.snap = snap;
    it->_private.stlist_end = field->data + field->size;
    it->_private.flags = flags;

    it->_private.err_msg = err_msg[ERR_OK];
    it->_private.err_code = FDS_OK;

    if (it->_private.stlist->semantic > 0 && it->_private.stlist->semantic <= 4){
        // Semantic is known
        it->semantic = (enum fds_ipfix_list_semantics) it->_private.stlist->semantic;
    } else {
        // Unknown semantic
        it->semantic = FDS_IPFIX_LIST_UNDEFINED;
    }
    // Setting the end of records to the same place as next record so the next function will read the header first
    it->_private.recs_end = it->_private.next_rec;
}

const struct fds_template *
read_header(struct fds_stlist_iter *it);

int
get_record_size(const struct fds_template *tmplt, uint8_t* data_start, uint8_t* data_end, uint16_t *record_size);

int
fds_stlist_iter_next(struct fds_stlist_iter *it)
{
    // Check if iterator is without errors
    if (it->_private.err_code != FDS_OK && it->_private.err_code != FDS_ERR_NOTFOUND) {
        return it->_private.err_code;
    }
    // Check if we are not reading beyond end of the list
    if (it->_private.next_rec >= it->_private.stlist_end) {
        if (it->_private.stlist_end == it->_private.recs_end) {
            // Size of the data that was read equals size in the header
            it->_private.err_code = FDS_EOC;
        } else {
            // Size of the data that was read is smaller than size in header - wrong format
            it->_private.err_code = FDS_ERR_FORMAT;
            it->_private.err_msg = err_msg[ERR_READ_RECORD];
        }
        return it->_private.err_code;

    }

    // Set the error code in advance
    it->_private.err_code = FDS_OK;

    const struct fds_template * tmplt;
    if (it->_private.recs_end == it->_private.next_rec) {
        // If we need to read header (start of iteration e.g.) we will read it
        tmplt = read_header(it);
    } else {
        // Otherwise template stays the same
        tmplt = it->rec.tmplt;
    }

    // read record
    uint16_t rec_size;
    if (tmplt != NULL) {
        // Template is available
        int rc = get_record_size(tmplt, it->_private.next_rec, it->_private.recs_end, &rec_size);
        if (rc != FDS_OK) {
            it->_private.err_code = FDS_ERR_FORMAT;
            it->_private.err_msg = err_msg[ERR_READ_RECORD];
            return it->_private.err_code;
        }
    } else {
        // Template is missing so we skip all the records because we don't know how to read them
        rec_size = (uint16_t) (it->_private.recs_end - it->_private.next_rec);
    }

    // Setting up the public part
    it->rec.size = rec_size;
    it->rec.snap = it->_private.snap;
    it->rec.data = it->_private.next_rec;
    it->rec.tmplt = tmplt;
    // Setting up the private part
    it->_private.next_rec += rec_size;
    return it->_private.err_code;
}

const char *
fds_stlist_iter_err(struct fds_stlist_iter *it)
{
    return it->_private.err_msg;
}

int
get_record_size(const struct fds_template *tmplt, uint8_t* data_start, uint8_t* data_end, uint16_t *record_size)
{
    uint32_t size = tmplt->data_length;

    if ((tmplt->flags & FDS_TEMPLATE_DYNAMIC) == 0) {
        // Processing a static record
        if (data_start + size <= data_end) {
            *record_size = (uint16_t) size;
            return FDS_OK;
        } else {
            return FDS_ERR_FORMAT;
        }
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
            return FDS_ERR_FORMAT;
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

    if (data_start + size <= data_end) {
        *record_size = (uint16_t) size;
        return FDS_OK;
    } else {
        return FDS_ERR_FORMAT;
    }
}

const struct fds_template *
read_header(struct fds_stlist_iter *it)
{
    uint16_t tmplt_id;
    const struct fds_template * tmplt;


    if (it->_private.type == FDS_ET_SUB_TEMPLATE_LIST) {
        tmplt_id = ntohs(it->_private.stlist->template_id);
        it->_private.recs_end = it->_private.stlist_end;
    } else {
        // Check if we are not reading beyond the message
        if (it->_private.next_rec + FDS_IPFIX_STMULTILIST_HDR_LEN > it->_private.stlist_end) {
            it->_private.err_msg = err_msg[ERR_STM_LIST_HDR_UNEXP_END];
            it->_private.err_code = FDS_ERR_FORMAT;
            return NULL;
        }
        struct fds_ipfix_set_hdr *hdr = (struct fds_ipfix_set_hdr *) it->_private.next_rec;
        tmplt_id = ntohs(hdr->flowset_id);
        uint16_t rec_size = ntohs(hdr->length);
        it->_private.recs_end = it->_private.next_rec + rec_size;
    }

    if (tmplt_id < 256) {
        // Template ID must be greater than 256
        it->_private.err_msg = err_msg[ERR_TMPLTID_NOT_VALID];
        it->_private.err_code = FDS_ERR_FORMAT;
        return NULL
    }

    // Get the template from the snapshot
    it->tid = tmplt_id;
    tmplt = fds_tsnapshot_template_get(it->_private.snap,tmplt_id);
    if ((tmplt == NULL) && ((it->_private.flags & FDS_STL_FLAG_REPORT) != 0)) {
        // Not a fatal error
        it->_private.err_msg = err_msg[ERR_TMPLT_NOTFOUND];
        it->_private.err_code = FDS_ERR_NOTFOUND;
    }
    return tmplt;

}
