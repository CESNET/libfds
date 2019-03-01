//
// Created by jan on 23.7.18.
//

#include <gtest/gtest.h>
#include <libfds.h>
#include <MsgGen.h>
#include <memory>

using set_uniq = std::unique_ptr<fds_ipfix_set_hdr, decltype(&free)>;

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class blistIter : public ::testing::Test {
private:
    // File with few IANA elements
    const char *ie_path = "data/iana.xml";

protected:
    // Expected error message when everything is OK
    const char *OK_MSG = "No error.";
    // IE manager
    fds_iemgr_t *ie_mgr = nullptr;

    // Prepared Data record
    struct fds_drec_field field_empty;
    struct fds_drec_field field_short_hdr;
    struct fds_drec_field field_long_hdr;
    struct fds_drec_field field_varlen_elems_short;
    struct fds_drec_field field_varlen_elems_long;

    struct fds_blist_iter it;

    // Different values and types for testing
    std::string VALUE_SRC_IP4_1  = "127.0.0.1";
    std::string VALUE_SRC_IP4_2  = "192.168.10.1";
    std::string VALUE_SRC_IP4_3  = "172.16.0.3";
    std::string VALUE_APP_NAME1 = "firefox";
    std::string VALUE_APP_NAME2 = "mozilla_esr";
    std::string VALUE_APP_NAME3 = ""; //empty string
    std::string VALUE_LINK_1 = "https://www.novinky.cz/domaci/478596-vystehovat-do-mesice-klienti-h-systemu-definitivne-prohrali.html";
    std::string VALUE_LINK_2 = "https://www.novinky.cz/domaci/478601-rozrezou-ho-a-odvezou-po-dne-nadrze-tezky-jerab-v-elektrarne-na-sumpersku-lezel-od-lonske-nehody.html";


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

        // Prepare fields with data
        ipfix_field fields;
        fields.append_ip(VALUE_SRC_IP4_1);
        fields.append_ip(VALUE_SRC_IP4_2);

        ipfix_field str_fields;
        str_fields.append_string(VALUE_APP_NAME1);
        str_fields.var_header(VALUE_APP_NAME2.length(), true);
        str_fields.append_string(VALUE_APP_NAME2, VALUE_APP_NAME2.length());
        str_fields.var_header(VALUE_APP_NAME3.length(), false);

        ipfix_field str_fields2;
        str_fields2.append_string(VALUE_LINK_1);
        str_fields2.var_header(VALUE_LINK_2.length(), true);
        str_fields2.append_string(VALUE_LINK_2, VALUE_LINK_2.length());

        //Prepare fields with basic lists, which will contain prepared data

        ipfix_blist blist_empty;
        // Semantic = 255, FieldID = 6 (TcpControlBits), Size of Element = 0
        blist_empty.header_short((int8_t )fds_ipfix_list_semantics::FDS_IPFIX_LIST_UNDEFINED, 6, 0);
        ipfix_field field_blist_empty;
        field_blist_empty.append_blist(blist_empty);

        ipfix_blist blist_short;
        // Semantic = 5, FieldID = 8 (SRC IPV4), Size of Element = 4B
        blist_short.header_short(fds_ipfix_list_semantics::FDS_IPFIX_LIST_ORDERED, 8, 4);
        blist_short.append_field(fields);
        ipfix_field field_short_blist;
        field_short_blist.append_blist(blist_short);

        ipfix_blist blist_long;
        // Add field to second list to make it different
        fields.append_ip(VALUE_SRC_IP4_3);
        // Semantic = 5, FieldID = 8 (SRC IPV4), Size of Element = 4B, Enterprise no. = 74
        blist_long.header_long(fds_ipfix_list_semantics::FDS_IPFIX_LIST_EXACTLY_ONE_OF, 8, 4, 74);
        blist_long.append_field(fields);
        ipfix_field field_long_blist;
        field_long_blist.append_blist(blist_long);

        ipfix_blist blist_varlen_short;
        // Semantic = 5, FieldID = 96 (ApplicationName), Size of Element = variable
        blist_varlen_short.header_short(fds_ipfix_list_semantics::FDS_IPFIX_LIST_ALL_OF, 96, FDS_IPFIX_VAR_IE_LEN);
        blist_varlen_short.append_field(str_fields);
        ipfix_field field_varlen_short_blist;
        field_varlen_short_blist.append_blist(blist_varlen_short);

        ipfix_blist blist_varlen_long;
        // Semantic = 3, FieldID = 94 (ApplicationDescription), Size of Element = variable
        blist_varlen_long.header_short(fds_ipfix_list_semantics::FDS_IPFIX_LIST_ALL_OF, 94, FDS_IPFIX_VAR_IE_LEN);
        blist_varlen_long.append_field(str_fields2);
        ipfix_field field_varlen_long_blist;
        field_varlen_long_blist.append_blist(blist_varlen_long);

        // Save size and data pointer in the structures
        field_empty.size = field_blist_empty.size();
        field_empty.data = field_blist_empty.release();

        field_short_hdr.size = field_short_blist.size();
        field_short_hdr.data = field_short_blist.release();

        field_long_hdr.size = field_long_blist.size();
        field_long_hdr.data = field_long_blist.release();

        field_varlen_elems_short.size = field_varlen_short_blist.size();
        field_varlen_elems_short.data = field_varlen_short_blist.release();

        field_varlen_elems_long.size = field_varlen_long_blist.size();
        field_varlen_elems_long.data = field_varlen_long_blist.release();
    }

    /** \brief After each test */
    void TearDown() override {
        // free allocated data here
        fds_iemgr_destroy(ie_mgr);
        free(field_empty.data);
        free(field_varlen_elems_long.data);
        free(field_varlen_elems_short.data);
        free(field_long_hdr.data);
        free(field_short_hdr.data);
    }
};

