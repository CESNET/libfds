/**
 * \file   src/iemgr/iemgr.cpp
 * \author Michal Režňák
 * \brief  Iemgr API functions implementation
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

#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <set>
#include <memory>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <libfds/xml_parser.h>
#include <libfds/iemgr.h>
#include <cassert>
#include "iemgr_common.h"
#include "iemgr_scope.h"
#include "iemgr_element.h"
#include "iemgr_alias.h"
#include "iemgr_mapping.h"

fds_iemgr_t *
fds_iemgr_create()
{
    fds_iemgr_t *mgr = nullptr;
    try {
        mgr = new fds_iemgr_t;
    } catch (...) {
        return nullptr;
    }

    return mgr;
}

fds_iemgr_t *
fds_iemgr_copy(const fds_iemgr_t *mgr)
{
    if (mgr == nullptr) {
        return nullptr;
    }

    fds_iemgr_t* res;
    try {
        res = mgr_copy(mgr);
    } catch (...) {
        // Allocation error
        return nullptr;
    }

    return res;
}

/**
 * \brief Remove modification time from a manager
 * \param[in,out] mgr Manager
 */
void
mtime_remove(fds_iemgr_t *mgr)
{
    for (const auto& mtime: mgr->mtime) {
        free(mtime.first);
    }
    mgr->mtime.clear();
}

/**
 * \brief Save modification time of a file to the manager
 * \param[in,out] mgr  Manager
 * \param[in]     path Path and name of the file e.g. 'tmp/user/elements/iana.xml'
 * \return True on success, otherwise False
 */
bool
mtime_save(fds_iemgr_t* mgr, const string& path)
{
    char* file_path = realpath(path.c_str(), nullptr);
    if (file_path == nullptr) {
        mgr->err_msg = "Relative path '" + path + "' could not be changed to absolute";
        return false;
    }

    struct stat sb{};
    if (stat(file_path, &sb) != 0) {
        mgr->err_msg = "Could not read information about the file '" + string(file_path) + "'";
        return false;
    }

    mgr->mtime.emplace_back(file_path, sb.st_mtim);
    return true;
}

void
fds_iemgr_clear(fds_iemgr_t *mgr)
{
    assert(mgr != nullptr);

    for (const auto &scope: mgr->pens) {
        scope_remove(scope.second);
    }

    mgr->pens.clear();
    mgr->prefixes.clear();

    mtime_remove(mgr);

    aliases_destroy(mgr);

    mappings_destroy(mgr);
}

void
fds_iemgr_destroy(fds_iemgr_t *mgr)
{
    fds_iemgr_clear(mgr);
    delete mgr;
}

int
fds_iemgr_compare_timestamps(fds_iemgr *mgr)
{
    assert(mgr != nullptr);

    struct stat sb{};
    for (const auto& mtime: mgr->mtime) {
        if (stat(mtime.first, &sb) != 0) {
            mgr->err_msg = "Could not read information about the file '" +string(mtime.first)+ "'";
            return FDS_ERR_FORMAT;
        }

        if (mtime.second.tv_sec != sb.st_mtim.tv_sec) {
            return FDS_ERR_DIFF;
        }
        if (mtime.second.tv_nsec != sb.st_mtim.tv_nsec) {
            return FDS_ERR_DIFF;
        }
    }

    return FDS_OK;
}

/**
 * \brief Read elements defined by parser from a file and save them to a manager
 * \param[out] mgr    Manager
 * \param[in]  file   XML file
 * \param[out] parser Parser with set arguments
 * \return True on success, otherwise False
 */
