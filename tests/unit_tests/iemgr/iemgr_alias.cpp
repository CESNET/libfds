#include <gtest/gtest.h>
#include <libfds.h>
#include "iemgr_common.h"

TEST_F(Fill, alias_valid)
{
    EXPECT_EQ(fds_iemgr_alias_read_file(mgr, FILES_VALID "aliases.xml"), FDS_OK);
    EXPECT_NO_ERROR;

    EXPECT_NE(fds_iemgr_alias_find(mgr, "ac"), nullptr);
    EXPECT_NE(fds_iemgr_alias_find(mgr, "ca"), nullptr);
    EXPECT_NE(fds_iemgr_alias_find(mgr, "d"), nullptr);
    EXPECT_EQ(fds_iemgr_alias_find(mgr, "a"), nullptr);
    EXPECT_EQ(fds_iemgr_alias_find(mgr, "b"), nullptr);
    EXPECT_EQ(fds_iemgr_alias_find(mgr, "c"), nullptr);

    auto *alias_ac = fds_iemgr_alias_find(mgr, "ac");
    ASSERT_NE(alias_ac, nullptr);
    auto *elem_a = fds_iemgr_elem_find_name(mgr, "iana:a");
    ASSERT_NE(elem_a, nullptr);
    auto *elem_c = fds_iemgr_elem_find_name(mgr, "iana:c");
    ASSERT_NE(elem_c, nullptr);
    auto *alias_d = fds_iemgr_alias_find(mgr, "d");
    ASSERT_NE(alias_d, nullptr);
    auto *elem_d = fds_iemgr_elem_find_name(mgr, "iana:d");
    ASSERT_NE(elem_d, nullptr);

    EXPECT_EQ(alias_ac->sources_cnt, 2);
    EXPECT_EQ(alias_ac->sources[0], elem_a);
    EXPECT_EQ(alias_ac->sources[1], elem_c);

    EXPECT_EQ(alias_d->sources_cnt, 1);
    EXPECT_EQ(alias_d->sources[0], elem_d);

    EXPECT_EQ(elem_a->aliases_cnt, 1);
    EXPECT_EQ(elem_a->aliases[0], alias_ac);
    EXPECT_EQ(elem_c->aliases_cnt, 1);
    EXPECT_EQ(elem_c->aliases[0], alias_ac);

    EXPECT_EQ(elem_d->aliases_cnt, 1);
    EXPECT_EQ(elem_d->aliases[0], alias_d);

    auto *elem_e = fds_iemgr_elem_find_name(mgr, "iana:e");
    ASSERT_NE(elem_e, nullptr);
    EXPECT_EQ(elem_e->aliases_cnt, 0);
}

TEST_F(Fill, alias_duplicate)
{
    EXPECT_NE(fds_iemgr_alias_read_file(mgr, FILES_INVALID "alias_duplicate.xml"), FDS_OK);
    EXPECT_ERROR;
}

TEST_F(Fill, alias_invalid_name_chars)
{
    EXPECT_NE(fds_iemgr_alias_read_file(mgr, FILES_INVALID "alias_invalid_name_chars.xml"), FDS_OK);
    EXPECT_ERROR;
}

TEST_F(Fill, alias_invalid_name_spaces)
{
    EXPECT_NE(fds_iemgr_alias_read_file(mgr, FILES_INVALID "alias_invalid_name_spaces.xml"), FDS_OK);
    EXPECT_ERROR;
}

TEST_F(Fill, alias_empty_name)
{
    EXPECT_NE(fds_iemgr_alias_read_file(mgr, FILES_INVALID "alias_empty_name.xml"), FDS_OK);
    EXPECT_ERROR;
}

TEST_F(Fill, alias_empty_sources)
{
    EXPECT_NE(fds_iemgr_alias_read_file(mgr, FILES_INVALID "alias_empty_sources.xml"), FDS_OK);
    EXPECT_ERROR;
}