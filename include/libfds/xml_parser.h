/**
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \brief  Simple XML parser (header file)
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

/*
 * TODO: support recursive structures (required by, for example, profiling)
 *    <class>
 *		 <name>CLASS A</name>
 *       <subclasses>
 *         <class>
 *           <name>CLASS A1</name>
 *         </class>
 *         <class>
 *           <name>CLASS A2</name>
 *         </class>
 *       <subclasses>
 *     </class>
 */

/**
 * \defgroup fds_xml_parser XML parser
 * \ingroup publicAPIs
 * \brief Simple XML parser with type and tag occurrence check
 *
 * The main purpose of this parser is to make parsing XML documents easier. Working directly with
 * libxml2 library is not always easy, therefore, this parser represents simplified interface
 * with few enhancements over the library.
 *
 * User MUST describe an XML document to parser using simple C structures. The description consists
 * of expected XML elements and attributes, their data type, identification numbers and occurrence
 * indicators. It allows the parser to check if all requirements are met during parsing so the
 * user doesn't have to check it manually. To make processing of parsed document easy, all
 * elements and attributes are mapped to user defined identification numbers. In other words,
 * the user doesn't have to compare strings or elements or attributes. XML namespaces are ignored.
 *
 * Example usage:
 *
 * Let's say that we want to parse following document:
 * \verbatim
 *   <params>
 *     <timeout>300</timeout>      <!-- optional -->
 *     <host proto="TCP">          <!-- nested & required (1 or more times) -->
 *       <ip>127.0.0.1</ip>        <!-- required -->
 *       <port>4379</port>         <!-- required -->
 *     </host>
 *   </params>
 * \endverbatim
 *
 * We have to describe an XML document structure. First, create a unique identification number
 * for all elements and arguments (enum module_xml) and describe the the document root (structure
 * args_main). As you can see the root of the document is called "params" (see FDS_OPTS_ROOT)
 * and consists of 2 children, "timeout" and "host". The timeout is simple element that should
 * hold unsigned number and it is also optional. To describe this, we use FDS_OPTS_ELEM with type
 * FDS_OPTS_T_UINT and property FDS_OPTS_P_OPT. On the other hand, the host is a nested node
 * (see FDS_OPTS_NESTED) with a parameter. Nested structures are described by another document
 * description structures. In this case, it is called args_host.
 *
 * NOTE: if #FDS_OPTS_P_OPT or #FDS_OPTS_P_MULTI is not defined as an element/attribute option,
 *   the parser expects that the element must occur exactly once.
 * NOTE: all structure descriptions MUST be ended with #FDS_OPTS_END.
 *
 * \code{.c}
 * enum module_xml {
 *     MODULE_TIMEOUT = 1,
 *     MODULE_HOST,
 *     HOST_PROTO,
 *     HOST_IP,
 *     HOST_PORT
 * };
 * static const struct fds_xml_args args_host[] = {
 *         FDS_OPTS_ATTR(HOST_PROTO, "proto", FDS_OPTS_T_STRING, 0),
 *         FDS_OPTS_ELEM(HOST_IP,    "ip",    FDS_OPTS_T_STRING, 0),
 *         FDS_OPTS_ELEM(HOST_PORT,  "port",  FDS_OPTS_T_UINT,   0),
 *         FDS_OPTS_END
 * };
 * static const struct fds_xml_args args_main[] = {
 *         FDS_OPTS_ROOT("params"),
 *         FDS_OPTS_ELEM(MODULE_TIMEOUT, "timeout", FDS_OPTS_T_UINT, FDS_OPTS_P_OPT),
 *         FDS_OPTS_NESTED(MODULE_HOST,  "host",    args_host,       FDS_OPTS_P_MULTI),
 *         FDS_OPTS_END
 * };
 *
 * \endcode
 *
 * When the description of the XML document is ready. Is time to use it inside a parser. Here
 * is an example code that tries to parse the document described above. Note that the function
 * fds_xml_set_args() takes the description of the root.
 *
 * \code{.c}
 * void
 * parse_host(fds_xml_ctx_t *ctx) {
 *     const struct fds_xml_cont *content;
 *     while(fds_xml_next(ctx, &content) != FDS_EOC) {
 *         switch (content->id) {
 *         case HOST_PROTO:
 *             // do something
 *             break;
 *         case HOST_IP:
 *             // do something
 *             break;
 *         case HOST_PORT:
 *             //do something
 *             break;
 *         default:
 *             // unexpected element
 *             break;
 *         }
 *     }
 * }
 *
 * void
 * parse_cfg(const char *cfg) {
 *     fds_xml_t *parser = fds_xml_create();
 *     if (!parser) {
 *         // error
 *     }
 *     if (fds_xml_set_args(parser, args_main) != FDS_OK) {
 *         // error
 *     }
 *     fds_xml_ctx_t *ctx = fds_xml_parse_mem(parser, cfg, true);
 *     if (ctx == NULL) {
 *         // error
 *     }
 *     const struct fds_xml_cont *content;
 *     while(fds_xml_next(ctx, &content) != FDS_EOC) {
 *         switch (content->id) {
 *         case MODULE_TIMEOUT:
 *             // do something
 *             break;
 *         case MODULE_HOST:
 *             // nested
 *             parse_host(content->ptr_ctx);
 *             break;
 *         default:
 *             // unexpected element
 *             break;
 *         }
 *     }
 *     fds_xml_destroy(parser);
 * }
 * \endcode
 *
 * @{
 */

