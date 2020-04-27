/**
 * \file   src/iemgr/iemgr_element.cpp
 * \author Michal Režňák
 * \brief  Implementation of the iemgr element
 * \date   8. August 2017
 */

/* Copyright (C) 2017 CESNET, z.s.p.o.
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

#include <libfds/iemgr.h>
#include "iemgr_common.h"
#include "iemgr_scope.h"

fds_iemgr_elem *
element_create()
{
    auto *elem = new fds_iemgr_elem;
    elem->id            = 0;
    elem->name          = nullptr;
    elem->scope         = nullptr;
    elem->data_type     = FDS_ET_UNASSIGNED;
    elem->data_semantic = FDS_ES_UNASSIGNED;
    elem->data_unit     = FDS_EU_NONE;
    elem->status        = FDS_ST_INVALID;
    elem->is_reverse    = false;
    elem->reverse_elem  = nullptr;
    elem->aliases       = nullptr;
    elem->aliases_cnt   = 0;
    elem->mappings      = nullptr;
    elem->mappings_cnt  = 0;
    return elem;
}

fds_iemgr_elem*
element_copy(fds_iemgr_scope_inter* scope, const fds_iemgr_elem* elem)
{
    auto res = new fds_iemgr_elem;

    res->scope         = &scope->head;
    res->name          = copy_str(elem->name);
    res->id            = elem->id;
    res->data_type     = elem->data_type;
    res->data_unit     = elem->data_unit;
    res->data_semantic = elem->data_semantic;
    res->is_reverse    = elem->is_reverse;
    res->reverse_elem  = elem->reverse_elem;
    res->status        = elem->status;
    res->aliases_cnt = 0;
    res->aliases = nullptr;
    res->mappings_cnt = 0;
    res->mappings = nullptr;
    // res->aliases_cnt   = elem->aliases_cnt;
    // res->aliases       = copy_flat_array(elem->aliases, elem->aliases_cnt);
    // res->mappings_cnt  = elem->mappings_cnt;
    // res->mappings      = copy_flat_array(elem->mappings, elem->mappings_cnt);

    return res;
}

fds_iemgr_elem *
element_create_reverse(fds_iemgr_elem* src, uint16_t new_id)
{
    auto res           = unique_elem(new fds_iemgr_elem, &::element_remove);
    res->id            = new_id;
    res->name          = copy_reverse(src->name);
    res->scope         = src->scope;
    res->data_type     = src->data_type;
    res->data_semantic = src->data_semantic;
    res->data_unit     = src->data_unit;
    res->status        = src->status;
    res->is_reverse    = true;
    res->reverse_elem  = src;
    res->aliases_cnt   = src->aliases_cnt;
    res->aliases       = copy_flat_array(src->aliases, src->aliases_cnt);
    res->mappings_cnt  = src->mappings_cnt;
    res->mappings      = copy_flat_array(src->mappings, src->mappings_cnt);

    src->reverse_elem  = res.get();
    return res.release();
}

void
element_remove(fds_iemgr_elem* elem)
{
    free(elem->aliases);
    free(elem->mappings);
    delete[] elem->name;
    delete elem;
}

bool
element_can_overwritten(fds_iemgr_t* mgr, const fds_iemgr_elem* dst, const fds_iemgr_elem* src)
{
    if (!mgr->can_overwrite_elem) {
        mgr->err_msg = "Element with ID '" +to_string(src->id)+ "' in scope with PEN '"
                       +to_string(src->scope->pen)+ "' cannot be overwritten.";
        return false;
    }

    if (dst->is_reverse != src->is_reverse) {
        mgr->err_msg = "Element with ID '" +to_string(src->id)+ "' in scope with PEN '"
                       +to_string(src->scope->pen)+ "' cannot overwrite reverse element with same ID.";
        return false;
    }

    return true;
}

bool
element_save(fds_iemgr_scope_inter* scope, fds_iemgr_elem* elem)
{
    scope->ids.emplace_back(elem->id, elem);
    scope->names.emplace_back(elem->name, elem);
    return true;
}

bool
element_check_reverse_param(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope, fds_iemgr_elem* elem,
                            uint64_t id)
{
    if (scope->head.biflow_mode != FDS_BF_INDIVIDUAL) {
        mgr->err_msg = "Reverse element, with ID '"+to_string(id)+
                       "' in a scope with PEN '" +to_string(scope->head.pen)+
                       "', can be defined only when scope biflow mode is INDIVIDUAL";
        return false;
    }

    if (id > UINT15_LIMIT) {
        mgr->err_msg = "ID '" +to_string(id)+
                       "' of a new reverse element is bigger than limit '" +to_string(UINT15_LIMIT);
        return false;
    }

    if (elem->id == id) {
        mgr->err_msg = "ID '"+to_string(id)+
                "' of the reverse element is already defined to the forward element.";
        return false;
    }

    return true;
}

fds_iemgr_elem *
element_add_reverse(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope, fds_iemgr_elem* elem,
                    uint16_t biflow_id)
{
    if (!element_check_reverse_param(mgr, scope, elem, biflow_id)) {
        return nullptr;
    }

    auto res = unique_elem(element_create_reverse(elem, biflow_id), &::element_remove) ;
    if (res == nullptr) {
        return nullptr;
    }

    if (!element_save(scope, res.get())) {
        return nullptr;
    }
    return res.release();
}

bool
element_overwrite_values(fds_iemgr_t *mgr, fds_iemgr_scope_inter *scope, fds_iemgr_elem *dst,
                         fds_iemgr_elem *src)
{
    if (src->name != nullptr) {
        auto index = find_first(scope->names, string(dst->name));
        if (index == nullptr) {
            mgr->err_msg = "Element with name '" +string(dst->name)+
                           "' could not be found in the scope with PEN '"+to_string(scope->head.pen)+ "'.";
            return false;
        }
        *index = src->name;

        delete[] dst->name;
        dst->name            = copy_str(src->name);
    }
    if (src->data_type      != FDS_ET_UNASSIGNED) {
        dst->data_type       = src->data_type;
    }
    if (src->data_semantic  != FDS_ES_UNASSIGNED) {
        dst->data_semantic   = src->data_semantic;
    }
    if (src->data_unit      != FDS_EU_NONE) {
        dst->data_unit       = src->data_unit;
    }
    if (src->status         != FDS_ST_INVALID) {
        dst->status          = src->status;
    }

    return true;
}

bool
element_overwrite_reverse(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope, fds_iemgr_elem* rev, fds_iemgr_elem* src, int id)
{
    fds_iemgr_scope_inter* tmp_scope = scope;
    if (scope->head.biflow_mode == FDS_BF_PEN) {
        tmp_scope = find_second(mgr->pens, scope->head.biflow_id);
        if (tmp_scope == nullptr) {
            mgr->err_msg = "Reverse scope with PEN '" +to_string(scope->head.biflow_id)+ "' cannot be found";
            return false;
        }
    }

    if (rev == nullptr) {
        if (id < 0) {
            return true;
        }

        if (!parsed_id_save(mgr, scope, static_cast<const uint16_t>(id))) {
            return false;
        }
        fds_iemgr_elem *tmp = element_add_reverse(mgr, tmp_scope, src, static_cast<uint16_t>(id));
        return tmp != nullptr;
    }

    if (id >= 0) {
        if (src->scope->biflow_mode != FDS_BF_INDIVIDUAL) {
            mgr->err_msg = "Scope with PEN '" +to_string(src->scope->pen)+ "' cannot define biflowID in elements, because it doesn't have biflow mode INDIVIDUAL.";
            return false;
        }

        if (rev->id != id) {
            mgr->err_msg = "Cannot define biflowID to the element with reverse ID '" +to_string(rev->id)+ "' in the scope with PEN '" +to_string(scope->head.pen)+ "' which overwrites previously defined element with same ID.";
            return false;
        }
    }

    char *tmp = copy_reverse(src->name);
    delete[] src->name;
    src->name = tmp;
    return element_overwrite_values(mgr, tmp_scope, rev, src);
}

bool
element_overwrite(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope, fds_iemgr_elem* dst,
                  unique_elem src, int biflow_id)
{
    if (!element_can_overwritten(mgr, dst, src.get())) {
        return false;
    }

    if (!element_overwrite_values(mgr, scope, dst, src.get())) {
        return false;
    }

    return element_overwrite_reverse(mgr, scope, dst->reverse_elem, src.get(), biflow_id);
}

bool
element_write(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope, unique_elem elem, int biflow_id)
{
    if (elem->name == nullptr) {
        mgr->err_msg = "Element with ID '" +to_string(elem->id)+
                       "' in the scope with PEN '" +to_string(scope->head.pen)+
                       "' has not defined name";
        return false;
    }
    if (elem->data_type == FDS_ET_UNASSIGNED) {
        mgr->err_msg = "Element with ID '" +to_string(elem->id)+
                       "' in the scope with PEN '" +to_string(scope->head.pen)+
                       "' has not defined data type";
        return false;
    }

    if (biflow_id >= 0) {
        elem->reverse_elem = element_add_reverse(mgr, scope, elem.get(),
                                                 static_cast<uint16_t>(biflow_id));
        if (elem->reverse_elem == nullptr) {
            return false;
        }
    }

    return element_save(scope, elem.release());
}

bool
element_push(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope, unique_elem elem, int biflow_id)
{
    if (!parsed_id_save(mgr, scope, elem->id)) {
        return false;
    }

    auto res = find_second(scope->ids, elem->id);
    if (res != nullptr) {
        return element_overwrite(mgr, scope, res, move(elem), biflow_id);
    }
    return element_write(mgr, scope, move(elem), biflow_id);
}

bool
element_read(fds_iemgr_t* mgr, fds_xml_ctx_t* ctx, fds_iemgr_scope_inter* scope)
{
    auto elem = unique_elem(element_create(), &::element_remove);
    int biflow_id = BIFLOW_ID_INVALID;

    const struct fds_xml_cont *cont;
    while(fds_xml_next(ctx, &cont) != FDS_EOC) {
        switch (cont->id) {
        case ELEM_ID:
            if (!get_id(mgr, elem->id, cont->val_int)) {
                return false;
            }
            break;
        case ELEM_NAME:
            if (strcmp(cont->ptr_string, "") == 0) {
                mgr->err_msg = "Element name cannot be empty";
                return false;
            }
            elem->name = copy_str(cont->ptr_string);
            break;
        case ELEM_DATA_TYPE:
            elem->data_type = fds_iemgr_str2type(cont->ptr_string);
            if (elem->data_type == FDS_ET_UNASSIGNED) {
                mgr->err_msg = "Data type of the element with ID '" +to_string(elem->id)+
                        "' in scope with PEN '" +to_string(scope->head.pen)+ "' not recognised.";
                return false;
            }
            break;
        case ELEM_DATA_SEMAN:
            elem->data_semantic = fds_iemgr_str2semantic(cont->ptr_string);
            if (elem->data_semantic == FDS_ES_UNASSIGNED) {
                mgr->err_msg = "Data semantic of the element with ID '" +to_string(elem->id)+
                        "' in scope with PEN '" +to_string(scope->head.pen)+ "' not recognised.";
                return false;
            }
            break;
        case ELEM_DATA_UNIT:
            elem->data_unit = fds_iemgr_str2unit(cont->ptr_string);
            if (elem->data_unit == FDS_EU_UNASSIGNED) {
                mgr->err_msg = "Data unit of the element with ID '" +to_string(elem->id)+
                        "' in scope with PEN '" +to_string(scope->head.pen)+ "' not recognised.";
                return false;
            }
            break;
        case ELEM_STATUS:
            elem->status = get_status(cont->ptr_string);
            if (elem->status == FDS_ST_INVALID) {
                mgr->err_msg = "Status of the element with ID '" +to_string(elem->id)+
                        "' in scope with PEN '" +to_string(scope->head.pen)+ "' not recognised.";
                return false;
            }
            break;
        case ELEM_BIFLOW:
            biflow_id = get_biflow_elem_id(mgr, cont->val_int);
            if (biflow_id < 0) {
                return false;
            }
            break;
        default:
            break;
        }
    }

    elem->scope = &scope->head;
    return element_push(mgr, scope, move(elem), biflow_id);
}

bool
elements_read(fds_iemgr_t* mgr, fds_xml_ctx_t* ctx, fds_iemgr_scope_inter* scope)
{
    const struct fds_xml_cont *cont;
    while(fds_xml_next(ctx, &cont) != FDS_EOC) {
        if (cont->id != ELEM) {
            continue;
        }

        if(!element_read(mgr, cont->ptr_ctx, scope)) {
            return false;
        }
    }
    return true;
}

bool
elements_copy_reverse(fds_iemgr_scope_inter* dst, const fds_iemgr_scope_inter* src)
{
    for (auto& elem: src->ids) {
        auto res = element_create_reverse(elem.second, elem.second->id);
        res->scope        = &dst->head;
        element_save(dst, res);
    }

    return true;
}

void
elements_remove_reverse_split(fds_iemgr_scope_inter* scope)
{
    for (auto iter = scope->names.begin(); iter != scope->names.end(); ) {
        if ((iter.base()->second->id & SPLIT_BIT) != 0) {
            iter = scope->names.erase(iter);
        } else {
            ++iter;
        }
    }

    for (auto iter = scope->ids.begin(); iter != scope->ids.end(); ) {
        if ((iter.base()->second->id & SPLIT_BIT) != 0) {
            element_remove(iter.base()->second);
            iter = scope->ids.erase(iter);
        } else {
            iter.base()->second->reverse_elem = nullptr;
            ++iter;
        }
    }
}

void
elements_remove_reverse(fds_iemgr_scope_inter* scope)
{
    if (scope->head.biflow_mode == FDS_BF_SPLIT) {
        elements_remove_reverse_split(scope);
    }
}

int
element_destroy(fds_iemgr_t *mgr, const uint32_t pen, const uint16_t id)
{
    const auto scope_pen_it = find_iterator(mgr->pens, pen);
    if (scope_pen_it == mgr->pens.end()) {
        return FDS_ERR_NOTFOUND;
    }
    const auto scope = scope_pen_it.base()->second;

    const auto elem_id_it = find_iterator(scope->ids, id);
    if (elem_id_it == scope->ids.end()) {
        return FDS_ERR_NOTFOUND;
    }

    const auto elem = elem_id_it.base()->second;
    const string name = elem->name;

    const auto elem_name_it = find_iterator(scope->names, name);
    if (elem_name_it == scope->names.end()) {
        return FDS_ERR_NOTFOUND;
    }

    scope->ids.erase(elem_id_it);
    scope->names.erase(elem_name_it);

    if (elem->is_reverse) {
        elem->reverse_elem->reverse_elem = nullptr;
        element_remove(elem);
        return FDS_OK;
    }

    if (elem->reverse_elem != nullptr) {
        const int ret = element_destroy(mgr, elem->reverse_elem->scope->pen,
                                              elem->reverse_elem->id);
        if (ret != FDS_OK) {
            return ret;
        }
    }

    if (scope->ids.empty()) {
        const auto scope_prefix_it = find_iterator(mgr->prefixes, string(scope->head.name));
        if (scope_prefix_it == mgr->prefixes.end()) {
            return FDS_ERR_NOTFOUND;
        }
        scope_remove(scope);
        mgr->pens.erase(scope_pen_it);
        mgr->prefixes.erase(scope_prefix_it);
        mgr_sort(mgr);
    }
    else {
        scope_sort(scope);
    }

    element_remove(elem);
    return FDS_OK;
}

bool
element_add_alias_ref(fds_iemgr_elem *elem, fds_iemgr_alias *alias)
{
    fds_iemgr_alias **ref = array_push(&elem->aliases, &elem->aliases_cnt);
    if (ref == NULL) {
        return false;
    }
    *ref = alias;
    printf("XXXX: Added alias to mapping %p from elem %p, now %d\n", alias, elem, elem->aliases_cnt);
    return true;
}

int
element_add_mapping_ref(fds_iemgr_elem *elem, fds_iemgr_mapping *mapping)
{
    fds_iemgr_mapping **ref = array_push(&elem->mappings, &elem->mappings_cnt);
    if (ref == NULL) {
        return false;
    }
    *ref = mapping;
    printf("XXXX: Added ref to mapping %p from elem %p, now %d\n", mapping, elem, elem->mappings_cnt);
    return true;
}
