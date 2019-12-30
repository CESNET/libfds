/**
 * \file   src/template_mgr/template.c
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \brief  Template (source file)
 * \date   October 2017
 */
/*
 * Copyright (C) 2017 CESNET, z.s.p.o.
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

#include <stddef.h>    // size_t
#include <arpa/inet.h> // ntohs
#include <string.h>    // memcpy
#include <strings.h>   // strcasecmp
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include <libfds.h>

/**
 * Calculate size of template structure (based on number of fields)
 */
#define TEMPLATE_STRUCT_SIZE(elem_cnt) \
    sizeof(struct fds_template) \
    - sizeof(struct fds_tfield) \
    + ((elem_cnt) * sizeof(struct fds_tfield))

/** Return only first bit from a _value_ */
#define EN_BIT_GET(_value_)  ((_value_) & (uint16_t) 0x8000)
/** Return _value_ without the first bit */
#define EN_BIT_MASK(_value_) ((_value_) & (uint16_t )0x7FFF)
/** Get the length of an array */
#define ARRAY_SIZE(_arr_) (sizeof(_arr_) / sizeof((_arr_)[0]))

/** Required field identification            */
struct opts_req_id {
    uint16_t id; /**< Information Element ID */
    uint32_t en; /**< Enterprise Number      */
};

/**
 * \brief Check presence of required non-scope Information Elements (IEs)
 * \note All scope IEs are ignored!
 * \param[in] tmplt   Template structure
 * \param[in] recs    Array of required Information Elements
 * \param[in] rec_cnt Number of elements in the array
 * \return If all required IEs are present, the function will return true. Otherwise, returns false.
 */
static bool
opts_has_required(const struct fds_template *tmplt, const struct opts_req_id *recs,
    size_t rec_cnt)
{
    const uint16_t idx_start = tmplt->fields_cnt_scope;
    const uint16_t idx_end = tmplt->fields_cnt_total;

    // Try to find each required field
    for (size_t rec_idx = 0; rec_idx < rec_cnt; ++rec_idx) {
        const struct opts_req_id rec = recs[rec_idx];
        const struct fds_tfield *field_ptr = NULL;
        uint16_t field_idx;

        for (field_idx = idx_start; field_idx < idx_end; ++field_idx) {
            field_ptr = &tmplt->fields[field_idx];
            if (rec.id != field_ptr->id || rec.en != field_ptr->en) {
                continue;
            }

            break;
        }

        if (field_idx == idx_end) {
            // Required field not found!
            return false;
        }
    }

    return true;
}

/**
 * \brief Check presence of non-scope observation time interval
 *
 * The function will try to find any 2 "observationTimeXXX" Information Elements, where XXX is
 * one of following: Seconds, Milliseconds, Microseconds, Nanoseconds.
 * \note All scope IEs are ignored!
 * \param[in] tmplt Template structure
 * \return If the interval is present, returns true. Otherwise, returns false.
 */
static bool
opts_has_obs_time(const struct fds_template *tmplt)
{
    unsigned int matches = 0;

    const uint16_t fields_start = tmplt->fields_cnt_scope;
    const uint16_t fields_end = tmplt->fields_cnt_total;
    for (uint16_t i = fields_start; i < fields_end; ++i) {
        const struct fds_tfield *field_ptr = &tmplt->fields[i];
        if (field_ptr->en != 0) {
            continue;
        }

        /* We are looking for IEs observationTimeXXX with different precision
         * observationTimeSeconds (322) - observationTimeNanoseconds (325)
         */
        if (field_ptr->id < 322 || field_ptr->id > 325) {
            continue;
        }

        matches++;
        if (matches > 2) {
            // Too many matches
            return false;
        }
    }
    return (matches == 2);
}

/**
 * \brief Detect Options Template types of Metering Process
 *
 * If one or more types are detected, appropriate flag(s) will be set.
 * \note Based on RFC 7011, Section 4.1. - 4.2..
 * \param[in] tmplt Template structure
 */
static void
opts_detect_mproc(struct fds_template *tmplt)
{
    // Metering Process Template
    const uint16_t IPFIX_IE_ODID = 149; // observationDomainId
    const uint16_t IPFIX_IE_MPID = 143; // meteringProcessId
    const struct fds_tfield *odid_ptr = fds_template_find(tmplt, 0, IPFIX_IE_ODID);
    const struct fds_tfield *mpid_ptr = fds_template_find(tmplt, 0, IPFIX_IE_MPID);
    if (odid_ptr == NULL && mpid_ptr == NULL) {
        // At least one field must be defined
        return;
    }

    // Check scope fields
    const struct fds_tfield *ptrs[] = {odid_ptr, mpid_ptr};
    for (size_t i = 0; i < ARRAY_SIZE(ptrs); ++i) {
        const struct fds_tfield *ptr = ptrs[i];
        if (ptr == NULL) {
            // Item not found, skip
            continue;
        }

        if ((ptr->flags & FDS_TFIELD_SCOPE) == 0) {
            // The field found, but not in the scope!
            return;
        }

        if (ptr->flags & FDS_TFIELD_MULTI_IE) {
            // Multiple definitions are not expected!
            return;
        }
    }

    // Check non-scope fields
    static const struct opts_req_id ids_mproc[] = {
        {40, 0}, // exportedOctetTotalCount
        {41, 0}, // exportedMessageTotalCount
        {42, 0}  // exportedFlowRecordTotalCount
    };

    if (opts_has_required(tmplt, ids_mproc, ARRAY_SIZE(ids_mproc))) {
        // Ok, this is definitely "The Metering Process Statistics Options Template"
        tmplt->opts_types |= FDS_OPTS_MPROC_STAT;
    }

    static const struct opts_req_id ids_mproc_stat[] = {
        {164, 0}, // ignoredPacketTotalCount
        {165, 0}  // ignoredOctetTotalCount
    };
    if (!opts_has_required(tmplt, ids_mproc_stat, ARRAY_SIZE(ids_mproc_stat))) {
        // Required fields not found
        return;
    }

    if (opts_has_obs_time(tmplt)) {
        // Ok, this is definitely "The Metering Process Reliability Statistics Options Template"
        tmplt->opts_types |= FDS_OPTS_MPROC_RELIABILITY_STAT;
    }
}