bool
file_read(fds_iemgr_t* mgr, FILE* file, fds_xml_t* parser)
{
    fds_xml_ctx_t *ctx = fds_xml_parse_file(parser, file, true);
    if (ctx == nullptr) {
        mgr->err_msg = fds_xml_last_err(parser);
        return false;
    }

    fds_iemgr_scope_inter *scope = scope_parse_and_store(mgr, ctx);
    if (scope == nullptr) {
        return false;
    }

    if (!elements_read(mgr, ctx, scope)) {
        return false;
    }

    if (!scope_set_biflow(mgr, scope)) {
        return false;
    }

    mgr->parsed_ids.clear();
    scope_sort(scope);
    return scope_check(mgr, scope);
}

/**
 * \brief Parse file and save scope with all elements to the manager
 * \param[in,out] mgr    Manager
 * \param[in]     parser Parser
 * \param[in]     path   Path to the file
 * \return True if scope and elements where created successfully, otherwise False
 */
bool
file_parse(fds_iemgr_t *mgr, fds_xml_t* parser, const char *path)
{
    auto file = unique_file(fopen(path, "r"), &::fclose);
    if (file == nullptr) {
        mgr->err_msg = "File '" + string(path) + "' could not be found!";
        return false;
    }

    if (!mtime_save(mgr, path)) {
        return false;
    }

    return file_read(mgr, file.get(), parser);
}

/**
 * \brief Read elements defined by XML files and saved them to a manager
 * \param[out] mgr    Manager
 * \param[in]  path   Path to the directory with XML files
 * \param[out] parser Parser with set arguments
 * \param[in]  name   Name of the directory
 * \return True on success, otherwise False
 */
bool
dir_read(fds_iemgr_t* mgr, const char* path, fds_xml_t* parser, const string& name)
{
    const string dir_path = string(path) + "/" +string(name)+ "/elements";

    auto dir = unique_dir(opendir(dir_path.c_str()), &::closedir);
    if (dir == nullptr) {
        mgr->err_msg = "Folder with path '" + dir_path + "' doesn't exist!";
        return false;
    }

    struct dirent *ent;
    while ((ent = readdir(dir.get())) != nullptr) {
        // Determine type of the file
        const string file_path = dir_path +'/'+ string(ent->d_name);
        struct stat file_info;
        std::unique_ptr<char, decltype(&free)> abs_path(realpath(file_path.c_str(), nullptr), &free);
        if (!abs_path || stat(abs_path.get(), &file_info) == -1) {
            mgr->err_msg = "Unable to access file '" + string(abs_path.get()) + "'!";
            return false;
        }

        if ((file_info.st_mode & S_IFMT) != S_IFREG) {
            // Skip non-regular files!
            continue;
        }

        if (ent->d_name[0] == '.') {
            // Skip hidden files!
            continue;
        }

        if (!file_parse(mgr, parser, file_path.c_str())) {
            return false;
        }
    }

    return true;
}

/**
 * \brief Create parser with arguments set to XML file
 * \param[out] mgr Manager
 * \return Parser on success, otherwise nullptr
 */
