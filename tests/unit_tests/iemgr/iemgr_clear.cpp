/**
 * \author Michal Režňák
 * \date   8. August 2017
 */

#include <gtest/gtest.h>
#include <libfds/iemgr.h>
#include <libfds/common.h>
#include "iemgr_common.h"

TEST_F(Mgr, success)
{
    fds_iemgr_t* temp = fds_iemgr_create();

    EXPECT_EQ(fds_iemgr_read_file(mgr, FILES_VALID"individual.xml", true), FDS_OK);
    EXPECT_NO_ERROR;

    fds_iemgr_clear(mgr);
    fds_iemgr_destroy(temp);
}

