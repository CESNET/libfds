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
#include <limits>
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

    int rc = fds_drec2json(&m_drec, 0, m_iemgr.get(), &buffer_ptr, &buffer_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(strlen(buffer_ptr), size_t(rc));
    EXPECT_EQ(buffer_size, buffer_size_orig);

    // Try to parse the JSON string and check values
    Config cfg = parse_string(buffer_ptr, JSON, "drec2json");
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

    int rc = fds_drec2json(&m_drec, 0, m_iemgr.get(), &buffer, &buffer_size);
    ASSERT_GT(rc, 0);
    ASSERT_NE(buffer, nullptr);
    EXPECT_NE(buffer_size, 0U);
    EXPECT_EQ(strlen(buffer), size_t(rc));

    // Try to parse the JSON string and check values
    Config cfg;
    ASSERT_NO_THROW(cfg = parse_string(buffer, JSON, "drec2json"));

    free(buffer);
}

// Try to store the JSON to too short buffer
TEST_F(Drec_basic, tooShortBuffer)
{
    constexpr size_t BSIZE = 5U; // This should be always insufficient
    char buffer_data[BSIZE];
    size_t buffer_size = BSIZE;

    char *buffer_ptr = buffer_data;
    ASSERT_EQ(fds_drec2json(&m_drec, 0, m_iemgr.get(), &buffer_ptr, &buffer_size), FDS_ERR_BUFFER);
    EXPECT_EQ(buffer_size, BSIZE);
}

// Convert Data Record with realloca support
TEST_F(Drec_basic, allowRealloc)
{
    constexpr size_t BSIZE = 5U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    EXPECT_NE(buff_size, BSIZE);
    free(buff);
}

// Test for formating flag (FDS_CD2J_FORMAT_TCPFLAGS)
TEST_F(Drec_basic, tcpFlag)
{
    constexpr size_t BSIZE = 5U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC | FDS_CD2J_FORMAT_TCPFLAGS;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    EXPECT_NE(buff_size, BSIZE);
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_EQ((std::string)cfg["iana:tcpControlBits"], ".A..SF");

    free(buff);
}


// -------------------------------------------------------------------------------------------------
/// IPFIX Data Record of a biflow
class Drec_biflow : public Drec_base {
protected:
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
    uint64_t    VALUE_TS_LST   = 1522670373000ULL;
    uint64_t    VALUE_TS_FST_R = 1522670364000ULL;
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
    // NOTE: "iana:interfaceName" has multiple occurrences, therefore, it MUST be converted
    //  into an array i.e. "iana:interfaceName" : ["", "enp0s31f6"]
    constexpr size_t BSIZE = 2U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

   int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
   ASSERT_GT(rc, 0);
   EXPECT_EQ(size_t(rc), strlen(buff));
   Config cfg = parse_string(buff, JSON, "drec2json");
   EXPECT_TRUE(cfg["iana:interfaceName"].is_array());
   auto cfg_arr = cfg["iana:interfaceName"].as_array();
   EXPECT_EQ(cfg_arr.size(), 2U);
   EXPECT_NE(std::find(cfg_arr.begin(), cfg_arr.end(), VALUE_IFC1), cfg_arr.end());
   EXPECT_NE(std::find(cfg_arr.begin(), cfg_arr.end(), VALUE_IFC2), cfg_arr.end());
   free(buff);
}

// Convert only forward fields
TEST_F(Drec_biflow, forwardOnly)
{
    constexpr size_t BSIZE = 2U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC | FDS_CD2J_REVERSE_SKIP;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    EXPECT_NE(buff_size, BSIZE);
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_TRUE(cfg.has_key("iana:sourceTransportPort"));
    EXPECT_TRUE(cfg.has_key("iana:sourceIPv6Address"));
    EXPECT_TRUE(cfg.has_key("iana:destinationTransportPort"));
    EXPECT_TRUE(cfg.has_key("iana:destinationIPv6Address"));
    EXPECT_TRUE(cfg.has_key("iana:protocolIdentifier"));
    EXPECT_TRUE(cfg.has_key("iana:flowStartNanoseconds"));
    EXPECT_TRUE(cfg.has_key("iana:flowEndNanoseconds"));
    EXPECT_TRUE(cfg.has_key("iana:applicationName"));
    EXPECT_TRUE(cfg.has_key("iana:applicationDescription"));
    EXPECT_TRUE(cfg.has_key("iana:interfaceName"));
    EXPECT_TRUE(cfg.has_key("iana:octetDeltaCount"));
    EXPECT_TRUE(cfg.has_key("iana:packetDeltaCount"));

    EXPECT_FALSE(cfg.has_key("iana@reverse:flowStartNanoseconds@reverse"));
    EXPECT_FALSE(cfg.has_key("iana@reverse:flowEndNanoseconds@reverse"));
    EXPECT_FALSE(cfg.has_key("iana@reverse:octetDeltaCount@reverse"));
    EXPECT_FALSE(cfg.has_key("iana@reverse:packetDeltaCount@reverse"));

    EXPECT_EQ((uint64_t) cfg["iana:octetDeltaCount"], VALUE_BYTES);        // octetDeltaCount
    EXPECT_EQ((uint64_t) cfg["iana:packetDeltaCount"], VALUE_PKTS);        // packetDeltaCount
    EXPECT_EQ((uint64_t) cfg["iana:sourceTransportPort"], VALUE_SRC_PORT); // sourceTransportPort
    EXPECT_EQ( cfg["iana:sourceIPv6Address"], VALUE_SRC_IP6);              // sourceIPv6Address
    EXPECT_EQ((uint64_t) cfg["iana:destinationTransportPort"], VALUE_DST_PORT); // destinationTransportPort
    EXPECT_EQ( cfg["iana:destinationIPv6Address"], VALUE_DST_IP6);         // destinationIPv6Address
    EXPECT_EQ((uint64_t) cfg["iana:protocolIdentifier"], VALUE_PROTO);     // protocolIdentifier
    EXPECT_EQ((uint64_t) cfg["iana:flowStartNanoseconds"], VALUE_TS_FST);  // flowStartNanoseconds
    EXPECT_EQ((uint64_t) cfg["iana:flowEndNanoseconds"], VALUE_TS_LST);    // flowEndNanoseconds
    EXPECT_EQ(cfg["iana:applicationName"], VALUE_APP_NAME);                // applicationName
    EXPECT_EQ(cfg["iana:applicationDescription"], VALUE_APP_DSC);          // applicationDescription
    free(buff);
}

// Convert from reverse point of view without "forward only" fields
TEST_F(Drec_biflow, ReverseOnly)
{
    constexpr size_t BSIZE = 2U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC | FDS_CD2J_BIFLOW_REVERSE | FDS_CD2J_REVERSE_SKIP;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    EXPECT_NE(buff_size, BSIZE);
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_TRUE(cfg.has_key("iana:sourceTransportPort"));
    EXPECT_TRUE(cfg.has_key("iana:sourceIPv6Address"));
    EXPECT_TRUE(cfg.has_key("iana:destinationTransportPort"));
    EXPECT_TRUE(cfg.has_key("iana:destinationIPv6Address"));
    EXPECT_TRUE(cfg.has_key("iana:protocolIdentifier"));
    EXPECT_TRUE(cfg.has_key("iana:flowStartNanoseconds"));
    EXPECT_TRUE(cfg.has_key("iana:flowEndNanoseconds"));
    EXPECT_TRUE(cfg.has_key("iana:applicationName"));
    EXPECT_TRUE(cfg.has_key("iana:applicationDescription"));
    EXPECT_TRUE(cfg.has_key("iana:interfaceName"));
    EXPECT_TRUE(cfg.has_key("iana:octetDeltaCount"));
    EXPECT_TRUE(cfg.has_key("iana:packetDeltaCount"));

    EXPECT_FALSE(cfg.has_key("iana@reverse:flowStartNanoseconds@reverse"));
    EXPECT_FALSE(cfg.has_key("iana@reverse:flowEndNanoseconds@reverse"));
    EXPECT_FALSE(cfg.has_key("iana@reverse:octetDeltaCount@reverse"));
    EXPECT_FALSE(cfg.has_key("iana@reverse:packetDeltaCount@reverse"));

    // Source and destination fields must be swapped
    EXPECT_EQ((uint64_t) cfg["iana:octetDeltaCount"], VALUE_BYTES_R);        // octetDeltaCount
    EXPECT_EQ((uint64_t) cfg["iana:packetDeltaCount"], VALUE_PKTS_R);        // packetDeltaCount
    EXPECT_EQ((uint64_t) cfg["iana:sourceTransportPort"], VALUE_DST_PORT); // sourceTransportPort
    EXPECT_EQ( cfg["iana:sourceIPv6Address"], VALUE_DST_IP6);              // sourceIPv6Address
    EXPECT_EQ((uint64_t) cfg["iana:destinationTransportPort"], VALUE_SRC_PORT); // destinationTransportPort
    EXPECT_EQ( cfg["iana:destinationIPv6Address"], VALUE_SRC_IP6);         // destinationIPv6Address
    EXPECT_EQ((uint64_t) cfg["iana:protocolIdentifier"], VALUE_PROTO);     // protocolIdentifier
    EXPECT_EQ((uint64_t) cfg["iana:flowStartNanoseconds"], VALUE_TS_FST_R);  // flowStartNanoseconds
    EXPECT_EQ((uint64_t) cfg["iana:flowEndNanoseconds"], VALUE_TS_LST_R);    // flowEndNanoseconds
    EXPECT_EQ(cfg["iana:applicationName"], VALUE_APP_NAME);                // applicationName
    EXPECT_EQ(cfg["iana:applicationDescription"], VALUE_APP_DSC);          // applicationDescription
    free(buff);
}

