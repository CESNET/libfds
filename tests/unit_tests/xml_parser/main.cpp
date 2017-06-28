#include <gtest/gtest.h>
#include <libxml2/libxml/parser.h>

extern "C" {
	#include <libfds/xml_parser.h>
}

int main(int argc, char **argv)
{
    bool a = NULL;
    std::cout << a << std::endl;

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
// TODO jak zkontrolovat neinicializovanou promennou?

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
    EXPECT_EQ(fds_xml_create(&parser), FDS_XML_OK);
}

TEST_F(Create, parser_null)
{
    EXPECT_EQ(fds_xml_create(NULL), FDS_XML_ERR_FMT);
}

/**
 * fds_xml_destroy
 */
class Destroy : public ::testing::Test
{
protected:
    fds_xml_t *parser = NULL;
};

TEST_F(Destroy, all)
{
    fds_xml_destroy(parser);

    EXPECT_EQ(parser, (fds_xml_t *) NULL);
}

TEST_F(Destroy, valid)
{
    fds_xml_create(&parser);
    fds_xml_destroy(parser);
    //EXPECT_EQ(parser, (fds_xml_t *) NULL); // TODO
}

/**
 * fds_xml_last_err
 */
class Last_err : public ::testing::Test
{
protected:
    fds_xml_t *parser = NULL;

    virtual void TearDown() {
        fds_xml_destroy(parser);
    }
};

TEST_F(Last_err, valid)
{
    fds_xml_create(&parser);
    fds_xml_parse(parser, NULL, true);

    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Last_err, parser_null)
{
    EXPECT_EQ(fds_xml_last_err(NULL), (char *) NULL);
}

/**
 * fds_xml_set_args
 */
class Set_args : public ::testing::Test
{
protected:
    fds_xml_t *parser = NULL;
    virtual void SetUp() {
        fds_xml_create(&parser);
    }

    virtual void TearDown() {
        fds_xml_destroy(parser);
    }
};

TEST_F(Set_args, opts_null)
{
    EXPECT_EQ(fds_xml_set_args(NULL, parser), FDS_XML_ERR_FMT);
}

TEST_F(Set_args, parser_null)
{
    const fds_xml_args args_main[] = {
            OPTS_ROOT("root"),
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args_main, NULL), FDS_XML_ERR_FMT);
}

