#include <gtest/gtest.h>
#include <libfds.h>
#include <MsgGen.h>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class drecFind : public ::testing::Test {
private:
    // File with few IANA elements
    const char *ie_path = "data/iana.xml";

protected:
    // IE manager
    fds_iemgr_t *ie_mgr = nullptr;
    // Prepared Data record
    struct fds_drec rec;

    // Record values
    uint16_t    VALUE_SRC_PORT = 65000;
    std::string VALUE_SRC_IP4  = "127.0.0.1";
    uint16_t    VALUE_DST_PORT = 80;
    std::string VALUE_DST_IP4  = "8.8.8.8";
    uint8_t     VALUE_PROTO    = 6; // TCP
    uint64_t    VALUE_TS_FST   = 1522670362000ULL;
    uint64_t    VALUE_TS_LST   = 1522670372999ULL;
    uint64_t    VALUE_TS_FST_R = 1522670363123ULL;
    uint64_t    VALUE_TS_LST_R = 1522670369000ULL;
    std::string VALUE_APP_NAME = "firefox";
    std::string VALUE_APP_DSC  = "linux/web browser";
    uint64_t    VALUE_BYTES    = 1234567;
    uint64_t    VALUE_PKTS     = 12345;
    double      VALUE_UNKNOWN  = 3.1416f;
    uint64_t    VALUE_BYTES_R  = 7654321;
    uint64_t    VALUE_PKTS_R   = 54321;
    std::string VALUE_IFC1     = ""; // empty string
    std::string VALUE_IFC2     = "eth0";

    /** \brief Before each test */
    void SetUp() override {
        // Prepare an IE manger
        ie_mgr = fds_iemgr_create();
        if (!ie_mgr) {
            throw std::runtime_error("IPFIX IE Manager is not ready!");
        }

        if (fds_iemgr_read_file(ie_mgr, ie_path, true) != FDS_OK) {
            throw std::runtime_error("Failed to load Information Elements: "
                + std::string(fds_iemgr_last_err(ie_mgr)));
        }

        // Prepare a template
        ipfix_trec trec {256};
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

        uint16_t tmplt_size = trec.size();
        uint8_t *tmplt_raw = trec.release();
        struct fds_template *tmplt;
        ASSERT_EQ(fds_template_parse(FDS_TYPE_TEMPLATE, tmplt_raw, &tmplt_size, &tmplt), FDS_OK);
        free(tmplt_raw);
        ASSERT_EQ(fds_template_ies_define(tmplt, ie_mgr, false), FDS_OK);

        // Prepare a data record
        ipfix_drec drec {};
        drec.append_uint(VALUE_SRC_PORT, 2);
        drec.append_ip(VALUE_SRC_IP4);
        drec.append_uint(VALUE_DST_PORT, 2);
        drec.append_ip(VALUE_DST_IP4);
        drec.append_uint(VALUE_PROTO, 1);
        drec.append_uint(0, 3); // Padding
        drec.append_datetime(VALUE_TS_FST, FDS_ET_DATE_TIME_MILLISECONDS);
        drec.append_datetime(VALUE_TS_LST, FDS_ET_DATE_TIME_MILLISECONDS);
        drec.append_datetime(VALUE_TS_FST_R, FDS_ET_DATE_TIME_MILLISECONDS);
        drec.append_datetime(VALUE_TS_LST_R, FDS_ET_DATE_TIME_MILLISECONDS);
        drec.append_string(VALUE_APP_NAME);       // Adds variable head automatically (short version)
        drec.var_header(VALUE_APP_DSC.length(), true); // Adds variable head manually (long version)
        drec.append_string(VALUE_APP_DSC, VALUE_APP_DSC.length());
        drec.append_uint(0, 5); // Padding
        drec.append_uint(VALUE_BYTES, 8);
        drec.append_uint(VALUE_PKTS, 8);
        drec.append_float(VALUE_UNKNOWN, 4);
        drec.append_uint(VALUE_BYTES_R, 8);
        drec.append_uint(VALUE_PKTS_R, 8);
        drec.var_header(VALUE_IFC1.length(), false); // empty string (only header)
        drec.append_string(VALUE_IFC2);

        rec.size = drec.size();
        rec.data = drec.release();
        rec.tmplt = tmplt;
        rec.snap = nullptr;
    }

    /** \brief After each test */
    void TearDown() override {
        fds_template_destroy(const_cast<fds_template *>(rec.tmplt));
        free(rec.data);
        fds_iemgr_destroy(ie_mgr);
    }
};

class drecIter : public drecFind {
    // Nothing, we need just another name :D
};

// SIMPLE FIND -------------------------------------------------------------------------------------
// Try to find missing field
TEST_F(drecFind, missing)
{
    struct fds_drec_field field;
    // Some random fields...
    ASSERT_EQ(fds_drec_find(&rec, 0, 1000, &field), FDS_EOC);
    ASSERT_EQ(fds_drec_find(&rec, 0, 0, &field), FDS_EOC);
    ASSERT_EQ(fds_drec_find(&rec, 8888, 100, &field), FDS_EOC);
}

