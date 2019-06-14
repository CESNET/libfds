/**
 * @file wr_env.hpp
 * @author Lukas Hutak (lukas.hutak@cesnet.cz)
 * @date June 2019
 * @brief
 *   Auxiliary components for writing unit test case for FDS file writing and reading.
 *
 * The file contains definition of value-parametrized class FileAPI for Google Tests,
 * IPFIX Data Record (and Template) generator, generator of Transport Session definitions, etc.
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <gtest/gtest.h>
#include <libfds.h>
#include <MsgGen.h>

#include <arpa/inet.h>
#include <algorithm>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <tuple>

// Default error message
static const char no_error_msg[] = "No error";

// Path to a file with Information Element definitions
static const char *ie_path = "data/iana.xml";
// Type of parameters passed to Test Cases
using product_type = std::tuple<uint32_t, uint32_t, bool>;

// Class of parametrized Test Cases (the parameters are writer/reader flags)
class FileAPI : public ::testing::TestWithParam<product_type> {
public:
    // Before the whole Test Suite (i.e. resources common for all Test Cases initialized only once)
    static void SetUpTestCase() {
        // Initialize and load Information Elements
        m_iemgr = fds_iemgr_create();
        if (m_iemgr == nullptr || fds_iemgr_read_file(m_iemgr, ie_path, true) != FDS_OK) {
            FAIL() << "Faield to initialize IE manager";
        }
    }

    // After the whole Test Suite
    static void TearDownTestCase() {
        // Destroy the common IE manager
        fds_iemgr_destroy(m_iemgr);
        m_iemgr = nullptr;
    }

protected:
    // Before each Test Case
    void SetUp() override  {
        // Parse file flags for the Test Case
        uint32_t flags_comp, flags_io;
        std::tie(flags_comp, flags_io, m_load_iemgr) = GetParam();
        m_flags_read = FDS_FILE_READ | flags_io; // compression flags are always ignored
        m_flags_write = FDS_FILE_WRITE | flags_io | flags_comp;

        // Generate the filename for the Test Case
        const ::testing::TestInfo* const test_info =
            ::testing::UnitTest::GetInstance()->current_test_info();
        std::string test_suite = test_info->test_case_name();
        std::string test_case  = test_info->name();
        std::replace(test_suite.begin(), test_suite.end(), '/', '-'); // replace "/" with "-"
        std::replace(test_case.begin(),  test_case.end(),  '/', '-');
        m_filename = "data/file_" + test_suite + "_" + test_case + ".fds";

        // Make sure that file doesn't exist before the test
        int rc = unlink(m_filename.c_str());
        if (rc != 0 && !(rc == -1 && errno == ENOENT)) {
            // Failed to remove the file
            FAIL() << "SetUp failed to remove file (" + m_filename + "): " + strerror(errno);
        }
    }

    // After each Test Case
    void TearDown() override {
        // Try to remove the file
        // unlink(m_filename.c_str());
    }

    // File writer flags for the specific test case
    uint32_t m_flags_write;
    // File reader flags for the specific test case
    uint32_t m_flags_read;
    // Filename for the specific test case
    std::string m_filename;
    // Load (or not) definitions of Information Elements
    bool m_load_iemgr;
    // Manager of Information Elements
    static fds_iemgr_t *m_iemgr;
};

/**
 * @brief Convert flags for opening a file in writer mode to append mode
 * @param[in] flags Writer flags
 * @return Append flags
 */
static inline uint32_t
write2append_flag(uint32_t flags)
{
    flags &= ~FDS_FILE_WRITE;
    flags |= FDS_FILE_APPEND;
    return flags;
}

// Initialize the static class member(s)
fds_iemgr_t * FileAPI::m_iemgr = nullptr;

