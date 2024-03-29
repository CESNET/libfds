/**
 * \file src/ipfix_filter.c
 * \author Michal Sedlak <xsedla0v@stud.fit.vutbr.cz>
 * \brief Specialization of the general filtering component for working with IPFIX data
 * \date 2020
 */

/*
 * Copyright (C) 2020 CESNET, z.s.p.o.
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

#include <assert.h>
#include <limits.h>
#include <libfds.h>
#include "../filter/error.h"

enum ipxfil_lookup_kind {
    IPXFIL_ALIAS_LOOKUP,
    IPXFIL_FIELD_LOOKUP,
    IPXFIL_CONST_LOOKUP
};

struct ipxfil_lookup_item {
    const char *name;
    int filter_data_type;
    enum ipxfil_lookup_kind kind;
    union {
        const struct fds_iemgr_elem *elem;
        const struct fds_iemgr_alias *alias;
        int64_t constant;
    };
};

struct ipxfil_lookup_table {
    size_t cnt;
    struct ipxfil_lookup_item *items;
};

struct ipxfil_lookup_state {
    size_t source_idx;
    uint16_t find_flags; // 0, FDS_DREC_BIFLOW_FWD or FDS_DREC_BIFLOW_REV
};

struct fds_ipfix_filter {
    struct fds_filter_error *error;

    fds_filter_t *filter;
    const fds_iemgr_t *iemgr;

    struct ipxfil_lookup_table lookup_tab;
    struct ipxfil_lookup_state lookup_state;
};

/**
 * Same as fds_drec_find, but with flags (see fds_drec_iter_init documentation).
 */
static int
fds_drec_find_with_flags(struct fds_drec *drec, uint32_t pen, uint16_t id, uint16_t flags, struct fds_drec_field *field)
{
    if (flags == 0) {
        return fds_drec_find(drec, pen, id, field);

    } else {
        struct fds_drec_iter iter;
        fds_drec_iter_init(&iter, drec, flags);

        int ret = fds_drec_iter_find(&iter, pen, id);
        if (ret != FDS_EOC) {
            *field = iter.field;
        }
        return ret;
    }
}

/**
 * Calculate index of a lookup item in a lookup table
 */
static size_t
get_item_index(struct ipxfil_lookup_table *tab, struct ipxfil_lookup_item *item)
{
    assert(item >= tab->items);
    assert(item <= &tab->items[tab->cnt - 1]);

    return item - tab->items;
}

/**
 * Add empty lookup item to the lookup table and return pointer to it
 */
static struct ipxfil_lookup_item *
add_lookup_item(struct ipxfil_lookup_table *tab)
{
    void *tmp = realloc(tab->items, (tab->cnt + 1) * sizeof(struct ipxfil_lookup_item));
    if (tmp == NULL) {
        return NULL;
    }
    tab->cnt++;
    tab->items = tmp;
    return &tab->items[tab->cnt - 1];
}

/**
 * Find lookup item in the lookup table by its name
 */
struct ipxfil_lookup_item *
find_lookup_item_by_name(struct ipxfil_lookup_table *tab, const char *name)
{
    for (size_t i = 0; i < tab->cnt; i++) {
        if (strcmp(name, tab->items[i].name) == 0) {
            return &tab->items[i];
        }
    }
    return NULL;
}

/**
 * Get the filter data type corresponding to the provided IPFIX data type
 */
static inline int
get_filter_data_type(const struct fds_iemgr_elem *elem)
{
    switch (elem->data_type) {
    case FDS_ET_UNSIGNED_8:
    case FDS_ET_UNSIGNED_16:
    case FDS_ET_UNSIGNED_32:
    case FDS_ET_UNSIGNED_64:
        if (elem->data_semantic == FDS_ES_FLAGS) {
            return FDS_FDT_FLAGS;
        }
        return FDS_FDT_UINT;

    case FDS_ET_SIGNED_8:
    case FDS_ET_SIGNED_16:
    case FDS_ET_SIGNED_32:
    case FDS_ET_SIGNED_64:
        return FDS_FDT_INT;

    case FDS_ET_BOOLEAN:
        return FDS_FDT_BOOL;

    case FDS_ET_IPV4_ADDRESS:
    case FDS_ET_IPV6_ADDRESS:
        return FDS_FDT_IP;

    case FDS_ET_MAC_ADDRESS:
        return FDS_FDT_MAC;

    case FDS_ET_STRING:
    case FDS_ET_OCTET_ARRAY:
        return FDS_FDT_STR;

    case FDS_ET_DATE_TIME_SECONDS:
    case FDS_ET_DATE_TIME_MILLISECONDS:
    case FDS_ET_DATE_TIME_MICROSECONDS:
    case FDS_ET_DATE_TIME_NANOSECONDS:
        return FDS_FDT_UINT;

    default:
        assert(0 && "not implemented");
    }
}

