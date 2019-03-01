/*
 * SubTemplateList and SubTemplateMultiList tests
 */

#include <gtest/gtest.h>
#include <libfds.h>
#include <MsgGen.h>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class st_list : public ::testing::Test {
private:
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

protected:
    // Expected error message when everything is OK
    const char *OK_MSG = "No error.";

    // Template manager with few templates
    fds_tmgr_t *tmgr;
    // Snapshot of the manager
    const fds_tsnapshot_t *tsnap;
    // Sample Data Records based on the templates in the manager
    ipfix_drec drec256;
    ipfix_drec drec257;
    ipfix_drec drec258_v1;
    ipfix_drec drec258_v2;

    // Field with the list content
    fds_drec_field list_field;

    /** Before each test  */
    void SetUp() override;
    /** After each test  */
    void TearDown() override;

    /** Add IPFIX Templates to the template manager */
    void prepare_templates();
    /** Prepare records */
    void prepare_records();

    /** Check if a record matches the Data Record based on Template ID 256 */
    void check256(struct fds_drec &rec);
    /** Check if a record matches the Data Record based on Template ID 257 */
    void check257(struct fds_drec &rec);
    /** Check if a record matches the Data Record based on Template ID 258 (v1) */
    void check258_v1(struct fds_drec &rec);
    /** Check if a record matches the Data Record based on Template ID 258 (v2) */
    void check258_v2(struct fds_drec &rec);

    uint8_t *reduce_size(const uint8_t *mem, size_t size);
};

void st_list::SetUp()
{
    // Create a new template manager
    tmgr = fds_tmgr_create(FDS_SESSION_UDP);
    ASSERT_NE(tmgr, nullptr);
    ASSERT_EQ(fds_tmgr_set_time(tmgr, 0), FDS_OK);

    // Prepare Templates and Data Records
    prepare_templates();
    prepare_records();

    // Initialize the list field
    list_field.data = nullptr;
    list_field.size = 0;
    list_field.info = nullptr;
}

void st_list::TearDown()
{
    // Remove the template manager
    fds_tmgr_destroy(tmgr);

    // Destroy the field (if defined)
    free(list_field.data);
}

void st_list::prepare_templates()
{
    std::unique_ptr<uint8_t, decltype(&::free)> tmplt_ptr(nullptr, &::free); // Raw template
    uint16_t tmplt_size;  // Size of the raw template
    struct fds_template *tmplt_parsed;  // Parser raw template

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

    tmplt_size = trec.size();
    tmplt_ptr.reset(trec.release());
    ASSERT_EQ(fds_template_parse(FDS_TYPE_TEMPLATE, tmplt_ptr.get(), &tmplt_size, &tmplt_parsed), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_add(tmgr, tmplt_parsed), FDS_OK);

    ipfix_trec trec2 {257};
    trec2.add_field( 96, ipfix_trec::SIZE_VAR); // applicationName
    trec2.add_field( 94, ipfix_trec::SIZE_VAR); // applicationDescription
    trec2.add_field(210, 5);                    // -- paddingOctets
    trec2.add_field(  1, 8);                    // octetDeltaCount
    trec2.add_field(  2, 8);                    // packetDeltaCount
    trec2.add_field(100, 4, 10000);             // -- field with unknown definition --
    trec2.add_field(  1, 8, 29305);             // octetDeltaCount (reverse)
    trec2.add_field(  2, 8, 29305);             // packetDeltaCount (reverse)
    trec2.add_field( 82, ipfix_trec::SIZE_VAR); // interfaceName
    trec2.add_field( 82, ipfix_trec::SIZE_VAR); // interfaceName (second occurrence)

    tmplt_size = trec2.size();
    tmplt_ptr.reset(trec2.release());
    ASSERT_EQ(fds_template_parse(FDS_TYPE_TEMPLATE, tmplt_ptr.get(), &tmplt_size, &tmplt_parsed), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_add(tmgr, tmplt_parsed), FDS_OK);

    ipfix_trec trec3 {258};
    trec3.add_field(  1, 8);                    // octetDeltaCount
    trec3.add_field( 82, ipfix_trec::SIZE_VAR); // interfaceName
    trec3.add_field(  2, 4);                    // packetDeltaCount

    tmplt_size = trec3.size();
    tmplt_ptr.reset(trec3.release());
    ASSERT_EQ(fds_template_parse(FDS_TYPE_TEMPLATE, tmplt_ptr.get(), &tmplt_size, &tmplt_parsed), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_add(tmgr, tmplt_parsed), FDS_OK);

    ASSERT_EQ(fds_tmgr_snapshot_get(tmgr, &tsnap), FDS_OK);
}

