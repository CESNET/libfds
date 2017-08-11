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

TEST_F(Mgr, null)
{
    EXPECT_EQ(fds_iemgr_copy(nullptr), nullptr);
}

TEST(Test, same_address)
{
    fds_iemgr_t* mgr = fds_iemgr_create();
    fds_iemgr_read_file(mgr, FILES_VALID "individual.xml", true);

    fds_iemgr_t* res = fds_iemgr_copy(mgr);
    EXPECT_NE(res, nullptr);
    EXPECT_NO_ERROR;

    fds_iemgr_destroy(mgr);

    const fds_iemgr_elem* elem = fds_iemgr_elem_find_id(res, 0, 1);
    EXPECT_NE(elem, nullptr);

    EXPECT_EQ(elem->data_type,     FDS_ET_UNSIGNED_64);
    EXPECT_EQ(elem->data_semantic, FDS_ES_DELTA_COUNTER);
    EXPECT_EQ(elem->data_unit,     FDS_EU_OCTETS);
    EXPECT_EQ(elem->status,        FDS_ST_CURRENT);
    EXPECT_FALSE(elem->is_reverse);
    EXPECT_NE(elem->reverse_elem, nullptr);

    fds_iemgr_destroy(res);
}
