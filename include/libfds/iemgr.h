/**
 * \file   include/libfds/iemgr.h
 * \author Michal Režňák <xrezna04@stud.fit.vutbr.cz>
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \brief  Manager of the informational elements
 * \date   2017-2018
 */

/* Copyright (C) 2017-2018 CESNET, z.s.p.o.
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

#ifndef LIBFDS_IEMGR_H
#define LIBFDS_IEMGR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <libfds/api.h>

/**
 * \defgroup fds_iemgr IPFIX Information Elements (IE) Manager
 * \ingroup publicAPIs
 * \brief Data types and tools for management and lookup of Information Elements used IPFIX Messages
 * \remark Based on RFC 7012. (see https://tools.ietf.org/html/rfc7012)
 *
 * @{
 */

/**
 * \enum fds_iemgr_element_type
 * \brief IPFIX Element data type
 *
 * This enumeration describes the set of valid abstract data types of the IPFIX information model,
 * independent of encoding. Note that further abstract data types may be specified by future
 * updates to this enumeration.
 *
 * \warning
 *   The abstract data type definitions in this section are intended only to define the values
 *   which can be taken by Information Elements of each type. For example, FDS_ET_UNSIGNED_64 does
 *   NOT mean that an element with this type occupies 8 bytes, because it can be stored on
 *   1,2,3,4,5,6,7 or 8 bytes. The encodings of these data types for use with the IPFIX protocol
 *   are defined in RFC 7011, Section 6.1.
 *
 * \remark Based on RFC 7012, Section 3.1 and RFC 6313, Section 11.1
 */
enum fds_iemgr_element_type {
    /** The type represents a finite-length string of octets.                                    */
    FDS_ET_OCTET_ARRAY = 0,
    /** The type represents a non-negative integer value in the range of 0 to 255.               */
    FDS_ET_UNSIGNED_8,
    /** The type represents a non-negative integer value in the range of 0 to 65,535.            */
    FDS_ET_UNSIGNED_16,
    /** The type represents a non-negative integer value in the range of 0 to 4,294,967,295.     */
    FDS_ET_UNSIGNED_32,
    /** The type represents a non-negative integer value in the range of 0
     *  to 18,446,744,073,709,551,615.                                                           */
    FDS_ET_UNSIGNED_64,
    /** The type represents an integer value in the range of -128 to 127.                        */
    FDS_ET_SIGNED_8,
    /** The type represents an integer value in the range of -32,768 to 32,767.                  */
    FDS_ET_SIGNED_16,
    /** The type represents an integer value in the range of -2,147,483,648 to 2,147,483,647.    */
    FDS_ET_SIGNED_32,
    /** The type represents an integer value in the range of -9,223,372,036,854,775,808
     *  to 9,223,372,036,854,775,807.                                                            */
    FDS_ET_SIGNED_64,
    /** The type corresponds to an IEEE single-precision 32-bit floating-point type.             */
    FDS_ET_FLOAT_32,
    /** The type corresponds to an IEEE double-precision 64-bit floating-point type.             */
    FDS_ET_FLOAT_64,
    /** The type "boolean" represents a binary value. The only allowed values are "true"
     *  and "false".                                                                             */
    FDS_ET_BOOLEAN,
    /** The type represents a MAC-48 address as define in IEEE 802.3, 2012                       */
    FDS_ET_MAC_ADDRESS,
    /** The type represents a finite-length string of valid characters from the Unicode coded
     *  character set.                                                                           */
    FDS_ET_STRING,
    /** The type represents a time value expressed with second-level precision.                  */
    FDS_ET_DATE_TIME_SECONDS,
    /** The type represents a time value expressed with millisecond-level precision.             */
    FDS_ET_DATE_TIME_MILLISECONDS,
    /** The type represents a time value expressed with microsecond-level precision.             */
    FDS_ET_DATE_TIME_MICROSECONDS,
    /** The type represents a time value expressed with nanosecond-level precision.              */
    FDS_ET_DATE_TIME_NANOSECONDS,
    /** The type represents an IPv4 address.                                                     */
    FDS_ET_IPV4_ADDRESS,
    /** The type represents an IPv6 address.                                                     */
    FDS_ET_IPV6_ADDRESS,
    /** The type represents a list of any Information Element used for single-valued data types. */
    FDS_ET_BASIC_LIST,
    /** The type represents a list of a structured data type, where the data type of each list
     *  element is the same and corresponds with a single Template Record.                       */
    FDS_ET_SUB_TEMPLATE_LIST,
    /** The type "subTemplateMultiList" represents a list of structured data types, where the
     *  data types of the list elements can be different and correspond with different Template
     *  definitions.                                                                             */
    FDS_ET_SUB_TEMPLATE_MULTILIST,
    /** An unassigned type (invalid value).                                                      */
    FDS_ET_UNASSIGNED = 255
};