/**
 * \brief Detect Options Template type of Exporting Process
 *
 * If the type is detected, an appropriate flag will be set.
 * \note Based on RFC 7011, Section 4.3.
 * \param[in] tmplt Template structure
 */
static void
opts_detect_eproc(struct fds_template *tmplt)
{
    const uint16_t IPFIX_IE_EXP_IPV4 = 130; // exporterIPv4Address
    const uint16_t IPFIX_IE_EXP_IPV6 = 131; // exporterIPv6Address
    const uint16_t IPFIX_IE_EXP_PID = 144;  // exportingProcessId

    // Check scope fields
    bool eid_found = false;
    const uint16_t eid[] = {IPFIX_IE_EXP_IPV4, IPFIX_IE_EXP_IPV6, IPFIX_IE_EXP_PID};
    for (size_t i = 0; i < ARRAY_SIZE(eid); ++i) {
        const struct fds_tfield *field_ptr = fds_template_find(tmplt, 0, eid[i]);
        if (!field_ptr) {
            // Not found
            continue;
        }

        if (field_ptr->flags & FDS_TFIELD_SCOPE && field_ptr->flags & FDS_TFIELD_LAST_IE) {
            eid_found = true;
            break;
        }
    }

    if (!eid_found) {
        return;
    }

    // Check non-scope fields
    static const struct opts_req_id ids_exp[] = {
        {166, 0}, // notSentFlowTotalCount
        {167, 0}, // notSentPacketTotalCount
        {168, 0}  // notSentOctetTotalCount
    };
    if (!opts_has_required(tmplt, ids_exp, ARRAY_SIZE(ids_exp))) {
        // Required fields not found
        return;
    }

    if (opts_has_obs_time(tmplt)) {
        // Ok, this is definitely "The Exporting Process Reliability Statistics Options Template"
        tmplt->opts_types |= FDS_OPTS_EPROC_RELIABILITY_STAT;
    }
}

/**
 * \brief Detect Options Template type of Flow keys
 *
 * If the type is detected, an appropriate flag will be set.
 * \note Based on RFC 7011, Section 4.4.
 * \param[in] tmplt Template structure
 */
static void
opts_detect_flowkey(struct fds_template *tmptl)
{
    // Check scope Field
    const uint16_t IPFIX_IE_TEMPLATE_ID = 145;
    const struct fds_tfield *id_ptr = fds_template_find(tmptl, 0, IPFIX_IE_TEMPLATE_ID);
    if (id_ptr == NULL) {
        // Not found
        return;
    }

    if ((id_ptr->flags & FDS_TFIELD_SCOPE) == 0 || id_ptr->flags & FDS_TFIELD_MULTI_IE) {
        // Not scope field or multiple definitions
        return;
    }

    // Check non-scope fields
    static const struct opts_req_id ids_key[] = {
        {173, 0} // flowKeyIndicator
    };
    if (opts_has_required(tmptl, ids_key, ARRAY_SIZE(ids_key))) {
        // Ok, this is definitely "The Flow Keys Options Template"
        tmptl->opts_types |= FDS_OPTS_FKEYS;
    }
}

/**
 * \brief Detect Options Template type of Information Element definition
 *
 * If the type is detected, an appropriate flag will be set.
 * \note Based on RFC 5610, Section 3.9.
 * \param[in] tmplt Template structure
 */
static void
opts_detect_ietype(struct fds_template *tmplt)
{
    const uint16_t FDS_IE_IE_ID = 303; // informationElementId
    const uint16_t FDS_IE_PEN = 346; // privateEnterpriseNumber
    const struct fds_tfield *ie_id_ptr = fds_template_find(tmplt, 0, FDS_IE_IE_ID);
    const struct fds_tfield *pen_ptr = fds_template_find(tmplt, 0, FDS_IE_PEN);

    // Check scope fields
    const struct fds_tfield *ptrs[] = {ie_id_ptr, pen_ptr};
    for (size_t i = 0; i < ARRAY_SIZE(ptrs); ++i) {
        const struct fds_tfield *ptr = ptrs[i];
        if (ptr == NULL) {
            // Required item not found
            return;
        }

        if ((ptr->flags & FDS_TFIELD_SCOPE) == 0) {
            // The field found, but not in the scope!
            return;
        }

        if (ptr->flags & FDS_TFIELD_MULTI_IE) {
            // Multiple definitions are not expected!
            return;
        }
    }

    // Mandatory non-scope fields
    static const struct opts_req_id ids_type[] = {
        {339, 0}, // informationElementDataType
        {344, 0}, // informationElementSemantics
        {341, 0}  // informationElementName
    };
    if (opts_has_required(tmplt, ids_type, ARRAY_SIZE(ids_type))) {
        // Ok, this is definitely "The Information Element Type Options Template"
        tmplt->opts_types |= FDS_OPTS_IE_TYPE;
    }
}