// Auxiliary function that generates human readable string of test case parameters
std::string
product_name(::testing::TestParamInfo<product_type> product)
{
    uint32_t flags_comp, flags_io;
    bool ie_mgr;
    std::tie(flags_comp, flags_io, ie_mgr) = product.param;

    // Warning: the returned string can contain only alphanumeric characters (i.e. no whitespaces)!
    std::string str;
    switch (flags_comp) {
    case 0:
        str += "NoCompression";
        break;
    case FDS_FILE_LZ4:
        str += "LZ4";
        break;
    case FDS_FILE_ZSTD:
        str += "ZSTD";
        break;
    default:
        throw std::runtime_error("Undefined compression flags");
    }

    str += "And";

    switch (flags_io) {
    case 0:
        str += "DefaultIO";
        break;
    case FDS_FILE_NOASYNC:
        str += "SyncIOonly";
        break;
    default:
        throw std::runtime_error("Undefined I/O flag");
    }

    if (ie_mgr) {
        str += "WithIEManager";
    }

    return str;
}

// -------------------------------------------------------------------------------------------------
// AUXILIARY TRANSPORT SESSION AND  DATA RECORD GENERATORS
// -------------------------------------------------------------------------------------------------

// Transport Session structure generator
class Session {
public:
    Session(const std::string &ip_src, const std::string &ip_dst, uint16_t port_src,
        uint16_t port_dst, enum fds_file_session_proto proto)
    {
        memset(&m_session, 0, sizeof m_session);
        m_session.port_src = port_src;
        m_session.port_dst = port_dst;
        m_session.proto = proto;
        set_addr(ip_src, m_session.ip_src);
        set_addr(ip_dst, m_session.ip_dst);
    }

    const struct fds_file_session *
    get() const {return &m_session;};

    bool
    cmp(const struct fds_file_session *session) const {
        if (m_session.proto != session->proto) {
            return false;
        }
        if (m_session.port_src != session->port_src || m_session.port_dst != session->port_dst) {
            return false;
        }
        if (memcmp(m_session.ip_src, session->ip_src, sizeof m_session.ip_src) != 0) {
            return false;
        }
        if (memcmp(m_session.ip_dst, session->ip_dst, sizeof m_session.ip_dst) != 0) {
            return false;
        }
        return true;
    }

private:
    void
    set_addr(const std::string &ip, uint8_t dest[16])
    {
        int rc;
        if (ip.find(':') != std::string::npos) {
            // IPv6
            rc = inet_pton(AF_INET6, ip.c_str(), dest);
        } else {
            // IPv4
            rc = inet_pton(AF_INET6, ("::FFFF:" +ip).c_str(), dest);
        }

        if (rc != 1) {
            throw std::runtime_error("Failed to convert IP address '" + ip + "' to binary form");
        }
    }

    // Internal Transport Session structure
    struct fds_file_session m_session;
};

// Base class for Data Record generators
class DRec_base {
public:
    const uint8_t *
    tmplt_data() const {return m_tmptl_ptr.get();};
    uint16_t
    tmplt_size() const {return m_tmplt_size;};
    enum fds_template_type
    tmplt_type() const {return m_tmplt_type;};
    uint16_t
    tmptl_id() const {return m_tmplt_id;};

    const uint8_t *
    rec_data() const {return m_rec_ptr.get();};
    uint16_t
    rec_size() const {return m_rec_size;};

    bool
    cmp_template(const uint8_t *data, uint16_t size)
    {
        if (size != m_tmplt_size) {
            return false;
        }
        return (memcmp(tmplt_data(), data, tmplt_size()) == 0);
    }
    bool
    cmp_record(const uint8_t *data, uint16_t size)
    {
        if (size != m_rec_size) {
            return false;
        }
        return (memcmp(rec_data(), data, rec_size()) == 0);
    }

protected:
    void
    set_template(enum fds_template_type type, uint16_t tid, ipfix_trec &rec)
    {
        m_tmplt_id = tid;
        m_tmplt_type = type;
        m_tmplt_size = rec.size();
        m_tmptl_ptr.reset(rec.release());
    }

