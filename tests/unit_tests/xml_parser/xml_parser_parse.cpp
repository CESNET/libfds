#include <gtest/gtest.h>
#include <libxml2/libxml/parser.h>
#include <libfds.h>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

/**
 * fds_xml_parse_mem
 */
class Parse : public ::testing::Test
{
public:
    fds_xml_t *parser = NULL;
    const char *err_msg = (const char *) "No error";

    virtual void SetUp() {
        parser = fds_xml_create();
    }

    virtual void TearDown() {
        fds_xml_destroy(parser);
    }
};

TEST_F(Parse, inputs_null)
{
    const struct fds_xml_args opts[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, "name", FDS_OPTS_T_UINT, 0),
            FDS_OPTS_END
    };
    fds_xml_set_args(parser, opts);

    const char *mem =
            "<root>"
                "<name>300</name>"
            "</root>";

    EXPECT_EQ(fds_xml_parse_mem(NULL, mem, true), (fds_xml_ctx_t *) NULL);
    EXPECT_EQ(fds_xml_parse_mem(parser, NULL, false), (fds_xml_ctx_t *) NULL);
}

TEST_F(Parse, xml_file_wrong)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_END
    };
    fds_xml_set_args(parser, args);

    const char *mem = "ABCD";

    EXPECT_EQ(fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx_t *) NULL);
    EXPECT_STRNE(fds_xml_last_err(parser), err_msg);
}

TEST_F(Parse, parser_opts_not_set)
{
    const char *mem = "<root></root>";

    EXPECT_EQ(fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx_t *) NULL);
    EXPECT_STRNE(fds_xml_last_err(parser), err_msg);
}

TEST_F(Parse, two_root_nodes)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_END
    };
    fds_xml_set_args(parser, args);

    const char *mem =
            "<root></root>"
            "<another></another>";

    EXPECT_EQ(fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx *) NULL);
    EXPECT_STRNE(fds_xml_last_err(parser), err_msg);
}

TEST_F(Parse, missing_element)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_END
    };

    const char *mem =
            "<root>"
                    "<name>300</name>"
            "</root>";

    fds_xml_set_args(parser, args);
    EXPECT_NE(fds_xml_parse_mem(parser, mem, false), (fds_xml_ctx_t *) NULL);
    EXPECT_EQ(fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx_t *) NULL);
    EXPECT_STRNE(fds_xml_last_err(parser), err_msg);
}

TEST_F(Parse, opts_flag)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, "name", FDS_OPTS_T_UINT, FDS_OPTS_P_OPT),
            FDS_OPTS_END
    };
    fds_xml_set_args(parser, args);

    const char *mem =
            "<root>"
            "</root>";

    EXPECT_NE(fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx_t *) NULL);
    EXPECT_STREQ(fds_xml_last_err(parser), err_msg);
}

TEST_F(Parse, valid)
{
    const struct fds_xml_cont *content;
    fds_xml_ctx *ctx;

    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, "name", FDS_OPTS_T_UINT, 0),
            FDS_OPTS_END
    };
    const char *mem =
            "<root>"
                "<name>300</name>"
            "</root>";

    fds_xml_set_args(parser, args);

    EXPECT_NE(ctx = fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx_t *) NULL);
    EXPECT_STREQ(fds_xml_last_err(parser), err_msg);
    EXPECT_NE(fds_xml_next(ctx, &content), FDS_EOC);

    EXPECT_EQ(content->id, 1);
    EXPECT_EQ(content->type, FDS_OPTS_T_UINT);
    EXPECT_EQ(content->val_uint, (uint64_t) 300);
}

TEST_F(Parse, one_more_element)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, "name", FDS_OPTS_T_UINT, 0),
            FDS_OPTS_ELEM(2, "second", FDS_OPTS_T_STRING, 0),
            FDS_OPTS_END
    };

    const char *mem =
            "<root>"
                "<name>300</name>"
                "<second>retezec</second>"
                "<third>42</third>"
            "</root>";

    fds_xml_set_args(parser, args);
    EXPECT_NE(fds_xml_parse_mem(parser, mem, false), (fds_xml_ctx_t *) NULL);
    EXPECT_EQ(fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx_t *) NULL);
    EXPECT_STRNE(fds_xml_last_err(parser), err_msg);
}

