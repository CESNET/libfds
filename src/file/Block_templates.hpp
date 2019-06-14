/**
 * @file   src/file/Block_templates.hpp
 * @brief  Template manager (header file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   April 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBFDS_BLOCK_TEMPLATES_HPP
#define LIBFDS_BLOCK_TEMPLATES_HPP

#include <cstdint>
#include <memory>
#include <set>
#include <libfds.h>
#include "File_base.hpp"

namespace fds_file {

/**
 * @brief Template manager
 *
 * An instance of this class is able to handle IPFIX (Options) Templates definitions (supports
 * adding/getting/removing) and it's possible to store or load all the Templates from a file.
 */
class Block_templates {
public:
    /// Class constructor of an empty Template manager
    Block_templates();
    /// Class destructor
    ~Block_templates() = default;

    // Copy constructors are disabled by default
    Block_templates(const Block_templates &other) = delete;
    Block_templates &operator=(const Block_templates &other) = delete;

    /**
     * @brief Add a reference to an IE manager and redefine all fields
     *
     * @warning
     *   All references to Template or Snapshots (from get() and snapshot()) are not valid after
     *   the change of the IE manager! User MUST get new references!
     * @note
     *   If the manager has been already configured before, all references to IE definitions will be
     *   overwritten with new ones. If a definition of an IE was previously available in the older
     *   manager and the new manager doesn't include the definition, the definition will be removed.
     * @param[in] mgr Information Element manager (can be nullptr)
     */
    void
    ie_source(const fds_iemgr_t *mgr);

    /**
     * @brief Load IPFIX (Options) Templates stored as a Template Block from a file
     *
     * @warning
     *   All IPFIX (Options) Template stored in the manager will be replaced or removed.
     * @param[in]  fd     File descriptor (must be opened for reading)
     * @param[in]  offset Offset in the file where the start of the Template Block is placed
     * @param[out] sid    Extracted Internal Transport Session ID (can be nullptr)
     * @param[out] odid   Extracted Observation domain ID (ODID) (can be nullptr)
     * @return Size of the block (in bytes)
     * @throw File_exception if the loading operation fails and the object is in an undefined state.
     */
    uint64_t
    load_from_file(int fd, off_t offset, uint16_t *sid = nullptr, uint32_t *odid = nullptr);

    /**
     * @brief Write all IPFIX (Options) Templates as a Template Block to a file
     *
     * The function will create the Template block header with identification of Transport Session
     * ID and ODID to which the templates belongs and dump all IPFIX (Options) Templates currently
     * registered.
     *
     * @param[in] fd     File descriptor (must be opened for writing)
     * @param[in] offset Offset in the file where the Template Block will be placed
     * @param[in] sid    Internal Transport Session ID
     * @param[in] odid   Observation Domain ID (ODID)
     * @return Size of the written block (in bytes)
     * @throw File_exception if the writing operation fails
     */
    uint64_t
    write_to_file(int fd, off_t offset, uint16_t sid, uint32_t odid);

    /**
     * @brief Add a new IPFIX (Options) Template or redefine the current one
     *
     * Template must be encoded the same way as in IPFIX Message i.e. using network byte order.
     * Templates Withdrawal cannot be added!
     * @param[in] type  Template type (#FDS_TYPE_TEMPLATE or #FDS_TYPE_TEMPLATE_OPTS)
     * @param[in] tdata Pointer to the IPFIX (Options) Template.
     *   Expected structure matches ::fds_ipfix_trec (for #FDS_TYPE_TEMPLATE type)
     *   or ::fds_ipfix_opts_trec (for #FDS_TYPE_TEMPLATE_OPTS type)
     * @param[in] tsize Size of the Template
     * @throw File_exception if the Template is malformed
     */
    void
    add(enum fds_template_type type, const uint8_t *tdata, uint16_t tsize);

    /**
     * @brief Get an IPFIX (Options) Template with a given Template ID
     * @param[in] tid Template ID
     * @return Pointer to the Template or nullptr
     */
    const struct fds_template *
    get(uint16_t tid);

    /**
     * @brief Remove an IPFIX (Options) Template with a given Template ID
     * @param[in] tid Template ID
     * @throw File_exception if the Template is not present
     */
    void
    remove(uint16_t tid);

    /**
     * @brief Get a snapshot with all IPFIX (Options) Templates
     * @return Pointer to the snapshot
     */
    const fds_tsnapshot_t *
    snapshot();

    /**
     * @brief Get total number of Templates
     * @return Number
     */
    uint16_t
    count() noexcept {return m_ids.size();};

    /**
     * @brief Remove all IPFIX (Options) Template from the manager
     */
    void
    clear();

private:
    /// Template manager with parsed Templates
    std::unique_ptr<fds_tmgr_t, decltype(&fds_tmgr_destroy)> m_tmgr;
    /// Set of Template IDs in the template manager
    std::set<uint16_t> m_ids;
};

} // namespace

#endif //LIBFDS_BLOCK_TEMPLATES_HPP