// ITERATOR -------------------------------------------------------------------------------------

/*
 * Zero length field
 */
TEST_F(blistIter, emptyField)
{
    std::unique_ptr<uint8_t []> random(new uint8_t[10]);
    struct fds_drec_field field;
    field.size = 0;
    field.data = random.get();

    fds_blist_iter_init(&it, &field, ie_mgr);
    EXPECT_EQ(fds_blist_iter_next(&it), FDS_ERR_FORMAT);
    EXPECT_STRCASENE(fds_blist_iter_err(&it), OK_MSG);
}

/*
 * Empty basicList
 */
TEST_F(blistIter, init_empty_blist)
{
    fds_blist_iter_init(&it, &field_empty, ie_mgr);
    ASSERT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_UNDEFINED);
    ASSERT_EQ(it.field.info->id, 6);
    ASSERT_EQ(it.field.info->length, 0);
    ASSERT_EQ((uint8_t*)it._private.field_next, (uint8_t*)it._private.blist_end);
}

/*
 * basicList with 2 IPv4 addresses (static size)
 */
TEST_F(blistIter, init_short_hdr)
{
    fds_blist_iter_init(&it, &field_short_hdr, ie_mgr);
    ASSERT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_ORDERED);
    ASSERT_EQ(it.field.info->id, 8);
    ASSERT_EQ(it.field.info->length, 4);
    ASSERT_EQ((uint8_t*)it._private.field_next, (uint8_t*)it._private.blist + FDS_IPFIX_BLIST_SHORT_HDR_LEN);
}

/*
 * basicList with 3 IPv4 address (static size) and non-zero Enterprise Number
 */
TEST_F(blistIter, init_long_hdr)
{
    fds_blist_iter_init(&it, &field_long_hdr, ie_mgr);
    ASSERT_EQ(it.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_EXACTLY_ONE_OF);
    ASSERT_EQ(it.field.info->id, 8);
    ASSERT_EQ(it.field.info->length, 4);
    ASSERT_EQ((uint8_t*)it._private.field_next, (uint8_t*)it._private.blist + FDS_IPFIX_BLIST_LONG_HDR_LEN);
    ASSERT_EQ(it.field.info->en, 74U);
}

