/**
 * \file src/drec/iterator.c
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \brief Data record in the IPFIX message (source file)
 * \date 2018-2020
 */

/* Copyright (C) 2018-2020 CESNET, z.s.p.o.
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

/**
 * \brief Get an index value for a given Information Element ID
 * \param[in] idx_array Index array
 * \param[in] id        Information Element ID
 * \return Value (can represent not-filled index)
 */
static inline uint8_t
index_get(const uint8_t *idx_array, uint16_t id)
{
    return idx_array[id % FDS_TEMPLATE_INDEX_SIZE];
}

/**
 * \brief Test if an index value is filled
 * \param[in] idx_val Index value
 * \return True/false
 */
static inline bool
index_is_valid(uint8_t idx_val)
{
    return (idx_val != FDS_TEMPLATE_INDEX_INV);
}

/**
 * \brief Test if an index value is used by multiple Template fields
 * \param[in] idx_val Index value
 * \return True/false
 */
static inline bool
index_is_multi(uint8_t idx_val)
{
    assert(index_is_valid(idx_val) && "Index MUST be valid!");
    return (idx_val & FDS_TEMPLATE_INDEX_FMULTI) != 0;
}

/**
 * \brief Get position of the Template definition for the given index value
 *
 * User MUST make sure that the particular Template field definition really
 * contains description of the required field (by checking IE ID and PEN).
 * \param[in] idx_val Index value
 * \return Position of the Template field definition
 * \return Negative value if a definition exists but it is out-of-range of index values.
 */
static inline int
index_tdef_pos(uint8_t idx_val)
{
    assert(index_is_valid(idx_val) && "Index MUST be valid!");
    uint8_t pos = idx_val & (~FDS_TEMPLATE_INDEX_FMULTI);
    if (pos == FDS_TEMPLATE_INDEX_RANGE) {
        return -1;
    }

    return pos;
}

/**
 * \brief Decode variable-length Data field
 *
 * If the Data field doesn't describe variable-length field (i.e. \p field_size is
 * NOT equal ::FDS_IPFIX_VAR_IE_LEN), then the function does nothing!
 *
 * However, if so, the real size is extracted from the Data field. The field pointer
 * and field size is then updated to represent correct values.
 *
 * \param[in,out] field_ptr  Pointer to the start of a Data field
 * \param[in,out] field_size Size of the given Data field
 */
static inline void
varlen_decode(uint8_t **field_ptr, uint16_t *field_size)
{
    if ((*field_size) != FDS_IPFIX_VAR_IE_LEN) {
        // Nothing to do
        return;
    }

    // Get the real size from the first byte
    (*field_size) = (**field_ptr);
    (*field_ptr)++;

    if (*field_size == 255U) {
        // Real size is on the next 2 bytes
        (*field_size) = ntohs(*(uint16_t *) (*field_ptr));
        (*field_ptr) += 2U;
    }
}

/**
 * \brief Get a field in a Data Record (start from a user provided hint)
 *
 * Try to find the first occurrence of the field defined by an Enterprise Number
 * and an Information Element ID in a Data Record.
 *
 * The hint is used to determine starting position for search. If the hint refers to
 * a Template field defintion without known offset from the beginning of the Data Record,
 * then it is ignored.
 *
 * \param[in]  rec   Pointer to the Data Record
 * \param[in]  pen   Private Enterprise Number
 * \param[in]  id    Information Element ID
 * \param[out] field Pointer to a variable where the result will be stored
 * \param[in]  hint  Template field from which searching should start
 *
 * \return
 *   If the field is present in the record, this function will fill \p field and return
 *   an index of the field in the record (the index starts from 0). Otherwise (the field is not
 *   present in the record) returns #FDS_EOC and the \p field is not filled.
 */
static int
find_with_hint(struct fds_drec *rec, uint32_t pen, uint16_t id,
    struct fds_drec_field *field, int hint)
{
    const struct fds_template *tmplt = rec->tmplt;
    const uint16_t fields_cnt = tmplt->fields_cnt_total;

    assert(hint < fields_cnt && "Invalid hint!");
    if (tmplt->fields[hint].offset == FDS_IPFIX_VAR_IE_LEN) {
        // Hint points to a field with unknown offset... we cannot use it
        hint = 0;
    }