// Convert Data Record with flag FDS_CD2J_NUMERIC_ID
TEST_F(Drec_biflow, numID)
{
    constexpr size_t BSIZE = 2U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC | FDS_CD2J_NUMERIC_ID;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    EXPECT_NE(buff_size, BSIZE);
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_TRUE(cfg.has_key("en0:id7"));
    EXPECT_TRUE(cfg.has_key("en0:id27"));
    EXPECT_TRUE(cfg.has_key("en0:id11"));
    EXPECT_TRUE(cfg.has_key("en0:id28"));
    EXPECT_FALSE(cfg.has_key("en0:id210"));
    EXPECT_TRUE(cfg.has_key("en0:id156"));
    EXPECT_TRUE(cfg.has_key("en0:id157"));
    EXPECT_TRUE(cfg.has_key("en29305:id156"));
    EXPECT_TRUE(cfg.has_key("en29305:id157"));
    EXPECT_TRUE(cfg.has_key("en0:id96"));
    EXPECT_TRUE(cfg.has_key("en0:id94"));
    EXPECT_TRUE(cfg.has_key("en0:id82"));
    EXPECT_TRUE(cfg.has_key("en0:id1"));
    EXPECT_TRUE(cfg.has_key("en0:id2"));
    EXPECT_TRUE(cfg.has_key("en29305:id1"));
    EXPECT_TRUE(cfg.has_key("en29305:id2"));

    EXPECT_EQ((uint64_t) cfg["en0:id1"], VALUE_BYTES);     // octetDeltaCount
    EXPECT_EQ((uint64_t) cfg["en0:id2"], VALUE_PKTS);      // packetDeltaCount
    EXPECT_EQ((uint64_t) cfg["en0:id7"], VALUE_SRC_PORT);  // sourceTransportPort
    EXPECT_EQ( cfg["en0:id27"], VALUE_SRC_IP6);            // sourceIPv6Address
    EXPECT_EQ((uint64_t) cfg["en0:id11"], VALUE_DST_PORT); // destinationTransportPort
    EXPECT_EQ( cfg["en0:id28"], VALUE_DST_IP6);            // destinationIPv6Address
    EXPECT_EQ((uint64_t) cfg["en0:id4"], VALUE_PROTO);     // protocolIdentifier
    EXPECT_EQ((uint64_t) cfg["en0:id156"], VALUE_TS_FST);  // flowStartNanoseconds
    EXPECT_EQ((uint64_t) cfg["en0:id157"], VALUE_TS_LST);  // flowEndNanoseconds
    EXPECT_EQ(cfg["en0:id96"], VALUE_APP_NAME);            // applicationName
    EXPECT_EQ(cfg["en0:id94"], VALUE_APP_DSC);             // applicationDescription
    free(buff);
}

// Convert Data Record from reverse point of view
TEST_F(Drec_biflow, reverseView)
{
    constexpr size_t BSIZE = 2U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC | FDS_CD2J_NUMERIC_ID | FDS_CD2J_BIFLOW_REVERSE;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    EXPECT_NE(buff_size, BSIZE);
    Config cfg = parse_string(buff, JSON, "drec2json");

    EXPECT_EQ((uint64_t) cfg["en0:id1"], VALUE_BYTES_R);     // octetDeltaCount (reverse)
    EXPECT_EQ((uint64_t) cfg["en0:id2"], VALUE_PKTS_R);      // packetDeltaCount (reverse)
    EXPECT_EQ((uint64_t) cfg["en0:id156"], VALUE_TS_FST_R);  // flowStartNanoseconds (reverse)
    EXPECT_EQ((uint64_t) cfg["en0:id157"], VALUE_TS_LST_R);  // flowEndNanoseconds   (reverse)
    free(buff);
}
// Testing return of error code FDS_ERR_BUFFER
TEST_F(Drec_biflow, errorBuff)
{
    // Default situation
    constexpr size_t BSIZE = 0U;
    char* def_buff = (char*) malloc(BSIZE);
    uint32_t def_flags = FDS_CD2J_ALLOW_REALLOC;
    size_t def_buff_size = BSIZE;

    int def_rc = fds_drec2json(&m_drec, def_flags, m_iemgr.get(), &def_buff, &def_buff_size);
    ASSERT_GT(def_rc, 0);
    EXPECT_EQ(size_t(def_rc), strlen(def_buff));
    EXPECT_NE(def_buff_size, BSIZE);
    free(def_buff);
    // Loop check error situations
    for (int i = 0; i < def_rc; i++){
        SCOPED_TRACE("i: " + std::to_string(i));
        char *new_buff = (char*) malloc(i);
        ASSERT_NE(new_buff, nullptr);
        uint32_t new_flags = 0;
        size_t new_buff_size = i;
        int new_rc = fds_drec2json(&m_drec, new_flags, m_iemgr.get(), &new_buff, &new_buff_size);
        EXPECT_EQ(new_rc, FDS_ERR_BUFFER);
        free(new_buff);
    }
}

// Test for time format (FDS_CD2J_TS_FORMAT_MSEC)
TEST_F(Drec_biflow, timeFormat)
{
    constexpr size_t BSIZE = 0U;
    size_t buff_size = BSIZE;
    char* buff = NULL;
    uint32_t flags = FDS_CD2J_TS_FORMAT_MSEC;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_EQ( cfg["iana:flowStartNanoseconds"], "2018-04-02T11:59:22.000Z");
    EXPECT_EQ( cfg["iana:flowEndNanoseconds"], "2018-04-02T11:59:33.000Z");

    free (buff);
}

// Test for string format of protocol (FDS_CD2J_FORMAT_PROTO)
TEST_F(Drec_biflow, protoFormat)
{
    constexpr size_t BSIZE = 2000U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_FORMAT_PROTO;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_EQ( cfg["iana:protocolIdentifier"], "UDP");

    free(buff);
}

// Test fro nonprintable characters (FDS_CD2J_NON_PRINTABLE)
TEST_F(Drec_biflow, nonPrint)
{
    constexpr size_t BSIZE = 2000U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_NON_PRINTABLE;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    Config cfg = parse_string(buff, JSON, "drec2json");

    free(buff);
}

// -------------------------------------------------------------------------------------------------
/// IPFIX Data Record for extra situations
class Drec_extra : public Drec_base {
protected:
    /// Before each Test case
    void SetUp() override {
        Drec_base::SetUp();

        // Prepare an IPFIX Template
        ipfix_trec trec{256};
        trec.add_field(84,ipfix_trec::SIZE_VAR); //samplerName1
        trec.add_field(84,ipfix_trec::SIZE_VAR); //samplerName2
        trec.add_field(84,ipfix_trec::SIZE_VAR); //samplerName3
        trec.add_field(84,ipfix_trec::SIZE_VAR); //samplerName4
        trec.add_field(83,ipfix_trec::SIZE_VAR); //interfaceDescription
        trec.add_field( 94, ipfix_trec::SIZE_VAR);// applicationDescription (string)
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
        trec.add_field(100, 8, 10000);      // -- field with unknown definition --
        trec.add_field(  6, 2);             // tcpControlBits
        trec.add_field(1001,1);             // myBool
        trec.add_field(1000,8);             // myFloat64
        trec.add_field(1003,4);             // myFloat32
        trec.add_field(1002,8);             // myInt
        trec.add_field(1004,8);             // myPInf
        trec.add_field(1005,8);             // myMInf
        trec.add_field(1006,8);             // myNan
        trec.add_field(56, 6);              // sourceMacAddress
        trec.add_field(1007, ipfix_trec::SIZE_VAR); // myOctetArray
        trec.add_field(95,10);              // applicationId

        // Prepare an IPFIX Data Record
        ipfix_drec drec{};
        drec.append_string(VALUE_SAMP_NAME1);
        drec.append_string(VALUE_SAMP_NAME2);
        drec.append_string(VALUE_SAMP_NAME3);
        drec.append_string(VALUE_SAMP_NAME4);
        drec.append_string(VALUE_INF_DES);
        drec.append_string(VALUE_APP_DES);
        drec.append_uint(VALUE_SRC_PORT, 2);
        drec.append_ip(VALUE_SRC_IP4);
        drec.append_ip(VALUE_DST_IP4);
        drec.append_uint(VALUE_DST_PORT, 2);
        drec.append_uint(VALUE_PROTO, 1);
        drec.append_uint(0, 3); // Padding
        drec.append_datetime(VALUE_TS_FST, FDS_ET_DATE_TIME_MILLISECONDS);
        drec.append_datetime(VALUE_TS_LST, FDS_ET_DATE_TIME_MILLISECONDS);
        drec.append_uint(VALUE_BYTES, 8);
        drec.append_uint(VALUE_PKTS, 8);
        drec.append_float(VALUE_UNKNOWN, 8);
        drec.append_uint(VALUE_TCPBITS, 2);
        drec.append_bool(VALUE_MY_BOOL);
        drec.append_float(VALUE_MY_FLOAT64, 8);
        drec.append_float(VALUE_MY_FLOAT32, 4);
        drec.append_int(VALUE_MY_INT,8);
        drec.append_float(VALUE_MY_PINF,8);
        drec.append_float(VALUE_MY_MINF,8);
        drec.append_float(VALUE_MY_NAN, 8);
        drec.append_mac(VALUE_SRC_MAC);
        drec.append_octets(VALUE_MY_OCTETS.c_str(), (uint16_t) 6, true);
        drec.append_octets(VALUE_APP_ID.c_str(),(uint16_t)10, false);

        register_template(trec);
        drec_create(256, drec);
    }