/**
 * \enum fds_iemgr_element_semantic
 * \brief IPFIX Element semantic type
 *
 * This enumeration describes the set of valid data type semantics of the IPFIX information model.
 * Further data type semantics may be specified by future updates to this enumeration.
 * \remark Based on RFC 7012, Section 3.2 and RFC 6313, Section 11.2
 */
enum fds_iemgr_element_semantic {
    /** No semantics apply to the field. It cannot be manipulated by a Collecting Process or
     *  File Reader that does not understand it a priori.                                        */
    FDS_ES_DEFAULT = 0,
    /** A numeric (integral or floating point) value representing a measured value pertaining to
     *  the record. This is the default semantic type of all numeric data types.                 */
    FDS_ES_QUANTITY,
    /** An integral value reporting the value of a counter. Counters are unsigned and wrap back
     *  to zero after reaching the limit of the type. A total counter counts independently of the
     *  export of its value.                                                                     */
    FDS_ES_TOTAL_COUNTER,
    /** An integral value reporting the value of a counter. Counters are unsigned and wrap back
     *  to zero after reaching the limit of the type. A delta counter is reset to 0 each time it
     *  is exported and/or expires without export.                                               */
    FDS_ES_DELTA_COUNTER,
    /** An integral value that serves as an identifier. Identifiers MUST be one of the signed or
     *  unsigned data types.                                                                     */
    FDS_ES_IDENTIFIER,
    /** An integral value that represents a set of bit fields. Flags MUST always be of an unsigned
     *  data type.                                                                               */
    FDS_ES_FLAGS,
    /** A list is a structured data type, being composed of a sequence of elements, e.g.,
     *  Information Element, Template Record.                                                    */
    FDS_ES_LIST,
    /** An integral value reporting the value of a counter, identical to the Counter32 and
     *  Counter64 semantics, as determined by the Field Length. This is similar to IPFIX's
     *  totalCounter semantic, except that total counters have an initial value of 0 but SNMP
     *  counters do not.                                                                         */
    FDS_ES_SNMP_COUNTER,
    /** An integral value identical to the Gauge32 semantic and the Gauge64 semantic, as
     *  determined by the Field Length.                                                          */
    FDS_ES_SNMP_GAUGE,
    /** An unassigned sematic type (invalid value).                                              */
    FDS_ES_UNASSIGNED = 255
};

/**
 * \enum fds_iemgr_element_unit
 * \brief IPFIX data unit
 *
 * A description of the units of an IPFIX Information Element. Further data units may be
 * specified by future updates to this enumeration.
 */
enum fds_iemgr_element_unit {
    /** The type represents a unitless field.                                                    */
    FDS_EU_NONE = 0,
    /** THe type represents a number of bits                                                     */
    FDS_EU_BITS,
    /** The type represents a number of octets (bytes)                                           */
    FDS_EU_OCTETS,
    /** The type represents a number of packets                                                  */
    FDS_EU_PACKETS,
    /** The type represents a number of flows                                                    */
    FDS_EU_FLOWS,
    /** The type represents a time value in seconds.                                             */
    FDS_EU_SECONDS,
    /** The type represents a time value in milliseconds.                                        */
    FDS_EU_MILLISECONDS,
    /** The type represents a time value in microseconds.                                        */
    FDS_EU_MICROSECONDS,
    /** The type represents a time value in nanoseconds.                                         */
    FDS_EU_NANOSECONDS,
    /** The type represents a length in units of 4 octets (e.g. IPv4 header).                    */
    FDS_EU_4_OCTET_WORDS,
    /** The type represents a number of IPFIX messages (e.g. for reporing).                      */
    FDS_EU_MESSAGES,
    /** The type represents a TTL (Time to Live) value                                           */
    FDS_EU_HOPS,
    /** The type represents a number of labels in the MPLS stack                                 */
    FDS_EU_ENTRIES,
    /** The type represents a number of L2 frames                                                */
    FDS_EU_FRAMES,
    /** The type represents a number of transport ports                                          */
    FDS_EU_PORTS,
    /** The type represents a units of the inferred Information Elements                         */
    FDS_EU_INFERRED,
    /** An unassigned unit type (invalid value)                                                  */
    FDS_EU_UNASSIGNED = 65535
};