/**
 * Get the filter data type corresponding to the alias
 */
static int
get_filter_data_type_for_alias(const struct fds_iemgr_alias *alias)
{
    int data_type = get_filter_data_type(alias->sources[0]);
    for (size_t i = 1; i < alias->sources_cnt; i++) {
        if (get_filter_data_type(alias->sources[i]) != data_type) {
            return FDS_FDT_NONE;
        }
    }
    return data_type;
}

/**
 * The filter lookup callback - maps filter var names to their ids and data types
 */
static int
lookup_callback(void *user_ctx, const char *name, const char *other_name,
                int *out_id, int *out_data_type, int *out_flags)
{
    struct fds_ipfix_filter *ipxfil = user_ctx;

    // check if the name wasn't looked up already, in that case
    // the entry is already in the lookup table and we don't have to create new one
    struct ipxfil_lookup_item *item = find_lookup_item_by_name(&ipxfil->lookup_tab, name);
    if (item != NULL) {
        *out_data_type = item->filter_data_type;
        *out_id = get_item_index(&ipxfil->lookup_tab, item);
        return FDS_OK;
    }

    // check if the name is a alias, add it to the lookup table
    const struct fds_iemgr_alias *alias = fds_iemgr_alias_find(ipxfil->iemgr, name);
    if (alias != NULL) {
        struct ipxfil_lookup_item *item = add_lookup_item(&ipxfil->lookup_tab);
        if (item == NULL) {
            ipxfil->error = MEMORY_ERROR;
            return ipxfil->error->code;
        }
        item->filter_data_type = get_filter_data_type_for_alias(alias);
        if (item->filter_data_type == FDS_FDT_NONE) {
            ipxfil->error = error_create(FDS_ERR_ARG,
                "Alias `%s`: cannot map all source data types to the same filter data type", name);
            return ipxfil->error->code;
        }
        item->name = name;
        item->kind = IPXFIL_ALIAS_LOOKUP;
        item->alias = alias;
        *out_data_type = item->filter_data_type;
        *out_id = get_item_index(&ipxfil->lookup_tab, item);
        return FDS_OK;
    }

    // check if the name is an element, add it to the lookup table
    const struct fds_iemgr_elem *elem = fds_iemgr_elem_find_name(ipxfil->iemgr, name);
    if (elem != NULL) {
        struct ipxfil_lookup_item *item = add_lookup_item(&ipxfil->lookup_tab);
        if (item == NULL) {
            ipxfil->error = MEMORY_ERROR;
            return ipxfil->error->code;
        }
        item->filter_data_type = get_filter_data_type(elem);
        if (item->filter_data_type == FDS_FDT_NONE) {
            ipxfil->error = error_create(FDS_ERR_ARG, "Field `%s`: unsupported data type", name);
            return ipxfil->error->code;
        }
        item->name = name;
        item->kind = IPXFIL_FIELD_LOOKUP;
        item->elem = elem;
        *out_data_type = item->filter_data_type;
        *out_id = get_item_index(&ipxfil->lookup_tab, item);
        return FDS_OK;
    }

    // check if the name is a constant
    if (other_name != NULL) {
        const struct fds_iemgr_mapping_item *mapping = fds_iemgr_mapping_find(ipxfil->iemgr, other_name, name);
        if (mapping != NULL) {
            struct ipxfil_lookup_item *item = add_lookup_item(&ipxfil->lookup_tab);
            if (item == NULL) {
                ipxfil->error = MEMORY_ERROR;
                return ipxfil->error->code;
            }
            item->name = name;
            item->kind = IPXFIL_CONST_LOOKUP;
            item->constant = mapping->value.i;
            item->filter_data_type = FDS_FDT_INT;
            *out_flags |= FDS_FILTER_FLAG_CONST;
            *out_data_type = item->filter_data_type;
            *out_id = get_item_index(&ipxfil->lookup_tab, item);
            return FDS_OK;
        }
    }

    return FDS_ERR_NOTFOUND;
}

/**
 * The filter const callback
 */
static void
const_callback(void *user_ctx, int id, fds_filter_value_u *out_value)
{
    struct fds_ipfix_filter *ipxfil = user_ctx;
    assert(id >= 0 && ipxfil->lookup_tab.cnt <= INT_MAX && id < (int) ipxfil->lookup_tab.cnt);
    struct ipxfil_lookup_item *item = &ipxfil->lookup_tab.items[id];
    assert(item->kind == IPXFIL_CONST_LOOKUP);
    out_value->i = item->constant;
}