// Find fixed field before variable fields
TEST_F(drecFind, fixedBeforeVar)
{
    struct fds_drec_field field;
    uint64_t uint64_result;

    // Try to find SRC Port
    ASSERT_GE(fds_drec_find(&rec, 0, 7, &field), 0);
    EXPECT_EQ(field.size, 2);
    EXPECT_EQ(field.info->def->id, 7);
    EXPECT_EQ(field.info->def->scope->pen, 0);
    EXPECT_EQ(field.info->def->data_type, FDS_ET_UNSIGNED_16);

    ASSERT_EQ(fds_get_uint_be(field.data, field.size, &uint64_result), FDS_OK);
    EXPECT_EQ(uint64_result, VALUE_SRC_PORT);

    // Try to find flowStartMilliseconds
    enum fds_iemgr_element_type type = FDS_ET_DATE_TIME_MILLISECONDS;
    ASSERT_GE(fds_drec_find(&rec, 0, 152, &field), 0);
    EXPECT_EQ(field.size, 8);
    EXPECT_EQ(field.info->def->id, 152);
    EXPECT_EQ(field.info->def->scope->pen, 0);
    EXPECT_EQ(field.info->def->data_type, type);

    ASSERT_EQ(fds_get_datetime_lp_be(field.data, field.size, type, &uint64_result), FDS_OK);
    EXPECT_EQ(uint64_result, VALUE_TS_FST);
}

// Find fixed field after variable fields
TEST_F(drecFind, fixedAfterVar)
{
    struct fds_drec_field field;
    uint64_t uint64_result;

    // Try to find bytes
    ASSERT_GE(fds_drec_find(&rec, 0, 1, &field), 0);
    EXPECT_EQ(field.size, 8);
    EXPECT_EQ(field.info->def->id, 1);
    EXPECT_EQ(field.info->def->scope->pen, 0);
    EXPECT_EQ(field.info->def->data_type, FDS_ET_UNSIGNED_64);

    ASSERT_EQ(fds_get_uint_be(field.data, field.size, &uint64_result), FDS_OK);
    EXPECT_EQ(uint64_result, VALUE_BYTES);

    // Try to find packetDeltaCount (reverse)
    ASSERT_GE(fds_drec_find(&rec, 29305, 2, &field), 0);
    EXPECT_EQ(field.size, 8);
    EXPECT_EQ(field.info->def->id, 2);
    EXPECT_EQ(field.info->def->scope->pen, 29305);
    EXPECT_EQ(field.info->def->data_type, FDS_ET_UNSIGNED_64);

    ASSERT_EQ(fds_get_uint_be(field.data, field.size, &uint64_result), FDS_OK);
    EXPECT_EQ(uint64_result, VALUE_PKTS_R);

    // Try to find field with unknown definition
    ASSERT_GE(fds_drec_find(&rec, 10000, 100, &field), 0);
    EXPECT_EQ(field.size, 4);
    EXPECT_EQ(field.info->def, nullptr);

    double double_result;
    ASSERT_EQ(fds_get_float_be(field.data, field.size, &double_result), FDS_OK);
    EXPECT_FLOAT_EQ(double_result, VALUE_UNKNOWN);
}

// Find the first variable field
TEST_F(drecFind, varBeforeVar)
{
    // Try to find applicationName
    struct fds_drec_field field;
    ASSERT_GE(fds_drec_find(&rec, 0, 96, &field), 0);
    EXPECT_EQ(field.size, VALUE_APP_NAME.length());
    EXPECT_EQ(field.info->def->id, 96);
    EXPECT_EQ(field.info->def->scope->pen, 0);
    EXPECT_EQ(field.info->def->data_type, FDS_ET_STRING);

    char *buffer = new char[VALUE_APP_NAME.size() + 1];
    ASSERT_EQ(fds_get_string(field.data, field.size, buffer), FDS_OK);
    buffer[VALUE_APP_NAME.size()] = '\0';
    EXPECT_STREQ(buffer, VALUE_APP_NAME.c_str());
    delete[] buffer;
}

// Find a variable field after another variable field
TEST_F(drecFind, varAfterVar)
{
    struct fds_drec_field field;
    char *buffer;

    // Try to find applicationDescription
    ASSERT_GE(fds_drec_find(&rec, 0, 94, &field), 0);
    EXPECT_EQ(field.size, VALUE_APP_DSC.length());
    EXPECT_EQ(field.info->def->id, 94);
    EXPECT_EQ(field.info->def->scope->pen, 0);
    EXPECT_EQ(field.info->def->data_type, FDS_ET_STRING);

    buffer = new char[VALUE_APP_DSC.size() + 1];
    ASSERT_EQ(fds_get_string(field.data, field.size, buffer), FDS_OK);
    buffer[VALUE_APP_DSC.size()] = '\0';
    EXPECT_STREQ(buffer, VALUE_APP_DSC.c_str());
    delete[] buffer;

    // Try to find interfaceName (only the first occurrence)
    ASSERT_GE(fds_drec_find(&rec, 0, 82, &field), 0);
    EXPECT_EQ(field.size, 0); // empty string
    EXPECT_EQ(field.info->def->id, 82);
    EXPECT_EQ(field.info->def->scope->pen, 0);
    EXPECT_EQ(field.info->def->data_type, FDS_ET_STRING);
    // Cannot check empty string!
}