fds_xml_t *
parser_create(fds_iemgr_t* mgr)
{
    fds_xml_t *parser = fds_xml_create();
    if (!parser) {
        mgr->err_msg = "No memory for creating an XML parser!";
        return nullptr;
    }

    static const struct fds_xml_args args_elem[] = {
        FDS_OPTS_ELEM(ELEM_ID,         "id",            FDS_OPTS_T_INT,   0),
        FDS_OPTS_ELEM(ELEM_NAME,       "name",          FDS_OPTS_T_STRING, FDS_OPTS_P_OPT),
        FDS_OPTS_ELEM(ELEM_DATA_TYPE,  "dataType",      FDS_OPTS_T_STRING, FDS_OPTS_P_OPT),
        FDS_OPTS_ELEM(ELEM_DATA_SEMAN, "dataSemantics", FDS_OPTS_T_STRING, FDS_OPTS_P_OPT),
        FDS_OPTS_ELEM(ELEM_DATA_UNIT,  "units",         FDS_OPTS_T_STRING, FDS_OPTS_P_OPT),
        FDS_OPTS_ELEM(ELEM_STATUS,     "status",        FDS_OPTS_T_STRING, FDS_OPTS_P_OPT),
        FDS_OPTS_ELEM(ELEM_BIFLOW,     "biflowId",      FDS_OPTS_T_INT,   FDS_OPTS_P_OPT),
        FDS_OPTS_END
    };
    static const struct fds_xml_args args_biflow[] = {
        FDS_OPTS_ATTR(BIFLOW_MODE, "mode", FDS_OPTS_T_STRING, 0         ),
        FDS_OPTS_TEXT(BIFLOW_TEXT,         FDS_OPTS_T_INT,   FDS_OPTS_P_OPT),
        FDS_OPTS_END
    };
    static const struct fds_xml_args args_scope[] = {
        FDS_OPTS_ELEM(  SCOPE_PEN,    "pen",    FDS_OPTS_T_INT,   0),
        FDS_OPTS_ELEM(  SCOPE_NAME,   "name",   FDS_OPTS_T_STRING, FDS_OPTS_P_OPT),
        FDS_OPTS_NESTED(SCOPE_BIFLOW, "biflow", args_biflow,   FDS_OPTS_P_OPT),
        FDS_OPTS_END
    };
    static const struct fds_xml_args args_main[] = {
        FDS_OPTS_ROOT(         "ipfix-elements"                                      ),
        FDS_OPTS_NESTED(SCOPE, "scope",         args_scope, 0                        ),
        FDS_OPTS_NESTED(ELEM,  "element",       args_elem,  FDS_OPTS_P_OPT | FDS_OPTS_P_MULTI),
        FDS_OPTS_END
    };

    if (fds_xml_set_args(parser, args_main) != FDS_OK) {
        mgr->err_msg = fds_xml_last_err(parser);
        fds_xml_destroy(parser);
        return nullptr;
    }
    return parser;
}

/**
 * \brief Read elements defined by XML from system/ and user/ folders and saved them to a manager
 * \param[out] mgr    Manager
 * \param[in]  path   Path to the directory with XML files
 * \return True on success, otherwise False
 */
bool
dirs_read(fds_iemgr_t* mgr, const char* path)
{
    auto parser = unique_parser(parser_create(mgr), &::fds_xml_destroy);
    if (parser == nullptr) {
        return false;
    }

    mgr->can_overwrite_elem = true;
    mgr->overwrite_scope.first = false;
    if (!dir_read(mgr, path, parser.get(), "system")) {
        return false;
    }

    mgr->overwrite_scope.first = true;
    if (!dir_read(mgr, path, parser.get(), "user")) {
        return false;
    }

    mgr_remove_temp(mgr);
    return true;
}

/**
 * \brief Check all definitions of a manager and sort manager
 * \param[in,out] mgr Manager
 * \return FDS_OK on success, otherwise FDS_ERR_FORMAT or FDS_ERR_NOMEM
 */
int
mgr_check(fds_iemgr_t *mgr)
{
    mgr_sort(mgr);

    const auto pen_pair_it = find_pair(mgr->pens);
    if (pen_pair_it != mgr->pens.end()) {
        mgr->err_msg = "PEN of a scope with PEN '"
            + to_string(pen_pair_it.base()->second->head.pen)
            + "' is defined multiple times.";
        return FDS_ERR_FORMAT;
    }

    const auto pref_pair_it = find_pair(mgr->prefixes);
    if (pref_pair_it != mgr->prefixes.end()) {
        mgr->err_msg = "Name '"+string(pref_pair_it.base()->second->head.name)
            + "' of a scope is defined multiple times.";
        return FDS_ERR_FORMAT;
    }

    return FDS_OK;
}