/*
 * Iterate over empty basicList
 */
TEST_F(blistIter, next_empty_blist)
{
    fds_blist_iter_init(&it, &field_empty, ie_mgr);
    int ret = fds_blist_iter_next(&it);
    ASSERT_EQ(ret, FDS_EOC);
    EXPECT_STRCASEEQ(fds_blist_iter_err(&it), OK_MSG);
    ASSERT_EQ(it._private.err_code, FDS_EOC);
    ASSERT_EQ((uint8_t*)it._private.field_next, (uint8_t*)it._private.blist_end);
    ASSERT_EQ(it.field.info->offset, 0);
    ASSERT_EQ(it.field.info->length, 0);
}

/*
 * Iterate over basicList with 2 IPv4 addresses (static size)
 */
TEST_F(blistIter, next_short_hdr)
{
    char out[20];
    int ret;
    fds_blist_iter_init(&it, &field_short_hdr, ie_mgr);
    // First field in list
    ret = fds_blist_iter_next(&it);
    ASSERT_EQ(ret, FDS_OK);
    ASSERT_EQ(it.field.size, 4);
    ASSERT_EQ((uint8_t*)it.field.data, (uint8_t*)it._private.blist + FDS_IPFIX_BLIST_SHORT_HDR_LEN);
    fds_ip2str(it.field.data, it.field.size, &out[0], 20);
    ASSERT_STREQ(out, VALUE_SRC_IP4_1.c_str());
    // Second field in list
    ret = fds_blist_iter_next(&it);
    ASSERT_EQ(ret, FDS_OK);
    ASSERT_EQ(it.field.size, 4);
    ASSERT_EQ((uint8_t*)it.field.data, (uint8_t*)it._private.blist + FDS_IPFIX_BLIST_SHORT_HDR_LEN + 4);
    fds_ip2str(it.field.data, it.field.size, &out[0], 20);
    ASSERT_STREQ(out, VALUE_SRC_IP4_2.c_str());
    // Testing end pointer and return code
    ASSERT_EQ(it._private.field_next, it._private.blist_end);
    ret = fds_blist_iter_next(&it);
    ASSERT_EQ(ret, FDS_EOC);
    EXPECT_STRCASEEQ(fds_blist_iter_err(&it), OK_MSG);
}

/*
 * Iterate over basicList with 3 IPv4 address (static size) and non-zero Enterprise Number
 */
TEST_F(blistIter, next_long_hdr)
{
    char out[20];
    int ret;

    fds_blist_iter_init(&it, &field_long_hdr, ie_mgr);
    // First field in list
    ret = fds_blist_iter_next(&it);
    fds_ip2str(it.field.data, it.field.size, &out[0], 20);
    ASSERT_EQ(ret, FDS_OK);
    ASSERT_EQ(it.field.size, 4);
    ASSERT_EQ((uint8_t*)it.field.data, (uint8_t*)it._private.blist + FDS_IPFIX_BLIST_LONG_HDR_LEN);
    ASSERT_STREQ(out, VALUE_SRC_IP4_1.c_str());
    // Second field in list
    ret = fds_blist_iter_next(&it);
    fds_ip2str(it.field.data, it.field.size, &out[0], 20);
    ASSERT_EQ(ret, FDS_OK);
    ASSERT_EQ(it.field.size, 4);
    ASSERT_EQ((uint8_t*)it.field.data, (uint8_t*)it._private.blist + FDS_IPFIX_BLIST_LONG_HDR_LEN + 4U);
    ASSERT_STREQ(out, VALUE_SRC_IP4_2.c_str());
    // Third field in list
    ret = fds_blist_iter_next(&it);
    fds_ip2str(it.field.data, it.field.size, &out[0], 20);
    ASSERT_EQ(ret, FDS_OK);
    ASSERT_EQ(it.field.size, 4);
    ASSERT_EQ((uint8_t*)it.field.data, (uint8_t*)it._private.blist + FDS_IPFIX_BLIST_LONG_HDR_LEN + 8U);
    ASSERT_STREQ(out, VALUE_SRC_IP4_3.c_str());
    ASSERT_EQ(it._private.field_next, it._private.blist_end); // testing end pointer and return code

    ret = fds_blist_iter_next(&it);
    ASSERT_EQ(ret, FDS_EOC);
    EXPECT_STRCASEEQ(fds_blist_iter_err(&it), OK_MSG);
}