/**
 * \enum fds_iemgr_element_status
 * \brief IPFIX element status
 *
 * A description of statuses of an IPFIX Information Element. Further data units may be
 * specified by future updates to this enumeration.
 */
enum fds_iemgr_element_status {
    /** The type represents current status                                                       */
    FDS_ST_CURRENT,
    /** The type represent deprecated status                                                     */
    FDS_ST_DEPRECATED,
    /** Invalid value                                                                            */
    FDS_ST_INVALID = 65535,
};

/** Modes for biflow configuration                                                         */
enum fds_iemgr_element_biflow {
    FDS_BF_INVALID,    /**< Invalid type (for internal use only)                           */
    FDS_BF_NONE,       /**< No reverse IEs are set                                         */
    FDS_BF_PEN,        /**< Separated PEN for reverse IEs                                  */
    FDS_BF_SPLIT,      /**< IDs 0-16383 for normal director and 16384-32767 for reverse    */
    FDS_BF_INDIVIDUAL, /**< Individually configured for each normal element within the PEN */
};

/** Alias modes                                           */
enum fds_iemgr_alias_mode {
    FDS_ALIAS_ANY_OF,   /**< Any of the elements listed   */
    FDS_ALIAS_FIRST_OF, /**< First of the elements listed */
};

/** Metadata for a scope                                                          */
struct fds_iemgr_scope {
    uint32_t pen;                                 /**< Private Enterprise Number  */
    char *name;                                   /**< Scope name                 */
    enum fds_iemgr_element_biflow biflow_mode;    /**< Mode for reverse IEs       */
    /**
     * If Biflow mode PEN is set, biflow ID define PEN of the reverse scope.
     * If Biflow mode SPLIT is set, biflow ID define base on which bit scope is split
     * Else biflow ID is ignored.
     */
    uint32_t biflow_id;                           /**< Biflow ID                  */
};

/**
 * \brief IPFIX Element definition
 *
 * Description of the IPFIX element from an user defined configuration or
 * directly from a flow exporter. Determine a name of the field, data types,
 * etc.
 */
struct fds_iemgr_elem {
    /** Element ID                                                           */
    uint16_t id;
    /** Name of the element                                                  */
    char *name;
    /** Scope of the element                                                 */
    struct fds_iemgr_scope *scope;
    /**
     * Abstract data type. Intend only to define the values which can be
     * taken by the element.
     * \warning Do NOT represent a size of the record!
     */
    enum fds_iemgr_element_type     data_type;
    /** Data semantic                                                        */
    enum fds_iemgr_element_semantic data_semantic;
    /** Data unit                                                            */
    enum fds_iemgr_element_unit     data_unit;
    /** Element status                                                       */
    enum fds_iemgr_element_status   status;
    /** Reverse element                                                      */
    bool                            is_reverse;
    /** Reverse element, when individual mode is defined in a elements scope */
    struct fds_iemgr_elem *         reverse_elem;

    struct fds_iemgr_alias **       aliases;
    size_t                          aliases_cnt;

    struct fds_iemgr_mapping **     mappings;
    size_t                          mappings_cnt;
};

/** Alias */
struct fds_iemgr_alias {
    char *name;

    enum fds_iemgr_alias_mode mode;
    
    char **aliased_names;
    size_t aliased_names_cnt;

    struct fds_iemgr_elem **sources;
    size_t sources_cnt;
};

