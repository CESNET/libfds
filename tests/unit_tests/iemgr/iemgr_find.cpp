/**
 * \author Michal Režňák
 * \date   8. August 2017
 */

#include <gtest/gtest.h>
#include <libfds/iemgr.h>
#include "iemgr_common.h"

TEST_F(Fill, elem_id_success)
{
    auto elem = fds_iemgr_elem_find_id(mgr, 0, 1);
    EXPECT_NE(elem, nullptr);
    EXPECT_NO_ERROR;

    EXPECT_EQ(elem->id, 1);
    EXPECT_FALSE(elem->is_reverse);
    EXPECT_EQ(elem->scope->pen, 0);
    EXPECT_EQ(elem->scope->biflow_mode, FDS_BW_INDIVIDUAL);
}

TEST_F(Fill, elem_id_null)
{
    auto elem = fds_iemgr_elem_find_id(nullptr, 0, 1);
    EXPECT_EQ(elem, nullptr);
}

TEST_F(Fill, elem_id_out_of_range)
{
    auto elem = fds_iemgr_elem_find_id(mgr, 0, 999);
    EXPECT_EQ(elem, nullptr);
    EXPECT_NO_ERROR;
}

TEST_F(Fill, elem_pen_out_of_range)
{
    auto elem = fds_iemgr_elem_find_id(mgr, 999, 1);
    EXPECT_EQ(elem, nullptr);
    EXPECT_NO_ERROR;
}

TEST_F(Fill, elem_name_success)
{
    auto elem = fds_iemgr_elem_find_name(mgr, "iana:a");
    EXPECT_NE(elem, nullptr);
    EXPECT_NO_ERROR;

    EXPECT_EQ(elem->id, 1);
    EXPECT_FALSE(elem->is_reverse);
    EXPECT_EQ(elem->scope->pen, 0);
    EXPECT_EQ(elem->scope->biflow_mode, FDS_BW_INDIVIDUAL);
}

TEST_F(Fill, elem_name_null)
{
    auto elem = fds_iemgr_elem_find_name(nullptr, "iana:a");
    EXPECT_EQ(elem, nullptr);

    elem = fds_iemgr_elem_find_name(nullptr, nullptr);
    EXPECT_EQ(elem, nullptr);

    elem = fds_iemgr_elem_find_name(mgr, nullptr);
    EXPECT_EQ(elem, nullptr);
    EXPECT_ERROR;
}

TEST_F(Fill, elem_name_invalid)
{
    auto elem = fds_iemgr_elem_find_name(mgr, "iana:not_existing_name");
    EXPECT_EQ(elem, nullptr);
    EXPECT_NO_ERROR;
}

TEST_F(Fill, elem_name_scope_invalid)
{
    auto elem = fds_iemgr_elem_find_name(mgr, "not_existing_scope_name:a");
    EXPECT_EQ(elem, nullptr);
    EXPECT_NO_ERROR;
}


TEST_F(Fill, scope_pen_success)
{
    auto scope = fds_iemgr_scope_find_pen(mgr, 0);
    EXPECT_NE(scope, nullptr);
    EXPECT_NO_ERROR;

    EXPECT_EQ(scope->pen, 0);
    EXPECT_EQ(scope->biflow_mode, FDS_BW_INDIVIDUAL);
    EXPECT_STREQ(scope->name, "iana");
}

TEST_F(Fill, scope_pen_null)
{
    auto elem = fds_iemgr_scope_find_pen(nullptr, 0);
    EXPECT_EQ(elem, nullptr);
}

TEST_F(Fill, scope_pen_out_of_range)
{
    auto elem = fds_iemgr_scope_find_pen(mgr, 999);
    EXPECT_EQ(elem, nullptr);
    EXPECT_NO_ERROR;
}
