#include <gtest/gtest.h>

extern "C" {
	#include <libfds.h>
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(dummy, simple_test) {
	EXPECT_EQ(dummy_api_test(), 1);
}
