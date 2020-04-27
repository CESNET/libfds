/**
 * \file   src/iemgr/iemgr_common.cpp
 * \author Michal Režňák
 * \brief  Implementation of the common functions
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

#include <cstring>
#include <limits>
#include <utility>
#include <libfds/iemgr.h>
#include "iemgr_common.h"
#include "iemgr_scope.h"
#include "iemgr_alias.h"
#include "iemgr_mapping.h"

bool
split_name(const string& str, pair<string, string>& res)
{
    const size_t pos = str.find(':');
    if (str.substr(pos+1).find(':') != str.npos) {
        return false;
    }

    if (pos == str.npos) {
        res.first = "iana";
        res.second = str.substr(pos+1);
    }
    else {
        res.first = str.substr(0, pos);
        res.second = str.substr(pos+1);
    }

    return true;
}

bool
parsed_id_save(fds_iemgr_t* mgr, const fds_iemgr_scope_inter* scope, const uint16_t id)
{
    if (mgr->parsed_ids.find(id) != mgr->parsed_ids.end()) {
        mgr->err_msg = "Element with ID '" + to_string(id)
            + "' is defined multiple times in the scope with PEN '"
            + to_string(scope->head.pen)+ "'";
        return false;
    }

    mgr->parsed_ids.insert(id);
    return true;
}

/**
 * \brief Copy string with postfix '\@reverse'
 * \param[in] str String
 * \return Copied string on success, otherwise nullptr
 */
char *
copy_reverse(const char *str)
{
    if (str == nullptr) {
        return nullptr;
    }

    const char *rev = REVERSE_NAME;

    int len = static_cast<const int>(strlen(str) + strlen(rev) + 1);
    char* res = new char[len];
    strcpy(res, (string(str)+rev).c_str());

    return res;
}

char *
copy_str(const char *str)
{
    if (str == nullptr) {
        return nullptr;
    }

    auto res = new char[strlen(str)+1];
    strcpy(res, str);
    return res;
}

fds_iemgr_element_status
get_status(const char *status)
{
    if (!strcasecmp(status,      "current"))
        return FDS_ST_CURRENT;
    else if (!strcasecmp(status, "deprecated"))
        return FDS_ST_DEPRECATED;
    else
        return FDS_ST_INVALID;
}

fds_iemgr_element_biflow
get_biflow(const char *mode)
{
    if (!strcasecmp(mode,        "pen"))
        return FDS_BF_PEN;
    else if (!strcasecmp(mode,   "none"))
        return FDS_BF_NONE;
    else if (!strcasecmp(mode,   "split"))
        return FDS_BF_SPLIT;
    else if (!strcasecmp(mode,   "individual"))
        return FDS_BF_INDIVIDUAL;
    else
        return FDS_BF_INVALID;
}

int64_t
get_biflow_id(fds_iemgr_t* mgr, const fds_iemgr_scope_inter* scope, int64_t id)
{
    if (id > UINT32_LIMIT) {
        mgr->err_msg = "Number '" +to_string(id)+ "' defined as biflow ID of a scope with PEN '"
                       +to_string(scope->head.pen)+ "' is bigger than limit " +to_string(UINT32_LIMIT);
        return BIFLOW_ID_INVALID;
    }
    if (id < 0) {
        mgr->err_msg = "Number '" +to_string(id)+
                "' defined as biflow ID of the scope with PEN cannot be negative.";
        return BIFLOW_ID_INVALID;
    }

    if (scope->head.biflow_mode == FDS_BF_SPLIT) {
        if (id < 1 || id > 15) {
            mgr->err_msg = "Number '" +to_string(id)+ "' defined as ID of a scope with PEN '"
           +to_string(scope->head.pen)+ "' must define which bit will be used for biflow SPLIT mode,"
           " thus can't be bigger than 15";
            return BIFLOW_ID_INVALID;
        }
    }

    return static_cast<uint32_t>(id);
}

