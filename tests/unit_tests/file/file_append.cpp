/**
 * @file file_simple.cpp
 * @author Lukas Hutak (lukas.hutak@cesnet.cz)
 * @date June 2019
 * @brief
 *   Simple test cases using FDS File API
 *
 * The tests usually try to create a simple file with or without Data Records and at most
 * few Transport Sessions.
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
uint32_t flags_comp[] = {0, FDS_FILE_LZ4, FDS_FILE_ZSTD};
uint32_t flags_io[] = {0, FDS_FILE_NOASYNC};
bool with_ie_mgr[] = {false, true};
auto product = ::testing::Combine(::testing::ValuesIn(flags_comp), ::testing::ValuesIn(flags_io),
    ::testing::ValuesIn(with_ie_mgr));
INSTANTIATE_TEST_CASE_P(Append, FileAPI, product, &product_name);


// Try to open non-existing file in append mode (should be the same as write)
TEST_P(FileAPI, appendNotExistingFile)
{
    // Prepare a Transport Session and single Data Record
    Session session2write{"192.168.0.1", "204.152.189.116", 80, 10000, FDS_FILE_SESSION_TCP};
    fds_file_sid_t session_sid;
    DRec_simple rec(256);

    // Open the file
    uint32_t append_flags = write2append_flag(m_flags_write);
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), append_flags), FDS_OK);

    // Add the Transport Session and the Data Record
    ASSERT_EQ(fds_file_session_add(file.get(), session2write.get(), &session_sid), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    ASSERT_EQ(fds_file_write_ctx(file.get(), session_sid, 0, 0), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec.tmplt_type(), rec.tmplt_data(), rec.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec.tmptl_id(), rec.rec_data(), rec.rec_size()), FDS_OK);
    // Close the file
    file.reset();

    // Open the file for reading and check the content
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    // Get the Data Record
    struct fds_file_read_ctx rec_ctx;
    struct fds_drec rec_data;
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    // Compare it with the written Data Record
    EXPECT_TRUE(rec.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    EXPECT_TRUE(rec.cmp_record(rec_data.data, rec_data.size));
    EXPECT_EQ(rec_ctx.odid, 0U);
    EXPECT_EQ(rec_ctx.exp_time, 0U);

    // Check the Transport Session
    const fds_file_session *src_desc;
    ASSERT_EQ(fds_file_session_get(file.get(), rec_ctx.sid, &src_desc), FDS_OK);
    EXPECT_TRUE(session2write.cmp(src_desc));

    // No more data records
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);
}

// Try to append empty file
TEST_P(FileAPI, appendEmptyFile)
{
    // Create a file
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    // Close it
    file.reset();

    // Open the file in append mode
    uint32_t append_flags = write2append_flag(m_flags_write);
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), append_flags), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    // Try to list all Transport Sessions
    fds_file_sid_t *list_data;
    size_t list_size;
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    EXPECT_EQ(list_size, 0U);
    free(list_data);

    // Close it
    file.reset();

    // Open the file for reading
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    // No Data Records
    struct fds_file_read_ctx rec_ctx;
    struct fds_drec rec_data;
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);

    // No Transport Sessions
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    EXPECT_EQ(list_size, 0U);
    free(list_data);
}

/*
 * Try to append empty with a Transport Session and add the same Transport Session defintion again.
 * Only on Transport Session must be defined.
 */
TEST_P(FileAPI, appendEmptyFileWithSession)
{
    // Create a Transport Session description
    Session session2write{"192.168.0.1", "204.152.189.116", 80, 10000, FDS_FILE_SESSION_TCP};
    fds_file_sid_t session_sid;

    // Create a file and add a Transport Session
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session2write.get(), &session_sid), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    // Close it
    file.reset();

    // Open the file in append mode
    uint32_t append_flags = write2append_flag(m_flags_write);
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), append_flags), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    // Try to add the same Transport Session (only the session ID should be returned)
    const struct fds_file_session *info;
    fds_file_sid_t *list_data;
    size_t list_size;
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 1U);
    ASSERT_EQ(fds_file_session_get(file.get(), list_data[0], &info), FDS_OK);
    EXPECT_TRUE(session2write.cmp(info));
    free(list_data);

    // Close it
    file.reset();

    // Open the file for reading
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    // Try to get a Data Record
    struct fds_file_read_ctx rec_ctx;
    struct fds_drec rec_data;
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);
    // List of Transport Sessions
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 1U);
    ASSERT_EQ(fds_file_session_get(file.get(), list_data[0], &info), FDS_OK);
    EXPECT_TRUE(session2write.cmp(info));
    free(list_data);
}