TEST_F(Parse, optional)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, "name", FDS_OPTS_T_UINT, 0),
            FDS_OPTS_ELEM(2, "opt", FDS_OPTS_T_UINT, FDS_OPTS_P_OPT),
            FDS_OPTS_END
    };

    const char *mem =
            "<root>"
                    "<name>300</name>"
                    "<opt>1</opt>"
                    "<opt>2</opt>"
            "</root>";

    fds_xml_set_args(parser, args);
    EXPECT_EQ(fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx_t *) NULL);
    EXPECT_STRNE(fds_xml_last_err(parser), err_msg);
}

TEST_F(Parse, no_trim)
{
    fds_xml_ctx *ctx;
    const struct fds_xml_cont *content;
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, "name", FDS_OPTS_T_STRING, FDS_OPTS_P_NOTRIM),
            FDS_OPTS_END
    };

    const char *mem =
            "<root>"
                    "<name>  retezec  </name>"
            "</root>";
    fds_xml_set_args(parser, args);

    EXPECT_NE(ctx = fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx_t *) NULL);
    EXPECT_STREQ(fds_xml_last_err(parser), err_msg);
    EXPECT_NE(fds_xml_next(ctx, &content), FDS_EOC);

    EXPECT_EQ(content->id, 1);
    EXPECT_EQ(content->type, FDS_OPTS_T_STRING);
    EXPECT_STREQ(content->ptr_string, "  retezec  ");
}

TEST_F(Parse, multi)
{
    fds_xml_ctx *ctx;
    const struct fds_xml_cont *content;
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, "name", FDS_OPTS_T_STRING, FDS_OPTS_P_MULTI),
            FDS_OPTS_END
    };

    const char *mem =
            "<root>"
                    "<name>  retezec  </name>"
                    "<name>  retezec  </name>"
            "</root>";
    fds_xml_set_args(parser, args);

    EXPECT_NE(ctx = fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx_t *) NULL);
    EXPECT_STREQ(fds_xml_last_err(parser), err_msg);

    // first elem
    EXPECT_NE(fds_xml_next(ctx, &content), FDS_EOC);
    EXPECT_EQ(content->id, 1);
    EXPECT_EQ(content->type, FDS_OPTS_T_STRING);
    EXPECT_STREQ(content->ptr_string, "retezec");

    // second elem
    EXPECT_NE(fds_xml_next(ctx, &content), FDS_EOC);
    EXPECT_EQ(content->id, 1);
    EXPECT_EQ(content->type, FDS_OPTS_T_STRING);
    EXPECT_STREQ(content->ptr_string, "retezec");
}

TEST_F(Parse, no_multi)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, "name", FDS_OPTS_T_STRING, 0),
            FDS_OPTS_END
    };

    const char *mem =
            "<root>"
                    "<name>  retezec  </name>"
                    "<name>  retezec  </name>"
                    "</root>";
    fds_xml_set_args(parser, args);

    EXPECT_EQ(fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx_t *) NULL);
    EXPECT_STRNE(fds_xml_last_err(parser), err_msg);
}

TEST_F(Parse, text_component)
{
    fds_xml_ctx_t *ctx;
    const struct fds_xml_cont *content;

    const struct fds_xml_args args[] {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_TEXT(1, FDS_OPTS_T_STRING, 0),
            FDS_OPTS_END
    };

    const char *mem =
            "<root>"
            "text component"
            "</root>";
    fds_xml_set_args(parser, args);

    EXPECT_NE(ctx = fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx_t *) NULL);
    EXPECT_STREQ(fds_xml_last_err(parser), err_msg);

    fds_xml_next(ctx, &content);

    EXPECT_STREQ(content->ptr_string, "text component");
}

TEST_F(Parse, no_text_component)
{
    fds_xml_ctx_t *ctx;

    const struct fds_xml_args args[] {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_TEXT(1, FDS_OPTS_T_STRING, 0),
            FDS_OPTS_END
    };

    const char *mem =
            "<root>"
            "</root>";

    fds_xml_set_args(parser, args);

    EXPECT_EQ(ctx = fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx_t *) NULL);
    EXPECT_STRNE(fds_xml_last_err(parser), err_msg);
}

