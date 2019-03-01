/**
 * \file src/parsers/list_parser.c
 * \brief Simple parsers of a structured data types in IPFIX Message (source file)
 * \author Jan Kala <xkalaj01@stud.fit.vutbr.cz>
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
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
    ERR_BLIST_ZERO,
    ERR_VARSIZE_UNEXP_END,
    ERR_ST_LIST_SHORT,
    ERR_STM_LIST_SHORT,
    ERR_STM_LIST_UNEXP_END,
    ERR_TMPLT_NOTFOUND,
    ERR_TMPLTID_NOT_VALID,
    ERR_SET_EXCEED_LIST,
    ERR_REC_EXCEED_LIST,
    ERR_STM_LIST_SET
};

/** Corresponding error messages */
static const char *err_msg[] = {
    [ERR_OK]                 = "No error.",
    [ERR_BLIST_SHORT]        = "Length of the field is smaller than the minimal size of the Basic list.",
    [ERR_BLIST_UNEXP_END]    = "Unexpected end of the list while reading its members.",
    [ERR_BLIST_ZERO]         = "Zero-length fields cannot be stored in the list.",
    [ERR_VARSIZE_UNEXP_END]  = "Unexpected end of the list while reading size of the member.",
    [ERR_ST_LIST_SHORT]      = "Field is too small for subTemplateList to fit in.",
    [ERR_STM_LIST_SHORT]     = "Field is too small for subTemplateMultiList to fit in.",
    [ERR_STM_LIST_UNEXP_END] = "Unexpected end of the list.",
    [ERR_TMPLT_NOTFOUND]     = "Template ID was not found in a snapshot.",
    [ERR_TMPLTID_NOT_VALID]  = "Template ID (< 256) is not valid for Data records.",
    [ERR_SET_EXCEED_LIST]    = "Inner Data Set length exceeds the length of its enclosing list.",
    [ERR_REC_EXCEED_LIST]    = "Inner Data Record Length exceeds the length of its enclosing list.",
    [ERR_STM_LIST_SET]       = "Invalid Data Records Length (< 4B, see also RFC6313 Errata)."
};

