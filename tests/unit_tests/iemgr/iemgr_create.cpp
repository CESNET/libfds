/**
 * \author Michal Režňák
 * \date   8.8.17
 */

#include <gtest/gtest.h>
#include <libfds.h>
#include "iemgr_common.h"

TEST(Create, success)
{
    fds_iemgr_t* mgr = fds_iemgr_create();
    EXPECT_NE(mgr, nullptr);
    fds_iemgr_destroy(mgr);
}
