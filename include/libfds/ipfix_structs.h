/**
 * \file include/libfds/ipfix_structs.h
 * \author Lukas Hutak <xhutak01@cesnet.cz>
 * \author Radek Krejci <rkrejci@cesnet.cz>
 * \brief Structures and macros definition for IPFIX processing
 */

/* Copyright (C) 2009-2018 CESNET, z.s.p.o.
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
 * This software is provided ``as is, and any express or implied
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

#ifndef FDS_IPFIX_STRUCTS_H
#define FDS_IPFIX_STRUCTS_H

/**
 * \defgroup ipfixStructures IPFIX Structures
 * \ingroup publicAPIs
 * \brief IPFIX structures allowing access to the raw data
 *
 * \warning
 *   All fields of all structures are stored in Network Byte Order (i.e. Big Endian Order). To read
 *   the content of the fields, you MUST ALWAYS use conversion functions from "Network Byte Order"
 *   to "Host Byte Order", such as ntohl, be64toh, etc. To write the content to the fields, you
 *   MUST use conversion functions from "Host Byte Order" to "Network Byte Order", such as htonl,
 *   htobe64, etc. See Linux manual pages of the functions. Note: "Host Byte Order" represents byte
 *   order of a machine, where this source code is build.
 *
 * \remark Based on RFC 7011
 *
 * @{
 */

#include <stdint.h>

/**
 * \struct fds_ipfix_msg_hdr
 * \brief IPFIX Message header structure
 *
 * This structure represents always present header of each IPFIX message.
 * \remark Based on RFC 7011, Section 3.1.
 */
struct __attribute__((__packed__)) fds_ipfix_msg_hdr {
    /**
     * \brief
     * Version of Flow Record format exported in this message.
     *
     * The value of this field is 0x000a for the current version, incrementing by one the version
     * used in the NetFlow services export version 9.
     */
    uint16_t version;

    /**
     * \brief
     * Total length of the IPFIX Message, measured in octets, including Message Header and Set(s).
     */
    uint16_t length;

    /**
     * \brief
     * Time at which the IPFIX Message Header leaves the Exporter.
     *
     * Expressed in seconds since the UNIX epoch of 1 January 1970 at 00:00 UTC, encoded as
     * an unsigned 32-bit integer.
     */
    uint32_t export_time;

    /**
     * \brief
     * Sequence counter
     *
     * Incremental sequence counter modulo 2^32 of all IPFIX Data Records sent in the current
     * stream from the current Observation Domain by the Exporting Process.
     *
     * \warning
     *   Each SCTP Stream counts sequence numbers separately, while all messages in a TCP connection
     *   or UDP session are considered to be part of the same stream.
     * \remark
     *   This value can be used by the Collecting Process to identify whether any IPFIX Data
     *   Records have been missed. Template and Options Template Records do not increase
     *   the Sequence Number.
     */
    uint32_t seq_num;

    /**
     * \brief
     * Observation Domain ID
     *
     * A 32-bit identifier of the Observation Domain that is locally unique to the Exporting
     * Process. The Exporting Process uses the Observation Domain ID to uniquely identify to
     * the Collecting Process the Observation Domain that metered the Flows. It is RECOMMENDED that
     * this identifier also be unique per IPFIX Device.
     *
     * \remark
     *   Collecting Processes SHOULD use the Transport Session and the Observation Domain ID field
     *   to separate different export streams originating from the same Exporter.
     * \remark
     *   The Observation Domain ID SHOULD be 0 when no specific Observation Domain ID is relevant
     *   for the entire IPFIX Message, for example, when exporting the Exporting Process Statistics,
     *   or in the case of a hierarchy of Collectors when aggregated Data Records are exported.
     */
    uint32_t odid;
};

/** IPFIX identification (NetFlow version 10)      */
#define FDS_IPFIX_VERSION      0x000a
/** Length of the IPFIX Message header (in bytes). */
#define FDS_IPFIX_MSG_HDR_LEN  16

/**
 * \struct fds_ipfix_set_hdr
 * \brief Common IPFIX Set (header) structure
 * \remark Based on RFC 7011, Section 3.3.2.
 */
struct __attribute__((__packed__)) fds_ipfix_set_hdr {
    /**
     * \brief Identifies the Set.
     *
     * A value of 2 (::FDS_IPFIX_SET_TMPLT) is reserved for Template Sets (see structure
     * fds_ipfix_tset). A value of 3 (::FDS_IPFIX_SET_OPTS_TMPLT) is reserved for Options Template
     * Sets (see structure fds_ipfix_opts_tset). Values from 4 to 255 are reserved for future use.
     * Values 256 and above are used for Data Sets (see structure fds_ipfix_dset). The Set ID
     * values of 0 and 1 are not used, for historical reasons.
     * \see For options enumeration see #fds_ipfix_set_id
     */
    uint16_t flowset_id;

