/**
 * \author Michal Režňák
 * \date   8. August 2017
 */

#include <gtest/gtest.h>
#include <libfds/iemgr.h>
#include "iemgr_common.h"

TEST(Destroy, null)
{
    EXPECT_NO_THROW(fds_iemgr_destroy(nullptr));
}

TEST(Destroy, success)
{
    auto mgr = fds_iemgr_create();
    EXPECT_NO_THROW(fds_iemgr_destroy(mgr));
}