    void
    set_record(ipfix_drec &rec)
    {
        m_rec_size = rec.size();
        m_rec_ptr.reset(rec.release());
    }

private:
    uint16_t m_tmplt_id;
    enum fds_template_type m_tmplt_type = FDS_TYPE_TEMPLATE_UNDEF;
    std::unique_ptr<uint8_t, decltype(&free)> m_tmptl_ptr = {nullptr, &free};
    uint16_t m_tmplt_size = 0;
    std::unique_ptr<uint8_t, decltype(&free)> m_rec_ptr = {nullptr, &free};
    uint16_t m_rec_size = 0;
};

// Generator of a partly parametrizable Data Record based on the simple IPFIX Template (pattern 1)
class DRec_simple : public DRec_base {
public:
    DRec_simple(uint16_t tid, uint16_t src_p = 80, uint16_t dst_p = 48714, uint8_t proto = 17,
        uint64_t bytes = 1223, uint64_t pkts = 2)
        : DRec_base()
    {
        // Create a Template
        ipfix_trec trec(tid);
        trec.add_field(  7, 2); // sourceTransportPort
        trec.add_field(  8, 4); // sourceIPv4Address
        trec.add_field( 11, 2); // destinationTransportPort
        trec.add_field( 12, 4); // destinationIPv4Address
        trec.add_field(  4, 1); // protocolIdentifier
        trec.add_field(210, 3); // -- paddingOctets
        trec.add_field(152, 8); // flowStartMilliseconds
        trec.add_field(153, 8); // flowEndMilliseconds
        trec.add_field(  1, 8); // octetDeltaCount
        trec.add_field(  2, 8); // packetDeltaCount
        DRec_base::set_template(FDS_TYPE_TEMPLATE, tid, trec);

        // Constant parameters
        const std::string VALUE_SRC_IP4  = "127.0.0.1";
        std::string VALUE_DST_IP4  = "1.1.1.1";
        uint64_t    VALUE_TS_FST   = 1522670362000ULL;
        uint64_t    VALUE_TS_LST   = 1522670372999ULL;

        ipfix_drec drec {};
        drec.append_uint(src_p, 2);
        drec.append_ip(VALUE_SRC_IP4);
        drec.append_uint(dst_p, 2);
        drec.append_ip(VALUE_DST_IP4);
        drec.append_uint(proto, 1);
        drec.append_uint(0, 3); // Padding
        drec.append_datetime(VALUE_TS_FST, FDS_ET_DATE_TIME_MILLISECONDS);
        drec.append_datetime(VALUE_TS_LST, FDS_ET_DATE_TIME_MILLISECONDS);
        drec.append_uint(bytes, 8);
        drec.append_uint(pkts, 8);
        DRec_base::set_record(drec);
    }
};