    /**
     * \brief Length
     *
     * Total length of the Set, in octets, including the Set Header, all records, and the optional
     * padding. Because an individual Set MAY contain multiple records, the Length value MUST be
     * used to determine the position of the next Set.
     */
    uint16_t length;
};

/** Length of IPFIX Set header (in bytes)  */
#define FDS_IPFIX_SET_HDR_LEN 4

/** Flowset type identifiers */
enum fds_ipfix_set_id {
    FDS_IPFIX_SET_TMPLT       = 2,  /**< Template Set ID              */
    FDS_IPFIX_SET_OPTS_TMPLT  = 3,  /**< Options Template Set ID      */
    FDS_IPFIX_SET_MIN_DSET    = 256 /**< Minimum ID for any Data Set  */
};

/**
 * \typedef fds_ipfix_tmplt_ie
 * \brief Template's definition of an IPFIX Information Element.
 *
 * The type is defined as 32b value containing one of (union) Enterprise Number
 * or standard element definition containing IE ID and its length. There are
 * two #fds_ipfix_tmplt_ie_u in the following scheme.
 * <pre>
 *   0                   1                   2                   3
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |E|  Information Element ident. |        Field Length           |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                      Enterprise Number                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * </pre>
 *
 * \remark Based on RFC 7011, Section 3.2.
 */
typedef union __attribute__((__packed__)) fds_ipfix_tmplt_ie_u {
    /**
     * \brief
     * Information Element identifier and the length of the corresponding encoded
     * Information Element
     */
    struct {
        /**
         * \brief
         * A numeric value that represents the Information Element.
         *
         * \warning
         *   The first (highest) bit is Enterprise bit. This is the first bit of the Field
         *   Specifier. If this bit is zero, the Information Element identifier identifies
         *   an Information Element in
         *   [<a href="http://www.iana.org/assignments/ipfix/ipfix.xhtml"> IANA-IPFIX</a>], and
         *   the four-octet Enterprise Number field MUST NOT be present. If this bit is one, the
         *   Information Element identifier identifies an enterprise-specific Information Element,
         *   and the Enterprise Number filed MUST be present.
         */
        uint16_t id;

        /**
         * \brief
         * The length of the corresponding encoded Information Element, in octets.
         *
         * The value ::FDS_IPFIX_VAR_IE_LEN (65535) is reserved for variable-length Information
         * Elements.
         */
        uint16_t length;
    } ie;

    /**
     * \brief
     * IANA enterprise number of the authority defining the Information Element identifier in this
     * Template Record.
     *
     * Refer to [<a href="http://www.iana.org/assignments/enterprise-numbers/">
     * IANA Private Enterprise Numbers</a>].
     */
    uint32_t enterprise_number;
} fds_ipfix_tmplt_ie;

/** Length value signaling variable-length IE  */
#define FDS_IPFIX_VAR_IE_LEN 65535

/**
 * \struct fds_ipfix_trec
 * \brief Structure of the IPFIX Template Record Header
 *
 * This record MUST be inside the IPFIX Template Set (see struct fds_ipfix_tset).
 */
struct __attribute__((__packed__)) fds_ipfix_trec {
    /**
     * \brief Template Identification Number
     *
     * Each Template Record is given a unique Template ID in the range 256
     * (::FDS_IPFIX_SET_MIN_DSET) to 65535.  This uniqueness is local to the Transport Session and
     * Observation Domain that generated the Template ID. Since Template IDs are used as Set IDs
     * in the Sets they describe, values 0-255 are reserved for special Set types (e.g., Template
     * Sets themselves).
     *
     * \remark
     *   Templates and Options Templates cannot share Template IDs within a Transport Session and
     *   Observation Domain.
     * \remark
     *   There are no constraints regarding the order of the Template ID allocation. As Exporting
     *   Processes are free to allocate Template IDs as they see fit, Collecting Processes MUST NOT
     *   assume incremental Template IDs, or anything about the contents of a Template based on its
     *   Template ID alone.
     */
    uint16_t template_id;

    /**
     * \brief Number of fields in this Template Record.
     *
     * The number of fields MUST be greater than 0. Otherwise (equal to 0) this is not Template
     * Record, but it is Template Withdrawal Record (see the structure fds_ipfix_wdrl_trec).
     * These types of records MUST not be mixed in the same Set.
     */
    uint16_t count;

    /**
     * \brief
     * Field Specifier(s)
     */
    fds_ipfix_tmplt_ie fields[1];
};

/**
 * \struct fds_ipfix_tset
 * \brief IPFIX Template Set structure
 *
 * Consists of the common Set header and the first Template record.
 */
struct __attribute__((__packed__)) fds_ipfix_tset {
    /**
     * Common IPFIX Set header.
     * Identification of the flowset MUST be 2 (::FDS_IPFIX_SET_TMPLT)
     */
    struct fds_ipfix_set_hdr header;