void st_list::prepare_records()
{
    // Prepare a data record
    drec256.append_uint(VALUE_SRC_PORT, 2);
    drec256.append_ip(VALUE_SRC_IP4);
    drec256.append_uint(VALUE_DST_PORT, 2);
    drec256.append_ip(VALUE_DST_IP4);
    drec256.append_uint(VALUE_PROTO, 1);
    drec256.append_uint(0, 3); // Padding
    drec256.append_datetime(VALUE_TS_FST, FDS_ET_DATE_TIME_MILLISECONDS);
    drec256.append_datetime(VALUE_TS_LST, FDS_ET_DATE_TIME_MILLISECONDS);
    drec256.append_datetime(VALUE_TS_FST_R, FDS_ET_DATE_TIME_MILLISECONDS);
    drec256.append_datetime(VALUE_TS_LST_R, FDS_ET_DATE_TIME_MILLISECONDS);

    drec257.append_string(VALUE_APP_NAME);       // Adds variable head automatically (short version)
    drec257.var_header(VALUE_APP_DSC.length(), true); // Adds variable head manually (long version)
    drec257.append_string(VALUE_APP_DSC, VALUE_APP_DSC.length());
    drec257.append_uint(0, 5); // Padding
    drec257.append_uint(VALUE_BYTES, 8);
    drec257.append_uint(VALUE_PKTS, 8);
    drec257.append_float(VALUE_UNKNOWN, 4);
    drec257.append_uint(VALUE_BYTES_R, 8);
    drec257.append_uint(VALUE_PKTS_R, 8);
    drec257.var_header(VALUE_IFC1.length(), false); // empty string (only header)
    drec257.append_string(VALUE_IFC2);

    drec258_v1.append_uint(VALUE_BYTES, 8);
    drec258_v1.var_header(VALUE_IFC1.length(), false); // empty string (only header)
    drec258_v1.append_uint(VALUE_PKTS, 4);

    drec258_v2.append_uint(VALUE_BYTES_R, 8);
    drec258_v2.append_string(VALUE_IFC2);
    drec258_v2.append_uint(VALUE_PKTS_R, 4);
}

void st_list::check256(struct fds_drec &rec)
{
    EXPECT_EQ(rec.snap, tsnap);
    EXPECT_EQ(rec.size, drec256.size());
    ASSERT_NE(rec.tmplt, nullptr);

    struct fds_drec_iter it;
    fds_drec_iter_init(&it, &rec, 0);
    ASSERT_NE(fds_drec_iter_next(&it), FDS_EOC);

    // Check the first value
    uint64_t src_port;
    ASSERT_EQ(fds_get_uint_be(it.field.data, it.field.size, &src_port), FDS_OK);
    EXPECT_EQ(src_port, VALUE_SRC_PORT);

    // Skip to the last record
    for (int i = 1; i < 9; i++) { // note: padding is automatically skipped, by default
        ASSERT_NE(fds_drec_iter_next(&it), FDS_EOC);
    }

    // Check the last value
    uint64_t ts_lsr_r;
    ASSERT_EQ(fds_get_datetime_lp_be(it.field.data, it.field.size, FDS_ET_DATE_TIME_MILLISECONDS,
        &ts_lsr_r), FDS_OK);
    EXPECT_EQ(ts_lsr_r, VALUE_TS_LST_R);
    ASSERT_EQ(fds_drec_iter_next(&it), FDS_EOC);
}

void st_list::check257(struct fds_drec &rec)
{
    EXPECT_EQ(rec.snap, tsnap);
    EXPECT_EQ(rec.size, drec257.size());
    ASSERT_NE(rec.tmplt, nullptr);

    struct fds_drec_iter it;
    fds_drec_iter_init(&it, &rec, 0);
    ASSERT_NE(fds_drec_iter_next(&it), FDS_EOC);

    // Check the first value
    ASSERT_EQ(it.field.size, VALUE_APP_NAME.size());
    std::unique_ptr<char[]> str(new char[it.field.size]);
    ASSERT_EQ(fds_get_string(it.field.data, it.field.size, str.get()), FDS_OK);
    ASSERT_EQ(memcmp(str.get(), VALUE_APP_NAME.c_str(), it.field.size), 0);

    // Skip to the last record
    for (int i = 1; i < 9; i++) { // note: padding is automatically skipped, by default
        ASSERT_NE(fds_drec_iter_next(&it), FDS_EOC);
    }

    // Check the last value
    ASSERT_EQ(it.field.size, VALUE_IFC2.size());
    str.reset(new char[it.field.size]);
    ASSERT_EQ(fds_get_string(it.field.data, it.field.size, str.get()), FDS_OK);
    ASSERT_EQ(memcmp(str.get(), VALUE_IFC2.c_str(), it.field.size), 0);

    ASSERT_EQ(fds_drec_iter_next(&it), FDS_EOC);
}

void st_list::check258_v1(struct fds_drec &rec)
{
    EXPECT_EQ(rec.snap, tsnap);
    EXPECT_EQ(rec.size, drec258_v1.size());
    ASSERT_NE(rec.tmplt, nullptr);

    struct fds_drec_iter it;
    fds_drec_iter_init(&it, &rec, 0);

    ASSERT_NE(fds_drec_iter_next(&it), FDS_EOC);
    uint64_t bytes;
    ASSERT_EQ(fds_get_uint_be(it.field.data, it.field.size, &bytes), FDS_OK);
    EXPECT_EQ(bytes, VALUE_BYTES);

    ASSERT_NE(fds_drec_iter_next(&it), FDS_EOC);
    ASSERT_EQ(it.field.size, VALUE_IFC1.size());
    std::unique_ptr<char[]> str(new char[it.field.size]);
    ASSERT_EQ(fds_get_string(it.field.data, it.field.size, str.get()), FDS_OK);
    ASSERT_EQ(memcmp(str.get(), VALUE_IFC1.c_str(), it.field.size), 0);

    ASSERT_NE(fds_drec_iter_next(&it), FDS_EOC);
    uint64_t pkts;
    EXPECT_EQ(it.field.size, 4U);
    ASSERT_EQ(fds_get_uint_be(it.field.data, it.field.size, &pkts), FDS_OK);
    EXPECT_EQ(pkts, VALUE_PKTS);

    ASSERT_EQ(fds_drec_iter_next(&it), FDS_EOC);
}