/**
 * \brief Detect all known types of Options Template and set appropriate flags.
 * \param[in] tmplt Template structure
 */
static void
opts_detector(struct fds_template *tmplt)
{
    assert(tmplt->type == FDS_TYPE_TEMPLATE_OPTS);

    opts_detect_mproc(tmplt);
    opts_detect_eproc(tmplt);
    opts_detect_flowkey(tmplt);
    opts_detect_ietype(tmplt);
}

/**
 * \brief Create an empty template structure
 * \note All parameters are set to zero.
 * \param[in] field_cnt Number of Field Specifiers
 * \return Point to the structure or NULL.
 */
static inline struct fds_template *
template_create_empty(uint16_t field_cnt)
{
    return calloc(1, TEMPLATE_STRUCT_SIZE(field_cnt));
}

/**
 * \brief Parse a raw template header and create a new template structure
 *
 * The new template structure will be prepared for adding appropriate number of Field Specifiers
 * based on information from the raw template.
 * \param[in]      type  Type of the template
 * \param[in]      ptr   Pointer to the template header
 * \param[in, out] len   [in] Maximal length of the raw template /
 *                       [out] real length of the header of the raw template (in octets)
 * \param[out]     tmplt New template structure
 * \return On success, the function will set the parameters \p len,\p tmplt and return #FDS_OK.
 *   Otherwise, the parameters will be unchanged and the function will return #FDS_ERR_FORMAT
 *   or #FDS_ERR_NOMEM.
 */
static int
template_parse_header(enum fds_template_type type, const void *ptr, uint16_t *len,
    struct fds_template **tmplt)
{
    assert(type == FDS_TYPE_TEMPLATE || type == FDS_TYPE_TEMPLATE_OPTS);
    const size_t size_normal = sizeof(struct fds_ipfix_trec) - sizeof(fds_ipfix_tmplt_ie);
    const size_t size_opts = sizeof(struct fds_ipfix_opts_trec) - sizeof(fds_ipfix_tmplt_ie);

    uint16_t template_id;
    uint16_t fields_total;
    uint16_t fields_scope = 0;
    uint16_t header_size = size_normal;

    if (*len < size_normal) { // the header must be at least 4 bytes long
        return FDS_ERR_FORMAT;
    }

    /*
     * Because Options Template header is superstructure of "Normal" Template header we can use it
     * also for parsing "Normal" Template. Just use only shared fields...
     */
    const struct fds_ipfix_opts_trec *rec = ptr;
    template_id = ntohs(rec->template_id);
    if (template_id < FDS_IPFIX_SET_MIN_DSET) {
        if (template_id != FDS_IPFIX_SET_TMPLT && template_id != FDS_IPFIX_SET_OPTS_TMPLT) {
            // Not All Options Template Withdrawal
            return FDS_ERR_FORMAT;
        }

        // Only All Options Templates Withdrawals
        if (ntohs(rec->count) != 0) {
            return FDS_ERR_FORMAT;
        }

        if (!(type == FDS_TYPE_TEMPLATE && template_id == FDS_IPFIX_SET_TMPLT)
            && !(type == FDS_TYPE_TEMPLATE_OPTS && template_id == FDS_IPFIX_SET_OPTS_TMPLT)) {
            // Types doesn't match...
            return FDS_ERR_FORMAT;
        }
    }

    fields_total = ntohs(rec->count);
    if (fields_total != 0 && type == FDS_TYPE_TEMPLATE_OPTS) {
        // It is not a withdrawal template, so it must be definitely an Options Template
        if (*len < size_opts) { // the header must be at least 6 bytes long
            return FDS_ERR_FORMAT;
        }

        header_size = size_opts;
        fields_scope = ntohs(rec->scope_field_count);
        if (fields_scope == 0 || fields_scope > fields_total) {
            return FDS_ERR_FORMAT;
        }
    }

    struct fds_template *tmplt_ptr = template_create_empty(fields_total);
    if (!tmplt_ptr) {
        return FDS_ERR_NOMEM;
    }

    tmplt_ptr->type = type;
    tmplt_ptr->id = template_id;
    tmplt_ptr->fields_cnt_total = fields_total;
    tmplt_ptr->fields_cnt_scope = fields_scope;
    *tmplt = tmplt_ptr;
    *len = header_size;
    return FDS_OK;
}

/**
 * \brief Parse Field Specifiers of a raw template
 *
 * Go through the Field Specifiers of the raw template and add them into the structure of the
 * parsed template \p tmplt.
 * \param[in]     tmplt     Template structure
 * \param[in]     field_ptr Pointer to the first specifier.
 * \param[in,out] len       [in] Maximal remaining length of the raw template /
 *                          [out] real length of the raw Field Specifiers (in octets).
 * \return On success, the function will set the parameter \p len and return #FDS_OK. Otherwise,
 *   the parameter will be unchanged and the function will return #FDS_ERR_FORMAT
 */
