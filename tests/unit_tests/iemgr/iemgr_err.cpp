/**
 * \author Michal Režňák
 * \date   8. August 2017
 */

#include <gtest/gtest.h>
#include <libfds/iemgr.h>
#include "iemgr_common.h"

TEST_F(Mgr, null)
{
    EXPECT_EQ(fds_iemgr_last_err(nullptr), nullptr);
}