TEST_F(Set_args, no_root)
{
    const fds_xml_args args_main[] = {
            OPTS_ELEM(1, "timeout", OPTS_T_INT, 0),
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args_main, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, root_context_type)
{
    const struct fds_xml_args args[] = {
            {OPTS_C_ROOT, OPTS_T_CONTEXT, 0, "root", NULL, 0},
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, root_uint_type)
{
    const struct fds_xml_args args[] = {
            {OPTS_C_ROOT, OPTS_T_UINT, 0, "root", NULL, 0},
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, root_negative_id)
{
    const struct fds_xml_args args[] = {
            {OPTS_C_ROOT, OPTS_T_NONE, -1, "root", NULL, 0},
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, root_no_name)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT(NULL),
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, root_two_same_name)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_ROOT("root"),
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, root_nested)
{
    const struct fds_xml_args nested[] = {
            OPTS_END
    };
    const struct fds_xml_args args[] = {
            {OPTS_C_ROOT, OPTS_T_NONE, 1, "root", nested, 0},
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, root_set_flags)
{
    const struct fds_xml_args args[] = {
            {OPTS_C_ROOT, OPTS_T_NONE, 1, "root", NULL, OPTS_P_MULTI},
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, elem_wrong_type)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_ELEM(1, "name", OPTS_T_CONTEXT, 0),
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, elem_negative_id)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_ELEM(-1, "name", OPTS_T_CONTEXT, 0),
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, elem_no_name)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_ELEM(1, NULL, OPTS_T_NONE, 0),
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, elem_nested)
{
    const struct fds_xml_args nested[] = {
            OPTS_END
    };
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            {OPTS_C_ELEMENT, OPTS_T_UINT, 1, "root", nested, OPTS_P_MULTI},
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, elem_negative_flags)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            {OPTS_C_ELEMENT, OPTS_T_UINT, 1, "root", NULL, -1},
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, elem_same_def)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_ELEM(1, "elem", OPTS_T_UINT, 0),
            OPTS_ELEM(2, "elem", OPTS_T_UINT, 0),
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, elem_same_ids)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_ELEM(1, "elem2", OPTS_T_UINT, 0),
            OPTS_ELEM(1, "elem1", OPTS_T_UINT, 0),
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, attr_wrong_type)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_ATTR(1, "name", OPTS_T_NONE, 0),
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, attr_negative_id)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_ATTR(-1, "name", OPTS_T_NONE, 0),
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, attr_no_name)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_ATTR(1, NULL, OPTS_T_UINT, 0),
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, attr_same_name)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_ATTR(1, "name", OPTS_T_UINT, 0),
            OPTS_ATTR(2, "name", OPTS_T_UINT, 0),
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, attr_multi_flag)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_ATTR(1, "name", OPTS_T_UINT, OPTS_P_MULTI),
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, attr_nested)
{
    const struct fds_xml_args nested[] = {
            OPTS_ROOT("nested"),
            OPTS_END
    };
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            {OPTS_C_ATTR, OPTS_T_UINT, 1, "name", nested, 0},
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, end_wrong_type)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            {OPTS_C_TERMINATOR, OPTS_T_INT, 0, NULL, NULL, 0},
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, end_negative_id)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            {OPTS_C_TERMINATOR, OPTS_T_NONE, -1, NULL, NULL, 0},
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, end_with_name)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            {OPTS_C_TERMINATOR, OPTS_T_NONE, -1, "name", NULL, 0},
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, end_nested)
{
    const struct fds_xml_args nested[] = {
            OPTS_ROOT("nested"),
            OPTS_END
    };
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            {OPTS_C_TERMINATOR, OPTS_T_UINT, 1, "name", nested, 0},
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, end_set_flags)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            {OPTS_C_TERMINATOR, OPTS_T_UINT, 1, NULL, NULL, OPTS_P_MULTI},
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, end_on_first_place)
{
    const struct fds_xml_args args[] = {
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, text_wrong_type)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_TEXT(1, OPTS_T_CONTEXT, 0),
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, text_negative_id)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_TEXT(-1, OPTS_T_STRING, 0),
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, text_with_name)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            {OPTS_C_TEXT, OPTS_T_STRING, 1, "name", NULL, 0},
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, text_nested)
{
    const struct fds_xml_args nested[] = {
            OPTS_ROOT("nested"),
            OPTS_END
    };
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            {OPTS_C_TEXT, OPTS_T_STRING, 1, NULL, nested, 0},
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, text_same_def)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_TEXT(1, OPTS_T_STRING, 0),
            OPTS_TEXT(2, OPTS_T_STRING, 0),
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, nested_wrong_type)
{
    const struct fds_xml_args nested[] = {
            OPTS_ROOT("nested"),
            OPTS_END
    };
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            {OPTS_C_NESTED, OPTS_T_UINT, 1, "name", nested, 0},
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, nested_negative_id)
{
    const struct fds_xml_args nested[] = {
            OPTS_ROOT("nested"),
            OPTS_END
    };
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            {OPTS_C_NESTED, OPTS_T_CONTEXT, -1, "name", nested, 0},
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, nested_no_name)
{
    const struct fds_xml_args nested[] = {
            OPTS_ROOT("nested"),
            OPTS_END
    };
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_NESTED(1, NULL, nested, 0),
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, nested_no_next)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_NESTED(1, "name", NULL, 0),
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, nested_same_name)
{
    const struct fds_xml_args nested[] = {
            OPTS_ROOT("nested"),
            OPTS_END
    };
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_NESTED(1, "name", nested, 0),
            OPTS_NESTED(2, "name", nested, 0),
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, nested_cyclic) // TODO
{
//    static const struct fds_xml_args args[];
//
//    static const struct fds_xml_args nested[] = {
//            OPTS_ROOT("nested"),
//            OPTS_NESTED(2, "args", args, 0),
//            OPTS_END
//    };
//    static const struct fds_xml_args args[] = {
//            OPTS_ROOT("root"),
//            OPTS_NESTED(1, "nested", nested, 0),
//            OPTS_END
//    };
//
//    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
//    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, no_end)
{
    static const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

/**
 * fds_xml_next
 */
class Next : public ::testing::Test
{
protected:
    fds_xml_ctx_t *ctx = NULL;
    struct fds_xml_cont content;
    fds_xml_t *parser;

    virtual void SetUp() {
        fds_xml_create(&parser);
    }

    virtual void TearDown() {
        fds_xml_destroy(parser);
    }
};

TEST_F(Next, all_null)
{
    EXPECT_EQ(fds_xml_next(NULL, &content), FDS_XML_ERR_FMT);
    EXPECT_EQ(fds_xml_next(ctx, NULL), FDS_XML_ERR_FMT);
    EXPECT_EQ(fds_xml_next(NULL, NULL), FDS_XML_ERR_FMT);
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
    fds_xml_set_args(args, parser);
    ctx = fds_xml_parse(parser, mem, true);

    fds_xml_cont content_prev;
    fds_xml_cont content_last;

    fds_xml_next(ctx, &content_prev);
    fds_xml_next(ctx, &content_last);

    EXPECT_NE(content_prev.id, content_last.id);
    EXPECT_NE(content_prev.type, content_last.type);
}

/**
 * fds_xml_parse
 */
class Parse : public ::testing::Test
{
public:
    fds_xml_t *parser = NULL;
    const char *err_msg = (const char *) "No error";

    virtual void SetUp() {
        fds_xml_create(&parser);
    }

    virtual void TearDown() {
        fds_xml_destroy(parser);
    }
};

TEST_F(Parse, inputs_null)
{
    const char *mem = "ABCD";

    EXPECT_EQ(fds_xml_parse(NULL, mem, true), (fds_xml_ctx_t *) NULL);
    EXPECT_EQ(fds_xml_parse(parser, NULL, false), (fds_xml_ctx_t *) NULL);
}

TEST_F(Parse, xml_file_wrong)
{
    const char *mem = "ABCD";

    EXPECT_EQ(fds_xml_parse(parser, mem, true), (fds_xml_ctx_t *) NULL);
    EXPECT_NE(fds_xml_last_err(parser), err_msg);
}

TEST_F(Parse, missing_element)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_END
    };

    const char *mem =
            "<root>"
                    "<name>300</name>"
            "</root>";

    fds_xml_set_args(args, parser);
    EXPECT_EQ(fds_xml_parse(parser, mem, true), (fds_xml_ctx_t *) NULL);
    EXPECT_NE(fds_xml_last_err(parser), err_msg);
}