// ITERATOR -------------------------------------------------------------------------------------
// Iterate over whole record
TEST_F(drecIter, overWholeRec) // Automatically skip padding
{
    enum fds_iemgr_element_type ts_type = FDS_ET_DATE_TIME_MILLISECONDS;
    uint64_t uint64_result;
    double   double_result;
    char     ip_result[FDS_CONVERT_STRLEN_IP];
    char    *buffer;

    std::vector<uint16_t> flags{0, FDS_DREC_UNKNOWN_SKIP};
    for (uint16_t iter_flags : flags) {
        SCOPED_TRACE("Iterator flags: " + std::to_string(iter_flags));
        struct fds_drec_iter iter;
        fds_drec_iter_init(&iter, &rec, iter_flags);

        // SRC port
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 2);
        EXPECT_EQ(iter.field.info->id, 7);
        EXPECT_EQ(iter.field.info->def->id, 7);
        EXPECT_EQ(iter.field.info->en, 0);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_16);
        ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_SRC_PORT);

        // SRC IP address
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 4);
        EXPECT_EQ(iter.field.info->def->id, 8);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_IPV4_ADDRESS);
        ASSERT_GT(fds_ip2str(iter.field.data, iter.field.size, ip_result, sizeof(ip_result)), 0);
        EXPECT_EQ(ip_result, VALUE_SRC_IP4);

        // DST port
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 2);
        EXPECT_EQ(iter.field.info->def->id, 11);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_16);
        ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_DST_PORT);

        // DST IP address
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 4);
        EXPECT_EQ(iter.field.info->def->id, 12);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_IPV4_ADDRESS);
        ASSERT_GT(fds_ip2str(iter.field.data, iter.field.size, ip_result, sizeof(ip_result)), 0);
        EXPECT_EQ(ip_result, VALUE_DST_IP4);

        // protocolIdentifier
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 1);
        EXPECT_EQ(iter.field.info->def->id, 4);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_8);
        ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_PROTO);

        // -- padding should be skipped

        // flowStartMilliseconds
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 8);
        EXPECT_EQ(iter.field.info->def->id, 152);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, ts_type);

        ASSERT_EQ(fds_get_datetime_lp_be(iter.field.data, iter.field.size, ts_type, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_TS_FST);

        // flowEndMilliseconds
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 8);
        EXPECT_EQ(iter.field.info->def->id, 153);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, ts_type);

        ASSERT_EQ(fds_get_datetime_lp_be(iter.field.data, iter.field.size, ts_type, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_TS_LST);

        // flowStartMilliseconds (reverse)
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 8);
        EXPECT_EQ(iter.field.info->def->id, 152);
        EXPECT_EQ(iter.field.info->def->scope->pen, 29305);
        EXPECT_EQ(iter.field.info->def->data_type, ts_type);

        ASSERT_EQ(fds_get_datetime_lp_be(iter.field.data, iter.field.size, ts_type, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_TS_FST_R);

        // flowEndMilliseconds (reverse)
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 8);
        EXPECT_EQ(iter.field.info->def->id, 153);
        EXPECT_EQ(iter.field.info->def->scope->pen, 29305);
        EXPECT_EQ(iter.field.info->def->data_type, ts_type);

        ASSERT_EQ(fds_get_datetime_lp_be(iter.field.data, iter.field.size, ts_type, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_TS_LST_R);

        // applicationName
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, VALUE_APP_NAME.length());
        EXPECT_EQ(iter.field.info->def->id, 96);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_STRING);

        buffer = new char[VALUE_APP_NAME.size() + 1];
        ASSERT_EQ(fds_get_string(iter.field.data, iter.field.size, buffer), FDS_OK);
        buffer[VALUE_APP_NAME.size()] = '\0';
        EXPECT_STREQ(buffer, VALUE_APP_NAME.c_str());
        delete[] buffer;

        // applicationDescription
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, VALUE_APP_DSC.length());
        EXPECT_EQ(iter.field.info->def->id, 94);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_STRING);

        buffer = new char[VALUE_APP_DSC.size() + 1];
        ASSERT_EQ(fds_get_string(iter.field.data, iter.field.size, buffer), FDS_OK);
        buffer[VALUE_APP_DSC.size()] = '\0';
        EXPECT_STREQ(buffer, VALUE_APP_DSC.c_str());
        delete[] buffer;

        // -- padding should be skipped

        // octetDeltaCount
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 8);
        EXPECT_EQ(iter.field.info->def->id, 1);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_64);
        ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_BYTES);

        // packetDeltaCount
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 8);
        EXPECT_EQ(iter.field.info->def->id, 2);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_64);
        ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_PKTS);

        // -- field with unknown definition --
        if ((iter_flags & FDS_DREC_UNKNOWN_SKIP) == 0) {
            ASSERT_GE(fds_drec_iter_next(&iter), 0);
            EXPECT_EQ(iter.field.size, 4);
            EXPECT_EQ(iter.field.info->id, 100);
            EXPECT_EQ(iter.field.info->en, 10000);
            EXPECT_EQ(iter.field.info->def, nullptr);
            ASSERT_EQ(fds_get_float_be(iter.field.data, iter.field.size, &double_result), FDS_OK);
            EXPECT_FLOAT_EQ(double_result, VALUE_UNKNOWN);
        }

        // octetDeltaCount (reverse)
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 8);
        EXPECT_EQ(iter.field.info->def->id, 1);
        EXPECT_EQ(iter.field.info->def->scope->pen, 29305);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_64);
        ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_BYTES_R);

        // packetDeltaCount (reverse)
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 8);
        EXPECT_EQ(iter.field.info->def->id, 2);
        EXPECT_EQ(iter.field.info->def->scope->pen, 29305);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_64);
        ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_PKTS_R);

        // interfaceName (1. occurrence)
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 0); // empty string
        EXPECT_EQ(iter.field.info->id, 82);
        EXPECT_EQ(iter.field.info->def->id, 82);
        EXPECT_EQ(iter.field.info->en, 0);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_STRING);
        // Cannot check empty string!

        // interfaceName (2. occurrence)
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, VALUE_IFC2.size()); // empty string
        EXPECT_EQ(iter.field.info->id, 82);
        EXPECT_EQ(iter.field.info->def->id, 82);
        EXPECT_EQ(iter.field.info->en, 0);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_STRING);

        buffer = new char[VALUE_IFC2.size() + 1];
        ASSERT_EQ(fds_get_string(iter.field.data, iter.field.size, buffer), FDS_OK);
        buffer[VALUE_IFC2.size()] = '\0';
        EXPECT_STREQ(buffer, VALUE_IFC2.c_str());
        delete[] buffer;

        // End reached
        EXPECT_EQ(fds_drec_iter_next(&iter), FDS_EOC);
    }
}