void
fds_blist_iter_init(struct fds_blist_iter *it, struct fds_drec_field *field,
    const fds_iemgr_t *ie_mgr)
{
    // Set default values before parsing
    memset(&it->field, 0, sizeof(it->field));
    it->semantic = FDS_IPFIX_LIST_UNDEFINED;

    if (field->size < FDS_IPFIX_BLIST_SHORT_HDR_LEN) {
        // Check if the Basic list can fit into the field
        it->_private.err_msg = err_msg[ERR_BLIST_SHORT];
        it->_private.err_code = FDS_ERR_FORMAT;
        return;
    }
    // Point to the start and end of the basic list
    it->_private.blist = (struct fds_ipfix_blist *) field->data;
    it->_private.blist_end = field->data + field->size;

    if (it->_private.blist->semantic <= FDS_IPFIX_LIST_ORDERED) {
        // Semantic is known
        it->semantic = (enum fds_ipfix_list_semantics) it->_private.blist->semantic;
    }

    // Filling the structure tfield
    it->_private.info.def = NULL;
    it->_private.info.id = ntohs(it->_private.blist->field_id);
    it->_private.info.length = ntohs(it->_private.blist->element_length);
    it->_private.info.en = 0;
    it->_private.info.flags = 0;
    it->_private.info.offset = 0;
    it->field.info = &it->_private.info;

    uint32_t hdr_size;
    if ((it->_private.info.id & (1U << 15)) == 0) {
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

    // Check if the element has non-zero length
    if (it->_private.info.length == 0 && it->_private.field_next != it->_private.blist_end) {
        // Non-empty list with zero-length fields would cause infinite loop!
        it->_private.err_msg = err_msg[ERR_BLIST_ZERO];
        it->_private.err_code = FDS_ERR_FORMAT;
        return;
    }

    if (ie_mgr != NULL) {
        // If an IE manager is defined, try to fill the definition of the field
        const struct fds_iemgr_elem *elem = fds_iemgr_elem_find_id(ie_mgr, it->_private.info.en,
            it->_private.info.id);
        it->_private.info.def = elem;

        if (elem != NULL) {
            // Try to fill flags
            it->_private.info.flags |= (fds_iemgr_is_type_list(elem->data_type))
                ? FDS_TFIELD_STRUCT : 0;
            it->_private.info.flags |= elem->is_reverse ? FDS_TFIELD_REVERSE : 0;
        }
    }

    // Everything is ready
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

    // Resolving the element length
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

/**
 * \brief Auxiliary function that determines the real size of a Data record
 *
 * \param[in]  tmplt      Template of the record to parse
 * \param[in]  data_start Start of the the record
 * \param[in]  data_end   First byte after the end of the block where the record can be stored
 * \return If the record is malformed or exceeds the size of the block, returns 0. Otherwise
 *   real size > 0 is returned.
 */
static uint16_t
stl_rec_size(const struct fds_template *tmplt, const uint8_t *data_start, const uint8_t *data_end)
{
    assert(data_start <= data_end && "Range doesn't represent a continuous block!");
    if (data_start + tmplt->data_length > data_end) {
        // The record cannot fit to the block - padding is not allowed for lists
        return 0;
    }

    if ((tmplt->flags & FDS_TEMPLATE_DYNAMIC) == 0) {
        // Processing a static record
        return tmplt->data_length;
    }

    // Processing a dynamic record
    uint32_t size = 0;

    for (uint16_t idx = 0; idx < tmplt->fields_cnt_total; ++idx) {
        uint16_t field_size = tmplt->fields[idx].length;
        if (field_size != FDS_IPFIX_VAR_IE_LEN) {
            size += field_size;
            continue;
        }

        // This is a field with variable-length encoding
        if (&data_start[size + 1] > data_end) {
            // The memory is beyond the end of the Data Set
            return 0;
        }

        field_size = data_start[size];
        size += 1U;
        if (field_size != 255U) {
            size += field_size;
            continue;
        }

        if (&data_start[size + 2] > data_end) {
            // The memory is beyond the end of the Data Set
            return 0;
        }

        field_size = ntohs(*(uint16_t *) &data_start[size]);
        size += 2U + field_size;
    }

    if (data_start + size > data_end || size > UINT16_MAX) {
        return 0;
    }

    return (uint16_t) size;
}

void
fds_stlist_iter_init(struct fds_stlist_iter *it, struct fds_drec_field *field,
    const fds_tsnapshot_t *snap, uint16_t flags)
{
    assert(snap != NULL && "Snapshot is NULL!");
    assert(field != NULL && "Field to iterate over is NULL!");

    // Set the default semantic and Template ID(just in case the header is invalid)
    it->semantic = FDS_IPFIX_LIST_UNDEFINED;
    it->tid = 0;

    if (field->size < FDS_IPFIX_STLIST_HDR_LEN) {
        // Header is too short
        it->private__.err_code = FDS_ERR_FORMAT;
        it->private__.err_msg = err_msg[ERR_ST_LIST_SHORT];
        return;
    }

    // Get the template from the snapshot
    const struct fds_ipfix_stlist *list_hdr = (const struct fds_ipfix_stlist *) field->data;
    uint16_t tmplt_id = ntohs(list_hdr->template_id);
    if (tmplt_id < FDS_IPFIX_SET_MIN_DSET) {
        it->private__.err_msg = err_msg[ERR_TMPLTID_NOT_VALID];
        it->private__.err_code = FDS_ERR_FORMAT;
        return;
    }

    // Check the type of list semantic
    if (list_hdr->semantic <= FDS_IPFIX_LIST_ORDERED) {
        it->semantic = (enum fds_ipfix_list_semantics) list_hdr->semantic;
    }

    // Set public section
    it->tid = tmplt_id;
    it->rec.snap = snap;
    it->rec.tmplt = fds_tsnapshot_template_get(snap, tmplt_id);

    if ((flags & FDS_STL_REPORT) != 0 && it->rec.tmplt == NULL) {
        it->private__.err_msg = err_msg[ERR_TMPLT_NOTFOUND];
        it->private__.err_code = FDS_ERR_NOTFOUND;
        return;
    }

    // Set private section
    it->private__.rec_next = field->data + FDS_IPFIX_STLIST_HDR_LEN;
    it->private__.list_end = field->data + field->size;
    it->private__.flags = flags;
    it->private__.err_code = FDS_OK;
    it->private__.err_msg = err_msg[ERR_OK];
}

int
fds_stlist_iter_next(struct fds_stlist_iter *it)
{
    if (it->private__.err_code != FDS_OK) {
        return it->private__.err_code;
    }

    if (it->rec.tmplt == NULL || it->private__.rec_next >= it->private__.list_end) {
        // Unknown template or the iterator has reached end of the list
        return FDS_EOC;
    }

    uint16_t rec_size = stl_rec_size(it->rec.tmplt, it->private__.rec_next, it->private__.list_end);
    if (rec_size == 0) {
        it->private__.err_code = FDS_ERR_FORMAT;
        it->private__.err_msg = err_msg[ERR_REC_EXCEED_LIST];
        return it->private__.err_code;
    }

    it->rec.data = it->private__.rec_next;
    it->rec.size = rec_size;
    it->private__.rec_next += rec_size;
    assert(it->private__.rec_next <= it->private__.list_end);
    return FDS_OK;
}

const char *
fds_stlist_iter_err(struct fds_stlist_iter *it)
{
    return it->private__.err_msg;
}

//////////////////////////////////////////////////////////////

void
fds_stmlist_iter_init(struct fds_stmlist_iter *it, struct fds_drec_field *field,
    const fds_tsnapshot_t *snap, uint16_t flags)
{
    assert(snap != NULL && "Snapshot is NULL!");
    assert(field != NULL && "Field to iterate over is NULL!");

    // Set the default semantic (just in case the header is invalid)
    it->semantic = FDS_IPFIX_LIST_UNDEFINED;

    if (field->size < FDS_IPFIX_STMULTILIST_HDR_LEN) {
        it->private__.err_code = FDS_ERR_FORMAT;
        it->private__.err_msg = err_msg[ERR_STM_LIST_SHORT];
        return;
    }

    struct fds_ipfix_stlist *list_hdr = (struct fds_ipfix_stlist *) field->data;

    // Check the type of list semantic
    if (list_hdr->semantic <= FDS_IPFIX_LIST_ORDERED) {
        it->semantic = (enum fds_ipfix_list_semantics) list_hdr->semantic;
    }

    // Set the public section
    memset(&it->rec, 0, sizeof(it->rec));

    // Set the private section
    it->private__.list_end = field->data + field->size;
    it->private__.block_next = (uint8_t *) &list_hdr->template_id;
    it->private__.rec_next = it->private__.block_next;
    it->private__.snap = snap;
    it->private__.tmplt = NULL;
    it->private__.flags = flags;
    it->private__.err_code = FDS_OK;
    it->private__.err_msg = err_msg[ERR_OK];
}

int
fds_stmlist_iter_next_block(struct fds_stmlist_iter *it)
{
    if (it->private__.err_code != FDS_OK) {
        return it->private__.err_code;
    }

    memset(&it->rec, 0, sizeof(it->rec));

    // Find the first block with a known Template record
    while (1) {
        if (it->private__.block_next >= it->private__.list_end) {
            // End of the list has been reached
            it->private__.rec_next = it->private__.block_next;
            return FDS_EOC;
        }

        if (it->private__.block_next + FDS_IPFIX_SET_HDR_LEN > it->private__.list_end) {
            // The next header exceeds the size of the lists
            it->private__.err_msg = err_msg[ERR_STM_LIST_UNEXP_END];
            it->private__.err_code = FDS_ERR_FORMAT;
            return it->private__.err_code;
        }

        struct fds_ipfix_dset *set_hdr = (struct fds_ipfix_dset *) it->private__.block_next;
        uint16_t tmplt_id = ntohs(set_hdr->header.flowset_id);
        uint16_t set_len = ntohs(set_hdr->header.length);

        if (tmplt_id < FDS_IPFIX_SET_MIN_DSET) {
            // Invalid Template ID
            it->private__.err_msg = err_msg[ERR_TMPLTID_NOT_VALID];
            it->private__.err_code = FDS_ERR_FORMAT;
            return it->private__.err_code;
        }

        if (set_len < FDS_IPFIX_SET_HDR_LEN) {
            // Invalid Header length of the header
            it->private__.err_msg = err_msg[ERR_STM_LIST_SET];
            it->private__.err_code = FDS_ERR_FORMAT;
            return it->private__.err_code;
        }

        if (it->private__.block_next + set_len > it->private__.list_end) {
            // The block exceeds the size of the lists
            it->private__.err_msg = err_msg[ERR_SET_EXCEED_LIST];
            it->private__.err_code = FDS_ERR_FORMAT;
            return it->private__.err_code;
        }

        it->private__.rec_next = &set_hdr->records[0];
        it->private__.block_next += set_len;
        assert(it->private__.rec_next <= it->private__.block_next);
        it->private__.tmplt = fds_tsnapshot_template_get(it->private__.snap, tmplt_id);
        it->tid = tmplt_id;

        if (it->private__.tmplt != NULL) {
            // Success
            break;
        }

        // Template not found...
        if ((it->private__.flags & FDS_STL_REPORT) != 0) {
            it->private__.rec_next = it->private__.block_next; // Skip the content
            return FDS_ERR_NOTFOUND;
        }

        // Continue -> try again
    }

    return FDS_OK;
}

int
fds_stmlist_iter_next_rec(struct fds_stmlist_iter *it)
{
    if (it->private__.err_code != FDS_OK) {
        return it->private__.err_code;
    }

    if (it->private__.rec_next >= it->private__.block_next) {
        // End of block has been reached
        return FDS_EOC;
    }

    uint8_t *rec_ptr = it->private__.rec_next;
    const struct fds_template *tmplt_ptr = it->private__.tmplt;
    uint16_t size = stl_rec_size(tmplt_ptr, rec_ptr, it->private__.block_next);
    if (size == 0) {
        // The next record in the block exceeds size of the block
        it->private__.err_msg = err_msg[ERR_REC_EXCEED_LIST];
        it->private__.err_code = FDS_ERR_FORMAT;
        return it->private__.err_code;
    }

    it->rec.data = rec_ptr;
    it->rec.size = size;
    it->rec.tmplt = it->private__.tmplt;
    it->rec.snap = it->private__.snap;

    it->private__.rec_next += size;
    assert(it->private__.rec_next <= it->private__.block_next);
    return FDS_OK;
}

const char *
fds_stmlist_iter_err(struct fds_stmlist_iter *it)
{
    return it->private__.err_msg;
}