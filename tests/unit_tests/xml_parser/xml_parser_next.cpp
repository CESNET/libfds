#include <gtest/gtest.h>
#include <libxml2/libxml/parser.h>
#include <libfds.h>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

/**
 * fds_xml_next
 */
class Next : public ::testing::Test
{
protected:
    fds_xml_ctx_t *ctx = NULL;
    const struct fds_xml_cont *content;
    fds_xml_t *parser = NULL;

    virtual void SetUp() {
        parser = fds_xml_create();
    }

    virtual void TearDown() {
        fds_xml_destroy(parser);
    }
};

TEST_F(Next, all_null)
{
    EXPECT_EQ(fds_xml_next(NULL, &content), FDS_ERR_FORMAT);
    EXPECT_EQ(fds_xml_next(ctx, NULL), FDS_ERR_FORMAT);
    EXPECT_EQ(fds_xml_next(NULL, NULL), FDS_ERR_FORMAT);
}

TEST_F(Next, not_same)
{
    static const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_ELEM(1, "elem1", OPTS_T_STRING, 0),
            OPTS_ELEM(2, "elem2", OPTS_T_BOOL, OPTS_P_OPT),
            OPTS_END
    };

    const char *mem =
            "<root>"
                    "<elem1>retezec</elem1>"
                    "<elem2>True</elem2>"
            "</root>";
    fds_xml_set_args(parser, args);
    ctx = fds_xml_parse_mem(parser, mem, true);

    const struct fds_xml_cont *content_prev;
    const struct fds_xml_cont *content_last;

    fds_xml_next(ctx, &content_prev);
    fds_xml_next(ctx, &content_last);

    EXPECT_NE(content_prev->id, content_last->id);
    EXPECT_NE(content_prev->type, content_last->type);
}

TEST_F(Next, last)
{
    static const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_ELEM(1, "elem1", OPTS_T_STRING, 0),
            OPTS_END
    };

    const char *mem =
            "<root>"
                "<elem1>retezec</elem1>"
            "</root>";

    fds_xml_set_args(parser, args);
    ctx = fds_xml_parse_mem(parser, mem, true);

    const struct fds_xml_cont *content;

    EXPECT_NE(fds_xml_next(ctx, &content), FDS_EOC);
    EXPECT_EQ(fds_xml_next(ctx, &content), FDS_EOC);
}

