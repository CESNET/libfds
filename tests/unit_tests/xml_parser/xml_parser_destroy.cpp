#include <gtest/gtest.h>
#include <libxml2/libxml/parser.h>
#include <libfds/common.h>

extern "C" {
	#include <libfds/xml_parser.h>
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

/**
 * fds_xml_destroy
 */
class Destroy : public ::testing::Test
{
protected:
    fds_xml_t *parser = NULL;
    const char *err_msg = (const char *) "No error";
};

TEST_F(Destroy, all)
{
    fds_xml_destroy(parser);

    EXPECT_EQ(parser, (fds_xml_t *) NULL);
}

TEST_F(Destroy, valid)
{
    EXPECT_EQ(fds_xml_create(&parser), FDS_OK);
    fds_xml_destroy(parser);
}

TEST_F(Destroy, nested_context)
{
    fds_xml_create(&parser);

    const struct fds_xml_args nested[] = {
            OPTS_ELEM(2, "name", OPTS_T_DOUBLE, 0),
            OPTS_END
    };
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_NESTED(1, "nested", nested, 0),
            OPTS_END
    };
    fds_xml_set_args(args, parser);

    const char *mem =
            "<root>"
                "<nested>"
                    "<name>4.2</name>"
                "</nested>"
            "</root>";

    EXPECT_NE(fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx *) NULL);
    EXPECT_STREQ(fds_xml_last_err(parser), err_msg);

    fds_xml_destroy(parser);
}
