/**
 * \author Michal Režňák
 * \date   8.8.17
 */

#include <gtest/gtest.h>
#include <libxml2/libxml/parser.h>
#include <libfds.h>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

/**
 * fds_xml_create
 */
class Create : public ::testing::Test
{
protected:
    fds_xml_t *parser = NULL;

    virtual void TearDown() {
        fds_xml_destroy(parser);
    }
};

TEST_F(Create, all)
{
    EXPECT_EQ(fds_xml_create(&parser), FDS_OK);
}

TEST_F(Create, parser_null)
{
    EXPECT_EQ(fds_xml_create(NULL), FDS_ERR_FORMAT);
}