// Iterate over whole record - forward direction
TEST_F(drecIter, overForwardDirection)
{
    enum fds_iemgr_element_type ts_type = FDS_ET_DATE_TIME_MILLISECONDS;
    uint64_t uint64_result;
    double   double_result;
    char     ip_result[FDS_CONVERT_STRLEN_IP];
    char    *buffer;

    std::vector<uint16_t> flags{
        FDS_DREC_BIFLOW_FWD,
        FDS_DREC_BIFLOW_FWD | FDS_DREC_UNKNOWN_SKIP,
        FDS_DREC_BIFLOW_FWD | FDS_DREC_REVERSE_SKIP
    };
    for (uint16_t iter_flags : flags) {
        SCOPED_TRACE("Iterator flags: " + std::to_string(iter_flags));
        struct fds_drec_iter iter;
        fds_drec_iter_init(&iter, &rec, iter_flags);

        // SRC port
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 2);
        EXPECT_EQ(iter.field.info->id, 7);
        EXPECT_EQ(iter.field.info->def->id, 7);
        EXPECT_EQ(iter.field.info->en, 0);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_16);
        ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_SRC_PORT);

        // SRC IP address
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 4);
        EXPECT_EQ(iter.field.info->def->id, 8);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_IPV4_ADDRESS);
        ASSERT_GT(fds_ip2str(iter.field.data, iter.field.size, ip_result, sizeof(ip_result)), 0);
        EXPECT_EQ(ip_result, VALUE_SRC_IP4);

        // DST port
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 2);
        EXPECT_EQ(iter.field.info->def->id, 11);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_16);
        ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_DST_PORT);

        // DST IP address
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 4);
        EXPECT_EQ(iter.field.info->def->id, 12);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_IPV4_ADDRESS);
        ASSERT_GT(fds_ip2str(iter.field.data, iter.field.size, ip_result, sizeof(ip_result)), 0);
        EXPECT_EQ(ip_result, VALUE_DST_IP4);

        // protocolIdentifier
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 1);
        EXPECT_EQ(iter.field.info->def->id, 4);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_8);
        ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_PROTO);

        // -- padding should be skipped

        // flowStartMilliseconds
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 8);
        EXPECT_EQ(iter.field.info->def->id, 152);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, ts_type);

        ASSERT_EQ(fds_get_datetime_lp_be(iter.field.data, iter.field.size, ts_type, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_TS_FST);

        // flowEndMilliseconds
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 8);
        EXPECT_EQ(iter.field.info->def->id, 153);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, ts_type);

        ASSERT_EQ(fds_get_datetime_lp_be(iter.field.data, iter.field.size, ts_type, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_TS_LST);

        if ((iter_flags & FDS_DREC_REVERSE_SKIP) == 0) {
            // flowStartMilliseconds (reverse)
            ASSERT_GE(fds_drec_iter_next(&iter), 0);
            EXPECT_EQ(iter.field.size, 8);
            EXPECT_EQ(iter.field.info->def->id, 152);
            EXPECT_EQ(iter.field.info->def->scope->pen, 29305);
            EXPECT_EQ(iter.field.info->def->data_type, ts_type);

            ASSERT_EQ(fds_get_datetime_lp_be(iter.field.data, iter.field.size, ts_type, &uint64_result), FDS_OK);
            EXPECT_EQ(uint64_result, VALUE_TS_FST_R);

            // flowEndMilliseconds (reverse)
            ASSERT_GE(fds_drec_iter_next(&iter), 0);
            EXPECT_EQ(iter.field.size, 8);
            EXPECT_EQ(iter.field.info->def->id, 153);
            EXPECT_EQ(iter.field.info->def->scope->pen, 29305);
            EXPECT_EQ(iter.field.info->def->data_type, ts_type);

            ASSERT_EQ(fds_get_datetime_lp_be(iter.field.data, iter.field.size, ts_type, &uint64_result), FDS_OK);
            EXPECT_EQ(uint64_result, VALUE_TS_LST_R);
        }

        // applicationName
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, VALUE_APP_NAME.length());
        EXPECT_EQ(iter.field.info->def->id, 96);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_STRING);

        buffer = new char[VALUE_APP_NAME.size() + 1];
        ASSERT_EQ(fds_get_string(iter.field.data, iter.field.size, buffer), FDS_OK);
        buffer[VALUE_APP_NAME.size()] = '\0';
        EXPECT_STREQ(buffer, VALUE_APP_NAME.c_str());
        delete[] buffer;

        // applicationDescription
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, VALUE_APP_DSC.length());
        EXPECT_EQ(iter.field.info->def->id, 94);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_STRING);

        buffer = new char[VALUE_APP_DSC.size() + 1];
        ASSERT_EQ(fds_get_string(iter.field.data, iter.field.size, buffer), FDS_OK);
        buffer[VALUE_APP_DSC.size()] = '\0';
        EXPECT_STREQ(buffer, VALUE_APP_DSC.c_str());
        delete[] buffer;

        // -- padding should be skipped

        // octetDeltaCount
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 8);
        EXPECT_EQ(iter.field.info->def->id, 1);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_64);
        ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_BYTES);

        // packetDeltaCount
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 8);
        EXPECT_EQ(iter.field.info->def->id, 2);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_64);
        ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_PKTS);

        // -- field with unknown definition --
        if ((iter_flags & FDS_DREC_UNKNOWN_SKIP) == 0) {
            ASSERT_GE(fds_drec_iter_next(&iter), 0);
            EXPECT_EQ(iter.field.size, 4);
            EXPECT_EQ(iter.field.info->id, 100);
            EXPECT_EQ(iter.field.info->en, 10000);
            EXPECT_EQ(iter.field.info->def, nullptr);
            ASSERT_EQ(fds_get_float_be(iter.field.data, iter.field.size, &double_result), FDS_OK);
            EXPECT_FLOAT_EQ(double_result, VALUE_UNKNOWN);
        }

        if ((iter_flags & FDS_DREC_REVERSE_SKIP) == 0) {
            // octetDeltaCount (reverse)
            ASSERT_GE(fds_drec_iter_next(&iter), 0);
            EXPECT_EQ(iter.field.size, 8);
            EXPECT_EQ(iter.field.info->def->id, 1);
            EXPECT_EQ(iter.field.info->def->scope->pen, 29305);
            EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_64);
            ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
            EXPECT_EQ(uint64_result, VALUE_BYTES_R);

            // packetDeltaCount (reverse)
            ASSERT_GE(fds_drec_iter_next(&iter), 0);
            EXPECT_EQ(iter.field.size, 8);
            EXPECT_EQ(iter.field.info->def->id, 2);
            EXPECT_EQ(iter.field.info->def->scope->pen, 29305);
            EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_64);
            ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
            EXPECT_EQ(uint64_result, VALUE_PKTS_R);
        }

        // interfaceName (1. occurrence)
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 0); // empty string
        EXPECT_EQ(iter.field.info->id, 82);
        EXPECT_EQ(iter.field.info->def->id, 82);
        EXPECT_EQ(iter.field.info->en, 0);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_STRING);
        // Cannot check empty string!

        // interfaceName (2. occurrence)
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, VALUE_IFC2.size()); // empty string
        EXPECT_EQ(iter.field.info->id, 82);
        EXPECT_EQ(iter.field.info->def->id, 82);
        EXPECT_EQ(iter.field.info->en, 0);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_STRING);

        buffer = new char[VALUE_IFC2.size() + 1];
        ASSERT_EQ(fds_get_string(iter.field.data, iter.field.size, buffer), FDS_OK);
        buffer[VALUE_IFC2.size()] = '\0';
        EXPECT_STREQ(buffer, VALUE_IFC2.c_str());
        delete[] buffer;

        // End reached
        EXPECT_EQ(fds_drec_iter_next(&iter), FDS_EOC);
    }
}

