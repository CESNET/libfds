/**
 * @file   src/file/File_base.cpp
 * @brief  Base class for file manipulation (source file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   April 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstring>
#include <cassert>
#include <endian.h>
#include <libfds.h>
#include <set>
#include <unistd.h>

#include "File_base.hpp"
#include "File_exception.hpp"
#include "Io_sync.hpp"
#include "structure.h"

using namespace fds_file;

File_base::File_base(const char *path, int oflag, mode_t mode, fds_file_alg calg)
{
    if (path == nullptr) {
        throw File_exception(FDS_ERR_ARG, "Path specification cannot be nullptr!");
    }

    // Open/create the file
    m_fd = open(path, oflag, mode);
    if (m_fd < 0) {
        File_exception::throw_errno(errno, "Failed to open the file");
    }

    // Clear statistics and prepare default file header
    memset(&m_stats, 0, sizeof m_stats);
    memset(&m_file_hdr, 0, sizeof m_file_hdr);

    m_file_hdr.magic = htole32(FDS_FILE_MAGIC);
    m_file_hdr.version = FDS_FILE_VERSION;
    m_file_hdr.comp_method = static_cast<uint8_t>(calg);
    m_file_hdr.flags = htole16(0);
    m_file_hdr.table_offset = htole64(0);
}

File_base::~File_base()
{
    close(m_fd);
}

void
File_base::not_impl_handler()
{
    throw File_exception(FDS_ERR_DENIED, "Operation is not available in the selected mode");
}

void
File_base::read_sfilter_conf(const fds_file_sid_t *sid, const uint32_t *odid)
{
    (void) sid;
    (void) odid;
    not_impl_handler();
}

void
File_base::read_rewind()
{
    not_impl_handler();
}

int
File_base::read_rec(struct fds_drec *rec, struct fds_file_read_ctx *ctx)
{
    (void) rec;
    (void) ctx;
    not_impl_handler();
}

fds_file_sid_t
File_base::session_add(const struct fds_file_session *info)
{
    (void) info;
    not_impl_handler();
}

void
File_base::select_ctx(fds_file_sid_t sid, uint32_t odid, uint32_t exp_time)
{
    (void) sid;
    (void) odid;
    (void) exp_time;
    not_impl_handler();
}

void
File_base::write_rec(uint16_t tid, const uint8_t *rec_data, uint16_t rec_size)
{
    (void) tid;
    (void) rec_data;
    (void) rec_size;
    not_impl_handler();
}

void
File_base::tmplt_add(enum fds_template_type t_type, const uint8_t *t_data, uint16_t t_size)
{
    (void) t_type;
    (void) t_data;
    (void) t_size;
    not_impl_handler();
}

void
File_base::tmplt_remove(uint16_t tid)
{
    (void) tid;
    not_impl_handler();
}

void
File_base::tmplt_get(uint16_t tid, enum fds_template_type *t_type, const uint8_t **t_data,
    uint16_t *t_size)
{
    (void) tid;
    (void) t_type;
    (void) t_data;
    (void) t_size;
    not_impl_handler();
}

void
File_base::stats_update(const uint8_t *rec_data, uint16_t rec_size,
    const struct fds_template *tmplt)
{
    assert(rec_data != nullptr && "Data Record must be defined!");
    assert(rec_size > 0 && "Size of the Data Record cannot be zero!");
    assert(tmplt != nullptr && "IPFIX (Options) Template of the Data Record must be defined!");

    const uint32_t IPFIX_PEN_IANA = 0;
    const uint32_t IPFIX_PEN_IANA_REV = 29305;
    const uint16_t IPFIX_IE_PROTO = 4;
    const uint16_t IPFIX_IE_BYTES = 1;
    const uint16_t IPFIX_IE_PKTS = 2;

    const uint8_t PROTO_UNKNOWN = 255; // Reserved
    const uint8_t PROTO_TCP = 6;
    const uint8_t PROTO_UDP = 17;
    const uint8_t PROTO_ICMP4 = 1;
    const uint8_t PROTO_ICMP6 = 58;

    if (tmplt->type == FDS_TYPE_TEMPLATE_OPTS) {
        // Data Record based on any IPFIX Options Template (i.e. not flow data)
        m_stats.recs_total++;
        m_stats.recs_opts_total++;
        return;
    }

    uint8_t proto = PROTO_UNKNOWN;
    uint64_t bytes = 0U;
    uint64_t packets = 0U;
    uint64_t converter_aux;
    bool biflow = false;

    // Extract protocol and number of bytes and packets
    struct fds_drec_field field;
    struct fds_drec drec = {const_cast<uint8_t *>(rec_data), rec_size, tmplt, nullptr};

    if (fds_drec_find(&drec, IPFIX_PEN_IANA, IPFIX_IE_PROTO, &field) != FDS_EOC
        && fds_get_uint_be(field.data, field.size, &converter_aux) == FDS_OK) {
        // Protocol successfully extracted
        proto = static_cast<uint8_t>(converter_aux); // Protocol is always 1 byte value
    }

    if (fds_drec_find(&drec, IPFIX_PEN_IANA, IPFIX_IE_BYTES, &field) != FDS_EOC
        && fds_get_uint_be(field.data, field.size, &converter_aux) == FDS_OK) {
        // Bytes
        bytes = converter_aux;
    }

    if (fds_drec_find(&drec, IPFIX_PEN_IANA, IPFIX_IE_PKTS, &field) != FDS_EOC
        && fds_get_uint_be(field.data, field.size, &converter_aux) == FDS_OK) {
        // Packets
        packets = converter_aux;
    }

    if (fds_drec_find(&drec, IPFIX_PEN_IANA_REV, IPFIX_IE_BYTES, &field) != FDS_EOC
        && fds_get_uint_be(field.data, field.size, &converter_aux) == FDS_OK) {
        // Bytes (reverse)
        bytes += converter_aux;
        biflow = true;
    }

    if (fds_drec_find(&drec, IPFIX_PEN_IANA_REV, IPFIX_IE_PKTS, &field) != FDS_EOC
        && fds_get_uint_be(field.data, field.size, &converter_aux) == FDS_OK) {
        // Packets (reverse)
        packets += converter_aux;
        biflow = true;
    }

    // Update statistics
    m_stats.recs_total++;
    m_stats.bytes_total += bytes;
    m_stats.pkts_total  += packets;

    switch (proto) {
    case PROTO_TCP:
        m_stats.recs_tcp++;
        m_stats.bytes_tcp += bytes;
        m_stats.pkts_tcp  += packets;
        break;
    case PROTO_UDP:
        m_stats.recs_udp++;
        m_stats.bytes_udp += bytes;
        m_stats.pkts_udp  += packets;
        break;
    case PROTO_ICMP4:
    case PROTO_ICMP6:
        m_stats.recs_icmp++;
        m_stats.bytes_icmp += bytes;
        m_stats.pkts_icmp  += packets;
        break;
    default:
        m_stats.recs_other++;
        m_stats.bytes_other += bytes;
        m_stats.pkts_other  += packets;
        break;
    }

    if (!biflow) {
        return;
    }

    // Biflow only
    m_stats.recs_bf_total++;
    switch (proto) {
    case PROTO_TCP:
        m_stats.recs_bf_tcp++;
        break;
    case PROTO_UDP:
        m_stats.recs_bf_udp++;
        break;
    case PROTO_ICMP4:
    case PROTO_ICMP6:
        m_stats.recs_bf_icmp++;
        break;
    default:
        m_stats.recs_bf_other++;
        break;
    }
}

void
File_base::stats_to_hdr()
{
    // Prepare the statistics in proper endianness
    m_file_hdr.stats.recs_total      = htole64(m_stats.recs_total);
    m_file_hdr.stats.recs_bf_total   = htole64(m_stats.recs_bf_total);
    m_file_hdr.stats.recs_opts_total = htole64(m_stats.recs_opts_total);
    m_file_hdr.stats.bytes_total     = htole64(m_stats.bytes_total);
    m_file_hdr.stats.pkts_total      = htole64(m_stats.pkts_total);

    m_file_hdr.stats.recs_tcp        = htole64(m_stats.recs_tcp);
    m_file_hdr.stats.recs_udp        = htole64(m_stats.recs_udp);
    m_file_hdr.stats.recs_icmp       = htole64(m_stats.recs_icmp);
    m_file_hdr.stats.recs_other      = htole64(m_stats.recs_other);
    m_file_hdr.stats.recs_bf_tcp     = htole64(m_stats.recs_bf_tcp);
    m_file_hdr.stats.recs_bf_udp     = htole64(m_stats.recs_bf_udp);
    m_file_hdr.stats.recs_bf_icmp    = htole64(m_stats.recs_bf_icmp);
    m_file_hdr.stats.recs_bf_other   = htole64(m_stats.recs_bf_other);

    m_file_hdr.stats.bytes_tcp       = htole64(m_stats.bytes_tcp);
    m_file_hdr.stats.bytes_udp       = htole64(m_stats.bytes_udp);
    m_file_hdr.stats.bytes_icmp      = htole64(m_stats.bytes_icmp);
    m_file_hdr.stats.bytes_other     = htole64(m_stats.bytes_other);

    m_file_hdr.stats.pkts_tcp        = htole64(m_stats.pkts_tcp);
    m_file_hdr.stats.pkts_udp        = htole64(m_stats.pkts_udp);
    m_file_hdr.stats.pkts_icmp       = htole64(m_stats.pkts_icmp);
    m_file_hdr.stats.pkts_other      = htole64(m_stats.pkts_other);
}

void
File_base::stats_from_hdr()
{
    // Convert to host byte order
    memset(&m_stats, 0, sizeof m_stats);
    m_stats.recs_total      = le64toh(m_file_hdr.stats.recs_total);
    m_stats.recs_bf_total   = le64toh(m_file_hdr.stats.recs_bf_total);
    m_stats.recs_opts_total = le64toh(m_file_hdr.stats.recs_opts_total);
    m_stats.bytes_total     = le64toh(m_file_hdr.stats.bytes_total);
    m_stats.pkts_total      = le64toh(m_file_hdr.stats.pkts_total);

    m_stats.recs_tcp        = le64toh(m_file_hdr.stats.recs_tcp);
    m_stats.recs_udp        = le64toh(m_file_hdr.stats.recs_udp);
    m_stats.recs_icmp       = le64toh(m_file_hdr.stats.recs_icmp);
    m_stats.recs_other      = le64toh(m_file_hdr.stats.recs_other);
    m_stats.recs_bf_tcp     = le64toh(m_file_hdr.stats.recs_bf_tcp);
    m_stats.recs_bf_udp     = le64toh(m_file_hdr.stats.recs_bf_udp);
    m_stats.recs_bf_icmp    = le64toh(m_file_hdr.stats.recs_bf_icmp);
    m_stats.recs_bf_other   = le64toh(m_file_hdr.stats.recs_bf_other);

    m_stats.bytes_tcp       = le64toh(m_file_hdr.stats.bytes_tcp);
    m_stats.bytes_udp       = le64toh(m_file_hdr.stats.bytes_udp);
    m_stats.bytes_icmp      = le64toh(m_file_hdr.stats.bytes_icmp);
    m_stats.bytes_other     = le64toh(m_file_hdr.stats.bytes_other);

    m_stats.pkts_tcp        = le64toh(m_file_hdr.stats.pkts_tcp);
    m_stats.pkts_udp        = le64toh(m_file_hdr.stats.pkts_udp);
    m_stats.pkts_icmp       = le64toh(m_file_hdr.stats.pkts_icmp);
    m_stats.pkts_other      = le64toh(m_file_hdr.stats.pkts_other);
}

void
File_base::file_hdr_store()
{
    // Convert current statistics to the header
    stats_to_hdr();

    // Store the header to the file
    const size_t file_hdr_size = sizeof m_file_hdr;
    Io_sync io_req(m_fd, &m_file_hdr, file_hdr_size);
    io_req.write(0, file_hdr_size);
    if (io_req.wait() != file_hdr_size) {
        throw File_exception(FDS_ERR_INTERNAL, "Failed to write the file header");
    }
}

void
File_base::file_hdr_load()
{
    // Try to load the file header
    struct fds_file_hdr file_hdr;
    const size_t file_hdr_size = sizeof file_hdr;
    Io_sync req(m_fd, &file_hdr, file_hdr_size);
    req.read(0, file_hdr_size);
    if (req.wait() != file_hdr_size) {
        throw File_exception(FDS_ERR_INTERNAL, "Failed to load the file header");
    }

    // Parse the header
    if (le32toh(file_hdr.magic) != FDS_FILE_MAGIC) {
        throw File_exception(FDS_ERR_INTERNAL, "File header doesn't match - it's not FDS file");
    }

    if (file_hdr.comp_method != FDS_FILE_CALG_NONE
            && file_hdr.comp_method != FDS_FILE_CALG_LZ4
            && file_hdr.comp_method != FDS_FILE_CALG_ZSTD) {
        throw File_exception(FDS_ERR_INTERNAL, "Unable to open the file due to unsupported "
            "compression algorithm");
    }

    // Everything seems to be ok, replace the internal version and extract statistics
    m_file_hdr = file_hdr;
    stats_from_hdr();
}

void
File_base::session_list_from_ctable(const Block_content &cblock, fds_file_sid_t **arr, size_t *size)
{
    const auto &sessions = cblock.get_sessions();
    size_t cnt = sessions.size();
    if (cnt == 0) {
        *arr = nullptr;
        *size = 0;
        return;
    }

    fds_file_sid_t *array = (fds_file_sid_t *) malloc(cnt * sizeof(fds_file_sid_t));
    if (!array) {
        throw std::bad_alloc();
    }

    for (size_t i = 0; i < cnt; ++i) {
        array[i] = sessions[i].session_id;
    }

    *arr = array;
    *size = cnt;
}

void
File_base::session_odids_from_ctable(const Block_content &cblock, fds_file_sid_t sid,
    uint32_t **arr, size_t *size)
{
    // Set of ODIDs of the given Transport Session
    std::set<uint32_t> odids;

    // Find each Data Block in the Content Table that belongs to the Transport Session
    for (const auto &dblock : cblock.get_data_blocks()) {
        if (dblock.session_id != sid) {
            // Skip different Transport Sessions
            continue;
        }

        // Insert the ODID
        odids.insert(dblock.odid);
    }

    const size_t cnt = odids.size();
    if (cnt == 0) {
        *arr = nullptr;
        *size = 0;
        return;
    }

    uint32_t *array = (uint32_t *) malloc(cnt * sizeof(*array));
    if (!array) {
        throw std::bad_alloc();
    }

    size_t idx = 0;
    for (uint32_t odid : odids) {
        assert(idx < cnt && "Index out of range!");
        array[idx++] = odid;
    }

    *arr = array;
    *size = cnt;
}