// Add few Data Records to a file with only one combination of Transport Session and ODID
TEST_P(FileAPI, appendWithSingleTransportSession)
{
    uint32_t odid = 134; // random values
    uint32_t exp_time = std::numeric_limits<uint32_t>::max() - 10;

    // Create a Transport Session description
    Session session2write{"192.168.0.1", "1.1.1.1", 5000, 10000, FDS_FILE_SESSION_UDP};
    fds_file_sid_t session_sid;

    uint16_t rec1_tid = 256;
    uint16_t rec2_tid = 257;
    DRec_simple rec1_a(rec1_tid);
    DRec_biflow rec1_b(rec1_tid);
    DRec_opts rec2(rec2_tid);

    // Open a file for writing and add the Transport Session
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session2write.get(), &session_sid), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    // Add few Data Records (variant A)
    ASSERT_EQ(fds_file_write_ctx(file.get(), session_sid, odid, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1_a.tmplt_type(), rec1_a.tmplt_data(),
        rec1_a.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec2.tmplt_type(), rec2.tmplt_data(),
        rec2.tmplt_size()), FDS_OK);

    size_t cnt1 = 1000;
    for (size_t i = 0; i < cnt1; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));
        ASSERT_EQ(fds_file_write_rec(file.get(), rec1_a.tmptl_id(), rec1_a.rec_data(),
            rec1_a.rec_size()), FDS_OK);
    }

    // Close the file
    file.reset();

    // Open the file for appending -----------------------------------------------------------------
    uint32_t append_flags = write2append_flag(m_flags_write);
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), append_flags), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    // Get the list of all Transport Sessions
    fds_file_sid_t *list_data;
    size_t list_size;
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 1U);
    fds_file_sid_t sid2get = list_data[0];
    free(list_data);

    const fds_file_session *info;
    ASSERT_EQ(fds_file_session_get(file.get(), sid2get, &info), FDS_OK);
    EXPECT_TRUE(session2write.cmp(info));

    // Try to get the previously defined Template (based on docs, they should not be available)
    enum fds_template_type t_type;
    const uint8_t *t_data;
    uint16_t t_size;

    ASSERT_EQ(fds_file_write_ctx(file.get(), sid2get, odid, exp_time), FDS_OK);
    EXPECT_EQ(fds_file_write_tmplt_get(file.get(), rec1_tid, &t_type, &t_data, &t_size),
        FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_file_write_tmplt_get(file.get(), rec2_tid, &t_type, &t_data, &t_size),
        FDS_ERR_NOTFOUND);

    // Add few Data Records (variant B)
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1_b.tmplt_type(), rec1_b.tmplt_data(),
        rec1_b.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec2.tmplt_type(), rec2.tmplt_data(),
        rec2.tmplt_size()), FDS_OK);

    size_t cnt2 = 500;
    for (size_t i = 0; i < cnt2; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));
        ASSERT_EQ(fds_file_write_rec(file.get(), rec1_b.tmptl_id(), rec1_b.rec_data(),
            rec1_b.rec_size()), FDS_OK);
    }

    ASSERT_EQ(fds_file_write_rec(file.get(), rec2.tmptl_id(), rec2.rec_data(), rec2.rec_size()),
        FDS_OK);

    // Close the file
    file.reset();

    // Open the file for reading -------------------------------------------------------------------
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    // Get the list of Transport Sessions
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 1U);
    sid2get = list_data[0];
    free(list_data);

    /* Try to read all Data Records
     * Only one combination Transport Session and ODID is used in the whole file, therefore,
     * all Data Records must preserve its order.
     */
    struct fds_file_read_ctx rec_ctx;
    struct fds_drec rec_data;

    for (size_t i = 0; i < cnt1; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));
        ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
        // Compare it with the written Data Record
        EXPECT_TRUE(rec1_a.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
        EXPECT_TRUE(rec1_a.cmp_record(rec_data.data, rec_data.size));
        EXPECT_EQ(rec_ctx.odid, odid);
        EXPECT_EQ(rec_ctx.exp_time, exp_time);
        EXPECT_EQ(rec_ctx.sid, sid2get);
    }

    for (size_t i = 0; i < cnt2; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));
        ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
        // Compare it with the written Data Record
        EXPECT_TRUE(rec1_b.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
        EXPECT_TRUE(rec1_b.cmp_record(rec_data.data, rec_data.size));
        EXPECT_EQ(rec_ctx.odid, odid);
        EXPECT_EQ(rec_ctx.exp_time, exp_time);
        EXPECT_EQ(rec_ctx.sid, sid2get);
    }

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    // Compare it with the written Data Record
    EXPECT_TRUE(rec2.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    EXPECT_TRUE(rec2.cmp_record(rec_data.data, rec_data.size));
    EXPECT_EQ(rec_ctx.odid, odid);
    EXPECT_EQ(rec_ctx.exp_time, exp_time);
    EXPECT_EQ(rec_ctx.sid, sid2get);

    // No more Data Records expected
    EXPECT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);
}

// Try to append a non-emtpy file, which is still opened for writting
TEST_P(FileAPI, tryToAppendNonEmptyFileWhichIsBeingWritten)
{
    // Create a Transport Session description
    Session session2write{"192.168.0.1", "204.152.189.116", 80, 10000, FDS_FILE_SESSION_TCP};
    fds_file_sid_t session_sid;

    // Open a file for writing and add the Transport Session
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session2write.get(), &session_sid), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    DRec_simple rec(256);
    ASSERT_EQ(fds_file_write_ctx(file.get(), session_sid, 0, 0), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec.tmplt_type(), rec.tmplt_data(), rec.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec.tmptl_id(), rec.rec_data(), rec.rec_size()), FDS_OK);

    // Try to open the file again for appending (must fail)
    uint32_t append_flags = write2append_flag(m_flags_write);
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file_append(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file_append.get(), m_filename.c_str(), append_flags), FDS_ERR_DENIED);
}
