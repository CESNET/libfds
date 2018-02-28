/**
 * \file src/parsers/ipfix_parser.c
 * \brief Simple parsers of an IPFIX Message (source file)
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

#include <inttypes.h>
#include <stdio.h>
#include <libfds.h>

/** Error code of IPFIX parsers */
enum error_codes {
    // No error
    ERR_OK,
    // IPFIX Sets iterator
    ERR_SETS_UNEXP_END,
    ERR_SETS_SET_SHORT,
    ERR_SETS_SET_LONG,
    // IPFIX Data record iterator
    ERR_DSET_VAR_LONG,
    // IPFIX Template record iterator
    ERR_TSET_WDRL_DEF,
    ERR_TSET_WDRL_ID,
    ERR_TSET_AW_ALONE,
    ERR_TSET_AW_LEN,
    ERR_TSET_AW_ID,
    ERR_TSET_DEF_SCNT,
    ERR_TSET_DEF_TID,
    ERR_TSET_DEF_CNT,
    ERR_TSET_DEF_END,
    ERR_TSET_DEF_DATA
};

/** Corresponding error messages */
static const char *err_msg[] = {
    [ERR_OK]             = "No error.",
    // IPFIX Sets iterator
    [ERR_SETS_UNEXP_END] = "The IPFIX Message size is invalid (unexpected end of the message).",
    [ERR_SETS_SET_SHORT] = "Total length of the Set is shorter than a length of an IPFIX Set "
        "header.",
    [ERR_SETS_SET_LONG]  = "Total length of the Set is longer than its enclosing IPFIX Message.",
    // IPFIX Data record iterator
    [ERR_DSET_VAR_LONG]  = "A variable-length Data Record is longer than its enclosing Data Set.",
    // IPFIX Template record iterator
    [ERR_TSET_WDRL_DEF]  = "An (Options) Template definition found within the (Options) Template "
        "Set Withdrawal (Field Count > 0).",
    [ERR_TSET_WDRL_ID]   = "Template ID of an (Options) Template Withdrawal is invalid (< 256).",
    [ERR_TSET_AW_ALONE]  = "All (Options) Template Withdrawal is not the only record in the Set.",
    [ERR_TSET_AW_LEN]    = "All (Options) Template Set Withdrawal has invalid length.",
    [ERR_TSET_AW_ID]     = "Template ID of All (Options) Template Set Withdrawal doesn't match "
        "its enclosing (Options) Template Set ID.",
    [ERR_TSET_DEF_SCNT]  = "Scope Field Count of an Options Template is zero.",
    [ERR_TSET_DEF_TID]   = "Template ID of an (Options) Template is invalid (< 256).",
    [ERR_TSET_DEF_CNT]   = "An (Options) Template Withdrawal found within the (Options) Template "
        "Set (Field Count = 0).",
    [ERR_TSET_DEF_END]   = "Invalid definition of an (Options) Template (unexpected end of the "
        "(Options) Template Set).",
    [ERR_TSET_DEF_DATA]  = "An (Options) Template defines a Data Record which length exceeds "
        "the maximum length of a Data Record that fits into an IPFIX Message."
};

void
fds_sets_iter_init(struct fds_sets_iter *it, struct fds_ipfix_msg_hdr *msg)
{
    it->_private.set_next = ((uint8_t *) msg) + FDS_IPFIX_MSG_HDR_LEN;
    it->_private.msg_end = ((uint8_t *) msg) + ntohs(msg->length);
    assert(it->_private.set_next <= it->_private.msg_end);
    it->_private.err_msg = err_msg[ERR_OK];
}

int
fds_sets_iter_next(struct fds_sets_iter *it)
{
    if (it->_private.set_next == it->_private.msg_end) {
        return FDS_ERR_NOTFOUND;
    }

    // Make sure that the iterator is not beyond the end of the message
    assert(it->_private.set_next < it->_private.msg_end);

    // Check the remaining length of the IPFIX message
    if (it->_private.set_next + FDS_IPFIX_SET_HDR_LEN > it->_private.msg_end) {
        // Unexpected end of the IPFIX Message
        it->_private.err_msg = err_msg[ERR_SETS_UNEXP_END];
        return FDS_ERR_FORMAT;
    }

    struct fds_ipfix_set_hdr *new_set = (struct fds_ipfix_set_hdr *) it->_private.set_next;
    uint16_t set_len = ntohs(new_set->length);

    // Check the length of the set
    if (set_len < FDS_IPFIX_SET_HDR_LEN) {
        // Length of a Set is shorter than an IPFIX Set header.
        it->_private.err_msg = err_msg[ERR_SETS_SET_SHORT];
        return FDS_ERR_FORMAT;
    }

    if (it->_private.set_next + set_len > it->_private.msg_end) {
        // Length of the Set is longer than its enclosing IPFIX Message
        it->_private.err_msg = err_msg[ERR_SETS_SET_LONG];
        return FDS_ERR_FORMAT;
    }

    it->_private.set_next += set_len;
    it->set = new_set;
    return FDS_OK;
}