/** Mapping key-value pair */
struct fds_iemgr_mapping_item {
    char *key;
    
    union fds_iemgr_mapping_value {
        int64_t i;
    } value;
};

/** Mapping */
struct fds_iemgr_mapping {
    char *name;
    
    bool key_case_sensitive;

    struct fds_iemgr_elem **elems;
    size_t elems_cnt;

    struct fds_iemgr_mapping_item *items;
    size_t items_cnt;
};


/** Element Manager  */
typedef struct fds_iemgr fds_iemgr_t;

/**
 * \brief Check if a type of an Information Element is a signed integer
 * \param[in] type IE type to test
 * \return True or false
 */
static inline bool
fds_iemgr_is_type_signed(enum fds_iemgr_element_type type)
{
    return (type == FDS_ET_SIGNED_64 || type == FDS_ET_SIGNED_32
        ||  type == FDS_ET_SIGNED_16 || type == FDS_ET_SIGNED_8);
}

/**
 * \brief Check if a type of an Information Element is an unsigned integer
 * \param[in] type IE type to test
 * \return True or false
 */
static inline bool
fds_iemgr_is_type_unsigned(enum fds_iemgr_element_type type)
{
    return (type == FDS_ET_UNSIGNED_64 || type == FDS_ET_UNSIGNED_32
        ||  type == FDS_ET_UNSIGNED_16 || type == FDS_ET_UNSIGNED_8);
}

/**
 * \brief Check if a type of an Information Element is a floating-point number
 * \param[in] type IE type to test
 * \return True or false
 */
static inline bool
fds_iemgr_is_type_float(enum fds_iemgr_element_type type)
{
    return (type == FDS_ET_FLOAT_32 || type == FDS_ET_FLOAT_64);
}

/**
 * \brief Check if a type of an Information Element is an IPv4/IPv6 address
 * \param[in] type IE type to test
 * \return True or false
 */
static inline bool
fds_iemgr_is_type_ip(enum fds_iemgr_element_type type)
{
    return (type == FDS_ET_IPV4_ADDRESS || type == FDS_ET_IPV6_ADDRESS);
}

/**
 * \brief Check if a type of an Information Element is a timestamp
 * \param[in] type IE type to test
 * \return True or false
 */
static inline bool
fds_iemgr_is_type_time(enum fds_iemgr_element_type type) {
    return (type == FDS_ET_DATE_TIME_SECONDS || type == FDS_ET_DATE_TIME_MILLISECONDS
        ||  type == FDS_ET_DATE_TIME_MICROSECONDS || type == FDS_ET_DATE_TIME_NANOSECONDS);
}

/**
 * \brief Check if a type of an Information Element is a list
 * \param[in] type IE type to test
 * \return True or false
 */
static inline bool
fds_iemgr_is_type_list(enum fds_iemgr_element_type type) {
    return (type == FDS_ET_BASIC_LIST || type == FDS_ET_SUB_TEMPLATE_LIST
        ||  type == FDS_ET_SUB_TEMPLATE_MULTILIST);
}

/**
 * \brief Get a string representation of an IE type
 * \param[in] type Type
 * \return Statically allocated string or NULL (invalid type)
 */
FDS_API const char *
fds_iemgr_type2str(enum fds_iemgr_element_type type);

/**
 * \brief Get an IE type from a string representation
 * \note The comparison function is case insensitive.
 * \param[in] str String
 * \return Type or #FDS_ET_UNASSIGNED, in case of failure.
 */
FDS_API enum fds_iemgr_element_type
fds_iemgr_str2type(const char *str);

/**
 * \brief Get a string representation of an IE semantic
 * \param[in] sem Semantic
 * \return Statically allocated string or NULL (invalid type)
 */
FDS_API const char *
fds_iemgr_semantic2str(enum fds_iemgr_element_semantic sem);

/**
 * \brief Get an IE semantic from a string representation
 * \note The comparison function is case insensitive.
 * \param[in] str String
 * \return Type or #FDS_ES_UNASSIGNED, in case of failure.
 */
FDS_API enum fds_iemgr_element_semantic
fds_iemgr_str2semantic(const char *str);