int
fds_iemgr_read_dir(fds_iemgr_t *mgr, const char *path)
{
    assert(mgr  != nullptr);
    assert(path != nullptr);

    if (!mgr->pens.empty()) {
        fds_iemgr_clear(mgr);
    }

    try {
        if (!dirs_read(mgr, path)) {
            return FDS_ERR_FORMAT;
        }

        int rc;

        rc = fds_iemgr_read_aliases(mgr, path);
        if (rc != FDS_OK && rc != FDS_ERR_NOTFOUND) {
            return rc;
        }

        rc = fds_iemgr_read_mappings(mgr, path);
        if (rc != FDS_OK && rc != FDS_ERR_NOTFOUND) {
            return rc;
        }
    }
    catch (...) {
        mgr->err_msg = "Error in function 'fds_iemgr_read_dir' while allocating memory for directory reading.";
        return FDS_ERR_NOMEM;
    }

    return mgr_check(mgr);
}

int
fds_iemgr_read_file(fds_iemgr_t *mgr, const char *path, bool overwrite)
{
    assert(mgr  != nullptr);
    assert(path != nullptr);

    mgr->can_overwrite_elem = overwrite;
    mgr->overwrite_scope.first = true;
    try {
        auto parser = unique_parser(parser_create(mgr), &::fds_xml_destroy);
        if (parser == nullptr) {
            return FDS_ERR_FORMAT;
        }

        if (!file_parse(mgr, parser.get(), path)) {
            return FDS_ERR_FORMAT;
        }
    } catch (...) {
        mgr->err_msg = "Error in function 'fds_iemgr_read_file' while allocating memory for file reading.";
        return FDS_ERR_NOMEM;
    }

    mgr_remove_temp(mgr);
    return mgr_check(mgr);
}

const fds_iemgr_elem *
fds_iemgr_elem_find_id(const fds_iemgr_t *mgr, uint32_t pen, uint16_t id)
{
    assert(mgr != nullptr);

    auto scope = binary_find(mgr->pens, pen);
    if (scope == nullptr) {
        return nullptr;
    }

    const auto elem = binary_find(scope->ids, id);
    if (elem == nullptr) {
        return nullptr;
    }

    return elem;
}

const struct fds_iemgr_elem *
fds_iemgr_elem_find_name(const fds_iemgr_t *mgr, const char *name)
{
    assert(mgr  != nullptr);
    assert(name != nullptr);

    pair<string, string> split;
    try {
        if (!split_name(name, split)) {
            // Error: the element name cannot contain the second ":"
            return nullptr;
        }
    } catch (...) {
        // Error: Probably memory allocation error
        return nullptr;
    }

    // FIXME: the scope and IE can be defined as numbers
    auto scope = binary_find(mgr->prefixes, split.first);
    if (scope == nullptr) {
        return nullptr;
    }

    const auto elem = binary_find(scope->names, split.second);
    if (elem == nullptr) {
        return nullptr;
    }

    return elem;
}

int
fds_iemgr_elem_add(fds_iemgr_t *mgr, const struct fds_iemgr_elem *elem, uint32_t pen,
    bool overwrite)
{
    assert(mgr != nullptr);

    if (elem == nullptr) {
        mgr->err_msg = "Element that should be added is not defined";
        return FDS_ERR_FORMAT;
    }

    mgr->can_overwrite_elem = overwrite;
    auto scope = find_second(mgr->pens, pen);
    try {
        if (scope == nullptr) {
            scope = scope_create().release();
            scope->head.pen = pen;
            scope->head.biflow_mode = FDS_BF_INDIVIDUAL;
            mgr->pens.emplace_back(scope->head.pen, scope);
            mgr_sort(mgr);
        }

        auto res = unique_elem(element_copy(scope, elem), &::element_remove);
        if (!element_push(mgr, scope, move(res), BIFLOW_ID_INVALID)) {
            return FDS_ERR_FORMAT;
        }
    } catch (...) {
        mgr->err_msg = "Error in function 'fds_iemgr_elem_add' while allocating memory for element adding.";
        return FDS_ERR_NOMEM;
    }

    scope_sort(scope);
    return FDS_OK;
}

