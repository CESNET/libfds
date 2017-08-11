/**
 * \author Michal Režňák
 * \date   11.8.17
 */
#pragma once

#include <iostream>
#include <vector>
#include <cstring>
#include <limits>
#include <algorithm>
#include <set>
#include <bits/unique_ptr.h>
#include <sys/stat.h>
#include <libfds/iemgr.h>
#include <libfds/xml_parser.h>

using std::vector;
using std::pair;
using std::string;
using std::set;
using std::sort;
using std::move;
using std::to_string;

#define uint32_limit (std::numeric_limits<uint32_t>::max()     )
#define uint15_limit (std::numeric_limits<uint16_t>::max() >> 1)
#define reverse_name ("@reverse")

/** Saved elements with same PEN and same PREFIX                                     */
struct fds_iemgr_scope_inter {
    struct fds_iemgr_scope                      head;   /**< Scope head              */
    vector <pair <uint16_t, fds_iemgr_elem *> > ids;    /**< Elements sorted by ID   */
    vector <pair <string,   fds_iemgr_elem *> > names;  /**< Elements sorted by NAME */
    bool                                        is_reverse; /**< True if scope is reverse */
};

/** Saved elements and sorted pointers by different values (PEN, PREFIX, etc.)               */
struct fds_iemgr {
    string                         err_msg;                     /**< Error message           */
    vector<pair<char*, timespec> > mtime;                 /**< Modification time of files */

    vector<pair<uint32_t, fds_iemgr_scope_inter *> > pens;     /**< Elements sorted by PEN     */
    vector<pair<string, fds_iemgr_scope_inter *> > prefixes;/**< Elements sorted by PREFIX  */

    set<uint16_t>              parsed_ids;       /**< IDs of parsed elements from one file **/
    bool                       can_overwrite_elem;             /**< Can overwrite elements */
    pair<bool, set<uint32_t> > overwrite_scope;                /**< PENs of overwritten scopes */
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
};

/** This declarations must be here because of the unique pointers */ // TODO
void scope_remove(fds_iemgr_scope_inter* scope);
void element_remove(fds_iemgr_elem* elem);

using unique_elem   = std::unique_ptr<fds_iemgr_elem,        decltype(&::element_remove)>;
using unique_scope  = std::unique_ptr<fds_iemgr_scope_inter, decltype(&::scope_remove)>;
using unique_dir    = std::unique_ptr<DIR,                   decltype(&::closedir)>;
using unique_file   = std::unique_ptr<FILE,                  decltype(&::fclose)>;
using unique_parser = std::unique_ptr<fds_xml_t,             decltype(&::fds_xml_destroy)>;
using unique_mgr    = std::unique_ptr<fds_iemgr_t,           decltype(&::fds_iemgr_destroy)>;

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
    bool operator() (const V value, const pair<V, P>& pair) const {
        return value < pair.first;
    }

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
    // todo change P::value type to not define twice
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
parsed_id_save(fds_iemgr_t* mgr, const fds_iemgr_scope_inter* scope, const uint16_t id);

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
 * \brief Get units of an element
 * \param unit Element units in a string
 * \return In what units are data
 */
fds_iemgr_element_unit
get_data_unit(const char *unit);

/**
 * \brief Get semantics of an element
 * \param[in] semantic Semantic of an element in string
 * \return In what semantic is the element
 */
fds_iemgr_element_semantic
get_data_semantic(const char *semantic);

/**
 * \brief Get data type of an element
 * \param[in] type Data type in string
 * \return Data type of the element
 */
fds_iemgr_element_type
get_data_type(const char *type);

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
get_biflow_elem_id(fds_iemgr_t* mgr, const int64_t id);

/**
 * \brief Get PEN of the scope from the value
 * \param[in,out] mgr       Manager
 * \param[out]    scope_pen Scope PEN
 * \param[in]     val       Value
 * \return True on success, otherwise False
 */
bool
get_pen(fds_iemgr_t *mgr, uint32_t& scope_pen, const int64_t val);

/**
 * \brief Save reverse elements from scopes
 * \param[in,out] mgr Manager
 * \return True on success, otherwise False
 * \warning Manager cannot contain reverse scopes
 */
bool
mgr_save_reverse(fds_iemgr_t* mgr);

/**
 * \brief Remve all temporary vectors etc. from a manager
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

// TODO
fds_iemgr_t*
mgr_copy(fds_iemgr_t* mgr);