static int
template_parse_fields(struct fds_template *tmplt, const fds_ipfix_tmplt_ie *field_ptr,
    uint16_t *len)
{
    const uint16_t field_size = sizeof(fds_ipfix_tmplt_ie);
    const uint16_t fields_cnt = tmplt->fields_cnt_total;
    struct fds_tfield *tfield_ptr = &tmplt->fields[0];
    uint16_t len_remain = *len;

    for (uint16_t i = 0; i < fields_cnt; ++i, ++tfield_ptr, ++field_ptr, len_remain -= field_size) {
        // Parse Information Element ID
        if (len_remain < field_size) {
            // Unexpected end of the template
            return FDS_ERR_FORMAT;
        }

        tfield_ptr->length = ntohs(field_ptr->ie.length);
        tfield_ptr->id = ntohs(field_ptr->ie.id);
        if (EN_BIT_GET(tfield_ptr->id) == 0) {
            continue;
        }

        // Parse Enterprise Number
        len_remain -= field_size;
        if (len_remain < field_size) {
            // Unexpected end of the template
            return FDS_ERR_FORMAT;
        }

        ++field_ptr;
        tfield_ptr->id = EN_BIT_MASK(tfield_ptr->id);
        tfield_ptr->en = ntohl(field_ptr->enterprise_number);
    }

    *len -= len_remain;
    return FDS_OK;
}

/**
 * \brief Set feature flags of all Field Specifiers in a template
 *
 * Only ::FDS_TFIELD_SCOPE, ::FDS_TFIELD_MULTI_IE, and ::FDS_TFIELD_LAST_IE can be determined
 * based on a structure of the template. Other flags require external information.
 * \note Global flags of the template as a whole are not modified.
 * \param[in] tmplt Template structure
 */
static void
template_fields_calc_flags(struct fds_template *tmplt)
{
    const uint16_t fields_total = tmplt->fields_cnt_total;
    const uint16_t fields_scope = tmplt->fields_cnt_scope;

    // Label Scope fields
    for (uint16_t i = 0; i < fields_scope; ++i) {
        tmplt->fields[i].flags |= FDS_TFIELD_SCOPE;
    }

    // Label Multiple and Last fields
    uint64_t hash = 0;

    for (int i = fields_total - 1; i >= 0; --i) {
        struct fds_tfield *tfield_ptr = &tmplt->fields[i];

        // Calculate "hash" from IE ID
        uint64_t my_hash = (1ULL << (tfield_ptr->id % 64));
        if ((hash & my_hash) == 0) {
            // No one has the same "hash" -> this is definitely the last
            tfield_ptr->flags |= FDS_TFIELD_LAST_IE;
            hash |= my_hash;
            continue;
        }

        // Someone has the same hash. Let's check if there is exactly the same IE.
        bool same_found = false;
        for (int x = i + 1; x < fields_total; ++x) {
            struct fds_tfield *tfield_older = &tmplt->fields[x];
            if (tfield_ptr->id != tfield_older->id || tfield_ptr->en != tfield_older->en) {
                continue;
            }

            // Oh... we have a match
            tfield_ptr->flags |= FDS_TFIELD_MULTI_IE;
            tfield_older->flags |= FDS_TFIELD_MULTI_IE;
            same_found = true;
            break;
        }

        if (!same_found) {
            tfield_ptr->flags |= FDS_TFIELD_LAST_IE;
        }
    }
}

/**
 * \brief Calculate template parameters
 *
 * Feature flags of each Field Specifier will set as described in the documentation of the
 * template_fields_calc_flags() function. Regarding the global feature flags of the template,
 * only the features ::FDS_TEMPLATE_HAS_MULTI_IE and ::FDS_TEMPLATE_HAS_DYNAMIC of the template
 * will be detected and set. The expected length of appropriate data records will be calculated
 * based on the length of individual Specifiers.
 *
 * In case the template is Option Template, the function will also try to detect known type(s).
 * \param[in] tmplt Template structure
 * \return On success, returns #FDS_OK. If any parameter is not valid, the function will return
 *   #FDS_ERR_FORMAT.
 */
static int
template_calc_features(struct fds_template *tmplt)
{
    // First, calculate basic flags of each template field
    template_fields_calc_flags(tmplt);

    // Calculate flags of the whole template and each field offset in a data record
    const uint16_t fields_total = tmplt->fields_cnt_total;
    uint32_t data_len = 0; // Get (minimum) data length of a record referenced by this template
    uint16_t field_offset = 0;

    for (uint16_t i = 0; i < fields_total; ++i) {
        struct fds_tfield *field_ptr = &tmplt->fields[i];
        field_ptr->offset = field_offset;

        if (field_ptr->flags & FDS_TFIELD_MULTI_IE) {
            tmplt->flags |= FDS_TEMPLATE_MULTI_IE;
        }

        const uint16_t field_len = field_ptr->length;
        if (field_len == FDS_IPFIX_VAR_IE_LEN) {
            // Variable length Information Element must be at least 1 byte long
            tmplt->flags |= FDS_TEMPLATE_DYNAMIC;
            data_len += 1;
            field_offset = FDS_IPFIX_VAR_IE_LEN;
            continue;
        }

        data_len += field_len;
        if (field_offset != FDS_IPFIX_VAR_IE_LEN) {
            field_offset += field_len; // Overflow is resolved by check of total data length
        }
    }

    // Check if a record described by this templates fits into an IPFIX message
    const uint16_t max_rec_size = UINT16_MAX // Maximum length of an IPFIX message
        - sizeof(struct fds_ipfix_msg_hdr)   // IPFIX message header
        - sizeof(struct fds_ipfix_set_hdr);  // IPFIX set header
    if (max_rec_size < data_len) {
        // Too long data record
        return FDS_ERR_FORMAT;
    }

    // Recognize Options Template
    if (tmplt->type == FDS_TYPE_TEMPLATE_OPTS) {
        opts_detector(tmplt);
    }

    tmplt->data_length = data_len;
    return FDS_OK;
}

