/**
 * \file lnffile.h
 * \brief Structures describing LNF file format
 *
 * \author Lukas Hutak <xhutak01@stud.fit.vutbr.cz>
 * \author Petr Velan  <petr.velan@cesnet.cz>
 * \date 2017
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
* This software is provided ``as is``, and any express or implied
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
*/

// TODO: jak vyresit sampling dat - pro kazdy zdroj dat? Jake metody?
// TODO: podporovat padding u jednotlivych zaznamu flow bloku
// TODO: Je treba mit podporu i pro options template (tj. zavest scope lenght)
// TODO: serazeni prvku seznamu
// TODO: endianita - urcite little, bude knihovna podporovat neco jineho

#ifndef LNFFILE_H
#define LNFFILE_H

#include <stdint.h>

/**
 * \defgroup lnfFileFormat File format structures
 * \brief General specification of header and various blocks
 *
 * A LNF file consists of a file header followed by various interleaved blocks
 * (Flow Data, Templates, Exporter Info, etc.) as shown in Figure A. All
 * supported blocks are defined below.
 * \verbatim
 *              +--------+---------+---------+-----+---------+
 *              |  File  |  Block  |  Block  | ... |  Block  |
 *              | Header |    1    |    2    |     |    N    |
 *              +--------+---------+---------+-----+---------+
 *                       Figure A. LNF file format
 * \endverbatim
 * @{
 */

/** File format identity */
#define LNF_FILE_MAGIC OxC330
/** Current version of the file format */
#define LNF_FILE_VERSION 0x01

/** Global flags */
enum LNF_FILE_HEADER_FLAGS {
	LNF_FILE_FLAG_COMPRESSION_BZ2 = 0x01,
	LNF_FILE_FLAG_COMPRESSION_LZO = 0x02,
};

/**
 * \brief File header
 * \verbatim
 *     0                   1                   2                   3
 *     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |              Magic            |           Version             |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                             Flags                             |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                       Number of blocks                        |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                    Extension table offset                     |
 *    |                             (64b)                             |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *                          Figure B. File header
 * \endverbatim
 */
struct lnf_file_header {
	/** Magic number (must be always #LNF_FILE_MAGIC)                        */
	uint16_t magic;
	/** Version of the format (must be #LNF_FILE_VERSION)                    */
	uint16_t version;

	/** Flags (see #LNF_FILE_HEADER_FLAGS)                                   */
	uint32_t flags;
	/** Total number of blocks (all types)                                   */
	uint32_t num_blocks;
	/**
	 * Offset from start of the file to an block offset table.
	 * Value 0 is used when the block is not present.
	 * (See "Block offset table" structure for more information)
	 */
	uint64_t table_offset;
} __attribute__((packed));

/**
 * \brief Block type
 *
 * The type ID value "0" is not used, for foolproof reasons.
 */
enum LNF_FILE_BLOCK {
	/** Flow source (see "Exporter information" block)                       */
	LNF_FILE_BLOCK_EXPORTER =   0x01,
	/** Flow data template (see "Template" block)                            */
	LNF_FILE_BLOCK_TMPLT =      0x02,
	/** Flow data blocks (see "Flow" block)                                  */
	LNF_FILE_BLOCK_FLOW =       0x03,
	/** Block offsets (see "Block offset table" block)                       */
	LNF_FILE_BLOCK_OFFSET_TBL = 0x04,
	/** Exporter statistics (see "Statistics" block)                         */
	LNF_FILE_BLOCK_STAT =       0x05
};

/**
 * \brief Common block header
 *
 * Every block contains a common header that defines type and length of the
 * block. In case a reader is not able to interpret a content of the block,
 * this common structure allows to skip to the next block.
 * \verbatim
 *     0                   1                   2                   3
 *     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |              Type             |            Flags              |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                             Length                            |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                                                               |
 *    |                ~ ~ ~ Content of the block ~ ~ ~               |
 *    |                                                               |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *                     Figure C. Common block header
 * \endverbatim
 */