    std::string VALUE_SRC_IP4    = "127.0.0.1";
    std::string VALUE_SAMP_NAME1  = "\xc2\xa1\xc3\xbd";
    std::string VALUE_SAMP_NAME2  = "\xFF\xEE"; // invalid characters
    std::string VALUE_SAMP_NAME3  = "\xef\xbf\xa6"; // FULLWIDTH WON SIGN (3 bytes)
    std::string VALUE_SAMP_NAME4  = "\xf0\x90\x8e\xa0"; // OLD PERSIAN SIGN A (4 bytes)
    std::string VALUE_DST_IP4    = "8.8.8.8";
    std::string VALUE_APP_DES    = "web\\\nclose\t\"open\bdog\fcat\r\"\x23";
    std::string VALUE_INF_DES    = "\x97\x98";
    double      VALUE_MY_PINF    = std::numeric_limits<double>::infinity();
    double      VALUE_MY_MINF    = -std::numeric_limits<double>::infinity();
    double      VALUE_MY_NAN     = NAN;
    uint16_t    VALUE_SRC_PORT   = 65000;
    uint16_t    VALUE_DST_PORT   = 80;
    uint8_t     VALUE_PROTO      = 6; // TCP
    uint64_t    VALUE_TS_FST     = 1522670362000ULL;
    uint64_t    VALUE_TS_LST     = 1522670372999ULL;
    uint64_t    VALUE_BYTES      = 1234567;
    uint64_t    VALUE_PKTS       = 12345;
    double      VALUE_UNKNOWN    = 3.141233454443216f;
    uint8_t     VALUE_TCPBITS    = 0x13; // ACK, SYN, FIN
    bool        VALUE_MY_BOOL    = true;
    double      VALUE_MY_FLOAT64 = 0.1234;
    double      VALUE_MY_FLOAT32 = 0.5678;
    signed      VALUE_MY_INT     = 1006;
    std::string VALUE_SRC_MAC    = "01:12:1F:13:11:8A";
    std::string VALUE_MY_OCTETS  = "\x1E\xA3\xAB\xAD\xC0\xDE"; // 33688308793566
    uint64_t    VALUE_MY_OCTETS_NUM = 33688308793566ULL;
    std::string VALUE_APP_ID     = "\x33\x23\x24\x30\x31\x32\x34\x35\x36\x37"; // 3#$0124567

};

// Test of different data types
TEST_F(Drec_extra, testTypes)
{
    constexpr size_t BSIZE = 10U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_EQ((double)cfg["iana:myFloat64"], VALUE_MY_FLOAT64);
    EXPECT_EQ((double)cfg["iana:myFloat32"], VALUE_MY_FLOAT32);
    EXPECT_EQ(cfg["iana:myBool"], VALUE_MY_BOOL);
    EXPECT_EQ((signed)cfg["iana:myInt"], VALUE_MY_INT);
    EXPECT_EQ(cfg["iana:myOctetArray"], VALUE_MY_OCTETS_NUM); // interpreted as number
    EXPECT_EQ(cfg["iana:sourceMacAddress"], VALUE_SRC_MAC);
    free(buff);
}

// Test of non printable characters (FDS_CD2J_NON_PRINTABLE)
TEST_F(Drec_extra, nonPrintable)
{
    constexpr size_t BSIZE = 10U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_NON_PRINTABLE | FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_EQ(cfg["iana:applicationDescription"], "web\\close\"opendogcat\"#");

    free(buff);
}

// Test of escaping characters
TEST_F(Drec_extra, printableChar)
{
    constexpr size_t BSIZE = 10U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    Config cfg = parse_string(buff, JSON, "drec2json");
    // For conversion from JSON to C natation cares JSON parser
    EXPECT_EQ((std::string)cfg["iana:applicationDescription"], VALUE_APP_DES);

    free(buff);
}

// Test of NAN, +INF, -INF values
TEST_F(Drec_extra, extraValue)
{
    constexpr size_t BSIZE = 5U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    EXPECT_NE(buff_size, BSIZE);
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_TRUE(cfg["iana:myNan"].is_string());
    EXPECT_EQ(cfg["iana:myNan"],"NaN");
    EXPECT_TRUE(cfg["iana:myPInf"].is_string());
    EXPECT_EQ(cfg["iana:myPInf"],"Infinity");
    EXPECT_TRUE(cfg["iana:myMInf"].is_string());
    EXPECT_EQ(cfg["iana:myMInf"],"-Infinity");

    free(buff);
}

// Test of other ASCII characters
TEST_F(Drec_extra, otherChar)
{
    constexpr size_t BSIZE = 5U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    EXPECT_NE(buff_size, BSIZE);
    Config cfg = parse_string(buff, JSON, "drec2json");

    EXPECT_EQ(cfg["iana:interfaceDescription"], "\u0097\u0098");
    ASSERT_TRUE(cfg["iana:samplerName"].is_array());

    auto arr = cfg["iana:samplerName"].as_array();
    EXPECT_EQ(std::find(arr.begin(), arr.end(), "\u00C2\u00A1\u00C3\u00BD" ),arr.end());
    EXPECT_EQ(std::find(arr.begin(), arr.end(), "\u00EF\u00BF\u00BD\u00EF\u00BF\u00BD" ),arr.end());
    EXPECT_EQ(std::find(arr.begin(), arr.end(), "\u00f0\u0090\u008e\u00a0" ),arr.end());
    EXPECT_EQ(std::find(arr.begin(), arr.end(), "\u00ef\u00bf\u00a6" ),arr.end());

    free(buff);
}

// Test of other MAC address
TEST_F(Drec_extra, macAdr)
{
    constexpr size_t BSIZE = 5U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    EXPECT_NE(buff_size, BSIZE);
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_EQ(cfg["iana:sourceMacAddress"], VALUE_SRC_MAC);

    free(buff);
}

// Test of octet values (FDS_CD2J_OCTETS_NOINT)
TEST_F(Drec_extra, octVal)
{
    constexpr size_t BSIZE = 5U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC | FDS_CD2J_OCTETS_NOINT;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_NE(buff_size, BSIZE);
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_EQ((std::string)cfg["iana:applicationId"], "0x33232430313234353637");
    EXPECT_EQ((std::string)cfg["iana:myOctetArray"], "0x1EA3ABADC0DE");

    free(buff);
}

// Test for executing branches with memory realocation
TEST_F(Drec_extra, forLoop)
{
    constexpr size_t BSIZE = 1U;
    char* def_buff = (char*) malloc(BSIZE);
    uint32_t def_flags = FDS_CD2J_ALLOW_REALLOC;
    size_t def_buff_size = BSIZE;

    int def_rc = fds_drec2json(&m_drec, def_flags, m_iemgr.get(), &def_buff, &def_buff_size);
    ASSERT_GT(def_rc, 0);
    EXPECT_NE(def_buff_size, BSIZE);
    EXPECT_EQ(size_t(def_rc), strlen(def_buff));
    EXPECT_NE(def_buff_size, BSIZE);

    free(def_buff);
    // Loop check right situations
    for (int i = 0; i < def_rc; i++){
        char*  new_buff = (char*) malloc(i);
        uint32_t new_flags = FDS_CD2J_ALLOW_REALLOC;
        size_t new_buff_size = i;

        int new_rc = fds_drec2json(&m_drec, new_flags, m_iemgr.get(), &new_buff, &new_buff_size);
        EXPECT_GT(new_rc, 0);
        free(new_buff);
    }
}

// Testing return of error code FDS_ERR_BUFFER
TEST_F(Drec_extra, errorBuff)
{
    // Default situation
    constexpr size_t BSIZE = 0U;
    char* def_buff = (char*) malloc(BSIZE);
    uint32_t def_flags = FDS_CD2J_ALLOW_REALLOC;
    size_t def_buff_size = BSIZE;

    int def_rc = fds_drec2json(&m_drec, def_flags, m_iemgr.get(), &def_buff, &def_buff_size);
    ASSERT_GT(def_rc, 0);
    EXPECT_EQ(size_t(def_rc), strlen(def_buff));
    EXPECT_NE(def_buff_size, BSIZE);
    free(def_buff);

    // Loop check error situations
    for (int i = 0; i < def_rc; i++) {
        SCOPED_TRACE("i: " + std::to_string(i));
        char *new_buff = (char*) malloc(i);
        ASSERT_NE(new_buff, nullptr);
        uint32_t new_flags = 0;
        size_t new_buff_size = i;
        int new_rc = fds_drec2json(&m_drec, new_flags, m_iemgr.get(), &new_buff, &new_buff_size);
        EXPECT_EQ(new_rc, FDS_ERR_BUFFER);
        free(new_buff);
    }
}

// Test for formating flag of size 2 (FDS_CD2J_FORMAT_TCPFLAGS)
TEST_F(Drec_extra, flagSize2)
{
    constexpr size_t BSIZE = 5U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC | FDS_CD2J_FORMAT_TCPFLAGS;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    EXPECT_NE(buff_size, BSIZE);
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_EQ((std::string)cfg["iana:tcpControlBits"], ".A..SF");

    free(buff);
}

TEST_F(Drec_extra, numID)
{
    constexpr size_t BSIZE = 2U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC | FDS_CD2J_NUMERIC_ID;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    EXPECT_NE(buff_size, BSIZE);
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_TRUE(cfg.has_key("en0:id1"));
    EXPECT_TRUE(cfg.has_key("en0:id2"));
    EXPECT_TRUE(cfg.has_key("en0:id6"));
    EXPECT_TRUE(cfg.has_key("en0:id7"));
    EXPECT_TRUE(cfg.has_key("en0:id8"));
    EXPECT_TRUE(cfg.has_key("en0:id11"));
    EXPECT_TRUE(cfg.has_key("en0:id12"));
    EXPECT_TRUE(cfg.has_key("en0:id56"));
    EXPECT_TRUE(cfg.has_key("en0:id83"));
    EXPECT_TRUE(cfg.has_key("en0:id84"));
    EXPECT_TRUE(cfg.has_key("en0:id94"));
    EXPECT_TRUE(cfg.has_key("en0:id95"));
    EXPECT_TRUE(cfg.has_key("en10000:id100"));
    EXPECT_TRUE(cfg.has_key("en0:id152"));
    EXPECT_TRUE(cfg.has_key("en0:id153"));
    EXPECT_TRUE(cfg.has_key("en0:id1000"));
    EXPECT_TRUE(cfg.has_key("en0:id1001"));
    EXPECT_TRUE(cfg.has_key("en0:id1002"));
    EXPECT_TRUE(cfg.has_key("en0:id1003"));
    EXPECT_TRUE(cfg.has_key("en0:id1004"));
    EXPECT_TRUE(cfg.has_key("en0:id1005"));
    EXPECT_TRUE(cfg.has_key("en0:id1006"));

    free(buff);
}