TEST_F(Parse, ignore_namespaces)
{
    fds_xml_ctx *ctx;
    const fds_xml_cont *cont;

    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, "value", FDS_OPTS_T_INT, 0),
            FDS_OPTS_END
    };
    fds_xml_set_args(parser, args);

    const char *mem =
            "<root xmlns:h=\"http://xmlsoft.org/namespaces.html\">"
                    "<h:value> 42 </h:value>"
            "</root>";

    EXPECT_NE(ctx = fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx *) NULL);
    EXPECT_STREQ(fds_xml_last_err(parser), err_msg);

    fds_xml_next(ctx, &cont);
    EXPECT_EQ(cont->val_int, 42);
}

TEST_F(Parse, bool_valid_values)
{
    fds_xml_ctx_t *ctx;
    const struct fds_xml_cont *content;

    const struct fds_xml_args args[] {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, "true", FDS_OPTS_T_BOOL, FDS_OPTS_P_MULTI),
            FDS_OPTS_ELEM(2, "false", FDS_OPTS_T_BOOL, FDS_OPTS_P_MULTI),
            FDS_OPTS_END
    };
    fds_xml_set_args(parser, args);

    const char *mem =
            "<root>"
                "<true>true</true>"
                "<true>1</true>"
                "<true>yes</true>"

                "<false>0</false>"
                "<false>false</false>"
                "<false>no</false>"
            "</root>";

    EXPECT_NE(ctx = fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx_t *) NULL);
    EXPECT_STREQ(fds_xml_last_err(parser), err_msg);

    for (int i = 0; i < 3; ++i) {
        fds_xml_next(ctx, &content);
        EXPECT_EQ(content->val_bool, true);
    }

    for (int i = 0; i < 3; ++i) {
        fds_xml_next(ctx, &content);
        EXPECT_EQ(content->val_bool, false);
    }
}

TEST_F(Parse, bool_wrong_value)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, "wrong", FDS_OPTS_T_BOOL, 0),
            FDS_OPTS_END
    };

    fds_xml_set_args(parser, args);

    const char *mem =
            "<root>"
                "<wrong>42</wrong>"
            "</root>";

    EXPECT_EQ(fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx *) NULL);
    EXPECT_STRNE(fds_xml_last_err(parser), err_msg);
}

TEST_F(Parse, uint_valid_value)
{
    fds_xml_ctx *ctx;
    const struct fds_xml_cont *content;

    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, "uint", FDS_OPTS_T_UINT, 0),
            FDS_OPTS_END
    };
    fds_xml_set_args(parser, args);

    const char *mem =
            "<root>"
                    "<uint>42</uint>"
                    "</root>";

    EXPECT_NE(ctx = fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx *) NULL);
    EXPECT_STREQ(fds_xml_last_err(parser), err_msg);

    fds_xml_next(ctx, &content);
    EXPECT_EQ(content->val_uint, (uint64_t) 42);
}

TEST_F(Parse, uint_text_instead_number)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, "wrong", FDS_OPTS_T_UINT, 0),
            FDS_OPTS_END
    };
    fds_xml_set_args(parser, args);

    const char *mem =
            "<root>"
                    "<wrong>text</wrong>"
                    "</root>";

    EXPECT_EQ(fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx *) NULL);
    EXPECT_STRNE(fds_xml_last_err(parser), err_msg);
}

TEST_F(Parse, int_valid_value)
{
    fds_xml_ctx *ctx;
    const struct fds_xml_cont *content;

    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, "int", FDS_OPTS_T_INT, 0),
            FDS_OPTS_END
    };
    fds_xml_set_args(parser, args);

    const char *mem =
            "<root>"
                "<int>42</int>"
            "</root>";

    EXPECT_NE(ctx = fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx *) NULL);
    EXPECT_STREQ(fds_xml_last_err(parser), err_msg);

    fds_xml_next(ctx, &content);
    EXPECT_EQ(content->val_int, 42);
}

TEST_F(Parse, int_text_instead_number)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, "wrong", FDS_OPTS_T_INT, 0),
            FDS_OPTS_END
    };
    fds_xml_set_args(parser, args);

    const char *mem =
            "<root>"
                "<wrong>text</wrong>"
            "</root>";

    EXPECT_EQ(fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx *) NULL);
    EXPECT_STRNE(fds_xml_last_err(parser), err_msg);
}

TEST_F(Parse, double_valid_value)
{
    fds_xml_ctx *ctx;
    const struct fds_xml_cont *content;

    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, "int", FDS_OPTS_T_DOUBLE, 0),
            FDS_OPTS_END
    };
    fds_xml_set_args(parser, args);

    const char *mem =
            "<root>"
                    "<int>42.3</int>"
                    "</root>";

    EXPECT_NE(ctx = fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx *) NULL);
    EXPECT_STREQ(fds_xml_last_err(parser), err_msg);

    fds_xml_next(ctx, &content);
    EXPECT_DOUBLE_EQ(content->val_double, 42.3);
}