    /**
     * The first of template records in this Template Set. Real size of the record is unknown
     * here due to a variable count of fields in each record.
     */
    struct fds_ipfix_trec first_record;
};

/**
 * \struct fds_ipfix_opts_trec
 * \brief Structure of the IPFIX Options Template record
 *
 * This record MUST be inside the IPFIX Options Template Set (see struct fds_ipfix_opts_tset).
 */
struct __attribute__((__packed__)) fds_ipfix_opts_trec {
    /**
     * \brief Template Identification Number
     *
     * Each Template Record is given a unique Template ID in the range 256
     * (::FDS_IPFIX_SET_MIN_DSET) to 65535. This uniqueness is local to the Transport Session and
     * Observation Domain that generated the Template ID. Since Template IDs are used as Set IDs
     * in the Sets they describe, values 0-255 are reserved for special Set types (e.g., Template
     * Sets themselves).
     *
     * \remark
     *   Templates and Options Templates cannot share Template IDs within a Transport Session and
     *   Observation Domain.
     * \remark
     *   There are no constraints regarding the order of the Template ID allocation. As Exporting
     *   Processes are free to allocate Template IDs as they see fit, Collecting Processes MUST NOT
     *   assume incremental Template IDs, or anything about the contents of a Template based on its
     *   Template ID alone.
     */
    uint16_t template_id;

    /**
     * \brief Number of all fields in this Options Template Record, including the Scope Fields.
     *
     * The number of fields MUST be greater than 0. Otherwise (equal to 0) this is not
     * Options Template Record, but it is Template Withdrawal Record (see the structure
     * fds_ipfix_wdrl_trec). These types of records MUST NOT be mixed in the same Set.
     */
    uint16_t count;

    /**
     * \brief Number of scope fields in this Options Template Record.
     *
     * The Scope Fields are normal Fields except that they are interpreted as scope at the
     * Collector. A scope field count of N specifies that the first N Field Specifiers in the
     * Template Record are Scope Fields
     * \warning The Scope Field Count MUST NOT be zero.
     */
    uint16_t scope_field_count;

    /**
     * \brief
     * Field Specifier(s)
     */
    fds_ipfix_tmplt_ie fields[1];
};

/**
 * \struct fds_ipfix_opts_tset
 * \brief IPFIX Options Template Set structure
 *
 * Consists of the common Set header and the first Options Template record.
 */
struct __attribute__((__packed__)) fds_ipfix_opts_tset {
    /**
     * Common IPFIX Set header.
     * Identification of the flowset MUST be 3 (::FDS_IPFIX_SET_OPTS_TMPLT)
     */
    struct fds_ipfix_set_hdr header;

    /**
     * The first of templates records in this Options Template Set. Real size of
     * the record is unknown here due to a variable count of fields in each
     * record.
     */
    struct fds_ipfix_opts_trec first_record;
};

/**
 * \struct fds_ipfix_wdrl_trec
 * \brief Structure of the IPFIX Template Withdrawal record
 * \remark Based on RFC 7011, Section 8.1.
 */
struct __attribute__((__packed__)) fds_ipfix_wdrl_trec {
    /**
     * \brief Template Identification Number
     *
     * Identification of the Template(s) to withdraw local to the Transport Session and Observation
     * Domain that generated the Template ID. If a unique Template ID is in the range 256
     * (::FDS_IPFIX_SET_MIN_DSET) to 65535, only the given template is withdrawn. If the reserved
     * values 2 (::FDS_IPFIX_SET_TMPLT) or 3 (::FDS_IPFIX_SET_OPTS_TMPLT) are used, all templates
     * of specified kind (i.e. data or options) are withdrawn.
     *
     * \remark
     *   The flowset ID of the parent IPFIX Set MUST contain the value 2 (::FDS_IPFIX_SET_TMPLT)
     *   for Template Set Withdrawal or the value 3 (::FDS_IPFIX_SET_OPTS_TMPLT) for Options
     *   Template Set Withdrawal.
     */
    uint16_t template_id;

    /**
     * \brief Number of fields.
     * \warning This field MUST be always 0. Otherwise it is NOT a template withdrawal record.
     */
    uint16_t count;
};

/** Size of a Template withdrawal record */
#define FDS_IPFIX_WDRL_TREC_LEN  4

/**
 * \struct fds_ipfix_wdrl_tset
 * \brief IPFIX (Options) Template Set Withdrawal structure
 *
 * Consists of the common Set header and the first Template Withdrawal record.
 */
struct __attribute__((__packed__)) fds_ipfix_wdrl_tset {
    /**
     * Common IPFIX Set header.
     * Identification of the flowset MUST be 2 (::FDS_IPFIX_SET_TMPLT) for (Data) Templates
     * or 3 (::FDS_IPFIX_SET_OPTS_TMPLT) for Options Templates.
     */
    struct fds_ipfix_set_hdr header;

