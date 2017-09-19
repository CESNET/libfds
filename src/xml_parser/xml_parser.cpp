/**
 * \file   src/xml_parser/xml_parser.cpp
 * \author Michal Režňák <xrezna04@stud.fit.vutbr.cz>
 * \brief  XML parser implementation
 * \date   10. July 2017
 */

/* Copyright (C) 2016 CESNET, z.s.p.o.
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
#include <libxml2/libxml/tree.h>
#include <cstring>
#include <limits>
#include <set>
#include <libfds/xml_parser.h>
#include <libfds/common.h>
#include <bits/unique_ptr.h>

/** \cond DOXYGEN_SKIP_THIS */
using unique_ctx = std::unique_ptr<xmlParserCtxt, decltype(&::xmlFreeParserCtxt)>;
using unique_doc = std::unique_ptr<xmlDoc,        decltype(&::xmlFreeDoc)>;
/** \endcond */

/** Parser structure.                                              */
struct fds_xml {
    const fds_xml_args *opts{}; /**< saved user defined conditions */
    fds_xml_ctx *ctx{};         /**< parsed context                */
    std::string error_msg;      /**< error message                 */
};

/** Context of one level of xml.                                      */
struct fds_xml_ctx {
    unsigned index = 0;             /**< index of last parsed content */
    std::vector<fds_xml_cont> cont; /**< content                      */
};

/** saved names of elements and attributes                  */
struct names {
    std::vector<std::string> elem; /**< names of elements   */
    std::vector<std::string> attr; /**< names of attributes */
};

/** saved IDs of elements and pointers to nested structures                           */
struct attributes {
    std::set<int> ids;                       /**< saved IDs                           */
    std::set<const fds_xml_args *> pointers; /**< saved pointers to nested structures */
    bool text = false;                       /**< only one definition of element text */
};

/** Return value when some elements are cyclically nested */
#define FDS_XML_NESTED (-4)

/**
 * Create parser
 *
 * \param[out] parser Created parser
 * \return Ok on success, otherwise Err
 */
int
fds_xml_create(fds_xml_t **parser)
{
    if (parser == nullptr) {
        return FDS_ERR_FMT;
    }

    *parser = new (std::nothrow) fds_xml;
    if (*parser == nullptr) {
        return FDS_ERR_NOMEM;
    }

    (*parser)->ctx = nullptr;
    (*parser)->opts = nullptr;

    return FDS_OK;
}

/**
 * Destroy all contexts recursively
 *
 * \param[in] ctx Destroyed context
 */
void
destroy_context(fds_xml_ctx *ctx)
{
    if (ctx == nullptr) {
        return;
    }

    for (auto const &cont : ctx->cont) {
        if (cont.type == OPTS_T_CONTEXT) {
            destroy_context(cont.ptr_ctx);
        } else if (cont.type == OPTS_T_STRING) {
            delete[] cont.ptr_string;
        }
    }

    delete ctx;
}

/**
 * Destroy XML parser.
 *
 * \param[in] parser Parser to destroy.
 */
void
fds_xml_destroy(fds_xml_t *parser)
{
    if (parser == nullptr) {
        return;
    }

    destroy_context(parser->ctx);
    delete parser;
}

/**
 * Get attribute name and arg name from args
 *
 * \param[in] opt Args
 * \return Attribute name of arg
 */
const std::string
get_type(const fds_xml_args *opt)
{
    if (opt == nullptr) {
        return "";
    }

    std::string res; // result

    switch (opt->comp) {
    case OPTS_C_ROOT:
        res = "OPTS_ROOT";
        break;
    case OPTS_C_ELEMENT:
        res = "OPTS_ELEM";
        break;
    case OPTS_C_ATTR:
        res = "OPTS_ATTR";
        break;
    case OPTS_C_TEXT:
        res = "OPTS_TEXT";
        break;
    case OPTS_C_NESTED:
        res = "OPTS_NESTED";
        break;
    case OPTS_C_TERMINATOR:
        res = "OPTS_END";
        break;
    default:
        break;
    }

    if (opt->name != nullptr)
        res += " '" + std::string(opt->name) + "'";

    return res;
}

/**
 * Find name in vector
 *
 * \param[in,out]  names Vector with saved names
 * \param[in]      name  Searched name
 * \return True on success, otherwise False
 */
bool
check_common_find_name(std::vector<std::string> &names, const std::string &name)
{
    // only one occurrence of element name
    for (const auto &cur_name : names) {
        if (cur_name == name) {
            return true;
        }
    }
    // save name
    names.emplace_back(name);

    return false;
}

/**
 * Check all conditions that are common for all elements.
 *
 * \param[in]     opt    One element.
 * \param[out]    parser Parsed user defined conditions
 * \param[in,out] attr   Saved names of elements, attributes and their IDs
 * \return OK on success, or Err.
 */
int
check_common(const fds_xml_args opt, fds_xml_t *parser, struct attributes &attr)
{
    // TYPE

    // ID
    if (opt.id < 0) {
        parser->error_msg = "Wrong ID of element " + get_type(&opt);
        return FDS_ERR_FMT;
    }

    // each element must have unique ID
    if (opt.id != 0 && attr.ids.find(opt.id) != attr.ids.end()) { // founded
        parser->error_msg = "ID of element " + get_type(&opt) + " is previously used";
        return FDS_ERR_FMT;
    }
    if (opt.id != 0) {
        attr.ids.insert(opt.id);
    }

    // FLAGS
    if (opt.flags < 0) {
        parser->error_msg = "Wrong flags of element " + get_type(&opt);
        return FDS_ERR_FMT;
    }

    // don't check name in element text and root
    if (opt.comp == OPTS_C_TEXT || opt.comp == OPTS_C_ROOT) {
        return FDS_OK;
    }

    // NAME
    if (opt.name == nullptr) {
        parser->error_msg = "Name of the " + get_type(&opt) + " is missing";
        return FDS_ERR_FMT;
    }

    return FDS_OK;
}