struct lnf_file_block_header {
	/** Block type (One of #LNF_FILE_BLOCK)                                  */
	uint16_t type;
	/** Special flags (unused now)                                           */
	uint16_t flags;
	/** Total length of the block, in octets, including this header          */
	uint32_t len;
} __attribute__((packed));

// ----------------------------------------------------------------------------

#define LNF_FILE_EXPORTER_NAME_LEN (64)

/**
 * \brief Exporter information block
 *
 * Because one file can include flows from multiple exporters it quite useful
 * to be able to determine/filter flow by source.
 */
struct lnf_file_block_exporter {
	/** Common header (type == LNF_FILE_BLOCK_EXPORTER)                      */
	struct lnf_file_block_header header;

	/**
	 * Identification ID of the flow exporter
	 * Note: Value "0" is reserved data record with unknown exporter(s).
	 */
	uint32_t exporter_id;
	/** Observation Domain ID                                                */
	uint32_t odid;
	/** IP address                                                           */
	uint8_t  addr[16];

	// TODO: sampling information

	/** Name (e.g. server name / IP address as string / ...)                 */
	uint8_t  description[LNF_FILE_EXPORTER_NAME_LEN];

} __attribute__((packed));

// ----------------------------------------------------------------------------

/**
 * \brief Template field specifier
 *
 * This structure corresponds to simplified version (for easier parsing) of
 * a field specifier (of Information Element) in IPFIX format.
 * \verbatim
 *     0                   1                   2                   3
 *     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                       Enterprise number                       |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |            Field ID           |            Length             |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *                    Figure D. Template field specifier
 * \endverbatim
 */
struct lnf_file_tmplt_field {
	/** Enterprise number */
	uint32_t en;
	/** Field ID */
	uint16_t id;
	/**
	 * \brief Field length
	 *
	 * The maximum value (65535) is reserved for variable length fields i.e.
	 * length value will be carried in the data record itself. Variable length
	 * is useful for data types such as string or octet array.
	 */
	uint16_t length;
} __attribute__((packed));

/**
 * \brief Template record
 *
 * Templates are one of essential elements of the LNF file format. They allow
 * to provide flexibility of data records and skip data records that a reader
 * is not able to interpret.
 *
 * \warning
 *   Specifiers of fixed-size fields MUST appear before specifiers of
 *   variable-length fields. Therefore, interleaving of fixed-size and
 *   variable-length fields is NOT allowed.
 * \note
 *   The size of a template record can be easily calculated as:
 *   size of template header + (field count * size of field specifier)
 * \verbatim
 *
 *     0                   1                   2                   3
 *     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                           Template ID                         |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |           Field count         |          ~ reserved ~         |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                                                               |
 *    |        ~ ~ ~ One or more Template field specifiers ~ ~ ~      |
 *    |                                                               |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *                         Figure E. Template record
 *
 * \endverbatim
 */
struct lnf_file_tmplt_rec {
	/** Template ID                                                          */
	uint32_t tmplt_id;
	/** Number of Template field specifiers in this record                   */
	uint16_t field_cnt;
	/** Reserved */
	uint16_t reserved;

	/** One or more specifiers                                               */
	struct lnf_file_tmplt_field field[1];
} __attribute__((packed));

/**
 * \brief Template block
 *
 * Template block is a collection of one or more template records. Each record
 * describes content of a Data block and therefore the record MUST appear in
 * the file before any Data block that is described by the template.
 *
 * Structure of the template record is specified in "lnf_file_tmplt_rec"
 * structure.
 * \verbatim
 *    +---------------------------------------------------------------+
 *    |       Common Block header (type == LNF_FILE_BLOCK_TMPLT)      |
 *    +---------------------------------------------------------------+
 *    |                       Template Record 1                       |
 *    +---------------------------------------------------------------+
 *    |                       Template Record 2                       |
 *    +---------------------------------------------------------------+
 *    |                              ...                              |
 *    +---------------------------------------------------------------+
 *    |                       Template Record N                       |
 *    +---------------------------------------------------------------+
 *                  Figure F. Structure of a template block
 * \endverbatim
 */
