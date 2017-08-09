#include <gtest/gtest.h>
#include <libxml2/libxml/parser.h>

extern "C" {
	#include <libfds/xml_parser.h>
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
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

TEST_F(Set_args, working)
{
    static const struct fds_xml_args nested[] = {
            OPTS_ELEM(2, "na", OPTS_T_INT, 0),
            OPTS_END
    };
    static const fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_NESTED(1, "name", nested, 0),
            OPTS_END
    };

    fds_xml_t *parser = nullptr;
    fds_xml_create(&parser);

    fds_xml_set_args(args, parser);
    fds_xml_destroy(parser);
}

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

TEST_F(Set_args, nested_cyclic)
{
    struct fds_xml_args main_args[4];

    struct fds_xml_args nested[] = {
            OPTS_NESTED(2, "args", main_args, 0),
            OPTS_END
    };

    main_args[0] = OPTS_ROOT("root");
    main_args[1] = OPTS_ELEM(4, "name", OPTS_T_STRING, 0);
    main_args[2] = OPTS_NESTED(1, "nested", nested, 0);
    main_args[3] = OPTS_END;

    EXPECT_EQ(fds_xml_set_args(main_args, parser), FDS_XML_OK);
    EXPECT_STREQ(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, raw_wrong_type)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            {OPTS_C_RAW, OPTS_T_UINT, 1, "name", NULL, 0},
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, raw_negative_id)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            {OPTS_C_RAW, OPTS_T_STRING, -1, "name", NULL, 0},
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, raw_no_name)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_RAW(1, NULL, 0),
            OPTS_END
    };


    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, raw_same_name)
{
    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            OPTS_RAW(1, "name", 0),
            OPTS_RAW(2, "name", 0),
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, raw_nested)
{
    const struct fds_xml_args nested[] = {
            OPTS_END
    };

    const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
            {OPTS_C_RAW, OPTS_T_STRING, 1, "name", nested, 0},
            OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

// TODO not use -- dangerous
//TEST_F(Set_args, no_end)
//{
//    static const struct fds_xml_args args[] = {
//            OPTS_ROOT("root"),
//    };
//
//    EXPECT_EQ(fds_xml_set_args(args, parser), FDS_XML_ERR_FMT);
//    EXPECT_NE(fds_xml_last_err(parser), "No error");
//}