/**
 * Set default value for filter value
 */
static void
set_default_value(fds_filter_value_u *out_value)
{
    memset(out_value, 0, sizeof(fds_filter_value_u));
}

/**
 * Try to read the desired field from a record into a filter value
 */
static int
read_record_field(struct fds_drec *record, const struct fds_iemgr_elem *field_def, uint16_t find_flags,
                  fds_filter_value_u *out_value)
{
    struct fds_drec_field field;

    // The wanted field does not exist in the record
    if (fds_drec_find_with_flags(record, field_def->scope->pen, field_def->id, find_flags, &field) == FDS_EOC) {
        return FDS_ERR_NOTFOUND;
    }

    uint8_t *data = field.data;
    uint16_t size = field.size;

    int rc;
    switch (field_def->data_type) {
        case FDS_ET_UNSIGNED_8:
        case FDS_ET_UNSIGNED_16:
        case FDS_ET_UNSIGNED_32:
        case FDS_ET_UNSIGNED_64:
            //if (field_def->data_semantic == FDS_ES_FLAGS) {
            //    assert(size == 1 || size == 2);
            //    if (size == 1) {
            //        out_value->u = *data;
            //    } else {
            //        out_value->u = ntohs(*(uint16_t*)(data));
            //    }
            //    //} else {
            rc = fds_get_uint_be(data, size, &out_value->u);
            assert(rc == FDS_OK);
            //}
            break;

        case FDS_ET_SIGNED_8:
        case FDS_ET_SIGNED_16:
        case FDS_ET_SIGNED_32:
        case FDS_ET_SIGNED_64:
            rc = fds_get_int_be(data, size, &out_value->i);
            assert(rc == FDS_OK);
            break;

        case FDS_ET_FLOAT_32:
        case FDS_ET_FLOAT_64:
            rc = fds_get_float_be(data, size, &out_value->f);
            assert(rc == FDS_OK);
            break;

        case FDS_ET_BOOLEAN:
            rc = fds_get_bool(data, size, &out_value->b);
            assert(rc == FDS_OK);
            break;

        case FDS_ET_IPV4_ADDRESS:
            out_value->ip.prefix = 32;
            out_value->ip.version = 4;
            rc = fds_get_ip(data, size, out_value->ip.addr);
            assert(rc == FDS_OK);
            break;

        case FDS_ET_IPV6_ADDRESS:
            out_value->ip.prefix = 128;
            out_value->ip.version = 6;
            rc = fds_get_ip(data, size, out_value->ip.addr);
            assert(rc == FDS_OK);
            break;

        case FDS_ET_MAC_ADDRESS:
            rc = fds_get_mac(data, size, out_value->mac.addr);
            assert(rc == FDS_OK);
            break;

        case FDS_ET_STRING:
        case FDS_ET_OCTET_ARRAY:
            out_value->str.len = size;
            out_value->str.chars = (char *) data;
            break;

        case FDS_ET_DATE_TIME_SECONDS:
        case FDS_ET_DATE_TIME_MILLISECONDS:
        case FDS_ET_DATE_TIME_MICROSECONDS:
        case FDS_ET_DATE_TIME_NANOSECONDS: {
            struct timespec ts;
            rc = fds_get_datetime_hp_be(data, size, field_def->data_type, &ts);
            assert(rc == FDS_OK);
            out_value->u = ts.tv_sec * 1000000000 + ts.tv_nsec;
            break;
        }

        default:
            assert(0 && "not implemented");
    }

    return FDS_OK;
}

/**
 * Read the first source that is found in the record
 */
static bool
read_first_of(struct fds_drec *record, const struct fds_iemgr_alias *alias, size_t *source_idx, uint16_t find_flags,
              fds_filter_value_u *out_value)
{
    while (*source_idx < alias->sources_cnt) {
        const struct fds_iemgr_elem *field_def = alias->sources[*source_idx];
        (*source_idx)++;
        if (read_record_field(record, field_def, find_flags, out_value) == FDS_OK) {
            return true;
        }
    }
    set_default_value(out_value);
    return false;
}

/**
 * The filter data callback
 */
