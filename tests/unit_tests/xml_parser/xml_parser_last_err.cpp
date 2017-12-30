#include <gtest/gtest.h>
#include <libxml2/libxml/parser.h>
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
    fds_xml_create(&parser);
    fds_xml_parse_mem(parser, NULL, true);

    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Last_err, parser_null)
{
    EXPECT_EQ(fds_xml_last_err(NULL), nullptr);
}

