//
// Created by Michal Režňák on 8/7/17.
//

#include <libfds/iemgr.h>
#include "fds_iemgr_todo_name.h"

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

    return res;
}

fds_iemgr_elem *
element_create_reverse(fds_iemgr_elem* src, uint16_t new_id)
{
    auto *res          = new fds_iemgr_elem;
    res->id            = new_id;
    res->name          = copy_reverse(src->name);
    res->scope         = src->scope;
    res->data_type     = src->data_type;
    res->data_semantic = src->data_semantic;
    res->data_unit     = src->data_unit;
    res->status        = src->status;
    res->is_reverse    = true;
    res->reverse_elem  = src;

    src->reverse_elem  = res;
    return res;
}

/**
 * \brief Remove element
 * \param[in,out] elem Element
 */
void
element_remove(fds_iemgr_elem* elem)
{
    delete[] elem->name;
    delete elem;
}

/**
 * \brief Check if element can be overwritten in a manager
 * \param[in,out] mgr  Manager
 * \param[in]     elem Element
 * \return True on success, otherwise False
 */
bool
element_can_overwritten(fds_iemgr_t* mgr, const fds_iemgr_elem* elem)
{
    if (!mgr->can_overwrite_elem) {
        mgr->err_msg = "Element with ID '" +to_string(elem->id)+ "' in scope with PEN '"
                       +to_string(elem->scope->pen)+ "' cannot be overwritten.";
        return false;
    }
    return true;
}

/**
 * \brief Save the element to the scope
 * \param[out] mgr   Manager
 * \param[out] scope Scope
 * \param[in]  elem  Saved element
 * \return True on success, otherwise False
 */
bool
element_save(fds_iemgr_scope_inter* scope, fds_iemgr_elem* elem)
{
    scope->ids.emplace_back(elem->id, elem);
    scope->names.emplace_back(elem->name, elem);

    scope_sort(scope);
    return true;
}

/**
 * \brief Check conditions if elemnt can overwrite previously defined element
 * \param[in,out] mgr   Manager
 * \param[in]     scope Scope
 * \param[in]     id    ID
 * \return True if can, otherwise False
 */
bool
element_check_overwrite(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope, uint64_t id)
{
    if (scope->head.biflow_mode != FDS_BW_INDIVIDUAL) {
        mgr->err_msg = "Reverse element, with ID '"+to_string(id)+
                       "' in a scope with PEN '" +to_string(scope->head.pen)+
                       "', can be defined only when scope biflow mode is INDIVIDUAL";
        return false;
    }

    if (id > uint15_limit) {
        mgr->err_msg = "ID '" +to_string(id)+
                       "' of a new reverse element is bigger than limit '" +to_string(uint15_limit);
        return false;
    }

    return true;
}

/**
 * \brief Get reverse element with \p biflow_id
 * \param[in,out] mgr       Manager
 * \param[in,out] scope     Scope
 * \param[in]     elem      Copied element
 * \param[in]     biflow_id New id of the element
 * \return Reverse element on success, otherwise nullptr
 * \note Function already save element to the manager
 */