TEST_F(Parse, valid) // TODO
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_ELEM(1, "name", OPTS_T_UINT, 0),
            OPTS_END
    };

    fds_xml_set_args(args, parser);
}

TEST_F(Parse, one_more_element)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_ELEM(1, "name", OPTS_T_UINT, 0),
            OPTS_ELEM(2, "second", OPTS_T_STRING, 0),
            OPTS_END
    };

    const char *mem =
            "<root>"
                    "<name>300</name>"
                    "</root>";

    fds_xml_set_args(args, parser);
    EXPECT_EQ(fds_xml_parse(parser, mem, true), (fds_xml_ctx_t *) NULL);
    EXPECT_NE(fds_xml_last_err(parser), err_msg);
}

TEST_F(Parse, optional)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_ELEM(1, "name", OPTS_T_UINT, 0),
            OPTS_ELEM(2, "opt", OPTS_T_UINT, OPTS_P_OPT),
            OPTS_END
    };

    const char *mem =
            "<root>"
                    "<name>300</name>"
                    "<opt>1</opt>"
                    "<opt>2</opt>"
            "</root>";

    fds_xml_set_args(args, parser);
    EXPECT_EQ(fds_xml_parse(parser, mem, true), (fds_xml_ctx_t *) NULL);
    EXPECT_NE(fds_xml_last_err(parser), err_msg);
}

/**
 * fds_xml_rewind
 */
class Rewind : public ::testing::Test
{
protected:
    fds_xml_ctx_t *ctx;
    fds_xml_t *parser;

    virtual void SetUp() {
        fds_xml_create(&parser);

    }

    virtual void TearDown(){
        fds_xml_destroy(parser);
    }
};

TEST_F(Rewind, ctx_null)
{
    fds_xml_rewind(NULL);
}

TEST_F(Rewind, nested) {
    static const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_ELEM(1, "elem", OPTS_T_STRING, 0),
            OPTS_END
    };
    const char *mem =
            "<root>"
            "   <elem>    retezec    </elem>"
            "</root>";
    fds_xml_set_args(args, parser);
    ctx = fds_xml_parse(parser, mem, true);

    fds_xml_cont content_prev;
    fds_xml_cont content_after;
    fds_xml_next(ctx, &content_prev);

    fds_xml_rewind(ctx);
    fds_xml_next(ctx, &content_after);

    EXPECT_EQ(content_prev.id, content_after.id);
    EXPECT_EQ(content_prev.type, content_after.type);
    EXPECT_EQ(content_prev.ptr_string, content_after.ptr_string);
}

TEST_F(Rewind, over)
{
    static const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_ELEM(1, "elem", OPTS_T_STRING, 0),
            OPTS_END
    };
    const char *mem =
            "<root>"
                    "   <elem>    retezec    </elem>"
                    "</root>";
    fds_xml_set_args(args, parser);
    ctx = fds_xml_parse(parser, mem, true);

    fds_xml_cont content_prev;
    fds_xml_cont content_after;
    fds_xml_next(ctx, &content_prev);

    fds_xml_rewind(ctx);
    fds_xml_next(ctx, &content_after);


    EXPECT_EQ(fds_xml_next(ctx, &content_after), FDS_XML_EOC);
}