int
fds_iemgr_elem_add_reverse(fds_iemgr_t *mgr, uint32_t pen, uint16_t id, uint16_t new_id,
    bool overwrite)
{
    assert(mgr != nullptr);

    auto scope = binary_find(mgr->pens, pen);
    if (scope == nullptr) {
        mgr->err_msg = "Scope with PEN '" + to_string(pen) + "' cannot be found.";
        return FDS_ERR_NOTFOUND;
    }
    if (scope->head.biflow_mode != FDS_BF_INDIVIDUAL) {
        mgr->err_msg = "Reverse element can be defined only to the scope with INDIVIDUAL biflow mode.";
        return FDS_ERR_FORMAT;
    }

    auto elem = binary_find(scope->ids, id);
    if (elem == nullptr) {
        mgr->err_msg = "Element with ID '" + to_string(id) + "' cannot be found.";
        return FDS_ERR_NOTFOUND;
    }

    if (elem->reverse_elem != nullptr && !overwrite) {
        mgr->err_msg = "Element with ID '" + to_string(id) + "' already has reverse element.";
        return FDS_ERR_FORMAT;
    }

    fds_iemgr_elem* rev = nullptr;
    try {
        rev = element_add_reverse(mgr, scope, elem, new_id);
    }
    catch (...) {
        mgr->err_msg = "Error while allocating memory for creating new reverse element.";
    }
    if (rev == nullptr) {
        return FDS_ERR_NOMEM;
    }

    scope_sort(scope);
    return FDS_OK;
}

int
fds_iemgr_elem_remove(fds_iemgr_t *mgr, uint32_t pen, uint16_t id)
{
    assert(mgr != nullptr);

    try {
        const int ret = element_destroy(mgr, pen, id);
        if (ret != FDS_OK) {
            return ret;
        }
    } catch (...) {
        mgr->err_msg = "Error in function 'fds_iemgr_elem_remove' while removing element.";
        return FDS_ERR_NOMEM;
    }

    return FDS_OK;
}

const struct fds_iemgr_scope *
fds_iemgr_scope_find_pen(const fds_iemgr_t *mgr, uint32_t pen)
{
    assert(mgr != nullptr);

    const auto res = binary_find(mgr->pens, pen);
    if (res == nullptr) {
        return nullptr;
    }
    return &res->head;
}

const fds_iemgr_scope *
fds_iemgr_scope_find_name(const fds_iemgr_t *mgr, const char *name)
{
    assert(mgr != nullptr);
    assert(name != nullptr);

    const auto res = binary_find(mgr->prefixes, string(name));
    if (res == nullptr) {
        return nullptr;
    }
    return &res->head;
}

const char *
fds_iemgr_last_err(const fds_iemgr_t *mgr)
{
    assert(mgr != nullptr);

    if (mgr->err_msg.empty()) {
        return "No error";
    }

    return mgr->err_msg.c_str();
}

/** String representation of ::fds_iemgr_element_type */
static const char *table_type[] = {
    "octetArray",             // 0
    "unsigned8",
    "unsigned16",
    "unsigned32",
    "unsigned64",
    "signed8",                // 5
    "signed16",
    "signed32",
    "signed64",
    "float32",
    "float64",                // 10
    "boolean",
    "macAddress",
    "string",
    "dateTimeSeconds",
    "dateTimeMilliseconds",   // 15
    "dateTimeMicroseconds",
    "dateTimeNanoseconds",
    "ipv4Address",
    "ipv6Address",
    "basicList",              // 20
    "subTemplateList",
    "subTemplateMultiList"
};

/** String representation of ::fds_iemgr_element_semantic */
static const char *table_semantic[] = {
    "default",                // 0
    "quantity",
    "totalCounter",
    "deltaCounter",
    "identifier",
    "flags",                  // 5
    "list",
    "snmpCounter",
    "snmpGauge"
};

