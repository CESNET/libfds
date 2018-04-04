#include <gtest/gtest.h>
#include <libxml2/libxml/parser.h>
#include <libfds.h>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

/**
 * fds_xml_rewind
 */
class Rewind : public ::testing::Test
{
protected:
    fds_xml_ctx_t *ctx;
    fds_xml_t *parser = NULL;

    virtual void SetUp() {
        parser = fds_xml_create();
    }

    virtual void TearDown(){
        fds_xml_destroy(parser);
    }
};

TEST_F(Rewind, ctx_null)
{
    fds_xml_rewind(NULL);
}

TEST_F(Rewind, valid) {
    static const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, "elem", FDS_OPTS_T_STRING, 0),
            FDS_OPTS_END
    };
    const char *mem =
            "<root>"
            "   <elem>    retezec    </elem>"
            "</root>";
    fds_xml_set_args(parser, args);
    ctx = fds_xml_parse_mem(parser, mem, true);

    const struct fds_xml_cont *content_prev;
    const struct fds_xml_cont *content_after;
    fds_xml_next(ctx, &content_prev);

    fds_xml_rewind(ctx);
    fds_xml_next(ctx, &content_after);

    EXPECT_EQ(content_prev->id, content_after->id);
    EXPECT_EQ(content_prev->type, content_after->type);
    EXPECT_EQ(content_prev->ptr_string, content_after->ptr_string);
}

TEST_F(Rewind, nested)
{
    fds_xml_ctx *ctx = NULL;
    const fds_xml_cont *cont;

    static const struct fds_xml_args nested[] = {
            FDS_OPTS_ELEM(2, "name", FDS_OPTS_T_UINT, 0),
            FDS_OPTS_END
    };
    static const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_NESTED(1, "nested", nested, 0),
            FDS_OPTS_END
    };
    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_OK);

    const char *mem =
            "<root>"
                "<nested>"
                    "<name>300</name>"
                "</nested>"
            "</root>";
    ctx = fds_xml_parse_mem(parser, mem, true);

    EXPECT_NE(fds_xml_next(ctx, &cont), FDS_EOC);
    fds_xml_ctx *cur_ctx = cont->ptr_ctx;
    EXPECT_NE(fds_xml_next(cur_ctx, &cont), FDS_EOC);

    fds_xml_rewind(ctx);
}

TEST_F(Rewind, over)
{
    static const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, "elem", FDS_OPTS_T_STRING, 0),
            FDS_OPTS_END
    };
    const char *mem =
            "<root>"
                    "   <elem>    retezec    </elem>"
                    "</root>";
    fds_xml_set_args(parser, args);
    ctx = fds_xml_parse_mem(parser, mem, true);

    const struct fds_xml_cont *content_prev;
    const struct fds_xml_cont *content_after;
    fds_xml_next(ctx, &content_prev);

    fds_xml_rewind(ctx);
    fds_xml_next(ctx, &content_after);

    EXPECT_EQ(fds_xml_next(ctx, &content_after), FDS_EOC);
}

