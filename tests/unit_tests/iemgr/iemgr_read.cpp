/**
 * \author Michal Režňák
 * \date   8. August 2017
 */

#include <gtest/gtest.h>
#include <libfds/iemgr.h>
#include "iemgr_common.h"

TEST_F(Mgr, null)
{
    EXPECT_EQ(fds_iemgr_read_file(mgr, nullptr, true), FDS_IEMGR_ERR);
    EXPECT_ERROR;
    EXPECT_EQ(fds_iemgr_read_file(nullptr, "test", true), FDS_IEMGR_ERR);
    EXPECT_EQ(fds_iemgr_read_file(nullptr, nullptr, true), FDS_IEMGR_ERR);

    EXPECT_EQ(fds_iemgr_read_dir(mgr, nullptr), FDS_IEMGR_ERR);
    EXPECT_ERROR;
    EXPECT_EQ(fds_iemgr_read_dir(nullptr, "test"), FDS_IEMGR_ERR);
    EXPECT_EQ(fds_iemgr_read_dir(nullptr, nullptr), FDS_IEMGR_ERR);
}

TEST_F(Mgr, file_empty)
{
    EXPECT_NE(fds_iemgr_read_file(mgr, FILES_VALID "empty.xml", true), FDS_IEMGR_OK);
    EXPECT_ERROR;
}

TEST_F(Mgr, file_individual)
{
    EXPECT_EQ(fds_iemgr_read_file(mgr, FILES_VALID "individual.xml", true), FDS_IEMGR_OK);

    for (uint16_t i = 0; i < 60; ++i) {
        auto elem = fds_iemgr_elem_find_id(mgr, 0, i);
        if (elem == nullptr) {
            continue;
        }

        if (!elem->is_reverse) {
            auto rev = fds_iemgr_elem_find_id(mgr, 0, i+40);
            ASSERT_NE(rev, nullptr);
            EXPECT_TRUE(rev->is_reverse);
            EXPECT_EQ(rev->reverse_elem, elem);
            EXPECT_STREQ(rev->name, (std::string(elem->name) + "@reverse").c_str());
        }
    }
}

TEST_F(Mgr, file_pen)
{
    EXPECT_EQ(fds_iemgr_read_file(mgr, FILES_VALID "pen.xml", true), FDS_IEMGR_OK);

    for (uint16_t i = 0; i < 20; ++i) {
        auto elem = fds_iemgr_elem_find_id(mgr, 0, i);
        if (elem == nullptr) {
            continue;
        }

        auto rev = fds_iemgr_elem_find_id(mgr, 1, i);
        ASSERT_NE(rev, nullptr);
        EXPECT_TRUE(rev->is_reverse);
        EXPECT_EQ(rev->reverse_elem, elem);
        EXPECT_STREQ(rev->name, (std::string(elem->name) + "@reverse").c_str());
    }
}

TEST_F(Mgr, file_split)
{
    EXPECT_EQ(fds_iemgr_read_file(mgr, FILES_VALID "split.xml", true), FDS_IEMGR_OK);

    for (uint16_t i = 0; i < 20; i+=2) {
        auto elem = fds_iemgr_elem_find_id(mgr, 0, i);
        if (elem == nullptr) {
            continue;
        }

        auto rev = fds_iemgr_elem_find_id(mgr, 0, i+1);
        ASSERT_NE(rev, nullptr);
        EXPECT_TRUE(rev->is_reverse);
        EXPECT_EQ(rev->reverse_elem, elem);
        EXPECT_STREQ(rev->name, (std::string(elem->name) + "@reverse").c_str());
    }
}

TEST_F(Mgr, file_overwrite_with_same)
{
    EXPECT_EQ(fds_iemgr_read_file(mgr, FILES_VALID "pen.xml", true), FDS_IEMGR_OK);
    EXPECT_NO_ERROR;

    EXPECT_EQ(fds_iemgr_read_file(mgr, FILES_VALID "pen.xml", true), FDS_IEMGR_OK);
    EXPECT_NO_ERROR;
}

TEST_F(Mgr, file_overwrite_diff_biflow)
{
    EXPECT_EQ(fds_iemgr_read_file(mgr, FILES_VALID "pen.xml", true), FDS_IEMGR_OK);
    EXPECT_NO_ERROR;

    EXPECT_NE(fds_iemgr_read_file(mgr, FILES_VALID "split.xml", true), FDS_IEMGR_OK);
    EXPECT_ERROR;
}

