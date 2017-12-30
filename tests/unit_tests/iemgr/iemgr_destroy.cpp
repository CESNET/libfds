/**
 * \author Michal Režňák
 * \date   8. August 2017
 */

#include <gtest/gtest.h>
#include <libfds.h>
#include "iemgr_common.h"

TEST(Destroy, success)
{
    auto mgr = fds_iemgr_create();
    EXPECT_NO_THROW(fds_iemgr_destroy(mgr));
}

TEST_F(Mgr, elem_remove)
{
    ASSERT_EQ(fds_iemgr_read_file(mgr, FILES_VALID "one_elem.xml", true), FDS_OK);
    EXPECT_NO_ERROR;

    EXPECT_EQ(fds_iemgr_elem_remove(mgr, 0, 1), FDS_OK);

    const fds_iemgr_elem* elem = fds_iemgr_elem_find_id(mgr, 0, 1);
    EXPECT_EQ(elem, nullptr);
    EXPECT_NO_ERROR;
}

TEST_F(Mgr, no_scope)
{
    EXPECT_EQ(fds_iemgr_elem_remove(mgr, 0, 0), FDS_ERR_NOTFOUND);
    EXPECT_NO_ERROR;
}