/**
 * \brief Create a copy of a raw template and assign the copy to a template structure
 * \param[in] tmplt Template structure
 * \param[in] ptr   Pointer to the raw template
 * \param[in] len   Real length of the raw template
 * \return #FDS_OK or #FDS_ERR_NOMEM
 */
static inline int
template_raw_copy(struct fds_template *tmplt, const void *ptr, uint16_t len)
{
    tmplt->raw.data = (uint8_t *) malloc(len);
    if (!tmplt->raw.data) {
        return FDS_ERR_NOMEM;
    }

    memcpy(tmplt->raw.data, ptr, len);
    tmplt->raw.length = len;
    return FDS_OK;
}

int
fds_template_parse(enum fds_template_type type, const void *ptr, uint16_t *len,
    struct fds_template **tmplt)
{
    assert(type == FDS_TYPE_TEMPLATE || type == FDS_TYPE_TEMPLATE_OPTS);
    struct fds_template *template;
    uint16_t len_header, len_fields, len_real;
    int ret_code;

    // Parse a header
    len_header = *len;
    ret_code = template_parse_header(type, ptr, &len_header, &template);
    if (ret_code != FDS_OK) {
        return ret_code;
    }

    if (template->fields_cnt_total == 0) {
        // No fields... just copy the raw template
        ret_code = template_raw_copy(template, ptr, len_header);
        if (ret_code != FDS_OK) {
            fds_template_destroy(template);
            return ret_code;
        }

        *len = len_header;
        *tmplt = template;
        return FDS_OK;
    }

    // Parse fields
    const fds_ipfix_tmplt_ie *fields_ptr = (fds_ipfix_tmplt_ie *)(((uint8_t *) ptr) + len_header);
    len_fields = *len - len_header;
    ret_code = template_parse_fields(template, fields_ptr, &len_fields);
    if (ret_code != FDS_OK) {
        fds_template_destroy(template);
        return ret_code;
    }

    // Copy raw template
    len_real = len_header + len_fields;
    ret_code = template_raw_copy(template, ptr, len_real);
    if (ret_code != FDS_OK) {
        fds_template_destroy(template);
        return ret_code;
    }

    // Calculate features of fields and the template
    ret_code = template_calc_features(template);
    if (ret_code != FDS_OK) {
        fds_template_destroy(template);
        return ret_code;
    }

    *len = len_real;
    *tmplt = template;
    return FDS_OK;
}

struct fds_template *
fds_template_copy(const struct fds_template *tmplt)
{
    const size_t size_main = TEMPLATE_STRUCT_SIZE(tmplt->fields_cnt_total);
    const size_t size_raw = tmplt->raw.length;
    const size_t size_rev = tmplt->fields_cnt_total * sizeof(*(tmplt->fields_rev));

    struct fds_template *cpy_main = malloc(size_main);
    uint8_t *cpy_raw = malloc(size_raw);
    struct fds_tfield *cpy_rev = (tmplt->fields_rev) ? malloc(size_rev) : NULL;
    if (!cpy_main || !cpy_raw || (tmplt->fields_rev && !cpy_rev)) {
        free(cpy_main);
        free(cpy_raw);
        free(cpy_rev);
        return NULL;
    }

    memcpy(cpy_main, tmplt, size_main);
    memcpy(cpy_raw, tmplt->raw.data, size_raw);
    if (tmplt->fields_rev) {
        memcpy(cpy_rev, tmplt->fields_rev, size_rev);
    }

    cpy_main->raw.data = cpy_raw;
    cpy_main->fields_rev = cpy_rev;
    return cpy_main;
}

void
fds_template_destroy(struct fds_template *tmplt)
{
    free(tmplt->raw.data);
    free(tmplt->fields_rev);
    free(tmplt);
}

/**
 * \brief Determine whether an Information Element is structured or not
 * \note Structured types are defined in RFC 6313
 * \param[in] elem Information Element
 * \return True or false
 */
static inline bool
is_structured(const struct fds_iemgr_elem *elem)
{
    switch (elem->data_type) {
    case FDS_ET_BASIC_LIST:
    case FDS_ET_SUB_TEMPLATE_LIST:
    case FDS_ET_SUB_TEMPLATE_MULTILIST:
        return true;
    default:
        return false;
    }
}

const struct fds_tfield *
fds_template_cfind(const struct fds_template *tmplt, uint32_t en, uint16_t id)
{
    const uint16_t field_cnt = tmplt->fields_cnt_total;
    for (uint16_t i = 0; i < field_cnt; ++i) {
        const struct fds_tfield *ptr = &tmplt->fields[i];
        if (ptr->id != id || ptr->en != en) {
            continue;
        }

        return ptr;
    }

    return NULL;
}