bool
get_id(fds_iemgr_t *mgr, uint16_t& elem_id, int64_t val)
{
    if (val > UINT15_LIMIT) {
        mgr->err_msg = "Number '" +to_string(val)+
                "' defined to the element as an ID is bigger than limit " +to_string(UINT15_LIMIT);
        return false;
    }
    if (val < 0) {
        mgr->err_msg = "Number '" +to_string(val)+
                "' defined to the element as an ID cannot be negative.";
        return false;
    }

    elem_id = (uint16_t ) val;
    return true;
}

int
get_biflow_elem_id(fds_iemgr_t* mgr, const int64_t id)
{
    if (id > UINT15_LIMIT) {
        mgr->err_msg = "ID '" +to_string(id)+"' defined to the element is bigger than limit "
                       +to_string(UINT15_LIMIT)+ ".";
        return BIFLOW_ID_INVALID;
    }
    if (id < 0) {
        mgr->err_msg = "ID '" +to_string(id)+"' defined to the element cannot be negative.";
        return BIFLOW_ID_INVALID;
    }
    return static_cast<uint16_t>(id);
}

bool
get_pen(fds_iemgr_t *mgr, uint32_t& scope_pen, const int64_t val)
{
    if (val > UINT32_LIMIT) {
        mgr->err_msg = "Number '" +to_string(val)+
                "' defined to the scope as PEN is bigger than limit " +to_string(UINT32_LIMIT);
        return false;
    }
    if (val < 0) {
        mgr->err_msg = "Number '" +to_string(val)+
                "' defined to the scope as PEN cannot be negative.";
        return false;
    }

    scope_pen = (uint32_t) val;
    return true;
}

bool
mgr_save_reverse(fds_iemgr_t* mgr)
{
    fds_iemgr_scope_inter* tmp;
    auto vec = mgr->pens;

    for (const auto& scope: vec) {
        if (scope.second->head.biflow_mode == FDS_BF_PEN) {
            tmp = scope_create_reverse(scope.second);
            scope_sort(tmp);
            mgr->pens.emplace_back(tmp->head.pen, tmp);
            mgr->prefixes.emplace_back(tmp->head.name, tmp);
        }
        else {
            if (!scope_save_reverse_elem(scope.second)) {
                return false;
            }
            scope_sort(scope.second);
        }
    }

    return true;
}

void
mgr_remove_temp(fds_iemgr_t* mgr)
{
    mgr->overwrite_scope.second.clear();
    mgr->parsed_ids.clear();
}

void
mgr_sort(fds_iemgr_t* mgr)
{
    sort_vec(mgr->pens);
    sort_vec(mgr->prefixes);
    //sort_vec(mgr->aliased_names);
    //sort_vec(mgr->mapped_names);

    const auto func_pred = [](const pair<string, timespec>& lhs,
                              const pair<string, timespec>& rhs)
    { return lhs.first < rhs.first; };
    sort(mgr->mtime.begin(), mgr->mtime.end(), func_pred);
}

fds_iemgr_t*
mgr_copy(const fds_iemgr_t* mgr)
{
    auto res = unique_mgr(new fds_iemgr_t, &::fds_iemgr_destroy);
    if (!mgr->err_msg.empty()) {
        res->err_msg = mgr->err_msg;
    }

    fds_iemgr_scope_inter* scope;
    for (const auto& tmp: mgr->pens) {
        if (tmp.second->is_reverse) {
            continue;
        }

        scope = scope_copy(tmp.second);

        res->pens.emplace_back(scope->head.pen, scope);
        res->prefixes.emplace_back(scope->head.name, scope);
    }

    for (const auto& mtime: mgr->mtime) {
        char *path = strdup(mtime.first);
        if (path == nullptr) {
            return nullptr;
        }
        res->mtime.emplace_back(path, mtime.second);
    }

    if (!mgr_save_reverse(res.get())) {
        return nullptr;
    }

    int rc;
    rc = aliases_copy(mgr, res.get());
    if (rc != FDS_OK) {
        return nullptr;
    }

    rc = mappings_copy(mgr, res.get());
    if (rc != FDS_OK) {
        return nullptr;
    }

    // New reverse scopes may have been added
    mgr_sort(res.get());
    return res.release();
}
