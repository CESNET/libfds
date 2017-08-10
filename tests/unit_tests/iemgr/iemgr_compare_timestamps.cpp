/**
 * \author Michal Režňák
 * \date   8. August 2017
 */

#include <gtest/gtest.h>
#include <libfds/iemgr.h>
#include "iemgr_common.h"

TEST_F(Mgr, valid)
{
    EXPECT_EQ(fds_iemgr_read_file(mgr, FILES_VALID "split.xml", true), FDS_IEMGR_OK);
    EXPECT_NO_ERROR;

    EXPECT_TRUE(fds_iemgr_compare_timestamps(mgr));
}