// Iterate over whole record - reverse direction
TEST_F(drecIter, overReverseDirection)
{
    enum fds_iemgr_element_type ts_type = FDS_ET_DATE_TIME_MILLISECONDS;
    uint64_t uint64_result;
    double   double_result;
    char     ip_result[FDS_CONVERT_STRLEN_IP];
    char    *buffer;

    std::vector<uint16_t> flags{
        FDS_DREC_BIFLOW_REV,
        FDS_DREC_BIFLOW_REV | FDS_DREC_UNKNOWN_SKIP,
        FDS_DREC_BIFLOW_REV | FDS_DREC_REVERSE_SKIP
    };
    for (uint16_t iter_flags : flags) {
        SCOPED_TRACE("Iterator flags: " + std::to_string(iter_flags));
        struct fds_drec_iter iter;
        fds_drec_iter_init(&iter, &rec, iter_flags);

        // destinationTransportPort
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 2);
        EXPECT_EQ(iter.field.info->id, 11);
        EXPECT_EQ(iter.field.info->def->id, 11);
        EXPECT_EQ(iter.field.info->en, 0);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_16);
        ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_SRC_PORT);

        // destinationIPv4Address
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 4);
        EXPECT_EQ(iter.field.info->def->id, 12);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_IPV4_ADDRESS);
        ASSERT_GT(fds_ip2str(iter.field.data, iter.field.size, ip_result, sizeof(ip_result)), 0);
        EXPECT_EQ(ip_result, VALUE_SRC_IP4);

        // sourceTransportPort
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 2);
        EXPECT_EQ(iter.field.info->def->id, 7);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_16);
        ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_DST_PORT);

        // sourceIPv4Address
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 4);
        EXPECT_EQ(iter.field.info->def->id, 8);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_IPV4_ADDRESS);
        ASSERT_GT(fds_ip2str(iter.field.data, iter.field.size, ip_result, sizeof(ip_result)), 0);
        EXPECT_EQ(ip_result, VALUE_DST_IP4);

        // protocolIdentifier
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 1);
        EXPECT_EQ(iter.field.info->def->id, 4);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_8);
        ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_PROTO);

        // -- padding should be skipped

        if ((iter_flags & FDS_DREC_REVERSE_SKIP) == 0) {
            // flowStartMilliseconds (reverse)
            ASSERT_GE(fds_drec_iter_next(&iter), 0);
            EXPECT_EQ(iter.field.size, 8);
            EXPECT_EQ(iter.field.info->def->id, 152);
            EXPECT_EQ(iter.field.info->def->scope->pen, 29305);
            EXPECT_EQ(iter.field.info->def->data_type, ts_type);

            ASSERT_EQ(fds_get_datetime_lp_be(iter.field.data, iter.field.size, ts_type, &uint64_result), FDS_OK);
            EXPECT_EQ(uint64_result, VALUE_TS_FST);

            // flowEndMilliseconds (reverse)
            ASSERT_GE(fds_drec_iter_next(&iter), 0);
            EXPECT_EQ(iter.field.size, 8);
            EXPECT_EQ(iter.field.info->def->id, 153);
            EXPECT_EQ(iter.field.info->def->scope->pen, 29305);
            EXPECT_EQ(iter.field.info->def->data_type, ts_type);

            ASSERT_EQ(fds_get_datetime_lp_be(iter.field.data, iter.field.size, ts_type, &uint64_result), FDS_OK);
            EXPECT_EQ(uint64_result, VALUE_TS_LST);
        }

        // flowStartMilliseconds
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 8);
        EXPECT_EQ(iter.field.info->def->id, 152);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, ts_type);

        ASSERT_EQ(fds_get_datetime_lp_be(iter.field.data, iter.field.size, ts_type, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_TS_FST_R);

        // flowEndMilliseconds
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 8);
        EXPECT_EQ(iter.field.info->def->id, 153);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, ts_type);

        ASSERT_EQ(fds_get_datetime_lp_be(iter.field.data, iter.field.size, ts_type, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_TS_LST_R);

        // applicationName
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, VALUE_APP_NAME.length());
        EXPECT_EQ(iter.field.info->def->id, 96);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_STRING);

        buffer = new char[VALUE_APP_NAME.size() + 1];
        ASSERT_EQ(fds_get_string(iter.field.data, iter.field.size, buffer), FDS_OK);
        buffer[VALUE_APP_NAME.size()] = '\0';
        EXPECT_STREQ(buffer, VALUE_APP_NAME.c_str());
        delete[] buffer;

        // applicationDescription
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, VALUE_APP_DSC.length());
        EXPECT_EQ(iter.field.info->def->id, 94);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_STRING);

        buffer = new char[VALUE_APP_DSC.size() + 1];
        ASSERT_EQ(fds_get_string(iter.field.data, iter.field.size, buffer), FDS_OK);
        buffer[VALUE_APP_DSC.size()] = '\0';
        EXPECT_STREQ(buffer, VALUE_APP_DSC.c_str());
        delete[] buffer;

        // -- padding should be skipped

        if ((iter_flags & FDS_DREC_REVERSE_SKIP) == 0) {
            // octetDeltaCount  (reverse)
            ASSERT_GE(fds_drec_iter_next(&iter), 0);
            EXPECT_EQ(iter.field.size, 8);
            EXPECT_EQ(iter.field.info->def->id, 1);
            EXPECT_EQ(iter.field.info->def->scope->pen, 29305);
            EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_64);
            ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
            EXPECT_EQ(uint64_result, VALUE_BYTES);

            // packetDeltaCount  (reverse)
            ASSERT_GE(fds_drec_iter_next(&iter), 0);
            EXPECT_EQ(iter.field.size, 8);
            EXPECT_EQ(iter.field.info->def->id, 2);
            EXPECT_EQ(iter.field.info->def->scope->pen, 29305);
            EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_64);
            ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
            EXPECT_EQ(uint64_result, VALUE_PKTS);
        }

        // -- field with unknown definition --
        if ((iter_flags & FDS_DREC_UNKNOWN_SKIP) == 0) {
            ASSERT_GE(fds_drec_iter_next(&iter), 0);
            EXPECT_EQ(iter.field.size, 4);
            EXPECT_EQ(iter.field.info->id, 100);
            EXPECT_EQ(iter.field.info->en, 10000);
            EXPECT_EQ(iter.field.info->def, nullptr);
            ASSERT_EQ(fds_get_float_be(iter.field.data, iter.field.size, &double_result), FDS_OK);
            EXPECT_FLOAT_EQ(double_result, VALUE_UNKNOWN);
        }

        // octetDeltaCount
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 8);
        EXPECT_EQ(iter.field.info->def->id, 1);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_64);
        ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_BYTES_R);

        // packetDeltaCount
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 8);
        EXPECT_EQ(iter.field.info->def->id, 2);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_64);
        ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
        EXPECT_EQ(uint64_result, VALUE_PKTS_R);


        // interfaceName (1. occurrence)
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, 0); // empty string
        EXPECT_EQ(iter.field.info->id, 82);
        EXPECT_EQ(iter.field.info->def->id, 82);
        EXPECT_EQ(iter.field.info->en, 0);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_STRING);
        // Cannot check empty string!

        // interfaceName (2. occurrence)
        ASSERT_GE(fds_drec_iter_next(&iter), 0);
        EXPECT_EQ(iter.field.size, VALUE_IFC2.size()); // empty string
        EXPECT_EQ(iter.field.info->id, 82);
        EXPECT_EQ(iter.field.info->def->id, 82);
        EXPECT_EQ(iter.field.info->en, 0);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_STRING);

        buffer = new char[VALUE_IFC2.size() + 1];
        ASSERT_EQ(fds_get_string(iter.field.data, iter.field.size, buffer), FDS_OK);
        buffer[VALUE_IFC2.size()] = '\0';
        EXPECT_STREQ(buffer, VALUE_IFC2.c_str());
        delete[] buffer;

        // End reached
        EXPECT_EQ(fds_drec_iter_next(&iter), FDS_EOC);
    }
}

