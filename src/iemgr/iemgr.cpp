/**
 * \author Michal Režňák
 * \date   8. August 2017
 */

#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <set>
#include <bits/unique_ptr.h>
#include <sys/stat.h>
#include <libfds/xml_parser.h>
#include <libfds/iemgr.h>
#include <assert.h>
#include "iemgr_common.h"
#include "iemgr_scope.h"
#include "iemgr_element.h"

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
fds_iemgr_copy(fds_iemgr_t *mgr)
{
    if (mgr == nullptr) {
        return nullptr;
    }

    fds_iemgr_t* res;
    try {
        res = mgr_copy(mgr);
    }
    catch (...) {
        mgr->err_msg = "Allocation error while copying manager";
        return nullptr;
    }

    return res;
}

/**
 * \brief Remove modification time from a manager
 * \param[in,out] mgr Manager
 */
void mtime_remove(fds_iemgr_t *mgr)
{
    for (const auto& mtime: mgr->mtime) {
        free(mtime.first);
    }
    mgr->mtime.clear();
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

    struct stat sb;
    for (const auto& mtime: mgr->mtime) {
        if (stat(mtime.first, &sb) != 0) {
            mgr->err_msg = "Could not read information about the file '" +string(mtime.first)+ "'";
            return FDS_IEMGR_ERR;
        }

        if (mtime.second.tv_sec != sb.st_mtim.tv_sec) {
            return FDS_IEMGR_DIFF_MTIME;
        }
        if (mtime.second.tv_nsec != sb.st_mtim.tv_nsec) {
            return FDS_IEMGR_DIFF_MTIME;
        }
    }

    return FDS_IEMGR_OK;
}

/**
 * \brief Save modification time of a file to the manager
 * \param[in,out] mgr  Manager
 * \param[in]     path Path and name of the file e.g. 'tmp/user/elements/iana.xml'
 * \return True on success, otherwise False
 */