    /**
     * The first of withdrawal records in this Withdrawal Set.
     * Only template withdrawals corresponding to the type of the Set header can be here i.e. no
     * combinations of withdrawals of Data and Options Templates.
     */
    struct fds_ipfix_wdrl_trec first_record;
};

/** Size of a All (Options) Template Withdrawal set */
#define FDS_IPFIX_WDRL_ALLSET_LEN 8

/**
 * \struct fds_ipfix_dset
 * \brief IPFIX Data records Set structure
 *
 * The Data Records are sent in Data Sets. It consists only of one or more Field Values. The
 * Template ID to which the Field Values belong is encoded in the Set Header field "Set ID", i.e.,
 * "Set ID" = "Template ID".
 */
struct __attribute__((__packed__)) fds_ipfix_dset {
    /**
     * Common IPFIX Set header.
     * Identification of the flowset MUST be at least 256 (::FDS_IPFIX_SET_MIN_DSET) and at
     * most 65535.
     */
    struct fds_ipfix_set_hdr header;

    /** Start of the first Data record */
    uint8_t records[1];
};

 /** Size of basicList header without Enterprise number            */
#define FDS_IPFIX_BLIST_SHORT_HDR_LEN 5U

/** Size of basicList header when the Enterprise number is present */
#define FDS_IPFIX_BLIST_LONG_HDR_LEN 9U

/** Structured data type semantics */
enum fds_ipfix_list_semantics {
    FDS_IPFIX_LIST_NONE_OF          = 0,  /**< "noneOf" structured data type semantic         */
    FDS_IPFIX_LIST_EXACTLY_ONE_OF   = 1,  /**< "exactlyOneOf" structured data type semantic   */
    FDS_IPFIX_LIST_ONE_OR_MORE_OF   = 2,  /**< "oneOrMoreOf" structured data type semantic    */
    FDS_IPFIX_LIST_ALL_OF           = 3,  /**< "allOf" structured data type semantic          */
    FDS_IPFIX_LIST_ORDERED          = 4,  /**< "ordered" structured data type semantic        */
    FDS_IPFIX_LIST_UNDEFINED        = 255 /**< "undefined" structured data type semantic      */
};

/**
 * \struct fds_ipfix_blist
 * \brief  IPFIX basicList header structure
 *
 * The type "basicList" represents a list of zero or more instances of
 * any Information Element, primarily used for single-valued data types.
 * Minimal size of basicList header is 5 bytes (::FDS_IPFIX_BLIST_SHORT_HDR_LEN)
 * but if the Enterprise number is present in this header, the minimal size is 9 bytes
 * (::FDS_IPFIX_BLIST_LONG_HDR_LEN).
 */
struct __attribute__((__packed__)) fds_ipfix_blist {
    /**
     * The Semantic field indicates the relationship among the different
     * Information Element values within this Structured Data Information
     * Element.  Refer to IANA's "IPFIX Structured Data Types Semantics"
     * registry.
     */
    uint8_t semantic;

    /**
     * Field ID is the Information Element identifier of the Information
     * Element(s) contained in the list.
     */
    uint16_t field_id;

    /**
     * Per Section 7 of [RFC5101], the Element Length field indicates the
     * length, in octets, of each list element specified by Field ID, or
     * contains the value 0xFFFF if the length is encoded as a variable-
     * length Information Element at the start of the basicList Content.
     */
    uint16_t element_length;

    /**
     * If the Enterprise bit in Field ID (most significant bit in Field ID) is set to 1,
     * 4-byte enterprise number is present. Otherwise it MUST NOT be used and data present
     * at this location is not valid.
     */
    uint32_t enterprise_number;
};

/**
 * \brief Minimal length of the header of subTemplateList
 * \note Represents total length the list with zero Data Records i.e. only Semantic and Template ID
 *   is present.
 */
#define FDS_IPFIX_STLIST_HDR_LEN 3U
/**
 * \brief Minimal length of the header of subTemplateMultiList
 * \note Represents total length the list with zero Data Records i.e. only Semantic
 */
#define FDS_IPFIX_STMULTILIST_HDR_LEN 1U

/**
 * \struct fds_ipfix_stlist
 * \brief  Common header structure of subTemplateList and subTemplateMultiList
 */
struct __attribute__((__packed__)) fds_ipfix_stlist {
    /** Semantic of the lists */
    uint8_t semantic;
    /**
     * The Template ID field contains the ID of the Template used to encode and decode the
     * subTemplateList Content or following Data Records in case of subTemplateMultiList.
     */
    uint16_t template_id;
};

/**@}*/

#endif /* FDS_IPFIX_STRUCTS_H */
