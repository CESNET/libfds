/**
 * \file   src/iemgr/iemgr_common.h
 * \author Michal Režňák
 * \brief  Internal common definitions
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

/**
 * This only remove warning when compiling with gcc. Should be fixed soon
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=69884
 */
#pragma GCC diagnostic ignored "-Wignored-attributes"

#include <iostream>
#include <vector>
#include <cstring>
#include <limits>
#include <algorithm>
#include <set>
#include <memory>
#include <unordered_set>
#include <functional>
#include <sys/stat.h>
#include <libfds/iemgr.h>
#include <libfds/xml_parser.h>
#include <dirent.h>

using std::vector;
using std::pair;
using std::string;
using std::set;
using std::sort;
using std::move;
using std::to_string;

#define ERRMSG_NOMEM    (string("Cannot allocate memory at ") + __FILE__ + ":" + to_string(__LINE__))   

/** Maximal value of the uint32                                  */
#define UINT32_LIMIT      (std::numeric_limits<uint32_t>::max()     )
/** Maximal value of the uint15                                  */
#define UINT15_LIMIT      (std::numeric_limits<uint16_t>::max() >> 1)
/** Postfix to the name of the reverse element                   */
#define REVERSE_NAME      ("@reverse")
/** Define when invalid biflow ID is defined                     */
#define BIFLOW_ID_INVALID (-1)
/** Defines one bit which in normal elements must be 0 and in reverse elements must be 1 */
#define SPLIT_BIT         (1 << (scope->head.biflow_id -1))

/** Saved elements with same PEN and same PREFIX                                          */
struct fds_iemgr_scope_inter {
    struct fds_iemgr_scope                      head;       /**< Scope head               */
    vector <pair <uint16_t, fds_iemgr_elem *> > ids;        /**< Elements sorted by ID    */
    vector <pair <string,   fds_iemgr_elem *> > names;      /**< Elements sorted by NAME  */
    bool                                        is_reverse; /**< True if scope is reverse */
};

/** Saved elements and sorted pointers by different values (PEN, PREFIX, etc.) */
struct fds_iemgr {
    /** Error message              */
    string                         err_msg;
    /** First is absolute path to the file and second is modification time */
    vector<pair<char*, timespec> > mtime;
    /**
     * First is scope's PEN and second points to the scope with that PEN.
     * PENs are sorted from the lowest value to the highest.
     */
    vector<pair<uint32_t, fds_iemgr_scope_inter *> > pens;
    /**
     * First is scope's PREFIX and second points to the scope with that PREFIX.
     * Prefixes are sorted alphabetically.
     */
    vector<pair<string,   fds_iemgr_scope_inter *> > prefixes;

    /**
     * A flat vector of all aliases
     */ 
    vector<fds_iemgr_alias *> aliases;
    /**
     * A flat vector of all mappings
     */
    vector<fds_iemgr_mapping *> mappings;

    /**
     * First is name of the alias, second points to the alias with that name.
     * Names are sorted alphabetically.
     */
    vector<pair<string, fds_iemgr_alias *> > aliased_names;
    /**
     * First is match name of the mapping, second is the mapping.
     * Sorted alphabetically by match name.
     */
    vector<pair<string, struct fds_iemgr_mapping *> > mapped_names; 

    /**
     * These are used only as a temporary values for overwriting.
     * On the end of the parsing should be empty
     */
    /** IDs of parsed elements from one file **/
    set<uint16_t>              parsed_ids;
    /** Define if can overwrite elements */
    bool                       can_overwrite_elem;
    /** First define if can overwrite scope and second are PENs of overwritten scopes */
    pair<bool, set<uint32_t> > overwrite_scope;
};

/** IDs of XML args */
enum FDS_XML_ID {
    SCOPE,
    SCOPE_PEN,
    SCOPE_NAME,
    SCOPE_BIFLOW,
    BIFLOW_MODE,
    BIFLOW_TEXT,
    ELEM,
    ELEM_ID,
    ELEM_NAME,
    ELEM_DATA_TYPE,
    ELEM_DATA_SEMAN,
    ELEM_DATA_UNIT,
    ELEM_STATUS,
    ELEM_BIFLOW,
    ELEM_ALIAS,
    ELEM_SOURCE,
    SOURCE_MODE,
    SOURCE_ID,
    GROUP,
    GROUP_NAME,
    GROUP_MATCH,
    GROUP_ITEM_LIST,
    ITEM_LIST_MODE,
    ITEM_LIST_ITEM,
    ITEM_KEY,
    ITEM_VALUE,
};

/** \cond DOXYGEN_SKIP_THIS */
/** These declarations must be here because unique pointers use them */
void scope_remove(fds_iemgr_scope_inter* scope);
void element_remove(fds_iemgr_elem* elem);