/**
 * Check all conditions that must be true for root
 *
 * \param[in]     opt    One element.
 * \param[out]    parser Parsed user defined conditions
 * \param[in,out] attr   Saved names of elements, attributes and their IDs
 * \return OK on success, or Err.
 */
int
check_root(const struct fds_xml_args opt, fds_xml_t *parser, struct attributes &attr)
{
    // check if it's really root
    if (opt.comp != OPTS_C_ROOT) {
        parser->error_msg = "First element must be root, not " + get_type(&opt);
        return FDS_ERR_FMT;
    }

    if (opt.type != OPTS_T_NONE) {
        parser->error_msg = "Root element " + get_type(&opt) + " must have type OPTS_T_NONE";
        return FDS_ERR_FMT;
    }

    if (opt.next != nullptr) {
        parser->error_msg = "Root element cannot be nested (have pointer to another struct)";
        return FDS_ERR_FMT;
    }

    if (opt.flags != 0) {
        parser->error_msg = "Root element cannot have flags";
        return FDS_ERR_FMT;
    }

    if (opt.name == nullptr) {
        parser->error_msg = "Root element must have name";
        return FDS_ERR_FMT;
    }

    if (check_common(opt, parser, attr) != FDS_OK) {
        return FDS_ERR_FMT;
    }

    return FDS_OK;
}

/**
 * Check all conditions that must be true for element
 *
 * \param[in]     opt    One element.
 * \param[out]    parser Parsed user defined conditions
 * \param[in,out] names  Saved names of elements, attributes and their IDs
 * \param[in,out] attr   Saved names of elements, attributes and their IDs
 * \return OK on success, or Err.
 */
int
check_element(
    const struct fds_xml_args opt, fds_xml_t *parser, struct names &names, struct attributes &attr)
{
    if (check_common(opt, parser, attr) != FDS_OK) {
        return FDS_ERR_FMT;
    }

    if (opt.next != nullptr) {
        parser->error_msg = "Element cannot be nested (opinter to next must be nullptr)";
        return FDS_ERR_FMT;
    }

    // find element with same name
    if (check_common_find_name(names.elem, opt.name)) {
        parser->error_msg = "More than one occurrence of element " + get_type(&opt);
        return FDS_ERR_FMT;
    }

    if (opt.type != OPTS_T_UINT && opt.type != OPTS_T_STRING && opt.type != OPTS_T_DOUBLE
        && opt.type != OPTS_T_BOOL && opt.type != OPTS_T_INT) {
        parser->error_msg = "Element " + get_type(&opt)
            + " must have one of these following types: \n"
              "OPTS_T_UINT\n"
              "OPTS_T_STRING\n"
              "OPTS_T_DOUBLE\n"
              "OPTS_T_BOOL\n"
              "OPTS_T_INT";
        return FDS_ERR_FMT;
    }

    return FDS_OK;
}

/**
 * Check all conditions that must be true for attribute
 *
 * \param[in]     opt    One element.
 * \param[out]    parser Parsed user defined conditions
 * \param[in,out] names  Saved names of elements, attributes and their IDs
 * \param[in,out] attr   Saved names of elements, attributes and their IDs
 * \return OK on success, or Err.
 */
int
check_attr(
    const struct fds_xml_args opt, fds_xml_t *parser, struct names &names, struct attributes &attr)
{
    if (check_common(opt, parser, attr) != FDS_OK) {
        return FDS_ERR_FMT;
    }
    // find attribute with same name
    if (check_common_find_name(names.attr, opt.name)) {
        parser->error_msg = "More than one occurrence of attribute " + get_type(&opt);
        return FDS_ERR_FMT;
    }

    if (opt.type != OPTS_T_UINT && opt.type != OPTS_T_STRING && opt.type != OPTS_T_DOUBLE
        && opt.type != OPTS_T_BOOL && opt.type != OPTS_T_INT) {
        parser->error_msg = "Element " + get_type(&opt)
            + " must have one of these following types: \n"
              "OPTS_T_UINT\n"
              "OPTS_T_STRING\n"
              "OPTS_T_DOUBLE\n"
              "OPTS_T_BOOL\n"
              "OPTS_T_INT";
        return FDS_ERR_FMT;
    }

    if (opt.next != nullptr) {
        parser->error_msg = "Attribute " + get_type(&opt) + " cannot be nested";
        return FDS_ERR_FMT;
    }

    if ((opt.flags & OPTS_P_MULTI) != 0) {
        parser->error_msg = "Attribute '" + get_type(&opt) + "' cannot have MULTI flag";
        return FDS_ERR_FMT;
    }

    return FDS_OK;
}

/**
 * Check all conditions that must be true for text
 *
 * \param[in]     opt    One element.
 * \param[out]    parser Parsed user defined conditions
 * \param[in,out] attr   Saved names of elements, attributes and their IDs
 * \return OK on success, or Err.
 */