    uint16_t offset = tmplt->fields[hint].offset;
    const struct fds_tfield *field_def;
    uint8_t *field_ptr = &rec->data[offset];
    uint16_t field_size;

    uint16_t idx;
    for (idx = hint; idx < fields_cnt; ++idx) {
        // Determine the start of the field and its real length
        field_def = &tmplt->fields[idx];
        field_size = field_def->length;
        varlen_decode(&field_ptr, &field_size);

        if (field_def->id == id && field_def->en == pen) {
            // Field found
            break;
        }

        field_ptr += field_size;
    }

    if (idx >= fields_cnt) {
        // No more fields
        static_assert(FDS_EOC < 0, "Error codes must be always negative!");
        return FDS_EOC;
    }

    // We found required field
    field->data = field_ptr;
    field->size = field_size;
    field->info = field_def;
    return idx;
}

int
fds_drec_find(struct fds_drec *rec, uint32_t pen, uint16_t id, struct fds_drec_field *field)
{
    const struct fds_template *tmplt = rec->tmplt;
    uint8_t *rec_start = rec->data;

    // Try to find the field using Template index
    const uint8_t index_val = index_get(tmplt->index, id);
    if (!index_is_valid(index_val)) {
        // Not found (i.e. the field doesn't exist in the record)
        return FDS_EOC;
    }

    int tdef_pos = index_tdef_pos(index_val);
    if (tdef_pos < 0) {
        // The field is available but it is out of index range
        return find_with_hint(rec, pen, id, field, 0);
    }

    assert(tdef_pos < (int) FDS_TEMPLATE_INDEX_RANGE && "Invalid value!");
    assert(tdef_pos < (int) tmplt->fields_cnt_total && "Out of range!");

    const struct fds_tfield *field_def = &tmplt->fields[tdef_pos];
    if (field_def->id != id || field_def->en != pen) {
        // The template field definition doesn't match
        if (!index_is_multi(index_val)) {
            // There are no more fields with this index i.e. the req. field doesn't exist
            return FDS_EOC;
        }

        // There are more fields with the given index
        return find_with_hint(rec, pen, id, field, tdef_pos);
    }

    // Match found!
    if (field_def->offset == FDS_IPFIX_VAR_IE_LEN) {
        // The field exists, but its placed after at least one variable length field
        return find_with_hint(rec, pen, id, field, 0);
    }

    uint8_t *field_ptr = &rec_start[field_def->offset];
    uint16_t field_size = field_def->length;
    varlen_decode(&field_ptr, &field_size);

    field->data = field_ptr;
    field->size = field_size;
    field->info = field_def;
    return tdef_pos;
}

//------------------------------------------------------------------------------

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
        assert(record->tmplt->rev_dir != NULL);
        iter->_private.fields = record->tmplt->rev_dir->fields;
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

    const struct fds_tfield *field_def;
    uint8_t *field_next_ptr = &rec_start[iter->_private.next_offset];
    uint8_t *field_ptr;
    uint16_t field_size;

    uint16_t idx;
    for (idx = iter->_private.next_idx; idx < fields_cnt; ++idx) {
        // Determine the start of the field and its real length
        field_def = &iter->_private.fields[idx];
        field_ptr = field_next_ptr;
        field_size = field_def->length;
        varlen_decode(&field_ptr, &field_size);

        field_next_ptr = field_ptr + field_size;

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

    // Update info about the next field
    ptrdiff_t new_offset = field_next_ptr - rec_start;
    assert(new_offset >= 0 && "Record offset cannot be negative!");
    assert(new_offset <= iter->_private.rec->size && "Out of range!");
    iter->_private.next_offset = (uint16_t) new_offset;

    if (idx >= fields_cnt) {
        // No more fields
        static_assert(FDS_EOC < 0, "Error codes must be always negative!");
        iter->_private.next_idx = idx;
        return FDS_EOC;
    }

    // We found required field
    iter->_private.next_idx = idx + 1U;
    iter->field.data = field_ptr;
    iter->field.size = field_size;
    iter->field.info = field_def;
    return idx;
}

/**
 * \brief Get a lookup hint for an Information Element
 *
 * For the given Information Element, the function will try to determine Template field
 * definition from which lookup should start. It is guaranteed that this field definition
 * has fixed offset from the start of the Data Record.
 *
 * \param[in] iter Pointer to the interator
 * \param[in] pen  Private Enterprise Number of IE
 * \param[in] id   Information Element ID
 *
 * \return A negative value if the IE doesn't exist in the record
 * \return Non-negative value (index) of Template field definition
 */