const char *
fds_sets_iter_err(const struct fds_sets_iter *it)
{
    return it->_private.err_msg;
}

// -------------------------------------------------------------------------------------------------

void
fds_dset_iter_init(struct fds_dset_iter *it, struct fds_ipfix_set_hdr *set,
    const struct fds_template *tmplt)
{
    const uint16_t set_id = ntohs(set->flowset_id);
    const uint16_t set_len = ntohs(set->length);
    assert(set_id == tmplt->id);
    assert(set_id >= FDS_IPFIX_SET_MIN_DSET);
    assert(set_len >= FDS_IPFIX_SET_HDR_LEN);
    assert(tmplt->fields_cnt_total > 0);

    it->_private.tmplt = tmplt;
    it->_private.rec_next = ((uint8_t *) set) + FDS_IPFIX_SET_HDR_LEN;
    it->_private.set_end = ((uint8_t *) set) + set_len;
    it->_private.err_msg = err_msg[ERR_OK];
}

int
fds_dset_iter_next(struct fds_dset_iter *it)
{
    if (it->_private.rec_next == it->_private.set_end) {
        // End of the Set
        return FDS_ERR_NOTFOUND;
    }

    // Make sure that the iterator is not beyond the end of the message
    assert(it->_private.rec_next < it->_private.set_end);

    // Is the rest of the message padding?
    const struct fds_template *tmplt = it->_private.tmplt;
    uint32_t size = tmplt->data_length;
    if (it->_private.rec_next + size > it->_private.set_end) {
        // The rest of the Data Set is padding
        return FDS_ERR_NOTFOUND;
    }

    if ((tmplt->flags & FDS_TEMPLATE_DYNAMIC) == 0) {
        // Processing a static record
        it->rec = it->_private.rec_next;
        it->size = (uint16_t) size;
        it->_private.rec_next += size;
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
        if (&it->_private.rec_next[size + 1] > it->_private.set_end) {
            // The memory is beyond the end of the Data Set
            break;
        }


        field_size = it->_private.rec_next[size];
        size += 1U;
        if (field_size != 255U) {
            size += field_size;
            continue;
        }

        if (&it->_private.rec_next[size + 2] > it->_private.set_end) {
            // The memory is beyond the end of the Data Set
            break;
        }

        field_size += ntohs(*(uint16_t *) &it->_private.rec_next[size]);
        size += 2U + field_size;
    }

    if (idx != tmplt->fields_cnt_total || &it->_private.rec_next[size] > it->_private.set_end) {
        // A variable-length Data Record is longer than its enclosing Data Set.
        it->_private.err_msg = err_msg[ERR_DSET_VAR_LONG];
        return FDS_ERR_FORMAT;
    }

    it->rec = it->_private.rec_next;
    it->size = (uint16_t) size;
    it->_private.rec_next += size;
    return FDS_OK;
}

const char *
fds_dset_iter_err(const struct fds_dset_iter *it)
{
    return it->_private.err_msg;
}

// -------------------------------------------------------------------------------------------------

void
fds_tset_iter_init(struct fds_tset_iter *it, struct fds_ipfix_set_hdr *set)
{
    const uint16_t set_id = ntohs(set->flowset_id);
    const uint16_t set_len = ntohs(set->length);

    assert(set_id == FDS_IPFIX_SET_TMPLT || set_id == FDS_IPFIX_SET_OPTS_TMPLT);
    assert(set_len >= FDS_IPFIX_SET_HDR_LEN);

    it->_private.type = set_id;
    it->_private.rec_next  = ((uint8_t *) set) + FDS_IPFIX_SET_HDR_LEN;
    it->_private.set_begin = set;
    it->_private.set_end   = ((uint8_t *) set) + set_len;
    it->_private.err_msg   = err_msg[ERR_OK];

    if (set_len <  FDS_IPFIX_SET_HDR_LEN + FDS_IPFIX_WDRL_TREC_LEN) {
        return;
    }

    if (ntohs(((struct fds_ipfix_wdrl_trec *) it->_private.rec_next)->count) == 0) {
        it->_private.type = 0; // Accept only (All) (Options) Template Withdrawal
    }
}