int
check_text(const struct fds_xml_args opt, fds_xml_t *parser, struct attributes &attr)
{
    if (check_common(opt, parser, attr) != FDS_OK) {
        return FDS_ERR_FMT;
    }

    if (opt.name != nullptr) {
        parser->error_msg = "Element " + get_type(&opt) + " cannot have name";
        return FDS_ERR_FMT;
    }

    if (opt.next != nullptr) {
        parser->error_msg = "Element " + get_type(&opt) + " cannot be nested";
        return FDS_ERR_FMT;
    }

    if (attr.text) {
        parser->error_msg = "Element text can be defined only once, second definition is " 
        + get_type(&opt);
        return FDS_ERR_FMT;
    }
    attr.text = true;

    if (opt.type != OPTS_T_UINT && opt.type != OPTS_T_STRING && opt.type != OPTS_T_DOUBLE
        && opt.type != OPTS_T_BOOL && opt.type != OPTS_T_INT) {
        parser->error_msg = "Element " + get_type(&opt)
            + " must have one of these following types: \n"
              "OPTS_T_UINT\n"
              "OPTS_T_STRING\n"
              "OPTS_T_DOUBLE\n"
              "OPTS_T_BOOL\n"
              "OPTS_T_INT";
        return FDS_ERR_FMT;
    }

    return FDS_OK;
}

/**
 * Check all conditions that must be true for nested
 *
 * \param[in]     opt    One element.
 * \param[out]    parser Parsed user defined conditions
 * \param[in,out] names  Saved names of elements, attributes and their IDs
 * \param[in,out] attr   Saved names of elements, attributes and their IDs
 * \return OK on success, or Err.
 */
int
check_nested(
    const struct fds_xml_args opt, fds_xml_t *parser, struct names &names, struct attributes &attr)
{
    // same tests for all components types
    int ret = check_common(opt, parser, attr);
    if (ret != FDS_OK) {
        return ret;
    }

    if (check_common_find_name(names.elem, opt.name)) {
        parser->error_msg = "More than one occurrence of element " + get_type(&opt);
        return FDS_ERR_FMT;
    }

    if (opt.type != OPTS_T_CONTEXT) {
        parser->error_msg = "Element " + get_type(&opt) + " must have type OPTS_T_CONTEXT";
        return FDS_ERR_FMT;
    }

    if (opt.next == nullptr) {
        parser->error_msg =
            "Next element of nested element '" + std::string(opt.name) + "' is missing";
        return FDS_ERR_FMT;
    }

    // called nested structure that was previously parsed (cyclic structures)
    if (attr.pointers.find(opt.next) != attr.pointers.end()) // founded
        return FDS_XML_NESTED;

    attr.pointers.insert(opt.next);
    return FDS_OK;
}

/**
 * Check all conditions that must be true for end
 *
 * \param[in]  opt    One element.
 * \param[out] parser Parsed user defined conditions
 * \return OK on success, or Err.
 */
int
check_end(const struct fds_xml_args opt, fds_xml_t *parser)
{
    if (opt.next != nullptr) {
        parser->error_msg = get_type(&opt) + " cannot have nested element";
        return FDS_ERR_FMT;
    }

    if (opt.id < 0) {
        parser->error_msg = get_type(&opt) + " cant' have negative id";
        return FDS_ERR_FMT;
    }

    if (opt.type != OPTS_T_NONE) {
        parser->error_msg = "Element " + get_type(&opt) + " must have type OPTS_T_NONE";
        return FDS_ERR_FMT;
    }

    return FDS_OK;
}

/**
 * \brief Check all conditions that must be true for OPTS_C_RAW
 * \param[in]     opt    One element
 * \param[in,out] parser Parser
 * \param[in,out] names  Saved names of elements, attributes and their IDs
 * \param[in]     attr   Saved names of elements, attributes and their IDs
 * \return OK on success, or Err.
 */
int
check_raw(const struct fds_xml_args opt, fds_xml_t *parser, struct names &names, 
        struct attributes &attr)
{
    // same tests for all components types
    int ret = check_common(opt, parser, attr);
    if (ret != FDS_OK) {
        return ret;
    }

    // find element with same name
    if (check_common_find_name(names.elem, opt.name)) {
        parser->error_msg = "More than one occurrence of element " + get_type(&opt);
        return FDS_ERR_FMT;
    }

    if (opt.next != nullptr) {
        parser->error_msg = get_type(&opt) + " cannot have nested element";
        return FDS_ERR_FMT;
    }

    if (opt.type != OPTS_T_STRING) {
        parser->error_msg = "Element " + get_type(&opt) + " must have type OPTS_T_STRING";
        return FDS_ERR_FMT;
    }

    return FDS_OK;
}

/**
 * Check all conditions for all elements
 *
 * \param[in]  opts   All elements. Must exist all the time. (when parsing, etc.)
 * \param[out] parser Parsed user defined conditions
 * \param[in]  attr   Saved names of elements, attributes and their IDs.
 * \return OK on success, or Err.
 */
