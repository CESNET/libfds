/**
 * \author Michal Režňák
 * \date   8.8.17
 */

#include <gtest/gtest.h>
#include <libfds/iemgr.h>
#include "iemgr_common.h"

TEST_F(Fill, success)
{
    fds_iemgr_t* res = nullptr;
    fds_iemgr_read_file(mgr, FILES_VALID "pen.xml", true);
    EXPECT_NO_THROW(res = fds_iemgr_copy(mgr));
    EXPECT_NE(res, nullptr);

    fds_iemgr_t* temp = fds_iemgr_create();
    fds_iemgr_destroy(temp);

    EXPECT_NO_THROW(fds_iemgr_destroy(res));
}

TEST(Mgr, null)
{
    EXPECT_EQ(fds_iemgr_copy(nullptr), nullptr);
}
