#include <gtest/gtest.h>
#include <libfds.h>
#include "iemgr_common.h"

TEST_F(FillAndAlias, mapping_valid)
{
    EXPECT_EQ(fds_iemgr_mapping_read_file(mgr, FILES_VALID "mappings.xml"), FDS_OK);
    EXPECT_NO_ERROR;

    auto *elem_e = fds_iemgr_elem_find_name(mgr, "iana:e");
    ASSERT_NE(elem_e, nullptr);
    auto *elem_a = fds_iemgr_elem_find_name(mgr, "iana:a");
    ASSERT_NE(elem_a, nullptr);
    auto *elem_c = fds_iemgr_elem_find_name(mgr, "iana:c");
    ASSERT_NE(elem_c, nullptr);

    EXPECT_EQ(elem_e->mappings_cnt, 1);
    EXPECT_EQ(elem_e->mappings[0]->items_cnt, 2);
    EXPECT_EQ(elem_e->mappings[0]->items[0].value.i, 1);
    EXPECT_EQ(elem_e->mappings[0]->items[1].value.i, 2);

    EXPECT_NE(fds_iemgr_mapping_find(mgr, "iana:e", "val1"), nullptr);
    EXPECT_NE(fds_iemgr_mapping_find(mgr, "iana:e", "val1"), nullptr);
    EXPECT_NE(fds_iemgr_mapping_find(mgr, "iana:e", "Val1"), nullptr);
    EXPECT_NE(fds_iemgr_mapping_find(mgr, "iana:e", "VAL1"), nullptr);
    EXPECT_NE(fds_iemgr_mapping_find(mgr, "iana:e", "VAL2"), nullptr);
    EXPECT_NE(fds_iemgr_mapping_find(mgr, "iana:e", "Val2"), nullptr);
    EXPECT_NE(fds_iemgr_mapping_find(mgr, "iana:e", "val2"), nullptr);

    EXPECT_EQ(fds_iemgr_mapping_find(mgr, "iana:e", "val1")->value.i, 1);
    EXPECT_EQ(fds_iemgr_mapping_find(mgr, "iana:e", "val2")->value.i, 2);

    EXPECT_NE(fds_iemgr_mapping_find(mgr, "ac", "val3"), nullptr);
    EXPECT_NE(fds_iemgr_mapping_find(mgr, "ca", "val3"), nullptr);
    
    EXPECT_EQ(fds_iemgr_mapping_find(mgr, "aca", "val3"), nullptr);
    EXPECT_EQ(fds_iemgr_mapping_find(mgr, "caca", "val3"), nullptr);

    EXPECT_EQ(fds_iemgr_mapping_find(mgr, "ac", "val1"), nullptr);
    EXPECT_EQ(fds_iemgr_mapping_find(mgr, "ac", "val2"), nullptr);

    EXPECT_EQ(fds_iemgr_mapping_find(mgr, "ca", "val1"), nullptr);
    EXPECT_EQ(fds_iemgr_mapping_find(mgr, "ca", "val2"), nullptr);

    EXPECT_EQ(fds_iemgr_mapping_find(mgr, "ac", "Val3"), nullptr);
    EXPECT_EQ(fds_iemgr_mapping_find(mgr, "ac", "VAL3"), nullptr);

    EXPECT_EQ(fds_iemgr_mapping_find(mgr, "ac", "val3")->value.i, 3);
    EXPECT_EQ(fds_iemgr_mapping_find(mgr, "ca", "val3")->value.i, 3);

    EXPECT_EQ(elem_a->mappings_cnt, 1);
    EXPECT_EQ(elem_a->mappings[0]->items_cnt, 1);
    EXPECT_EQ(elem_a->mappings[0]->items[0].value.i, 3);
    EXPECT_EQ(elem_c->mappings_cnt, 1);
    EXPECT_EQ(elem_c->mappings[0]->items_cnt, 1);
    EXPECT_EQ(elem_c->mappings[0]->items[0].value.i, 3);
}

TEST_F(FillAndAlias, mappings_blank_match)
{
    EXPECT_NE(fds_iemgr_mapping_read_file(mgr, FILES_VALID "mappings_blank_match.xml"), FDS_OK);
    EXPECT_ERROR;
}

TEST_F(FillAndAlias, mappings_invalid_key_blank)
{
    EXPECT_NE(fds_iemgr_mapping_read_file(mgr, FILES_VALID "mappings_invalid_key_blank.xml"), FDS_OK);
    EXPECT_ERROR;
}

TEST_F(FillAndAlias, mappings_invalid_key_chars)
{
    EXPECT_NE(fds_iemgr_mapping_read_file(mgr, FILES_VALID "mappings_invalid_key_chars.xml"), FDS_OK);
    EXPECT_ERROR;
}

TEST_F(FillAndAlias, mappings_invalid_key_space)
{
    EXPECT_NE(fds_iemgr_mapping_read_file(mgr, FILES_VALID "mappings_invalid_key_space.xml"), FDS_OK);
    EXPECT_ERROR;
}

TEST_F(FillAndAlias, mappings_invalid_match)
{
    EXPECT_NE(fds_iemgr_mapping_read_file(mgr, FILES_VALID "mappings_invalid_match.xml"), FDS_OK);
    EXPECT_ERROR;
}

TEST_F(FillAndAlias, mappings_invalid_value)
{
    EXPECT_NE(fds_iemgr_mapping_read_file(mgr, FILES_VALID "mappings_invalid_value.xml"), FDS_OK);
    EXPECT_ERROR;
}

TEST_F(FillAndAlias, mappings_no_match)
{
    EXPECT_NE(fds_iemgr_mapping_read_file(mgr, FILES_VALID "mappings_no_match.xml"), FDS_OK);
    EXPECT_ERROR;
}

TEST_F(FillAndAlias, mappings_nonexistent)
{
    EXPECT_NE(fds_iemgr_mapping_read_file(mgr, FILES_VALID "mappings_nonexistent.xml"), FDS_OK);
    EXPECT_ERROR;
}

TEST_F(FillAndAlias, mappings_duplicate)
{
    EXPECT_NE(fds_iemgr_mapping_read_file(mgr, FILES_VALID "mappings_duplicate.xml"), FDS_OK);
    EXPECT_ERROR;
}