int
check_all(const struct fds_xml_args *opts, fds_xml_t *parser, struct attributes &attr)
{
    struct names names; // saved attributes for one struct
    int i = 0;          // number of opts
    int rec;            // return value for recursion
    int ret;            // return values of case

    // go through all elements and check them
    for (i = 0; opts[i].comp != OPTS_C_TERMINATOR; ++i) {
        switch (opts[i].comp) {
        case OPTS_C_ELEMENT:
            ret = check_element(opts[i], parser, names, attr);
            break;
        case OPTS_C_ATTR:
            ret = check_attr(opts[i], parser, names, attr);
            break;
        case OPTS_C_TEXT:
            ret = check_text(opts[i], parser, attr);
            break;
        case OPTS_C_NESTED:
            ret = check_nested(opts[i], parser, names, attr);
            if (ret == FDS_XML_NESTED)
                return FDS_OK;
            if (ret != FDS_OK)
                return ret;
            // check nested recursively
            rec = check_all(opts[i].next, parser, attr);
            if (rec != FDS_OK)
                return rec;
            break;
        case OPTS_C_RAW:
            ret = check_raw(opts[i], parser, names, attr);
            break;
        case OPTS_C_ROOT:
            parser->error_msg = get_type(&opts[i]) + " should be only on beginning of args";
            return FDS_ERR_FMT;
        default:
            parser->error_msg = "Wrong definition of arg component in args with root '"
                + std::string(opts->name) + "'\n"
                + "It's possible that OPTS_END is missing after element '"
                + std::string(opts[i - 1].name) + "'";
            return FDS_ERR_FMT;
        }

        // check return value from switch case
        if (ret != FDS_OK)
            return ret;
    }

    // check last end
    ret = check_end(opts[i], parser);
    if (ret != FDS_OK) {
        return ret;
    }

    return FDS_OK;
}

/**
 * Take all conditions defined by user and check if are true
 *
 * \param[in, out]  opts   All elements. Must exist all the time. (when parsing, etc.)
 * \param[out] parser Parsed elements for another usage.
 * \return OK if conditions are true, else format error, or memory error.
 */
int
fds_xml_set_args(const fds_xml_args *opts, fds_xml_t *parser)
{
    if (opts == nullptr || parser == nullptr) {
        return FDS_ERR_FMT;
    }

    struct attributes attr = {};
    attr.pointers.insert(*&opts);

    // check first if it's root
    int retr = check_root(*opts, parser, attr);
    if (retr != FDS_OK) {
        return retr;
    }

    // check user defined conditions
    int ret;
    try {
        ret = check_all(opts+1, parser, attr);
    } catch (...) {
        parser->error_msg = "Memory allocation problem";
        return FDS_ERR_NOMEM;
    }
    if (ret != FDS_OK) {
        parser->opts = nullptr;
        return ret;
    }

    parser->opts = opts;

    return FDS_OK;
}

/**
 * Go through args in opts and try to find arg with same name as node name.
 *
 * \param[in] opts Args with user defined conditions
 * \param[in] name Searched name
 * \return Founded arg with same name
 */
const fds_xml_args *
find_arg(const fds_xml_args *opts, const xmlChar *name)
{
    if (name == nullptr || opts == nullptr) {
        return nullptr;
    }

    for (int i = 0; opts[i].comp != OPTS_C_TERMINATOR; ++i) {
        // have same name
        if (xmlStrcmp(name, (const xmlChar *) opts[i].name) == 0) {
            return &opts[i];
        }
    }

    return nullptr;
}

/**
 * Try to find in args argument with text component.
 *
 * \param[in] opt Args with user defined conditions
 * \return Found text component on success, otherwise nullptr
 */
const fds_xml_args *
find_text(const fds_xml_args *opt)
{
    if (opt == nullptr) {
        return nullptr;
    }

    for (int i = 0; opt[i].comp != OPTS_C_TERMINATOR; ++i) {
        if (opt[i].comp == OPTS_C_TEXT) {
            return &opt[i];
        }
    }

    return nullptr;
}

/**
 * Parse string from content to context
 *
 * \param[in]     content   Content
 * \param[in,out] ctx       Context
 * \param[in]     opt       Contents ID.
 * \return Ok on success, otherwise Err
 */
int
parse_string(
    const std::string &content, fds_xml_ctx *ctx, const fds_xml_args *opt)
{
    fds_xml_cont cont{};
    char *res = new char[content.length() + 1];
    std::size_t len = content.copy(res, content.length(), 0);
    res[len] = '\0';

    // save node to content
    cont.id = opt->id;
    cont.type = OPTS_T_STRING;
    cont.ptr_string = res;

    // save context
    ctx->cont.push_back(cont);
    return FDS_OK;
}

/**
 * Convert int from content and save to context
 *
 * \param[in]     content   Content
 * \param[in,out] ctx       Context
 * \param[in]     opt       Contents ID.
 * \param[out]    error_msg Error message
 * \return Ok on success, otherwise Err
 */
int
parse_int(
    const std::string &content, fds_xml_ctx *ctx, const fds_xml_args *opt, std::string &error_msg)
{
    long long res;                                     // string to number
    char *err;                                         // if char contains some string
    fds_xml_cont cont{};                               // saved content to vector of fds_xml_cont
    int64_t max = std::numeric_limits<int64_t>::max(); // max value of long

    res = std::strtoull(content.c_str(), &err, 10);
    if (*err != '\0') {
        error_msg = "In element '" + std::string(opt->name) + "' should be only number (int), not "
            + content;
        return FDS_ERR_FMT;
    }
    if (res > max) {
        error_msg =
            "Number in element '" + std::string(opt->name) + "' is bigger than limit of int";
        return FDS_ERR_FMT;
    }

    // save node to content
    cont.id = opt->id;
    cont.type = OPTS_T_INT;
    cont.val_int = (int64_t) res;

    // save context
    ctx->cont.push_back(cont);
    return FDS_OK;
}