/**
 * \brief Parse (All) Template or Options Template Withdrawals
 * \param[in,out] it Template Set iterator
 * \return #FDS_OK on success. #FDS_ERR_NOTFOUND if the end of the set has been reached.
 *   #FDS_ERR_FORMAT if an error has occurred and a corresponding error message is set.
 */
static inline int
fds_tset_iter_withdrawals(struct fds_tset_iter *it)
{
    assert(it->_private.type == 0);
    if (it->_private.rec_next + FDS_IPFIX_WDRL_TREC_LEN > it->_private.set_end) {
        // Skip padding
        return FDS_ERR_NOTFOUND;
    }

    struct fds_ipfix_wdrl_trec *rec = (struct fds_ipfix_wdrl_trec *) it->_private.rec_next;
    if (ntohs(rec->count) != 0) {
        // Found an (Options) Template definition among (Options) Template Withdrawals
        it->_private.err_msg = err_msg[ERR_TSET_WDRL_DEF];
        return FDS_ERR_FORMAT;
    }

    const uint16_t tid = ntohs(rec->template_id);
    if (tid == FDS_IPFIX_SET_TMPLT || tid == FDS_IPFIX_SET_OPTS_TMPLT) {
        // All (Options) Templates Withdrawal
        if (((uint8_t *) it->_private.set_begin) + FDS_IPFIX_SET_HDR_LEN != it->_private.rec_next) {
            // All (Options) Template Withdrawal Set must be only record in the Set
            it->_private.err_msg = err_msg[ERR_TSET_AW_ALONE];
            return FDS_ERR_FORMAT;
        }

        if (ntohs(it->_private.set_begin->length) != FDS_IPFIX_WDRL_ALLSET_LEN) {
            // All (Options) Template Withdrawal Set must be 8 bytes long
            it->_private.err_msg = err_msg[ERR_TSET_AW_LEN];
            return FDS_ERR_FORMAT;
        }

        if (ntohs(it->_private.set_begin->flowset_id) != tid) {
            // Template Withdrawal ID doesn't match enclosing Set ID
            it->_private.err_msg = err_msg[ERR_TSET_AW_ID];
            return FDS_ERR_FORMAT;
        }
    } else if (tid < FDS_IPFIX_SET_MIN_DSET) {
        // Invalid Template Withdrawal ID
        it->_private.err_msg = err_msg[ERR_TSET_WDRL_ID];
        return FDS_ERR_FORMAT;
    }

    it->ptr.wdrl_trec = rec;
    it->size = FDS_IPFIX_WDRL_TREC_LEN;
    it->field_cnt = 0;
    it->scope_cnt = 0;
    it->_private.rec_next += FDS_IPFIX_WDRL_TREC_LEN;
    return FDS_OK;
}

/**
 * \brief Parse Template or Options Template definition
 * \param[in,out] it Template Set iterator
 * \return #FDS_OK on success. #FDS_ERR_NOTFOUND if the end of the set has been reached.
 *   #FDS_ERR_FORMAT if an error has occurred and a corresponding error message is set.
 */