using unique_elem   = std::unique_ptr<fds_iemgr_elem,        decltype(&::element_remove)>;
using unique_scope  = std::unique_ptr<fds_iemgr_scope_inter, decltype(&::scope_remove)>;
using unique_dir    = std::unique_ptr<DIR,                   decltype(&::closedir)>;
using unique_file   = std::unique_ptr<FILE,                  decltype(&::fclose)>;
using unique_parser = std::unique_ptr<fds_xml_t,             decltype(&::fds_xml_destroy)>;
using unique_mgr    = std::unique_ptr<fds_iemgr_t,           decltype(&::fds_iemgr_destroy)>;
/** \endcond */

/**
 * \brief Find iterator in vector
 * \tparam     P      Vector type
 * \tparam     T      Value type same as type of the first element of the vector
 * \param[in]  vector Vector
 * \param[in]  value  Searched value
 * \return Location of founded value in vector on success, otherwise end of the vector
 */

template<typename P, typename T>
typename P::iterator find_iterator(P& vector, const T value)
{
    auto it = std::find_if(vector.begin(), vector.end(), [&](typename P::value_type& ref)
                { return ref.first == value; });

    if (it == vector.end()) {
        return vector.end();
    }
    if (value < it.base()->first) {
        return vector.end();
    }
    return it;
}

/**
 * \brief Find the value in a vector
 * \tparam    P      Vector type
 * \tparam    T      Value type same as type of the first element of the vector
 * \param[in] vector Vector
 * \param[in] value  Searched value
 * \return First element of the founded vector on success, otherwise ??
 */
template<typename P, typename T>
auto find_first(P& vector, const T value) -> decltype(&vector.begin().base()->first)
{
    const auto it = find_iterator(vector, value);
    if (it == vector.end()) {
        return nullptr;
    }

    return &it.base()->first;
}
/**
 * \brief Find the value in a vector
 * \tparam    P      Vector type
 * \tparam    T      Value type same as type of the first element of the vector
 * \param[in] vector Vector
 * \param[in] value  Searched value
 * \return Second element of the founded vector on success, otherwise nullptr
 */
template<typename P, typename T>
auto find_second(P& vector, const T value) -> decltype(vector.begin().base()->second)
{
    const auto it = find_iterator(vector, value);
    if (it == vector.end()) {
        return nullptr;
    }

    return it.base()->second;
}

/**
 * \brief Compare first member of a pair with some other value
 * \tparam V Type of a value
 * \tparam P Type of a pair with first member of same type as \p value
 */
template<typename V, typename P>
struct func_pred_pair
{
    /**
     * \brief Compare first element of the pair with a value
     * \param[in] value Value
     * \param[in] pair  Pair
     * \return Comparison between \p value and first element of \p pair
     */
    bool operator() (const V value, const pair<V, P>& pair) const {
        return value < pair.first;
    }

    /**
     * \brief Compare first element of the pair with a value
     * \param[in] pair  Pair
     * \param[in] value Value
     * \return Comparison between \p value and first element of \p pair
     */
    bool operator() (const pair<V, P>& pair, const V value) const {
        return pair.first < value;
    }
};

/**
 * \brief Find the value in a vector of pair
 * \tparam    P      Vector type
 * \tparam    T      Value type same as type of the first element of the vector
 * \param[in] vector Vector
 * \param[in] value  Searchde value
 * \return Second element of the founded vector on success, otherwise nullptr
 * \warning Binary find algorithm is used thus vector must be sorted
 */
template<typename P, typename T>
auto binary_find(P& vector, const T value) -> decltype(vector.begin().base()->second)
{
    auto it = std::lower_bound(vector.begin(), vector.end(), value,
                               func_pred_pair<T, decltype(vector.begin().base()->second)>());
    if (it == vector.end()) {
        return nullptr;
    }
    if (value < it.base()->first) {
        return nullptr;
    }

    return it.base()->second;
}

/**
 * \brief Find two elements with same first value
 * \tparam P Vector type
 * \param vector Vector
 * \return Const iterator to first element when is defined two times
 * \warning Vector must be sorted
 */
template <typename P>
typename P::const_iterator find_pair(const P& vector)
{
    const auto Func = [](const typename P::value_type& lhs, const typename P::value_type& rhs)
    { return (lhs.first == rhs.first); };

    const auto it = std::adjacent_find(vector.begin(), vector.end(), Func);
    return it;
}

/**
 * \brief Sort elements in a vector
 * \tparam        P      Vector type
 * \param[in,out] vector Unsorted vector
 */
