/**
 * \file src/drec/iterator.c
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \brief Data record in the IPFIX message (source file)
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
#include <assert.h>
#include <arpa/inet.h> // ntohs

/** Information Element ID ("paddingOctets") for data padding */
#define IPFIX_PADDING_IE 210
/** IANA Private Enterprise Number for common forward fields */
#define IANA_PEN_FWD 0
/** IANA Private Enterprise Number for common reverse fields */
#define IANA_PEN_REV 29305


int
fds_drec_find(struct fds_drec *rec, uint32_t pen, uint16_t id, struct fds_drec_field *field)
{
    const uint16_t fields_cnt = rec->tmplt->fields_cnt_total;
    uint8_t *rec_start = rec->data;

    uint16_t offset = 0;
    uint16_t field_size;
    const struct fds_tfield *field_def;

    uint16_t idx;
    for (idx = 0; idx < fields_cnt; ++idx) {
        // Determine the start of the field and its real length
        field_def = &rec->tmplt->fields[idx];
        field_size = field_def->length;

        if (field_size == FDS_IPFIX_VAR_IE_LEN) {
            // This is field with variable length encoding -> read size from data
            field_size = rec_start[offset];
            offset++;

            if (field_size == 255U) {
                // Real size is on next 2 bytes
                field_size = ntohs(*(uint16_t *) &rec_start[offset]);
                offset += 2U;
            }
        }

        if (field_def->id == id && field_def->en == pen) {
            // Field found
            break;
        }

        offset += field_size;
    }

    if (idx >= fields_cnt) {
        // No more fields
        static_assert(FDS_EOC < 0, "Error codes must be always negative!");
        return FDS_EOC;
    }

    // We found required field
    field->data = &rec_start[offset];
    field->size = field_size;
    field->info = field_def;
    return idx;
}

void
fds_drec_iter_init(struct fds_drec_iter *iter, struct fds_drec *record, uint16_t flags)
{
    assert(iter != NULL);
    assert(record != NULL);

    static const uint16_t mask = FDS_DREC_BIFLOW_FWD | FDS_DREC_BIFLOW_REV;
    if ((record->tmplt->flags & FDS_TEMPLATE_BIFLOW) == 0) {
        // Disable direction flags
        flags &= ~mask;
    }

    // Both direction flags (forward + reverse) cannot be set together
    assert((flags & mask) != mask);

    iter->_private.rec = record;
    iter->_private.next_offset = 0;
    iter->_private.next_idx = 0;
    iter->_private.flags = flags;
    if ((flags & FDS_DREC_BIFLOW_REV) == 0) {
        // Use forward fields
        iter->_private.fields = record->tmplt->fields;
    } else {
        // Use reverse fields
        iter->_private.fields = record->tmplt->fields_rev;
        assert(iter->_private.fields != NULL);
    }
}

void
fds_drec_iter_rewind(struct fds_drec_iter *iter)
{
    iter->_private.next_offset = 0;
    iter->_private.next_idx = 0;
}

int
fds_drec_iter_next(struct fds_drec_iter *iter)
{
    const uint16_t fields_cnt = iter->_private.rec->tmplt->fields_cnt_total;
    uint8_t *rec_start = iter->_private.rec->data;

    uint32_t offset;
    uint16_t field_size;
    const struct fds_tfield *field_def;

    uint16_t idx;
    for (idx = iter->_private.next_idx; idx < fields_cnt; ++idx) {
        // Determine the start of the field and its real length
        offset = iter->_private.next_offset;
        field_def = &iter->_private.fields[idx];
        field_size = field_def->length;

        if (field_size == FDS_IPFIX_VAR_IE_LEN) {
            // This is field with variable length encoding -> read size from data
            field_size = rec_start[offset];
            offset++;

            if (field_size == 255U) {
                // Real size is on next 2 bytes
                field_size = ntohs(*(uint16_t *) &rec_start[offset]);
                offset += 2U;
            }
        }

        // Update info about the next record
        iter->_private.next_offset = offset + field_size;

        // Check padding field
        const uint16_t flags = iter->_private.flags;
        if ((flags & FDS_DREC_PADDING_SHOW) == 0 && field_def->id == IPFIX_PADDING_IE
                && (field_def->en == IANA_PEN_FWD || field_def->en == IANA_PEN_REV)) {
            // Skip this field
            continue;
        }

        // Check flags
        if (flags == 0) {
            break;
        }

        if ((flags & FDS_DREC_UNKNOWN_SKIP) && field_def->def == NULL) {
            // Skip fields with unknown definitions
            continue;
        }

        if ((flags & FDS_DREC_REVERSE_SKIP) && (field_def->flags & FDS_TFIELD_REVERSE)) {
            // Skip reverse fields
            continue;
        }

        break;
    }

    if (idx >= fields_cnt) {
        // No more fields
        static_assert(FDS_EOC < 0, "Error codes must be always negative!");
        iter->_private.next_idx = idx;
        return FDS_EOC;
    }

    // We found required field
    iter->_private.next_idx = idx + 1U;
    iter->field.data = &rec_start[offset];
    iter->field.size = field_size;
    iter->field.info = field_def;
    return idx;
}

int
fds_drec_iter_find(struct fds_drec_iter *iter, uint32_t pen, uint16_t id)
{
    const uint16_t fields_cnt = iter->_private.rec->tmplt->fields_cnt_total;
    uint8_t *rec_start = iter->_private.rec->data;

    uint16_t offset;
    uint16_t field_size;
    const struct fds_tfield *field_def;

    uint16_t idx;
    for (idx = iter->_private.next_idx; idx < fields_cnt; ++idx) {
        // Determine the start of the field and its real length
        offset = iter->_private.next_offset;
        field_def = &iter->_private.fields[idx];
        field_size = field_def->length;

        if (field_size == FDS_IPFIX_VAR_IE_LEN) {
            // This is field with variable length encoding -> read size from data
            field_size = rec_start[offset];
            offset++;

            if (field_size == 255U) {
                // Real size is on next 2 bytes
                field_size = ntohs(*(uint16_t *) &rec_start[offset]);
                offset += 2U;
            }
        }

        iter->_private.next_offset = offset + field_size;
        if (field_def->id == id && field_def->en == pen) {
            // Field found
            break;
        }
    }

    if (idx >= fields_cnt) {
        // No more fields
        static_assert(FDS_EOC < 0, "Error codes must be always negative!");
        iter->_private.next_idx = idx;
        return FDS_EOC;
    }

    // We found required field
    iter->_private.next_idx = idx + 1U;
    iter->field.data = &rec_start[offset];
    iter->field.size = field_size;
    iter->field.info = field_def;
    return idx;
}