/**
 * Convert double from content and save to context
 *
 * \param[in]     content   Content
 * \param[in,out] ctx       Context
 * \param[in]     opt       Contents ID.
 * \param[out]    error_msg Error message
 * \return Ok on success, otherwise Err
 */
int
parse_double(
    const std::string &content, fds_xml_ctx *ctx, const fds_xml_args *opt, std::string &error_msg)
{
    double res;
    char *err;
    fds_xml_cont cont{};

    res = std::strtod(content.c_str(), &err);
    if (*err != '\0') {
        error_msg = "In element '" + std::string(opt->name)
            + "' should be only number (double), not " + content;
        return FDS_ERR_FMT;
    }

    // save node to content
    cont.id = opt->id;
    cont.type = OPTS_T_DOUBLE;
    cont.val_double = res;

    // save context
    ctx->cont.push_back(cont);
    return FDS_OK;
}

/**
 * Convert bool from content and save to context
 *
 * \param[in]     text      Bool value in a string
 * \param[in,out] ctx       Context
 * \param[in]     opt       Contents ID.
 * \param[out]    error_msg Error message
 * \return Ok on success, otherwise Err
 */
int
parse_bool(
    const std::string &text, fds_xml_ctx *ctx, const fds_xml_args *opt, std::string &error_msg)
{
    bool res;
    fds_xml_cont cont{};

    // find if boll value is true or false
    if ((strcasecmp(text.c_str(),    "true") == 0)
        || (strcasecmp(text.c_str(), "1")    == 0)
        || (strcasecmp(text.c_str(), "yes")  == 0)) {
        res = true;
    } else if ((strcasecmp(text.c_str(), "false") == 0)
           || (strcasecmp(text.c_str(),  "0")     == 0)
           || (strcasecmp(text.c_str(),  "no")    == 0)) {
        res = false;
    } else {
        error_msg = "Incorrect bool value in element '" + std::string(opt->name)
            + "', valid values are: 'true', '1', 'yes', or negatives";
        return FDS_ERR_FMT;
    }

    // save node to content
    cont.id = opt->id;
    cont.type = OPTS_T_BOOL;
    cont.val_bool = res;

    // save context
    ctx->cont.push_back(cont);
    return FDS_OK;
}

/**
 * Convert unsigned int from content and save to context
 *
 * \param[in]     content   String with unsigned int
 * \param[in,out] ctx       Context
 * \param[in]     opt       Opt ID
 * \param[out]    error_msg Error message
 * \return Ok on success, otherwise Err
 */
int
parse_uint(
    const std::string &content, fds_xml_ctx *ctx, const fds_xml_args *opt, std::string &error_msg)
{
    unsigned long long res;
    char *err;
    fds_xml_cont cont{};

    res = std::strtoull(content.c_str(), &err, 10);
    if (*err != '\0') {
        error_msg = "In element '" + std::string(opt->name)
            + "' should be only number (unsigned int), not " + content;
        return FDS_ERR_FMT;
    }
    if (res > std::numeric_limits<uint64_t>::max()) {
        error_msg = "Number in element '" + std::string(opt->name)
            + "' is bigger than limit of unsigned int";
        return FDS_ERR_FMT;
    }

    // save node to content
    cont.id = opt->id;
    cont.type = OPTS_T_UINT;
    cont.val_uint = (uint64_t) res;

    // save context
    ctx->cont.push_back(cont);
    return FDS_OK;
}

/**
 * Save opt ID and content type to context
 *
 * \param[in,out] ctx Saved contents
 * \param[in]     opt Content ID
 * \return Ok on success, otherwise Err
 */
int
parse_context(fds_xml_ctx *ctx, const fds_xml_args *opt)
{
    fds_xml_cont cont{};

    cont.id = opt->id;
    cont.type = OPTS_T_CONTEXT;
    cont.ptr_ctx = nullptr;

    // save context
    ctx->cont.push_back(cont);
    return FDS_OK;
}

/**
 * Remove trailing and leading spaces from string
 *
 * \param[in] text String what wil be trimmed
 * \return Trimmed string, input string must exist
 */
void
remove_ws(std::string &text)
{
    /* erase end of string */
    text.erase(text.find_last_not_of(" \n\r\t") + 1);

    /* erase begin of string */
    size_t pos;
    if ((pos = text.find_first_not_of(" \n\r\t")) != 0) {
        text.erase(0, pos);
    }
}

/**
 * Convert content to wanted type and then save it to context
 *
 * \param[in]     content   String with content
 * \param[in,out] ctx       Structure with saved contents
 * \param[in]     opt       Contains content ID
 * \param[out]    error_msg Error message
 * \return OK on success, otherwise Err
 */