struct fds_tfield *
fds_template_find(struct fds_template *tmplt, uint32_t en, uint16_t id)
{
    return (struct fds_tfield *) fds_template_cfind(tmplt, en, id);
}

/**
 * \brief Convert Biflow Source IE ID to Biflow Destination IE ID and vice versa
 *
 * This function can be used to map Directional Key fields of Biflow records to opposite direction
 * fields. For example, source IPv4 address to destination IPv4 address and vice versa.
 * \param[in]     iemgr Manager of Information Elements
 * \param[in]     pen   Private Enterprise Number of an IE
 * \param[in,out] id    [in] Source resp. Destination IE / [out] Destination resp. Source ID
 * \return On success returns #FDS_OK and fills \p id. Otherwise returns #FDS_ERR_NOTFOUND and
 *   the parameter \p id is unchanged.
 */
static int
template_ies_biflow_src2dst(const fds_iemgr_t *iemgr, uint32_t pen, uint16_t *id)
{
    if (pen == 0) {
        // Table of standard conversions SRC <-> DST
        // URL: www.iana.org/assignments/ipfix/ipfix.xhtml
        static const struct pairs_struct {
            uint16_t src_id; // Source Field
            uint16_t dst_id; // Destination Field
        } pairs[] = {
            {  7,  11}, // sourceTransportPort            X destinationTransportPort
            {  8,  12}, // sourceIPv4Address              X destinationIPv4Address
            {  9,  13}, // sourceIPv4PrefixLength         X destinationIPv4PrefixLength
            { 10,  14}, // ingressInterface               X egressInterface
            { 16,  17}, // bgpSourceAsNumber              X bgpDestinationAsNumber
            { 27,  28}, // sourceIPv6Address              X destinationIPv6Address
            { 29,  30}, // sourceIPv6PrefixLength         X destinationIPv6PrefixLength
            { 44,  45}, // sourceIPv4Prefix               X destinationIPv4Prefix
            { 56,  80}, // sourceMacAddress               X destinationMacAddress
            { 58,  59}, // vlanId                         X postVlanId
            { 81,  57}, // postSourceMacAddress           X postDestinationMacAddress
            { 92,  93}, // srcTrafficIndex                X dstTrafficIndex
            {128, 129}, // bgpNextAdjacentAsNumber        X bgpPrevAdjacentAsNumber
            {170, 169}, // sourceIPv6Prefix               X destinationIPv6Prefix
            {180, 181}, // udpSourcePort                  X udpDestinationPort
            {182, 183}, // tcpSourcePort                  X tcpDestinationPort
            {225, 226}, // postNATSourceIPv4Address       X postNATDestinationIPv4Address
            {227, 228}, // postNAPTSourceTransportPort    X postNAPTDestinationTransportPort
            {234, 235}, // ingressVRFID                   X egressVRFID
            {281, 282}, // postNATSourceIPv6Address       X postNATDestinationIPv6Address
            // Note: (ingress/egress)(Unicast/Multicast/Broadcast)PacketTotalCount ignored
            {368, 369}, // ingressInterfaceType           X egressInterfaceType
            {414, 415}, // dot1qCustomerSourceMacAddress  X dot1qCustomerDestinationMacAddress
            // Note: sourceTransportPortsLimit doesn't have its counterpart
            {484, 485}, // bgpSourceCommunityList         X bgpDestinationCommunityList
            {487, 488}, // bgpSourceExtendedCommunityList X bgpDestinationExtendedCommunityList
            {490, 491}  // bgpSourceLargeCommunityList    X bgpDestinationLargeCommunityList
        };

        uint16_t new_id = 0;
        const size_t array_cnt = sizeof(pairs) / sizeof(pairs[0]);

        for (size_t i = 0; i < array_cnt; ++i) {
            struct pairs_struct pair = pairs[i];
            if (pair.src_id == *id) {
                new_id = pair.dst_id;
                break;
            }
            if (pair.dst_id == *id) {
                new_id = pair.src_id;
                break;
            }
        }

        if (new_id != 0) {
            *id = new_id;
            return FDS_OK;
        }
    }

    // Try to find it in the IE manager
    const struct fds_iemgr_elem *elem = fds_iemgr_elem_find_id(iemgr, pen, *id);
    if (!elem || !elem->name) {
        return FDS_ERR_NOTFOUND;
    }

    const char *str_src = "source";
    const char *str_dst = "destination";
    const size_t str_src_len = 6U;
    const size_t str_dst_len = 11U;

    const size_t buff_size = 256;
    char buff_data[buff_size];

    if (strncasecmp(elem->name, str_src, str_src_len) == 0) {
        // Find an IE that starts with "destination"
        strcpy(buff_data, str_dst);
        strncpy(buff_data + str_dst_len, elem->name + str_src_len, buff_size - str_dst_len);
    } else if (strncasecmp(elem->name, str_dst, str_dst_len) == 0) {
        // Find an IE that starts with "source"
        strcpy(buff_data, str_src);
        strncpy(buff_data + str_src_len, elem->name + str_dst_len, buff_size - str_src_len);
    } else {
        return FDS_ERR_NOTFOUND;
    }

    // Make sure that the string is always properly terminated
    buff_data[buff_size - 1] = '\0';

    // FIXME: add support for non-standard PENs (PEN is missing...)
    elem = fds_iemgr_elem_find_name(iemgr, buff_data);
    if (!elem) {
        return FDS_ERR_NOTFOUND;
    }

    *id = elem->id;
    assert(elem->scope->pen == pen);
    return FDS_OK;
}

