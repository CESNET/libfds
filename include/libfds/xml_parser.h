/**
 * \file src/xml_parser.c
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \brief  Simple XML parser (header file)
 * \date   2017
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

/*
 * Implementacni poznamka:
 * - rekurzivni struktury musi byt podporovany (vyžadováno např. v konfiguraci
 *   profilovani toků)
 *   Např.:
 *     <class>
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
 *
 * TODO:
 * - Jakym zpusobem budou zajištěna podpora pro "namespace"
 *   U takových položek by šlo použít např. definici jako "ns:name", kde
 *   - "ns" je jmeno pro namespace
 *   - "name" je identifikator polozky
 *   Zde se ale neověřuje řetězec definovany v namespacu
 */

/** TODO zvlast soubor
 * <root>
 *     <timeout>300</timeout>      <- optional
 *     <host proto="TCP">          <- nested & required (1 or more times)
 *         <ip>127.0.0.1</ip>          <- required
 *         <port>4379</port>           <- required
 *     </host>
 * </root>
 *
 * ///user identification of XML elements
 * enum MODULE_XML {
 *     MODULE_TIMEOUT = 1,
 *     MODULE_HOST,
 *     HOST_PROTO,
 *     HOST_IP,
 *     HOST_PORT
 * };
 *
 * static const struct FDS_XML_ARGS args_host[] = {
 *         OPTS_ROOT("host"),
 *         OPTS_ATTR(HOST_PROTO, "proto", OPTS_T_STRING, 0),
 *         OPTS_ELEM(HOST_IP, "ip", OPTS_T_STRING, 0),
 *         OPTS_ELEM(HOST_PORT, "port", OPTS_T_UINT, 0),
 *         OPTS_END
 * };
 *
 * static const struct fds_xml_args args_main[] = {
 *         OPTS_ROOT("root"),
 *         OPTS_ELEM(MODULE_TIMEOUT, "timeout", OPTS_UINT, OPTS_P_OPT),
 *         OPTS_NESTED(MODULE_HOST, "host", args_host, OPTS_P_MULTI),
 *         OPTS_END
 * };
 *
 * void
 * parse_cfg(const char *cfg) {
 *     fds_xml_parser *parser;
 *
 *     if (fds_xml_create(args_main, &parser) != FDS_XML_OK) {
 *         // error
 *     }
 *
 *     if(fds_xml_set_args(args_main, parser) != FDS_XML_OK) {
 *         // error
 *     }
 *
 *     fds_xml_ctx_t *ctx = fds_xml_parse(parser, cfg, true);
 *     if (ctx == NULL) {
 *         // error
 *     }
 *
 *     struct fds_xml_cont content;
 *     while(fds_xml_next(ctx, &content) != FDS_XML_EOC) {
 *         switch (content.id) {
 *         case MODULE_TIMEOUT:
 *             // do something
 *             break;
 *         case MODULE_HOST:
 *             // nested
 *             parse_host(content.ptr_ctx);
 *             break;
 *         default:
 *             // unexpected element
 *             break;
 *         }
 *     }
 *
 *     fds_xml_destroy(parser);
 * }
 *
 * void
 * parse_host(fds_xml_ctx_t *ctx) {
 *     struct fds_xml_cont content;
 *     while(fds_xml_next(ctx, &content) != FDS_XML_EOC) {
 *         switch (content.id) {
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
 */

#ifndef XML_PARSER_H_
#define XML_PARSER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "api.h"

/** XML Documents component                                                   */
enum FDS_XML_COMP {
    OPTS_C_ROOT,      /**< Root element identification                     */
    OPTS_C_ELEMENT,   /**< Simple element (no attributes and no children)  */
    OPTS_C_ATTR,      /**< Attribute                                       */
    OPTS_C_TEXT,      /**< Text content                                    */
    OPTS_C_NESTED,    /**< Nested element (allows attributes + children)   */
    OPTS_C_TERMINATOR /**< Input termination (internal type)               */
};

/** Data type of an XML element (or attribute)                                */
enum FDS_XML_TYPE {
    OPTS_T_NONE,   /**< Invalid type (for internal use only)            */
    OPTS_T_BOOL,   /**< Boolean (true/false, yes/no, 1/0)               */
    OPTS_T_INT,    /**< Signed integer                                  */
    OPTS_T_UINT,   /**< Unsigned integer                                */
    OPTS_T_DOUBLE, /**< Double                                          */
    OPTS_T_STRING, /**< String                                          */
    OPTS_T_CONTEXT /**< Context of a nested element                     */
};

/**
 * \brief Define a parent element (optional)
 * \param[in] _name_  Name of the element
 */
#define OPTS_ROOT(_name_)                                                                          \
    {                                                                                              \
        OPTS_C_ROOT, OPTS_T_NONE, 0, _name_, NULL, 0                                               \
    }

/**
 * \brief Define an XML element
 * \param[in] _id_    User identification of an element
 * \param[in] _name_  Name of the element
 * \param[in] _type_  Data type of the element
 * \param[in] _flags_ Properties
 */
#define OPTS_ELEM(_id_, _name_, _type_, _flags_)                                                   \
    {                                                                                              \
        OPTS_C_ELEMENT, _type_, _id_, _name_, NULL, _flags_                                        \
    }

/**
 * \brief Define a text content
 * \param[in] _id_    User identification of the content
 * \param[in] _type_  Data type of the text (usually just string)
 * \param[in] _flags_ Properties
 * \warning There cannot be more than one text context inside an element!
 */
#define OPTS_TEXT(_id_, _type_, _flags_)                                                           \
    {                                                                                              \
        OPTS_C_TEXT, _type_, _id_, NULL, NULL, _flags_                                             \
    }