// TODO change file_path to string, not necessary
bool
mtime_save(fds_iemgr_t* mgr, const string& path)
{
    char* file_path = realpath(path.c_str(), nullptr);
    if (file_path == nullptr) {
        mgr->err_msg = "Relative path '"+path+ "' could not be changed to absolute";
        return false;
    }

    struct stat sb;
    if (stat(file_path, &sb) != 0) {
        mgr->err_msg = "Could not read information about the file '" +string(file_path)+ "'";
        return false;
    }

    mgr->mtime.emplace_back(file_path, sb.st_mtim);
    return true;
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
 * \brief Parse file to the manager
 * \param[in,out] mgr    Manager
 * \param[in]     parser Parser
 * \param[in]     path   Path to the file
 * \return True on success, otherwise False
 */
bool
file_parse(fds_iemgr_t *mgr, fds_xml_t* parser, const char *path)
{
    auto file = unique_file(fopen(path, "r"), &::fclose);
    if (file == nullptr) {
        mgr->err_msg = "File '" +string(path)+ "' could not be found";
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
        mgr->err_msg = "Folder with path '" +string(path)+ "' doesn't exist";
        return false;
    }

    struct dirent *ent;
    while ((ent = readdir(dir.get())) != nullptr) {
        if (ent->d_type != DT_REG) {
            continue;
        }
        if (ent->d_name[0] == '.') {
            continue;
        }

        const string file_path = dir_path +'/'+ string(ent->d_name);
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
    fds_xml_t *parser = nullptr;
    if (fds_xml_create(&parser) != FDS_XML_OK) {
        mgr->err_msg = "No memory for creating a parser in fds_iemgr_read_dir";
        return nullptr;
    }

    static const struct fds_xml_args args_elem[] = {
        OPTS_ELEM(ELEM_ID,         "id",            OPTS_T_INT,   0),
        OPTS_ELEM(ELEM_NAME,       "name",          OPTS_T_STRING, OPTS_P_OPT),
        OPTS_ELEM(ELEM_DATA_TYPE,  "dataType",      OPTS_T_STRING, OPTS_P_OPT),
        OPTS_ELEM(ELEM_DATA_SEMAN, "dataSemantics", OPTS_T_STRING, OPTS_P_OPT),
        OPTS_ELEM(ELEM_DATA_UNIT,  "units",         OPTS_T_STRING, OPTS_P_OPT),
        OPTS_ELEM(ELEM_STATUS,     "status",        OPTS_T_STRING, OPTS_P_OPT),
        OPTS_ELEM(ELEM_BIFLOW,     "biflowId",      OPTS_T_INT,   OPTS_P_OPT),
        OPTS_END
    };
    static const struct fds_xml_args args_biflow[] = {
        OPTS_ATTR(BIFLOW_MODE, "mode", OPTS_T_STRING, 0         ),
        OPTS_TEXT(BIFLOW_TEXT,         OPTS_T_INT,   OPTS_P_OPT),
        OPTS_END
    };
    static const struct fds_xml_args args_scope[] = {
        OPTS_ELEM(  SCOPE_PEN,    "pen",    OPTS_T_INT,   0),
        OPTS_ELEM(  SCOPE_NAME,   "name",   OPTS_T_STRING, OPTS_P_OPT),
        OPTS_NESTED(SCOPE_BIFLOW, "biflow", args_biflow,   OPTS_P_OPT),
        OPTS_END
    };
    static const struct fds_xml_args args_main[] = {
        OPTS_ROOT(         "ipfix-elements"                                      ),
        OPTS_NESTED(SCOPE, "scope",         args_scope, 0                        ),
        OPTS_NESTED(ELEM,  "element",       args_elem,  OPTS_P_OPT | OPTS_P_MULTI),
        OPTS_END
    };

    if (fds_xml_set_args(args_main, parser) != FDS_XML_OK) {
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
 * \param[out] parser Parser with set arguments
 * \param[in]  name   Name of the directory
 * \return True on success, otherwise False
 */
bool
dirs_read(fds_iemgr_t* mgr, const char* path)
{
    auto parser = unique_parser(parser_create(mgr), &::fds_xml_destroy);
    if (parser == nullptr) {
        return FDS_IEMGR_ERR;
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
 * \return #FDS_IEMGR_OK on success, otherwise #FDS_IEMGR_ERR or #FDS_IEMGR_NO_MEM
 */
int
mgr_check(fds_iemgr_t *mgr)
{
    mgr_sort(mgr);

    const auto pen_pair_it = find_pair(mgr->pens);
    if (pen_pair_it != mgr->pens.end()) {
        mgr->err_msg = "PEN of a scope with PEN '" +to_string(pen_pair_it.base()->second->head.pen)+ "' is defined multiple times.";
        return FDS_IEMGR_ERR;
    }

    const auto pref_pair_it = find_pair(mgr->prefixes);
    if (pref_pair_it != mgr->prefixes.end()) {
        mgr->err_msg = "Name '"+string(pref_pair_it.base()->second->head.name)+ "' of a scope is defined multiple times.";
        return FDS_IEMGR_ERR;
    }

    return FDS_IEMGR_OK;
}

int
fds_iemgr_read_dir(fds_iemgr_t *mgr, const char *path)
{
    assert(mgr != nullptr);

    if (path == nullptr) {
        mgr->err_msg = "No directory specified in fds_iemgr_read_dir";
        return FDS_IEMGR_ERR;
    }

    if (!mgr->pens.empty()) {
        fds_iemgr_clear(mgr);
    }

    try {
        if (!dirs_read(mgr, path)) {
            return FDS_IEMGR_ERR;
        }
    }
    catch (...) {
        mgr->err_msg = "Error in function 'fds_iemgr_read_dir' while allocating memory for directory reading.";
        return FDS_IEMGR_ERR;
    }

    return mgr_check(mgr);
}

int
fds_iemgr_read_file(fds_iemgr_t *mgr, const char *path, bool overwrite)
{
    assert(mgr != nullptr);

    if (path == nullptr) {
        mgr->err_msg = "No file specified in fds_iemgr_read_file";
        return FDS_IEMGR_ERR;
    }

    mgr->can_overwrite_elem = overwrite;
    mgr->overwrite_scope.first = true;
    try {
        auto parser = unique_parser(parser_create(mgr), &::fds_xml_destroy);
        if (parser == nullptr) {
            return false;
        }

        if (!file_parse(mgr, parser.get(), path)) {
            return FDS_IEMGR_ERR;
        }
    } catch (...) {
        mgr->err_msg = "Error in function 'fds_iemgr_read_file' while allocating memory for file reading.";
        return FDS_IEMGR_ERR;
    }

    mgr_remove_temp(mgr);
    return mgr_check(mgr);
}

const fds_iemgr_elem *
fds_iemgr_elem_find_id(fds_iemgr_t *mgr, const uint32_t pen, const uint16_t id)
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

const fds_iemgr_elem *
fds_iemgr_elem_find_name(fds_iemgr_t *mgr, const char *name)
{
    assert(mgr  != nullptr);
    assert(name != nullptr);

    pair<string, string> split;
    try {
        if (!split_name(name, split)){
            mgr->err_msg = "Element name " +string(name)+ " cannot contain second ':'";
            return nullptr;
        }
    } catch (...) {
        mgr->err_msg = "Error in function 'fds_iemgr_elem_find_name' while allocating memory for string splitting.";
        return nullptr;
    }

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
fds_iemgr_elem_add(fds_iemgr_t *mgr, const fds_iemgr_elem *elem, const uint32_t pen, bool overwrite)
{
    assert(mgr != nullptr);

    if (elem == nullptr) {
        mgr->err_msg = "Element that should be added is not defined";
        return FDS_IEMGR_ERR;
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
        if (!element_push(mgr, scope, move(res), -1)) {
            return FDS_IEMGR_ERR;
        }
    } catch (...) {
        mgr->err_msg = "Error in function 'fds_iemgr_elem_add' while allocating memory for element adding.";
        return FDS_IEMGR_ERR;
    }

    scope_sort(scope);
    return FDS_IEMGR_OK;
}

int
fds_iemgr_elem_add_reverse(fds_iemgr_t *mgr, const uint32_t pen, const uint16_t id,
                           const uint16_t new_id, bool overwrite)
{
    assert(mgr != nullptr);

    auto scope = binary_find(mgr->pens, pen);
    if (scope == nullptr) {
        mgr->err_msg = "Scope with PEN '" +to_string(pen)+ "' cannot be found.";
        return FDS_IEMGR_NOT_FOUND;
    }
    if (scope->head.biflow_mode != FDS_BF_INDIVIDUAL) {
        mgr->err_msg = "Reverse element can be defined only to the scope with INDIVIDUAL biflow mode.";
        return FDS_IEMGR_ERR;
    }

    auto elem = binary_find(scope->ids, id);
    if (elem == nullptr) {
        mgr->err_msg = "Element with ID '" +to_string(id)+ "' cannot be found.";
        return FDS_IEMGR_NOT_FOUND;
    }

    if (elem->reverse_elem != nullptr && !overwrite) {
        mgr->err_msg = "Element with ID '" +to_string(id)+ "' already has reverse element.";
        return FDS_IEMGR_ERR;
    }

    fds_iemgr_elem* rev = nullptr;
    try {
        rev = element_add_reverse(mgr, scope, elem, new_id);
    }
    catch (...) {
        mgr->err_msg = "Error while allocating memory for creating new reverse element.";
    }
    if (rev == nullptr) {
        return FDS_IEMGR_ERR;
    }

    scope_sort(scope);
    return FDS_IEMGR_OK;
}

int
fds_iemgr_elem_remove(fds_iemgr_t *mgr, const uint32_t pen, const uint16_t id)
{
    assert(mgr != nullptr);

    try {
        const int ret = element_destroy(mgr, pen, id);
        if (ret != FDS_IEMGR_OK) {
            return ret;
        }
    } catch (...) {
        mgr->err_msg = "Error in function 'fds_iemgr_elem_remove' while removing element.";
        return FDS_IEMGR_ERR;
    }

    return FDS_IEMGR_OK;
}

const fds_iemgr_scope *
fds_iemgr_scope_find_pen(fds_iemgr_t *mgr, const uint32_t pen)
{
    assert(mgr != nullptr);

    const auto res = binary_find(mgr->pens, pen);
    if (res == nullptr) {
        return nullptr;
    }
    return &res->head;
}

const fds_iemgr_scope *
fds_iemgr_scope_find_name(fds_iemgr_t *mgr, const char *name)
{
    assert(mgr != nullptr);

    if (name == nullptr) {
        mgr->err_msg = "Name of a scope was not defined.";
        return nullptr;
    }

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