void st_list::check258_v2(struct fds_drec &rec)
{
    EXPECT_EQ(rec.snap, tsnap);
    EXPECT_EQ(rec.size, drec258_v2.size());
    ASSERT_NE(rec.tmplt, nullptr);

    struct fds_drec_iter it;
    fds_drec_iter_init(&it, &rec, 0);

    ASSERT_NE(fds_drec_iter_next(&it), FDS_EOC);
    uint64_t bytes;
    ASSERT_EQ(fds_get_uint_be(it.field.data, it.field.size, &bytes), FDS_OK);
    EXPECT_EQ(bytes, VALUE_BYTES_R);

    ASSERT_NE(fds_drec_iter_next(&it), FDS_EOC);
    ASSERT_EQ(it.field.size, VALUE_IFC2.size());
    std::unique_ptr<char[]> str(new char[it.field.size]);
    ASSERT_EQ(fds_get_string(it.field.data, it.field.size, str.get()), FDS_OK);
    ASSERT_EQ(memcmp(str.get(), VALUE_IFC2.c_str(), it.field.size), 0);

    ASSERT_NE(fds_drec_iter_next(&it), FDS_EOC);
    uint64_t pkts;
    EXPECT_EQ(it.field.size, 4U);
    ASSERT_EQ(fds_get_uint_be(it.field.data, it.field.size, &pkts), FDS_OK);
    EXPECT_EQ(pkts, VALUE_PKTS_R);

    ASSERT_EQ(fds_drec_iter_next(&it), FDS_EOC);
}

/**
 * \brief Change size of allocated memory (useful for valgrind check)
 * \param mem  Original memory block (MUST be at least \p size bytes long)
 * \param size Size of the new block
 * \return Pointer to the new copy of the block with reduce size
 */
uint8_t* st_list::reduce_size(const uint8_t *mem, size_t size)
{
    uint8_t *data = reinterpret_cast<uint8_t *>(malloc(size * sizeof(uint8_t)));
    if (!data) {
        throw std::runtime_error("reduce_size() failed!");
    }

    memcpy(data, mem, size);
    return data;
}

/**
 * \brief Test iteration over field with zero length
 */
TEST_F(st_list, emptlyField)
{
    ipfix_stlist list;
    list_field.size = list.size();
    list_field.data = list.release();

    struct fds_stmlist_iter it;
    fds_stmlist_iter_init(&it, &list_field, tsnap, 0);
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_ERR_FORMAT);
    EXPECT_STRCASENE(fds_stmlist_iter_err(&it), OK_MSG);
}

/**
 * \brief Empty list
 */
TEST_F(st_list, empty)
{
    ipfix_stlist list;
    list.subTemp_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_ALL_OF, 256);

    list_field.size = list.size();
    list_field.data = list.release();

    struct fds_stlist_iter it;
    fds_stlist_iter_init(&it, &list_field, tsnap, 0);
    EXPECT_EQ(it.tid, 256U);
    EXPECT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_ALL_OF);
    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_EOC);
    EXPECT_STREQ(fds_stlist_iter_err(&it), OK_MSG);
    // Try again... the result should be the same
    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_EOC);
}

/**
 * \brief List with a single record
 */
TEST_F(st_list, single256)
{
    ipfix_stlist list;
    list.subTemp_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_ALL_OF, 256U);
    list.append_data_record(drec256);
    list_field.size = list.size();
    list_field.data = list.release();

    struct fds_stlist_iter it;
    fds_stlist_iter_init(&it, &list_field, tsnap, 0);
    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_OK);

    EXPECT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_ALL_OF);
    EXPECT_EQ(it.tid, 256U);
    check256(it.rec);

    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_EOC);
    // Try again... the result should be the same
    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_EOC);
    EXPECT_STREQ(fds_stlist_iter_err(&it), OK_MSG);
}

/**
 * \brief List with a single record
 */
TEST_F(st_list, single257)
{
    ipfix_stlist list;
    list.subTemp_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_NONE_OF, 257U);
    list.append_data_record(drec257);
    list_field.size = list.size();
    list_field.data = list.release();

    struct fds_stlist_iter it;
    fds_stlist_iter_init(&it, &list_field, tsnap, 0);
    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_OK);

    EXPECT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_NONE_OF);
    EXPECT_EQ(it.tid, 257U);
    check257(it.rec);

    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_EOC);
    EXPECT_STREQ(fds_stlist_iter_err(&it), OK_MSG);
}

/**
 * \brief Multiple occurrences of Data Record 257
 */
TEST_F(st_list, multi257)
{
    ipfix_stlist list;
    list.subTemp_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_ORDERED, 257U);
    list.append_data_record(drec257);
    list.append_data_record(drec257);
    list_field.size = list.size();
    list_field.data = list.release();

    struct fds_stlist_iter it;
    fds_stlist_iter_init(&it, &list_field, tsnap, 0);
    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_OK);
    EXPECT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_ORDERED);
    EXPECT_EQ(it.tid, 257U);
    check257(it.rec);
    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_OK);
    EXPECT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_ORDERED);
    EXPECT_EQ(it.tid, 257U);
    check257(it.rec);

    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_EOC);
    EXPECT_STREQ(fds_stlist_iter_err(&it), OK_MSG);
}

/**
 * \brief Multiple different occurrences of Data Record 258
 */
TEST_F(st_list, multi258)
{
    ipfix_stlist list;
    list.subTemp_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_UNDEFINED, 258U);
    list.append_data_record(drec258_v1);
    list.append_data_record(drec258_v2);
    list.append_data_record(drec258_v2);
    list.append_data_record(drec258_v1);
    list_field.size = list.size();
    list_field.data = list.release();

    struct fds_stlist_iter it;
    fds_stlist_iter_init(&it, &list_field, tsnap, 0);
    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_OK);
    EXPECT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_UNDEFINED);
    EXPECT_EQ(it.tid, 258U);
    check258_v1(it.rec);
    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_OK);
    EXPECT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_UNDEFINED);
    EXPECT_EQ(it.tid, 258U);
    check258_v2(it.rec);
    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_OK);
    EXPECT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_UNDEFINED);
    EXPECT_EQ(it.tid, 258U);
    check258_v2(it.rec);
    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_OK);
    EXPECT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_UNDEFINED);
    EXPECT_EQ(it.tid, 258U);
    check258_v1(it.rec);

    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_EOC);
    EXPECT_STREQ(fds_stlist_iter_err(&it), OK_MSG);
}