// -------------------------------------------------------------------------------------------------
/// IPFIX Data Record for invalid situations
class Drec_invalid : public Drec_base {
protected:
    /// Before each Test case
    void SetUp() override {
        Drec_base::SetUp();

        // Prepare an IPFIX Template
        ipfix_trec trec{256};
        trec.add_field( 8,  0);             // sourceIPv4Address
        trec.add_field(12,  0);             // destinationIPv4Address
        trec.add_field(24,  0);             // postPacketDeltaCount
        trec.add_field(1002,0);             // myInt
        trec.add_field(1003,0);             // myFloat32
        trec.add_field(1000,0);             // myFloat64
        trec.add_field(156, 0);             // flowStartNanoseconds
        trec.add_field(  4, 0);             // protocolIdentifier
        trec.add_field(  6, 0);             // tcpControlBits
        trec.add_field(56,  0);             // sourceMacAddress
        trec.add_field( 12, 4);             // destinationIPv4Address (second occurrence)
        trec.add_field( 11, 2);             // destinationTransportPort
        trec.add_field( 82, ipfix_trec::SIZE_VAR); // interfaceName
        trec.add_field( 82, 0);             // interfaceName (second occurrence)
        trec.add_field(32000, ipfix_trec::SIZE_VAR); // undefined field (octetArray)
        trec.add_field(1001,2);             // myBool

        // Prepare an IPFIX Data Record
        ipfix_drec drec{};
        drec.append_ip(VALUE_DST_IP4);
        drec.append_uint(VALUE_DST_PORT, 2);
        drec.append_string(VALUE_IFC1);     // empty string (only header)
        drec.var_header(0);                 // zero size octetArray
        drec.append_uint(VALUE_MY_BOOL, 2); // invalid size (bool must be 1 byte)

        register_template(trec);
        drec_create(256, drec);
    }

    std::string VALUE_DST_IP4    = "8.8.8.8";
    std::string VALUE_IFC1       = "qwert";           // empty string
    uint16_t    VALUE_DST_PORT   = 80;
    bool        VALUE_MY_BOOL    = true;

};

//  Test for adding null in case of unvalid field
TEST_F(Drec_invalid, invalidField)
{
    constexpr size_t BSIZE = 2U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

   int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
   ASSERT_GT(rc, 0);
   EXPECT_EQ(size_t(rc), strlen(buff));
   Config cfg = parse_string(buff, JSON, "drec2json");
   EXPECT_TRUE(cfg["iana:sourceIPv4Address"].is_null());
   EXPECT_TRUE(cfg["iana:myBool"].is_null());
   EXPECT_TRUE(cfg["iana:postPacketDeltaCount"].is_null());
   EXPECT_TRUE(cfg["iana:myInt"].is_null());
   EXPECT_TRUE(cfg["iana:myFloat32"].is_null());
   EXPECT_TRUE(cfg["iana:myFloat64"].is_null());
   EXPECT_TRUE(cfg["iana:flowStartNanoseconds"].is_null());
   EXPECT_TRUE(cfg["iana:protocolIdentifier"].is_null());
   EXPECT_TRUE(cfg["iana:tcpControlBits"].is_null());
   EXPECT_TRUE(cfg["iana:sourceMacAddress"].is_null());
   EXPECT_TRUE(cfg["en0:id32000"].is_null());

   free(buff);
}

//  Test for adding null to multifield in case of invalid field
TEST_F(Drec_invalid, nullInMulti)
{
    constexpr size_t BSIZE = 2U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    Config cfg = parse_string(buff, JSON, "drec2json");

    ASSERT_TRUE(cfg["iana:destinationIPv4Address"].is_array());
    auto cfg_arr = cfg["iana:interfaceName"].as_array();
    EXPECT_EQ(cfg_arr.size(), 2U);
    EXPECT_EQ(std::find(cfg_arr.begin(), cfg_arr.end(), VALUE_DST_IP4), cfg_arr.end());
    EXPECT_EQ(std::find(cfg_arr.begin(), cfg_arr.end(), NULL), cfg_arr.end());

    free(buff);
}

// Test for string with size 0
TEST_F(Drec_invalid, zeroSizeStr)
{
    constexpr size_t BSIZE = 2U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

   int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
   ASSERT_GT(rc, 0);
   EXPECT_EQ(size_t(rc), strlen(buff));
   Config cfg = parse_string(buff, JSON, "drec2json");
   ASSERT_TRUE(cfg["iana:interfaceName"].is_array());
   auto cfg_arr = cfg["iana:interfaceName"].as_array();
   EXPECT_EQ(cfg_arr.size(), 2U);
   EXPECT_NE(std::find(cfg_arr.begin(), cfg_arr.end(), VALUE_IFC1), cfg_arr.end());
   EXPECT_NE(std::find(cfg_arr.begin(), cfg_arr.end(), ""), cfg_arr.end());

   free(buff);
}

// Test for octetArray with size 0 and disabled integer conversion
TEST_F(Drec_invalid, zeroSizeOctetArray)
{
    constexpr size_t BSIZE = 2U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC | FDS_CD2J_OCTETS_NOINT; // do not use int conversion!
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_TRUE(cfg["en0:id32000"].is_null());
    free(buff);
}

// Testing return of error code FDS_ERR_BUFFER
TEST_F(Drec_invalid, errorBuff)
{
    // Default situation
    constexpr size_t BSIZE = 0U;
    char* def_buff = (char*) malloc(BSIZE);
    uint32_t def_flags = FDS_CD2J_ALLOW_REALLOC;
    size_t def_buff_size = BSIZE;

    int def_rc = fds_drec2json(&m_drec, def_flags, m_iemgr.get(), &def_buff, &def_buff_size);
    ASSERT_GT(def_rc, 0);
    EXPECT_EQ(size_t(def_rc), strlen(def_buff));
    EXPECT_NE(def_buff_size, BSIZE);
    free(def_buff);

    // Loop check error situations
    for (int i = 0; i < def_rc; i++) {
        SCOPED_TRACE("i: " + std::to_string(i));
        char *new_buff = (char*) malloc(i);
        ASSERT_NE(new_buff, nullptr);
        uint32_t new_flags = 0;
        size_t new_buff_size = i;
        int new_rc = fds_drec2json(&m_drec, new_flags, m_iemgr.get(), &new_buff, &new_buff_size);
        EXPECT_EQ(new_rc, FDS_ERR_BUFFER);
        free(new_buff);
    }
}

// -------------------------------------------------------------------------------------------------
/// IPFIX Data Record with basicList
class Drec_basicLists : public Drec_base {
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
        trec.add_field(484, ipfix_trec::SIZE_VAR); // bgpSourceCommunityList (empty)
        trec.add_field(485, ipfix_trec::SIZE_VAR); // bgpDestinationCommunityList (non-empty)
        trec.add_field(291, ipfix_trec::SIZE_VAR); // basicList (of observationDomainName strings)
        trec.add_field(487, ipfix_trec::SIZE_VAR); // bgpSourceExtendedCommunityList (empty)
        trec.add_field(488, ipfix_trec::SIZE_VAR); // bgpDestinationExtendedCommunityList (empty)
        trec.add_field(490, ipfix_trec::SIZE_VAR); // bgpSourceLargeCommunityList (empty)
        trec.add_field(491, ipfix_trec::SIZE_VAR); // bgpDestinationLargeCommunityList (empty)

        // Prepare an empty basicList (i.e. bgpSourceCommunityList of bgpCommunity)
        ipfix_blist blist_empty;
        blist_empty.header_short(FDS_IPFIX_LIST_NONE_OF, 483, 4);

        // Prepare a single element basicList (i.e. bgpDestinationCommunityList of bgpCommunity)
        ipfix_field fields_one;
        fields_one.append_uint(VALUE_BGP_DST, 4);
        ipfix_blist blist_one;
        blist_one.header_short(FDS_IPFIX_LIST_ALL_OF, 483, 4);
        blist_one.append_field(fields_one);

        // Prepare a basicList of strings (i.e. basicList of observationDomainName)
        ipfix_field fields_multi;
        fields_multi.append_string(VALUE_BLIST_STR1);
        fields_multi.var_header(VALUE_BLIST_STR2.length(), false); // empty string (only header)
        fields_multi.append_string(VALUE_BLIST_STR3);
        ipfix_blist blist_multi;
        blist_multi.header_short(FDS_IPFIX_LIST_UNDEFINED, 300, FDS_IPFIX_VAR_IE_LEN);
        blist_multi.append_field(fields_multi);

        //
        ipfix_field fields_oneof;
        fields_oneof.append_uint(VALUE_MY_BOOL, 4);
        ipfix_blist blist_oneof;
        blist_oneof.header_short(FDS_IPFIX_LIST_EXACTLY_ONE_OF, 1001, 4);
        blist_oneof.append_field(fields_oneof);

        ipfix_blist blist_one_or_more;
        blist_one_or_more.header_short(FDS_IPFIX_LIST_ONE_OR_MORE_OF, 488, 4);