TEST_F(Mgr, file_elem_no_name)
{
    EXPECT_NE(fds_iemgr_read_file(mgr, FILES_INVALID "elem_no_name.xml", true), FDS_IEMGR_OK);
    EXPECT_ERROR;
}

TEST_F(Mgr, file_elem_no_id)
{
    EXPECT_NE(fds_iemgr_read_file(mgr, FILES_INVALID "elem_no_id.xml", true), FDS_IEMGR_OK);
    EXPECT_ERROR;
}

TEST_F(Mgr, file_elem_no_data_type)
{
    EXPECT_NE(fds_iemgr_read_file(mgr, FILES_INVALID "elem_no_data_type.xml", true), FDS_IEMGR_OK);
    EXPECT_ERROR;
}

TEST_F(Mgr, file_pen_split_with_biflowId)
{
    EXPECT_NE(fds_iemgr_read_file(mgr, FILES_INVALID "split_with_biflowId.xml", true),
              FDS_IEMGR_OK);
    EXPECT_ERROR;
}

TEST_F(Mgr, file_split_id_out_of_range)
{
    EXPECT_NE(fds_iemgr_read_file(mgr, FILES_INVALID "split_id_out_of_range.xml", true),
              FDS_IEMGR_OK);
    EXPECT_ERROR;
}

TEST_F(Mgr, file_scope_no_name)
{
    EXPECT_NE(fds_iemgr_read_file(mgr, FILES_INVALID "scope_no_name.xml", true),
              FDS_IEMGR_OK);
    EXPECT_ERROR;
}

TEST_F(Mgr, file_scope_no_pen)
{
    EXPECT_NE(fds_iemgr_read_file(mgr, FILES_INVALID "scope_no_pen.xml", true),
              FDS_IEMGR_OK);
    EXPECT_ERROR;
}

TEST_F(Mgr, file_scope_invalid_mode)
{
    EXPECT_NE(fds_iemgr_read_file(mgr, FILES_INVALID "scope_invalid_mode.xml", true),
              FDS_IEMGR_OK);
    EXPECT_ERROR;
}

TEST_F(Mgr, file_else_if)
{
    ASSERT_EQ(fds_iemgr_read_file(mgr, FILES_VALID "else_if.xml", true), FDS_IEMGR_OK);
    EXPECT_NO_ERROR;

    const fds_iemgr_elem* elem;

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 1), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type,     FDS_ET_OCTET_ARRAY);
    EXPECT_EQ(elem->data_semantic, FDS_ES_DEFAULT);
    EXPECT_EQ(elem->data_unit,     FDS_EU_NONE);
    EXPECT_EQ(elem->status,        FDS_ST_CURRENT);

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 2), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type,     FDS_ET_UNSIGNED_8);
    EXPECT_EQ(elem->data_semantic, FDS_ES_QUANTITY);
    EXPECT_EQ(elem->data_unit,     FDS_EU_BITS);
    EXPECT_EQ(elem->status,        FDS_ST_DEPRECATED);

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 3), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type,     FDS_ET_UNSIGNED_16);
    EXPECT_EQ(elem->data_semantic, FDS_ES_TOTAL_COUNTER);
    EXPECT_EQ(elem->data_unit,     FDS_EU_OCTETS);

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 4), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type,     FDS_ET_UNSIGNED_32);
    EXPECT_EQ(elem->data_semantic, FDS_ES_DELTA_COUNTER);
    EXPECT_EQ(elem->data_unit,     FDS_EU_PACKETS);

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 5), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type,     FDS_ET_UNSIGNED_64);
    EXPECT_EQ(elem->data_semantic, FDS_ES_IDENTIFIER);
    EXPECT_EQ(elem->data_unit,     FDS_EU_FLOWS);

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 6), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type,     FDS_ET_SIGNED_8);
    EXPECT_EQ(elem->data_semantic, FDS_ES_FLAGS);
    EXPECT_EQ(elem->data_unit,     FDS_EU_SECONDS);

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 7), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type,     FDS_ET_SIGNED_16);
    EXPECT_EQ(elem->data_semantic, FDS_ES_LIST);
    EXPECT_EQ(elem->data_unit,     FDS_EU_MILLISECONDS);

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 8), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type, FDS_ET_SIGNED_32);
    EXPECT_EQ(elem->data_unit, FDS_EU_MICROSECONDS);

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 9), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type, FDS_ET_SIGNED_64);
    EXPECT_EQ(elem->data_unit, FDS_EU_NANOSECONDS);

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 10), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type, FDS_ET_FLOAT_32);
    EXPECT_EQ(elem->data_unit, FDS_EU_4_OCTET_WORDS);

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 11), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type, FDS_ET_FLOAT_64);
    EXPECT_EQ(elem->data_unit, FDS_EU_MESSAGES);

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 12), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type, FDS_ET_BOOLEAN);
    EXPECT_EQ(elem->data_unit, FDS_EU_HOPS);

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 13), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type, FDS_ET_MAC_ADDRESS);
    EXPECT_EQ(elem->data_unit, FDS_EU_ENTRIES);

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 14), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type, FDS_ET_STRING);
    EXPECT_EQ(elem->data_unit, FDS_EU_FRAMES);

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 15), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type, FDS_ET_DATE_TIME_SECONDS);

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 16), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type, FDS_ET_DATE_TIME_MILLISECONDS);

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 17), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type, FDS_ET_DATE_TIME_MICROSECONDS);

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 18), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type, FDS_ET_DATE_TIME_NANOSECONDS);

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 19), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type, FDS_ET_IPV4_ADDRESS);

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 20), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type, FDS_ET_IPV6_ADDRESS);

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 21), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type, FDS_ET_BASIC_LIST);

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 22), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type, FDS_ET_SUB_TEMPLATE_LIST);

    EXPECT_NE(elem = fds_iemgr_elem_find_id(mgr, 0, 23), nullptr);
    EXPECT_NO_ERROR;
    EXPECT_EQ(elem->data_type, FDS_ET_SUB_TEMPLATE_MULTILIST);
}

