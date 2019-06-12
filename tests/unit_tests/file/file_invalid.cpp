/**
 * @file file_invalid.cpp
 * @author Lukas Hutak (lukas.hutak@cesnet.cz)
 * @date June 2019
 * @brief
 *   Simple test cases using FDS File API
 *
 * The tests usually try perform invalid operation and expects appropriate error codes.
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "wr_env.hpp"

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// Run all tests independently for all following combinations of compression algorithms and I/Os
uint32_t flags_comp[] = {0};
uint32_t flags_io[] = {0};
bool with_ie_mgr[] = {false};
auto product = ::testing::Combine(::testing::ValuesIn(flags_comp), ::testing::ValuesIn(flags_io),
    ::testing::ValuesIn(with_ie_mgr));
INSTANTIATE_TEST_CASE_P(Invalid, FileAPI, product, &product_name);


// Try to open file without using proper read/write flags
TEST_P(FileAPI, openWithNoFlags)
{
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    //EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    EXPECT_EQ(fds_file_open(file.get(), m_filename.c_str(), 0), FDS_ERR_ARG);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
}

// Try to open file without filename
TEST_P(FileAPI, openWithNoFilename)
{
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    EXPECT_EQ(fds_file_open(file.get(), nullptr, m_flags_write), FDS_ERR_ARG);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);

    file.reset(fds_file_init());
    EXPECT_EQ(fds_file_open(file.get(), nullptr, m_flags_read), FDS_ERR_ARG);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
}

// Try to open folder for writing and reading
TEST_P(FileAPI, openFolder)
{
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    EXPECT_EQ(fds_file_open(file.get(), "./data", m_flags_write), FDS_ERR_INTERNAL);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);

    file.reset(fds_file_init());
    EXPECT_EQ(fds_file_open(file.get(), "./data", m_flags_read), FDS_ERR_INTERNAL);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
}

// Try to open non-existing file for reading
TEST_P(FileAPI, openNonExistingFile)
{
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    EXPECT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_ERR_INTERNAL);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
}

// Try to get statistics without opening a file
TEST_P(FileAPI, statsGetNoFile)
{
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    EXPECT_EQ(fds_file_stats_get(file.get()), nullptr);
}

// Try to add a Transport Session in the reader mode
TEST_P(FileAPI, sessionAddInvalidMode)
{
    // First of all, create an empty file
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);

    // Open the file in the reader mode
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);

    // Try to add a Transport Session
    Session session_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_UDP};
    fds_file_sid_t sid;

    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    ASSERT_EQ(fds_file_session_add(file.get(), session_def.get(), &sid), FDS_ERR_DENIED);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
}

// Try to add Transport Session with invalid specification of arguments
TEST_P(FileAPI, sessionAddInvalidArgs)
{
    // First of all, create an empty file
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);

    // Try to add a Transport Session with invalid argument specification
    Session session_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_UDP};
    fds_file_sid_t sid;

    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    ASSERT_EQ(fds_file_session_add(file.get(), nullptr, nullptr), FDS_ERR_ARG);
    ASSERT_EQ(fds_file_session_add(file.get(), nullptr, &sid), FDS_ERR_ARG);
    ASSERT_EQ(fds_file_session_add(file.get(), session_def.get(), nullptr), FDS_ERR_ARG);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
}

// Try to add maximum possible number of Transport Sessions
TEST_P(FileAPI, sessionAddMaxNumberOfSessions)
{
    // Create a file
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);

    // Try to add maximum possible number of Transport Sessions
    size_t max_cnt = 65535; // based on documentation of fds_file_session_add()
    for (size_t i = 0; i < max_cnt; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));
        // Create a new definition
        uint16_t src_port = i / 256;
        uint16_t dst_port = i % 256;

        Session session_def {"10.0.10.12", "127.0.0.1", src_port, dst_port, FDS_FILE_SESSION_TCP};
        fds_file_sid_t sid;

        EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
        ASSERT_EQ(fds_file_session_add(file.get(), session_def.get(), &sid), FDS_OK);
        EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    }

    // It should fail now
    Session session_def {"8.8.8.8", "1.1.1.1", 10000, 11324, FDS_FILE_SESSION_UDP};
    fds_file_sid_t sid;
    ASSERT_EQ(fds_file_session_add(file.get(), session_def.get(), &sid), FDS_ERR_DENIED);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);

    // Try to list all Sessions
    fds_file_sid_t *list_data;
    size_t list_size;
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, max_cnt);
    for (size_t i = 0; i < list_size; ++i) {
        // Try to get its definition
        SCOPED_TRACE("i: " + std::to_string(i));
        const struct fds_file_session *info;
        ASSERT_EQ(fds_file_session_get(file.get(), list_data[i], &info), FDS_OK);
        EXPECT_EQ(info->proto, FDS_FILE_SESSION_TCP);
    }
    free(list_data);

    // Reopen the file for appending and try to add more Transport Sessions
    uint32_t append_flags = write2append_flag(m_flags_write);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), append_flags), FDS_OK);

    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    Session session_def2 {"192.168.0.1", "192.168.0.2", 11324, 4739, FDS_FILE_SESSION_UDP};
    ASSERT_EQ(fds_file_session_add(file.get(), session_def2.get(), &sid), FDS_ERR_DENIED);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);

    // Reopen the file for reading and list all Sessions
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, max_cnt);

    for (size_t i = 0; i < list_size; ++i) {
        // Try to get its definition
        SCOPED_TRACE("i: " + std::to_string(i));
        const struct fds_file_session *info;
        ASSERT_EQ(fds_file_session_get(file.get(), list_data[i], &info), FDS_OK);
        EXPECT_EQ(info->proto, FDS_FILE_SESSION_TCP);
    }
    free(list_data);
}

// Try to get a definition of non-existing Transport Session
TEST_P(FileAPI, sessionGetNonExistingSession)
{
    fds_file_sid_t sid = 0;
    const struct fds_file_session *info = nullptr;

    // Open a file for writting
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);

    // Try to get a Transport Sessions
    ASSERT_EQ(fds_file_session_get(file.get(), sid, &info), FDS_ERR_NOTFOUND);
    EXPECT_EQ(info, nullptr);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);

    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);
    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);

    // Try to get a Transport Sessions
    ASSERT_EQ(fds_file_session_get(file.get(), sid, &info), FDS_ERR_NOTFOUND);
    EXPECT_EQ(info, nullptr);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
}

// Try to a definition of Transport Session with invalid function arguments
TEST_P(FileAPI, sessionGetInvalidArgs)
{
    // Transport Session defintion
    Session session_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_UDP};
    fds_file_sid_t sid;

    // Open a file for writting
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session_def.get(), &sid), FDS_OK);
    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    // Try to get the Transport Sessions definition with invalid argument(s)
    EXPECT_EQ(fds_file_session_get(file.get(), sid, nullptr), FDS_ERR_ARG);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);

    // Open the file for reading
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);

    // Get list of Transport Sessions
    fds_file_sid_t *list_data;
    size_t list_size;
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 1U);
    // Try to get the Transport Sessions definition with invalid argument(s)
    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    ASSERT_EQ(fds_file_session_get(file.get(), list_data[0], nullptr), FDS_ERR_ARG);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
    free(list_data);
}

// Try to get a list of Transport Sessions with invalid function arguments
TEST_P(FileAPI, sessionListInvalidArgs)
{
    // Transport Session defintion
    Session session_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_UDP};
    fds_file_sid_t sid;

    // Open a file for writting and add at least one Transport Session
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session_def.get(), &sid), FDS_OK);

    fds_file_sid_t *list_data;
    size_t list_size;
    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    EXPECT_EQ(fds_file_session_list(file.get(), &list_data, nullptr), FDS_ERR_ARG);
    EXPECT_EQ(fds_file_session_list(file.get(), nullptr, &list_size), FDS_ERR_ARG);
    EXPECT_EQ(fds_file_session_list(file.get(), nullptr, nullptr), FDS_ERR_ARG);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);

    // Open the file for reading
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);

    // Get list of Transport Sessions
    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    EXPECT_EQ(fds_file_session_list(file.get(), &list_data, nullptr), FDS_ERR_ARG);
    EXPECT_EQ(fds_file_session_list(file.get(), nullptr, &list_size), FDS_ERR_ARG);
    EXPECT_EQ(fds_file_session_list(file.get(), nullptr, nullptr), FDS_ERR_ARG);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
}

// Try to get a list of ODIDs of a non-existing Transport Session
TEST_P(FileAPI, sessionODIDNonExistingSession)
{
    fds_file_sid_t sid = 0;
    uint32_t *odid_list = nullptr;
    size_t odid_size;

    // Open a file for writting
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    // Try to get a Transport Sessions
    EXPECT_EQ(fds_file_session_odids(file.get(), sid, &odid_list, &odid_size), FDS_ERR_NOTFOUND);
    EXPECT_EQ(odid_list, nullptr);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);

    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);
    // Try to get a Transport Sessions
    EXPECT_EQ(fds_file_session_odids(file.get(), sid, &odid_list, &odid_size), FDS_ERR_NOTFOUND);
    EXPECT_EQ(odid_list, nullptr);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
}

// Try to get a list of Transport Sessions with invalid function arguments
TEST_P(FileAPI, sessionODIDInvalidArgs)
{
    // Transport Session defintion
    Session session_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_UDP};
    fds_file_sid_t sid;

    // Open a file for writting and add at least one Transport Session
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session_def.get(), &sid), FDS_OK);

    uint32_t *odid_list = nullptr;
    size_t odid_size;
    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    EXPECT_EQ(fds_file_session_odids(file.get(), sid, &odid_list, nullptr), FDS_ERR_ARG);
    EXPECT_EQ(fds_file_session_odids(file.get(), sid, nullptr, &odid_size), FDS_ERR_ARG);
    EXPECT_EQ(fds_file_session_odids(file.get(), sid, nullptr, nullptr), FDS_ERR_ARG);
    EXPECT_EQ(odid_list, nullptr);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);

    // Open the file for reading
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);

    fds_file_sid_t *list_data;
    size_t list_size;
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 1U);
    sid = list_data[0];
    free(list_data);

    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    EXPECT_EQ(fds_file_session_odids(file.get(), sid, &odid_list, nullptr), FDS_ERR_ARG);
    EXPECT_EQ(fds_file_session_odids(file.get(), sid, nullptr, &odid_size), FDS_ERR_ARG);
    EXPECT_EQ(fds_file_session_odids(file.get(), sid, nullptr, nullptr), FDS_ERR_ARG);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
}

// Try to configure the Transport Session and ODID filter with invalid arguments
TEST_P(FileAPI, sFilterInvalidArgs)
{
    // Transport Session defintion
    Session session_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_UDP};
    fds_file_sid_t sid;
    uint32_t odid = 0;

    // Open a file for writting
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session_def.get(), &sid), FDS_OK);

    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    EXPECT_EQ(fds_file_read_sfilter(file.get(), &sid, &odid), FDS_ERR_DENIED);
    EXPECT_EQ(fds_file_read_sfilter(file.get(), &sid, nullptr), FDS_ERR_DENIED);
    EXPECT_EQ(fds_file_read_sfilter(file.get(), nullptr, &odid), FDS_ERR_DENIED);
    EXPECT_EQ(fds_file_read_sfilter(file.get(), nullptr, nullptr), FDS_ERR_DENIED);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);

    // Open file for reading
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);

    fds_file_sid_t *list_data;
    size_t list_size;
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 1U);
    sid = list_data[0];
    free(list_data);

    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    sid++; // Make it invalid
    EXPECT_EQ(fds_file_read_sfilter(file.get(), &sid, nullptr), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_file_read_sfilter(file.get(), &sid, &odid), FDS_ERR_NOTFOUND);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
}

// Try to read a Data Record from a file in the writer/append mode
TEST_P(FileAPI, readRecInvalidMode)
{
    struct fds_drec rec_data;
    struct fds_file_read_ctx rec_ctx;

    // Writer mode
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    EXPECT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_ERR_DENIED);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);

    // Append mode
    uint32_t append_flags = write2append_flag(m_flags_write);
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), append_flags), FDS_OK);
    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    EXPECT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_ERR_DENIED);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
}

// Try to set writer context in a reader mode
TEST_P(FileAPI, writeCtxInvalidMode)
{
    // Transport Session defintion
    Session session_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_UDP};
    fds_file_sid_t sid;

    // First of all, create a simple file
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session_def.get(), &sid), FDS_OK);

    // Try to open it in the reader mode and set a writer context
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);

    fds_file_sid_t *list_data;
    size_t list_size;
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 1U);
    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    ASSERT_EQ(fds_file_write_ctx(file.get(), list_data[0], 0, 0), FDS_ERR_DENIED);
    free(list_data);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
}

// Try to set invalid a writer context to an undefined Transport Session
TEST_P(FileAPI, writeCtxUknownContext)
{
    fds_file_sid_t sid = 0;

    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), sid, 0, 0), FDS_ERR_NOTFOUND);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
}

// Try to defined a Template without previous definition of the writer context
TEST_P(FileAPI, writeTemplateAddNoContext)
{
    uint16_t tid = 256;
    DRec_biflow rec(tid);

    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    EXPECT_EQ(fds_file_write_tmplt_add(file.get(), rec.tmplt_type(), rec.tmplt_data(),
        rec.tmplt_size()), FDS_ERR_ARG);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
}

// Try to add a Template in the reader mode
TEST_P(FileAPI, writeTemplateAddInvalidMode)
{
    // First of all, create an empty file
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);

    // Open it in the reader mode and try to add a Template
    DRec_simple rec(256);
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);
    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    EXPECT_EQ(fds_file_write_tmplt_add(file.get(), rec.tmplt_type(), rec.tmplt_data(),
        rec.tmplt_size()), FDS_ERR_DENIED);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
}

// Try to add a malformed Template
TEST_P(FileAPI, writeTemplateAddMalformed)
{
    // Transport Session defintion
    Session session_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_UDP};
    fds_file_sid_t sid;

    // First of all, create a simple file
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session_def.get(), &sid), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), sid, 0, 0), FDS_OK);

    uint16_t rec1_tid = 256;
    uint16_t rec2_tid = 257;
    uint16_t rec3_tid = 258;
    uint16_t rec4_tid = 255; // invalid ID

    // Try to insert a Template with unexpected Template type
    DRec_biflow rec1(rec1_tid);
    EXPECT_EQ(fds_file_write_tmplt_add(file.get(), FDS_TYPE_TEMPLATE_UNDEF, rec1.tmplt_data(),
        rec1.tmplt_size()), FDS_ERR_FORMAT);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);

    // Try to insert an invalid Template (too short definition)
    DRec_biflow rec2(rec2_tid);
    uint16_t rec2_new_size = rec2.tmplt_size() - 1; // remove 1 byte
    std::unique_ptr<uint8_t[]> rec2_new_data(new uint8_t[rec2_new_size]);
    memcpy(rec2_new_data.get(), rec2.tmplt_data(), rec2_new_size);
    EXPECT_EQ(fds_file_write_tmplt_add(file.get(), rec2.tmplt_type(), rec2_new_data.get(),
        rec2_new_size), FDS_ERR_FORMAT);

    // Try to insert an invalid Template (too long definition)
    DRec_biflow rec3(rec3_tid);
    uint16_t rec3_new_size = rec3.tmplt_size() + 1; // add 1 byte
    std::unique_ptr<uint8_t[]> rec3_new_data(new uint8_t[rec3_new_size]);
    memset(rec3_new_data.get(), 0, rec3_new_size);
    memcpy(rec3_new_data.get(), rec3.tmplt_data(), rec3.tmplt_size());
    EXPECT_EQ(fds_file_write_tmplt_add(file.get(), rec3.tmplt_type(), rec3_new_data.get(),
        rec3_new_size), FDS_ERR_FORMAT);

    // Try to insert a Template with invalid Template ID (less than 256)
    DRec_simple rec4(rec4_tid);
    EXPECT_EQ(fds_file_write_tmplt_add(file.get(), rec4.tmplt_type(), rec4.tmplt_data(),
        rec4.tmplt_size()), FDS_ERR_FORMAT);

    // Try to use invalid arguments (nullptr and zero size length)
    EXPECT_EQ(fds_file_write_tmplt_add(file.get(), FDS_TYPE_TEMPLATE, nullptr, 0), FDS_ERR_ARG);
    EXPECT_EQ(fds_file_write_tmplt_add(file.get(), FDS_TYPE_TEMPLATE, nullptr, rec1.tmplt_size()),
        FDS_ERR_ARG);
    EXPECT_EQ(fds_file_write_tmplt_add(file.get(), FDS_TYPE_TEMPLATE, rec1.tmplt_data(), 0),
        FDS_ERR_ARG);

    // Check that no definition has been added
    enum fds_template_type tmplt_type;
    const uint8_t *tmplt_data;
    uint16_t tmplt_size;
    EXPECT_EQ(fds_file_write_tmplt_get(file.get(), rec1_tid, &tmplt_type, &tmplt_data, &tmplt_size),
        FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_file_write_tmplt_get(file.get(), rec2_tid, &tmplt_type, &tmplt_data, &tmplt_size),
        FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_file_write_tmplt_get(file.get(), rec3_tid, &tmplt_type, &tmplt_data, &tmplt_size),
        FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_file_write_tmplt_get(file.get(), rec4_tid, &tmplt_type, &tmplt_data, &tmplt_size),
        FDS_ERR_NOTFOUND);
}

// Try to remove an undefined Template
TEST_P(FileAPI, writeTemplateRemoveUnknownTemplate)
{
    // Transport Session defintion
    Session session_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_UDP};
    fds_file_sid_t sid;

    // First of all, create a simple file
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session_def.get(), &sid), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), sid, 0, 0), FDS_OK);

    // Try to remove a Template
    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    EXPECT_EQ(fds_file_write_tmplt_remove(file.get(), 256), FDS_ERR_NOTFOUND);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
}

// Try to remove a Template without definition of the writer context
TEST_P(FileAPI, writeTemplateRemoveUndefinedContext)
{
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    EXPECT_EQ(fds_file_write_tmplt_remove(file.get(), 256), FDS_ERR_ARG);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
}

// Try to remove a Template in the reader mode
TEST_P(FileAPI, writeTemplateRemoveInvalidMode)
{
    uint16_t rec_tid = 256;
    DRec_biflow rec(rec_tid);

    // Transport Session defintion
    Session session_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_UDP};
    fds_file_sid_t sid;

    // First of all, create a simple file and add a Data Record
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session_def.get(), &sid), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), sid, 0, 0), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec.tmplt_type(), rec.tmplt_data(),
        rec.tmplt_size()), FDS_OK);
    EXPECT_EQ(fds_file_write_rec(file.get(), rec.tmptl_id(), rec.rec_data(), rec.rec_size()),
        FDS_OK);

    // Open the file in the reader mode
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);
    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    EXPECT_EQ(fds_file_write_tmplt_remove(file.get(), rec_tid), FDS_ERR_DENIED);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);

    // Try to get the Data Record
    struct fds_file_read_ctx rec_ctx;
    struct fds_drec rec_data;
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);

    // Try to remove the Template again
    EXPECT_EQ(fds_file_write_tmplt_remove(file.get(), rec_tid), FDS_ERR_DENIED);
}

// Try to get a previously undefined Template
TEST_P(FileAPI, writeTemplateGetUnknownTemplate)
{
    Session session_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_UDP};
    fds_file_sid_t sid;

    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session_def.get(), &sid), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), sid, 0, 0), FDS_OK);

    // Try to get a Template
    enum fds_template_type tmplt_type;
    const uint8_t *tmplt_data;
    uint16_t tmplt_size;
    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    EXPECT_EQ(fds_file_write_tmplt_get(file.get(), 256, &tmplt_type, &tmplt_data, &tmplt_size),
        FDS_ERR_NOTFOUND);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
}

// Try to get a Template without definition of the writer context
TEST_P(FileAPI, writeTemplateGetUndefinedContext)
{
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);

    enum fds_template_type tmplt_type;
    const uint8_t *tmplt_data;
    uint16_t tmplt_size;
    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    EXPECT_EQ(fds_file_write_tmplt_get(file.get(), 256, &tmplt_type, &tmplt_data, &tmplt_size),
        FDS_ERR_ARG);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
}

// Try to get a Template in the reader mode
TEST_P(FileAPI, writeTemplateGetInvalidMode)
{
    uint16_t rec_tid = 256;
    DRec_biflow rec(rec_tid);

    // Transport Session defintion
    Session session_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_UDP};
    fds_file_sid_t sid;

    // First of all, create a simple file and add a Data Record
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session_def.get(), &sid), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), sid, 0, 0), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec.tmplt_type(), rec.tmplt_data(),
        rec.tmplt_size()), FDS_OK);
    EXPECT_EQ(fds_file_write_rec(file.get(), rec.tmptl_id(), rec.rec_data(), rec.rec_size()),
        FDS_OK);

    // Reopen the file in the reader mode
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);

    // Try to get the Template
    enum fds_template_type tmplt_type;
    const uint8_t *tmplt_data;
    uint16_t tmplt_size;
    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    EXPECT_EQ(fds_file_write_tmplt_get(file.get(), rec_tid, &tmplt_type, &tmplt_data, &tmplt_size),
        FDS_ERR_DENIED);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
}

// Try to write a Data Record without definition of the writer context
TEST_P(FileAPI, writeRecWithoutContext)
{
    DRec_biflow rec(256);

    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    EXPECT_EQ(fds_file_write_rec(file.get(), rec.tmptl_id(), rec.rec_data(), rec.rec_size()),
        FDS_ERR_ARG);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);
}

// Try to write a Data Record in the reader mode
TEST_P(FileAPI, writeRecInvalidMode)
{
    // Transport Session defintion
    Session session_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_UDP};
    fds_file_sid_t sid;

    // First of all, create a simple file
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session_def.get(), &sid), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), sid, 0, 0), FDS_OK);

    // Reopen the file in the reader mode
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);

    // Try to write a Data Record
    DRec_biflow rec(256);
    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    EXPECT_EQ(fds_file_write_rec(file.get(), rec.tmptl_id(), rec.rec_data(), rec.rec_size()),
        FDS_ERR_DENIED);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);

    // Try to get a Data Record
    struct fds_file_read_ctx rec_ctx;
    struct fds_drec rec_data;
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);
}

// Try to write a Data Record based on a undefined Template
TEST_P(FileAPI, writeRecMissingTemplate)
{
    // Transport Session defintion
    Session session_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_UDP};
    fds_file_sid_t sid;

    // First of all, create a simple file
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session_def.get(), &sid), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), sid, 0, 0), FDS_OK);

    // Try to write a Data Record (no previous Template definition)
    DRec_biflow rec(256);
    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    EXPECT_EQ(fds_file_write_rec(file.get(), rec.tmptl_id(), rec.rec_data(), rec.rec_size()),
        FDS_ERR_NOTFOUND);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);

    // Try to use invalid arguments (nullptr, zero size)
    EXPECT_EQ(fds_file_write_rec(file.get(), rec.tmptl_id(), nullptr, 0), FDS_ERR_ARG);
    EXPECT_EQ(fds_file_write_rec(file.get(), rec.tmptl_id(), rec.rec_data(), 0), FDS_ERR_ARG);
    EXPECT_EQ(fds_file_write_rec(file.get(), rec.tmptl_id(), nullptr, rec.rec_size()), FDS_ERR_ARG);
}

// Try to add malformed Data Record (for example, based on different Template)
TEST_P(FileAPI, writeRecInvalidDataRecord)
{
    // Transport Session defintion
    Session session_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_UDP};
    fds_file_sid_t sid;

    // First of all, create a simple file
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session_def.get(), &sid), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), sid, 0, 0), FDS_OK);

    // Define few Templates
    uint16_t rec1_tid = 256;
    uint16_t rec2_tid = 257;
    uint16_t rec3_tid = 258;

    DRec_simple rec1(rec1_tid);
    DRec_biflow rec2(rec2_tid);
    DRec_opts rec3(rec3_tid);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1.tmplt_type(), rec1.tmplt_data(),
        rec1.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec2.tmplt_type(), rec2.tmplt_data(),
        rec2.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec3.tmplt_type(), rec3.tmplt_data(),
        rec3.tmplt_size()), FDS_OK);

    // Try all invalid write combinations of Data Records (non-matching Templates)
    EXPECT_STREQ(fds_file_error(file.get()), no_error_msg);
    EXPECT_EQ(fds_file_write_rec(file.get(), rec2_tid, rec1.rec_data(), rec1.rec_size()),
        FDS_ERR_FORMAT);
    EXPECT_EQ(fds_file_write_rec(file.get(), rec3_tid, rec1.rec_data(), rec1.rec_size()),
        FDS_ERR_FORMAT);

    EXPECT_EQ(fds_file_write_rec(file.get(), rec1_tid, rec2.rec_data(), rec2.rec_size()),
        FDS_ERR_FORMAT);
    EXPECT_EQ(fds_file_write_rec(file.get(), rec3_tid, rec2.rec_data(), rec2.rec_size()),
        FDS_ERR_FORMAT);

    EXPECT_EQ(fds_file_write_rec(file.get(), rec1_tid, rec3.rec_data(), rec3.rec_size()),
        FDS_ERR_FORMAT);
    EXPECT_EQ(fds_file_write_rec(file.get(), rec2_tid, rec3.rec_data(), rec3.rec_size()),
        FDS_ERR_FORMAT);
    EXPECT_STRNE(fds_file_error(file.get()), no_error_msg);

    // Try to open for reading and check that no Data Records are available
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);
    struct fds_file_read_ctx rec_ctx;
    struct fds_drec rec_data;
    EXPECT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);
}