/*
 * basicList with 3 variable-length strings (application names)
 */
TEST_F(blistIter, next_varlen_data_short)
{
    char out[15];

    fds_blist_iter_init(&it, &field_varlen_elems_short, ie_mgr);
    ASSERT_EQ(it.field.info->length,FDS_IPFIX_VAR_IE_LEN);

    // Short var-length header
    int ret = fds_blist_iter_next(&it);
    fds_string2str(it.field.data, it.field.size, &out[0], 15);
    ASSERT_EQ(ret, FDS_OK);
    ASSERT_EQ(it.field.size, 7U); //First field is 7 Bytes long
    ASSERT_STREQ(out, VALUE_APP_NAME1.c_str());

    // Long var-length header
    ret = fds_blist_iter_next(&it);
    fds_string2str(it.field.data, it.field.size, &out[0], 15);
    ASSERT_EQ(ret, FDS_OK);
    ASSERT_EQ(it.field.size, 11U); //Second field is 11 Bytes long
    ASSERT_STREQ(out, VALUE_APP_NAME2.c_str());

    // Short var-length header with zero size field
    ret = fds_blist_iter_next(&it);
    fds_string2str(it.field.data, it.field.size, &out[0], 15);
    ASSERT_EQ(ret, FDS_OK);
    ASSERT_EQ(it.field.size,0); //First field is 0 Bytes long
    ASSERT_STREQ(out, VALUE_APP_NAME3.c_str());

    ret = fds_blist_iter_next(&it);
    ASSERT_EQ(ret, FDS_EOC);
    EXPECT_STRCASEEQ(fds_blist_iter_err(&it), OK_MSG);
}

/*
 * basicList with 2 variable-length strings
 */
TEST_F(blistIter, next_varlen_data_long)
{
    char out[200];

    fds_blist_iter_init(&it,&field_varlen_elems_long,ie_mgr);

    int ret = fds_blist_iter_next(&it);
    fds_string2str(it.field.data, it.field.size, &out[0], 200);
    ASSERT_EQ(ret,FDS_OK);
    ASSERT_EQ(it.field.size,VALUE_LINK_1.length());
    ASSERT_STREQ(out, VALUE_LINK_1.c_str());

    ret = fds_blist_iter_next(&it);
    fds_string2str(it.field.data, it.field.size, &out[0], 200);
    ASSERT_EQ(ret,FDS_OK);
    ASSERT_EQ(it.field.size,VALUE_LINK_2.length());
    ASSERT_STREQ(out, VALUE_LINK_2.c_str());

    ret = fds_blist_iter_next(&it);
    ASSERT_EQ(ret, FDS_EOC);
    EXPECT_STRCASEEQ(fds_blist_iter_err(&it), OK_MSG);
}

/*
 * Header with constant size but data with variable size
 */