/**
 * \brief Recalculate Biflow template fields
 *
 * If the template doesn't contain any reverse fields, the function does nothing. Otherwise
 * reverse template fields are created, if hasn't been defined earlier. For each template field
 * tries to determine whether it is a key or non-key field. A Biflow contains two non-key
 * fields for each value it represents associated with a single direction or endpoint: one for the
 * forward direction and one for the reverse direction. Key fields are shared by both directions
 * and will be labeled by #FDS_TFIEDL_BKEY flag. For more information, see RFC 5103.
 *
 * The function can be called multiple times on the template but fields processed earlier are
 * ignored.
 * \warning Function expects that template fields already have references to IE definitions.
 * \warning This function can be used only if at least one template field is reverse. Otherwise
 *   flags doesn't make sense.
 * \param[in] tmplt Template
 * \param[in] iemgr IE manager
 * \return On success returns #FDS_OK. Otherwise (memory allocation error) returns #FDS_ERR_NOMEM.
 */
static int
template_ies_biflow(struct fds_template *tmplt, const fds_iemgr_t *iemgr)
{
    if ((tmplt->flags & FDS_TEMPLATE_BIFLOW) == 0) {
        // Nothing to do...
        return FDS_OK;
    }

    // Create reverse template fields if required
    const uint16_t fields_cnt = tmplt->fields_cnt_total;
    if (!tmplt->fields_rev) {
        // Create reverse template fields
        const size_t fields_size = fields_cnt * sizeof(tmplt->fields[0]);
        tmplt->fields_rev = malloc(fields_size);
        if (!tmplt->fields_rev) {
            return FDS_ERR_NOMEM;
        }

        // Copy fields
        memcpy(tmplt->fields_rev, tmplt->fields, fields_size);

        // Remove all definitions, so we can recognized newly added definitions
        for (uint16_t i = 0; i < fields_cnt; ++i)  {
            tmplt->fields_rev[i].def = NULL;
        }
    }

    // Update reverse fields
    for (uint16_t i = 0; i < fields_cnt; ++i) {
        struct fds_tfield *field_fwd = &tmplt->fields[i];
        struct fds_tfield *field_rev = &tmplt->fields_rev[i];

        if (field_fwd->def == NULL) {
            assert(field_rev->def == NULL);
            // Definition is unknown...
            field_fwd->flags |= FDS_TFIELD_BKEY;
            field_rev->flags |= FDS_TFIELD_BKEY;
            continue;
        }

        if (field_rev->def != NULL) {
            // Already defined field
            continue;
        }

        // Newly defined field - common flags is set, if the field was previously unknown
        field_fwd->flags &= ~(fds_template_flag_t) FDS_TFIELD_BKEY;
        field_rev->flags &= ~(fds_template_flag_t) FDS_TFIELD_BKEY;

        // Only newly defined fields can go here
        assert(field_fwd->def != NULL && field_rev->def == NULL);
        const struct fds_iemgr_elem *def_rev = field_fwd->def->reverse_elem;

        if (field_fwd->def->is_reverse) {
            // Reverse only field detected
            assert((field_fwd->flags & FDS_TFIELD_REVERSE) != 0);
            assert(def_rev != NULL); // Reverse fields must have defined forward fields
            // Update "reverse" flag, IDs and the definition of the reverse template field
            field_rev->flags &= ~(fds_template_flag_t) FDS_TFIELD_REVERSE;
            field_rev->def = def_rev;
            field_rev->en = def_rev->scope->pen;
            field_rev->id = def_rev->id;
            continue;
        }

        if (def_rev && fds_template_cfind(tmplt, def_rev->scope->pen, def_rev->id) != NULL) {
            // Forward only field detected
            assert((field_fwd->flags & FDS_TFIELD_REVERSE) == 0);
            // Update "reverse" flag, IDs and the definition of the reverse template field
            field_rev->flags |= FDS_TFIELD_REVERSE;
            field_rev->def = def_rev;
            field_rev->en = def_rev->scope->pen;
            field_rev->id = def_rev->id;
            continue;
        }

        // Biflow Flow Keys only...
        field_fwd->flags |= FDS_TFIELD_BKEY;
        field_rev->flags |= FDS_TFIELD_BKEY;

        uint16_t new_id = field_fwd->id;
        if (template_ies_biflow_src2dst(iemgr, field_fwd->en, &new_id) == FDS_OK) {
            field_rev->id = new_id;
            // Find new definition
            field_rev->def = fds_iemgr_elem_find_id(iemgr, field_rev->en, new_id);
            continue;
        }

        // Non-directional Key field -> just copy reference to the definition
        field_rev->def = field_fwd->def;
    }

    return FDS_OK;
}