        ipfix_blist blist_order;
        blist_order.header_short(FDS_IPFIX_LIST_ORDERED, 490, 4);

        ipfix_field fields_octet;
        fields_octet.append_octets(VALUE_APP_ID1.c_str(),(uint16_t)10, false);
        fields_octet.append_octets(VALUE_APP_ID2.c_str(),(uint16_t)10, false);
        ipfix_blist blist_octet;
        blist_octet.header_short(FDS_IPFIX_LIST_UNDEFINED, 1110, 10);
        blist_octet.append_field(fields_octet);

        // Prepare an IPFIX Data Record
        ipfix_drec drec{};
        drec.append_ip(VALUE_SRC_IP4);
        drec.append_ip(VALUE_DST_IP4);
        drec.append_uint(VALUE_SRC_PORT, 2);
        drec.append_uint(VALUE_DST_PORT, 2);
        drec.append_uint(VALUE_PROTO, 1);
        drec.var_header(blist_empty.size()); // bgpSourceCommunityList
        drec.append_blist(blist_empty);
        drec.var_header(blist_one.size());   // bgpDestinationCommunityList
        drec.append_blist(blist_one);
        drec.var_header(blist_multi.size()); // basicList
        drec.append_blist(blist_multi);
        drec.var_header(blist_oneof.size()); // bgpSourceExtendedCommunityList
        drec.append_blist(blist_oneof);
        drec.var_header(blist_one_or_more.size()); // bgpDestinationExtendedCommunityList
        drec.append_blist(blist_one_or_more);
        drec.var_header(blist_order.size()); // bgpSourceLargeCommunityList
        drec.append_blist(blist_order);
        drec.var_header(blist_octet.size()); // bgpDestinationLargeCommunityList
        drec.append_blist(blist_octet);

        register_template(trec);
        drec_create(256, drec);
    }
    uint32_t    VALUE_BGP_DST    = 23;
    std::string VALUE_UNV_STR    = "\x33";
    std::string VALUE_BLIST_STR1 = "RandomString";
    std::string VALUE_BLIST_STR2 = "";
    std::string VALUE_BLIST_STR3 = "Another non-empty string";
    std::string VALUE_SRC_IP4    = "127.0.0.1";
    std::string VALUE_DST_IP4    = "8.8.8.8";
    std::string VALUE_APP_ID1     = "\x33\x23\x24\x30\x31\x32\x34\x35\x36\x37"; // 3#$0124567
    std::string VALUE_APP_ID2     = "\x33\x23\x24\x30\x31\x32\x34\x35\x36\x37"; // 3#$0124567
    uint16_t    VALUE_SRC_PORT   = 65000;
    uint16_t    VALUE_DST_PORT   = 80;
    uint8_t     VALUE_PROTO      = 6; // TCP
    bool        VALUE_MY_BOOL    = 21;
};

TEST_F(Drec_basicLists, simple)
{
    char *buffer = nullptr;
    size_t buffer_size = 0;

    int rc = fds_drec2json(&m_drec, 0, m_iemgr.get(), &buffer, &buffer_size);
    ASSERT_GT(rc, 0);
    ASSERT_NE(buffer, nullptr);
    EXPECT_NE(buffer_size, 0U);
    EXPECT_EQ(strlen(buffer), size_t(rc));

    free(buffer);
}

TEST_F(Drec_basicLists, rightValues)
{
    char *buffer = nullptr;
    size_t buffer_size = 0;
    uint32_t flags = 0;

    auto iemgr = m_iemgr.get();
    int rc = fds_drec2json(&m_drec, flags, iemgr, &buffer, &buffer_size);
    ASSERT_GT(rc, 0);
    ASSERT_NE(buffer, nullptr);
    EXPECT_NE(buffer_size, 0U);
    EXPECT_EQ(strlen(buffer), size_t(rc));

    Config cfg = parse_string(buffer, JSON, "drec2json");
    ASSERT_TRUE(cfg["iana:bgpSourceCommunityList"].is_object());
    ASSERT_TRUE(cfg["iana:bgpDestinationCommunityList"].is_object());
    ASSERT_TRUE(cfg["iana:basicList"].is_object());

    auto& src_obj = cfg["iana:bgpSourceCommunityList"];
    auto& dst_obj = cfg["iana:bgpDestinationCommunityList"];
    auto& basic_obj = cfg["iana:basicList"];
    auto& ext_src_obj = cfg["iana:bgpSourceExtendedCommunityList"];
    auto& octet_obj = cfg["iana:bgpDestinationLargeCommunityList"];


    EXPECT_TRUE(src_obj["data"].is_array());
    EXPECT_TRUE(dst_obj["data"].is_array());
    EXPECT_TRUE(basic_obj["data"].is_array());
    EXPECT_TRUE(octet_obj["data"].is_array());
    EXPECT_TRUE(ext_src_obj["data"].is_array());

    auto dst_data_arr = dst_obj["data"].as_array();
    auto basic_data_arr = basic_obj["data"].as_array();
    auto octet_data_arr = octet_obj["data"].as_array();
    auto ext_src_data_arr = ext_src_obj["data"].as_array();

    EXPECT_NE(std::find(dst_data_arr.begin(), dst_data_arr.end(), VALUE_BGP_DST), dst_data_arr.end());
    EXPECT_NE(std::find(basic_data_arr.begin(), basic_data_arr.end(), VALUE_BLIST_STR1), basic_data_arr.end());
    EXPECT_NE(std::find(basic_data_arr.begin(), basic_data_arr.end(), VALUE_BLIST_STR2), basic_data_arr.end());
    EXPECT_NE(std::find(basic_data_arr.begin(), basic_data_arr.end(), VALUE_BLIST_STR3), basic_data_arr.end());
    EXPECT_EQ(std::find(octet_data_arr.begin(), octet_data_arr.end(), VALUE_APP_ID1), octet_data_arr.end());
    EXPECT_EQ(std::find(octet_data_arr.begin(), octet_data_arr.end(), VALUE_APP_ID2), octet_data_arr.end());
    EXPECT_NE(std::find(ext_src_data_arr.begin(), ext_src_data_arr.end(), nullptr), ext_src_data_arr.end());

    free(buffer);
}

TEST_F(Drec_basicLists, sematic)
{
    size_t buffer_size = 2U;
    char* buffer = (char*) malloc(buffer_size);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;

    auto iemgr = m_iemgr.get();
    int rc = fds_drec2json(&m_drec, flags, iemgr, &buffer, &buffer_size);
    ASSERT_GT(rc, 0);
    ASSERT_NE(buffer, nullptr);
    EXPECT_NE(buffer_size, 0U);
    EXPECT_EQ(strlen(buffer), size_t(rc));
    Config cfg = parse_string(buffer, JSON, "drec2json");

    auto& src_obj = cfg["iana:bgpSourceCommunityList"];
    auto& dst_obj = cfg["iana:bgpDestinationCommunityList"];
    auto& basic_obj = cfg["iana:basicList"];
    auto& ext_src_obj = cfg["iana:bgpSourceExtendedCommunityList"];
    auto& ext_dst_obj = cfg["iana:bgpDestinationExtendedCommunityList"];
    auto& lrg_src_obj = cfg["iana:bgpSourceLargeCommunityList"];


    EXPECT_EQ(src_obj["semantic"], "noneOf");
    EXPECT_EQ(dst_obj["semantic"], "allOf");
    EXPECT_EQ(basic_obj["semantic"], "undefined");
    EXPECT_EQ(ext_src_obj["semantic"], "exactlyOneOf");
    EXPECT_EQ(ext_dst_obj["semantic"], "oneOrMoreOf");
    EXPECT_EQ(lrg_src_obj["semantic"], "ordered");

    free(buffer);
}

TEST_F(Drec_basicLists, allocLoop)
{
    size_t buffer_size = 2U;
    char* buffer = (char*) malloc(buffer_size);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;

    auto iemgr = m_iemgr.get();
    int rc = fds_drec2json(&m_drec, flags, iemgr, &buffer, &buffer_size);
    ASSERT_GT(rc, 0);
    ASSERT_NE(buffer, nullptr);
    EXPECT_NE(buffer_size, 0U);
    EXPECT_EQ(strlen(buffer), size_t(rc));
    Config cfg = parse_string(buffer, JSON, "drec2json");


    for (int i = 0; i < rc; i++){
        size_t new_buff_size = i;
        char*  new_buff= (char *) malloc(new_buff_size);
        uint32_t new_flags = FDS_CD2J_ALLOW_REALLOC;

        int new_rc = fds_drec2json(&m_drec, new_flags, m_iemgr.get(), &new_buff, &new_buff_size);
        ASSERT_GT(new_rc, 0);

        free(new_buff);
    }

    free(buffer);
}

// Testing return of error code FDS_ERR_BUFFER
TEST_F(Drec_basicLists, errorBuff)
{
    // Default situation
    constexpr size_t BSIZE = 0U;
    char* def_buff = (char*) malloc(BSIZE);
    uint32_t def_flags = FDS_CD2J_ALLOW_REALLOC;
    size_t def_buff_size = BSIZE;

    int def_rc = fds_drec2json(&m_drec, def_flags, m_iemgr.get(), &def_buff, &def_buff_size);
    ASSERT_GT(def_rc, 0);
    EXPECT_EQ(size_t(def_rc), strlen(def_buff));
    EXPECT_NE(def_buff_size, BSIZE);
    free(def_buff);

    // Loop check error situations
    for (int i = 0; i < def_rc; i++) {
        SCOPED_TRACE("i: " + std::to_string(i));
        char *new_buff = (char*) malloc(i);
        ASSERT_NE(new_buff, nullptr);
        uint32_t new_flags = 0;
        size_t new_buff_size = i;
        int new_rc = fds_drec2json(&m_drec, new_flags, m_iemgr.get(), &new_buff, &new_buff_size);
        EXPECT_EQ(new_rc, FDS_ERR_BUFFER);
        free(new_buff);
    }
}