TEST_F(Mgr, file_cannot_overwrite)
{
    ASSERT_EQ(fds_iemgr_read_file(mgr, FILES_VALID "individual.xml", true), FDS_IEMGR_OK);
    EXPECT_NO_ERROR;
    EXPECT_NE(fds_iemgr_read_file(mgr, FILES_VALID "individual.xml", false), FDS_IEMGR_OK);
    EXPECT_ERROR;
}

TEST_F(Mgr, file_split_with_reverse)
{
    EXPECT_NE(fds_iemgr_read_file(mgr, FILES_INVALID "split_with_reverse.xml", true),
              FDS_IEMGR_OK);
    EXPECT_ERROR;
}

TEST_F(Mgr, file_elem_big_id)
{
    EXPECT_NE(fds_iemgr_read_file(mgr, FILES_INVALID "elem_big_id.xml", true),
              FDS_IEMGR_OK);
    EXPECT_ERROR;
}

TEST_F(Mgr, file_elem_empty_name)
{
    EXPECT_NE(fds_iemgr_read_file(mgr, FILES_INVALID "elem_empty_name.xml", true),
              FDS_IEMGR_OK);
    EXPECT_ERROR;
}

TEST_F(Mgr, file_remove_reverse_split)
{
    ASSERT_EQ(fds_iemgr_read_file(mgr, FILES_VALID "split.xml", true), FDS_IEMGR_OK);
    EXPECT_NO_ERROR;
    EXPECT_EQ(fds_iemgr_read_file(mgr, FILES_VALID "split.xml", true), FDS_IEMGR_OK);
    EXPECT_NO_ERROR;
}

TEST_F(Mgr, file_elem_invalid_type)
{
    EXPECT_NE(fds_iemgr_read_file(mgr, FILES_INVALID "elem_data_type.xml", true), FDS_IEMGR_OK);
    EXPECT_ERROR;
}

TEST_F(Mgr, file_elem_invalid_seman)
{
    EXPECT_NE(fds_iemgr_read_file(mgr, FILES_INVALID "elem_data_seman.xml", true), FDS_IEMGR_OK);
    EXPECT_ERROR;
}

TEST_F(Mgr, file_elem_invalid_unit)
{
    EXPECT_NE(fds_iemgr_read_file(mgr, FILES_INVALID "elem_data_unit.xml", true), FDS_IEMGR_OK);
    EXPECT_ERROR;
}

TEST_F(Mgr, file_elem_invalid_status)
{
    EXPECT_NE(fds_iemgr_read_file(mgr, FILES_INVALID "elem_data_status.xml", true), FDS_IEMGR_OK);
    EXPECT_ERROR;
}

//TEST_F(Mgr, dir_no_file) // TODO git doesn't preserve empty dirs
//{
//    EXPECT_EQ(fds_iemgr_read_dir(mgr, FILES_VALID "no_file"), FDS_IEMGR_OK);
//    EXPECT_NO_ERROR;
//}

//TEST_F(Mgr, dir_success)
//{
//    EXPECT_EQ(fds_iemgr_read_dir(mgr, FILES_VALID "valid"), FDS_IEMGR_OK);
//    EXPECT_NO_ERROR;
//}