template <typename P>
void sort_vec(P& vector)
{
    sort(vector.begin(), vector.end());
}

template <typename T>
T *
array_push(T **items, std::size_t *items_cnt_ptr)
{
    void *tmp = std::realloc(*items, sizeof(T) * ((*items_cnt_ptr) + 1));
    if (tmp == NULL) {
        return NULL;
    }
    *items = (T *) tmp;
    *items_cnt_ptr += 1;
    T *last_item = (T *) &((*items)[*items_cnt_ptr - 1]);
    return last_item;
}

template <typename T>
T *
copy_flat_array(T *items, std::size_t items_cnt)
{
    void *tmp = std::malloc(sizeof(T) * items_cnt);
    if (tmp == nullptr) {
        return nullptr;
    }
    std::memcpy(tmp, (void *) items, sizeof(T) * items_cnt);
    return (T *) tmp;
}

/**
 * \brief Checks if the supplied string is a valid name.
 * A name must consist of only alphanumeric characters and underscores while
 * not starting with a number.
 * \param str  The text string to check.
 * \return true or false.
 */
bool
check_valid_name(const char *str);

/**
 * \brief Split string to prefix and suffix
 * \param str String with prefix and suffix separated by ':'
 * \param res pair with splitted strings
 * \return True on success, otherwise False
 */
bool
split_name(const string& str, pair<string, string>& res);

/**
 * \brief Check if ID of an element was not previously parsed, if not save to the manager
 * \param[in,out] mgr   Manager
 * \param[in]     scope Scope
 * \param[in]     id    ID of the parsed element
 * \return True if not found, otherwise False
 */
bool
parsed_id_save(fds_iemgr_t* mgr, const fds_iemgr_scope_inter* scope, uint16_t id);

char *
copy_reverse(const char *str);

/**
 * \brief Create new string and copy contain of \p str
 * \param[in] str String
 * \return Copied string on success, otherwise nullptr
 */
char *
copy_str(const char *str);

/**
 * \brief Get element status
 * \param[in] status Element status
 * \return Element status
 */
fds_iemgr_element_status
get_status(const char *status);

/**
 * \brief Get biflow mode
 * \param[in]  mode Biflow mode in a string
 * \return Biflow mode
 */
fds_iemgr_element_biflow
get_biflow(const char *mode);

/**
 * \brief Check biflow ID and return converted ID
 * \param[in,out] mgr   Manager
 * \param[in]     scope Scope
 * \param[in]     id    ID
 * \return Return ID on success, otherwise return -1
 */
int64_t
get_biflow_id(fds_iemgr_t* mgr, const fds_iemgr_scope_inter* scope, int64_t id);

/**
 * \brief Get ID of the element from the value
 * \param[in,out] mgr     Manager
 * \param[out]    elem_id Element ID
 * \param[in]     val     Value
 * \return True on success, otherwise False
 */
bool
get_id(fds_iemgr_t *mgr, uint16_t& elem_id, int64_t val);

/**
 * \brief Get ID of reverse element
 * \param[in,out] mgr Manager
 * \param[in]     id  ID
 * \return
 */
int
get_biflow_elem_id(fds_iemgr_t* mgr, int64_t id);

/**
 * \brief Get PEN of the scope from the value
 * \param[in,out] mgr       Manager
 * \param[out]    scope_pen Scope PEN
 * \param[in]     val       Value
 * \return True on success, otherwise False
 */
bool
get_pen(fds_iemgr_t *mgr, uint32_t& scope_pen, int64_t val);

/**
 * \brief Save reverse elements from scopes
 * \param[in,out] mgr Manager
 * \return True on success, otherwise False
 * \warning Manager cannot contain reverse scopes
 */
bool
mgr_save_reverse(fds_iemgr_t* mgr);

/**
 * \brief Remove all temporary vectors etc. from a manager
 * \param mgr Manager
 */
void
mgr_remove_temp(fds_iemgr_t* mgr);

/**
 * \brief Sort scopes in a manager
 * \param mgr Manager
 */
void
mgr_sort(fds_iemgr_t* mgr);

/**
 * \brief Create copy of the manager. Don't copy temporary parts of the manager.
 * \param[in] mgr Manager
 * \return Copied manager on success,
 * otherwise nullptr and an error message is set (see fds_iemgr_last_err())
 */
fds_iemgr_t*
mgr_copy(const fds_iemgr_t* mgr);

/**
 * \brief Save modification time of a file to the manager
 * \param[in,out] mgr  Manager
 * \param[in]     path Path and name of the file e.g. 'tmp/user/elements/iana.xml'
 * \return True on success, otherwise False
 */
bool
mtime_save(fds_iemgr_t* mgr, const string& path);