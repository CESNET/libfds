/**
 * @file   src/file/structure.h
 * @brief  Internal file structure (header file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   April 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBFDS_STRUCTURE_H
#define LIBFDS_STRUCTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <endian.h>
#include <libfds/file.h>

/* Simplified file structure
 *
 *                 reference to the content table
 *             +-------------------------------------+
 *             |                                     |
 *             |                                     v
 *         +----------------------+     +-----------------+
 *         |      |       |       |     |       |         |
 *         |Header| Block | Block | ... | Block | C.Table |
 *         |      |   1   |   2   |     |   N   |         |
 *         +----------------------+     +-----------------+
 *                    ^       ^             ^        |
 *                    |       |             |        |
 *                    +------------------------------+
 *                references to all blocks of selected types
 */


/// File identifier ("FDS1") at the beginning of the file
#define FDS_FILE_MAGIC 0x31534446
/// Current file version
#define FDS_FILE_VERSION 1U

/**
 * @brief Maximum size of uncompressed content of Data Block (1MiB) [bytes]
 * @warning
 *   Size of compressed Data Block content can be hypothetically slightly bigger. It depends on
 *   selected compression algorithm and its overhead in case of compression of not compressible
 *   data.
 * @warning
 *   DO NOT CHANGE THIS VALUE! This could cause incompatibilities!
 */
#define FDS_FILE_DBLOCK_SIZE 1048576U

/**
 * @brief Selected compression/decompression method
 */
enum fds_file_alg {
    /// Compression disabled
    FDS_FILE_CALG_NONE = 0,
    /// LZ4 algorithm (fast, slightly worse compression ratio)
    FDS_FILE_CALG_LZ4,
    /// ZSTD algorithm (slightly slower, better compression ratio)
    FDS_FILE_CALG_ZSTD
};

/**
 * @brief File header structure
 */
struct __attribute__((packed)) fds_file_hdr {
    /// File identification (always ::FDS_FILE_MAGIC)
    uint32_t magic;
    /// Version of the file
    uint8_t version;
    /// Compression method (see ::fds_file_alg)
    uint8_t comp_method;
    /// Additional flags (reserved for the future)
    uint16_t flags;
    /// Offset of the table of important blocks (0 == not present)
    uint64_t table_offset;

    /// Global statistics of all flow records (in little endian!)
    struct fds_file_stats stats;
};

// Common header of each block ---------------------------------------------------------------------

/**
 * @brief Type of file block
 *
 * The type ID value "0" is not used, for foolproof reasons.
 */
enum fds_file_btype {
    /// Identification of flow session i.e. connection between an exporter and collector
    FDS_FILE_BTYPE_SESSION = 1,
    /// Block of IPFIX Templates (specific for <Session ID, ODID>)
    FDS_FILE_BTYPE_TMPLTS,
    /// Block of IPFIX Data (specific for <Session ID, ODID>)
    FDS_FILE_BTYPE_DATA,
    /// Block with the content table
    FDS_FILE_BTYPE_TABLE

    /*
     * Other possible blocks:
     * - FDS_FILE_BTYPE_INDEX     (index block)
     * - FDS_FILE_BTYPE_IE_LIST   (list of Information Elements in the file
     * - FDS_FILE_BSTATS_ODID     (list of statistics for each TS + ODID)
     */
};

/**
 * @brief Common flags of all file blocks
 */
enum fds_file_cflgs {
    /// The content of the block is compressed
    FDS_FILE_CFLGS_COMP = (1U << 0)
};

/**
 * @brief Common Block header of file blocks
 *
 * Every block contains a common header that defines type and length of the block. In case a reader
 * is not able to interpret a content of the block, this common structure allows to skip to the
 * next block.
 */
struct __attribute__((packed)) fds_file_bhdr {
    /** Block type (see ::fds_file_btype)                       */
    uint16_t type;
    /** Additional flags (see ::fds_file_cflgs)                 */
    uint16_t flags;
    /** Length of the block in octets (including this header)   */
    uint64_t length;
};

/// Size of Common Block header
#define FDS_FILE_BHDR_SIZE (sizeof(struct fds_file_bhdr))

// Session block ------------------------------------------------------------------------------------

/**
 * @brief Session identification block
 *
 * The block contains information about an Transport Session (i.e. connection between a flow
 * exporter and collector).
 * @note
 *   The block does NOT support compression.
 *
 * @note
 *   The Session identification block MUST precede Template and Data blocks that contains
 *   records from this Transport Session.
 *
 * @verbatim
 *      ~ -------------------------------------- ~
 *          |         |           |          |
 *          | Session |  Template |   Data   |
 *          |  block  |   block   |   block  |
 *          |    A    |     A     |     A    |
 *      ~ -------------------------------------- ~
 * @endverbatim
 *
 */
struct __attribute__((packed)) fds_file_bsession {
    /// Common block header (type == ::FDS_FILE_BTYPE_SESSION)
    struct fds_file_bhdr hdr;
    /// Flags (reserved for the future, for example: bitset of detected sampling methods)
    uint32_t feature_flags;
    /// Session identification (internally assigned ID)
    uint16_t session_id;

    /// Transport protocol (see ::fds_file_session_proto)
    uint16_t proto;
    /// Exporter IPv4/IPv6 address (for IPv4-Mapped IPv6 Address, see RFC 4291, 2.5.5.2.)
    uint8_t ip_src[16];
    /// Collector IPv4/IPv6 address (for IPv4-Mapped IPv6 Address, see RFC 4291, 2.5.5.2.)
    uint8_t ip_dst[16];

    /// Exporter port (0 == unknown)
    uint16_t port_src;
    /// Collector port (0 == unknown)
    uint16_t port_dst;
};