static int
iter_find_get_hint(const struct fds_drec_iter *iter, uint32_t pen, uint16_t id)
{
    const struct fds_drec *rec = iter->_private.rec;
    const struct fds_template *tmplt = rec->tmplt;

    const uint8_t *index_ptr = (iter->_private.flags & FDS_DREC_BIFLOW_REV) == 0
        ? tmplt->index : tmplt->rev_dir->index;
    const uint8_t index_val = index_get(index_ptr, id);
    if (!index_is_valid(index_val)) {
        // Not found (i.e. the field doesn't exist in the record)
        return -1;
    }

    int tdef_pos = index_tdef_pos(index_val);
    if (tdef_pos < 0) {
        // The field is probably present, but out of the index range
        return 0;
    }

    const struct fds_tfield *tfields = iter->_private.fields;
    const uint16_t tfields_cnt = tmplt->fields_cnt_total;
    assert(tdef_pos < (int) tfields_cnt && "Out of range!");

    const struct fds_tfield *tfield_def = &tfields[tdef_pos];
    if (tfield_def->id != id || tfield_def->en != pen) {
        // Not match, but...
        if (!index_is_multi(index_val)) {
            // There are no more fields (i.e. the field doesn't exist)
            return -1;
        }

        // Ok, let's then search from this field further (if possible)
    }

    if (tfield_def->offset == FDS_IPFIX_VAR_IE_LEN) {
        // The field probably exists, but it's placed after at least one var. len. field
        return 0;
    }

    return tdef_pos;
}

int
fds_drec_iter_find(struct fds_drec_iter *iter, uint32_t pen, uint16_t id)
{
    const struct fds_drec *rec = iter->_private.rec;
    const struct fds_template *tmplt = rec->tmplt;
    const uint16_t tfields_cnt = tmplt->fields_cnt_total;
    const struct fds_tfield *tfields = iter->_private.fields;
    uint16_t idx = iter->_private.next_idx;

    uint8_t *rec_start = rec->data;
    uint8_t *field_next_ptr;

    if (idx == 0) {
        /* After initialization, we can use Template index to jump the first
         * Template field definition that matches index of the required IE.
         * However, due to index collisions, it's possible that it is not
         * the required field!
         */
        int def_hint = iter_find_get_hint(iter, pen, id);
        if (def_hint < 0) {
            // Doesn't exist!
            iter->_private.next_idx = tfields_cnt;
            iter->_private.next_offset = rec->size;
            return FDS_EOC;
        }

        // Jump
        assert(def_hint < tfields_cnt && "Invalid hint!");
        uint16_t offset = tfields[def_hint].offset;
        assert(offset != FDS_IPFIX_VAR_IE_LEN && "Not a fixed offset!");

        field_next_ptr = &rec_start[offset];
        idx = (uint16_t) def_hint;
    } else {
        field_next_ptr = &rec_start[iter->_private.next_offset];
    }

    uint8_t *field_ptr;
    uint16_t field_size;
    const struct fds_tfield *field_def;

    for (; idx < tfields_cnt; ++idx) {
        // Determine the start of the field and its real length
        field_def = &tfields[idx];
        field_ptr = field_next_ptr;
        field_size = field_def->length;
        varlen_decode(&field_ptr, &field_size);

        field_next_ptr = field_ptr + field_size;
        if (field_def->id == id && field_def->en == pen) {
            // Field found
            break;
        }
    }

    // Update info about the next field
    ptrdiff_t new_offset = field_next_ptr - rec_start;
    assert(new_offset >= 0 && "Record offset cannot be negative!");
    assert(new_offset <= rec->size && "Out of range!");
    iter->_private.next_offset = (uint16_t) new_offset;

    if (idx >= tfields_cnt) {
        // No more fields
        static_assert(FDS_EOC < 0, "Error codes must be always negative!");
        iter->_private.next_idx = idx;
        return FDS_EOC;
    }

    // We found required field
    iter->_private.next_idx = idx + 1U;
    iter->field.data = field_ptr;
    iter->field.size = field_size;
    iter->field.info = field_def;
    return idx;
}