/**
 * \brief A missing template in the Template Snapshot
 */
TEST_F(st_list, missingTemplate)
{
    ipfix_stlist list;
    list.subTemp_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_UNDEFINED, 300U);
    list.append_data_record(drec256); // Just some data
    list_field.size = list.size();
    list_field.data = list.release();

    // Without the report flag
    struct fds_stlist_iter it;
    fds_stlist_iter_init(&it, &list_field, tsnap, 0);
    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_EOC);

    // With the report flag
    fds_stlist_iter_init(&it, &list_field, tsnap, FDS_STL_REPORT);
    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_ERR_NOTFOUND);
    // Try again... the result should be the same
    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_ERR_NOTFOUND);
}

/**
 * \brief Invalid Template ID (<256) used for a list
 */
TEST_F(st_list, invalidTemplateID)
{
    ipfix_stlist list;
    list.subTemp_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_ALL_OF, 255U);
    list.append_data_record(drec256); // Just some data
    list_field.size = list.size();
    list_field.data = list.release();

    // Without the report flag
    struct fds_stlist_iter it;
    fds_stlist_iter_init(&it, &list_field, tsnap, 0);
    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_ERR_FORMAT);

    // With the report flag
    fds_stlist_iter_init(&it, &list_field, tsnap, FDS_STL_REPORT);
    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_ERR_FORMAT); // Should be still format error
}

/**
 * \brief Malformed list header
 */
TEST_F(st_list, malformedHeader)
{
    ipfix_stlist list;
    list.subTemp_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_UNDEFINED, 258U);

    // Remove 1 bytes and re-allocate memory so valgrind can detect invalid access...
    uint16_t list_size = list.size() - 1;
    uint8_t *list_tmp = list.release();

    list_field.size = list_size;
    list_field.data = reduce_size(list_tmp, list_size);
    free(list_tmp);

    struct fds_stlist_iter it;
    fds_stlist_iter_init(&it, &list_field, tsnap, 0);
    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_ERR_FORMAT);
    EXPECT_STRCASENE(fds_stlist_iter_err(&it), OK_MSG);
}

/**
 * \brief A list with single record that is longer than the list itself
 */
TEST_F(st_list, malformedRecSingle)
{
    ipfix_stlist list;
    list.subTemp_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_UNDEFINED, 257U);
    list.append_data_record(drec257);

    // Remove 1 bytes and re-allocate memory so valgrind can detect invalid access...
    uint16_t list_size = list.size() - 1;
    uint8_t *list_tmp = list.release();

    list_field.size = list_size;
    list_field.data = reduce_size(list_tmp, list_size);
    free(list_tmp);

    struct fds_stlist_iter it;
    fds_stlist_iter_init(&it, &list_field, tsnap, 0);
    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_ERR_FORMAT);
    EXPECT_STRCASENE(fds_stlist_iter_err(&it), OK_MSG);
}

/**
 * \brief A list with multiple records where the last is longer than the list itself
 */
TEST_F(st_list, malformedRecLast)
{
    ipfix_stlist list;
    list.subTemp_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_ORDERED, 258U);
    list.append_data_record(drec258_v1);
    list.append_data_record(drec258_v2);

    // Remove 1 bytes and re-allocate memory so valgrind can detect invalid access...
    uint16_t list_size = list.size() - 1;
    uint8_t *list_tmp = list.release();

    list_field.size = list_size;
    list_field.data = reduce_size(list_tmp, list_size);
    free(list_tmp);

    struct fds_stlist_iter it;
    fds_stlist_iter_init(&it, &list_field, tsnap, 0);
    // The first record should be OK
    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_OK);
    check258_v1(it.rec);

    // The next one is malformed
    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_ERR_FORMAT);
    EXPECT_STRCASENE(fds_stlist_iter_err(&it), OK_MSG);

    // Try again... the result should be the same
    EXPECT_EQ(fds_stlist_iter_next(&it), FDS_ERR_FORMAT);
    EXPECT_STRCASENE(fds_stlist_iter_err(&it), OK_MSG);
}

/**
 * \brief A list with a single dynamic-length record that is always too long
 */
TEST_F(st_list, malformedDynamic)
{
    ipfix_stlist list;
    list.subTemp_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_ORDERED, 257U);
    list.append_data_record(drec257);
    uint16_t list_len = list.size();
    uint8_t *list_ptr = list.release();

    for (uint16_t i = 1U; i < drec257.size(); ++i) {
        // Try to check every possible combination of the too short list
        SCOPED_TRACE("Removed " + std::to_string(i) + " byte(s) from the list");
        list_field.size = list_len - i;
        list_field.data = reduce_size(list_ptr, list_field.size);

        struct fds_stlist_iter it;
        fds_stlist_iter_init(&it, &list_field, tsnap, 0);
        EXPECT_EQ(fds_stlist_iter_next(&it), FDS_ERR_FORMAT);
        EXPECT_STRCASENE(fds_stlist_iter_err(&it), OK_MSG);

        free(list_field.data);
    }

    free(list_ptr);
    list_field.data = nullptr;
}

// ------------------------------------------------------------------------------------------------

// Create just another class to distinguish test types
class stmulti_list : public st_list {};