// Check if padding fields are shown if the padding flag is set
TEST_F(drecIter, showPadding)
{
    struct fds_drec_iter iter;
    fds_drec_iter_init(&iter, &rec, FDS_DREC_PADDING_SHOW);

    const unsigned int padding_exp = 2U;
    unsigned int padding_cnt = 0;

    while (fds_drec_iter_next(&iter) != FDS_EOC) {
        // Check if the field is padding
        if (iter.field.info->en != 0 || iter.field.info->id != 210) {
            continue;
        }

        ++padding_cnt;
    }

    EXPECT_EQ(padding_cnt, padding_exp);
}

// ITERATOR - find --------------------------------------------------------------------------------

// Try to find missing field
TEST_F(drecIter, findMissing)
{
    std::vector<uint16_t> flags{
        0,
        FDS_DREC_BIFLOW_FWD,
        FDS_DREC_BIFLOW_FWD | FDS_DREC_UNKNOWN_SKIP,
        FDS_DREC_BIFLOW_FWD | FDS_DREC_REVERSE_SKIP,
        FDS_DREC_BIFLOW_REV,
        FDS_DREC_BIFLOW_REV | FDS_DREC_UNKNOWN_SKIP,
        FDS_DREC_BIFLOW_REV | FDS_DREC_REVERSE_SKIP
    };

    for (uint16_t iter_flags : flags) {
        SCOPED_TRACE("Flags: " + std::to_string(iter_flags));
        struct fds_drec_iter iter;
        fds_drec_iter_init(&iter, &rec, iter_flags);

        // Some random fields...
        EXPECT_EQ(fds_drec_iter_find(&iter, 0, 0), FDS_EOC);
        fds_drec_iter_rewind(&iter);
        EXPECT_EQ(fds_drec_iter_find(&iter, 0, 1000), FDS_EOC);
        fds_drec_iter_rewind(&iter);
        EXPECT_EQ(fds_drec_iter_find(&iter, 8888, 100), FDS_EOC);
    }
}