struct lnf_file_block_tmplts {
	/** Common header (type == LNF_FILE_BLOCK_TMPLT)                         */
	struct lnf_file_block_header header;

	/**
	 * First template record
	 * \note This is NOT an array of template records! Only start of the first
	 *   record. Size of each template record must be determined individually.
	 */
	struct lnf_file_tmplt_rec rec[1];
} __attribute__((packed));

// ----------------------------------------------------------------------------

/**
 * \brief Flow record
 *
 * A record consists of field values put one after another in an order specified
 * by its template.
 *
 * Records are divided into 3 sections:
 *  1. Values of all fixed-length fields
 *  2. Meta-information about variable-length fields
 *  3. Data of variable-length fields
 *
 * The second section contains two 16bit numbers for each variable-length
 * field offset of the beginning of the field's data and length of the data
 * (in bytes). The offset is counted from the beginning of the record.
 *
 * The first two sections are called the "fixed-length part" of a record,
 * since their total size is always the same and all data are present on fixed
 * offsets (for a given template). The last section is called "variable-length
 * part" because its total length as well as position of individual fields may
 * be different in each record.
 *
 * \note Structure and description of the record is strongly inspired by UniRec
 *   data format(see github.com/CESNET/Nemea-Framework/tree/master/unirec)
 *
 * Example layout:
 * \verbatim
 *     0                   1                   2                   3
 *     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |          Record Length        |         Field Value 1         |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                          Field Value 2                        |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |          Field Value 3        |         Field Value 4         |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |          Field Value 5        |         Field Value 6         |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                          Field Value 7                        |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |          OFFSET Field 8       |         LENGTH Field 8        |  Static
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  part
 *    |          OFFSET Field 9       |         LENGTH Field 9        |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ - - - -
 *    |                          Field Value 8                        |
 *    +                                               +-+-+-+-+-+-+-+-+  Dynamic
 *    |                                               |               |  part
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+               +
 *    |                          Field Value 9                        |
 *    +                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                               |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *                      Figure G. Example Data record
 * \endverbatim
 */
struct lnf_file_flow_rec
{
	/** Record length                                                        */
	uint16_t length;
	/** Start of data record (structure is describe by a template)           */
	uint8_t data[1];

} __attribute__((packed));

/**
 * \brief Flow data block
 *
 * Data block consists of one or more flow records defined by a template.
 * In case of data compression, a flow block header is unchanged and ONLY
 * data of records are compressed i.e. variables \p tmplt_id and \p exporter_id
 * are NOT compressed.
 *
 * \verbatim
 *    +---------------------------------------------------------------+
 *    |       Common Block header (type == LNF_FILE_BLOCK_FLOW)       |
 *    +---------------------------------------------------------------+
 *    |                       Data block header                       |
 *    +---------------------------------------------------------------+
 *    |                         Flow Record 1                         |
 *    +---------------------------------------------------------------+
 *    |                         Flow Record 2                         |
 *    +---------------------------------------------------------------+
 *    |                              ...                              |
 *    +---------------------------------------------------------------+
 *    |                         Flow Record N                         |
 *    +---------------------------------------------------------------+
 *                       Figure H. Flow data block
 * \endverbatim
 *
 * Where the header is defined as
 * \verbatim
 *
 *     0                   1                   2                   3
 *     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                          Template ID                          |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                          Exporter ID                          |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \endverbatim
 */
struct lnf_file_block_flow {
	/** Common header (type == LNF_FILE_BLOCK_FLOW)                          */
	struct lnf_file_block_header header;

	/** Template ID                                                          */
	uint32_t tmplt_id;
	/**
	 * Exporter ID
	 * Value 0 is reserved for situation when a flow exporter is unknown.
	 */
	uint32_t exporter_id;

