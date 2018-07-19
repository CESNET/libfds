#include <gtest/gtest.h>
#include <libfds.h>

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
        parser = fds_xml_create();
    }

    virtual void TearDown() {
        fds_xml_destroy(parser);
    }
};

TEST_F(Set_args, working)
{
    static const struct fds_xml_args nested[] = {
            FDS_OPTS_ELEM(2, "na", FDS_OPTS_T_INT, 0),
            FDS_OPTS_END
    };
    static const fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_NESTED(1, "name", nested, 0),
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_OK);
}

TEST_F(Set_args, opts_null)
{
    EXPECT_EQ(fds_xml_set_args(parser, NULL), FDS_ERR_FORMAT);
}

TEST_F(Set_args, no_root)
{
    const fds_xml_args args_main[] = {
            FDS_OPTS_ELEM(1, "timeout", FDS_OPTS_T_INT, 0),
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args_main), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, root_context_type)
{
    const struct fds_xml_args args[] = {
            {FDS_OPTS_C_ROOT, FDS_OPTS_T_CONTEXT, 0, "root", NULL, 0},
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, root_uint_type)
{
    const struct fds_xml_args args[] = {
            {FDS_OPTS_C_ROOT, FDS_OPTS_T_UINT, 0, "root", NULL, 0},
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, root_negative_id)
{
    const struct fds_xml_args args[] = {
            {FDS_OPTS_C_ROOT, FDS_OPTS_T_NONE, -1, "root", NULL, 0},
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, root_no_name)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT(NULL),
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, root_two_same_name)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, root_nested)
{
    const struct fds_xml_args nested[] = {
            FDS_OPTS_END
    };
    const struct fds_xml_args args[] = {
            {FDS_OPTS_C_ROOT, FDS_OPTS_T_NONE, 1, "root", nested, 0},
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, root_set_flags)
{
    const struct fds_xml_args args[] = {
            {FDS_OPTS_C_ROOT, FDS_OPTS_T_NONE, 1, "root", NULL, FDS_OPTS_P_MULTI},
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, elem_wrong_type)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, "name", FDS_OPTS_T_CONTEXT, 0),
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, elem_negative_id)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(-1, "name", FDS_OPTS_T_CONTEXT, 0),
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, elem_no_name)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, NULL, FDS_OPTS_T_NONE, 0),
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, elem_nested)
{
    const struct fds_xml_args nested[] = {
            FDS_OPTS_END
    };
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            {FDS_OPTS_C_ELEMENT, FDS_OPTS_T_UINT, 1, "root", nested, FDS_OPTS_P_MULTI},
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, elem_negative_flags)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            {FDS_OPTS_C_ELEMENT, FDS_OPTS_T_UINT, 1, "root", NULL, -1},
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, elem_same_def)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, "elem", FDS_OPTS_T_UINT, 0),
            FDS_OPTS_ELEM(2, "elem", FDS_OPTS_T_UINT, 0),
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, elem_same_ids)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ELEM(1, "elem2", FDS_OPTS_T_UINT, 0),
            FDS_OPTS_ELEM(1, "elem1", FDS_OPTS_T_UINT, 0),
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, attr_wrong_type)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ATTR(1, "name", FDS_OPTS_T_NONE, 0),
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, attr_negative_id)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ATTR(-1, "name", FDS_OPTS_T_NONE, 0),
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, attr_no_name)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ATTR(1, NULL, FDS_OPTS_T_UINT, 0),
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, attr_same_name)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ATTR(1, "name", FDS_OPTS_T_UINT, 0),
            FDS_OPTS_ATTR(2, "name", FDS_OPTS_T_UINT, 0),
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, attr_multi_flag)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_ATTR(1, "name", FDS_OPTS_T_UINT, FDS_OPTS_P_MULTI),
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, attr_nested)
{
    const struct fds_xml_args nested[] = {
            FDS_OPTS_END
    };
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            {FDS_OPTS_C_ATTR, FDS_OPTS_T_UINT, 1, "name", nested, 0},
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, end_wrong_type)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            {FDS_OPTS_C_TERMINATOR, FDS_OPTS_T_INT, 0, NULL, NULL, 0},
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, end_negative_id)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            {FDS_OPTS_C_TERMINATOR, FDS_OPTS_T_NONE, -1, NULL, NULL, 0},
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, end_with_name)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            {FDS_OPTS_C_TERMINATOR, FDS_OPTS_T_NONE, -1, "name", NULL, 0},
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, end_nested)
{
    const struct fds_xml_args nested[] = {
            FDS_OPTS_END
    };
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            {FDS_OPTS_C_TERMINATOR, FDS_OPTS_T_UINT, 1, "name", nested, 0},
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, end_set_flags)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            {FDS_OPTS_C_TERMINATOR, FDS_OPTS_T_UINT, 1, NULL, NULL, FDS_OPTS_P_MULTI},
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, end_on_first_place)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, text_wrong_type)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_TEXT(1, FDS_OPTS_T_CONTEXT, 0),
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, text_negative_id)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_TEXT(-1, FDS_OPTS_T_STRING, 0),
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, text_with_name)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            {FDS_OPTS_C_TEXT, FDS_OPTS_T_STRING, 1, "name", NULL, 0},
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, text_nested)
{
    const struct fds_xml_args nested[] = {
            FDS_OPTS_END
    };
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            {FDS_OPTS_C_TEXT, FDS_OPTS_T_STRING, 1, NULL, nested, 0},
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, text_same_def)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_TEXT(1, FDS_OPTS_T_STRING, 0),
            FDS_OPTS_TEXT(2, FDS_OPTS_T_STRING, 0),
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, nested_wrong_type)
{
    const struct fds_xml_args nested[] = {
            FDS_OPTS_END
    };
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            {FDS_OPTS_C_NESTED, FDS_OPTS_T_UINT, 1, "name", nested, 0},
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, nested_negative_id)
{
    const struct fds_xml_args nested[] = {
            FDS_OPTS_END
    };
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            {FDS_OPTS_C_NESTED, FDS_OPTS_T_CONTEXT, -1, "name", nested, 0},
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, nested_no_name)
{
    const struct fds_xml_args nested[] = {
            FDS_OPTS_END
    };
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_NESTED(1, NULL, nested, 0),
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, nested_no_next)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_NESTED(1, "name", NULL, 0),
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, nested_same_name)
{
    const struct fds_xml_args nested[] = {
            FDS_OPTS_END
    };
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_NESTED(1, "name", nested, 0),
            FDS_OPTS_NESTED(2, "name", nested, 0),
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, nested_cyclic)
{
    struct fds_xml_args main_args[4];

    struct fds_xml_args nested[] = {
            FDS_OPTS_NESTED(2, "args", main_args, 0),
            FDS_OPTS_END
    };

    main_args[0] = FDS_OPTS_ROOT("root");
    main_args[1] = FDS_OPTS_ELEM(4, "name", FDS_OPTS_T_STRING, 0);
    main_args[2] = FDS_OPTS_NESTED(1, "nested", nested, 0);
    main_args[3] = FDS_OPTS_END;

    EXPECT_EQ(fds_xml_set_args(parser, main_args), FDS_OK);
    EXPECT_STREQ(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, raw_wrong_type)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            {FDS_OPTS_C_RAW, FDS_OPTS_T_UINT, 1, "name", NULL, 0},
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, raw_negative_id)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            {FDS_OPTS_C_RAW, FDS_OPTS_T_STRING, -1, "name", NULL, 0},
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, raw_no_name)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_RAW(1, NULL, 0),
            FDS_OPTS_END
    };


    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, raw_same_name)
{
    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            FDS_OPTS_RAW(1, "name", 0),
            FDS_OPTS_RAW(2, "name", 0),
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}

TEST_F(Set_args, raw_nested)
{
    const struct fds_xml_args nested[] = {
            FDS_OPTS_END
    };

    const struct fds_xml_args args[] = {
            FDS_OPTS_ROOT("root"),
            {FDS_OPTS_C_RAW, FDS_OPTS_T_STRING, 1, "name", nested, 0},
            FDS_OPTS_END
    };

    EXPECT_EQ(fds_xml_set_args(parser, args), FDS_ERR_FORMAT);
    EXPECT_NE(fds_xml_last_err(parser), "No error");
}