int
parse_content(
    const xmlChar *content, fds_xml_ctx *ctx, const fds_xml_args *opt, std::string &error_msg)
{
    std::string cur_content;
    if (content != nullptr) {
        cur_content = std::string((char *) content);
    }

    // remove WS if TRIM, don't remove if NOTRIM flag is on
    if ((opt->flags & OPTS_P_NOTRIM) == 0) {
        remove_ws(cur_content);
    }

    switch (opt->type) {
    case OPTS_T_CONTEXT:
        if (parse_context(ctx, opt) != FDS_OK)
            return FDS_ERR_FMT;
        break;
    case OPTS_T_UINT:
        if (parse_uint(cur_content, ctx, opt, error_msg) != FDS_OK)
            return FDS_ERR_FMT;
        break;
    case OPTS_T_BOOL:
        if (parse_bool(cur_content, ctx, opt, error_msg) != FDS_OK)
            return FDS_ERR_FMT;
        break;
    case OPTS_T_DOUBLE:
        if (parse_double(cur_content, ctx, opt, error_msg) != FDS_OK)
            return FDS_ERR_FMT;
        break;
    case OPTS_T_INT:
        if (parse_int(cur_content, ctx, opt, error_msg) != FDS_OK)
            return FDS_ERR_FMT;
        break;
    case OPTS_T_STRING:
        if (parse_string(cur_content, ctx, opt) != FDS_OK)
            return FDS_ERR_FMT;
        break;
    default:
        error_msg = "User element '" + std::string(opt->name) + "' has wrong type";
        return FDS_ERR_FMT;
    }

    return FDS_OK;
}

/**
 * \brief Parse all raw elements
 * \param[in]  node      Node with raw element
 * \param[in]  ctx       Context
 * \param[in]  opt       User define elements
 * \param[out] error_msg Error message
 * \return Ok on success, otherwise Err
 */
int
parse_raw(xmlNodePtr node, fds_xml_ctx *ctx, const fds_xml_args *opt, std::string &error_msg)
{
    // FIXME deprecated
    xmlBufferPtr buf = xmlBufferCreate();
    xmlNodeDump(buf, node->doc, node, 5, 1);

    const xmlChar *content = xmlBufferContent(buf);

    parse_content(content, ctx, opt, error_msg);

    xmlBufferFree(buf);
    return FDS_OK;
}

/**
 * Check if all user defined elements are in xml file
 *
 * \param[in] opt        User defined elements
 * \param[in] ids        Founded IDs in xml file
 * \param[out] error_msg Error message
 * \return Ok on success, otherwise Err
 */
int
parse_all_check(const fds_xml_args *opt, const std::set<int> &ids, std::string &error_msg)
{
    for (int i = 0; opt[i].comp != OPTS_C_TERMINATOR; ++i) {
        if (ids.find(opt[i].id) != ids.end()) {
            continue; // founded
        }

        // optional element
        if ((opt[i].flags & OPTS_P_OPT) != 0) {
            continue;
        }

        // not found
        error_msg = "Element " + get_type(&opt[i]) + " with ID " + std::to_string(opt[i].id)
            + " not found in xml";
        return FDS_ERR_FMT;
    }

    return FDS_OK;
}

/**
 * \brief Check if string contain only whitespaces
 * \param str String
 * \return True on success, otherwise False
 */
bool
is_empty(const char *str)
{
    if (str == nullptr) {
        return true;
    }

    for (int i = 0; str[i] != '\0'; ++i) {
        if (isspace(str[i]) == 0) {
            return false;
        }
    }
    return true;
}

/**
 * \brief Check if element have OPTS_TEXT even when it shouldn't have
 * \param[in]  node      Node
 * \param[in]  opts      Args
 * \param[out] error_msg Error message
 * \return True on success, otherwise False
 */
bool
have_text(const xmlNodePtr node, const fds_xml_args *opts, std::string& error_msg)
{
    if (is_empty((const char *) node->content)) {
        return true;
    }

    const fds_xml_args *opt = find_text(opts);
    if (opt == nullptr) {
        std::string str = std::string((char *) node->content);
        remove_ws(str);
        error_msg = "Line: " +std::to_string(node->line)+
                    " Element has not defined OPTS_TEXT, text '" +str+ "' is invalid";
        return false;
    }
    return true;
}

// This declaration must be here because of recursion
fds_xml_ctx *
parse_all(const fds_xml_args *opts, xmlNodePtr node, bool pedantic, std::string &error_msg);

/**
 * Go through all elements and parse them, nested recursively
 *
 * \param[in]     node      Beginning of xml, root node
 * \param[in,out] ctx       Parsed context
 * \param[in]     opts      User defined conditions
 * \param[in]     pedantic  Stop on warning
 * \param[out]    error_msg Error message
 * \param[in]     ids       Founded IDs in xml file
 * \return OK on success, otherwise Err
 */
