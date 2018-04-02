#include <gtest/gtest.h>
#include <libfds.h>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(api, getCfgDir)
{
    const char *dir = fds_api_cfg_dir();
    ASSERT_NE(dir, nullptr);
    EXPECT_GT(strlen(dir), 0);
}