/**
 * \file   src/iemgr/iemgr_scope.cpp
 * \author Michal Režňák
 * \brief  Implementation of the iemgr scope
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

#include "iemgr_common.h"
#include "iemgr_element.h"

void
scope_sort(fds_iemgr_scope_inter* scope)
{
    sort_vec(scope->ids);
    sort_vec(scope->names);
}

/**
 * \brief Copy scope \p scope
 * \param[in] scope Source scope
 * \return Copied scope on success, otherwise nullptr
 */
fds_iemgr_scope_inter*
scope_copy(const fds_iemgr_scope_inter* scope)
{
    auto res = unique_scope(new fds_iemgr_scope_inter, &::scope_remove);
    res->head.name        = copy_str(scope->head.name);
    res->head.pen         = scope->head.pen;
    res->head.biflow_mode = scope->head.biflow_mode;
    res->head.biflow_id   = scope->head.biflow_id;
    res->is_reverse       = scope->is_reverse;

    fds_iemgr_elem* elem;
    for (const auto& tmp: scope->ids) {
        if (tmp.second->is_reverse) {
            continue;
        }

        elem = element_copy(res.get(), tmp.second);
        if (elem->reverse_elem != nullptr && scope->head.biflow_mode != FDS_BF_PEN) {
            elem->reverse_elem = element_copy(res.get(), tmp.second->reverse_elem);
        }

        res->ids.emplace_back(elem->id, elem);
        res->names.emplace_back(elem->name, elem);
    }

    scope_sort(res.get());
    return res.release();
}

fds_iemgr_scope_inter*
scope_create_reverse(const fds_iemgr_scope_inter* scope)
{
    auto res = unique_scope(new fds_iemgr_scope_inter, &::scope_remove);
    res->head.pen         = scope->head.biflow_id;
    res->head.name        = copy_reverse(scope->head.name);
    res->head.biflow_id   = scope->head.pen;
    res->head.biflow_mode = scope->head.biflow_mode;
    res->is_reverse       = true;

    if (!elements_copy_reverse(res.get(), scope)) {
        return nullptr;
    }

    return res.release();
}

void
scope_remove_elements(fds_iemgr_scope_inter* scope)
{
    for (const auto& elem: scope->ids) {
        element_remove(elem.second);
    }

    scope->ids.clear();
    scope->names.clear();
}

unique_scope
scope_create()
{
    auto scope = unique_scope(new fds_iemgr_scope_inter, &::scope_remove);
    scope->head.name        = nullptr;
    scope->head.pen         = 0;
    scope->head.biflow_id   = 0;
    scope->head.biflow_mode = FDS_BF_INVALID;
    scope->is_reverse       = false;

    return scope;
}

void
scope_remove(fds_iemgr_scope_inter* scope)
{
    if (scope == nullptr) {
        return;
    }

    scope_remove_elements(scope);

    delete[] scope->head.name;
    delete scope;
}

bool
scope_save_reverse_elem(fds_iemgr_scope_inter *scope)
{
    fds_iemgr_elem* rev;
    auto vec = scope->ids;

    for (const auto& elem: vec) {
        if (elem.second->reverse_elem == nullptr) {
            continue;
        }

        rev = elem.second->reverse_elem;

        scope->ids.emplace_back(rev->id, rev);
        scope->names.emplace_back(rev->name, rev);
    }

    return true;
}

bool
scope_set_biflow_overwrite(fds_iemgr_t* mgr, const fds_iemgr_scope_inter* scope)
{
    auto res = find_second(mgr->pens, scope->head.biflow_id);
    scope_remove_elements(res);

    return elements_copy_reverse(res, scope);
}

bool
scope_set_biflow_split(fds_iemgr_t *mgr, fds_iemgr_scope_inter *scope)
{
    auto ids = scope->ids;
    uint16_t new_id;

    for (const auto& elem: ids) {

        if ((elem.second->id & SPLIT_BIT) != 0) {
            mgr->err_msg = "Element with ID '" +to_string(elem.second->id)+
                    "' in the scope with PEN '" +to_string(scope->head.pen)+
                    "' have defined ID reserved for reverse element. "
            "Bit in element ID on position '" +to_string(scope->head.biflow_id)+ "' can't be set.";
            return false;
        }

        new_id = static_cast<uint16_t>(elem.second->id | SPLIT_BIT);
        auto res = element_create_reverse(elem.second, new_id);
        if (res == nullptr) {
            return false;
        }
        element_save(scope, res);
    }

    return true;
}

fds_iemgr_scope_inter *
scope_overwrite(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope)
{
    if (!mgr->overwrite_scope.first) {
        mgr->err_msg = "Scope with PEN '" +to_string(scope->head.pen)+
                       "' is defined multiple times in 'system/elements' folder";
        return nullptr;
    }
    if (mgr->overwrite_scope.second.find(scope->head.pen) != mgr->overwrite_scope.second.end()) {
        mgr->err_msg = "Scope with PEN '" +to_string(scope->head.pen)+
                       "' is defined multiple times in 'user/elements' folder";
        return nullptr;
    }

    elements_remove_reverse(scope);

    mgr->overwrite_scope.second.insert(scope->head.pen);
    return scope;
}

