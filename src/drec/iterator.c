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

/** Variable-length constant */  // TODO: replace
#define VAR_IE_LENGTH 65535

/** Information Element ID ("paddingOctets") for data padding */
#define IPFIX_PADDING_IE 210
/** IANA Private Enterprise Number for common forward fields */
#define IANA_PEN_FWD 0
/** IANA Private Enterprise Number for common reverse fields */
#define IANA_PEN_REV 29305


int
fds_drec_find(struct fds_drec *rec, uint32_t pen, uint16_t id, struct fds_drec_field *field)
{
    (void) rec;
    (void) pen;
    (void) id;
    (void) field;

    // TODO: implementovat
    return FDS_ERR_NOTFOUND;
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

    iter->internal.rec = record;
    iter->internal.next_offset = 0;
    iter->internal.next_idx = 0;
    iter->internal.flags = flags;
    if ((flags & FDS_DREC_BIFLOW_REV) == 0) {
        // Use forward fields
        iter->internal.fields = record->tmplt->fields;
    } else {
        // Use reverse fields
        iter->internal.fields = record->tmplt->fields_rev;
        assert(iter->internal.fields != NULL);
    }
}

void
fds_drec_iter_destroy(struct fds_drec_iter *iter)
{
    // Placeholder for the future modifications
    (void) iter;
}

void
fds_drec_iter_rewind(struct fds_drec_iter *iter)
{
    iter->internal.next_offset = 0;
    iter->internal.next_idx = 0;
}

int
fds_drec_iter_next(struct fds_drec_iter *iter)
{
    const uint16_t fields_cnt = iter->internal.rec->tmplt->fields_cnt_total;
    uint8_t *rec_start = iter->internal.rec->data;

    uint32_t offset;
    uint16_t field_size;
    const struct fds_tfield *field_def;

    uint16_t idx;
    for (idx = iter->internal.next_idx; idx < fields_cnt; ++idx) {
        // Determine the start of the field and its real length
        offset = iter->internal.next_offset;
        field_def = &iter->internal.fields[idx];
        field_size = field_def->length;

        if (field_size == VAR_IE_LENGTH) {
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
        iter->internal.next_offset = offset + field_size;
        // Check padding field
        if (field_def->id == IPFIX_PADDING_IE
                && (field_def->en == IANA_PEN_FWD || field_def->en == IANA_PEN_REV)) {
            // Skip this field
            continue;
        }

        // Check flags
        const uint16_t flags = iter->internal.flags;
        if (flags == 0) {
            break;
        }

        if ((flags & FDS_DREC_UNKNOWN_SKIP) && field_def->def == NULL) {
            // Skip fields with unknown definitions
            continue;
        }

        const fds_template_flag_t dir_flgs = FDS_DREC_BIFLOW_FWD | FDS_DREC_BIFLOW_REV;
        if ((flags & dir_flgs) != 0 && (field_def->flags & FDS_TFIELD_REVERSE) != 0) {
            // Skip reverse fields
            continue;
        }

        break;
    }

    if (idx >= fields_cnt) {
        // No more fields
        assert(FDS_ERR_NOTFOUND < 0);
        iter->internal.next_idx = idx;
        return FDS_ERR_NOTFOUND;
    }

    // We found required field
    iter->internal.next_idx = idx + 1;
    iter->field.data = &rec_start[offset];
    iter->field.size = field_size;
    iter->field.info = field_def;
    return idx;
}

int
fds_drec_iter_find(struct fds_drec_iter *iter, uint32_t pen, uint16_t id)
{
    const uint16_t fields_cnt = iter->internal.rec->tmplt->fields_cnt_total;
    uint8_t *rec_start = iter->internal.rec->data;

    uint16_t offset;
    uint16_t field_size;
    const struct fds_tfield *field_def;

    uint16_t idx;
    for (idx = iter->internal.next_idx; idx < fields_cnt; ++idx) {
        // Determine the start of the field and its real length
        offset = iter->internal.next_offset;
        field_def = &iter->internal.fields[idx];
        field_size = field_def->length;

        if (field_size == VAR_IE_LENGTH) {
            // This is field with variable length encoding -> read size from data
            field_size = rec_start[offset];
            offset++;

            if (field_size == 255U) {
                // Real size is on next 2 bytes
                field_size = ntohs(*(uint16_t *) &rec_start[offset]);
                offset += 2U;
            }
        }

        iter->internal.next_offset = offset + field_size;
        if (field_def->id == id && field_def->en == pen) {
            // Field found
            break;
        }

        // TODO: ignorovani forward a reverse polozek???
    }

    if (idx >= fields_cnt) {
        // No more fields
        assert(FDS_ERR_NOTFOUND < 0);
        iter->internal.next_idx = idx;
        return FDS_ERR_NOTFOUND;
    }

    // We found required field
    iter->internal.next_idx = idx + 1;
    iter->field.data = &rec_start[offset];
    iter->field.size = field_size;
    iter->field.info = field_def;
    return idx;
}