	/** Start of data record (can be compressed)                             */
	struct lnf_file_flow_rec data[1];
} __attribute__((packed));

// ----------------------------------------------------------------------------

/**
 * \brief Extension record
 *
 * \verbatim
 *
 *     0                   1                   2                   3
 *     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |            Type               |                               |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               +
 *    |                          Block Offset                         |
 *    +                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                               |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \endverbatim
 */
struct lnf_file_offset_rec {
	/** Block type (One of #LNF_FILE_BLOCK)                                  */
	uint16_t type;
	/** Block offset from start of the file                                  */
	uint64_t offset;
} __attribute__((packed));

/**
 * \brief Block offset table
 *
 * This block describes a table of important block positions (statistics,
 * indexes, etc). The table is NOT intended for flow data and template blocks.
 *
 * \warning
 *   Only one instance of this block can be in the file.
 *
 * The table is placed at the end of the file and it is also referenced in
 * file header (by offset from the beginning of the file).
 * \verbatim
 *
 *                        +--------------------+
 *                  +-----|       Header       |
 *                  |     +====================+
 *                  |     |        ...         |
 *                  |     +--------------------+
 *                  |     |    Exporter Info   |------------+
 *                  |     +--------------------+            |
 *                  |     |        ...         |            |
 *                  |     +--------------------+            |
 *                  |     |    Exporter Info   |---------+  |
 *                  |     +--------------------+         |  |
 *                  |     |        ...         |         |  |
 *                  |     +--------------------+         |  |
 *                  |     |      Indexes       |------+  |  |
 *                  |     +--------------------+      |  |  |
 *                  +---> |   Extension table  |------+--+--+
 *                        +--------------------+
 *           Figure I. Position of "Block offset table" in a file
 *
 *
 *
 * \endverbatim
 *
 * Format is defined as list of records:
 * \verbatim
 *    +---------------------------------------------------------------+
 *    |    Common Block header (type == LNF_FILE_BLOCK_OFFSET_TBL)    |
 *    +---------------------------------------------------------------+
 *    |                       Block Offset Record 1                   |
 *    +---------------------------------------------------------------+
 *    |                       Block Offset Record 2                   |
 *    +---------------------------------------------------------------+
 *    |                               ...                             |
 *    +---------------------------------------------------------------+
 *    |                       Block Offset Record N                   |
 *    +---------------------------------------------------------------+
 *                      Figure J. Block offset table
 * \endverbatim
 */
struct lnf_file_block_ext_pos {
	/** Common header (type == LNF_FILE_BLOCK_EXT_TABLE)                     */
	struct lnf_file_block_header header;

	/** Records of positions */
	struct lnf_file_offset_rec rec[1];
} __attribute__((packed));;

// ----------------------------------------------------------------------------

/**
 * \brief Statistic block about a flow exporter
 *
 * Represents statistics about flow record captured by the exporter.
 * For each exporter there MUST be exactly ONE record that corresponds to
 * Exporter information block.
 */
struct lnf_file_block_stat {
	/** Common header (type == LNF_FILE_BLOCK_STAT)                          */
	struct lnf_file_block_header header;

	/** Identification ID of an exporter */
	uint32_t exporter_id;

	// Statistics
	uint64_t num_flows;
	uint64_t num_bytes;
	uint64_t num_packets;

	uint64_t num_flow_tcp;
	uint64_t num_flow_udp;
	uint64_t num_flow_icmp;
	uint64_t num_flow_others;

	uint64_t num_bytes_tcp;
	uint64_t num_bytes_udp;
	uint64_t num_bytes_icmp;
	uint64_t num_bytes_others;

	uint64_t num_packets_tcp;
	uint64_t num_packets_udp;
	uint64_t num_packets_icmp;
	uint64_t num_packets_others;
} __attribute__((packed));

/**@}*/

#endif /* LNFFILE_H */