int
parse_all_contents(const xmlNodePtr node, fds_xml_ctx *ctx, const fds_xml_args *opts, bool pedantic,
    std::string &error_msg, std::set<int> &ids)
{
    xmlNodePtr cur_node = node; // parsed node
    const fds_xml_args *opt = nullptr;
    int ret; // return value

    // when some element contain text and still is not nested
    if (cur_node != nullptr && cur_node->next == nullptr && cur_node->children == nullptr 
    && cur_node->content != nullptr) {
        opt = find_text(opts); // find element with text
        if (opt == nullptr) {
            if (pedantic) { // end
                error_msg = "Line: " + std::to_string(cur_node->line) + " Node '"
                    + std::string((char *) cur_node->name)
                    + "' doesn't contain optional description: "
                    + std::string((char *) cur_node->content);
                return FDS_ERR_FMT;
            }
        } else { // parse
            ret = parse_content(cur_node->content, ctx, opt, error_msg);
            if (ret != FDS_OK) {
                return ret;
            }
            ids.insert(opt->id); // save element ID
        }
    }

    for (; cur_node != nullptr; cur_node = cur_node->next) {
        if (cur_node->type == XML_TEXT_NODE) {
            if (!have_text(cur_node, opts, error_msg)) {
                return FDS_ERR_FMT;
            }
        }
        if (cur_node->type != XML_ELEMENT_NODE) {
            continue;
        }

        // find element with same name
        opt = find_arg(opts, cur_node->name);
        if (opt == nullptr) {
            if (!pedantic) {
                continue;
            }
            error_msg = "Line: " + std::to_string(cur_node->line) + " Element '"
                + std::string((char *) cur_node->name) + "' not defined";
            return FDS_ERR_FMT;
        }

        // only one occurrence
        if ((opt->flags & OPTS_P_MULTI) == 0) {
            if (ids.find(opt->id) != ids.end()) {
                error_msg = "Line: " + std::to_string(cur_node->line)
                    + " More than one occurrence of element '"
                    + std::string((char *) cur_node->name) + "'";
                return FDS_ERR_FMT;
            }
        }

        // parse element
        if (opt->comp == OPTS_C_RAW) {
            ret = parse_raw(cur_node, ctx, opt, error_msg);
        } else if (cur_node->children == nullptr) {
            ret = parse_content((xmlChar *) "", ctx, opt, error_msg);
        } else {
            ret = parse_content(cur_node->children->content, ctx, opt, error_msg);
        }
        if (ret != FDS_OK) {
            return ret;
        }
        ids.insert(opt->id); // save element ID

        // NESTED
        if (opt->comp == OPTS_C_NESTED) {
            ctx->cont.back().ptr_ctx = parse_all(opt->next, cur_node, pedantic, error_msg);
            if (ctx->cont.back().ptr_ctx == nullptr) {
                return FDS_ERR_FMT;
            }
        }
    }

    return FDS_OK;
}

/**
 * Go through all properties and parse them
 *
 * \param[in]     attr      Root attribute
 * \param[in,out] ctx       Parsed context
 * \param[in]     opts      User defined conditions
 * \param[in]     pedantic  Stop on warning
 * \param[out]    error_msg Error message
 * \param[out]    ids       Saved opt ID
 * \return OK on success, otherwise Err
 */
int
parse_all_properties(const xmlAttrPtr attr, fds_xml_ctx *ctx, const fds_xml_args *opts,
    bool pedantic, std::string &error_msg, std::set<int> &ids)
{
    const fds_xml_args *opt = nullptr; // found element with same name

    for (xmlAttrPtr cur_attr = attr; cur_attr != nullptr; cur_attr = cur_attr->next) {
        if (cur_attr->type != XML_ATTRIBUTE_NODE) {
            continue;
        }

        // find attribute with same name
        opt = find_arg(opts, cur_attr->name);
        if (opt == nullptr) {
            if (!pedantic) {
                continue;
            }
            error_msg = "Attribute '" + std::string((char *) cur_attr->name) + "' not defined";
            return FDS_ERR_FMT;
        }

        // parse it, libxml2 auto check if property always contain some context
        parse_content(cur_attr->children->content, ctx, opt, error_msg);
        ids.insert(opt->id);
    }

    return FDS_OK;
}

/**
 * Take one by one elements from XML file and parse them
 *
 * \param[in]  opts      User defined conditions
 * \param[in]  node      First node in XML file, except root node
 * \param[in]  pedantic  Strictly compare conditions
 * \param[out] error_msg If anything is wrong, fill this with error message
 * \return Context with parsed elements on success, otherwise nullptr
 */
fds_xml_ctx *
parse_all(const fds_xml_args *opts, xmlNodePtr node, bool pedantic, std::string &error_msg)
{
    std::set<int> ids;                  // parsed IDs in one level
    int ret;                            // return value
    auto ctx = new fds_xml_ctx;         // new context

    // parse properties
    ret = parse_all_properties(node->properties, ctx, opts, pedantic, error_msg, ids);
    if (ret != FDS_OK) {
        destroy_context(ctx);
        return nullptr;
    }

    // parse contents
    ret = parse_all_contents(node->children, ctx, opts, pedantic, error_msg, ids);
    if (ret != FDS_OK) {
        destroy_context(ctx);
        return nullptr;
    }

    // check if all user defined elements are in xml
    ret = parse_all_check(opts, ids, error_msg);
    if (ret != FDS_OK) {
        destroy_context(ctx);
        return nullptr;
    }

    return ctx;
}

/**
 * Handling libxml2 native error messages.
 *
 * \param[out] parser Parser with error message
 * \param[in]  msg    libxml2 error message.
 * \param ...
 */
void
error_handler(fds_xml *parser, const char *msg, ...)
{
    // get error message
    char message[1024];
    va_list arg_ptr;
    va_start(arg_ptr, msg);
    vsnprintf(message, 1024, msg, arg_ptr);
    va_end(arg_ptr);

    // save error message
    parser->error_msg += std::string(message);
}

/**
 * \brief Parse context
 * \param[in,out] parser   Parser
 * \param[in]     conf     Configuration
 * \param[in]     pedantic Pedantic
 * \return Parsed context on success, otherwise nullptr
 */