// -------------------------------------------------------------------------------------------------
/// IPFIX Data Record with subTemplateList
class Drec_subTemplateList : public Drec_base {
protected:
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
        trec.add_field(292, ipfix_trec::SIZE_VAR); // subTemplateList

        // Prepare an IPFIX Template for the subTemplateList
        ipfix_trec sub_trec{257};
        sub_trec.add_field(459, ipfix_trec::SIZE_VAR); // httpRequestMethod (string)
        sub_trec.add_field(461, ipfix_trec::SIZE_VAR); // httpRequestTarget (string)
        sub_trec.add_field(1001, 1);                   // myBool

        // Prepare few Data Records based on the subTemplateList
        ipfix_drec sub_rec_v1;
        sub_rec_v1.append_string(VALUE_HTTP_METHOD1);
        sub_rec_v1.append_string(VALUE_HTTP_TARGET1);
        sub_rec_v1.append_uint(VALUE_MY_BOLL, 1);
        ipfix_drec sub_rec_v2;
        sub_rec_v2.append_string(VALUE_HTTP_METHOD2);
        sub_rec_v2.append_string(VALUE_HTTP_TARGET2);
        sub_rec_v2.append_uint(VALUE_MY_BOLL, 1);

        // Prepare a subTemplate field with "sub" Data Records
        ipfix_stlist st_list;
        st_list.subTemp_header(FDS_IPFIX_LIST_ALL_OF, 257U);
        st_list.append_data_record(sub_rec_v1);
        st_list.append_data_record(sub_rec_v2);

        // Prepare an IPFIX Data Record
        ipfix_drec drec{};
        drec.append_uint(VALUE_SRC_PORT, 2);
        drec.append_ip(VALUE_SRC_IP6);
        drec.append_uint(VALUE_DST_PORT, 2);
        drec.append_ip(VALUE_DST_IP6);
        drec.append_uint(VALUE_PROTO, 1);
        drec.var_header(st_list.size());
        drec.append_stlist(st_list);

        register_template(trec);
        register_template(sub_trec);
        drec_create(256, drec);
    }

    std::string VALUE_SRC_IP6  = "2001:db8::2:1";
    std::string VALUE_DST_IP6  = "fe80::fea9:6fc4:2e98:cdb2";
    uint16_t    VALUE_SRC_PORT = 1234;
    uint16_t    VALUE_DST_PORT = 8754;
    uint8_t     VALUE_PROTO    = 17; // UDP
    bool        VALUE_MY_BOLL  = 10;

    std::string VALUE_HTTP_METHOD1 = "GET";
    std::string VALUE_HTTP_METHOD2 = "POST";
    std::string VALUE_HTTP_TARGET1 = "/api/example/";
    std::string VALUE_HTTP_TARGET2 = "/api/article/";
};

// Just try to convert the record
TEST_F(Drec_subTemplateList, simple)
{
    size_t buffer_size = 2U;
    char* buffer = (char*) malloc(buffer_size);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;

    auto iemgr = m_iemgr.get();
    int rc = fds_drec2json(&m_drec, flags, iemgr, &buffer, &buffer_size);
    ASSERT_GT(rc, 0);
    ASSERT_NE(buffer, nullptr);
    EXPECT_NE(buffer_size, 0U);
    EXPECT_EQ(strlen(buffer), size_t(rc));

    free(buffer);
}

// Check values in the sub-records
TEST_F(Drec_subTemplateList, values)
{
    size_t buffer_size = 2U;
    char* buffer = (char*) malloc(buffer_size);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;

    auto iemgr = m_iemgr.get();
    int rc = fds_drec2json(&m_drec, flags, iemgr, &buffer, &buffer_size);
    ASSERT_GT(rc, 0);
    ASSERT_NE(buffer, nullptr);
    EXPECT_NE(buffer_size, 0U);
    EXPECT_EQ(strlen(buffer), size_t(rc));
    Config cfg = parse_string(buffer, JSON, "drec2json");

    ASSERT_TRUE(cfg["iana:subTemplateList"].is_object());
    auto& stlist_obj = cfg["iana:subTemplateList"];

    EXPECT_TRUE(stlist_obj["data"].is_array());
    auto stlist_arr = stlist_obj["data"].as_array();
    ASSERT_EQ(stlist_arr.size(),2U);

    for (int i = 0; i < 2; i++){
        ASSERT_TRUE(stlist_arr[i].is_object());
    }

    auto& obj0 = stlist_arr[0];
    EXPECT_EQ(obj0["iana:httpRequestMethod"], VALUE_HTTP_METHOD1);
    EXPECT_EQ(obj0["iana:httpRequestTarget"], VALUE_HTTP_TARGET1);

    auto& obj1 = stlist_arr[1];
    EXPECT_EQ(obj1["iana:httpRequestMethod"], VALUE_HTTP_METHOD2);
    EXPECT_EQ(obj1["iana:httpRequestTarget"], VALUE_HTTP_TARGET2);

    free(buffer);
}

// Testing return of error code FDS_ERR_BUFFER
TEST_F(Drec_subTemplateList, errorBuff)
{
    // Default situation
    constexpr size_t BSIZE = 0U;
    char* def_buff = (char*) malloc(BSIZE);
    uint32_t def_flags = FDS_CD2J_ALLOW_REALLOC;
    size_t def_buff_size = BSIZE;

    int def_rc = fds_drec2json(&m_drec, def_flags, m_iemgr.get(), &def_buff, &def_buff_size);
    ASSERT_GT(def_rc, 0);
    EXPECT_EQ(size_t(def_rc), strlen(def_buff));
    EXPECT_NE(def_buff_size, BSIZE);
    free(def_buff);

    // Loop check error situations
    for (int i = 0; i < def_rc; i++) {
        SCOPED_TRACE("i: " + std::to_string(i));
        char *new_buff = (char*) malloc(i);
        ASSERT_NE(new_buff, nullptr);
        uint32_t new_flags = 0;
        size_t new_buff_size = i;
        int new_rc = fds_drec2json(&m_drec, new_flags, m_iemgr.get(), &new_buff, &new_buff_size);
        EXPECT_EQ(new_rc, FDS_ERR_BUFFER);
        free(new_buff);
    }
}

// -------------------------------------------------------------------------------------------------
/// IPFIX Data Record with subTemplateMultiList
class Drec_subTemplateMultiList : public Drec_base{
protected:
        /// Before each Test case
        void SetUp() override {
            Drec_base::SetUp();

            // Prepare an IPFIX Template
            ipfix_trec trec{256};
            trec.add_field( 8, 4); // sourceIPv4Address
            trec.add_field( 7, 2); // sourceTransportPort
            trec.add_field(11, 2); // destinationTransportPort
            trec.add_field( 82, ipfix_trec::SIZE_VAR); // interfaceName
            trec.add_field(293, ipfix_trec::SIZE_VAR); // subTemplateMultiList

            // Prepare 1. templeta
            ipfix_trec sub_trec1{257};
            sub_trec1.add_field(1002, 8); // myInt
            sub_trec1.add_field(1004, 8); // myMInf
            // Prepare 2. templeta
            ipfix_trec sub_trec2{258};
            sub_trec2.add_field(56, 6); // sourceMacAddress
            sub_trec2.add_field(94, ipfix_trec::SIZE_VAR);// applicationDescription (string)

            // Prepare first record
            ipfix_drec sub_drec1;
            sub_drec1.append_uint(VALUE_MY_INT1, 8);
            sub_drec1.append_float(VALUE_MY_PINF,8);
            // Prepare second record
            ipfix_drec sub_drec2;
            sub_drec2.append_uint(VALUE_MY_INT2, 8);
            sub_drec2.append_float(VALUE_MY_PINF,8);
            ipfix_drec sub_drec3;
            sub_drec3.append_mac(VALUE_SRC_MAC);
            sub_drec3.append_string(VALUE_APP_DES);

            // Prepare a subTemplteMultiList field
            ipfix_stlist stm_list;
            stm_list.subTempMulti_header(FDS_IPFIX_LIST_ALL_OF);
            stm_list.subTempMulti_data_hdr(257, sub_drec1.size() + sub_drec2.size() );
            stm_list.append_data_record(sub_drec1);
            stm_list.append_data_record(sub_drec2);
            stm_list.subTempMulti_data_hdr(258, sub_drec3.size());
            stm_list.append_data_record(sub_drec3);

            ipfix_drec drec{};
            drec.append_ip(VALUE_SRC_IP4);
            drec.append_uint(VALUE_SRC_PORT, 2);
            drec.append_uint(VALUE_DST_PORT, 2);
            drec.append_string(VALUE_INF_NAME);
            drec.var_header(stm_list.size());
            drec.append_stlist(stm_list);

            register_template(trec);
            register_template(sub_trec1);
            register_template(sub_trec2);
            drec_create(256, drec);

        }

        signed      VALUE_MY_INT1     = 1006;
        signed      VALUE_MY_INT2     = 10000006;
        double      VALUE_MY_PINF    = std::numeric_limits<double>::infinity();
        std::string VALUE_SRC_MAC    = "01:12:1F:13:11:8A";
        std::string VALUE_APP_DES    = "web\\\nclose\t\"open\bdog\fcat\r\"\x23";
        std::string VALUE_SRC_IP4    = "127.0.0.1";
        std::string VALUE_INF_NAME   = "enp0s31f6";
        uint16_t    VALUE_SRC_PORT   = 1234;
        uint16_t    VALUE_DST_PORT   = 4321;

};