// Find fixed field before variable fields
TEST_F(drecIter, findFixedBeforeVar)
{
    std::vector<uint16_t> flags{
        0,
        FDS_DREC_BIFLOW_FWD,
        FDS_DREC_BIFLOW_FWD | FDS_DREC_UNKNOWN_SKIP,
        FDS_DREC_BIFLOW_FWD | FDS_DREC_REVERSE_SKIP,
        FDS_DREC_BIFLOW_REV,
        FDS_DREC_BIFLOW_REV | FDS_DREC_UNKNOWN_SKIP,
        FDS_DREC_BIFLOW_REV | FDS_DREC_REVERSE_SKIP
    };

    for (uint16_t iter_flags : flags) {
        SCOPED_TRACE("Flags: " + std::to_string(iter_flags));

        struct fds_drec_iter iter;
        fds_drec_iter_init(&iter, &rec, iter_flags);
        uint64_t uint64_result;

        // Try to find SRC Port
        ASSERT_GE(fds_drec_iter_find(&iter, 0, 7), 0);
        EXPECT_EQ(iter.field.size, 2);
        EXPECT_EQ(iter.field.info->def->id, 7);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_16);

        ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
        if ((iter_flags & FDS_DREC_BIFLOW_REV) == 0) {
            EXPECT_EQ(uint64_result, VALUE_SRC_PORT);
        } else {
            EXPECT_EQ(uint64_result, VALUE_DST_PORT);
        }

        EXPECT_EQ(fds_drec_iter_find(&iter, 0, 7), FDS_EOC);
        fds_drec_iter_rewind(&iter);

        // Try to find flowStartMilliseconds
        enum fds_iemgr_element_type type = FDS_ET_DATE_TIME_MILLISECONDS;

        ASSERT_GE(fds_drec_iter_find(&iter, 0, 152), 0);
        EXPECT_EQ(iter.field.size, 8);
        EXPECT_EQ(iter.field.info->def->id, 152);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, type);

        ASSERT_EQ(fds_get_datetime_lp_be(iter.field.data, iter.field.size, type, &uint64_result),
            FDS_OK);
        if ((iter_flags & FDS_DREC_BIFLOW_REV) == 0) {
            EXPECT_EQ(uint64_result, VALUE_TS_FST);
        } else {
            EXPECT_EQ(uint64_result, VALUE_TS_FST_R);
        }
        EXPECT_EQ(fds_drec_iter_find(&iter, 0, 152), FDS_EOC);
    }
}

// Find fixed field after variable fields
TEST_F(drecIter, findFixedAfterVar)
{
    std::vector<uint16_t> flags{
        0,
        FDS_DREC_BIFLOW_FWD,
        FDS_DREC_BIFLOW_FWD | FDS_DREC_UNKNOWN_SKIP,
        FDS_DREC_BIFLOW_FWD | FDS_DREC_REVERSE_SKIP,
        FDS_DREC_BIFLOW_REV,
        FDS_DREC_BIFLOW_REV | FDS_DREC_UNKNOWN_SKIP,
        FDS_DREC_BIFLOW_REV | FDS_DREC_REVERSE_SKIP
    };

    for (uint16_t iter_flags : flags) {
        SCOPED_TRACE("Flags: " + std::to_string(iter_flags));

        struct fds_drec_iter iter;
        fds_drec_iter_init(&iter, &rec, iter_flags);
        uint64_t uint64_result;

        // Try to find bytes
        ASSERT_GE(fds_drec_iter_find(&iter, 0, 1), 0);
        EXPECT_EQ(iter.field.size, 8);
        EXPECT_EQ(iter.field.info->def->id, 1);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_64);

        ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
        if ((iter_flags & FDS_DREC_BIFLOW_REV) == 0) {
            EXPECT_EQ(uint64_result, VALUE_BYTES);
        } else {
            EXPECT_EQ(uint64_result, VALUE_BYTES_R);
        }
        EXPECT_EQ(fds_drec_iter_find(&iter, 0, 1), FDS_EOC);
        fds_drec_iter_rewind(&iter);

        // Try to find packetDeltaCount (reverse)
        ASSERT_GE(fds_drec_iter_find(&iter, 29305, 2), 0);
        EXPECT_EQ(iter.field.size, 8);
        EXPECT_EQ(iter.field.info->def->id, 2);
        EXPECT_EQ(iter.field.info->def->scope->pen, 29305);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_UNSIGNED_64);

        ASSERT_EQ(fds_get_uint_be(iter.field.data, iter.field.size, &uint64_result), FDS_OK);
        if ((iter_flags & FDS_DREC_BIFLOW_REV) == 0) {
            EXPECT_EQ(uint64_result, VALUE_PKTS_R);
        } else {
            EXPECT_EQ(uint64_result, VALUE_PKTS);
        }
        EXPECT_EQ(fds_drec_iter_find(&iter, 29305, 2), FDS_EOC);
        fds_drec_iter_rewind(&iter);

        // Try to find field with unknown definition
        ASSERT_GE(fds_drec_iter_find(&iter, 10000, 100), 0);
        EXPECT_EQ(iter.field.size, 4);
        EXPECT_EQ(iter.field.info->def, nullptr);

        double double_result;
        ASSERT_EQ(fds_get_float_be(iter.field.data, iter.field.size, &double_result), FDS_OK);
        EXPECT_FLOAT_EQ(double_result, VALUE_UNKNOWN);
        EXPECT_EQ(fds_drec_iter_find(&iter, 10000, 100), FDS_EOC);
    }
}

