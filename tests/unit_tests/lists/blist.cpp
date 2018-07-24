//
// Created by jan on 23.7.18.
//

#include <gtest/gtest.h>
#include <libfds.h>
#include <MsgGen.h>

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
    // IE manager
    fds_iemgr_t *ie_mgr = nullptr;

    // Prepared Data record
    struct fds_drec_field field_short;
    struct fds_drec_field field_long;

    struct fds_blist_iter it_short;
    struct fds_blist_iter it_long;

    // Different values and types for testing
//    uint16_t    VALUE_SRC_PORT = 65000;
    std::string VALUE_SRC_IP4_1  = "127.0.0.1";
    std::string VALUE_SRC_IP4_2  = "192.168.10.1";
    std::string VALUE_SRC_IP4_3  = "172.16.0.3";
//    uint16_t    VALUE_DST_PORT = 80;
//    std::string VALUE_DST_IP4  = "8.8.8.8";
//    uint8_t     VALUE_PROTO    = 6; // TCP
//    uint64_t    VALUE_TS_FST   = 1522670362000ULL;
//    uint64_t    VALUE_TS_LST   = 1522670372999ULL;
//    uint64_t    VALUE_TS_FST_R = 1522670363123ULL;
//    uint64_t    VALUE_TS_LST_R = 1522670369000ULL;
    std::string VALUE_APP_NAME1 = "firefox";
    std::string VALUE_APP_NAME2 = "mozilla";
    std::string VALUE_APP_NAME3 = ""; //empty string
    std::string VALUE_LINK_1 = "https://www.novinky.cz/domaci/478596-vystehovat-do-mesice-klienti-h-systemu-definitivne-prohrali.html";
    std::string VALUE_LINK_2 = "https://www.novinky.cz/domaci/478601-rozrezou-ho-a-odvezou-po-dne-nadrze-tezky-jerab-v-elektrarne-na-sumpersku-lezel-od-lonske-nehody.html";
//    std::string VALUE_APP_DSC  = "linux/web browser";
//    uint64_t    VALUE_BYTES    = 1234567;
//    uint64_t    VALUE_PKTS     = 12345;
//    double      VALUE_UNKNOWN  = 3.1416f;
//    uint64_t    VALUE_BYTES_R  = 7654321;
//    uint64_t    VALUE_PKTS_R   = 54321;
//    std::string VALUE_IFC1     = ""; // empty string
//    std::string VALUE_IFC2     = "eth0";

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

        // Prepare fields with IP adresses - constant size 4B
        ipfix_field fields;
        fields.append_ip(VALUE_SRC_IP4_1);
        fields.append_ip(VALUE_SRC_IP4_2);

        // Prepare fields with variable size
        ipfix_field str_fields;
        str_fields.append_string(VALUE_APP_NAME1);
        str_fields.var_header(VALUE_APP_NAME2.length(), true);
        str_fields.append_string(VALUE_APP_NAME2,VALUE_APP_NAME2.length());
        str_fields.var_header(VALUE_APP_NAME3.length(),false);

        ipfix_blist blist_short;
        ipfix_blist blist_long;

        // Semantic = 5, FieldID = 8 (SRC IPV4), Size of Element = 4B
        blist_short.header_short(fds_ipfix_list_semantics::FDS_IPFIX_LIST_ORDERED, 8, 4);
        blist_short.append_field(fields);

        // Add field to second list to make it different
        fields.append_ip(VALUE_SRC_IP4_3);

        // Semantic = 5, FieldID = 8 (SRC IPV4), Size of Element = 4B, Enterprise no. = 74
        blist_long.header_long(fds_ipfix_list_semantics::FDS_IPFIX_LIST_EXACTLY_ONE_OF, 8, 4, 74);
        blist_long.append_field(fields);

        // Prepare a field with basic list.
        ipfix_field field_short_blist;
        field_short_blist.append_blist(blist_short);

        ipfix_field field_long_blist;
        field_long_blist.append_blist(blist_long);

        // Save additional info in the structure
        field_short.size = field_short_blist.size();
        field_short.data = field_short_blist.release();

        field_long.size = field_long_blist.size();
        field_long.data = field_long_blist.release();



    }

    /** \brief After each test */
    void TearDown() override {
        //free(field_short.data);
        //free(field_long.data);
        fds_iemgr_destroy(ie_mgr);
    }
};

