/**
 * \file tests/unit_tests/converters/json.cpp
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \author Pavel Yadlouski <xyadlo00@stud.fit.vutbr.cz>
 * \brief IPFIX Data Record to JSON converter
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2019 CESNET z.s.p.o.
 */

#include <libfds.h>
#include <stdexcept>

#include <gtest/gtest.h>
#include <MsgGen.h>

// Add JSON parser
#define CONFIGURU_IMPLEMENTATION 1
#include "tools/configuru.hpp"
using namespace configuru;

// Path to file with defintion of Information Elements
static const char *cfg_path = "data/iana.xml";

int
main(int argc, char **argv)
{
    // Process command line parameters and run tests
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

/// Base TestCase fixture
class Drec_base : public ::testing::Test {
protected:
    /// Before each Test case
    void SetUp() override {
        memset(&m_drec, 0, sizeof m_drec);

        // Load Information Elements
        m_iemgr.reset(fds_iemgr_create());
        ASSERT_NE(m_iemgr, nullptr);
        ASSERT_EQ(fds_iemgr_read_file(m_iemgr.get(), cfg_path, false), FDS_OK);

        // Create a Template Manager
        m_tmgr.reset(fds_tmgr_create(FDS_SESSION_FILE));
        ASSERT_NE(m_tmgr, nullptr);
        ASSERT_EQ(fds_tmgr_set_iemgr(m_tmgr.get(), m_iemgr.get()), FDS_OK);
        ASSERT_EQ(fds_tmgr_set_time(m_tmgr.get(), 0), FDS_OK);
    }

    /// After each Test case
    void TearDown() override {
        // Free the Data Record
        free(m_drec.data);
    }

    /**
     * @brief Add a Template to the Template manager
     * @param[in] trec Template record generator
     * @param[in] type Type of the Template
     * @warning The @p trec cannot be used after this call!
     */
    void
    register_template(ipfix_trec &trec, enum fds_template_type type = FDS_TYPE_TEMPLATE)
    {
        // Parse the Template
        uint16_t tmplt_size = trec.size();
        std::unique_ptr<uint8_t, decltype(&free)> tmptl_raw(trec.release(), &free);
        struct fds_template *tmplt_temp;
        ASSERT_EQ(fds_template_parse(type, tmptl_raw.get(), &tmplt_size, &tmplt_temp), FDS_OK);

        // Add the Template to the Template Manager
        std::unique_ptr<fds_template, decltype(&fds_template_destroy)> tmplt_wrap(tmplt_temp,
            &fds_template_destroy);
        ASSERT_EQ(fds_tmgr_template_add(m_tmgr.get(), tmplt_wrap.get()), FDS_OK);
        tmplt_wrap.release();
    }

    /**
     * @brief Create a IPFIX Data Record from a generator
     * @param[in] tid  Template ID based on which the Data Record is formatted
     * @param[in] drec Data Record generator
     * @warning The @p drec cannot be used after this call!
     */
    void
    drec_create(uint16_t tid, ipfix_drec &drec)
    {
        // Extract the Data Record
        uint16_t drec_size = drec.size();
        std::unique_ptr<uint8_t, decltype(&free)> drec_raw(drec.release(), &free);

        // Prepare a Template snapshot and Template
        ASSERT_EQ(fds_tmgr_snapshot_get(m_tmgr.get(), &m_drec.snap), FDS_OK);
        m_drec.tmplt = fds_tsnapshot_template_get(m_drec.snap, tid);
        ASSERT_NE(m_drec.tmplt, nullptr) << "Template ID not found";

        m_drec.data = drec_raw.release();
        m_drec.size = drec_size;
    }

    /// Manager of Information Elements
    std::unique_ptr<fds_iemgr_t, decltype(&fds_iemgr_destroy)> m_iemgr = {nullptr, &fds_iemgr_destroy};
    /// Template manager
    std::unique_ptr<fds_tmgr_t, decltype(&fds_tmgr_destroy)> m_tmgr = {nullptr, &fds_tmgr_destroy};
    /// IPFIX Data Record
    struct fds_drec m_drec;
};

// -------------------------------------------------------------------------------------------------
/// IPFIX Data Record of a simple flow
class Drec_basic : public Drec_base {
protected:
    /// Before each Test case
    void SetUp() override {
        Drec_base::SetUp();

        // Prepare an IPFIX Template
        ipfix_trec trec{256};
        trec.add_field(  8, 4);             // sourceIPv4Address
        trec.add_field( 12, 4);             // destinationIPv4Address
        trec.add_field(  7, 2);             // sourceTransportPort
        trec.add_field( 11, 2);             // destinationTransportPort
        trec.add_field(  4, 1);             // protocolIdentifier
        trec.add_field(210, 3);             // -- paddingOctets
        trec.add_field(152, 8);             // flowStartMilliseconds
        trec.add_field(153, 8);             // flowEndMilliseconds
        trec.add_field(  1, 8);             // octetDeltaCount
        trec.add_field(  2, 8);             // packetDeltaCount
        trec.add_field(100, 4, 10000);      // -- field with unknown definition --
        trec.add_field(  6, 1);             // tcpControlBits

        // Prepare an IPFIX Data Record
        ipfix_drec drec{};
        drec.append_ip(VALUE_SRC_IP4);
        drec.append_ip(VALUE_DST_IP4);
        drec.append_uint(VALUE_SRC_PORT, 2);
        drec.append_uint(VALUE_DST_PORT, 2);
        drec.append_uint(VALUE_PROTO, 1);
        drec.append_uint(0, 3); // Padding
        drec.append_datetime(VALUE_TS_FST, FDS_ET_DATE_TIME_MILLISECONDS);
        drec.append_datetime(VALUE_TS_LST, FDS_ET_DATE_TIME_MILLISECONDS);
        drec.append_uint(VALUE_BYTES, 8);
        drec.append_uint(VALUE_PKTS, 8);
        drec.append_float(VALUE_UNKNOWN, 4);
        drec.append_uint(VALUE_TCPBITS, 1);

        register_template(trec);
        drec_create(256, drec);
    }

    std::string VALUE_SRC_IP4  = "127.0.0.1";
    std::string VALUE_DST_IP4  = "8.8.8.8";
    uint16_t    VALUE_SRC_PORT = 65000;
    uint16_t    VALUE_DST_PORT = 80;
    uint8_t     VALUE_PROTO    = 6; // TCP
    uint64_t    VALUE_TS_FST   = 1522670362000ULL;
    uint64_t    VALUE_TS_LST   = 1522670372999ULL;
    uint64_t    VALUE_BYTES    = 1234567;
    uint64_t    VALUE_PKTS     = 12345;
    double      VALUE_UNKNOWN  = 3.1416f;
    uint8_t     VALUE_TCPBITS  = 0x13; // ACK, SYN, FIN
};

// Convert Data Record with default flags to user provided buffer (without reallocation support)
TEST_F(Drec_basic, defaultConverter)
{
    size_t buffer_size = 2048;
    std::unique_ptr<char[]> buffer_data(new char[buffer_size]);

    const size_t buffer_size_orig = buffer_size;
    char *buffer_ptr = buffer_data.get();
    int rc = fds_drec2json(&m_drec, 0, &buffer_ptr, &buffer_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(strlen(buffer_ptr), size_t(rc));
    EXPECT_EQ(buffer_size, buffer_size_orig);

    // Try to parse the JSON string and check values
    Config cfg;
    ASSERT_NO_THROW(cfg = parse_string(buffer_ptr, JSON, "drec2json"));
    EXPECT_EQ((std::string) cfg["iana:sourceIPv4Address"], VALUE_SRC_IP4);
    EXPECT_EQ((std::string) cfg["iana:destinationIPv4Address"], VALUE_DST_IP4);
    EXPECT_EQ((uint16_t) cfg["iana:sourceTransportPort"], VALUE_SRC_PORT);
    EXPECT_EQ((uint16_t) cfg["iana:destinationTransportPort"], VALUE_DST_PORT);
    EXPECT_EQ((uint16_t) cfg["iana:protocolIdentifier"], VALUE_PROTO);
    EXPECT_EQ((uint64_t) cfg["iana:flowStartMilliseconds"], VALUE_TS_FST);
    EXPECT_EQ((uint64_t) cfg["iana:flowEndMilliseconds"], VALUE_TS_LST);
    EXPECT_EQ((uint64_t) cfg["iana:octetDeltaCount"], VALUE_BYTES);
    EXPECT_EQ((uint64_t) cfg["iana:packetDeltaCount"], VALUE_PKTS);
    EXPECT_EQ((uint8_t)  cfg["iana:tcpControlBits"], VALUE_TCPBITS);

    // Check if the field with unknown definition of IE is present
    EXPECT_TRUE(cfg.has_key("en10000:id100"));
    // Padding field(s) should not be in the JSON
    EXPECT_FALSE(cfg.has_key("iana:paddingOctets"));
}

// Convert Data Record to JSON and make the parser to allocate buffer for us
TEST_F(Drec_basic, defaultConverterWithAlloc)
{
    char *buffer = nullptr;
    size_t buffer_size = 0;

    int rc = fds_drec2json(&m_drec, 0, &buffer, &buffer_size);
    ASSERT_GT(rc, 0);
    ASSERT_NE(buffer, nullptr);
    EXPECT_NE(buffer_size, 0U);
    EXPECT_EQ(strlen(buffer), size_t(rc));

    // Try to parse the JSON string and check values
    Config cfg;
    ASSERT_NO_THROW(cfg = parse_string(buffer, JSON, "drec2json"));
    // TODO: implement me!

    free(buffer);
}

// Try to store the JSON to too short buffer
TEST_F(Drec_basic, tooShortBuffer)
{
    constexpr size_t BSIZE = 5U; // This should be always insufficient
    char buffer_data[BSIZE];
    size_t buffer_size = BSIZE;

    char *buffer_ptr = buffer_data;
    ASSERT_EQ(fds_drec2json(&m_drec, 0, &buffer_ptr, &buffer_size), FDS_ERR_BUFFER);
    EXPECT_EQ(buffer_size, BSIZE);
}

// TODO: add more tests here...


// -------------------------------------------------------------------------------------------------
/// IPFIX Data Record of a biflow
class Drec_biflow : public Drec_base {
    /// Before each Test case
    void SetUp() override {
        Drec_base::SetUp();

        // Prepare an IPFIX Template
        ipfix_trec trec{256};
        trec.add_field(  7, 2);                    // sourceTransportPort
        trec.add_field( 27, 16);                   // sourceIPv6Address
        trec.add_field( 11, 2);                    // destinationTransportPort
        trec.add_field( 28, 16);                   // destinationIPv6Address
        trec.add_field(  4, 1);                    // protocolIdentifier
        trec.add_field(210, 3);                    // -- paddingOctets
        trec.add_field(156, 8);                    // flowStartNanoseconds
        trec.add_field(157, 8);                    // flowEndNanoseconds
        trec.add_field(156, 8, 29305);             // flowStartNanoseconds (reverse)
        trec.add_field(157, 8, 29305);             // flowEndNanoseconds   (reverse)
        trec.add_field( 96, ipfix_trec::SIZE_VAR); // applicationName
        trec.add_field( 94, ipfix_trec::SIZE_VAR); // applicationDescription
        trec.add_field(210, 5);                    // -- paddingOctets
        trec.add_field( 82, ipfix_trec::SIZE_VAR); // interfaceName
        trec.add_field( 82, ipfix_trec::SIZE_VAR); // interfaceName (second occurrence)
        trec.add_field(  1, 8);                    // octetDeltaCount
        trec.add_field(  2, 4);                    // packetDeltaCount
        trec.add_field(  1, 8, 29305);             // octetDeltaCount (reverse)
        trec.add_field(  2, 4, 29305);             // packetDeltaCount (reverse)

        // Prepare an IPFIX Data Record
        ipfix_drec drec{};
        drec.append_uint(VALUE_SRC_PORT, 2);
        drec.append_ip(VALUE_SRC_IP6);
        drec.append_uint(VALUE_DST_PORT, 2);
        drec.append_ip(VALUE_DST_IP6);
        drec.append_uint(VALUE_PROTO, 1);
        drec.append_uint(0, 3); // Padding
        drec.append_datetime(VALUE_TS_FST, FDS_ET_DATE_TIME_NANOSECONDS);
        drec.append_datetime(VALUE_TS_LST, FDS_ET_DATE_TIME_NANOSECONDS);
        drec.append_datetime(VALUE_TS_FST_R, FDS_ET_DATE_TIME_NANOSECONDS);
        drec.append_datetime(VALUE_TS_LST_R, FDS_ET_DATE_TIME_NANOSECONDS);
        drec.append_string(VALUE_APP_NAME);      // Adds variable head automatically (short version)
        drec.var_header(VALUE_APP_DSC.length(), true); // Adds variable head manually (long version)
        drec.append_string(VALUE_APP_DSC, VALUE_APP_DSC.length());
        drec.append_uint(0, 5); // Padding
        drec.var_header(VALUE_IFC1.length(), false); // empty string (only header)
        drec.append_string(VALUE_IFC2);
        drec.append_uint(VALUE_BYTES, 8);
        drec.append_uint(VALUE_PKTS, 4);
        drec.append_uint(VALUE_BYTES_R, 8);
        drec.append_uint(VALUE_PKTS_R, 4);

        register_template(trec);
        drec_create(256, drec);
    }

    std::string VALUE_SRC_IP6  = "2001:db8::2:1";
    std::string VALUE_DST_IP6  = "fe80::fea9:6fc4:2e98:cdb2";
    uint16_t    VALUE_SRC_PORT = 1234;
    uint16_t    VALUE_DST_PORT = 8754;
    uint8_t     VALUE_PROTO    = 17; // UDP
    uint64_t    VALUE_TS_FST   = 1522670362000ULL;
    uint64_t    VALUE_TS_LST   = 1522670372999ULL;
    uint64_t    VALUE_TS_FST_R = 1522670363123ULL;
    uint64_t    VALUE_TS_LST_R = 1522670369000ULL;
    std::string VALUE_APP_NAME = "firefox";
    std::string VALUE_APP_DSC  = "linux/web browser";
    uint64_t    VALUE_BYTES    = 1234567;
    uint64_t    VALUE_PKTS     = 12345;
    uint64_t    VALUE_BYTES_R  = 7654321;
    uint64_t    VALUE_PKTS_R   = 54321;
    std::string VALUE_IFC1     = ""; // empty string
    std::string VALUE_IFC2     = "enp0s31f6";
};


// Convert Data Record with default flags to user provided buffer (without reallocation support)
TEST_F(Drec_biflow, simpleParser)
{
    // TODO: implement me!

    // NOTE: "iana:interfaceName" has multiple occurrences, therefore, it MUST be converted
    //  into an array i.e. "iana:interfaceName" : ["", "enp0s31f6"]

    // EXPECT_TRUE(cfg["iana:interfaceName"].is_array());
    // auto cfg_arr = cfg["iana:interfaceName"].as_array();
    // EXPECT_EQ(cfg_arr.size(), 2U);
    // EXPECT_NE(cfg_arr.find(VALUE_IFC1), cfg_arr.end());
    // EXPECT_NE(cfg_arr.find(VALUE_IFC2), cfg_arr.end());
}

// Convert Data Record from reverse point of view
TEST_F(Drec_biflow, reverseView)
{
    // TODO: implement me!
}


