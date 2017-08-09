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

TEST_F(Mgr, dir_no_file) // TODO git doesn't preserve empty dirs
{
    EXPECT_EQ(fds_iemgr_read_dir(mgr, FILES_VALID "no_file"), FDS_IEMGR_OK);
    EXPECT_NO_ERROR;
}

TEST_F(Mgr, dir_success)
{
    EXPECT_EQ(fds_iemgr_read_dir(mgr, FILES_VALID "valid"), FDS_IEMGR_OK);
    EXPECT_NO_ERROR;
}
