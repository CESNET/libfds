#include <gtest/gtest.h>
#include <libfds.h>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

/**
 * fds_xml_last_err
 */
class Last_err : public ::testing::Test
{
protected:
    fds_xml_t *parser = NULL;

    void TearDown() override {
        fds_xml_destroy(parser);
    }
};

TEST_F(Last_err, valid)
{
    parser = fds_xml_create();
    EXPECT_EQ(fds_xml_parse_mem(parser, NULL, true), nullptr);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Last_err, parser_null)
{
    EXPECT_EQ(fds_xml_last_err(NULL), nullptr);
}