/** Test iteration over field with zero length */
TEST_F(stmulti_list, emptlyField)
{
    ipfix_stlist list;
    list_field.size = list.size();
    list_field.data = list.release();

    struct fds_stmlist_iter it;
    fds_stmlist_iter_init(&it, &list_field, tsnap, 0);
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_ERR_FORMAT);
    EXPECT_STRCASENE(fds_stmlist_iter_err(&it), OK_MSG);
}

/**
 * List with one empty block
 */
TEST_F(stmulti_list, emptySingle)
{
    ipfix_stlist list;
    list.subTempMulti_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_ALL_OF);
    list.subTempMulti_data_hdr(256, 0); // Zero data length

    list_field.size = list.size();
    list_field.data = list.release();

    struct fds_stmlist_iter it;
    fds_stmlist_iter_init(&it, &list_field, tsnap, 0);
    EXPECT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_ALL_OF);
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_OK);
    EXPECT_EQ(it.tid, 256U);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);
    // Result should be still the same...
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_EOC);
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_EOC);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);
    EXPECT_STREQ(fds_stmlist_iter_err(&it), OK_MSG);
}

/**
 * \brief List with multiple block where each block is empty
 */
TEST_F(stmulti_list, emptyMulti)
{
    ipfix_stlist list;
    list.subTempMulti_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_ORDERED);
    list.subTempMulti_data_hdr(258, 0); // Zero data length
    list.subTempMulti_data_hdr(256, 0); // Zero data length
    list.subTempMulti_data_hdr(257, 0); // Zero data length
    list.subTempMulti_data_hdr(258, 0); // Zero data length

    list_field.size = list.size();
    list_field.data = list.release();

    struct fds_stmlist_iter it;
    fds_stmlist_iter_init(&it, &list_field, tsnap, 0);
    EXPECT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_ORDERED);
    // The first block
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_OK);
    EXPECT_EQ(it.tid, 258U);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);
    // The second block
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_OK);
    EXPECT_EQ(it.tid, 256U);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);
    // The third block
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_OK);
    EXPECT_EQ(it.tid, 257U);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);
    // The fourth block
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_OK);
    EXPECT_EQ(it.tid, 258U);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);

    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_EOC);
}

/**
 * \brief List with one block and one record
 */
TEST_F(stmulti_list, oneBlockWithOneRecord)
{
    ipfix_stlist list;
    list.subTempMulti_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_UNDEFINED);
    list.subTempMulti_data_hdr(257, drec257.size());
    list.append_data_record(drec257);
    list_field.size = list.size();
    list_field.data = list.release();

    struct fds_stmlist_iter it;
    fds_stmlist_iter_init(&it, &list_field, tsnap, 0);
    EXPECT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_UNDEFINED);
    // Get the block first
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_OK);
    EXPECT_EQ(it.tid, 257U);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_OK);
    check257(it.rec);

    // Next record should not be available
    EXPECT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);
    // Next block should not be available too
    EXPECT_EQ(fds_stmlist_iter_next_block(&it), FDS_EOC);
    // No error messages
    EXPECT_STREQ(fds_stmlist_iter_err(&it), OK_MSG);
}

/**
 * \brief List with one block and multiple records
 */
TEST_F(stmulti_list, oneBlockWithMultipleRecords)
{
    ipfix_stlist list;
    list.subTempMulti_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_EXACTLY_ONE_OF);
    list.subTempMulti_data_hdr(258, 2 * drec258_v1.size() + 2 * drec258_v2.size());
    list.append_data_record(drec258_v1);
    list.append_data_record(drec258_v2);
    list.append_data_record(drec258_v2);
    list.append_data_record(drec258_v1);
    list_field.size = list.size();
    list_field.data = list.release();

    struct fds_stmlist_iter it;
    fds_stmlist_iter_init(&it, &list_field, tsnap, 0);
    EXPECT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_EXACTLY_ONE_OF);
    // Get the block first
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_OK);
    EXPECT_EQ(it.tid, 258U);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_OK);
    check258_v1(it.rec);
    EXPECT_EQ(it.tid, 258U);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_OK);
    check258_v2(it.rec);
    EXPECT_EQ(it.tid, 258U);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_OK);
    check258_v2(it.rec);
    EXPECT_EQ(it.tid, 258U);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_OK);
    check258_v1(it.rec);

    // Next record should not be available
    EXPECT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);
    // Next block should not be available too
    EXPECT_EQ(fds_stmlist_iter_next_block(&it), FDS_EOC);
}

/**
 * List with mulitple blocks and different Template (all known)
 */
TEST_F(stmulti_list, multipleBlocksWithMultipleRecords)
{
    ipfix_stlist list;
    list.subTempMulti_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_NONE_OF);
    // 2 Data Records based on TID 256
    list.subTempMulti_data_hdr(256, 2 * drec256.size());
    list.append_data_record(drec256);
    list.append_data_record(drec256);
    // 0 Data Records based on TID 258
    list.subTempMulti_data_hdr(258, 0);
    // 3 Data Records based on TID 257
    list.subTempMulti_data_hdr(257, 3 * drec257.size());
    list.append_data_record(drec257);
    list.append_data_record(drec257);
    list.append_data_record(drec257);
    list_field.size = list.size();
    list_field.data = list.release();

    struct fds_stmlist_iter it;
    fds_stmlist_iter_init(&it, &list_field, tsnap, 0);
    EXPECT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_NONE_OF);

    // Get the first block
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_OK);
    EXPECT_EQ(it.tid, 256U);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_OK);
    check256(it.rec);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_OK);
    check256(it.rec);
    EXPECT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);

    // Get the second block
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_OK);
    EXPECT_EQ(it.tid, 258U);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);

    // Get the third block
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_OK);
    EXPECT_EQ(it.tid, 257U);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_OK);
    check257(it.rec);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_OK);
    check257(it.rec);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_OK);
    check257(it.rec);
    EXPECT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);

    // End of the list
    EXPECT_EQ(fds_stmlist_iter_next_block(&it), FDS_EOC);
}