// Template block ----------------------------------------------------------------------------------

/**
 * @brief (Options) Template record
 */
struct __attribute__((packed)) fds_file_trec {
    /// Type of the template (::FDS_TYPE_TEMPLATE or ::FDS_TYPE_TEMPLATE_OPTS)
    uint16_t type;
    /// Length of the (Options) Template record (including this header) [bytes]
    uint16_t length;

    /**
     * Start of the definition of IPFIX (Options) Template
     *
     * @note The data are encoded as structure #fds_ipfix_trec or #fds_ipfix_opts_trec
     * @warning The data are encoded in network-byte order (i.e. big endian)!
     */
    uint8_t data[1];
};

/**
 * @brief Template block
 *
 * The block consists of one or more (Options) Template records.
 * @note The combination of \p session_id and \p odid represents a context for which the
 *   (Options) Templates are defined.
 * @note The block does NOT support compression.
 */
struct __attribute__((packed)) fds_file_btmplt {
    /// Common block header (type == ::FDS_FILE_BTYPE_TMPLTS)
    struct fds_file_bhdr hdr;

    /// Observation Domain ID
    uint32_t odid;
    /// Session identification
    uint16_t session_id;

    /// One of more (Options) Template records
    struct fds_file_trec recs[1];
};



// Data block --------------------------------------------------------------------------------------

/**
 * @brief Data block
 *
 * The block contains one or more IPFIX Messages from an exporter with a specific Observation
 * Domain ID. All (Options) Templates Sets have been removed from the records.
 *
 * The Data Block can contain Data Records based on different IPFIX (Options) Templates. All of
 * these Templates MUST have a unique Template ID. In other words, there MUST NOT be Data Records
 * based on different Templates with the same Template ID.
 *
 * @note The block supports compression. ONLY the \p data content is compressed!
 */
struct __attribute__((packed)) fds_file_bdata {
    /// Common block header (type == #FDS_FILE_BTYPE_DATA)
    struct fds_file_bhdr hdr;
    /// Additional flags (reserved)
    uint16_t flags;
    /// Identification of the session (i.e. exporter)
    uint16_t session_id;
    /// Observation Domain ID
    uint32_t odid;
    /// Offset of the Template block with (Options) Templates of Data Records
    uint64_t offset_tmptls;

    /**
     * Link to the first IPFIX Message
     *
     * @note The data are encoded as structure #fds_ipfix_msg_hdr
     * @warning The data are encoded in network-byte order (i.e. big endian)!
     */
    uint8_t data[1];
};

/// Size of Data Block header
#define FDS_FILE_BDATA_HDR_SIZE (offsetof(struct fds_file_bdata, data))

/**
 * @brief Get size of a Data block content
 * @param[in] block Pointer to the Data block header
 * @return Size of the (un)compressed content
 */
static inline size_t
fds_file_bdata_csize(struct fds_file_bdata *block)
{
    return (le32toh((block)->hdr.length) - FDS_FILE_BDATA_HDR_SIZE);
}

// Content table block -----------------------------------------------------------------------------

/// Identification of blocks present in the Table Block
enum fds_file_ctable_blocks {
    /// List of Transport Session blocks
    FDS_FILE_CTB_SESSION =  (1U << 0),
    /// List of all Data blocks
    FDS_FILE_CTB_DATA = (1U << 1)
};

/// Auxiliary Content table record of a Transport Session block
struct __attribute__((packed)) fds_file_ctable_session_rec {
    /// Offset of the block from the start of the file
    uint64_t offset;
    /// Length of the block
    uint64_t length;
    /// Internal Transport Session identification
    uint16_t session_id;
    /// Additional flags (reserved for the future use)
    uint16_t flags;
};

/// Position of all Transport Session blocks in the file (#FDS_FILE_CTB_SESSION)
struct __attribute__((packed)) fds_file_ctable_session {
    /// Total number or records
    uint16_t rec_cnt;
    /// Records
    struct fds_file_ctable_session_rec recs[1];
};

/// Auxiliary Content table record of a Data block
struct __attribute__((packed)) fds_file_ctable_data_rec {
    /// Offset of the block from the start of the file
    uint64_t offset;
    /// Length of the block
    uint64_t length;
    /// Offset of the Template block used to interpret Data Records
    uint64_t offset_tmptls;
    /// Observation Domain ID
    uint32_t odid;
    /// Session ID
    uint16_t session_id;
    /// Additional flags (reserved for the future use)
    uint16_t flags;
};

/// Position of all Data blocks in the file (#FDS_FILE_CTB_DATA)
struct __attribute__((packed)) fds_file_ctable_data {
    /// Total number of records
    uint32_t rec_cnt;
    /// Records
    struct fds_file_ctable_data_rec recs[1];
};

/**
 * @brief Content Table block
 */
struct __attribute__((packed)) fds_file_bctable {
    /// Common block header (type == ::FDS_FILE_BTYPE_TABLE)
    struct fds_file_bhdr hdr;

    /**
     * Bitset of blocks present in the table (see ::fds_file_ctable_blocks)
     *
     * @note
     *   The field is implemented as a bitset which represents identification of blocks present
     *   in this table. The number of set bits matches the size of the offset array, where each
     *   relative offset is reference to one of the record type.
     */
    uint32_t block_flags;
    /**
     * Array of relative offsets from the start of this block
     * @note
     *   The offsets are sorted in ascending order according to a value of flags set
     *   in "block_flags".
     */
    uint64_t offsets[1];
};

#ifdef  __cplusplus
}
#endif

#endif // LIBFDS_STRUCTURE_H