#ifndef LIBFDS_XML_PARSER_H
#define LIBFDS_XML_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <libfds/api.h>

/** XML Documents component                                                   */
enum fds_opts_comp {
    FDS_OPTS_C_ROOT,       /**< Root element identification                     */
    FDS_OPTS_C_ELEMENT,    /**< Simple element (no attributes and no children)  */
    FDS_OPTS_C_ATTR,       /**< Attribute                                       */
    FDS_OPTS_C_TEXT,       /**< Text content                                    */
    FDS_OPTS_C_NESTED,     /**< Nested element (allows attributes + children)   */
    FDS_OPTS_C_TERMINATOR, /**< Input termination (internal type)               */
    FDS_OPTS_C_RAW         /**< Raw content of an element                       */
};

/** Data type of an XML element (or attribute)                                */
enum fds_opts_type {
    FDS_OPTS_T_NONE,   /**< Invalid type (for internal use only)            */
    FDS_OPTS_T_BOOL,   /**< Boolean (true/false, yes/no, 1/0)               */
    FDS_OPTS_T_INT,    /**< Signed integer                                  */
    FDS_OPTS_T_UINT,   /**< Unsigned integer                                */
    FDS_OPTS_T_DOUBLE, /**< Double                                          */
    FDS_OPTS_T_STRING, /**< String                                          */
    FDS_OPTS_T_CONTEXT /**< Context of a nested element                     */
};

/**
 * \brief Define a parent element (optional)
 * \param[in] name  Name of the element
 */
#define FDS_OPTS_ROOT(name) \
    {FDS_OPTS_C_ROOT, FDS_OPTS_T_NONE, 0, (name), NULL, 0}

/**
 * \brief Define an XML element
 * \param[in] id    User identification of an element
 * \param[in] name  Name of the element
 * \param[in] type  Data type of the element
 * \param[in] flags Properties
 */
#define FDS_OPTS_ELEM(id, name, type, flags) \
    {FDS_OPTS_C_ELEMENT, (type), (id), (name), NULL, (flags)}

/**
 * \brief Define a text content
 * \param[in] id    User identification of the content
 * \param[in] type  Data type of the text (usually just string)
 * \param[in] flags Properties
 * \warning There cannot be more than one text context inside an element!
 */
#define FDS_OPTS_TEXT(id, type, flags) \
    {FDS_OPTS_C_TEXT, (type), (id), NULL, NULL, (flags)}

/**
 * \brief Define an XML attribute
 * \param[in] id    User identification of an attribute
 * \param[in] name  Name of the attribute
 * \param[in] type  Data type of the attribute
 * \param[in] flags Properties
 * \warning Flag #FDS_OPTS_P_MULTI is not allowed.
 */
#define FDS_OPTS_ATTR(id, name, type, flags) \
    {FDS_OPTS_C_ATTR, (type), (id), (name), NULL, (flags)}

/**
 * \brief Define a nested XML element
 * \param[in] id    User identification of an element
 * \param[in] name  Name of the element
 * \param[in] ptr   Pointer to the description of nested element
 * \param[in] flags Properties
 */
#define FDS_OPTS_NESTED(id, name, ptr, flags) \
    {FDS_OPTS_C_NESTED, FDS_OPTS_T_CONTEXT, (id), (name), (ptr), (flags)}

/**
 * \brief Define a raw XML element
 * \param[in] id    User identification of an element
 * \param[in] name  Name of the element
 * \param[in] flags Properties
 */
#define FDS_OPTS_RAW(id, name, flags) \
    {FDS_OPTS_C_RAW, FDS_OPTS_T_STRING, (id), (name), NULL, (flags)}

/**
 * \brief Terminator of a description array of XML elements
 * \warning This element always MUST be last field in the array!
 */
#define FDS_OPTS_END \
    {FDS_OPTS_C_TERMINATOR, FDS_OPTS_T_NONE, 0, NULL, NULL, 0}

