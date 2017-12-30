/**
 * \author Michal Režňák
 * \date   8. August 2017
 */

#include <gtest/gtest.h>
#include <libfds.h>
#include "iemgr_common.h"

TEST_F(Mgr, valid)
{
    EXPECT_EQ(fds_iemgr_read_file(mgr, FILES_VALID "split.xml", true), FDS_OK);
    EXPECT_NO_ERROR;

    EXPECT_EQ(fds_iemgr_compare_timestamps(mgr), FDS_OK);
}