fds_iemgr_scope_inter *
scope_save(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope)
{
    mgr->prefixes.emplace_back(scope->head.name, scope);
    mgr->pens.emplace_back(scope->head.pen, scope);

    return scope;
}

bool
scope_set_biflow_pen(fds_iemgr_t* mgr, const fds_iemgr_scope_inter* scope)
{
    if (mgr->overwrite_scope.second.find(scope->head.pen) != mgr->overwrite_scope.second.end()) {
        return scope_set_biflow_overwrite(mgr, scope);
    }

    fds_iemgr_scope_inter* res = scope_create_reverse(scope);
    if (res == nullptr) {
        return false;
    }
    scope_save(mgr, res);
    scope_sort(res);
    return true;
}

bool
scope_set_biflow(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope)
{
    if (scope->head.biflow_mode == FDS_BF_PEN) {
        if (!scope_set_biflow_pen(mgr, scope)) {
            return false;
        }
    }
    else if (scope->head.biflow_mode == FDS_BF_SPLIT) {
        if (!scope_set_biflow_split(mgr, scope)) {
            return false;
        }
    }

    return true;
}

fds_iemgr_scope_inter *
scope_write(fds_iemgr_t* mgr, unique_scope scope, bool biflow_read)
{
    if (scope->head.name == nullptr) {
        mgr->err_msg = "Name of the scope with PEN '" +to_string(scope->head.pen)+
                       "' wasn't defined";
        return nullptr;
    }
    if (!biflow_read) {
        mgr->err_msg = "Biflow of the scope with PEN " +to_string(scope->head.pen)+
                       " wasn't defined";
        return nullptr;
    }

    return scope_save(mgr, scope.release());
}

fds_iemgr_scope_inter *
scope_push(fds_iemgr_t* mgr, unique_scope scope, bool biflow_read)
{
    auto res = find_second(mgr->pens, scope->head.pen);
    if (res != nullptr) {
        return scope_overwrite(mgr, res);
    }

    return scope_write(mgr, move(scope), biflow_read);
}

bool
scope_read_biflow(fds_iemgr_t* mgr, fds_xml_ctx_t* ctx, struct fds_iemgr_scope_inter* scope)
{
    int64_t id;

    const struct fds_xml_cont *cont;
    while(fds_xml_next(ctx, &cont) != FDS_EOC) {
        switch (cont->id) {
        case BIFLOW_MODE:
            scope->head.biflow_mode  = get_biflow(cont->ptr_string);
            if (scope->head.biflow_mode == FDS_BF_INVALID) {
                mgr->err_msg = "Biflow mode doesn't have a type " + string(cont->ptr_string);
                return false;
            }
            break;
        case BIFLOW_TEXT:
            id = get_biflow_id(mgr, scope, cont->val_int);
            if (id < 0) {
                return false;
            }
            scope->head.biflow_id = static_cast<uint32_t>(id);
            break;
        default:
            break;
        }
    }
    return true;
}

fds_iemgr_scope_inter *
scope_read(fds_iemgr_t* mgr, fds_xml_ctx_t* ctx)
{
    auto scope = scope_create();
    bool biflow_read = false;

    const struct fds_xml_cont *cont;
    while(fds_xml_next(ctx, &cont) != FDS_EOC) {
        switch (cont->id) {
        case SCOPE_PEN:
            if (!get_pen(mgr, scope->head.pen, cont->val_uint)) {
                return nullptr;
            }
            break;
        case SCOPE_NAME:
            if (strcmp(cont->ptr_string, "") == 0) {
                mgr->err_msg = "Scope name cannot be empty";
                return nullptr;
            }
            scope->head.name = copy_str(cont->ptr_string);
            break;
        case SCOPE_BIFLOW:
            biflow_read = true;
            if (!scope_read_biflow(mgr, cont->ptr_ctx, scope.get())) {
                return nullptr;
            }
            break;
        default:
            break;
        }
    }
    return scope_push(mgr, move(scope), biflow_read);
}

const fds_xml_cont *
scope_find_cont(fds_iemgr_t* mgr, fds_xml_ctx_t* ctx)
{
    const struct fds_xml_cont *cont;
    while(fds_xml_next(ctx, &cont) != FDS_EOC) {
        if (cont->id == SCOPE) {
            return cont;
        }
    }
    mgr->err_msg = "Scope must be defined on a top level of the file";
    return nullptr;
}

fds_iemgr_scope_inter *
scope_parse_and_store(fds_iemgr_t* mgr, fds_xml_ctx_t* ctx)
{
    const fds_xml_cont *cont = scope_find_cont(mgr, ctx);
    if (cont == nullptr) {
        return nullptr;
    }

    fds_xml_rewind(ctx);
    return scope_read(mgr, cont->ptr_ctx);
}

bool
scope_check(fds_iemgr_t* mgr, const fds_iemgr_scope_inter* scope)
{
    const auto pair = find_pair(scope->names);
    if(pair != scope->names.end()) {
        mgr->err_msg = "Element with name '" +string(pair.base()->first)+
                       "' is defined multiple times";
        return false;
    }

    return true;
}
