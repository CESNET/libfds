/**
 * \author Michal Režňák
 * \date   11.8.17
 */
#pragma once

#include "iemgr_common.h"
#include <libfds/iemgr.h>

/**
 * \brief Clear and delete scope
 * \param[in,out] scope Scope
 */
void
scope_remove(fds_iemgr_scope_inter* scope);

/**
 * \brief Sort elements in a scope
 * \param scope Scope
 */
void
scope_sort(fds_iemgr_scope_inter* scope);

/**
 * \brief Remove elements from a scope
 * \param scope Scope
 */
void
scope_remove_elements(fds_iemgr_scope_inter* scope);

/**
 * \brief Copy scope \p scope
 * \param[in] scope Source scope
 * \return Copied scope on success, otherwise nullptr
 */
fds_iemgr_scope_inter*
scope_copy(const fds_iemgr_scope_inter* scope);

/**
 * \brief Create reverse scope
 * \param[in] scope Scope
 * \return Reverse scope on success, otherwise nullptr
 */
fds_iemgr_scope_inter*
scope_create_reverse(const fds_iemgr_scope_inter* scope);

// TODO
unique_scope
scope_create();

/**
 * \brief Save reverse elements of elements from \p src to dst
 * \param[in]  scope Source scope
 * \return True on success, otherwise False
 * \warning Scope cannot contain reverse elements already
 */
bool
scope_save_reverse_elem(fds_iemgr_scope_inter *scope);

/**
 * \brief Overwrite reverse scope
 * \param[in,out] mgr   Manager
 * \param[in]     scope Scope with elements that will be written to reverse scope
 * \return True on success, otherwise False
 */
bool
scope_set_biflow_overwrite(fds_iemgr_t* mgr, const fds_iemgr_scope_inter* scope);

/**
 * \brief Split scope and create reverse elements
 * \param[in,out] mgr   Manager
 * \param[in,out] scope Scope
 * \return True on success, otherwise False
 */
bool
scope_set_biflow_split(fds_iemgr_t *mgr, fds_iemgr_scope_inter *scope);

/**
 * \brief Overwrite \p src with \p temp
 * \param mgr Manager
 * \param scope Scope
 * \return Overwritten scope on success, otherwise nullptr
 */
fds_iemgr_scope_inter *
scope_overwrite(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope);

/**
 * \brief Save scope to the manager
 * \param[in,out] mgr   Manager
 * \param[in]     scope Saved scope
 * \return Saved scope on success, otherwise nullptr
 */
fds_iemgr_scope_inter *
scope_save(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope);

/**
 * \brief Create reverse copy of a scope in a manager, if exists overwrite
 * \param[in,out] mgr   Manager
 * \param[in]     scope Scope
 * \return True on success, otherwise False
 */
bool
scope_set_biflow_pen(fds_iemgr_t* mgr, const fds_iemgr_scope_inter* scope);

/**
 * \brief Find scope's biflow mode and set in manager in appropriate way
 * \param[in,out] mgr   Manager
 * \param[in]     scope Scope
 * \return True on success, otherwise False
 * \note Scope must be sorted, because when creating biflow 'pen' scope it only copy elements
 */
bool
scope_set_biflow(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope);

/**
 * \brief Write \p scope to a manager
 * \param[in,out] mgr         Manager
 * \param[in]     scope       Scope
 * \param[in]     biflow_read True if scope biflow parameters were read
 * \return Written scope on success, otherwise nullptr
 */
fds_iemgr_scope_inter *
scope_write(fds_iemgr_t* mgr, unique_scope scope, bool biflow_read);

/**
 * \brief Try to find a scope in the manager with same PEN
 * \param[in,out] mgr         Manager
 * \param[in]     scope       Scope
 * \param[in]     biflow_read Biflow was read
 * \return Founded scope if exist, created scope or nullptr on failure
 */
fds_iemgr_scope_inter *
scope_push(fds_iemgr_t* mgr, unique_scope scope, bool biflow_read);

/**
 * \brief Save scope from a context to a manager
 * \param[out] mgr   Manager
 * \param[in]  ctx   Context with scope
 * \param[out] scope Scope
 * \return True on success, otherwise False
 */
bool
scope_read_biflow(fds_iemgr_t* mgr, fds_xml_ctx_t* ctx, struct fds_iemgr_scope_inter* scope);

/**
 * \brief Save scope with elements from a context to a manager
 * \param[out] mgr   Manager
 * \param[in]  ctx   Context
 * \param[out] scope Scope
 * \return Created scope on success, otherwise nullptr
 */
fds_iemgr_scope_inter *
scope_read(fds_iemgr_t* mgr, fds_xml_ctx_t* ctx);

/**
 * \brief Find content with scope
 * \param mgr Manager
 * \param ctx Context
 * \return Content with scope on success, otherwise nullptr
 */
const fds_xml_cont *
scope_find_cont(fds_iemgr_t* mgr, fds_xml_ctx_t* ctx);

/**
 * \brief Save scope details from a context to a manager
 * \param[out] mgr Manager
 * \param[in]  ctx Context
 * \return Scope with saved detail on success, on error return nullptr
 */
fds_iemgr_scope_inter *
scope_parse_and_store(fds_iemgr_t* mgr, fds_xml_ctx_t* ctx);

/**
 * \brief Check if scope meets all requirements
 * \param[in,out] mgr   Manager
 * \param[in]     scope Scope
 * \return True on success, otherwise False
 */
bool
scope_check(fds_iemgr_t* mgr, const fds_iemgr_scope_inter* scope);