fds_xml_ctx_t*
ctx_parse(fds_xml_t *parser, unique_doc conf, bool pedantic)
{
    if (parser->ctx != nullptr) {
        destroy_context(parser->ctx);
        parser->ctx = nullptr;
    }

    // fds_xml_set_args was not call
    if (parser->opts == nullptr) {
        parser->error_msg = "Parser opts aren't set, first must be used fds_xml_set_args";
        return nullptr;
    }

    // read and check first element
    // libxml auto check whether there is only one root element
    xmlNodePtr node = xmlDocGetRootElement(conf.get());
    if (node == nullptr || !parser->error_msg.empty()) {
        return nullptr;
    }

    if (xmlStrcmp(BAD_CAST parser->opts[0].name, node->name) != 0) {
        parser->error_msg = "Name of the root element in file is '" 
        +std::string((char *) node->name) + "', should be " +get_type(&parser->opts[0]);
        return nullptr;
    }

    fds_xml_ctx *ctx = nullptr;
    try {
        ctx = parse_all(parser->opts+1, node, pedantic, parser->error_msg);
    } catch (...) {
        parser->error_msg = "Memory allocation problem for context";
        return nullptr;
    }
    // go through first level of XML, then recursively to other levels and parse XML mem to context
    if (ctx == nullptr) {
        return nullptr;
    }

    // save context to parser
    parser->ctx = ctx;
    return ctx;
}

/**
 * Check XML file with user defined conditions and parse it to better form.
 *
 * \param[in] parser   User defined conditions.
 * \param[in] mem      Part of XML file.
 * \param[in] pedantic Strictly compare conditions.
 * \return Context with saved elements on success, otherwise nullptr.
 */
fds_xml_ctx_t *
fds_xml_parse_mem(fds_xml_t *parser, const char *mem, bool pedantic)
{
    if (parser == nullptr)
        return nullptr;
    if (mem == nullptr) {
        parser->error_msg = "Mem points to nullptr";
        return nullptr;
    }

    LIBXML_TEST_VERSION;
    xmlInitParser();

    // error handling function
    auto handler = (xmlGenericErrorFunc) error_handler;
    xmlSetGenericErrorFunc(parser, handler);

    // create new parser context (multi thread safety)
    unique_ctx xmlCtx(xmlNewParserCtxt(), &::xmlFreeParserCtxt);
    if (xmlCtx == nullptr || !parser->error_msg.empty()) {
        parser->error_msg = "Failed to create context";
        return nullptr;
    }

    // read configuration
    unique_doc conf(xmlCtxtReadMemory(xmlCtx.get(), mem, (int) strlen(mem), nullptr /*??*/, nullptr, 0),
        &::xmlFreeDoc);
    if (conf == nullptr || !parser->error_msg.empty()) {
        xmlCtx.reset(nullptr);
        xmlCleanupParser();
        return nullptr;
    }

    fds_xml_ctx_t *ctx = ctx_parse(parser, std::move(conf), pedantic);

    xmlCtx.reset(nullptr);
    xmlCleanupParser();
    return ctx;
}

/**
 * Check XML file with user defined conditions and parse it to better form.
 *
 * \param[in] parser   User defined conditions.
 * \param[in] file     File descriptor
 * \param[in] pedantic Strictly compare conditions.
 * \return Context with saved elements on success, otherwise nullptr.
 */
fds_xml_ctx_t *
fds_xml_parse_file(fds_xml_t *parser, FILE *file, bool pedantic)
{
    if (parser == nullptr)
        return nullptr;
    if (file == nullptr) {
        parser->error_msg = "FILE points to nullptr";
        return nullptr;
    }

    LIBXML_TEST_VERSION;
    xmlInitParser();

    // error handling function
    auto handler = (xmlGenericErrorFunc) error_handler;
    xmlSetGenericErrorFunc(parser, handler);

    // create new parser context (multi thread safety)
    unique_ctx xmlCtx(xmlNewParserCtxt(), &::xmlFreeParserCtxt);
    if (xmlCtx == nullptr || !parser->error_msg.empty()) {
        parser->error_msg = "Failed to create context";
        return nullptr;
    }

    // read configuration
    unique_doc conf(xmlCtxtReadFd(xmlCtx.get(), fileno(file), nullptr /*??*/, nullptr, 0),
        &::xmlFreeDoc);
    if (conf == nullptr || !parser->error_msg.empty()) {
        xmlCtx.reset(nullptr);
        xmlCleanupParser();
        return nullptr;
    }

    fds_xml_ctx_t *ctx = ctx_parse(parser, std::move(conf), pedantic);

    xmlCtx.reset(nullptr);
    xmlCleanupParser();
    return ctx;
}

int
fds_xml_next(fds_xml_ctx_t *ctx, const struct fds_xml_cont **content)
{
    if (ctx == nullptr || content == nullptr) {
        return FDS_ERR_FMT;
    }

    if (ctx->index >= ctx->cont.size()) {
        return FDS_EOC;
    }

    *content = &ctx->cont[ctx->index];

    ctx->index++;
    return FDS_OK;
}

/**
 * Reset index to 0, recursively
 *
 * \param[in,out] ctx In which context
 */
void
fds_xml_rewind(fds_xml_ctx_t *ctx)
{
    if (ctx == nullptr) {
        return;
    }

    // rewind nested context
    for (const auto &cont : ctx->cont) {
        if (cont.type == OPTS_T_CONTEXT) {
            fds_xml_rewind(cont.ptr_ctx);
        }
    }

    ctx->index = 0;
}

/**
 * Get last error.
 *
 * \param[in] parser Error from which parser.
 * \return Error message.
 */
const char *
fds_xml_last_err(fds_xml_t *parser)
{
    if (parser == nullptr) {
        return nullptr;
    }

    if (parser->error_msg.empty()) {
        return "No error";
    }

    return parser->error_msg.c_str();
}
