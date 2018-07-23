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

    // Different values and types for testing
//    uint16_t    VALUE_SRC_PORT = 65000;
    std::string VALUE_SRC_IP4  = "127.0.0.1";
//    uint16_t    VALUE_DST_PORT = 80;
//    std::string VALUE_DST_IP4  = "8.8.8.8";
//    uint8_t     VALUE_PROTO    = 6; // TCP
//    uint64_t    VALUE_TS_FST   = 1522670362000ULL;
//    uint64_t    VALUE_TS_LST   = 1522670372999ULL;
//    uint64_t    VALUE_TS_FST_R = 1522670363123ULL;
//    uint64_t    VALUE_TS_LST_R = 1522670369000ULL;
//    std::string VALUE_APP_NAME = "firefox";
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

        // Prepare field which will be used in basic list
        ipfix_field field;
        field.append_ip(VALUE_SRC_IP4);

        //Here crete class for blist and set some fields inside
        ipfix_blist blist_short;
        ipfix_blist blist_long;

        blist_short.header_short(0, 8, FDS_IPFIX_BLIST_HDR_SHORT + 8 );
        blist_short.append_field(field);
        blist_short.append_field(field);

        blist_long.header_long(0, 8, FDS_IPFIX_BLIST_HDR_LONG + 12 , 74);
        blist_long.append_field(field);
        blist_long.append_field(field);
        blist_long.append_field(field);
        blist_long.dump();


        // Prepare a field with basic list.
        ipfix_field field_short_blist;
        field_short_blist.append_blist(blist_short);

        ipfix_field field_long_blist;
        field_long_blist.append_blist(blist_long);

        //field_short_blist.dump();
        field_short.size = field_short_blist.size();
        field_short.data = field_short_blist.release();

        //field_long_blist.dump();
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
TEST_F(blistIter, init)
{
    struct fds_blist_iter it;
    // Some random fields...
    int res = fds_blist_iter_init(&it, &field_short, ie_mgr);
    ASSERT_EQ(res, FDS_OK);
}