/** String representation of ::fds_iemgr_element_unit */
static const char *table_unit[] = {
    "none",                   // 0
    "bits",
    "octets",
    "packets",
    "flows",
    "seconds",                // 5
    "milliseconds",
    "microseconds",
    "nanoseconds",
    "4-octet words",
    "messages",               // 10
    "hops",
    "entries",
    "frames",
    "ports",
    "inferred"                // 15
};

/** Size of table with IE types */
constexpr size_t table_type_size = sizeof(table_type) / sizeof(table_type[0]);
/** Size of table with IE semantics */
constexpr size_t table_semantic_size = sizeof(table_semantic) / sizeof(table_semantic[0]);
/** Size of table with IE units */
constexpr size_t table_unit_size = sizeof(table_unit) / sizeof(table_unit[0]);

const char *
fds_iemgr_type2str(enum fds_iemgr_element_type type)
{
    if (type < table_type_size) {
        return table_type[type];
    } else {
        return NULL;
    }
}

const char *
fds_iemgr_semantic2str(enum fds_iemgr_element_semantic sem)
{
    if (sem < table_semantic_size) {
        return table_semantic[sem];
    } else {
        return NULL;
    }
}

const char *
fds_iemgr_unit2str(enum fds_iemgr_element_unit unit)
{
    if (unit < table_unit_size) {
        return table_unit[unit];
    } else {
        return NULL;
    }
}

enum fds_iemgr_element_type
fds_iemgr_str2type(const char *str)
{
    for (size_t i = 0; i < table_type_size; ++i) {
        if (strcasecmp(str, table_type[i]) != 0) {
            continue;
        }

        return (enum fds_iemgr_element_type) i;
    }

    return FDS_ET_UNASSIGNED;
}

enum fds_iemgr_element_semantic
fds_iemgr_str2semantic(const char *str)
{
    for (size_t i = 0; i < table_semantic_size; ++i) {
        if (strcasecmp(str, table_semantic[i]) != 0) {
            continue;
        }

        return (enum fds_iemgr_element_semantic) i;
    }

    return FDS_ES_UNASSIGNED;
}

enum fds_iemgr_element_unit
fds_iemgr_str2unit(const char *str)
{
    for (size_t i = 0; i < table_unit_size; ++i) {
        if (strcasecmp(str, table_unit[i]) != 0) {
            continue;
        }

        return (enum fds_iemgr_element_unit) i;
    }

    return FDS_EU_UNASSIGNED;
}

int
fds_iemgr_read_aliases(fds_iemgr_t *mgr, const char *dir)
{
    int rc = read_aliases_file(mgr, (std::string(dir) + "system/aliases.xml").c_str());
    return rc;
}

const struct fds_iemgr_alias *
fds_iemgr_alias_find(const fds_iemgr_t *mgr, const char *aliased_name)
{
    return binary_find(mgr->aliased_names, std::string(aliased_name));
}

int
fds_iemgr_read_mappings(fds_iemgr_t *mgr, const char *dir)
{
    int rc = read_mappings_file(mgr, (std::string(dir) + "system/mappings.xml").c_str());
    return rc;
}

const struct fds_iemgr_mapping_item *
fds_iemgr_mapping_find(const fds_iemgr_t *mgr, const char *name, const char *key)
{
    const fds_iemgr_mapping_item *item;

    const fds_iemgr_alias *alias = fds_iemgr_alias_find(mgr, name);
    if (alias != nullptr) {
        for (size_t i = 0; i < alias->sources_cnt; i++) {
            item = find_mapping_in_elem(alias->sources[i], key);
            if (item != nullptr) {
                return item;
            }
        }
        }

    const fds_iemgr_elem *elem = fds_iemgr_elem_find_name(mgr, name);
    if (elem != nullptr) {
        item = find_mapping_in_elem(elem, key);
        if (item != nullptr) {
            return item;
        }
        }

    return nullptr;
}