// Find the first variable field
TEST_F(drecIter, findVarBeforeVar)
{
    std::vector<uint16_t> flags{
        0,
        FDS_DREC_BIFLOW_FWD,
        FDS_DREC_BIFLOW_FWD | FDS_DREC_UNKNOWN_SKIP,
        FDS_DREC_BIFLOW_FWD | FDS_DREC_REVERSE_SKIP,
        FDS_DREC_BIFLOW_REV,
        FDS_DREC_BIFLOW_REV | FDS_DREC_UNKNOWN_SKIP,
        FDS_DREC_BIFLOW_REV | FDS_DREC_REVERSE_SKIP
    };

    for (uint16_t iter_flags : flags) {
        SCOPED_TRACE("Flags: " + std::to_string(iter_flags));
        struct fds_drec_iter iter;
        fds_drec_iter_init(&iter, &rec, iter_flags);

        // Try to find applicationName
        ASSERT_GE(fds_drec_iter_find(&iter, 0, 96), 0);
        EXPECT_EQ(iter.field.size, VALUE_APP_NAME.length());
        EXPECT_EQ(iter.field.info->def->id, 96);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_STRING);

        char *buffer = new char[VALUE_APP_NAME.size() + 1];
        ASSERT_EQ(fds_get_string(iter.field.data, iter.field.size, buffer), FDS_OK);
        buffer[VALUE_APP_NAME.size()] = '\0';
        EXPECT_STREQ(buffer, VALUE_APP_NAME.c_str());
        delete[] buffer;

        EXPECT_EQ(fds_drec_iter_find(&iter, 0, 96), FDS_EOC);
    }
}

// Find a variable field after another variable field
TEST_F(drecIter, findVarAfterVar)
{
    std::vector<uint16_t> flags{
        0,
        FDS_DREC_BIFLOW_FWD,
        FDS_DREC_BIFLOW_FWD | FDS_DREC_UNKNOWN_SKIP,
        FDS_DREC_BIFLOW_FWD | FDS_DREC_REVERSE_SKIP,
        FDS_DREC_BIFLOW_REV,
        FDS_DREC_BIFLOW_REV | FDS_DREC_UNKNOWN_SKIP,
        FDS_DREC_BIFLOW_REV | FDS_DREC_REVERSE_SKIP
    };

    for (uint16_t iter_flags : flags) {
        SCOPED_TRACE("Flags: " + std::to_string(iter_flags));
        struct fds_drec_iter iter;
        fds_drec_iter_init(&iter, &rec, iter_flags);
        char *buffer;

        // Try to find applicationDescription
        ASSERT_GE(fds_drec_iter_find(&iter, 0, 94), 0);
        EXPECT_EQ(iter.field.size, VALUE_APP_DSC.length());
        EXPECT_EQ(iter.field.info->def->id, 94);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_STRING);

        buffer = new char[VALUE_APP_DSC.size() + 1];
        ASSERT_EQ(fds_get_string(iter.field.data, iter.field.size, buffer), FDS_OK);
        buffer[VALUE_APP_DSC.size()] = '\0';
        EXPECT_STREQ(buffer, VALUE_APP_DSC.c_str());
        delete[] buffer;

        EXPECT_EQ(fds_drec_iter_find(&iter, 0, 94), FDS_EOC);
    }
}

TEST_F(drecIter, findMultipleOccurrences)
{
    std::vector<uint16_t> flags{
        0,
        FDS_DREC_BIFLOW_FWD,
        FDS_DREC_BIFLOW_FWD | FDS_DREC_UNKNOWN_SKIP,
        FDS_DREC_BIFLOW_FWD | FDS_DREC_REVERSE_SKIP,
        FDS_DREC_BIFLOW_REV,
        FDS_DREC_BIFLOW_REV | FDS_DREC_UNKNOWN_SKIP,
        FDS_DREC_BIFLOW_REV | FDS_DREC_REVERSE_SKIP
    };

    for (uint16_t iter_flags : flags) {
        SCOPED_TRACE("Flags: " + std::to_string(iter_flags));
        struct fds_drec_iter iter;
        fds_drec_iter_init(&iter, &rec, iter_flags);
        char *buffer;

        // interfaceName (1. occurrence)
        ASSERT_GE(fds_drec_iter_find(&iter, 0, 82), 0);
        EXPECT_EQ(iter.field.size, 0); // empty string
        EXPECT_EQ(iter.field.info->def->id, 82);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_STRING);
        // Cannot check empty string!

        // interfaceName (2. occurrence)
        ASSERT_GE(fds_drec_iter_find(&iter, 0, 82), 0);
        EXPECT_EQ(iter.field.size, VALUE_IFC2.size()); // empty string
        EXPECT_EQ(iter.field.info->id, 82);
        EXPECT_EQ(iter.field.info->def->id, 82);
        EXPECT_EQ(iter.field.info->en, 0);
        EXPECT_EQ(iter.field.info->def->scope->pen, 0);
        EXPECT_EQ(iter.field.info->def->data_type, FDS_ET_STRING);

        buffer = new char[VALUE_IFC2.size() + 1];
        ASSERT_EQ(fds_get_string(iter.field.data, iter.field.size, buffer), FDS_OK);
        buffer[VALUE_IFC2.size()] = '\0';
        EXPECT_STREQ(buffer, VALUE_IFC2.c_str());
        delete[] buffer;

        // End reached
        EXPECT_EQ(fds_drec_iter_next(&iter), FDS_EOC);
    }
}