/**
 * \brief Get a string representation of an IE unit
 * \param[in] unit Unit
 * \return Statically allocated string or NULL (invalid type)
 */
FDS_API const char *
fds_iemgr_unit2str(enum fds_iemgr_element_unit unit);

/**
 * \brief Get an IE semantic from a string representation
 * \note The comparison function is case insensitive.
 * \param[in] str String
 * \return Type or #FDS_EU_UNASSIGNED, in case of failure.
 */
FDS_API enum fds_iemgr_element_unit
fds_iemgr_str2unit(const char *str);

/**
 * \brief Create a manager
 * \return Manager on success, otherwise NULL;
 */
FDS_API fds_iemgr_t *
fds_iemgr_create();

/**
 * \brief Create and copy a manager
 * \param[in] mgr Manager what will be copied
 * \return Copied manager on success. Otherwise NULL (i.e. memory allocation error)
 */
FDS_API fds_iemgr_t *
fds_iemgr_copy(const fds_iemgr_t *mgr);

/**
 * Remove all elements from the manager
 * \param[in,out] mgr Manager
 */
FDS_API void
fds_iemgr_clear(fds_iemgr_t *mgr);

/**
 * \brief Destroy the manager
 * \param[in] mgr Manager
 */
FDS_API void
fds_iemgr_destroy(fds_iemgr_t *mgr);

/**
 * \brief Compare last modified time of parsed files
 * \param[in,out] mgr Manager
 * \return FDS_OK if nothing changed,
 *         FDS_DIFF_MTIME if at least one file changed timestamp
 *         and FDS_ERR_FORMAT when error occurred (an error message is set - see fds_iemgr_last_err())
 */
FDS_API int
fds_iemgr_compare_timestamps(fds_iemgr_t *mgr);

/**
 * \brief Load an XML files from dir and save elements to the manager
 * \param[in,out] mgr  Manager
 * \param[in]     path Path to the directories with saved XML files
 * \return FDS_OK on success, otherwise FDS_ERR_NOMEM or FDS_ERR_FORMAT and an error message is set
 *   (see fds_iemgr_last_err())
 *
 * \note Load only files in 'system/elements' and 'user/elements'
 * \note Scope and biflow definitions are ignored if new elements are added to the existing scope.
 * \note Element that overwrite another element should define only the ID of the Element and
 *   information (i.e. parameters) that should be rewritten.
 * \note All files which starts with '.' in the name are ignored (hidden files)
 * \warning Each element can be rewritten only once
 * \warning All previously parsed elements will be removed
 */
FDS_API int
fds_iemgr_read_dir(fds_iemgr_t *mgr, const char *path);

/**
 * \brief Load an XML file and save elements to the manager
 * \param[in,out] mgr       Manager with elements
 * \param[in]     file_path Path with XML file
 * \param[in]     overwrite On collision overwrite elements previously defined
 * \return FDS_OK on success, otherwise FDS_ERR_NOMEM or FDS_ERR_FORMAT and an error message is set
 *   (see fds_iemgr_last_err())
 *
 * \note If \p overwrite is false, then elements can be only added, they cannot be rewritten
 * \note Element that overwrite another element should define only the ID of the Element and
 *   information (i.e. parameters) that should be rewritten.
 * \note Biflow and name of a scope which overwrites another scope are ignored
 */
FDS_API int
fds_iemgr_read_file(fds_iemgr_t *mgr, const char *file_path, bool overwrite);

/**
 * TODO
 */
FDS_API int
fds_iemgr_read_aliases(fds_iemgr_t *mgr, const char *dir);

/**
 * \brief Find an element with a given ID in the manager
 * \param[in] mgr Manager
 * \param[in] pen Private Enterprise Number
 * \param[in] id  ID of element
 * \return Element if exists, otherwise NULL.
 */
FDS_API const struct fds_iemgr_elem *
fds_iemgr_elem_find_id(const fds_iemgr_t *mgr, uint32_t pen, uint16_t id);

