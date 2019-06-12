/**
 * @file   src/file/Block_session.cpp
 * @brief  Session identification (source file)
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @date   May 2019
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstring>
#include <netinet/in.h>

#include "Block_session.hpp"
#include "File_exception.hpp"
#include "Io_sync.hpp"
#include "structure.h"

using namespace fds_file;

Block_session::Block_session(uint16_t sid, const struct fds_file_session *session) : m_sid(sid)
{
    // Check if the structure is valid and copy it
    check_validity(*session);
    memcpy(&m_struct, session, sizeof(m_struct));
}

Block_session::Block_session(int fd, off_t offset)
{
    // Just load all parameters from the file
    load_from_file(fd, offset);
}

uint64_t
Block_session::load_from_file(int fd, off_t offset)
{
    // Try to load the block using synchronous I/O
    struct fds_file_bsession session_data;
    const size_t session_size = sizeof session_data;
    Io_sync io_req(fd, &session_data, session_size);
    io_req.read(offset, session_size);
    if (io_req.wait() != session_size) {
        throw File_exception(FDS_ERR_INTERNAL, "Failed to load Session Block");
    }

    // Check the common header (type and size)
    if (le16toh(session_data.hdr.type) != FDS_FILE_BTYPE_SESSION) {
        throw File_exception(FDS_ERR_INTERNAL, "The Session Block type doesn't match");
    }

    // Note: it is possible that in the future the structure can be extended (i.e. it can be longer)
    uint64_t real_size = le64toh(session_data.hdr.length);
    if (session_size > real_size) {
        throw File_exception(FDS_ERR_INTERNAL, "The Session Block is not loaded properly");
    }

    // Extract parameters (be aware of endianness)
    m_sid = le16toh(session_data.session_id);
    memcpy(m_struct.ip_src, session_data.ip_src, sizeof m_struct.ip_src);
    memcpy(m_struct.ip_dst, session_data.ip_dst, sizeof m_struct.ip_dst);
    m_struct.port_src = le16toh(session_data.port_src);
    m_struct.port_dst = le16toh(session_data.port_dst);
    m_struct.proto = le16toh(session_data.proto);

    // Change the type to unknown if not supported (for the future backwards compatibility)
    if (m_struct.proto != FDS_FILE_SESSION_TCP
            && m_struct.proto != FDS_FILE_SESSION_UDP
            && m_struct.proto != FDS_FILE_SESSION_SCTP) {
        m_struct.proto = FDS_FILE_SESSION_UNKNOWN;
    }

    // Check if extracted parameters are valid
    check_validity(m_struct);
    // For the future backwards compatibility return real size of the Session Block
    return real_size;
}

uint64_t
Block_session::write_to_file(int fd, off_t offset)
{
    struct fds_file_bsession session_data;
    const size_t session_size = sizeof session_data;
    memset(&session_data, 0, session_size);

    // Fill the header (be aware of endianness)
    session_data.hdr.type = htole16(FDS_FILE_BTYPE_SESSION);
    session_data.hdr.flags = htole16(0);
    session_data.hdr.length = htole64(session_size);

    // Fill the structure (be aware of endianness)
    session_data.feature_flags = htole32(0);
    session_data.session_id = htole16(m_sid);
    session_data.proto = htole16(m_struct.proto);
    memcpy(session_data.ip_src, m_struct.ip_src, sizeof(session_data.ip_src));
    memcpy(session_data.ip_dst, m_struct.ip_dst, sizeof(session_data.ip_dst));
    session_data.port_src = htole16(m_struct.port_src);
    session_data.port_dst = htole16(m_struct.port_dst);

    // Write the block to the file
    Io_sync io_req(fd, &session_data, session_size);
    io_req.write(offset, session_size);
    if (io_req.wait() != session_size) {
        throw File_exception(FDS_ERR_INTERNAL, "Synchronous writer() failed to write a Session block");
    }

    return session_size;
}

void
Block_session::check_validity(const struct fds_file_session &session)
{
    // Check if the protocol type is known
    if (session.proto != FDS_FILE_SESSION_UNKNOWN
            &&session.proto != FDS_FILE_SESSION_TCP
            && session.proto != FDS_FILE_SESSION_UDP
            && session.proto != FDS_FILE_SESSION_SCTP) {
        throw File_exception(FDS_ERR_FORMAT, "Unknown type of Transport protocol");
    }
}

bool
fds_file::operator<(const Block_session &lhs, const Block_session &rhs)
{
    const struct fds_file_session &lhs_str = lhs.get_struct();
    const struct fds_file_session &rhs_str = rhs.get_struct();

    // First, compare it by ports
    if (lhs_str.port_src != rhs_str.port_src) {
        return (lhs_str.port_src < rhs_str.port_src);
    }

    if (lhs_str.port_dst != rhs_str.port_dst) {
        return (lhs_str.port_dst < rhs_str.port_dst);
    }

    // Second, compare it by protocol
    if (lhs_str.proto != rhs_str.proto) {
        return (lhs_str.proto < rhs_str.proto);
    }

    // Finally, compare it by IP addresses
    static_assert(sizeof lhs_str.ip_src == 16U, "Address MUST have 16 bytes");
    static_assert(sizeof lhs_str.ip_dst == 16U, "Address MUST have 16 bytes");

    int rc;
    rc = memcmp(lhs_str.ip_src, rhs_str.ip_src, sizeof lhs_str.ip_src);
    if (rc != 0) {
        return rc < 0;
    }

    rc = memcmp(lhs_str.ip_dst, rhs_str.ip_dst, sizeof lhs_str.ip_dst);
    if (rc != 0) {
        return rc < 0;
    }

    // The same objects
    return false;
}

bool
fds_file::operator==(const Block_session &lhs, const struct fds_file_session &rhs)
{
    return (memcmp(&lhs.m_struct, &rhs, sizeof rhs) == 0) ? true : false;
}

bool
fds_file::operator==(const struct fds_file_session &lhs, const Block_session &rhs)
{
    return (rhs == lhs);
}