/**
 * List with one block but its template is unknown
 */
TEST_F(stmulti_list, oneBlockWithMissingTemplate)
{
    const uint16_t unknown_tid = 280;

    ipfix_stlist list;
    list.subTempMulti_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_ONE_OR_MORE_OF);
    list.subTempMulti_data_hdr(unknown_tid, 3 * drec256.size()); // Some random Template ID
    list.append_data_record(drec256); // Some "dummy" data
    list.append_data_record(drec256);
    list.append_data_record(drec256);
    list_field.size = list.size();
    list_field.data = list.release();

    struct fds_stmlist_iter it;

    // Without the report flag
    fds_stmlist_iter_init(&it, &list_field, tsnap, 0);
    EXPECT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_ONE_OR_MORE_OF);
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_EOC); // The unknown block should be skipped
    // No error messages
    EXPECT_STREQ(fds_stmlist_iter_err(&it), OK_MSG);

    // With the report flag
    fds_stmlist_iter_init(&it, &list_field, tsnap, FDS_STL_REPORT);
    EXPECT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_ONE_OR_MORE_OF);
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_ERR_NOTFOUND);
    EXPECT_EQ(it.tid, unknown_tid);
    // The record iterator should return EOC
    EXPECT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);
    // Skip the block
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_EOC);
    EXPECT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);
}

/**
 * List with multiple blocks where all Templates are unknown
 */
TEST_F(stmulti_list, multipleBlocksWithAllTemplatesMissing)
{
    const uint16_t tid_missing1 = 587U;
    const uint16_t tid_missing2 = 65535U;
    const uint16_t tid_missing3 = 12345U;

    ipfix_stlist list;
    list.subTempMulti_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_ORDERED);
    list.subTempMulti_data_hdr(tid_missing1, 0); // First unknown block (empty)
    list.subTempMulti_data_hdr(tid_missing2, drec257.size()); // Second unknown block (+ dummy data)
    list.append_data_record(drec257);
    list.subTempMulti_data_hdr(tid_missing3, 0); // Third unknown block (empty)
    list_field.size = list.size();
    list_field.data = list.release();

    struct fds_stmlist_iter it;

    // Without the report flag
    fds_stmlist_iter_init(&it, &list_field, tsnap, 0);
    EXPECT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_ORDERED);
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_EOC); // All blocks should be skipped
    EXPECT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);
    // No error messages
    EXPECT_STREQ(fds_stmlist_iter_err(&it), OK_MSG);

    // With the report flag
    fds_stmlist_iter_init(&it, &list_field, tsnap, FDS_STL_REPORT);
    EXPECT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_ORDERED);
    // First unknown block
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_ERR_NOTFOUND);
    EXPECT_EQ(it.tid, tid_missing1);
    EXPECT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);
    // Second unknown block
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_ERR_NOTFOUND);
    EXPECT_EQ(it.tid, tid_missing2);
    EXPECT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);
    // Third unknown block
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_ERR_NOTFOUND);
    EXPECT_EQ(it.tid, tid_missing3);
    // No more blocks and no error messages
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_EOC);
    EXPECT_STREQ(fds_stmlist_iter_err(&it), OK_MSG);
}

/**
 * List with multiple blocks where only some Templates are known
 */
TEST_F(stmulti_list, multipleBlocksWithSomeMissingTemplates)
{
    const uint16_t tid_missing1 = 300U;
    const uint16_t tid_missing2 = 65000U;
    const uint16_t tid_missing3 = 2001U;
    const uint16_t tid_missing4 = 10000U;

    ipfix_stlist list;
    list.subTempMulti_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_ALL_OF);
    list.subTempMulti_data_hdr(tid_missing1, drec256.size()); // First unknown block (+ dummy data)
    list.append_data_record(drec256);
    list.subTempMulti_data_hdr(tid_missing2, drec257.size()); // Second unknown block (+ dummy data)
    list.append_data_record(drec257);
    list.subTempMulti_data_hdr(258, drec258_v1.size() + drec258_v2.size()); // Known Template ID
    list.append_data_record(drec258_v2);
    list.append_data_record(drec258_v1);
    list.subTempMulti_data_hdr(tid_missing3, drec258_v1.size()); // Third unknown block (+ dummy data)
    list.append_data_record(drec258_v1);
    list.subTempMulti_data_hdr(257, drec257.size()); // Known Template ID
    list.append_data_record(drec257);
    list.subTempMulti_data_hdr(tid_missing4, 0); // Fourth unknown block (empty)
    list_field.size = list.size();
    list_field.data = list.release();

    struct fds_stmlist_iter it;

    // Without the report flag (blocks with unknown Template ID should be automatically skipped)
    fds_stmlist_iter_init(&it , &list_field, tsnap, 0);
    EXPECT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_ALL_OF);
    // The first _known_ block
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_OK);
    EXPECT_EQ(it.tid, 258U);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_OK);
    check258_v2(it.rec);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_OK);
    check258_v1(it.rec);
    EXPECT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);
    // The second _known_ block
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_OK);
    EXPECT_EQ(it.tid, 257U);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_OK);
    check257(it.rec);
    EXPECT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);
    // No more blocks should be available + no error should be set
    EXPECT_EQ(fds_stmlist_iter_next_block(&it), FDS_EOC);
    EXPECT_STREQ(fds_stmlist_iter_err(&it), OK_MSG);

    // With the report flag (unknown Templates should be reported!)
    fds_stmlist_iter_init(&it , &list_field, tsnap, FDS_STL_REPORT);
    EXPECT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_ALL_OF);
    // Block 1 (unknown)
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_ERR_NOTFOUND);
    EXPECT_EQ(it.tid, tid_missing1);
    EXPECT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);
    // Block 2 (unknown)
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_ERR_NOTFOUND);
    EXPECT_EQ(it.tid, tid_missing2);
    // Block 3 (known)
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_OK);
    EXPECT_EQ(it.tid, 258U);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_OK);
    check258_v2(it.rec);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_OK);
    check258_v1(it.rec);
    EXPECT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);
    // Block 4 (unknown)
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_ERR_NOTFOUND);
    EXPECT_EQ(it.tid, tid_missing3);
    EXPECT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);
    // Block 5 (known)
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_OK);
    EXPECT_EQ(it.tid, 257U);
    // Block 6 (unknown)
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_ERR_NOTFOUND);
    EXPECT_EQ(it.tid, tid_missing4);
    EXPECT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);
    // No more blocks should be available + no error should be set
    EXPECT_EQ(fds_stmlist_iter_next_block(&it), FDS_EOC);
    EXPECT_STRCASEEQ(fds_stmlist_iter_err(&it), OK_MSG);
    EXPECT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);
}