/**
 * \brief Find an element with a given name in the manager
 *
 * If the element is not part of the IANA scope, the scope must be explicitly defined. Otherwise
 * only the IANA scope is searched for. The format of the searched element can be
 * "element_name" (IANA only) or "scope_name:element_name" (semicolon separated names).
 *
 * \warning The name of the element and the name of a scope cannot contain only numbers or ':'
 * \param[in] mgr  Manager
 * \param[in] name Prefix and Name of element separated by ':', e.g. 'scope_name:element_name'
 * \return Element if exists, otherwise NULL.

 */
FDS_API const struct fds_iemgr_elem *
fds_iemgr_elem_find_name(const fds_iemgr_t *mgr, const char *name);

/**
 * \brief Add an element to the manager
 * \param[in,out] mgr        Manager
 * \param[in]     elem       Element
 * \param[in]     pen        Private enterprise number
 * \param[in]     overwrite  Overwrite previously defined element
 * \return FDS_OK on success, otherwise FDS_ERR_NOMEM or FDS_ERR_FORMAT and an error message is set
 *   (see fds_iemgr_last_err())
 *
 * \note Element's \p scope is ignored.
 * \note Element's \p reverse element is ignored.
 */
FDS_API int
fds_iemgr_elem_add(fds_iemgr_t *mgr, const struct fds_iemgr_elem *elem, uint32_t pen,
    bool overwrite);

/**
 * \brief Add a reverse element to the manager
 * \param[in,out] mgr       Manager
 * \param[in]     pen       Private enterprise number of the scope
 * \param[in]     id        ID of the forward element
 * \param[in]     new_id    ID of a new reverse element
 * \param[in]     overwrite Overwrite previously defined reverse element
 * \return FDS_OK on success, otherwise FDS_ERR_NOMEM, FDS_ERR_FORMAT or FDS_ERR_NOTFOUND
 * and an error message is set (see fds_iemgr_last_err())
 */
FDS_API int
fds_iemgr_elem_add_reverse(fds_iemgr_t *mgr, uint32_t pen, uint16_t id, uint16_t new_id,
    bool overwrite);

/**
 * \brief Remove an element from the manager
 * \param[in,out] mgr  Manager
 * \param[in]     pen  Private Enterprise Number
 * \param[in]     id   ID of an element
 * \return FDS_OK on success, otherwise FDS_ERR_NOTFOUND or FDS_ERR_NOMEM and an error message is
 *   set (see fds_iemgr_last_err())
 */
FDS_API int
fds_iemgr_elem_remove(fds_iemgr_t *mgr, uint32_t pen, uint16_t id);

/**
 * \brief Find a scope in the manager by a PEN
 * \param[in,out] mgr Manager
 * \param[in]     pen Private Enterprise Number
 * \return Scope if exists, otherwise NULL.
 */
FDS_API const struct fds_iemgr_scope *
fds_iemgr_scope_find_pen(const fds_iemgr_t *mgr, uint32_t pen);

/**
 * \brief Find a scope in the manager by a name
 * \param[in,out] mgr    Manager
 * \param[in]     name   Scope name
 * \return Scope if exists, otherwise NULL.
 */
FDS_API const struct fds_iemgr_scope *
fds_iemgr_scope_find_name(const fds_iemgr_t *mgr, const char *name);

/**
 * \brief Get the last error message
 * \param[in] mgr Manager
 */
FDS_API const char *
fds_iemgr_last_err(const fds_iemgr_t *mgr);

/**
 * \brief Find an alias by name
 * \param[in] mgr  Manager
 * \param[in] name The aliased name
 * \return Pointer to the alias
 */
FDS_API int
fds_iemgr_read_aliases(fds_iemgr_t *mgr, const char *dir);

/**
 * \brief Find an alias by name
 * \param[in] mgr  Manager
 * \param[in] name The aliased name
 * \return Pointer to the alias
 */
FDS_API const struct fds_iemgr_alias *
fds_iemgr_alias_find(const fds_iemgr_t *mgr, const char *aliased_name);

FDS_API const struct fds_iemgr_mapping_item *
fds_iemgr_mapping_find(const fds_iemgr_t *mgr, const char *name, const char *key);

FDS_API int
fds_iemgr_read_mappings(fds_iemgr_t *mgr, const char *dir);


#ifdef __cplusplus
}
#endif

#endif /* LIBFDS_IEMGR_H */

/**
 * @}
 */