/**
 * Component properties
 *
 * By default, an XML element is required exactly once in the context of its occurrence.
 * This can be modified using #FDS_OPTS_P_OPT and/or #FDS_OPTS_P_MULTI. For example,
 * 0 - N occurrences can be reached using combination of flags (FDS_OPTS_P_OPT | FDS_OPTS_P_MULTI)
 */
enum fds_opts_properties {
    /** Optional occurrence (zero or one occurrence)                                  */
    FDS_OPTS_P_OPT = 1,
    /** Allow multiple occurrences of the same element (one or more)                  */
    FDS_OPTS_P_MULTI = 2,
    /** Do not trim leading and tailing whitespaces before conversion/processing      */
    FDS_OPTS_P_NOTRIM = 4
};

/** Internal description of XML elements, attributes, etc.                            */
struct fds_xml_args {
    enum fds_opts_comp comp;         /**< Type of component                           */
    enum fds_opts_type type;         /**< Data type                                   */
    int id;                          /**< User identification of an element           */
    const char *name;                /**< XML name of the element                     */
    const struct fds_xml_args *next; /**< Pointer to the nested structure             */
    int flags;                       /**< Properties                                  */
};

/** Description of a parsed element                                                    */
struct fds_xml_cont {
    /** User identification of the element                                             */
    int id;
    /** Data type of the value                                                         */
    enum fds_opts_type type;
    /** Pointer to the value                                                           */
    union {
        bool val_bool;               /**< Boolean                                      */
        int64_t val_int;             /**< Signed integer                               */
        uint64_t val_uint;           /**< Unsigned integer                             */
        double val_double;           /**< Double                                       */
        char *ptr_string;            /**< String                                       */
        struct fds_xml_ctx *ptr_ctx; /**< Context of the nested element                */
    };
};

/** Internal XML parser type  */
typedef struct fds_xml fds_xml_t;
/** Internal XML context type */
typedef struct fds_xml_ctx fds_xml_ctx_t;

/**
 * \brief Create an XML parser
 * \return Pointer to the newly created parser or NULL (memory allocation error)
 */
FDS_API fds_xml_t *
fds_xml_create();

/**
 * \brief Destroy an XML parser
 * \param[in,out] parser Parser
 */
FDS_API void
fds_xml_destroy(fds_xml_t *parser);

/**
 * \brief Check user defined conditions and save opts to parser
 *
 * \param[out] parser Pointer to filled parser
 * \param[in]  opts   Pointer to the XML definition of the root element
 * \return #FDS_OK on success
 * \return #FDS_ERR_NOMEM if a memory allocation error has occurred
 * \return #FDS_ERR_FORMAT if an XML document definition is invalid and an error message is set.
 */
FDS_API int
fds_xml_set_args(fds_xml_t *parser, const struct fds_xml_args *opts);

/**
 * \brief Parse an XML from a memory
 *
 * After successful document parsing it is guaranteed that all elements have met all required
 * conditions.
 * \param[in] parser   Parser
 * \param[in] mem      XML configuration
 * \param[in] pedantic If enabled, all unexpected XML elements are considered as errors. Otherwise
 *   unexpected elements are ignored.
 * \return On success returns a pointer to the context of the root element.
 *   Otherwise returns NULL and an error message is set (see fds_xml_last_err()).
 */
FDS_API fds_xml_ctx_t *
fds_xml_parse_mem(fds_xml_t *parser, const char *mem, bool pedantic);

/**
 * \brief Parse an XML from a file
 *
 * After successful document parsing it is guaranteed that all elements have met all required
 * conditions.
 * \param[in] parser   Parser
 * \param[in] file     XML configuration
 * \param[in] pedantic If enabled, all unexpected XML elements are considered as errors.
 *   Otherwise unexpected elements are ignored.
 * \return On success returns a pointer to the context of the root element.
 *   Otherwise returns NULL and an error message is set (see fds_xml_last_err()).
 */
FDS_API fds_xml_ctx_t *
fds_xml_parse_file(fds_xml_t *parser, FILE *file, bool pedantic);

/**
 * \brief Get the next option
 *
 * The content of the element (with type corresponding to the definition) is filled to
 * the \p content.
 * \param[in]  ctx     Parser context
 * \param[out] content Value of the element (or attribute)
 * \return On success returns FDS_OK.
 *   If all options have been parsed, then returns FDS_EOC.
 */
FDS_API int
fds_xml_next(fds_xml_ctx_t *ctx, const struct fds_xml_cont **content);

/**
 * \brief Rewind iterator
 *
 * The function resets the iterator position to the beginning of the current context.
 * \param[in] ctx Parser context
 */
FDS_API void
fds_xml_rewind(fds_xml_ctx_t *ctx);

/**
 * \brief Get the last error message
 * \param[in] parser Parser
 * \return Error message
 */
FDS_API const char *
fds_xml_last_err(fds_xml_t *parser);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif // LIBFDS_XML_PARSER_H