TEST_F(Drec_subTemplateMultiList, simple)
{
    size_t buffer_size = 2U;
    char* buffer = (char*) malloc(buffer_size);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;

    auto iemgr = m_iemgr.get();
    int rc = fds_drec2json(&m_drec, flags, iemgr, &buffer, &buffer_size);
    ASSERT_GT(rc, 0);
    ASSERT_NE(buffer, nullptr);
    EXPECT_NE(buffer_size, 0U);
    EXPECT_EQ(strlen(buffer), size_t(rc));

    free(buffer);
}

TEST_F(Drec_subTemplateMultiList, values)
{
    size_t buffer_size = 2U;
    char* buffer = (char*) malloc(buffer_size);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;

    auto iemgr = m_iemgr.get();
    int rc = fds_drec2json(&m_drec, flags, iemgr, &buffer, &buffer_size);
    ASSERT_GT(rc, 0);
    ASSERT_NE(buffer, nullptr);
    EXPECT_NE(buffer_size, 0U);
    EXPECT_EQ(strlen(buffer), size_t(rc));
    Config cfg = parse_string(buffer, JSON, "drec2json");

    EXPECT_TRUE(cfg["iana:subTemplateMultiList"].is_object());

    auto& cfg_obj = cfg["iana:subTemplateMultiList"];
    ASSERT_TRUE(cfg_obj["data"].is_array());
    auto cfg_arr = cfg_obj["data"].as_array();

    for (int i = 0; i < 2; i++){
        ASSERT_TRUE(cfg_arr[i].is_array());
    }

    auto main_arr1 = cfg_arr[0].as_array();

    auto& obj1_1 = main_arr1[0];
    EXPECT_EQ(obj1_1["iana:myInt"], VALUE_MY_INT1);
    EXPECT_EQ(obj1_1["iana:myPInf"], "Infinity");

    auto& obj1_2 = main_arr1[1];
    EXPECT_EQ(obj1_2["iana:myInt"], VALUE_MY_INT2);
    EXPECT_EQ(obj1_2["iana:myPInf"], "Infinity");

    auto main_arr2 = cfg_arr[1].as_array();
    auto& obj2_1 = main_arr2[0];
    EXPECT_EQ(obj2_1["iana:sourceMacAddress"], VALUE_SRC_MAC);
    EXPECT_EQ(obj2_1["iana:applicationDescription"], VALUE_APP_DES);

    free(buffer);
}

// Testing return of error code FDS_ERR_BUFFER
TEST_F(Drec_subTemplateMultiList, errorBuff)
{
    // Default situation
    constexpr size_t BSIZE = 0U;
    char* def_buff = (char*) malloc(BSIZE);
    uint32_t def_flags = FDS_CD2J_ALLOW_REALLOC;
    size_t def_buff_size = BSIZE;

    int def_rc = fds_drec2json(&m_drec, def_flags, m_iemgr.get(), &def_buff, &def_buff_size);
    ASSERT_GT(def_rc, 0);
    EXPECT_EQ(size_t(def_rc), strlen(def_buff));
    EXPECT_NE(def_buff_size, BSIZE);
    free(def_buff);

    // Loop check error situations
    for (int i = 0; i < def_rc; i++) {
        SCOPED_TRACE("i: " + std::to_string(i));
        char *new_buff = (char*) malloc(i);
        ASSERT_NE(new_buff, nullptr);
        uint32_t new_flags = 0;
        size_t new_buff_size = i;
        int new_rc = fds_drec2json(&m_drec, new_flags, m_iemgr.get(), &new_buff, &new_buff_size);
        EXPECT_EQ(new_rc, FDS_ERR_BUFFER);
        free(new_buff);
    }
}

// -------------------------------------------------------------------------------------------------
/// IPFIX Data Record with subTemplateMultiList
class Drec_nested_stList_in_blist : public Drec_base{
protected:
        /// Before each Test case
        void SetUp() override {
            Drec_base::SetUp();

            // Prepare an IPFIX Template
            ipfix_trec trec{256};
            trec.add_field( 8, 4); // sourceIPv4Address
            trec.add_field( 7, 2); // sourceTransportPort
            trec.add_field(11, 2); // destinationTransportPort
            trec.add_field(484, ipfix_trec::SIZE_VAR); // bgpSourceCommunityList

            // Prepare 1. template
            ipfix_trec sub_trec1{257};
            sub_trec1.add_field(82, ipfix_trec::SIZE_VAR); // interfaceName
            sub_trec1.add_field(1004, 8); // myMInf
            // Prepare 1.1 data record
            ipfix_drec rec_1_1;
            rec_1_1.append_string(VALUE_IFC_NAME1);
            rec_1_1.append_float(VALUE_MY_PINF,8);
            // Prepare 1.2 data record
            ipfix_drec rec_1_2;
            rec_1_2.append_string(VALUE_IFC_NAME2);
            rec_1_2.append_float(VALUE_MY_PINF,8);
            // Prepare 2. template
            ipfix_trec sub_trec2{258};
            sub_trec2.add_field(56, 6); // sourceMacAddress
            sub_trec2.add_field(94, ipfix_trec::SIZE_VAR);// applicationDescription (string)
            // Prepare 2.1 data record
            ipfix_drec rec_2_1;
            rec_2_1.append_mac(VALUE_SRC_MAC1);
            rec_2_1.append_string(VALUE_APP_DES1);
            // Prepare 2.2 data record
            ipfix_drec rec_2_2;
            rec_2_2.append_mac(VALUE_SRC_MAC2);
            rec_2_2.append_string(VALUE_APP_DES2);


            // Prepare 1. subTemplateList
            ipfix_stlist stlist_1;
            stlist_1.subTemp_header(FDS_IPFIX_LIST_ALL_OF, 257U);
            stlist_1.append_data_record(rec_1_1);
            stlist_1.append_data_record(rec_1_2);
            // Prepare 2. subTemplateList
            ipfix_stlist stlist_2;
            stlist_2.subTemp_header(FDS_IPFIX_LIST_ALL_OF, 258U);
            stlist_2.append_data_record(rec_2_1);
            stlist_2.append_data_record(rec_2_2);

            // Prepare basicList (i.e. bgpSourceCommunityList of bgpCommunity)
            ipfix_field stlist_field_1;
            stlist_field_1.var_header(stlist_1.size());
            stlist_field_1.append_stlist(stlist_1);
            ipfix_field stlist_field_2;
            stlist_field_2.var_header(stlist_2.size());
            stlist_field_2.append_stlist(stlist_2);

            ipfix_blist blist;
            blist.header_short(FDS_IPFIX_LIST_ALL_OF, 292, ipfix_trec::SIZE_VAR);
            blist.append_field(stlist_field_1);
            blist.append_field(stlist_field_2);

            ipfix_drec drec{};
            drec.append_ip(VALUE_SRC_IP4);
            drec.append_uint(VALUE_SRC_PORT, 2);
            drec.append_uint(VALUE_DST_PORT, 2);
            drec.var_header(blist.size());
            drec.append_blist(blist);

            register_template(trec);
            register_template(sub_trec1);
            register_template(sub_trec2);
            drec_create(256, drec);
        }

        double      VALUE_MY_PINF    = std::numeric_limits<double>::infinity();
        std::string VALUE_IFC_NAME1  = "ONE";
        std::string VALUE_IFC_NAME2  = "TWO";
        std::string VALUE_SRC_MAC1   = "01:12:1F:13:11:8A";
        std::string VALUE_APP_DES1   = "web\\\nclose\t\"open\bdog\fcat\r\"\x23";
        std::string VALUE_SRC_MAC2   = "21:01:4A:31:20:8C";
        std::string VALUE_APP_DES2   = "small\\\nbig\t\"mam\bdoor\fcat";
        std::string VALUE_SRC_IP4    = "127.0.0.1";
        std::string VALUE_INF_NAME   = "enp0s31f6";
        uint16_t    VALUE_SRC_PORT   = 1234;
        uint16_t    VALUE_DST_PORT   = 4321;

};

TEST_F(Drec_nested_stList_in_blist, simple)
{
    size_t buffer_size = 2U;
    char* buffer = (char*) malloc(buffer_size);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;

    auto iemgr = m_iemgr.get();
    int rc = fds_drec2json(&m_drec, flags, iemgr, &buffer, &buffer_size);
    ASSERT_GT(rc, 0);
    ASSERT_NE(buffer, nullptr);
    EXPECT_NE(buffer_size, 0U);
    EXPECT_EQ(strlen(buffer), size_t(rc));

    free(buffer);
}

