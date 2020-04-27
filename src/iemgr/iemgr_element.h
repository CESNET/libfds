/**
 * \file   src/iemgr/iemgr_element.h
 * \author Michal Režňák
 * \brief  Definition of the iemgr element
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

#pragma once

#include "iemgr_common.h"
#include <libfds/iemgr.h>

/**
 * \brief Create element with default parameters
 * \return Created element on success, otherwise nullptr
 */
fds_iemgr_elem *
element_create();

/**
 * \brief Remove element
 * \param[in,out] elem Element
 */
void
element_remove(fds_iemgr_elem* elem);

/**
 * \brief Copy element \p elem to the new element with a scope \p scope
 * \param[in]     scope Scope
 * \param[in]     elem  Element
 * \return True on success, otherwise False
 */
fds_iemgr_elem *
element_copy(fds_iemgr_scope_inter* scope, const fds_iemgr_elem* elem);

/**
 * \brief Create new reverse element and copy properties from \p src
 * \param[in]     src    Source element
 * \param[in]     new_id New ID of an element
 * \return Copied reverse element on success, otherwise nullptr
 * \note Reverse element's scope is set to the same as normal element's scope.
 */
fds_iemgr_elem *
element_create_reverse(fds_iemgr_elem* src, uint16_t new_id);

/**
 * \brief Check if element can be overwritten in a manager
 * \param[in,out] mgr  Manager
 * \param[out]    dst  Destination element
 * \param[in]     elem Element
 * \return True on success, otherwise False
 */
bool
element_can_overwritten(fds_iemgr_t* mgr, const fds_iemgr_elem* dst, const fds_iemgr_elem* elem);

/**
 * \brief Save ID of the element to the scope's vector of IDs
 * and name of the element to the scope's vector of names
 * \param[out] scope Scope
 * \param[in]  elem  Saved element
 * \return True on success, otherwise False
 * \note Scope's vectors are NOT sorted
 */
bool
element_save(fds_iemgr_scope_inter* scope, fds_iemgr_elem* elem);

/**
 * \brief Add reverse element with \p biflow_id
 * \param[in,out] mgr       Manager
 * \param[in,out] scope     Scope
 * \param[in]     elem      Copied element
 * \param[in]     biflow_id New id of the element
 * \return Reverse element on success, otherwise nullptr
 * \note Function save element to the manager
 */
fds_iemgr_elem *
element_add_reverse(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope, fds_iemgr_elem* elem,
                    uint16_t biflow_id);

/**
 * \brief Overwrite values of the \p dst element with values from the \p src element
 * \param mgr Manager
 * \param scope Scope
 * \param dst Destination element
 * \param src Source element
 * \return True on success, otherwise False
 */
bool
element_overwrite_values(fds_iemgr_t *mgr, fds_iemgr_scope_inter *scope, fds_iemgr_elem *dst, fds_iemgr_elem *src);

/**
 * \brief Overwrite reverse element with the \p src
 * If \p id is bigger than 0, new element will have ID \p id
 * \param[in,out] mgr   Manager
 * \param[in,out] scope Scope
 * \param[out]    rev   Reverse element
 * \param[in]     src   Source element
 * \param[in]     id    New ID
 * \return True on success, otherwise False
 */
bool
element_overwrite_reverse(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope, fds_iemgr_elem* rev, fds_iemgr_elem* src, int id);

/**
 * \brief Check conditions if elemnt can overwrite previously defined element
 * \param[in,out] mgr   Manager
 * \param[in]     scope Scope
 * \param[in]     elem  Element
 * \param[in]     id    ID
 * \return True if can, otherwise False
 */
bool
element_check_reverse_param(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope, fds_iemgr_elem* elem,
                            uint64_t id);

/**
 * \brief Overwrite \p temp with \p res
 * \param[in,out] mgr       Manager
 * \param[out]    scope     Scope
 * \param[out]    dst       Overwritten Element
 * \param[in]     src       Temporary element
 * \param[in]     biflow_id ID of a new element
 * \return True on success, otherwise False
 */
bool
element_overwrite(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope, fds_iemgr_elem* dst, unique_elem src, int biflow_id);

/**
 * \brief Check if name and data type of the element are set and save element to the scope.
 * If \p biflow_id is bigger or equal than 0, reverse element is created and saved to the scope.
 * \param[in,out] mgr       Manager
 * \param[out]    scope     Scope
 * \param[in]     elem      Written element
 * \param[in]     biflow_id ID of a new element
 * \return True on success, otherwise False
 */
bool
element_write(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope, unique_elem elem, int biflow_id);

/**
 * \brief Push an element to the scope
 *
 * Try to find the element with same ID in a scope, if success overwrite information in founded
 * element, else check name and data type in element and create new element
 *
 * \param[out]    mgr       Manager
 * \param[in,out] scope     Scope
 * \param[in]     elem      Element
 * \param[in]     biflow_id ID of a new element
 * \return True on success, otherwise False
 */
bool
element_push(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope, unique_elem elem, int biflow_id);

/**
 * \brief Save an element from a context to the manager
 * \param[out] mgr   Manager
 * \param[in]  ctx   Context
 * \param[out] scope Scope
 * \return True on success, otherwise False
 */
bool
element_read(fds_iemgr_t* mgr, fds_xml_ctx_t* ctx, fds_iemgr_scope_inter* scope);

/**
 * \brief Save all elements from a context to a manager
 * \param[out] mgr   Manager
 * \param[in]  ctx   Context
 * \param[out] scope Scope
 * \return True on success, otherwise False
 */
bool
elements_read(fds_iemgr_t* mgr, fds_xml_ctx_t* ctx, fds_iemgr_scope_inter* scope);

/**
 * \brief Copy reverse elements from \p src to \p dst
 * \param[out] dst Destination scope
 * \param[in]  src Source scope
 * \return True on success, otherwise False
 */
bool
elements_copy_reverse(fds_iemgr_scope_inter* dst, const fds_iemgr_scope_inter* src);

/**
 * \brief Remove all reverse elements from a split scope
 * \param[in,out] scope Scope
 * \return True on success, otherwise false
 */
void
elements_remove_reverse_split(fds_iemgr_scope_inter* scope);

/**
 * \brief Remove reverse elements from the scope with biflow mode SPLIT
 * \param[in,out] scope Scope with reverse elements
 */
void
elements_remove_reverse(fds_iemgr_scope_inter* scope);

/**
 * \brief Remove an element from all vectors that contains element
 * \param[in,out] mgr Manager
 * \param[in]     pen Scope with the element
 * \param[in]     id  ID of the element
 * \return FDS_OK on success, otherwise FDS_ERR_NOMEM or FDS_ERR_NOTFOUND
 */
int
element_destroy(fds_iemgr_t *mgr, uint32_t pen, uint16_t id);

int
element_add_alias_ref(fds_iemgr_elem *elem, fds_iemgr_alias *alias);

int
element_add_mapping_ref(fds_iemgr_elem *elem, fds_iemgr_mapping *mapping);
