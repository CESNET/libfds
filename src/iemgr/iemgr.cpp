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
#include "fds_iemgr_todo_name.h"

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

fds_iemgr_t*
mgr_copy(fds_iemgr_t* mgr)
{
    auto res = unique_mgr(new fds_iemgr_t, &::fds_iemgr_destroy);
    res->err_msg = mgr->err_msg;

    fds_iemgr_scope_inter* scope;
    for (const auto& tmp: mgr->pens) {
        scope = scope_copy(tmp.second);

        res->pens.emplace_back(scope->head.pen, scope);
        res->prefixes.emplace_back(scope->head.name, scope);
    }

    for (const auto& mtime: mgr->mtime) {
//        res->mtime.emplace_back(copy_str(mtime.first), mtime.second);
    }

    return res.release();
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
    if (mgr == nullptr) {
        return;
    }

    for (const auto& mtime: mgr->mtime) {
        free(mtime.first);
    }
    mgr->mtime.clear();
}

void
fds_iemgr_clear(fds_iemgr_t *mgr)
{
    if (mgr == nullptr) {
        return;
    }

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
    mgr = nullptr;
}

/**
 * \brief Sort scopes in a manager
 * \param mgr Manager
 */
void
mgr_sort(fds_iemgr_t* mgr)
{
    sort_vec(mgr->pens);
    sort_vec(mgr->prefixes);

    const auto func_pred = [](const pair<string, timespec>& lhs,
                              const pair<string, timespec>& rhs)
    { return lhs.first < rhs.first; }; // TODO neccessary?
    sort(mgr->mtime.begin(), mgr->mtime.end(), func_pred);
}

/**
 * \brief Remve all temporary vectors etc. from a manager
 * \param mgr Manager
 */
void
mgr_remove_temp(fds_iemgr_t* mgr)
{
    mgr->overwrite_scope.second.clear();
    mgr->parsed_ids.clear();
}

bool
fds_iemgr_compare_timestamps(fds_iemgr *mgr)
{
    if (mgr == nullptr) {
        return false;
    }

    struct stat sb;
    for (const auto& mtime: mgr->mtime) {
        if (stat(mtime.first, &sb) != 0) {
            mgr->err_msg = "Could not read information about the file '" +string(mtime.first)+ "'";
            return false;
        }

        if (mtime.second.tv_sec != sb.st_mtim.tv_sec) { // TODO is it enough?
            return false;
        }
    }

    return true;
}

/**
 * \brief Save scope to the manager
 * \param[in,out] mgr   Manager
 * \param[in]     scope Saved scope
 * \return Saved scope on success, otherwise nullptr
 */