/**
 * \brief Skipping to a next block without iteration over all records inside a current block
 *
 * Block and record functions should be independent.
 */
TEST_F(stmulti_list, skipBlocksWithoutGoingThroughAllRecords)
{
    ipfix_stlist list;
    list.subTempMulti_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_NONE_OF);
    list.subTempMulti_data_hdr(258, drec258_v1.size() + drec258_v2.size());
    list.append_data_record(drec258_v1);
    list.append_data_record(drec258_v2);
    list.subTempMulti_data_hdr(257, 3 * drec257.size());
    list.append_data_record(drec257);
    list.append_data_record(drec257);
    list.append_data_record(drec257);
    list.subTempMulti_data_hdr(258, 0); // Empty block
    list.subTempMulti_data_hdr(256, drec256.size());
    list.append_data_record(drec256);
    list_field.size = list.size();
    list_field.data = list.release();

    struct fds_stmlist_iter it;
    fds_stmlist_iter_init(&it , &list_field, tsnap, FDS_STL_REPORT);
    // Check if the block is here and immediately skip it
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_OK);
    EXPECT_EQ(it.tid, 258U);
    // Check first few records in the next block
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_OK);
    EXPECT_EQ(it.tid, 257U);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_OK);
    check257(it.rec);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_OK);
    check257(it.rec);
    // Don't check the last one and skip directly to the next block
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_OK);
    EXPECT_EQ(it.tid, 258U);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);
    // The last block
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_OK);
    EXPECT_EQ(it.tid, 256U);
    // Ignore this block content and continue
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_EOC);
}

/**
 * \brief One empty block with invalid Template ID (< 256)
 */
TEST_F(stmulti_list, malformedEmptyBlockWithInvalidTemplateID)
{
    ipfix_stlist list;
    list.subTempMulti_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_NONE_OF);
    list.subTempMulti_data_hdr(255, 0);
    list_field.size = list.size();
    list_field.data = list.release();

    struct fds_stmlist_iter it;
    fds_stmlist_iter_init(&it , &list_field, tsnap, 0);
    EXPECT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_NONE_OF);
    EXPECT_EQ(fds_stmlist_iter_next_block(&it), FDS_ERR_FORMAT);
    EXPECT_STRCASENE(fds_stmlist_iter_err(&it), OK_MSG);
}

/**
 * \brief One non-empty block with invalid Template ID (< 256)
 */
TEST_F(stmulti_list, malformedBlockWithInvalidTemplateID)
{
    ipfix_stlist list;
    list.subTempMulti_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_ALL_OF);
    list.subTempMulti_data_hdr(0, drec257.size()); // Template ID 0
    list.append_data_record(drec257);
    list_field.size = list.size();
    list_field.data = list.release();

    struct fds_stmlist_iter it;
    fds_stmlist_iter_init(&it , &list_field, tsnap, 0);
    EXPECT_EQ(fds_stmlist_iter_next_block(&it), FDS_ERR_FORMAT);
    EXPECT_STRCASENE(fds_stmlist_iter_err(&it), OK_MSG);
}

/**
 * \brief Too short header of a list
 */
TEST_F(stmulti_list, malformedListHeader)
{
    ipfix_stlist list;
    list.subTempMulti_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_ALL_OF);
    list.subTempMulti_data_hdr(256, 0);

    uint16_t list_len = list.size();
    uint8_t *list_ptr = list.release();

    for (uint16_t i = 1U; i < list_len - 1; ++i) {  // 1 byte list (i.e. semantic only) is valid
        // Try to check every possible combination of the too short header
        SCOPED_TRACE("Removed " + std::to_string(i) + " byte(s) from the header");
        list_field.size = list_len - i;
        list_field.data = reduce_size(list_ptr, list_field.size);

        struct fds_stmlist_iter it;
        fds_stmlist_iter_init(&it, &list_field, tsnap, 0);
        EXPECT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_ALL_OF);
        EXPECT_EQ(fds_stmlist_iter_next_block(&it), FDS_ERR_FORMAT);
        EXPECT_STRCASENE(fds_stmlist_iter_err(&it), OK_MSG);

        free(list_field.data);
    }

    free(list_ptr);
    list_field.data = nullptr;
}