static int
data_callback(void *user_ctx, bool reset_ctx, int id, void *data, fds_filter_value_u *out_value)
{

    struct fds_ipfix_filter *ipxfil = user_ctx;
    struct fds_drec *rec = data;


    assert(id >= 0 && (size_t) id < ipxfil->lookup_tab.cnt);

    struct ipxfil_lookup_item *item = &ipxfil->lookup_tab.items[id];

    assert(item->kind != IPXFIL_CONST_LOOKUP);

    switch (item->kind) {
    case IPXFIL_ALIAS_LOOKUP:
        switch (item->alias->mode) {
        case FDS_ALIAS_FIRST_OF:
            if (reset_ctx) {
                ipxfil->lookup_state.source_idx = 0;
                return read_first_of(rec, item->alias, &ipxfil->lookup_state.source_idx, ipxfil->lookup_state.find_flags, out_value)
                    ? FDS_OK : FDS_ERR_NOTFOUND;
            }
            set_default_value(out_value);
            return FDS_ERR_NOTFOUND;

        case FDS_ALIAS_ANY_OF:
            if (reset_ctx) {
                ipxfil->lookup_state.source_idx = 0;
            }
            return read_first_of(rec, item->alias, &ipxfil->lookup_state.source_idx, ipxfil->lookup_state.find_flags, out_value)
                ? FDS_OK_MORE : FDS_ERR_NOTFOUND;
        default:
            assert(0);
        }
        break;

    case IPXFIL_FIELD_LOOKUP: {
        int rc = read_record_field(rec, item->elem, ipxfil->lookup_state.find_flags, out_value);
        if (rc != FDS_OK) {
            set_default_value(out_value);
        }
        return rc;
    }

    default:
        assert(0);
    }

    return FDS_ERR_NOTFOUND;
}

int
fds_ipfix_filter_create(struct fds_ipfix_filter **ipxfil, const fds_iemgr_t *iemgr, const char *expr)
{
    *ipxfil = calloc(1, sizeof(struct fds_ipfix_filter));
    if (*ipxfil == NULL) {
        return FDS_ERR_NOMEM;
    }
    (*ipxfil)->iemgr = iemgr;

    fds_filter_opts_t *opts = fds_filter_create_default_opts();
    if (opts == NULL) {
        (*ipxfil)->error = MEMORY_ERROR;
        return FDS_ERR_NOMEM;
    }
    fds_filter_opts_set_lookup_cb(opts, lookup_callback);
    fds_filter_opts_set_const_cb(opts, const_callback);
    fds_filter_opts_set_data_cb(opts, data_callback);
    fds_filter_opts_set_user_ctx(opts, *ipxfil);

    int rc = fds_filter_create(&(*ipxfil)->filter, expr, opts);
    if (rc != FDS_OK) {
        (*ipxfil)->error = fds_filter_get_error((*ipxfil)->filter);
        return rc;
    }

    return FDS_OK;
}

bool
fds_ipfix_filter_eval(struct fds_ipfix_filter *ipxfil, struct fds_drec *record)
{
    ipxfil->lookup_state.source_idx = 0;
    ipxfil->lookup_state.find_flags = 0;
    return fds_filter_eval(ipxfil->filter, record);
}

enum fds_ipfix_filter_match
fds_ipfix_filter_eval_biflow(struct fds_ipfix_filter *ipxfil, struct fds_drec *record)
{
    if (record->tmplt->flags & FDS_TEMPLATE_BIFLOW) {
        int result = 0;

        ipxfil->lookup_state.source_idx = 0;
        ipxfil->lookup_state.find_flags = FDS_DREC_BIFLOW_FWD;
        if (fds_filter_eval(ipxfil->filter, record)) {
            result |= FDS_IPFIX_FILTER_MATCH_FWD;
        }

        ipxfil->lookup_state.source_idx = 0;
        ipxfil->lookup_state.find_flags = FDS_DREC_BIFLOW_REV;
        if (fds_filter_eval(ipxfil->filter, record)) {
            result |= FDS_IPFIX_FILTER_MATCH_REV;
        }

        return result;

    } else {
        ipxfil->lookup_state.source_idx = 0;
        ipxfil->lookup_state.find_flags = 0;

        if (fds_filter_eval(ipxfil->filter, record)) {
            return FDS_IPFIX_FILTER_MATCH_FWD;
        } else {
            return FDS_IPFIX_FILTER_NO_MATCH;
        }
    }
}

void
fds_ipfix_filter_destroy(struct fds_ipfix_filter *ipxfil)
{
    fds_filter_destroy(ipxfil->filter);
    free(ipxfil->lookup_tab.items);
    free(ipxfil);
}

const char *
fds_ipfix_filter_get_error(struct fds_ipfix_filter *ipxfil)
{
    if (ipxfil == NULL) {
        return MEMORY_ERROR->msg;
    }
    return ipxfil->error->msg;
}