static inline int
fds_tset_iter_definitions(struct fds_tset_iter *it)
{
    const uint16_t type = it->_private.type;
    assert(type == FDS_IPFIX_SET_TMPLT || type == FDS_IPFIX_SET_OPTS_TMPLT);

    // Minimal size of a record a is template with just one field (4B + 4B, resp. 6B + 4B)
    const uint16_t min_size = (type == FDS_IPFIX_SET_TMPLT) ? 8U : 10U;
    if (it->_private.rec_next + min_size > it->_private.set_end) {
        // Skip padding
        return FDS_ERR_NOTFOUND;
    }

    uint16_t tmplt_id;
    uint16_t field_cnt;
    uint16_t scope_cnt = 0;
    uint32_t data_size = 0;
    const fds_ipfix_tmplt_ie *field_ptr;

    if (type == FDS_IPFIX_SET_TMPLT) {
        // Template
        struct fds_ipfix_trec *rec = (struct fds_ipfix_trec *) it->_private.rec_next;
        tmplt_id  = ntohs(rec->template_id);
        field_cnt = ntohs(rec->count);
        field_ptr = &rec->fields[0];
    } else {
        // Options Template
        struct fds_ipfix_opts_trec *rec = (struct fds_ipfix_opts_trec *) it->_private.rec_next;
        tmplt_id  = ntohs(rec->template_id);
        field_cnt = ntohs(rec->count);
        scope_cnt = ntohs(rec->scope_field_count);
        field_ptr = &rec->fields[0];

        if (scope_cnt == 0) {
            // Scope count cannot be zero
            it->_private.err_msg = err_msg[ERR_TSET_DEF_SCNT];
            return FDS_ERR_FORMAT;
        }
    }

    if (tmplt_id < FDS_IPFIX_SET_MIN_DSET) {
        // Invalid Template ID
        it->_private.err_msg = err_msg[ERR_TSET_DEF_TID];
        return FDS_ERR_FORMAT;
    }

    if (field_cnt == 0) {
        // This is a (Options) Template Withdrawal
        it->_private.err_msg = err_msg[ERR_TSET_DEF_CNT];
        return FDS_ERR_FORMAT;
    }

    const uint16_t field_size = sizeof(fds_ipfix_tmplt_ie);
    for (uint16_t i = 0; i < field_cnt; ++i, ++field_ptr) {
        // Parse Information Element ID
        if (((uint8_t *) field_ptr) + field_size > it->_private.set_end) {
            // Unexpected end of the Set
            it->_private.err_msg = err_msg[ERR_TSET_DEF_END];
            return FDS_ERR_FORMAT;
        }

        uint16_t field_id  = ntohs(field_ptr->ie.id);
        uint16_t field_len = ntohs(field_ptr->ie.length);
        if (field_len == FDS_IPFIX_VAR_IE_LEN) {
            field_len = 1; // Dynamic field is long at least 1 byte
        }

        data_size += field_len;
        if ((field_id & (uint16_t) 0x8000) == 0) {
            continue;
        }

        // Parse Enterprise Number
        ++field_ptr;
        if (((uint8_t *) field_ptr) + field_size > it->_private.set_end) {
            // Unexpected end of the Set
            it->_private.err_msg = err_msg[ERR_TSET_DEF_END];
            return FDS_ERR_FORMAT;
        }
        // Ignore the content of the field...
    }

    // Maximum size of a data record (Max. length - IPFIX Message header - IPFIX Set header)
    const size_t data_max =
        UINT16_MAX - sizeof(struct fds_ipfix_msg_hdr) - sizeof(struct fds_ipfix_set_hdr);
    if (data_size > data_max) {
        it->_private.err_msg = err_msg[ERR_TSET_DEF_DATA];
        return FDS_ERR_FORMAT;
    }

    // Everything seems ok...
    if (scope_cnt == 0) {
        it->ptr.trec = (struct fds_ipfix_trec *) it->_private.rec_next;
    } else {
        it->ptr.opts_trec = (struct fds_ipfix_opts_trec *) it->_private.rec_next;
    }

    const uint16_t tmplt_size = ((uint8_t *) field_ptr) - it->_private.rec_next;
    it->size = tmplt_size;
    it->field_cnt = field_cnt;
    it->scope_cnt = scope_cnt;
    it->_private.rec_next += tmplt_size;
    return FDS_OK;
}

int
fds_tset_iter_next(struct fds_tset_iter *it)
{
    if (it->_private.rec_next == it->_private.set_end) {
        // End of the Set
        return FDS_ERR_NOTFOUND;
    }

    // Make sure that the iterator is not beyond the end of the message
    assert(it->_private.rec_next < it->_private.set_end);

    if (it->_private.type == 0) {
        // (All) (Options) Template Withdrawal
        return fds_tset_iter_withdrawals(it);
    } else {
        // (Options) Template definitions
        return fds_tset_iter_definitions(it);
    }
}

const char *
fds_tset_iter_err(const struct fds_tset_iter *it)
{
    return it->_private.err_msg;
}