// ITERATOR -------------------------------------------------------------------------------------
// Init iterator of blist with short header
TEST_F(blistIter, init_short)
{
    fds_blist_iter_init(&it_short, &field_short, ie_mgr);
    ASSERT_EQ(it_short.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_ORDERED);
    ASSERT_EQ(it_short._private.info.id,8);
    ASSERT_EQ(it_short._private.info.length, 4);
    ASSERT_EQ((void*)it_short._private.field_next, (void*)it_short._private.blist + FDS_IPFIX_BLIST_HDR_SHORT);
}

TEST_F(blistIter, init_long)
{
    fds_blist_iter_init(&it_long, &field_long, ie_mgr);
    ASSERT_EQ(it_long.semantic, fds_ipfix_list_semantics::FDS_IPFIX_LIST_EXACTLY_ONE_OF);
    ASSERT_EQ(it_long._private.info.id,8);
    ASSERT_EQ(it_long._private.info.length, 4);
    ASSERT_EQ((void*)it_long._private.field_next, (void*)it_long._private.blist + FDS_IPFIX_BLIST_HDR_LONG);
    ASSERT_EQ(it_long._private.info.en,74);
}

TEST_F(blistIter, iter_next_short)
{
    char out[20];
    int ret;
    fds_blist_iter_init(&it_short, &field_short, ie_mgr);
    // First field in list
    ret = fds_blist_iter_next(&it_short);
    ASSERT_EQ(ret,FDS_OK);
    ASSERT_EQ(it_short.field.size,4);
    ASSERT_EQ((void*)it_short.field.data, (void*)it_short._private.blist + FDS_IPFIX_BLIST_HDR_SHORT);
    fds_ip2str(it_short.field.data,it_short.field.size,&out[0],20);
    ASSERT_STREQ(out,VALUE_SRC_IP4_1.c_str());
    // Second field in list
    ret = fds_blist_iter_next(&it_short);
    ASSERT_EQ(ret,FDS_OK);
    ASSERT_EQ(it_short.field.size,4);
    ASSERT_EQ((void*)it_short.field.data, (void*)it_short._private.blist + FDS_IPFIX_BLIST_HDR_SHORT + 4);
    fds_ip2str(it_short.field.data,it_short.field.size,&out[0],20);
    ASSERT_STREQ(out,VALUE_SRC_IP4_2.c_str());
    // Testing end pointer and return code
    ASSERT_EQ(it_short._private.field_next,it_short._private.blist_end);
    ret = fds_blist_iter_next(&it_short);
    ASSERT_EQ(ret,FDS_EOC);
}

TEST_F(blistIter, iter_next_long)
{
    char out[20];
    int ret;

    fds_blist_iter_init(&it_long, &field_long, ie_mgr);
    // First field in list
    ret = fds_blist_iter_next(&it_long);
    ASSERT_EQ(ret,FDS_OK);
    ASSERT_EQ(it_long.field.size,4);
    ASSERT_EQ((void*)it_long.field.data, (void*)it_long._private.blist + FDS_IPFIX_BLIST_HDR_LONG);
    fds_ip2str(it_long.field.data,it_long.field.size,&out[0],20);
    ASSERT_STREQ(out,VALUE_SRC_IP4_1.c_str());
    // Second field in list
    ret = fds_blist_iter_next(&it_long);
    ASSERT_EQ(ret,FDS_OK);
    ASSERT_EQ(it_long.field.size,4);
    ASSERT_EQ((void*)it_long.field.data, (void*)it_long._private.blist + FDS_IPFIX_BLIST_HDR_LONG + 4U);
    fds_ip2str(it_long.field.data,it_long.field.size,&out[0],20);
    ASSERT_STREQ(out,VALUE_SRC_IP4_2.c_str());
    // Third field in list
    ret = fds_blist_iter_next(&it_long);
    std::cout << fds_blist_iter_err(&it_long)<<std::endl;
    ASSERT_EQ(ret,FDS_OK);
    ASSERT_EQ(it_long.field.size,4);
    ASSERT_EQ((void*)it_long.field.data, (void*)it_long._private.blist + FDS_IPFIX_BLIST_HDR_LONG + 8U);
    fds_ip2str(it_long.field.data,it_long.field.size,&out[0],20);
    ASSERT_STREQ(out,VALUE_SRC_IP4_3.c_str());
    // testing end pointer and return code
    ASSERT_EQ(it_long._private.field_next,it_long._private.blist_end);
    ret = fds_blist_iter_next(&it_long);
    ASSERT_EQ(ret,FDS_EOC);
}