// Generator of a partly parametrizable Data Record based on the biflow IPFIX Template (pattern 2)
class DRec_biflow : public DRec_base {
public:
    DRec_biflow(uint16_t tid, std::string app_name = "ipfixcol2", std::string ifc_name = "eth0",
        uint16_t sp = 65145, uint16_t dp = 53, uint8_t proto = 6, uint64_t bts = 87984121,
        uint64_t pkts = 251, uint64_t bts_rev = 1323548, uint64_t pkts_rev = 213)
        : DRec_base()
    {
        ipfix_trec trec {tid};
        trec.add_field(  7, 2);                    // sourceTransportPort
        trec.add_field(  8, 4);                    // sourceIPv4Address
        trec.add_field( 11, 2);                    // destinationTransportPort
        trec.add_field( 12, 4);                    // destinationIPv4Address
        trec.add_field(  4, 1);                    // protocolIdentifier
        trec.add_field(210, 3);                    // -- paddingOctets
        trec.add_field(152, 8);                    // flowStartMilliseconds
        trec.add_field(153, 8);                    // flowEndMilliseconds
        trec.add_field(152, 8, 29305);             // flowStartMilliseconds (reverse)
        trec.add_field(153, 8, 29305);             // flowEndMilliseconds   (reverse)
        trec.add_field( 96, ipfix_trec::SIZE_VAR); // applicationName
        trec.add_field( 94, ipfix_trec::SIZE_VAR); // applicationDescription
        trec.add_field(210, 5);                    // -- paddingOctets
        trec.add_field(  1, 8);                    // octetDeltaCount
        trec.add_field(  2, 8);                    // packetDeltaCount
        trec.add_field(100, 4, 10000);             // -- field with unknown definition --
        trec.add_field(  1, 8, 29305);             // octetDeltaCount (reverse)
        trec.add_field(  2, 8, 29305);             // packetDeltaCount (reverse)
        trec.add_field( 82, ipfix_trec::SIZE_VAR); // interfaceName
        trec.add_field( 82, ipfix_trec::SIZE_VAR); // interfaceName (second occurrence)
        DRec_base::set_template(FDS_TYPE_TEMPLATE, tid, trec);

        // Constant parameters
        std::string VALUE_SRC_IP4  = "127.0.0.1";
        std::string VALUE_DST_IP4  = "8.8.8.8";
        uint64_t    VALUE_TS_FST   = 226710362000ULL;
        uint64_t    VALUE_TS_LST   = 226710372999ULL;
        uint64_t    VALUE_TS_FST_R = 226710363123ULL;
        uint64_t    VALUE_TS_LST_R = 226710369000ULL;
        double      VALUE_UNKNOWN  = 3.1416f;
        std::string rev_app_name(app_name.rbegin(), app_name.rend());

        ipfix_drec drec {};
        drec.append_uint(sp, 2);
        drec.append_ip(VALUE_SRC_IP4);
        drec.append_uint(dp, 2);
        drec.append_ip(VALUE_DST_IP4);
        drec.append_uint(proto, 1);
        drec.append_uint(0, 3); // Padding
        drec.append_datetime(VALUE_TS_FST, FDS_ET_DATE_TIME_MILLISECONDS);
        drec.append_datetime(VALUE_TS_LST, FDS_ET_DATE_TIME_MILLISECONDS);
        drec.append_datetime(VALUE_TS_FST_R, FDS_ET_DATE_TIME_MILLISECONDS);
        drec.append_datetime(VALUE_TS_LST_R, FDS_ET_DATE_TIME_MILLISECONDS);
        drec.append_string(app_name);       // Adds variable head automatically (short version)
        drec.var_header(rev_app_name.length(), true); // Adds variable head manually (long version)
        drec.append_string(rev_app_name, rev_app_name.length());
        drec.append_uint(0, 5); // Padding
        drec.append_uint(bts, 8);
        drec.append_uint(pkts, 8);
        drec.append_float(VALUE_UNKNOWN, 4);
        drec.append_uint(bts_rev, 8);
        drec.append_uint(pkts_rev, 8);
        drec.var_header(0, false); // empty string (only header)
        drec.append_string(ifc_name);
        DRec_base::set_record(drec);
    }
};

// Generator of a parametrizable Data Record based on the Options IPFIX Template (pattern 3)
class DRec_opts : public DRec_base {
public:
    DRec_opts(uint16_t tid, uint32_t odid = 4, uint32_t mpid = 1554, uint64_t msg_cnt = 171141,
        uint64_t flow_cnt = 212457447U, uint64_t octet_cnt = 2245744700U)
        : DRec_base()
    {
        ipfix_trec trec {tid, 2}; // 2 scope fields
        trec.add_field(149, 4); // observationDomainID
        trec.add_field(143, 4); // meteringProcessId
        trec.add_field(41, 8);  // exportedMessageTotalCount
        trec.add_field(42, 8);  // exportedFlowRecordTotalCount
        trec.add_field(40, 8);  // exportedOctetTotalCount
        DRec_base::set_template(FDS_TYPE_TEMPLATE_OPTS, tid, trec);

        ipfix_drec drec {};
        drec.append_uint(odid, 4);
        drec.append_uint(mpid, 4);
        drec.append_uint(msg_cnt, 8);
        drec.append_uint(flow_cnt, 8);
        drec.append_uint(octet_cnt, 8);
        DRec_base::set_record(drec);
    }
};
