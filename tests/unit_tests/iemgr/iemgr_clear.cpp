/**
 * \author Michal Režňák
 * \date   8. August 2017
 */

#include <gtest/gtest.h>
#include <libfds/iemgr.h>
#include "iemgr_common.h"

TEST_F(Mgr, success)
{
    fds_iemgr_t* temp = fds_iemgr_create();

    EXPECT_EQ(fds_iemgr_read_file(mgr, FILES_VALID"individual.xml", true), FDS_IEMGR_OK);
    EXPECT_NO_ERROR;

    fds_iemgr_clear(mgr);

//    EXPECT_EQ(mgr->err_msg, ""); TODO how to check manager if is empty?
//    EXPECT_TRUE(mgr->mtime.empty());
//    EXPECT_TRUE(mgr->prefixes.empty());
//    EXPECT_TRUE(mgr->pens.empty());
//    EXPECT_TRUE(mgr->parsed_ids.empty());
//    EXPECT_TRUE(mgr->overwrite_scope.second.empty());

    fds_iemgr_destroy(temp);
}

TEST_F(Mgr, null)
{
    EXPECT_NO_THROW(fds_iemgr_clear(nullptr));
}