int
fds_template_ies_define(struct fds_template *tmplt, const fds_iemgr_t *iemgr, bool preserve)
{
    if (!iemgr && preserve) {
        // Nothing to do
        return FDS_OK;
    }

    if (!preserve && tmplt->fields_rev != NULL) {
        // Cleanup first
        free(tmplt->fields_rev);
        tmplt->fields_rev = NULL;
    }

    bool has_reverse = preserve ? (tmplt->flags & FDS_TEMPLATE_BIFLOW) != 0 : false;
    bool has_struct =  preserve ? (tmplt->flags & FDS_TEMPLATE_STRUCT) != 0 : false;
    // Ignore reverse (if preserve is enabled and the template is not already labeled as Biflow)
    bool ignore_rev = preserve && !has_reverse;

    // Find new definitions
    const uint16_t fields_cnt = tmplt->fields_cnt_total;

    for (uint16_t i = 0; i < fields_cnt; ++i) {
        struct fds_tfield *field_ptr = &tmplt->fields[i];
        if (preserve && field_ptr->def != NULL) {
            // Skip known fields
            continue;
        }

        // Clear previous flags
        const fds_template_flag_t field_mask = FDS_TFIELD_STRUCT | FDS_TFIELD_REVERSE
            | FDS_TFIELD_BKEY;
        field_ptr->flags &= ~field_mask;

        // Find new definition
        const struct fds_iemgr_elem *def_ptr;
        def_ptr = (!iemgr) ? NULL : fds_iemgr_elem_find_id(iemgr, field_ptr->en, field_ptr->id);
        if (ignore_rev && def_ptr != NULL && def_ptr->is_reverse) {
            def_ptr = NULL;
        }

        field_ptr->def = def_ptr;
        if (!def_ptr) {
            // Nothing to do...
            assert(!tmplt->fields_rev || tmplt->fields_rev[i].def == NULL);
            continue;
        }

        if (def_ptr->is_reverse) {
            field_ptr->flags |= FDS_TFIELD_REVERSE;
            has_reverse = true;
        }

        if (is_structured(def_ptr)) {
            field_ptr->flags |= FDS_TFIELD_STRUCT;
            has_struct = true;
        }
    }

    // Add/remove template flags
    tmplt->flags &= ~(fds_template_flag_t)(FDS_TEMPLATE_BIFLOW | FDS_TEMPLATE_STRUCT);
    if (has_reverse) {
        tmplt->flags |= FDS_TEMPLATE_BIFLOW;
    }
    if (has_struct) {
        tmplt->flags |= FDS_TEMPLATE_STRUCT;
    }

    return template_ies_biflow(tmplt, iemgr);
}

int
fds_template_flowkey_applicable(const struct fds_template *tmplt, uint64_t flowkey)
{
    // Get the highest bit and check the correctness
    unsigned int bit_highest = 0;
    uint64_t key_cpy = flowkey;
    while (key_cpy) {
        key_cpy >>= 1;
        bit_highest++;
    }

    const uint16_t fields_cnt = tmplt->fields_cnt_total;
    if (bit_highest > fields_cnt) {
        return FDS_ERR_FORMAT;
    }

    return FDS_OK;
}

int
fds_template_flowkey_define(struct fds_template *tmplt, uint64_t flowkey)
{
    int ret_code;
    if ((ret_code = fds_template_flowkey_applicable(tmplt, flowkey)) != FDS_OK) {
        return ret_code;
    }

    if (flowkey != 0) {
        // Set the global flow key flag
        tmplt->flags |= FDS_TEMPLATE_FKEY;
    } else {
        // Clear the global flow key flag
        tmplt->flags &= ~(FDS_TEMPLATE_FKEY);
    }

    // Set flow key flags
    bool biflow = (tmplt->fields_rev != NULL);
    const uint16_t fields_cnt = tmplt->fields_cnt_total;
    for (uint16_t i = 0; i < fields_cnt; ++i, flowkey >>= 1) {
        // Add flow key flags
        if (flowkey & 0x1) {
            tmplt->fields[i].flags |= FDS_TFIELD_FKEY;
            if (biflow) {
                tmplt->fields_rev[i].flags |= FDS_TFIELD_FKEY;
            }
        } else {
            tmplt->fields[i].flags &= ~(FDS_TFIELD_FKEY);
            if (biflow) {
                tmplt->fields_rev[i].flags &= ~(FDS_TFIELD_FKEY);
            }
        }
    }

    return FDS_OK;
}

int
fds_template_flowkey_cmp(const struct fds_template *tmplt, uint64_t flowkey)
{
    // Simple check
    bool value_exp, value_real;
    value_exp = flowkey != 0;
    value_real = (tmplt->flags & FDS_TEMPLATE_FKEY) != 0;

    if (value_exp == false && value_real == false) {
        return 0;
    } else if (value_exp != value_real) {
        return 1;
    }

    // Get the highest bit and check the correctness
    unsigned int bit_highest = 0;
    uint64_t key_cpy = flowkey;
    while (key_cpy) {
        key_cpy >>= 1;
        bit_highest++;
    }

    const uint16_t fields_cnt = tmplt->fields_cnt_total;
    if (bit_highest > fields_cnt) {
        return 1;
    }

    // Check flags
    for (uint16_t i = 0; i < fields_cnt; ++i, flowkey >>= 1) {
        value_exp = (flowkey & 0x1) != 0;
        value_real = (tmplt->fields[i].flags & FDS_TFIELD_FKEY) != 0;
        if (value_exp != value_real) {
            return 1;
        }
    }

    return 0;
}

int
fds_template_cmp(const struct fds_template *t1, const struct fds_template *t2)
{
    if (t1->raw.length != t2->raw.length) {
        return (t1->raw.length > t2->raw.length) ? 1 : (-1);
    }

    return memcmp(t1->raw.data, t2->raw.data, t1->raw.length);
}