TEST_F(Parse, double_text_instead_number)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, "wrong", FDS_OPTS_T_DOUBLE, 0),
            FDS_OPTS_END
    };
    fds_xml_set_args(parser, args);

    const char *mem =
            "<root>"
                "<wrong>text</wrong>"
            "</root>";

    EXPECT_EQ(fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx *) NULL);
    EXPECT_STRNE(fds_xml_last_err(parser), err_msg);
}

TEST_F(Parse, properties_valid)
{
    fds_xml_ctx *ctx;
    const struct fds_xml_cont *content;

    const struct fds_xml_args nested[] = {
            FDS_OPTS_ATTR(2, "attr", FDS_OPTS_T_STRING, FDS_OPTS_P_NOTRIM),
            FDS_OPTS_END
    };
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_NESTED(1, "nes", nested, 0),
            FDS_OPTS_END
    };
    fds_xml_set_args(parser, args);

    const char *mem =
            "<root>"
                "<nes attr=\"  some text  \">"
                "</nes>"
            "</root>";

    EXPECT_NE(ctx = fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx *) NULL);
    EXPECT_STREQ(fds_xml_last_err(parser), err_msg);

    // get attribute
    EXPECT_NE(fds_xml_next(ctx, &content), FDS_EOC);
    ctx = content->ptr_ctx;
    EXPECT_NE(fds_xml_next(ctx, &content), FDS_EOC);
    EXPECT_STREQ(content->ptr_string, "  some text  ");
}

TEST_F(Parse, properties_not_defined)
{
    const struct fds_xml_args nested[] = {
            FDS_OPTS_END
    };
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_NESTED(1, "nes", nested, 0),
            FDS_OPTS_END
    };
    fds_xml_set_args(parser, args);

    const char *mem =
            "<root>"
                "<nes attr=\"some text\"> </nes>"
            "</root>";

    EXPECT_NE(fds_xml_parse_mem(parser, mem, false), (fds_xml_ctx *) NULL);
    EXPECT_STREQ(fds_xml_last_err(parser), err_msg);

    EXPECT_EQ(fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx *) NULL);
    EXPECT_STRNE(fds_xml_last_err(parser), err_msg);
}

TEST_F(Parse, content_not_defined)
{
    const struct fds_xml_args nested[] = {
            FDS_OPTS_END
    };
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_NESTED(1, "nes", nested, 0),
            FDS_OPTS_END
    };
    fds_xml_set_args(parser, args);

    const char *mem =
            "<root>"
                "<nes>"
                    "<con> 42 </con>"
                "</nes>"
            "</root>";

    EXPECT_EQ(fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx *) NULL);
    EXPECT_STRNE(fds_xml_last_err(parser), err_msg);
}

TEST_F(Parse, raw_valid)
{
    const struct fds_xml_cont *content;
    struct fds_xml_ctx *ctx = NULL;

    static const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_RAW(1, "raw", 0),
            FDS_OPTS_END
    };

    const char *mem =
            "<root>"
                "<raw>"
                "   <some_text>asdas</some_text>"
                "</raw>"
            "</root>";

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_OK);
    ctx = fds_xml_parse_mem(parser, mem, true);

    EXPECT_EQ(fds_xml_next(ctx, &content), FDS_OK);

    EXPECT_EQ(content->id, 1);
    EXPECT_EQ(content->type, FDS_OPTS_T_STRING);
    EXPECT_STREQ(content->ptr_string, "<raw>   <some_text>asdas</some_text></raw>");

    EXPECT_STREQ(fds_xml_last_err(parser), err_msg);
}

TEST_F(Parse, text_not_defined)
{
    const struct fds_xml_args nested[] = {
            FDS_OPTS_END
    };
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_NESTED(1, "nes", nested, 0),
            FDS_OPTS_END
    };
    fds_xml_set_args(parser, args);

    const char *mem =
            "<root>"
                "<nes> optional description"
                "</nes>"
            "</root>";

    EXPECT_EQ(fds_xml_parse_mem(parser, mem, true), (fds_xml_ctx *) NULL);
    EXPECT_STRNE(fds_xml_last_err(parser), err_msg);
}

