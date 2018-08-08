//
// Created by jan on 2.8.18.
//

#include <gtest/gtest.h>
#include <libfds.h>
#include <MsgGen.h>
#include <chrono>
#include "../../../src/template_mgr/snapshot.h"

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class stlistIter : public ::testing::Test {
private:

protected:
    // Snapshot
    const fds_tsnapshot_t *snap;

    // Data records for creating lists
    ipfix_drec drec {};
    ipfix_drec drec2 {};

    ipfix_stlist subTempList;
    ipfix_stlist subTempMultiList;

    // Field for storing lists
    fds_drec_field field;
    fds_stlist_iter it;

    // Field info for subTemplateList and subTemplateMultiList
    fds_tfield subTempLst_info;
    fds_tfield subTempMultiLst_info;
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

    void SetUp() override {
        fds_tmgr_t *tmgr;
        tmgr = fds_tmgr_create(FDS_SESSION_UDP);
        if (!tmgr) {
            throw std::runtime_error("Failed to create the template manager");
        }

        // Prepare a templates
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

        uint16_t tmplt_size = trec.size();
        uint8_t *tmplt_raw = trec.release();
        struct fds_template *tmplt;
        ASSERT_EQ(fds_template_parse(FDS_TYPE_TEMPLATE, tmplt_raw, &tmplt_size, &tmplt), FDS_OK);
        free(tmplt_raw);

        uint16_t tmplt2_size = trec2.size();
        uint8_t *tmplt2_raw = trec2.release();
        struct fds_template *tmplt2;
        ASSERT_EQ(fds_template_parse(FDS_TYPE_TEMPLATE, tmplt2_raw, &tmplt2_size, &tmplt2), FDS_OK);
        free(tmplt2_raw);

        std::time_t curr_time = std::time(0);
        fds_tmgr_set_time(tmgr, static_cast<uint32_t >(curr_time));
        //Adding templates into manager
        ASSERT_EQ(fds_tmgr_template_add(tmgr, tmplt), FDS_OK);
        ASSERT_EQ(fds_tmgr_template_add(tmgr, tmplt2), FDS_OK);

        fds_tmgr_snapshot_get(tmgr, &snap);

        // Prepare a data record
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

        drec2.append_string(VALUE_APP_NAME);       // Adds variable head automatically (short version)
        drec2.var_header(VALUE_APP_DSC.length(), true); // Adds variable head manually (long version)
        drec2.append_string(VALUE_APP_DSC, VALUE_APP_DSC.length());
        drec2.append_uint(0, 5); // Padding
        drec2.append_uint(VALUE_BYTES, 8);
        drec2.append_uint(VALUE_PKTS, 8);
        drec2.append_float(VALUE_UNKNOWN, 4);
        drec2.append_uint(VALUE_BYTES_R, 8);
        drec2.append_uint(VALUE_PKTS_R, 8);
        drec2.var_header(VALUE_IFC1.length(), false); // empty string (only header)
        drec2.append_string(VALUE_IFC2);

        fds_iemgr_elem *def1 = (fds_iemgr_elem *) malloc(sizeof(fds_iemgr_elem));
        def1->data_type = FDS_ET_SUB_TEMPLATE_LIST;
        subTempLst_info.def = def1;

        fds_iemgr_elem *def2 = (fds_iemgr_elem *) malloc(sizeof(fds_iemgr_elem));
        def2->data_type = FDS_ET_SUB_TEMPLATE_MULTILIST;
        subTempMultiLst_info.def = def2;
    }

    void TearDown() override {
        free((void*)subTempLst_info.def);
        free((void*)subTempMultiLst_info.def);
    }
};

TEST_F(stlistIter, subTemplateList_init)
{
    subTempList.subTemp_header(0,256);
    subTempList.append_data_record(drec);

    field.size = subTempList.size();
    field.data = subTempList.release();
    field.info = (const fds_tfield *)&subTempLst_info;

    fds_stlist_iter_init(&it,&field,snap,FDS_STL_TIGNORE);
    ASSERT_EQ(it._private.err_code, FDS_OK);
    ASSERT_EQ(it._private.next_rec, field.data + 3U);
    ASSERT_EQ(it.semantic,0);
    ASSERT_EQ((uint8_t *)it._private.stlist, field.data);
    ASSERT_EQ(it._private.stlist_end, field.data + field.size);
    ASSERT_EQ(it._private.type, SUB_TMPLT_LIST_ID);
    ASSERT_EQ(it._private.flags, FDS_STL_TIGNORE);
}

TEST_F(stlistIter, subTemplateMultiList_init)
{
    subTempMultiList.subTempMulti_header(1);

    subTempMultiList.subTempMulti_data_hdr(256,drec.size());
    subTempMultiList.append_data_record(drec);

    subTempMultiList.subTempMulti_data_hdr(256,drec2.size());
    subTempMultiList.append_data_record(drec2);

    field.size = subTempMultiList.size();
    field.data = subTempMultiList.release();
    field.info = (const fds_tfield *)&subTempMultiLst_info;

    fds_stlist_iter_init(&it,&field,snap,FDS_STL_REPORT);
    ASSERT_EQ(it._private.err_code, FDS_OK);
    ASSERT_EQ(it._private.next_rec, field.data + 1U);
    ASSERT_EQ(it.semantic,1);
    ASSERT_EQ((uint8_t *)it._private.stlist, field.data);
    ASSERT_EQ(it._private.stlist_end, field.data + field.size);
    ASSERT_EQ(it._private.type, SUB_TMPLT_MULTI_LIST_ID);
    ASSERT_EQ(it._private.flags, FDS_STL_REPORT);
}

TEST_F(stlistIter, subTemplateList_first_record)
{
    subTempList.subTemp_header(0,256);
    subTempList.append_data_record(drec);
    subTempList.dump();

    field.size = subTempList.size();
    field.data = subTempList.release();
    field.info = (const fds_tfield *)&subTempLst_info;

    fds_stlist_iter_init(&it, &field, snap, FDS_STL_REPORT);

    int ret = fds_stlist_iter_next(&it);
    ASSERT_EQ(ret,FDS_OK);
    ASSERT_EQ(it.tid,256);
    uint16_t *src_port = reinterpret_cast<uint16_t *>(it.rec.data);
    ASSERT_EQ(ntohs(*src_port),VALUE_SRC_PORT);
    ASSERT_EQ(it._private.next_rec, it._private.stlist_end);


    ret = fds_stlist_iter_next(&it);
    ASSERT_EQ(ret,FDS_EOC);
}

TEST_F(stlistIter, subTemplateList_threeRecords)
{
    subTempList.subTemp_header(0,256);
    subTempList.append_data_record(drec2);
    subTempList.append_data_record(drec2);
    subTempList.append_data_record(drec2);
    subTempList.dump();

    field.size = subTempList.size();
    field.data = subTempList.release();
    field.info = (const fds_tfield *)&subTempLst_info;

    fds_stlist_iter_init(&it, &field, snap, FDS_STL_REPORT);

    for (int i=0; i<3; i++){
        int ret = fds_stlist_iter_next(&it);
        ASSERT_EQ(ret,FDS_OK);
        ASSERT_EQ(it.tid,256);
        uint16_t *src_port = reinterpret_cast<uint16_t *>(it.rec.data);
        ASSERT_EQ(ntohs(*src_port),VALUE_SRC_PORT);
        ASSERT_EQ(it._private.next_rec, it._private.stlist_end);
    }

    int ret = fds_stlist_iter_next(&it);
    ASSERT_EQ(ret,FDS_EOC);
}