/**
 * \brief List with two blocks and a record that exceeds size of the first block
 */
TEST_F(stmulti_list, malformedListTooShortBlock)
{
    for (uint16_t i = 1; i < drec257.size(); i++) {
        SCOPED_TRACE("Removed " + std::to_string(i) + " byte(s) from the block");
        // Create a new record
        ipfix_stlist list;
        list.subTempMulti_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_UNDEFINED);
        list.subTempMulti_data_hdr(257, drec257.size() - i); // Reduce size of the block
        list.append_data_record(drec257);
        list.subTempMulti_data_hdr(256, drec256.size());
        list_field.size = list.size();
        list_field.data = list.release();

        struct fds_stmlist_iter it;
        fds_stmlist_iter_init(&it, &list_field, tsnap, 0);
        ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_OK);
        // The record should be broken
        EXPECT_EQ(fds_stmlist_iter_next_rec(&it), FDS_ERR_FORMAT);
        EXPECT_STRCASENE(fds_stmlist_iter_err(&it), OK_MSG);

        // The next block should not be available because it can be malformed too
        EXPECT_NE(fds_stmlist_iter_next_block(&it), FDS_OK);
        EXPECT_NE(fds_stmlist_iter_next_rec(&it), FDS_OK);

        // Cleanup
        free(list_field.data);
    }

    list_field.data = nullptr;
}

/**
 * \brief List with one block which length is longer that the enclosing list
 */
TEST_F(stmulti_list, malformedListTooShortList)
{
    ipfix_stlist list;
    list.subTempMulti_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_ALL_OF);
    list.subTempMulti_data_hdr(257, drec257.size());
    list.append_data_record(drec257);

    uint16_t list_len = list.size();
    uint8_t *list_ptr = list.release();

    // Try all combination of too short list (missing record or missing header bytes)
    for (uint16_t i = 1; i < drec257.size() + FDS_IPFIX_SET_HDR_LEN; i++) {
        // Try to check every possible combination of the too short header
        SCOPED_TRACE("Removed " + std::to_string(i) + " byte(s) from the list");
        list_field.size = list_len - i;
        list_field.data = reduce_size(list_ptr, list_field.size);

        struct fds_stmlist_iter it;
        fds_stmlist_iter_init(&it, &list_field, tsnap, 0);
        ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_ERR_FORMAT);
        EXPECT_STRCASENE(fds_stmlist_iter_err(&it), OK_MSG);

        // No other actions are allowed
        EXPECT_EQ(fds_stmlist_iter_next_rec(&it), FDS_ERR_FORMAT);
        EXPECT_EQ(fds_stmlist_iter_next_block(&it), FDS_ERR_FORMAT);

        free(list_field.data);
    }

    free(list_ptr);
    list_field.data = nullptr;
}

/**
 * \brief List with invalid size of a block header (< 4B, don't forget to check RFC 6313 Errata!)
 */
TEST_F(stmulti_list, malformedListInvalidBlockSize)
{
    ipfix_stlist list;
    list.subTempMulti_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_ALL_OF);
    list.subTempMulti_data_hdr(258, drec258_v1.size()); // Size which will be changed
    list.append_data_record(drec258_v1); // Dummy data
    list_field.size = list.size();
    list_field.data = list.release();

    // Header must be at least 4 bytes long... always
    for (uint16_t i = 0; i < FDS_IPFIX_SET_HDR_LEN; ++i) {
        auto list_hdr = reinterpret_cast<struct fds_ipfix_stlist *>(list_field.data);
        auto block_hdr = reinterpret_cast<struct fds_ipfix_set_hdr *>(&list_hdr->template_id);
        block_hdr->length = htons(i);

        struct fds_stmlist_iter it;
        fds_stmlist_iter_init(&it, &list_field, tsnap, 0);
        ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_ERR_FORMAT);
        EXPECT_STRCASENE(fds_stmlist_iter_err(&it), OK_MSG);

        // No other actions are allowed
        EXPECT_EQ(fds_stmlist_iter_next_rec(&it), FDS_ERR_FORMAT);
        EXPECT_EQ(fds_stmlist_iter_next_block(&it), FDS_ERR_FORMAT);
    }
}

/**
 * \brief Calling next_block and next_rec in an invalid order
 */
TEST_F(stmulti_list, callNextRecordBeforeNextBlock)
{
    ipfix_stlist list;
    list.subTempMulti_header(fds_ipfix_list_semantics::FDS_IPFIX_LIST_NONE_OF);
    list.subTempMulti_data_hdr(256, 2 * drec256.size());
    list.append_data_record(drec256);
    list.append_data_record(drec256);
    list.subTempMulti_data_hdr(257, drec257.size());
    list.append_data_record(drec257);
    list_field.size = list.size();
    list_field.data = list.release();

    struct fds_stmlist_iter it;
    fds_stmlist_iter_init(&it, &list_field, tsnap, 0);
    // First call the next record and then the block
    EXPECT_NE(fds_stmlist_iter_next_rec(&it), FDS_OK);
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_OK);
    // Now everything should work as usual
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_OK);
    EXPECT_EQ(it.tid, 256);
    check256(it.rec);
    ASSERT_EQ(fds_stmlist_iter_next_rec(&it), FDS_OK);
    EXPECT_EQ(it.tid, 256);
    check256(it.rec);
    EXPECT_EQ(fds_stmlist_iter_next_rec(&it), FDS_EOC);
    // Skip the rest
    ASSERT_EQ(fds_stmlist_iter_next_block(&it), FDS_OK);
    EXPECT_EQ(it.tid, 257);
    EXPECT_EQ(fds_stmlist_iter_next_block(&it), FDS_EOC);
}
