/**
 * @file File_exception.cpp
 * @author Lukas Hutak (lukas.hutak@cesnet.cz)
 * @date June 2019
 * @brief
 *   File_exception tests
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <gtest/gtest.h>
#include <libfds.h>

#include "../../../src/file/File_exception.hpp"

using namespace fds_file;

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// Throw string based exception
TEST(Exception, throwString)
{
    const std::string err_msg = "some message";
    bool got = false;
    try {
        throw File_exception(FDS_ERR_ARG, err_msg);
    } catch (File_exception &ex) {
        EXPECT_EQ(ex.code(), FDS_ERR_ARG);
        EXPECT_EQ(ex.what(), err_msg);
        got = true;
    } catch (...) {
        // Catch all
    }

    EXPECT_TRUE(got);
}

// Throw char array based exception
TEST(Exception, throwCharArray)
{
    const char err_msg[] = "some message";
    bool got = false;
    try {
        throw File_exception(FDS_ERR_ARG, err_msg);
    } catch (File_exception &ex) {
        EXPECT_EQ(ex.code(), FDS_ERR_ARG);
        EXPECT_STREQ(ex.what(), err_msg);
        got = true;
    } catch (...) {
        // Catch all
    }

    EXPECT_TRUE(got);
}

// Throw an errno based message with a user defined prefix message
TEST(Exception, throwErrnoWithPrefix)
{
    const std::string err_msg = "some message";
    bool got = false;

    try {
        errno = EAGAIN;
        File_exception::throw_errno(errno, err_msg, FDS_ERR_ARG);
    } catch (File_exception &ex) {
        EXPECT_EQ(ex.code(), FDS_ERR_ARG);
        EXPECT_EQ(std::string(ex.what()).rfind(err_msg, 0), 0U); // user defined message as prefix
        got = true;
    } catch (...) {
        // Catch all
    }

    EXPECT_TRUE(got);
}

// Throw an errno based message without a user defined prefix message
TEST(Exception, throwErrnoWithoutPrefix)
{
    bool got = false;

    try {
        errno = EAGAIN;
        File_exception::throw_errno(errno);
    } catch (File_exception &ex) {
        EXPECT_EQ(ex.code(), FDS_ERR_INTERNAL);
        got = true;
    } catch (...) {
        // Catch all
    }

    EXPECT_TRUE(got);
}