/**
 * \brief Define an XML attribute
 * \param[in] _id_    User identification of an attribute
 * \param[in] _name_  Name of the attribute
 * \param[in] _type_  Data type of the attribute
 * \param[in] _flags_ Properties
 * \warning Flag #OPTS_P_MULTI is not allowed.
 */
#define OPTS_ATTR(_id_, _name_, _type_, _flags_)                                                   \
    {                                                                                              \
        OPTS_C_ATTR, _type_, _id_, _name_, NULL, _flags_                                           \
    }

/**
 * \brief Define a nested XML element
 * \param[in] _id_    User identification of an element
 * \param[in] _name_  Name of the element
 * \param[in] _ptr_   Pointer to the description of nested element
 * \param[in] _flags_ Properties
 */
#define OPTS_NESTED(_id_, _name_, _ptr_, _flags_)                                                  \
    {                                                                                              \
        OPTS_C_NESTED, OPTS_T_CONTEXT, _id_, _name_, _ptr_, _flags_                                \
    }

/**
 * \brief Terminator of a description array of XML elements
 * \warning This element always MUST be last field in the array!
 */
#define OPTS_END                                                                                   \
    {                                                                                              \
        OPTS_C_TERMINATOR, OPTS_T_NONE, 0, NULL, NULL, 0                                           \
    }

/** Status code for success                                                   */
#define FDS_XML_OK 0
/** Status code for the end of a context                                      */
#define FDS_XML_EOC (-1)
/** Status code for memory allocation error                                   */
#define FDS_XML_ERR_NOMEM (-2)
/** Invalid format description for parser                                     */
#define FDS_XML_ERR_FMT (-3)

/**
 * Component properties
 *
 * By default, an XML element is requried exactly once in the context of its
 * occurrence. This can be modified using #OPTS_P_OPT and/or #OPTS_P_MULTI.
 * For example, 0 - N occurrences can be reached using combination of flags
 * (OPTS_P_OPT | OPTS_P_MULTI)
 */
enum FDS_XML_FLAG {
    OPTS_P_OPT = 1,   /**< Optional occurrence (zero or one occurrence)      */
    OPTS_P_MULTI = 2, /**< Allow multiple occurrences of the same element    */
    OPTS_P_NOTRIM = 4 /**< Do not trim leading and tailing whitespaces before
                        *  conversion/processing                             */
};

/** Internal description of XML elements, attributes, etc.                    */
struct fds_xml_args {
    enum FDS_XML_COMP comp;          /**< Type of component                  */
    enum FDS_XML_TYPE type;          /**< Data type                          */
    int id;                          /**< User identification of an element  */
    const char *name;                /**< XML name of the element            */
    const struct fds_xml_args *next; /**< Pointer to the nested structure    */
    int flags;                       /**< Properties                         */
};

/** Description of a parsed element                                           */
struct fds_xml_cont {
    /** User identification of the element                                    */
    int id;
    /** Data type of the value                                                */
    enum FDS_XML_TYPE type;
    /** Pointer to the value                                                  */
    union {
        bool val_bool;               /**< Boolean                         */
        int64_t val_int;             /**< Signed integer                  */
        uint64_t val_uint;           /**< Unsigned integer                */
        double val_double;           /**< Double                          */
        char *ptr_string;            /**< String                          */
        struct fds_xml_ctx *ptr_ctx; /**< Context of the nested element   */
    };
};

typedef struct fds_xml fds_xml_t;
typedef struct fds_xml_ctx fds_xml_ctx_t;

/**
 * \brief Create an XML parser
 * \param[out] parser Pointer to the newly created parser
 * \return On success returns #FDS_XML_OK. Otherwise #FDS_XML_ERR_NOMEM or
 *   #FDS_XML_ERR_FMT.
 */
FDS_API int
fds_xml_create(fds_xml_t **parser);

/**
 * \brief Destroy an XML parser
 * \param[in,out] parser Parser
 */
FDS_API void
fds_xml_destroy(fds_xml_t *parser);

/**
 * \brief Check user defined conditions and save opts to parser
 *
 * \param[in]  opts   Pointer to the definition of the root element
 * \param[out] parser Pointer to filled parser
 * \return
 */
FDS_API int
fds_xml_set_args(const struct fds_xml_args *opts, fds_xml_t *parser);

/**
 * \brief Parse an XML document
 *
 * After successful document parsing it is guaranteed that all elements have
 * met all required conditions.
 * \param[in] parser   Parser
 * \param[in] mem      XML configuration
 * \param[in] pedantic If enabled, all unexpected XML elements are considered
 *   as errors. Otherwise unexpected elements are ignored.
 * \return On success returns a pointer to the context of the root element.
 *   Otherwise returns NULL and an error message is set
 *   (see fds_xml_last_err()).
 */
FDS_API fds_xml_ctx_t *
fds_xml_parse(fds_xml_t *parser, const char *mem, bool pedantic);

/**
 * \brief Get the next option
 *
 * The content of the element (with type corresponding to the definition) is
 * filled to the \p content.
 * \param[in]  ctx Parser context
 * \param[out] content Value of the element (or attribute)
 * \return On success returns #FDS_XML_OK.
 *   If all options have been parsed, then returns #FDS_XML_EOC.
 */
FDS_API int
fds_xml_next(fds_xml_ctx_t *ctx, const struct fds_xml_cont **content);

/**
 * \brief Rewind iterator
 *
 * The function resets the iterator position for to the beginnig of the current
 * context.
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

#endif // XML_PARSER_H_
