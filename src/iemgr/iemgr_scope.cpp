//
// Created by Michal Režňák on 8/7/17.
//

#include "fds_iemgr_todo_name.h"

/**
 * \brief Sort elements in a scope
 * \param scope Scope
 */
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

    fds_iemgr_elem* elem;
    for (const auto& tmp: scope->ids) {
        elem = element_copy(res.get(), tmp.second);

        res->ids.emplace_back(elem->id, elem);
        res->names.emplace_back(elem->name, elem);
    }

    return res.release();
}


/**
 * \brief Remove elements from a scope
 * \param scope Scope
 */
void
scope_remove_elements(fds_iemgr_scope_inter* scope)
{
    for (const auto& elem: scope->ids) {
        element_remove(elem.second);
    }

    scope->ids.clear();
    scope->names.clear();
}

/**
 * \brief Clear and delete scope
 * \param[in,out] scope Scope
 */
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