TEST_F(blistIter, malformed_field_short_hdr )
{
    struct fds_drec_field malformed;
    malformed.data = (uint8_t*) malloc(field_short_hdr.size -2 + FDS_IPFIX_BLIST_SHORT_HDR_LEN);
    memcpy( malformed.data, field_short_hdr.data, FDS_IPFIX_BLIST_SHORT_HDR_LEN);
    memcpy( malformed.data+FDS_IPFIX_BLIST_SHORT_HDR_LEN,
            field_varlen_elems_short.data+FDS_IPFIX_BLIST_SHORT_HDR_LEN,
            field_short_hdr.size - 2);
    malformed.size = static_cast<uint16_t>(field_short_hdr.size - 3);

    int ret;
    fds_blist_iter_init(&it, &malformed, ie_mgr);
    ASSERT_EQ(it._private.err_code,FDS_OK);

    fds_blist_iter_next(&it);
    ret = fds_blist_iter_next(&it);
    ASSERT_EQ(ret, FDS_ERR_FORMAT);
    EXPECT_STRCASENE(fds_blist_iter_err(&it), OK_MSG);
    free(malformed.data);
}

TEST_F(blistIter, malformed_field_varlen)
{
    field_varlen_elems_long.size -= 150U;
    ASSERT_GT(field_varlen_elems_long.size, 0);

    fds_blist_iter_init(&it,&field_varlen_elems_long,ie_mgr);

    int ret = fds_blist_iter_next(&it);
    ASSERT_EQ(ret,FDS_ERR_FORMAT);
    EXPECT_STRCASENE(fds_blist_iter_err(&it), OK_MSG);
}

TEST_F(blistIter, malformed_sizehdr_no_data)
{
    // Create only 3 bytes var-length specification without data
    ipfix_field str_fields;
    str_fields.var_header(VALUE_APP_NAME2.length(), true);

    ipfix_blist blist_varlen_short;
    // Semantic = 3, FieldID = 96 (ApplicationName), Size of Element = variable
    blist_varlen_short.header_short(fds_ipfix_list_semantics::FDS_IPFIX_LIST_ALL_OF, 96, FDS_IPFIX_VAR_IE_LEN);
    blist_varlen_short.append_field(str_fields);
    ipfix_field field_varlen_short_blist;
    field_varlen_short_blist.append_blist(blist_varlen_short);

    struct fds_drec_field malformed;
    malformed.size = field_varlen_short_blist.size();
    malformed.data = field_varlen_short_blist.release();

    fds_blist_iter_init(&it,&malformed,ie_mgr);
    int ret = fds_blist_iter_next(&it);
    //std::cout<<fds_blist_iter_err(&it)<<std::endl;
    ASSERT_EQ(ret, FDS_ERR_FORMAT);
    EXPECT_STRCASENE(fds_blist_iter_err(&it), OK_MSG);

    // Reduce size of the var-length header to 1 byte
    malformed.size -=2U;

    fds_blist_iter_init(&it,&malformed,ie_mgr);
    ret = fds_blist_iter_next(&it);
    //std::cout<<fds_blist_iter_err(&it)<<std::endl;
    ASSERT_EQ(ret, FDS_ERR_FORMAT);
    EXPECT_STRCASENE(fds_blist_iter_err(&it), OK_MSG);

    free(malformed.data);
}

/*
 * A non-empty basicList with zero-size fields (can cause infinite iteration)
 */
TEST_F(blistIter, malformed_zero_size_fields)
{
    // Create a dummy content
    ipfix_field list_fields;
    list_fields.append_uint(0, 8);

    ipfix_blist list;
    list.header_short(fds_ipfix_list_semantics::FDS_IPFIX_LIST_UNDEFINED, 8, 0);
    list.append_field(list_fields); // Dummy content
    ipfix_field field_blist;
    field_blist.append_blist(list);

    struct fds_drec_field malformed;
    malformed.size = field_blist.size();
    malformed.data = field_blist.release();

    fds_blist_iter_init(&it, &malformed, ie_mgr);
    ASSERT_EQ(fds_blist_iter_next(&it), FDS_ERR_FORMAT);
    EXPECT_STRCASENE(fds_blist_iter_err(&it), OK_MSG);
    //std::cout<<fds_blist_iter_err(&it)<<std::endl;
    free(malformed.data);
}