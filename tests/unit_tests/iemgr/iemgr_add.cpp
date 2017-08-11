/**
 * \author Michal Režňák
 * \date   11.8.17
 */

#include <gtest/gtest.h>
#include <libfds/iemgr.h>
#include "iemgr_common.h"

TEST_F(Fill, success)
{
    fds_iemgr_elem elem{};
    elem.id = 422;
    elem.name = const_cast<char*>("name");
    elem.data_type = FDS_ET_UNSIGNED_64;

    EXPECT_EQ(fds_iemgr_elem_add(mgr, &elem, 0, false), FDS_IEMGR_OK);
    EXPECT_NO_ERROR;

    EXPECT_EQ(fds_iemgr_elem_add_reverse(mgr, 0, 422, 999, false), FDS_IEMGR_OK);
    EXPECT_NO_ERROR;
}

TEST_F(Mgr, same_biflow_id)
{
    EXPECT_NE(fds_iemgr_read_file(mgr, FILES_INVALID "same_biflow_id.xml", false), FDS_IEMGR_OK);
    EXPECT_ERROR;
}

TEST_F(Mgr, success)
{
    fds_iemgr_elem elem{};
    elem.id = 422;
    elem.name = const_cast<char*>("name");
    elem.data_type = FDS_ET_UNSIGNED_64;

    EXPECT_EQ(fds_iemgr_elem_add(mgr, &elem, 0, false), FDS_IEMGR_OK);
    EXPECT_NO_ERROR;

    EXPECT_EQ(fds_iemgr_elem_add_reverse(mgr, 0, 422, 999, false), FDS_IEMGR_OK);
    EXPECT_NO_ERROR;
}

TEST_F(Mgr, elem_not_defined)
{
    EXPECT_EQ(fds_iemgr_elem_add(mgr, nullptr, 1, false), FDS_IEMGR_ERR);
    EXPECT_ERROR;
}

TEST_F(Fill, elem_not_found)
{
    EXPECT_EQ(fds_iemgr_elem_add_reverse(mgr, 0, 0, 1, false), FDS_IEMGR_NOT_FOUND);
    EXPECT_ERROR;
}

TEST_F(Mgr, scope_not_found)
{
    EXPECT_EQ(fds_iemgr_elem_add_reverse(mgr, 0, 0, 1, false), FDS_IEMGR_NOT_FOUND);
    EXPECT_ERROR;
}

TEST_F(Mgr, add_elem_to_not_individual)
{
    ASSERT_EQ(fds_iemgr_read_file(mgr, FILES_VALID "pen.xml", true), FDS_IEMGR_OK);

    EXPECT_EQ(fds_iemgr_elem_add_reverse(mgr, 0, 999, 1, false), FDS_IEMGR_ERR);
    EXPECT_ERROR;
}
