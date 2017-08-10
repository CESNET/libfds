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
    res->is_reverse       = scope->is_reverse;

    fds_iemgr_elem* elem;
    for (const auto& tmp: scope->ids) {
        if (tmp.second->is_reverse) {
            continue;
        }

        elem = element_copy(res.get(), tmp.second);
        if (elem->reverse_elem != nullptr) {
            elem->reverse_elem = element_copy(res.get(), tmp.second->reverse_elem);
        }

        res->ids.emplace_back(elem->id, elem);
        res->names.emplace_back(elem->name, elem);
    }

    return res.release();
}

/**
 * \brief Create reverse scope
 * \param[in] scope Scope
 * \return Reverse scope on success, otherwise nullptr
 */
fds_iemgr_scope_inter*
scope_create_reverse(const fds_iemgr_scope_inter* scope)
{
    auto res = unique_scope(new fds_iemgr_scope_inter, &::scope_remove);
    res->head.pen         = scope->head.biflow_id;
    res->head.name        = copy_reverse(scope->head.name);
    res->head.biflow_id   = scope->head.pen;
    res->head.biflow_mode = scope->head.biflow_mode;
    res->is_reverse       = true; // TODO usefull in visible scope?

    if (!elements_copy_reverse(res.get(), scope)) {
        return nullptr;
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

unique_scope
scope_create()
{
    auto scope = unique_scope(new fds_iemgr_scope_inter, &::scope_remove);
    scope->head.name        = nullptr;
    scope->head.pen         = 0;
    scope->head.biflow_id   = 0;
    scope->head.biflow_mode = FDS_BW_INVALID;
    scope->is_reverse       = false;

    return scope;
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


/**
 * \brief Save reverse elements of elements from \p src to dst
 * \param[in]  scope Source scope
 * \return True on success, otherwise False
 * \warning Scope cannot contain reverse elements already
 */
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


/**
 * \brief Overwrite reverse scope
 * \param[in,out] mgr   Manager
 * \param[in]     scope Scope with elements that will be written to reverse scope
 * \return True on success, otherwise False
 */
// TODO don't remove all elements when it's not needed
bool
scope_set_biflow_overwrite(fds_iemgr_t* mgr, const fds_iemgr_scope_inter* scope)
{
    auto res = find_second(mgr->pens, scope->head.biflow_id);
    scope_remove_elements(res);

    return elements_copy_reverse(res, scope);
}

/**
 * \brief Split scope and create reverse elements
 * \param[in,out] mgr   Manager
 * \param[in,out] scope Scope
 * \return True on success, otherwise False
 */
bool
scope_set_biflow_split(fds_iemgr_t *mgr, fds_iemgr_scope_inter *scope)
{
    // TODO don't make another vector
    auto ids = scope->ids;
    uint16_t new_id;
    for (const auto& elem: ids) {

        if ((elem.second->id & (1 << scope->head.biflow_id)) != 0) {
            mgr->err_msg = "Element with ID '" +to_string(elem.second->id)+ "' in the scope with PEN '" +to_string(scope->head.pen)+ "' have defined ID reversed for reverse element. Bit in element ID on position '" +to_string(scope->head.biflow_id)+ "' can't be set.";
            return false;
        }

        new_id = static_cast<uint16_t>(elem.second->id | (1 << scope->head.biflow_id));
        auto res = element_create_reverse(elem.second, new_id);
        if (res == nullptr) {
            return false;
        }
        element_save(scope, res);
    }
    scope_sort(scope);
    return true;
}

/**
 * \brief Overwrite \p src with \p temp
 * \param mgr Manager
 * \param scope Scope
 * \return Overwritten scope on success, otherwise nullptr
 */
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

    if (!elements_remove_reverse(scope)) {
        return nullptr;
    }

    mgr->overwrite_scope.second.insert(scope->head.pen);
    return scope;
}