fds_iemgr_scope_inter *
scope_save(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope)
{
    mgr->prefixes.emplace_back(scope->head.name, scope);
    mgr->pens.emplace_back(scope->head.pen, scope);

    mgr_sort(mgr);
    return scope;
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

    if (!elements_copy_reverse(res.get(), scope)) {
        return nullptr;
    }

    return res.release();
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
 * \brief Create reverse copy of a scope in a manager, if exists overwrite
 * \param[in,out] mgr   Manager
 * \param[in]     scope Scope
 * \return True on success, otherwise False
 */
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
    return true;
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
 * \brief Find scope's biflow mode and set in manager in appropriate way
 * \param[in,out] mgr   Manager
 * \param[in]     scope Scope
 * \return True on success, otherwise False
 */
bool
scope_set_biflow(fds_iemgr_t* mgr, fds_iemgr_scope_inter* scope)
{
    if (scope->head.biflow_mode == FDS_BW_PEN) {
        if (!scope_set_biflow_pen(mgr, scope)) {
            return false;
        }
    }
    else if (scope->head.biflow_mode == FDS_BW_SPLIT) {
        if (!scope_set_biflow_split(mgr, scope)) {
            return false;
        }
    }

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

/**
 * \brief Write \p scope to a manager
 * \param[in,out] mgr         Manager
 * \param[in]     scope       Scope
 * \param[in]     biflow_read True if scope biflow parameters were read
 * \return Written scope on success, otherwise nullptr
 */
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

/**
 * \brief Try to find a scope in the manager with same PEN
 * \param[in,out] mgr         Manager
 * \param[in]     scope       Scope
 * \param[in]     biflow_read Biflow was read
 * \return Founded scope if exist, created scope or nullptr on failure
 */
fds_iemgr_scope_inter *
scope_push(fds_iemgr_t* mgr, unique_scope scope, bool biflow_read)
{
    auto res = find_second(mgr->pens, scope->head.pen);
    if (res != nullptr) {
        return scope_overwrite(mgr, res);
    }

    return scope_write(mgr, move(scope), biflow_read);
}

/**
 * \brief Save scope from a context to a manager
 * \param[out] mgr   Manager
 * \param[in]  ctx   Context with scope
 * \param[out] scope Scope
 * \return True on success, otherwise False
 */
bool
scope_read_biflow(fds_iemgr_t* mgr, fds_xml_ctx_t* ctx, struct fds_iemgr_scope_inter* scope)
{
    int64_t id;

    const struct fds_xml_cont *cont;
    while(fds_xml_next(ctx, &cont) != FDS_XML_EOC) {
        switch (cont->id) {
        case BIFLOW_MODE:
            scope->head.biflow_mode  = get_biflow(cont->ptr_string);
            if (scope->head.biflow_mode == FDS_BW_INVALID) {
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

unique_scope
scope_create()
{
    auto scope = unique_scope(new fds_iemgr_scope_inter, &::scope_remove);
    scope->head.name        = nullptr;
    scope->head.pen         = 0;
    scope->head.biflow_id   = 0;
    scope->head.biflow_mode = FDS_BW_INVALID;

    return scope;
}

/**
 * \brief Save scope with elements from a context to a manager
 * \param[out] mgr   Manager
 * \param[in]  ctx   Context
 * \param[out] scope Scope
 * \return Created scope on success, otherwise nullptr
 */
fds_iemgr_scope_inter *
scope_read(fds_iemgr_t* mgr, fds_xml_ctx_t* ctx)
{
    auto scope = scope_create();
    bool biflow_read = false;

    const struct fds_xml_cont *cont;
    while(fds_xml_next(ctx, &cont) != FDS_XML_EOC) {
        switch (cont->id) {
        case SCOPE_PEN:
            if (!get_pen(mgr, scope->head.pen, cont->val_uint)) {
                return nullptr;
            }
            break;
        case SCOPE_NAME:
            if (!strcmp(cont->ptr_string, "")) {
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

/**
 * \brief Find content with scope
 * \param mgr Manager
 * \param ctx Context
 * \return Content with scope on success, otherwise nullptr
 */
const fds_xml_cont *
scope_find_cont(fds_iemgr_t* mgr, fds_xml_ctx_t* ctx)
{
    const struct fds_xml_cont *cont;
    while(fds_xml_next(ctx, &cont) != FDS_XML_EOC) {
        if (cont->id == SCOPE) {
            return cont;
        }
    }
    mgr->err_msg = "Scope must be defined on a top level of the file";
    return nullptr;
}

/**
 * \brief Save scope details from a context to a manager
 * \param[out] mgr Manager
 * \param[in]  ctx Context
 * \return Scope with saved detail on success, on error return nullptr
 */
fds_iemgr_scope_inter *
scope_set(fds_iemgr_t* mgr, fds_xml_ctx_t* ctx)
{
    const fds_xml_cont *cont = scope_find_cont(mgr, ctx);
    if (cont == nullptr) {
        return nullptr;
    }

    fds_xml_rewind(ctx);
    return scope_read(mgr, cont->ptr_ctx);
}

/**
 * \brief Check if scope meets all requirements
 * \param[in,out] mgr   Manager
 * \param[in]     scope Scope
 * \return True on success, otherwise False
 */
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

    fds_iemgr_scope_inter *scope = scope_set(mgr, ctx);
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
        mgr->err_msg = "Folder with relative path '" +string(path)+ "' doesn't exist";
        return false;
    }

    struct dirent *ent;
    while ((ent = readdir(dir.get())) != nullptr) {
        if (ent->d_type != 0x8 /*isFile*/) {
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
    if (mgr == nullptr) {
        return FDS_IEMGR_ERR;
    }
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
    if (mgr == nullptr) {
        return FDS_IEMGR_ERR;
    }
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
    if (mgr == nullptr) {
        return nullptr;
    }

    auto scope = find_second(mgr->pens, pen);
    if (scope == nullptr) {
        return nullptr;
    }

    const auto elem = find_second(scope->ids, id);
    if (elem == nullptr) {
        return nullptr;
    }

    return elem;
}

const fds_iemgr_elem *
fds_iemgr_elem_find_name(fds_iemgr_t *mgr, const char *name)
{
    if (mgr == nullptr) {
        return nullptr;
    }
    if (name == nullptr) {
        mgr->err_msg = "Name of the element and the scope not defined";
        return nullptr;
    }

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

    auto scope = find_second(mgr->prefixes, split.first);
    if (scope == nullptr) {
        return nullptr;
    }

    const auto elem = find_second(scope->names, split.second);
    if (elem == nullptr) {
        return nullptr;
    }

    return elem;
}

int
fds_iemgr_elem_add(fds_iemgr_t *mgr, const fds_iemgr_elem *elem, const uint32_t pen, bool overwrite)
{
    if (mgr == nullptr) {
        return FDS_IEMGR_ERR;
    }
    if (elem == nullptr) {
        mgr->err_msg = "Element what should be added is not defined";
        return FDS_IEMGR_ERR;
    }

    mgr->can_overwrite_elem = overwrite;
    try {
        auto scope = find_second(mgr->pens, pen);
        if (scope == nullptr) {
            scope = new fds_iemgr_scope_inter;
            scope->head.pen = pen;
            mgr->pens.emplace_back(scope->head.pen, scope);
            mgr_sort(mgr);
        }

        auto res = unique_elem(element_copy(scope, elem), &::element_remove);
        if (res->reverse_elem != nullptr) {
            element_push(mgr, scope, move(res), elem->reverse_elem->id);
        } else {
            element_push(mgr, scope, move(res), -1);
        }
    } catch (...) {
        mgr->err_msg = "Error in function 'fds_iemgr_elem_add' while allocating memory for element adding.";
        return FDS_IEMGR_ERR;
    }

    return FDS_IEMGR_OK;
}

int
fds_iemgr_elem_remove(fds_iemgr_t *mgr, const uint32_t pen, const uint16_t id)
{
    if (mgr == nullptr) {
        return FDS_IEMGR_ERR;
    }

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
    if (mgr == nullptr) {
        return nullptr;
    }

    const auto res = find_second(mgr->pens, pen);
    if (res == nullptr) {
        return nullptr;
    }
    return &res->head;
}

const fds_iemgr_scope *
fds_iemgr_scope_find_name(fds_iemgr_t *mgr, const char *name)
{
    if (mgr == nullptr) {
        return nullptr;
    }
    if (name == nullptr) {
        mgr->err_msg = "Name of a scope was not defined.";
        return nullptr;
    }

    const auto res = find_second(mgr->prefixes, string(name));
    if (res == nullptr) {
        return nullptr;
    }
    return &res->head;
}

const char *
fds_iemgr_last_err(const fds_iemgr_t *mgr)
{
    if (mgr == nullptr) {
        return nullptr;
    }

    if (mgr->err_msg.empty()) {
        return "No error";
    }

    return mgr->err_msg.c_str();
}