fds_iemgr_elem *
element_get_reverse(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope, fds_iemgr_elem* elem,
                    uint16_t biflow_id)
{
    if (!element_check_overwrite(mgr, scope, biflow_id)) {
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

/**
 * \brief Overwrite values of the \p dst element with values from the \p src element
 * \param mgr Manager
 * \param scope Scope
 * \param dst Destination element
 * \param src Source element
 * \return True on success, otherwise False
 * \fixme Function doesn't allocate memory for element name, just copy pointer
 */
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
        dst->name            = src->name; // TODO copy memory
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

    scope_sort(scope);
    return true;
}

/**
 * \brief Overwrite reverse element with the \p src if \p id is bigger than 0, new element will have ID \p id
 * \param[in,out] mgr   Manager
 * \param[in,out] scope Scope
 * \param[out]    rev   Reverse element
 * \param[in]     src   Source element
 * \param[in]     id    New ID
 * \return True on success, otherwise False
 */
bool
element_overwrite_reverse(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope, fds_iemgr_elem* rev, fds_iemgr_elem* src, int id)
{
    fds_iemgr_scope_inter* tmp_scope = scope;
    if (scope->head.biflow_mode == FDS_BW_PEN) {
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
        fds_iemgr_elem *tmp = element_get_reverse(mgr, tmp_scope, src, static_cast<uint16_t>(id));
        return tmp != nullptr;
    }

    if (id >= 0) {
        if (src->scope->biflow_mode != FDS_BW_INDIVIDUAL) {
            mgr->err_msg = "Scope with PEN '" +to_string(src->scope->pen)+ "' cannot define biflowID in elements, because it doesn't have biflow mode INDIVIDUAL.";
            return false;
        }

        if (rev->id != id) {
            mgr->err_msg = "Cannot define biflowID to the element with reverse ID '" +to_string(rev->id)+ "' in the scope with PEN '" +to_string(scope->head.pen)+ "' which overwrites previously defined element with same ID.";
            return false;
        }
    }

    src->name = copy_reverse(src->name);
    return element_overwrite_values(mgr, tmp_scope, rev, src);
}

/**
 * \brief Overwrite \p temp with \p res
 * \param[in,out] mgr  Manager
 * \param[out]    dst  Overwritten Element
 * \param[in]     src Temporary element
 * \return True on success, otherwise False
 */
bool
element_overwrite(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope, fds_iemgr_elem* dst,
                  unique_elem src, int biflow_id)
{
    if (!element_can_overwritten(mgr, dst)) {
        return false;
    }

    if (!element_overwrite_values(mgr, scope, dst, src.get())) {
        return false;
    }

    if (!element_overwrite_reverse(mgr, scope, dst->reverse_elem, src.get(), biflow_id)) {
        return false;
    }

    delete src.release();
    return true;
}

/**
 * \brief Write element to the scope and to the manager
 * \param[in,out] mgr   Manager
 * \param[out]    scope Scope
 * \param[in]     elem  Written element
 * \return True on success, otherwise False
 */
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
        elem->reverse_elem = element_get_reverse(mgr, scope, elem.get(),
                                                 static_cast<uint16_t>(biflow_id));
        if (elem->reverse_elem == nullptr) {
            return false;
        }
    }

    return element_save(scope, elem.release());
}

/**
 * \brief Push an element to the scope
 * Try to find the element with same ID in a scope, if success overwrite information in founded
 * element, else check name and data type in element and create new element
 *
 * \param[out]    mgr   Manager
 * \param[in,out] scope Scope
 * \param[in]     elem  Element
 * \return True on success, otherwise False
 */
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

/**
 * \brief Save an element from a context to the manager
 * \param[out] mgr   Manager
 * \param[in]  ctx   Context
 * \param[out] scope Scope
 * \return True on success, otherwise False
 */