TEST_F(Drec_nested_stList_in_blist, values)
{
    size_t buffer_size = 2U;
    char* buffer = (char*) malloc(buffer_size);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;

    auto iemgr = m_iemgr.get();
    int rc = fds_drec2json(&m_drec, flags, iemgr, &buffer, &buffer_size);
    ASSERT_GT(rc, 0);
    ASSERT_NE(buffer, nullptr);
    EXPECT_NE(buffer_size, 0U);
    EXPECT_EQ(strlen(buffer), size_t(rc));

    Config cfg = parse_string(buffer, JSON, "drec2json");
    ASSERT_TRUE(cfg["iana:bgpSourceCommunityList"].is_object());
    auto& src_obj = cfg["iana:bgpSourceCommunityList"];

    EXPECT_TRUE(src_obj["data"].is_array());
    auto src_data_arr = src_obj["data"].as_array();

    /* From blist data (1. layer) get all objects
       basicList (data) -> obj0
                 -> obj1
    */
    auto& obj0 = src_data_arr[0];
    ASSERT_TRUE(obj0.is_object());
    auto& obj1 = src_data_arr[1];
    ASSERT_TRUE(obj1.is_object());

    /* From each object from 1. layer get array with data (2. layer)
       basicList -> obj0 -> arr0 (data)
                 -> obj1 -> arr1 (data)
    */
    ASSERT_TRUE(obj0["data"].is_array());
    auto arr0 = obj0["data"].as_array();
    ASSERT_TRUE(obj1["data"].is_array());
    auto arr1 = obj1["data"].as_array();

    /* From each array from 2. layer get every obejct (3. layer)
       basicList -> obj0 -> arr0 -> obj0_0
                                 -> obj0_1
                 -> obj1 -> arr1 -> obj1_0
                                 -> obj1_1
    */
    ASSERT_TRUE(arr0[0].is_object());
    auto& obj0_0 = arr0[0];
    ASSERT_TRUE(arr0[1].is_object());
    auto& obj0_1 = arr0[1];

    ASSERT_TRUE(arr1[0].is_object());
    auto& obj1_0 = arr1[0];
    ASSERT_TRUE(arr1[1].is_object());
    auto& obj1_1 = arr1[1];

    // Check values in each object from 3. layer

    EXPECT_EQ(obj0_0["iana:interfaceName"],VALUE_IFC_NAME1);
    EXPECT_EQ(obj0_0["iana:myPInf"],"Infinity");

    EXPECT_EQ(obj0_1["iana:interfaceName"],VALUE_IFC_NAME2);
    EXPECT_EQ(obj0_1["iana:myPInf"],"Infinity");

    EXPECT_EQ(obj1_0["iana:sourceMacAddress"],VALUE_SRC_MAC1);
    EXPECT_EQ(obj1_0["iana:applicationDescription"],VALUE_APP_DES1);

    EXPECT_EQ(obj1_1["iana:sourceMacAddress"],VALUE_SRC_MAC2);
    EXPECT_EQ(obj1_1["iana:applicationDescription"],VALUE_APP_DES2);

    free(buffer);
}

// Testing return of error code FDS_ERR_BUFFER
TEST_F(Drec_nested_stList_in_blist, errorBuff)
{
    // Default situation
    constexpr size_t BSIZE = 0U;
    char* def_buff = (char*) malloc(BSIZE);
    uint32_t def_flags = FDS_CD2J_ALLOW_REALLOC;
    size_t def_buff_size = BSIZE;

    int def_rc = fds_drec2json(&m_drec, def_flags, m_iemgr.get(), &def_buff, &def_buff_size);
    ASSERT_GT(def_rc, 0);
    EXPECT_EQ(size_t(def_rc), strlen(def_buff));
    EXPECT_NE(def_buff_size, BSIZE);
    free(def_buff);

    // Loop check error situations
    for (int i = 0; i < def_rc; i++) {
        SCOPED_TRACE("i: " + std::to_string(i));
        char *new_buff = (char*) malloc(i);
        ASSERT_NE(new_buff, nullptr);
        uint32_t new_flags = 0;
        size_t new_buff_size = i;
        int new_rc = fds_drec2json(&m_drec, new_flags, m_iemgr.get(), &new_buff, &new_buff_size);
        EXPECT_EQ(new_rc, FDS_ERR_BUFFER);
        free(new_buff);
    }
}

// -------------------------------------------------------------------------------------------------
/// IPFIX Data Record with subTemplateMultiList
class Drec_nested_blist_in_stlist : public Drec_base{
protected:
    /// Before each Test case
    void SetUp() override {
        Drec_base::SetUp();
        // Prepare an IPFIX Template
        ipfix_trec trec{256};
        trec.add_field( 8, 4); // sourceIPv4Address
        trec.add_field( 7, 2); // sourceTransportPort
        trec.add_field(11, 2); // destinationTransportPort
        trec.add_field(292, ipfix_trec::SIZE_VAR); // subTemplateList

        //  Prepare IPFIX subTempate
        ipfix_trec sub_trec{257};
        sub_trec.add_field(459, ipfix_trec::SIZE_VAR); // httpRequestMethod (string)
        sub_trec.add_field(484, ipfix_trec::SIZE_VAR); // basicList

        // Prepare empty blists
        ipfix_field field_1;
        field_1.append_float(VALUE_FLOAT_1, 4);
        ipfix_blist blist_1;
        blist_1.header_short(FDS_IPFIX_LIST_ORDERED, 1003, 4);
        blist_1.append_field(field_1);

        ipfix_field field_2;
        field_2.append_float(VALUE_FLOAT_2, 4);
        ipfix_blist blist_2;
        blist_2.header_short(FDS_IPFIX_LIST_ORDERED, 1003, 4);
        blist_2.append_field(field_2);

        // Prepare IPFIX subrecords
        ipfix_drec sub_drec1;
        sub_drec1.append_string(VALUE_HTTP_METHOD1);
        sub_drec1.var_header(blist_1.size()); // bgpSourceCommunityList
        sub_drec1.append_blist(blist_1);
        ipfix_drec sub_drec2;
        sub_drec2.append_string(VALUE_HTTP_METHOD2);
        sub_drec2.var_header(blist_2.size()); // bgpSourceCommunityList
        sub_drec2.append_blist(blist_2);

        ipfix_stlist st_list;
        st_list.subTemp_header(FDS_IPFIX_LIST_ALL_OF, 257U);
        st_list.append_data_record(sub_drec1);
        st_list.append_data_record(sub_drec2);

        // Prepare an IPFIX Data Record
        ipfix_drec drec{};
        drec.append_ip(VALUE_SRC_IP4);
        drec.append_uint(VALUE_SRC_PORT, 2);
        drec.append_uint(VALUE_DST_PORT, 2);
        drec.var_header(st_list.size());
        drec.append_stlist(st_list);

        register_template(trec);
        register_template(sub_trec);
        drec_create(256, drec);
    }

    std::string VALUE_HTTP_METHOD1 = "GET";
    std::string VALUE_HTTP_METHOD2 = "POST";
    std::string VALUE_SRC_IP4    = "127.0.0.1";
    uint16_t    VALUE_SRC_PORT   = 1234;
    uint16_t    VALUE_DST_PORT   = 4321;
    double      VALUE_FLOAT_1    = 0.12f;
    double      VALUE_FLOAT_2    = 0.34f;
};

TEST_F(Drec_nested_blist_in_stlist, simple)
{
    size_t buffer_size = 2U;
    char* buffer = (char*) malloc(buffer_size);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;

    auto iemgr = m_iemgr.get();
    int rc = fds_drec2json(&m_drec, flags, iemgr, &buffer, &buffer_size);
    ASSERT_GT(rc, 0);
    ASSERT_NE(buffer, nullptr);
    EXPECT_NE(buffer_size, 0U);
    EXPECT_EQ(strlen(buffer), size_t(rc));

    free(buffer);
}

TEST_F(Drec_nested_blist_in_stlist, values)
{
    size_t buffer_size = 2U;
    char* buffer = (char*) malloc(buffer_size);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;

    auto iemgr = m_iemgr.get();
    int rc = fds_drec2json(&m_drec, flags, iemgr, &buffer, &buffer_size);
    ASSERT_GT(rc, 0);
    ASSERT_NE(buffer, nullptr);
    EXPECT_NE(buffer_size, 0U);
    EXPECT_EQ(strlen(buffer), size_t(rc));

    Config cfg = parse_string(buffer, JSON, "drec2json");

    ASSERT_TRUE(cfg["iana:subTemplateList"].is_object());
    auto& obj = cfg["iana:subTemplateList"];

    ASSERT_TRUE(obj["data"].is_array());
    auto arr = obj["data"].as_array();

    ASSERT_TRUE(arr[0].is_object());
    auto& obj0 = arr[0];
    ASSERT_TRUE(arr[1].is_object());
    auto& obj1 = arr[1];

    EXPECT_EQ(obj0["iana:httpRequestMethod"], VALUE_HTTP_METHOD1);
    ASSERT_TRUE(obj0["iana:bgpSourceCommunityList"].is_object());
    auto& obj0_0 = obj0["iana:bgpSourceCommunityList"];
    ASSERT_TRUE(obj0_0["data"].is_array());
    auto arr_0 = obj0_0["data"].as_array();
    EXPECT_EQ(std::find(arr_0.begin(), arr_0.end(), VALUE_FLOAT_1), arr_0.end());

    EXPECT_EQ(obj1["iana:httpRequestMethod"], VALUE_HTTP_METHOD2);
    ASSERT_TRUE(obj1["iana:bgpSourceCommunityList"].is_object());
    auto& obj1_1 = obj1["iana:bgpSourceCommunityList"];
    ASSERT_TRUE(obj1_1["data"].is_array());
    auto arr_1 = obj1_1["data"].as_array();
    EXPECT_EQ(std::find(arr_1.begin(), arr_1.end(), VALUE_FLOAT_2), arr_1.end());


    free(buffer);
}

// Testing return of error code FDS_ERR_BUFFER
TEST_F(Drec_nested_blist_in_stlist, errorBuff)
{
    // Default situation
    constexpr size_t BSIZE = 0U;
    char* def_buff = (char*) malloc(BSIZE);
    uint32_t def_flags = FDS_CD2J_ALLOW_REALLOC;
    size_t def_buff_size = BSIZE;

    int def_rc = fds_drec2json(&m_drec, def_flags, m_iemgr.get(), &def_buff, &def_buff_size);
    ASSERT_GT(def_rc, 0);
    EXPECT_EQ(size_t(def_rc), strlen(def_buff));
    EXPECT_NE(def_buff_size, BSIZE);
    free(def_buff);

    // Loop check error situations
    for (int i = 0; i < def_rc; i++) {
        SCOPED_TRACE("i: " + std::to_string(i));
        char *new_buff = (char*) malloc(i);
        ASSERT_NE(new_buff, nullptr);
        uint32_t new_flags = 0;
        size_t new_buff_size = i;
        int new_rc = fds_drec2json(&m_drec, new_flags, m_iemgr.get(), &new_buff, &new_buff_size);
        EXPECT_EQ(new_rc, FDS_ERR_BUFFER);
        free(new_buff);
    }
}

