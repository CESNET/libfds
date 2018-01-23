/**
 * \author Michal Režňák
 * \date   8.8.17
 */

#include <gtest/gtest.h>
#include <libfds.h>
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

TEST(Test, pen_copy)
{
    const struct fds_iemgr_elem *elem = nullptr;
    fds_iemgr_t *mgr_orig = fds_iemgr_create();
    ASSERT_NE(mgr_orig, nullptr);
    ASSERT_EQ(fds_iemgr_read_file(mgr_orig, FILES_VALID "pen.xml", true), FDS_OK);

    // Try to read something first
    elem = fds_iemgr_elem_find_id(mgr_orig, 1, 1);
    ASSERT_NE(elem, nullptr);
    EXPECT_EQ(elem->id, 1);
    EXPECT_EQ(elem->scope->pen, 1);
    EXPECT_TRUE(elem->is_reverse);

    // Copy the scope
    fds_iemgr_t *mgr_copy = fds_iemgr_copy(mgr_orig);
    ASSERT_NE(mgr_copy, nullptr);

    // Remove something from the original manager
    EXPECT_EQ(fds_iemgr_elem_remove(mgr_orig, 1, 5), FDS_OK);
    elem = fds_iemgr_elem_find_id(mgr_orig, 1, 5);
    EXPECT_EQ(elem, nullptr);

    // It should be still available in the copy
    elem = fds_iemgr_elem_find_id(mgr_copy, 1, 5);
    ASSERT_NE(elem, nullptr);
    EXPECT_EQ(elem->id, 5);

    // Remove the original scope
    fds_iemgr_destroy(mgr_orig);

    // The copy should be still available
    elem = fds_iemgr_elem_find_id(mgr_copy, 0, 5);
    ASSERT_NE(elem, nullptr);
    EXPECT_EQ(elem->id, 5);
    EXPECT_EQ(elem->scope->pen, 0);

    fds_iemgr_destroy(mgr_copy);
}