bool
element_read(fds_iemgr_t* mgr, fds_xml_ctx_t* ctx, fds_iemgr_scope_inter* scope)
{
    auto elem = unique_elem(element_create(), &::element_remove);
    int biflow_id = -1;

    const struct fds_xml_cont *cont;
    while(fds_xml_next(ctx, &cont) != FDS_XML_EOC) {
        switch (cont->id) {
        case ELEM_ID:
            if (!get_id(mgr, elem->id, cont->val_int)) {
                return false;
            }
            break;
        case ELEM_NAME:
            elem->name = copy_str(cont->ptr_string);
            if (elem->name == nullptr) {
                mgr->err_msg = "Name of the element with ID '" +to_string(elem->id)+
                        "' in scope with PEN '" +to_string(scope->head.pen)+ "' not recognised.";
                return false;
            }
            break;
        case ELEM_DATA_TYPE:
            elem->data_type = get_data_type(cont->ptr_string);
            if (elem->data_type == FDS_ET_UNASSIGNED) {
                mgr->err_msg = "Data type of the element with ID '" +to_string(elem->id)+
                        "' in scope with PEN '" +to_string(scope->head.pen)+ "' not recognised.";
                return false;
            }
            break;
        case ELEM_DATA_SEMAN:
            elem->data_semantic = get_data_semantic(cont->ptr_string);
            if (elem->data_semantic == FDS_ES_UNASSIGNED) {
                mgr->err_msg = "Data semantic of the element with ID '" +to_string(elem->id)+
                        "' in scope with PEN '" +to_string(scope->head.pen)+ "' not recognised.";
                return false;
            }
            break;
        case ELEM_DATA_UNIT:
            elem->data_unit = get_data_unit(cont->ptr_string);
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

/**
 * \brief Save all elements from a context to a manager
 * \param[out] mgr   Manager
 * \param[in]  ctx   Context
 * \param[out] scope Scope
 * \return True on success, otherwise False
 */
bool
elements_read(fds_iemgr_t* mgr, fds_xml_ctx_t* ctx, fds_iemgr_scope_inter* scope)
{
    const struct fds_xml_cont *cont;
    while(fds_xml_next(ctx, &cont) != FDS_XML_EOC) {
        if (cont->id != ELEM) {
            continue;
        }

        if(!element_read(mgr, cont->ptr_ctx, scope)) {
            return false;
        }
    }
    return true;
}

/**
 * \brief Copy reverse elements from \p src to \p dst
 * \param[out] dst Destination scope
 * \param[in]  src Source scope
 * \return True on success, otherwise False
 */
bool
elements_copy_reverse(fds_iemgr_scope_inter* dst, const fds_iemgr_scope_inter* src)
{
    for (auto& elem: src->ids) {
        auto res = element_create_reverse(elem.second, elem.second->id);
        if (res == nullptr) {
            return false;
        }
        res->scope        = &dst->head;
        element_save(dst, res);
    }

    return true;
}

/**
 * \brief Remove all reverse elements from a split scope
 * \param[in,out] mgr   Manager
 * \param[in,out] scope Scope
 * \return True on success, otherwise false
 */
bool
elements_remove_reverse_split(fds_iemgr_scope_inter* scope)
{
    // TODO better
    for (auto iter = scope->names.begin(); iter != scope->names.end(); ) {
        if ((iter.base()->second->id & (1 << scope->head.biflow_id)) != 0) {
            iter = scope->names.erase(iter);
        } else {
            ++iter;
        }
    }

    for (auto iter = scope->ids.begin(); iter != scope->ids.end(); ) {
        if ((iter.base()->second->id & (1 << scope->head.biflow_id)) != 0) {
            element_remove(iter.base()->second);
            iter = scope->ids.erase(iter);
        } else {
            iter.base()->second->reverse_elem = nullptr;
            ++iter;
        }
    }
    return true;
}

bool
elements_remove_reverse(fds_iemgr_scope_inter* scope)
{
    if (scope->head.biflow_mode == FDS_BW_SPLIT) {
        if (!elements_remove_reverse_split(scope)) {
            return false;
        }
    }

    return true;
}

/**
 * \brief Remove an element from all vectors that contains element
 * \param[in,out] mgr Manager
 * \param[in]     pen Scope with the element
 * \param[in]     id  ID of the element
 * \return #FDS_IEMGR_OK on success, otherwise #FDS_IEMGR_ERR_NOMEM or #FDS_IEMGR_ERR
 */
int
element_destroy(fds_iemgr_t *mgr, const uint32_t pen, const uint16_t id)
{
    const auto scope_pen_it = find_iterator(mgr->pens, pen);
    if (scope_pen_it == mgr->pens.end()) {
        return FDS_IEMGR_NOT_FOUND;
    }
    const auto scope = scope_pen_it.base()->second;

    const auto elem_id_it = find_iterator(scope->ids, id);
    if (elem_id_it == scope->ids.end()) {
        return FDS_IEMGR_NOT_FOUND;
    }

    const auto elem = elem_id_it.base()->second;
    const string name = elem->name;

    const auto elem_name_it = find_iterator(scope->names, name);
    if (elem_name_it == scope->names.end()) {
        return FDS_IEMGR_NOT_FOUND;
    }

    if (elem->reverse_elem != nullptr && !elem->is_reverse) {
        const int ret = element_destroy(mgr, elem->reverse_elem->scope->pen,
                                              elem->reverse_elem->id);
        if (ret != FDS_IEMGR_OK) {
            return ret;
        }
    }

    if (scope->ids.empty()) {
        const auto scope_prefix_it = find_iterator(mgr->prefixes, string(scope->head.name));
        if (scope_prefix_it == mgr->prefixes.end()) {
            return FDS_IEMGR_NOT_FOUND;
        }
        scope_remove(scope);
        mgr->pens.erase(scope_pen_it);
        mgr->prefixes.erase(scope_prefix_it);
    }

    scope->ids.erase(elem_id_it);
    scope->names.erase(elem_name_it);
    element_remove(elem);
    return FDS_IEMGR_OK;
}

