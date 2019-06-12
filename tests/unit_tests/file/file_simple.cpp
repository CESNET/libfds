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
INSTANTIATE_TEST_CASE_P(Simple, FileAPI, product, &product_name);


// Create empty file (no Data Records, no Transport Sessions)
TEST_P(FileAPI, createEmpty)
{
    fds_file_sid_t *list_data = nullptr;
    size_t list_size;

    // Open a file for writing and close it
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }
    file.reset();

    // Open the file for reading and try to read it
    file.reset(fds_file_init());
    ASSERT_NE(file, nullptr);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);

    // Try to get a Data Record
    struct fds_file_read_ctx rec_ctx;
    struct fds_drec rec_data;
    EXPECT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);

    // Try to get list of Transport Sessions
    EXPECT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    EXPECT_EQ(list_size, 0U);
    free(list_data);
}

// Create an empty file (i.e. no Data Records) with one Transport Session description
TEST_P(FileAPI, createEmptyWithSource)
{
    // Create a Transport Session description
    Session session2write{"192.168.0.1", "204.152.189.116", 80, 10000, FDS_FILE_SESSION_TCP};
    fds_file_sid_t session_sid;

    // Open a file for writing, add Transport Session(s) and close it
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session2write.get(), &session_sid), FDS_OK);
    file.reset();

    // Open the file for reading
    file.reset(fds_file_init());
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);

    // Try to list all Transport Session
    fds_file_sid_t *list_data = nullptr;
    size_t list_size;
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 1U);
    fds_file_sid_t sid2read = list_data[0];
    free(list_data);

    // Get the Transport Session and compare it
    const struct fds_file_session *session2read;
    ASSERT_EQ(fds_file_session_get(file.get(), sid2read, &session2read), FDS_OK);
    EXPECT_TRUE(session2write.cmp(session2read));

    // Try to get a Data Record
    struct fds_file_read_ctx rec_ctx;
    struct fds_drec rec_data;
    EXPECT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);
}

// Write a single Data Record to the file
TEST_P(FileAPI, singleRecord)
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

    // Set Transport Session context
    ASSERT_EQ(fds_file_write_ctx(file.get(), session_sid, 123, 456), FDS_OK);
    // Add an IPFIX Template and Data Record
    uint16_t gen_tid = 256;
    DRec_simple gen_rec(gen_tid);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), gen_rec.tmplt_type(), gen_rec.tmplt_data(), gen_rec.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), gen_tid, gen_rec.rec_data(), gen_rec.rec_size()), FDS_OK);
    // Close the file
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
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    // Compare it with the written Data Record
    EXPECT_TRUE(gen_rec.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    EXPECT_TRUE(gen_rec.cmp_record(rec_data.data, rec_data.size));
    EXPECT_EQ(rec_ctx.odid, 123U);
    EXPECT_EQ(rec_ctx.exp_time, 456U);

    // Try to get a field and check if an Infomation Element definition is available
    struct fds_drec_field field;
    ASSERT_NE(fds_drec_find(&rec_data, 0, 1, &field), FDS_EOC); // octetDeltaCount
    if (m_load_iemgr) {
        ASSERT_NE(field.info->def, nullptr);
        EXPECT_NE(field.info->def->name, nullptr);
        EXPECT_EQ(field.info->def->data_type, FDS_ET_UNSIGNED_64);
        EXPECT_EQ(field.info->def->data_unit, FDS_EU_OCTETS);
    } else {
        EXPECT_EQ(field.info->def, nullptr);
    }

    // Check the Transport Session
    const fds_file_session *src_desc;
    ASSERT_EQ(fds_file_session_get(file.get(), rec_ctx.sid, &src_desc), FDS_OK);
    EXPECT_TRUE(session2write.cmp(src_desc));

    // No more data records
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);

    // Rewind the file and try again...
    ASSERT_EQ(fds_file_read_rewind(file.get()), FDS_OK);

    // Get the Data Record and compare it with the written one
    memset(&rec_ctx, 0, sizeof rec_ctx);
    memset(&rec_data, 0, sizeof rec_data);

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    EXPECT_TRUE(gen_rec.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    EXPECT_TRUE(gen_rec.cmp_record(rec_data.data, rec_data.size));
    EXPECT_EQ(rec_ctx.odid, 123U);
    EXPECT_EQ(rec_ctx.exp_time, 456U);

    // No more data records
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);
}

/*
 * Check if adding Template Definitions later don't break readability of the Data Records
 *
 * Only on Transport Session and ODID is used.
 */
TEST_P(FileAPI, addTemplateDefinitionsLater)
{
    uint32_t odid = 1654; // random values
    uint32_t exp_time = std::numeric_limits<uint32_t>::max() - 1;

    // Create a Transport Session description
    Session session2write{"255.255.255.0", "10.10.10.10", 123, 789, FDS_FILE_SESSION_TCP};
    fds_file_sid_t session_sid;

    // Prepare few Data Records
    uint16_t rec1_tid = 256;
    uint16_t rec2_tid = 300;
    uint16_t rec3_tid = 270;
    DRec_simple rec1(rec1_tid);
    DRec_opts rec2(rec2_tid);
    DRec_biflow rec3(rec3_tid);

    // Open a file for writing, add the Transport Session and the IPFIX Template
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    // Open the file for writting and add the Transport Session
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session2write.get(), &session_sid), FDS_OK);

    // Add the first Template defintion and few Data Records
    ASSERT_EQ(fds_file_write_ctx(file.get(), session_sid, odid, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1.tmplt_type(), rec1.tmplt_data(), rec1.tmplt_size()), FDS_OK);

    size_t cnt1 = 1200;
    for (size_t i = 0; i < cnt1; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));
        ASSERT_EQ(fds_file_write_rec(file.get(), rec1.tmptl_id(), rec1.rec_data(), rec1.rec_size()), FDS_OK);
    }

    // Add the second Template definition and add few Data Records
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec2.tmplt_type(), rec2.tmplt_data(), rec2.tmplt_size()), FDS_OK);
    size_t cnt2 = 2500;
    for (size_t i = 0; i < cnt2; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));
        ASSERT_EQ(fds_file_write_rec(file.get(), rec2.tmptl_id(), rec2.rec_data(), rec2.rec_size()), FDS_OK);
    }

    // Try to add the first Template definition again (should not affect anything)
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1.tmplt_type(), rec1.tmplt_data(), rec1.tmplt_size()), FDS_OK);

    // Change Export Time (timestamp overlap)
    exp_time = 2;
    // Add the third Template definition and add few Data Records
    ASSERT_EQ(fds_file_write_ctx(file.get(), session_sid, odid, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec3.tmplt_type(), rec3.tmplt_data(), rec3.tmplt_size()), FDS_OK);

    size_t cnt3 = 1500;
    for (size_t i = 0; i < cnt3; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));
        ASSERT_EQ(fds_file_write_rec(file.get(), rec3.tmptl_id(), rec3.rec_data(), rec3.rec_size()), FDS_OK);
    }

    // Try to add previous type of Data Records
    size_t cnt4 = 100;
    for (size_t i = 0; i < cnt4; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));
        ASSERT_EQ(fds_file_write_rec(file.get(), rec2.tmptl_id(), rec2.rec_data(), rec2.rec_size()), FDS_OK);
    }
    size_t cnt5 = 120;
    for (size_t i = 0; i < cnt5; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));
        ASSERT_EQ(fds_file_write_rec(file.get(), rec1.tmptl_id(), rec1.rec_data(), rec1.rec_size()), FDS_OK);
    }

    // Close the file
    file.reset();

    // Open the file for reading
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    struct fds_file_read_ctx rec_ctx;
    struct fds_drec rec_data;
    exp_time = std::numeric_limits<uint32_t>::max() - 1;

    /* Try to read all Data Records
     * Only one combination Transport Session and ODID is used in the whole file, therefore,
     * all Data Records must preserve its order.
     */
    for (size_t i = 0; i < cnt1; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));
        ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
        ASSERT_TRUE(rec1.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
        ASSERT_TRUE(rec1.cmp_record(rec_data.data, rec_data.size));
        ASSERT_EQ(rec_ctx.odid, odid);
        ASSERT_EQ(rec_ctx.exp_time, exp_time);

        // Try to get a field and check if an Infomation Element definition is available
        struct fds_drec_field field;
        ASSERT_NE(fds_drec_find(&rec_data, 0, 1, &field), FDS_EOC); // octetDeltaCount
        if (m_load_iemgr) {
            ASSERT_NE(field.info->def, nullptr);
            EXPECT_NE(field.info->def->name, nullptr);
            EXPECT_EQ(field.info->def->data_type, FDS_ET_UNSIGNED_64);
            EXPECT_EQ(field.info->def->data_unit, FDS_EU_OCTETS);
        } else {
            EXPECT_EQ(field.info->def, nullptr);
        }
    }
    for (size_t i = 0; i < cnt2; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));
        ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
        ASSERT_TRUE(rec2.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
        ASSERT_TRUE(rec2.cmp_record(rec_data.data, rec_data.size));
        ASSERT_EQ(rec_ctx.odid, odid);
        ASSERT_EQ(rec_ctx.exp_time, exp_time);
    }
    exp_time = 2;
    for (size_t i = 0; i < cnt3; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));
        ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
        ASSERT_TRUE(rec3.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
        ASSERT_TRUE(rec3.cmp_record(rec_data.data, rec_data.size));
        ASSERT_EQ(rec_ctx.odid, odid);
        ASSERT_EQ(rec_ctx.exp_time, exp_time);

        // Try to get a field and check if an Infomation Element definition is available
        struct fds_drec_field field;
        ASSERT_NE(fds_drec_find(&rec_data, 0, 1, &field), FDS_EOC); // octetDeltaCount
        if (m_load_iemgr) {
            ASSERT_NE(field.info->def, nullptr);
            EXPECT_NE(field.info->def->name, nullptr);
            EXPECT_EQ(field.info->def->data_type, FDS_ET_UNSIGNED_64);
            EXPECT_EQ(field.info->def->data_unit, FDS_EU_OCTETS);
        } else {
            EXPECT_EQ(field.info->def, nullptr);
        }
    }
    for (size_t i = 0; i < cnt4; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));
        ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
        ASSERT_TRUE(rec2.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
        ASSERT_TRUE(rec2.cmp_record(rec_data.data, rec_data.size));
        ASSERT_EQ(rec_ctx.odid, odid);
        ASSERT_EQ(rec_ctx.exp_time, exp_time);
    }
    for (size_t i = 0; i < cnt5; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));
        ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
        ASSERT_TRUE(rec1.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
        ASSERT_TRUE(rec1.cmp_record(rec_data.data, rec_data.size));
        ASSERT_EQ(rec_ctx.odid, odid);
        ASSERT_EQ(rec_ctx.exp_time, exp_time);

        // Try to get a field and check if an Infomation Element definition is available
        struct fds_drec_field field;
        ASSERT_NE(fds_drec_find(&rec_data, 0, 1, &field), FDS_EOC); // octetDeltaCount
        if (m_load_iemgr) {
            ASSERT_NE(field.info->def, nullptr);
            EXPECT_NE(field.info->def->name, nullptr);
            EXPECT_EQ(field.info->def->data_type, FDS_ET_UNSIGNED_64);
            EXPECT_EQ(field.info->def->data_unit, FDS_EU_OCTETS);
        } else {
            EXPECT_EQ(field.info->def, nullptr);
        }
    }

    // No more Data Records expected
    EXPECT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);
}

/* Try to redefine IPFIX Template with different definition.
 * All records must stay readable.
 */
TEST_P(FileAPI, redefineTemplate)
{
    const uint32_t odid = 1654; // random values
    const uint32_t exp_time1 = std::numeric_limits<uint32_t>::max() / 2U;
    const uint32_t exp_time2 = exp_time1 - 1; // go back in time

    // Create a Transport Session description
    Session session2write{"192.168.10.12", "245.255.0.1", 10, 9999, FDS_FILE_SESSION_TCP};
    fds_file_sid_t session_sid;


    // Prepare few Data Records
    uint16_t rec1_tid = 256;
    uint16_t rec2_tid = 257;
    DRec_simple rec1_a(rec1_tid); // 3 version of Templates and Data Records
    DRec_biflow rec1_b(rec1_tid);
    DRec_opts rec1_c(rec1_tid);
    DRec_simple rec2(rec2_tid); // Control Data Record (definition should not be changed)

    // Open a file for writing and add the Transport Session
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session2write.get(), &session_sid), FDS_OK);

    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    // Add first version of Data Record
    ASSERT_EQ(fds_file_write_ctx(file.get(), session_sid, odid, exp_time1), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1_a.tmplt_type(), rec1_a.tmplt_data(), rec1_a.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec2.tmplt_type(), rec2.tmplt_data(), rec2.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec1_a.tmptl_id(), rec1_a.rec_data(), rec1_a.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec2.tmptl_id(), rec2.rec_data(), rec2.rec_size()), FDS_OK);

    // Change the definition of the first Template
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1_b.tmplt_type(), rec1_b.tmplt_data(), rec1_b.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec1_b.tmptl_id(), rec1_b.rec_data(), rec1_b.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec2.tmptl_id(), rec2.rec_data(), rec2.rec_size()), FDS_OK);

    // Try to write Data Record based on previous Template (must fail)
    ASSERT_EQ(fds_file_write_rec(file.get(), rec1_a.tmptl_id(), rec1_a.rec_data(), rec1_a.rec_size()), FDS_ERR_FORMAT);

    // Change the definition back but don't add any records (try to use Export Time in history i.e. less than the current)
    ASSERT_EQ(fds_file_write_ctx(file.get(), session_sid, odid, exp_time2), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1_a.tmplt_type(), rec1_a.tmplt_data(), rec1_a.tmplt_size()), FDS_OK);

    // Change the defintion again...
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1_c.tmplt_type(), rec1_c.tmplt_data(), rec1_c.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec1_c.tmptl_id(), rec1_c.rec_data(), rec1_c.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec2.tmptl_id(), rec2.rec_data(), rec2.rec_size()), FDS_OK);

    // Go back to the first definition
    ASSERT_EQ(fds_file_write_ctx(file.get(), session_sid, odid, exp_time1), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1_a.tmplt_type(), rec1_a.tmplt_data(), rec1_a.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec1_a.tmptl_id(), rec1_a.rec_data(), rec1_a.rec_size()), FDS_OK);

    // Close the file
    file.reset();

    // Open the file for reading
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    struct fds_file_read_ctx rec_ctx;
    struct fds_drec rec_data;
    struct fds_drec_field field;

    /* Try to read all Data Records
     * Only one combination Transport Session and ODID is used in the whole file, therefore,
     * all Data Records must preserve its order.
     */
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec1_a.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec1_a.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid);
    ASSERT_EQ(rec_ctx.exp_time, exp_time1);

    ASSERT_NE(fds_drec_find(&rec_data, 0, 1, &field), FDS_EOC); // octetDeltaCount
    if (m_load_iemgr) {
        ASSERT_NE(field.info->def, nullptr);
        EXPECT_NE(field.info->def->name, nullptr);
        EXPECT_EQ(field.info->def->data_type, FDS_ET_UNSIGNED_64);
        EXPECT_EQ(field.info->def->data_unit, FDS_EU_OCTETS);
    } else {
        EXPECT_EQ(field.info->def, nullptr);
    }

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec2.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec2.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid);
    ASSERT_EQ(rec_ctx.exp_time, exp_time1);

    ASSERT_NE(fds_drec_find(&rec_data, 0, 1, &field), FDS_EOC); // octetDeltaCount
    if (m_load_iemgr) {
        ASSERT_NE(field.info->def, nullptr);
        EXPECT_NE(field.info->def->name, nullptr);
        EXPECT_EQ(field.info->def->data_type, FDS_ET_UNSIGNED_64);
        EXPECT_EQ(field.info->def->data_unit, FDS_EU_OCTETS);
    } else {
        EXPECT_EQ(field.info->def, nullptr);
    }

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec1_b.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec1_b.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid);
    ASSERT_EQ(rec_ctx.exp_time, exp_time1);

    ASSERT_NE(fds_drec_find(&rec_data, 0, 1, &field), FDS_EOC); // octetDeltaCount
    if (m_load_iemgr) {
        ASSERT_NE(field.info->def, nullptr);
        EXPECT_NE(field.info->def->name, nullptr);
        EXPECT_EQ(field.info->def->data_type, FDS_ET_UNSIGNED_64);
        EXPECT_EQ(field.info->def->data_unit, FDS_EU_OCTETS);
    } else {
        EXPECT_EQ(field.info->def, nullptr);
    }

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec2.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec2.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid);
    ASSERT_EQ(rec_ctx.exp_time, exp_time1);

    ASSERT_NE(fds_drec_find(&rec_data, 0, 1, &field), FDS_EOC); // octetDeltaCount
    if (m_load_iemgr) {
        ASSERT_NE(field.info->def, nullptr);
        EXPECT_NE(field.info->def->name, nullptr);
        EXPECT_EQ(field.info->def->data_type, FDS_ET_UNSIGNED_64);
        EXPECT_EQ(field.info->def->data_unit, FDS_EU_OCTETS);
    } else {
        EXPECT_EQ(field.info->def, nullptr);
    }

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec1_c.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec1_c.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid);
    ASSERT_EQ(rec_ctx.exp_time, exp_time2);

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec2.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec2.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid);
    ASSERT_EQ(rec_ctx.exp_time, exp_time2);

    ASSERT_NE(fds_drec_find(&rec_data, 0, 1, &field), FDS_EOC); // octetDeltaCount
    if (m_load_iemgr) {
        ASSERT_NE(field.info->def, nullptr);
        EXPECT_NE(field.info->def->name, nullptr);
        EXPECT_EQ(field.info->def->data_type, FDS_ET_UNSIGNED_64);
        EXPECT_EQ(field.info->def->data_unit, FDS_EU_OCTETS);
    } else {
        EXPECT_EQ(field.info->def, nullptr);
    }

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec1_a.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec1_a.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid);
    ASSERT_EQ(rec_ctx.exp_time, exp_time1);

    ASSERT_NE(fds_drec_find(&rec_data, 0, 1, &field), FDS_EOC); // octetDeltaCount
    if (m_load_iemgr) {
        ASSERT_NE(field.info->def, nullptr);
        EXPECT_NE(field.info->def->name, nullptr);
        EXPECT_EQ(field.info->def->data_type, FDS_ET_UNSIGNED_64);
        EXPECT_EQ(field.info->def->data_unit, FDS_EU_OCTETS);
    } else {
        EXPECT_EQ(field.info->def, nullptr);
    }

    // No more Data Records expected
    EXPECT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);
}

/* Try to remove IPFIX Template definition and define different Template.
 * All records must stay readable.
 */
TEST_P(FileAPI, removeTemplate)
{
    uint32_t odid = 10; // random values
    uint32_t exp_time1 = std::numeric_limits<uint32_t>::max() - 1;
    uint32_t exp_time2 = 10; // i.e. export time overflow

    enum fds_template_type tmplt_type;
    const uint8_t *tmplt_ptr;
    uint16_t tmplt_size;

    // Create a Transport Session description
    Session session2write {"192.168.10.12", "245.255.0.1", 10, 9999, FDS_FILE_SESSION_SCTP};
    fds_file_sid_t session_sid;

    // Prepare few Data Records
    uint16_t rec1_tid = 256;
    uint16_t rec2_tid = 10000;
    uint16_t rec3_tid = 48791;
    DRec_simple rec1_a(rec1_tid);
    DRec_biflow rec1_b(rec1_tid);
    DRec_opts rec1_c(rec1_tid);
    DRec_simple rec2(rec2_tid); // control record 1 (not to be changed)
    DRec_opts rec3(rec3_tid);   // control record 2 (not to be changed)

    // Open a file for writing and add the Transport Session
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session2write.get(), &session_sid), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    // Try to remove a non-existing Template
    ASSERT_EQ(fds_file_write_ctx(file.get(), session_sid, odid, exp_time1), FDS_OK);
    EXPECT_EQ(fds_file_write_tmplt_remove(file.get(), rec1_a.tmptl_id()), FDS_ERR_NOTFOUND);
    ASSERT_EQ(fds_file_write_tmplt_get(file.get(), rec1_a.tmptl_id(), &tmplt_type, &tmplt_ptr,
        &tmplt_size), FDS_ERR_NOTFOUND);

    // Add the first version of the Data Records
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1_a.tmplt_type(), rec1_a.tmplt_data(), rec1_a.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec2.tmplt_type(), rec2.tmplt_data(), rec2.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec1_a.tmptl_id(), rec1_a.rec_data(), rec1_a.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec2.tmptl_id(), rec2.rec_data(), rec2.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_get(file.get(), rec1_a.tmptl_id(), &tmplt_type, &tmplt_ptr,
        &tmplt_size), FDS_OK);
    EXPECT_TRUE(rec1_a.cmp_template(tmplt_ptr, tmplt_size));

    // Remove the IPFIX Template and try to add the Data Record again
    EXPECT_EQ(fds_file_write_tmplt_remove(file.get(), rec1_a.tmptl_id()), FDS_OK);
    EXPECT_EQ(fds_file_write_tmplt_remove(file.get(), rec1_a.tmptl_id()), FDS_ERR_NOTFOUND);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec1_a.tmptl_id(), rec1_a.rec_data(), rec1_a.rec_size()), FDS_ERR_NOTFOUND);
    ASSERT_EQ(fds_file_write_tmplt_get(file.get(), rec1_a.tmptl_id(), &tmplt_type, &tmplt_ptr,
        &tmplt_size), FDS_ERR_NOTFOUND);

    // Define additional "control record 2"
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec3.tmplt_type(), rec3.tmplt_data(), rec3.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec3.tmptl_id(), rec3.rec_data(), rec3.rec_size()), FDS_OK);

    // Change Export Time
    ASSERT_EQ(fds_file_write_ctx(file.get(), session_sid, odid, exp_time2), FDS_OK);

    // Add the second version of the Data Records
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1_b.tmplt_type(), rec1_b.tmplt_data(), rec1_b.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec1_b.tmptl_id(), rec1_b.rec_data(), rec1_b.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_get(file.get(), rec1_b.tmptl_id(), &tmplt_type, &tmplt_ptr,
        &tmplt_size), FDS_OK);
    EXPECT_TRUE(rec1_b.cmp_template(tmplt_ptr, tmplt_size));

    // Remove the multiple Templates
    EXPECT_EQ(fds_file_write_tmplt_remove(file.get(), rec3.tmptl_id()), FDS_OK);
    EXPECT_EQ(fds_file_write_tmplt_remove(file.get(), rec1_b.tmptl_id()), FDS_OK);

    // Try to add Data Records based on the removed templates
    ASSERT_EQ(fds_file_write_rec(file.get(), rec1_b.tmptl_id(), rec1_b.rec_data(), rec1_b.rec_size()), FDS_ERR_NOTFOUND);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec3.tmptl_id(), rec3.rec_data(), rec3.rec_size()), FDS_ERR_NOTFOUND);

    // Defined the third version of the Data Records and add few Data Records
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1_c.tmplt_type(), rec1_c.tmplt_data(), rec1_c.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec1_c.tmptl_id(), rec1_c.rec_data(), rec1_c.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec2.tmptl_id(), rec2.rec_data(), rec2.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_get(file.get(), rec2.tmptl_id(), &tmplt_type, &tmplt_ptr,
        &tmplt_size), FDS_OK);
    EXPECT_TRUE(rec2.cmp_template(tmplt_ptr, tmplt_size));

    // Close the file
    file.reset();

    // Open the file for reading -------------------------------------------------------------------
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    struct fds_file_read_ctx rec_ctx;
    struct fds_drec rec_data;
    struct fds_drec_field field;

    /* Try to read all Data Records
     * Only one combination Transport Session and ODID is used in the whole file, therefore,
     * all Data Records must preserve its order.
     */
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec1_a.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec1_a.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid);
    ASSERT_EQ(rec_ctx.exp_time, exp_time1);

    ASSERT_NE(fds_drec_find(&rec_data, 0, 1, &field), FDS_EOC); // octetDeltaCount
    if (m_load_iemgr) {
        ASSERT_NE(field.info->def, nullptr);
        EXPECT_NE(field.info->def->name, nullptr);
        EXPECT_EQ(field.info->def->data_type, FDS_ET_UNSIGNED_64);
        EXPECT_EQ(field.info->def->data_unit, FDS_EU_OCTETS);
    } else {
        EXPECT_EQ(field.info->def, nullptr);
    }

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec2.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec2.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid);
    ASSERT_EQ(rec_ctx.exp_time, exp_time1);

    ASSERT_NE(fds_drec_find(&rec_data, 0, 1, &field), FDS_EOC); // octetDeltaCount
    if (m_load_iemgr) {
        ASSERT_NE(field.info->def, nullptr);
        EXPECT_NE(field.info->def->name, nullptr);
        EXPECT_EQ(field.info->def->data_type, FDS_ET_UNSIGNED_64);
        EXPECT_EQ(field.info->def->data_unit, FDS_EU_OCTETS);
    } else {
        EXPECT_EQ(field.info->def, nullptr);
    }

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec3.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec3.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid);
    ASSERT_EQ(rec_ctx.exp_time, exp_time1);

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec1_b.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec1_b.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid);
    ASSERT_EQ(rec_ctx.exp_time, exp_time2);

    ASSERT_NE(fds_drec_find(&rec_data, 0, 1, &field), FDS_EOC); // octetDeltaCount
    if (m_load_iemgr) {
        ASSERT_NE(field.info->def, nullptr);
        EXPECT_NE(field.info->def->name, nullptr);
        EXPECT_EQ(field.info->def->data_type, FDS_ET_UNSIGNED_64);
        EXPECT_EQ(field.info->def->data_unit, FDS_EU_OCTETS);
    } else {
        EXPECT_EQ(field.info->def, nullptr);
    }

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec1_c.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec1_c.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid);
    ASSERT_EQ(rec_ctx.exp_time, exp_time2);

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec2.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec2.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid);
    ASSERT_EQ(rec_ctx.exp_time, exp_time2);

    ASSERT_NE(fds_drec_find(&rec_data, 0, 1, &field), FDS_EOC); // octetDeltaCount
    if (m_load_iemgr) {
        ASSERT_NE(field.info->def, nullptr);
        EXPECT_NE(field.info->def->name, nullptr);
        EXPECT_EQ(field.info->def->data_type, FDS_ET_UNSIGNED_64);
        EXPECT_EQ(field.info->def->data_unit, FDS_EU_OCTETS);
    } else {
        EXPECT_EQ(field.info->def, nullptr);
    }
}

/* Try to redefine IPFIX Template with different definition and remove it.
 * All records must stay readable and other Transport Sessions should stay untouched.
 */
TEST_P(FileAPI, redefineAndRemoveTemplate)
{
    const uint32_t odid1 = 1654; // random values
    const uint32_t odid2 = 30;
    const uint32_t exp_time = 1;

    enum fds_template_type tmplt_type;
    const uint8_t *tmplt_ptr;
    uint16_t tmplt_size;

    // Prepare Transport Sessions
    Session s1_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_UDP};
    Session s2_def {"10.0.10.12", "127.0.0.1", 1000, 11324, FDS_FILE_SESSION_TCP};
    fds_file_sid_t s1_id, s2_id;

    // Prepare few Data Records
    uint16_t rec1_tid = 256;
    uint16_t rec2_tid = 10000;
    DRec_simple rec1_a(rec1_tid);
    DRec_biflow rec1_b(rec1_tid);
    DRec_opts rec1_c(rec1_tid);
    DRec_simple rec2(rec2_tid); // control record (not to be changed)

    // Open a file for writting
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    // Session 1 - ODID 1: Add the first ("control") Transport Session and define few Data Records
    ASSERT_EQ(fds_file_session_add(file.get(), s1_def.get(), &s1_id), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), s1_id, odid1, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1_a.tmplt_type(), rec1_a.tmplt_data(), rec1_a.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec2.tmplt_type(), rec2.tmplt_data(), rec2.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec1_a.tmptl_id(), rec1_a.rec_data(), rec1_a.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec2.tmptl_id(), rec2.rec_data(), rec2.rec_size()), FDS_OK);

    // Session 2 - ODID 2: Add the seconds Transport Session (with "control" ODID)
    ASSERT_EQ(fds_file_session_add(file.get(), s2_def.get(), &s2_id), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), s2_id, odid2, exp_time), FDS_OK);
    EXPECT_EQ(fds_file_write_tmplt_remove(file.get(), rec1_a.tmptl_id()), FDS_ERR_NOTFOUND);
    ASSERT_EQ(fds_file_write_tmplt_get(file.get(), rec1_a.tmptl_id(), &tmplt_type, &tmplt_ptr,
        &tmplt_size), FDS_ERR_NOTFOUND);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1_a.tmplt_type(), rec1_a.tmplt_data(), rec1_a.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec1_a.tmptl_id(), rec1_a.rec_data(), rec1_a.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec2.tmplt_type(), rec2.tmplt_data(), rec2.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec2.tmptl_id(), rec2.rec_data(), rec2.rec_size()), FDS_OK);

    // Session 2 - ODID 1: Change writer context (ODID)
    ASSERT_EQ(fds_file_write_ctx(file.get(), s2_id, odid1, exp_time), FDS_OK);
    EXPECT_EQ(fds_file_write_tmplt_remove(file.get(), rec1_a.tmptl_id()), FDS_ERR_NOTFOUND);
    ASSERT_EQ(fds_file_write_tmplt_get(file.get(), rec1_a.tmptl_id(), &tmplt_type, &tmplt_ptr,
        &tmplt_size), FDS_ERR_NOTFOUND);
    // Define Templates and add few Data Records
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1_a.tmplt_type(), rec1_a.tmplt_data(), rec1_a.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec2.tmplt_type(), rec2.tmplt_data(), rec2.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec1_a.tmptl_id(), rec1_a.rec_data(), rec1_a.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec2.tmptl_id(), rec2.rec_data(), rec2.rec_size()), FDS_OK);
    // Redefine the Template
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1_b.tmplt_type(), rec1_b.tmplt_data(), rec1_b.tmplt_size()), FDS_OK);
    // ... and redefine it again and add a Data Record
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1_c.tmplt_type(), rec1_c.tmplt_data(), rec1_c.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec1_c.tmptl_id(), rec1_c.rec_data(), rec1_c.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec2.tmptl_id(), rec2.rec_data(), rec2.rec_size()), FDS_OK);
    // Remove the Template and try to add a Data Record
    EXPECT_EQ(fds_file_write_tmplt_remove(file.get(), rec1_c.tmptl_id()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec1_c.tmptl_id(), rec1_c.rec_data(), rec1_c.rec_size()), FDS_ERR_NOTFOUND);
    // Define the Template again and few Data Records
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1_b.tmplt_type(), rec1_b.tmplt_data(), rec1_b.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec1_b.tmptl_id(), rec1_b.rec_data(), rec1_b.rec_size()), FDS_OK);
    // Redefine the Template again, but don't add records
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1_a.tmplt_type(), rec1_a.tmplt_data(), rec1_a.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec2.tmptl_id(), rec2.rec_data(), rec2.rec_size()), FDS_OK);

    // Check if the Templates hasn't been removed or redefined in other Sessions and ODIDs
    // Session 1 - ODID1
    ASSERT_EQ(fds_file_write_ctx(file.get(), s1_id, odid1, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_get(file.get(), rec1_a.tmptl_id(), &tmplt_type, &tmplt_ptr,
        &tmplt_size), FDS_OK);
    EXPECT_TRUE(rec1_a.cmp_template(tmplt_ptr, tmplt_size));
    ASSERT_EQ(fds_file_write_tmplt_get(file.get(), rec2.tmptl_id(), &tmplt_type, &tmplt_ptr,
        &tmplt_size), FDS_OK);
    EXPECT_TRUE(rec2.cmp_template(tmplt_ptr, tmplt_size));
    // Session 2 - ODID2
    ASSERT_EQ(fds_file_write_ctx(file.get(), s2_id, odid2, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_get(file.get(), rec1_a.tmptl_id(), &tmplt_type, &tmplt_ptr,
        &tmplt_size), FDS_OK);
    EXPECT_TRUE(rec1_a.cmp_template(tmplt_ptr, tmplt_size));
    ASSERT_EQ(fds_file_write_tmplt_get(file.get(), rec2.tmptl_id(), &tmplt_type, &tmplt_ptr,
        &tmplt_size), FDS_OK);
    EXPECT_TRUE(rec2.cmp_template(tmplt_ptr, tmplt_size));

    // Close the file
    file.reset();

    // Open file for reading
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    // Internal Transport Session IDs could be different. We have to determine them...
    fds_file_sid_t *list_data;
    size_t list_size;
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 2U); // 2 Transport Sessions

    for (size_t i = 0; i < list_size; ++i) {
        // We can determine Transport Session by comparison with previously added defintions
        const struct fds_file_session *info;
        ASSERT_EQ(fds_file_session_get(file.get(), list_data[i], &info), FDS_OK);
        if (s1_def.cmp(info)) {
            s1_id = list_data[i];
        } else if (s2_def.cmp(info)) {
            s2_id = list_data[i];
        } else {
            FAIL() << "Unexpected Transport Session defintion!";
        }
    }
    free(list_data);

    struct fds_file_read_ctx rec_ctx;
    struct fds_drec rec_data;
    struct fds_drec_field field;
    /* Try to read all Data Records from the file
     * Note: Multiple Transport Sessions and ODIDs were used, so we don't know exact order in which
     * Data Records will be read. However, we can use the Transport Session and ODID filter to
     * receive only specific combination (of Session + ODID) that must preserve its order.
     */

    // Check Session 1 - ODID 1 (control)
    //  rec1_a -> rec2
    ASSERT_EQ(fds_file_read_sfilter(file.get(), &s1_id, &odid1), FDS_OK);

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec1_a.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec1_a.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid1);
    ASSERT_EQ(rec_ctx.exp_time, exp_time);

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec2.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec2.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid1);
    ASSERT_EQ(rec_ctx.exp_time, exp_time);

    EXPECT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);

    // Check Session 2 - ODID 1 (tested combination)
    //  rec1_a -> rec2 -> rec1_c -> rec2 -> rec1_b -> rec2
    ASSERT_EQ(fds_file_read_sfilter(file.get(), nullptr, nullptr), FDS_OK);
    ASSERT_EQ(fds_file_read_sfilter(file.get(), &s2_id, &odid1), FDS_OK);

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec1_a.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec1_a.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid1);
    ASSERT_EQ(rec_ctx.exp_time, exp_time);

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec2.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec2.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid1);
    ASSERT_EQ(rec_ctx.exp_time, exp_time);

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec1_c.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec1_c.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid1);
    ASSERT_EQ(rec_ctx.exp_time, exp_time);

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec2.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec2.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid1);
    ASSERT_EQ(rec_ctx.exp_time, exp_time);

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec1_b.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec1_b.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid1);
    ASSERT_EQ(rec_ctx.exp_time, exp_time);

    ASSERT_NE(fds_drec_find(&rec_data, 0, 1, &field), FDS_EOC); // octetDeltaCount
    if (m_load_iemgr) {
        ASSERT_NE(field.info->def, nullptr);
        EXPECT_NE(field.info->def->name, nullptr);
        EXPECT_EQ(field.info->def->data_type, FDS_ET_UNSIGNED_64);
        EXPECT_EQ(field.info->def->data_unit, FDS_EU_OCTETS);
    } else {
        EXPECT_EQ(field.info->def, nullptr);
    }

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec2.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec2.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid1);
    ASSERT_EQ(rec_ctx.exp_time, exp_time);

    ASSERT_NE(fds_drec_find(&rec_data, 0, 1, &field), FDS_EOC); // octetDeltaCount
    if (m_load_iemgr) {
        ASSERT_NE(field.info->def, nullptr);
        EXPECT_NE(field.info->def->name, nullptr);
        EXPECT_EQ(field.info->def->data_type, FDS_ET_UNSIGNED_64);
        EXPECT_EQ(field.info->def->data_unit, FDS_EU_OCTETS);
    } else {
        EXPECT_EQ(field.info->def, nullptr);
    }

    EXPECT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);

    // Check Session 2 - ODID 2 (control)
    //  rec1_a -> rec2
    ASSERT_EQ(fds_file_read_sfilter(file.get(), nullptr, nullptr), FDS_OK);
    ASSERT_EQ(fds_file_read_sfilter(file.get(), &s2_id, &odid2), FDS_OK);

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec1_a.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec1_a.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid2);
    ASSERT_EQ(rec_ctx.exp_time, exp_time);

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec2.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec2.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, odid2);
    ASSERT_EQ(rec_ctx.exp_time, exp_time);

    EXPECT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);
}

// Check if Flow Statistics are correctly updated
TEST_P(FileAPI, checkStats)
{
    Session session_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_UDP};
    fds_file_sid_t session_sid;
    uint32_t odid = 1;
    uint32_t exp_time = 1000;

    constexpr uint8_t NUM_TCP = 6;
    constexpr uint8_t NUM_UDP = 17;
    constexpr uint8_t NUM_ICMP4 = 1;
    constexpr uint8_t NUM_ICMP6 = 58;
    constexpr uint8_t NUM_OTHER = 255;

    uint16_t t1_id = 256; // Simple IPFIX Template
    uint16_t t2_id = 257; // Biflow IPFIX Template
    uint16_t t3_id = 258; // IPFIX Options Template
    constexpr uint64_t tcp_bytes_per_rec = 2134;
    constexpr uint64_t tcp_bytes_per_rec_rev = 10044;   // reverse direction
    constexpr uint64_t udp_bytes_per_rec = 10200;
    constexpr uint64_t udp_bytes_per_rec_rev = 81237;   // reverse direction
    constexpr uint64_t icmp_bytes_per_rec = 100;
    constexpr uint64_t icmp_bytes_per_rec_rev = 1324;   // reverse direction
    constexpr uint64_t other_bytes_per_rec = 8791;
    constexpr uint64_t other_bytes_per_rec_rev = 65157; // reverse direction
    constexpr uint64_t tcp_pkts_per_rec = 15;
    constexpr uint64_t tcp_pkts_per_rec_rev = 65;       // reverse direction
    constexpr uint64_t udp_pkts_per_rec = 141;
    constexpr uint64_t udp_pkts_per_rec_rev = 156;      // reverse direction
    constexpr uint64_t icmp_pkts_per_rec = 1;
    constexpr uint64_t icmp_pkts_per_rec_rev = 3;       // reverse direction
    constexpr uint64_t other_pkts_per_rec = 23;
    constexpr uint64_t other_pkts_per_rec_rev = 214;    // reverse direction

    // Prepare Data Records (simple and biflow)
    DRec_simple t1_rec_tcp(t1_id, 0, 0, NUM_TCP, tcp_bytes_per_rec, tcp_pkts_per_rec);
    DRec_simple t1_rec_udp(t1_id, 0, 0, NUM_UDP, udp_bytes_per_rec, udp_pkts_per_rec);
    DRec_simple t1_rec_icmp4(t1_id, 0, 0, NUM_ICMP4, icmp_bytes_per_rec, icmp_pkts_per_rec);
    DRec_simple t1_rec_icmp6(t1_id, 0, 0, NUM_ICMP6, icmp_bytes_per_rec, icmp_pkts_per_rec);
    DRec_simple t1_rec_other(t1_id, 0, 0, NUM_OTHER, other_bytes_per_rec, other_pkts_per_rec);
    DRec_biflow t2_rec_tcp(t2_id, "a", "b", 0, 0, NUM_TCP, tcp_bytes_per_rec, tcp_pkts_per_rec,
        tcp_bytes_per_rec_rev, tcp_pkts_per_rec_rev);
    DRec_biflow t2_rec_udp(t2_id, "a", "b", 0, 0, NUM_UDP, udp_bytes_per_rec, udp_pkts_per_rec,
        udp_bytes_per_rec_rev, udp_pkts_per_rec_rev);
    DRec_biflow t2_rec_icmp4(t2_id, "a", "b", 0, 0, NUM_ICMP4, icmp_bytes_per_rec, icmp_pkts_per_rec,
        icmp_bytes_per_rec_rev, icmp_pkts_per_rec_rev);
    DRec_biflow t2_rec_icmp6(t2_id, "a", "b", 0, 0, NUM_ICMP6, icmp_bytes_per_rec, icmp_pkts_per_rec,
        icmp_bytes_per_rec_rev, icmp_pkts_per_rec_rev);
    DRec_biflow t2_rec_other(t2_id, "a", "b", 0, 0, NUM_OTHER, other_bytes_per_rec, other_pkts_per_rec,
        other_bytes_per_rec_rev, other_pkts_per_rec_rev);
    DRec_opts t3_rec_opts(t3_id); // Options Template (no bytes, no packets, no protocol)

    // Open a file for writing, add the Transport Session and Templates
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session_def.get(), &session_sid), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), session_sid, odid, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), t1_rec_tcp.tmplt_type(), t1_rec_tcp.tmplt_data(),
        t1_rec_tcp.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), t2_rec_tcp.tmplt_type(), t2_rec_tcp.tmplt_data(),
        t2_rec_tcp.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), t3_rec_opts.tmplt_type(), t3_rec_opts.tmplt_data(),
        t3_rec_opts.tmplt_size()), FDS_OK);

    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    // All stats should be set to zero
    const struct fds_file_stats *file_stats;
    file_stats = fds_file_stats_get(file.get());
    ASSERT_NE(file_stats, nullptr);

    // Note: the structure is "packed", therefore, we can use memcmp to compare it with expected one
    struct fds_file_stats my_stats;
    memset(&my_stats, 0, sizeof my_stats);
    EXPECT_EQ(memcmp(file_stats, &my_stats, sizeof my_stats), 0);

    // Add TCP Data Record and analyze changes
    ASSERT_EQ(fds_file_write_rec(file.get(), t1_rec_tcp.tmptl_id(), t1_rec_tcp.rec_data(),
        t1_rec_tcp.rec_size()), FDS_OK);
    my_stats.recs_total++;
    my_stats.recs_tcp++;
    my_stats.bytes_total += tcp_bytes_per_rec;
    my_stats.bytes_tcp += tcp_bytes_per_rec;
    my_stats.pkts_total += tcp_pkts_per_rec;
    my_stats.pkts_tcp += tcp_pkts_per_rec;
    file_stats = fds_file_stats_get(file.get());
    EXPECT_EQ(memcmp(file_stats, &my_stats, sizeof my_stats), 0);

    // Add UDP Data Record and analyze changes
    ASSERT_EQ(fds_file_write_rec(file.get(), t1_rec_udp.tmptl_id(), t1_rec_udp.rec_data(),
        t1_rec_udp.rec_size()), FDS_OK);
    my_stats.recs_total++;
    my_stats.recs_udp++;
    my_stats.bytes_total += udp_bytes_per_rec;
    my_stats.bytes_udp += udp_bytes_per_rec;
    my_stats.pkts_total += udp_pkts_per_rec;
    my_stats.pkts_udp += udp_pkts_per_rec;
    file_stats = fds_file_stats_get(file.get());
    EXPECT_EQ(memcmp(file_stats, &my_stats, sizeof my_stats), 0);

    // Add ICMP4 Data Record and analyze changes
    ASSERT_EQ(fds_file_write_rec(file.get(), t1_rec_icmp4.tmptl_id(), t1_rec_icmp4.rec_data(),
        t1_rec_icmp4.rec_size()), FDS_OK);
    my_stats.recs_total++;
    my_stats.recs_icmp++;
    my_stats.bytes_total += icmp_bytes_per_rec;
    my_stats.bytes_icmp += icmp_bytes_per_rec;
    my_stats.pkts_total += icmp_pkts_per_rec;
    my_stats.pkts_icmp += icmp_pkts_per_rec;
    file_stats = fds_file_stats_get(file.get());
    EXPECT_EQ(memcmp(file_stats, &my_stats, sizeof my_stats), 0);

    // Add ICMP4 Data Record and analyze changes
    ASSERT_EQ(fds_file_write_rec(file.get(), t1_rec_icmp6.tmptl_id(), t1_rec_icmp6.rec_data(),
        t1_rec_icmp6.rec_size()), FDS_OK);
    my_stats.recs_total++;
    my_stats.recs_icmp++;
    my_stats.bytes_total += icmp_bytes_per_rec;
    my_stats.bytes_icmp += icmp_bytes_per_rec;
    my_stats.pkts_total += icmp_pkts_per_rec;
    my_stats.pkts_icmp += icmp_pkts_per_rec;
    file_stats = fds_file_stats_get(file.get());
    EXPECT_EQ(memcmp(file_stats, &my_stats, sizeof my_stats), 0);

    // Add "other" Data Record and analyze changes
    ASSERT_EQ(fds_file_write_rec(file.get(), t1_rec_other.tmptl_id(), t1_rec_other.rec_data(),
        t1_rec_other.rec_size()), FDS_OK);
    my_stats.recs_total++;
    my_stats.recs_other++;
    my_stats.bytes_total += other_bytes_per_rec;
    my_stats.bytes_other += other_bytes_per_rec;
    my_stats.pkts_total += other_pkts_per_rec;
    my_stats.pkts_other += other_pkts_per_rec;
    file_stats = fds_file_stats_get(file.get());
    EXPECT_EQ(memcmp(file_stats, &my_stats, sizeof my_stats), 0);

    // Add Options Template Data Record and analyze changes
    ASSERT_EQ(fds_file_write_rec(file.get(), t3_rec_opts.tmptl_id(), t3_rec_opts.rec_data(),
        t3_rec_opts.rec_size()), FDS_OK);
    my_stats.recs_total++;
    my_stats.recs_opts_total++;
    EXPECT_EQ(memcmp(file_stats, &my_stats, sizeof my_stats), 0);

    // Add TCP Biflow Data Record and analyze changes
    ASSERT_EQ(fds_file_write_rec(file.get(), t2_rec_tcp.tmptl_id(), t2_rec_tcp.rec_data(),
        t2_rec_tcp.rec_size()), FDS_OK);
    my_stats.recs_total++;
    my_stats.recs_tcp++;
    my_stats.recs_bf_total++;
    my_stats.recs_bf_tcp++;
    my_stats.bytes_total += tcp_bytes_per_rec + tcp_bytes_per_rec_rev;
    my_stats.bytes_tcp += tcp_bytes_per_rec + tcp_bytes_per_rec_rev;
    my_stats.pkts_total += tcp_pkts_per_rec + tcp_pkts_per_rec_rev;
    my_stats.pkts_tcp += tcp_pkts_per_rec + tcp_pkts_per_rec_rev;
    file_stats = fds_file_stats_get(file.get());
    EXPECT_EQ(memcmp(file_stats, &my_stats, sizeof my_stats), 0);

    // Add UDP Biflow Data Record and analyze changes
    ASSERT_EQ(fds_file_write_rec(file.get(), t2_rec_udp.tmptl_id(), t2_rec_udp.rec_data(),
        t2_rec_udp.rec_size()), FDS_OK);
    my_stats.recs_total++;
    my_stats.recs_udp++;
    my_stats.recs_bf_total++;
    my_stats.recs_bf_udp++;
    my_stats.bytes_total += udp_bytes_per_rec + udp_bytes_per_rec_rev;
    my_stats.bytes_udp += udp_bytes_per_rec + udp_bytes_per_rec_rev;
    my_stats.pkts_total += udp_pkts_per_rec + udp_pkts_per_rec_rev;
    my_stats.pkts_udp += udp_pkts_per_rec + udp_pkts_per_rec_rev;
    file_stats = fds_file_stats_get(file.get());
    EXPECT_EQ(memcmp(file_stats, &my_stats, sizeof my_stats), 0);

    // Add ICMP4 Biflow Data Record and analyze changes
    ASSERT_EQ(fds_file_write_rec(file.get(), t2_rec_icmp4.tmptl_id(), t2_rec_icmp4.rec_data(),
        t2_rec_icmp4.rec_size()), FDS_OK);
    my_stats.recs_total++;
    my_stats.recs_icmp++;
    my_stats.recs_bf_total++;
    my_stats.recs_bf_icmp++;
    my_stats.bytes_total += icmp_bytes_per_rec + icmp_bytes_per_rec_rev;
    my_stats.bytes_icmp += icmp_bytes_per_rec + icmp_bytes_per_rec_rev;
    my_stats.pkts_total += icmp_pkts_per_rec + icmp_pkts_per_rec_rev;
    my_stats.pkts_icmp += icmp_pkts_per_rec + icmp_pkts_per_rec_rev;
    file_stats = fds_file_stats_get(file.get());
    EXPECT_EQ(memcmp(file_stats, &my_stats, sizeof my_stats), 0);

    // Add ICMP6 Biflow Data Record and analyze changes
    ASSERT_EQ(fds_file_write_rec(file.get(), t2_rec_icmp6.tmptl_id(), t2_rec_icmp6.rec_data(),
        t2_rec_icmp6.rec_size()), FDS_OK);
    my_stats.recs_total++;
    my_stats.recs_icmp++;
    my_stats.recs_bf_total++;
    my_stats.recs_bf_icmp++;
    my_stats.bytes_total += icmp_bytes_per_rec + icmp_bytes_per_rec_rev;
    my_stats.bytes_icmp += icmp_bytes_per_rec + icmp_bytes_per_rec_rev;
    my_stats.pkts_total += icmp_pkts_per_rec + icmp_pkts_per_rec_rev;
    my_stats.pkts_icmp += icmp_pkts_per_rec + icmp_pkts_per_rec_rev;
    file_stats = fds_file_stats_get(file.get());
    EXPECT_EQ(memcmp(file_stats, &my_stats, sizeof my_stats), 0);

    // Add "other" Biflow Data Record and analyze changes
    ASSERT_EQ(fds_file_write_rec(file.get(), t2_rec_other.tmptl_id(), t2_rec_other.rec_data(),
        t2_rec_other.rec_size()), FDS_OK);
    my_stats.recs_total++;
    my_stats.recs_other++;
    my_stats.recs_bf_total++;
    my_stats.recs_bf_other++;
    my_stats.bytes_total += other_bytes_per_rec + other_bytes_per_rec_rev;
    my_stats.bytes_other += other_bytes_per_rec + other_bytes_per_rec_rev;
    my_stats.pkts_total += other_pkts_per_rec + other_pkts_per_rec_rev;
    my_stats.pkts_other += other_pkts_per_rec + other_pkts_per_rec_rev;
    file_stats = fds_file_stats_get(file.get());
    EXPECT_EQ(memcmp(file_stats, &my_stats, sizeof my_stats), 0);

    // Close the file
    file.reset();

    // Open the file for reading and check stats
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);

    file_stats = fds_file_stats_get(file.get());
    ASSERT_NE(file_stats, nullptr);
    EXPECT_EQ(memcmp(file_stats, &my_stats, sizeof my_stats), 0);
}

// Check if list of Transport Session and ODIDs are properly updated while writing
TEST_P(FileAPI, listSessionAndOdids)
{
    // List of Transport Session and ODIDs
    fds_file_sid_t *list_data;
    size_t list_size;
    uint32_t *odid_data;
    size_t odid_size;

    // Create few Transport Session descriptions
    Session s1_def {"255.255.255.0", "10.10.10.10", 123, 789, FDS_FILE_SESSION_TCP};
    Session s2_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_UDP};
    Session s3_def {"192.168.10.12", "245.255.0.1", 10, 9999, FDS_FILE_SESSION_SCTP};
    fds_file_sid_t s1_id, s2_id, s3_id;
    const struct fds_file_session *session_ptr;

    // Create few Data Records
    uint16_t rec1_tid = 256;
    uint16_t rec2_tid = 300;
    DRec_simple rec1(rec1_tid);
    DRec_opts rec2(rec2_tid);

    // Create a file for writing
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);

    if (m_load_iemgr) {
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    EXPECT_EQ(list_size, 0U);
    free(list_data);

    // Add a Transport Session
    ASSERT_EQ(fds_file_session_add(file.get(), s1_def.get(), &s1_id), FDS_OK);
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 1U);
    EXPECT_EQ(list_data[0], s1_id);
    ASSERT_EQ(fds_file_session_odids(file.get(), s1_id, &odid_data, &odid_size), FDS_OK);
    ASSERT_EQ(odid_size, 0U);
    free(odid_data);
    free(list_data);
    ASSERT_EQ(fds_file_session_get(file.get(), s1_id, &session_ptr), FDS_OK);
    ASSERT_NE(session_ptr, nullptr);
    EXPECT_TRUE(s1_def.cmp(session_ptr));

    // Try to add another Transport Session
    ASSERT_EQ(fds_file_session_add(file.get(), s2_def.get(), &s2_id), FDS_OK);
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 2U);
    free(list_data);

    // Try to add the same Session definition
    fds_file_sid_t aux_sid;
    ASSERT_EQ(fds_file_session_add(file.get(), s2_def.get(), &aux_sid), FDS_OK);
    // Expect that the definition is not added and the ID is still the same
    EXPECT_EQ(aux_sid, s2_id);
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 2U);
    free(list_data);

    // Try to call it with invalid parameters
    EXPECT_EQ(fds_file_session_add(file.get(), nullptr, nullptr), FDS_ERR_ARG);
    EXPECT_EQ(fds_file_session_add(file.get(), s3_def.get(), nullptr), FDS_ERR_ARG);
    EXPECT_EQ(fds_file_session_add(file.get(), nullptr, &aux_sid), FDS_ERR_ARG);
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 2U);
    free(list_data);

    // Try to add some Data Records and see if particular ODID lists have been changed
    uint32_t s1_odid1 = 213;
    uint32_t s1_odid2 = 48798;
    ASSERT_EQ(fds_file_write_ctx(file.get(), s1_id, s1_odid1, 0), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1.tmplt_type(), rec1.tmplt_data(), rec1.tmplt_size()), FDS_OK);
    EXPECT_EQ(fds_file_write_rec(file.get(), rec1.tmptl_id(), rec1.rec_data(), rec1.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), s1_id, s1_odid2, 0), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec2.tmplt_type(), rec2.tmplt_data(), rec2.tmplt_size()), FDS_OK);
    EXPECT_EQ(fds_file_write_rec(file.get(), rec2.tmptl_id(), rec2.rec_data(), rec2.rec_size()), FDS_OK);

    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 2U);
    free(list_data);

    ASSERT_EQ(fds_file_session_odids(file.get(), s1_id, &odid_data, &odid_size), FDS_OK);
    ASSERT_EQ(odid_size, 2U); // We added 2 different ODIDs
    std::vector<uint32_t> odid_wrap(odid_data, odid_data + odid_size);
    EXPECT_NE(std::find(odid_wrap.begin(), odid_wrap.end(), s1_odid1), odid_wrap.end());
    EXPECT_NE(std::find(odid_wrap.begin(), odid_wrap.end(), s1_odid2), odid_wrap.end());
    free(odid_data);

    // Check if the second Transport Session hasn't been changed
    ASSERT_EQ(fds_file_session_odids(file.get(), s2_id, &odid_data, &odid_size), FDS_OK);
    EXPECT_EQ(odid_size, 0U);
    free(odid_data);

    // Try to add another Transport Session with few Data Records from different ODIDs
    uint32_t s3_odid1 = 112;
    uint32_t s3_odid2 = 213;
    uint32_t s3_odid3 = 897458;
    ASSERT_EQ(fds_file_session_add(file.get(), s3_def.get(), &s3_id), FDS_OK);
    // ODID 1
    ASSERT_EQ(fds_file_write_ctx(file.get(), s3_id, s3_odid1, 0), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1.tmplt_type(), rec1.tmplt_data(), rec1.tmplt_size()), FDS_OK);
    EXPECT_EQ(fds_file_write_rec(file.get(), rec1.tmptl_id(), rec1.rec_data(), rec1.rec_size()), FDS_OK);
    // ODID 2
    ASSERT_EQ(fds_file_write_ctx(file.get(), s3_id, s3_odid2, 0), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec2.tmplt_type(), rec2.tmplt_data(), rec2.tmplt_size()), FDS_OK);
    EXPECT_EQ(fds_file_write_rec(file.get(), rec2.tmptl_id(), rec2.rec_data(), rec2.rec_size()), FDS_OK);
    // ODID 3 (different export time)
    ASSERT_EQ(fds_file_write_ctx(file.get(), s3_id, s3_odid3, 10), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1.tmplt_type(), rec1.tmplt_data(), rec1.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec2.tmplt_type(), rec2.tmplt_data(), rec2.tmplt_size()), FDS_OK);
    EXPECT_EQ(fds_file_write_rec(file.get(), rec1.tmptl_id(), rec1.rec_data(), rec1.rec_size()), FDS_OK);
    EXPECT_EQ(fds_file_write_rec(file.get(), rec2.tmptl_id(), rec2.rec_data(), rec2.rec_size()), FDS_OK);

    // Try to get the definition and check if the proper ODIDs are on the list
    ASSERT_EQ(fds_file_session_get(file.get(), s3_id, &session_ptr), FDS_OK);
    EXPECT_TRUE(s3_def.cmp(session_ptr));
    ASSERT_EQ(fds_file_session_odids(file.get(), s3_id, &odid_data, &odid_size), FDS_OK);
    ASSERT_EQ(odid_size, 3U); // In the previous step we added 3 different ODIDs
    odid_wrap = std::vector<uint32_t>(odid_data, odid_data + odid_size);
    EXPECT_NE(std::find(odid_wrap.begin(), odid_wrap.end(), s3_odid1), odid_wrap.end());
    EXPECT_NE(std::find(odid_wrap.begin(), odid_wrap.end(), s3_odid2), odid_wrap.end());
    EXPECT_NE(std::find(odid_wrap.begin(), odid_wrap.end(), s3_odid3), odid_wrap.end());
    free(odid_data);

    // Check that all Transport Sessions are on the list
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 3U); // 3 Transport Sessions have been previously added
    std::vector<fds_file_sid_t> list_wrap(list_data, list_data + list_size);
    EXPECT_NE(std::find(list_wrap.begin(), list_wrap.end(), s1_id), list_wrap.end());
    EXPECT_NE(std::find(list_wrap.begin(), list_wrap.end(), s2_id), list_wrap.end());
    EXPECT_NE(std::find(list_wrap.begin(), list_wrap.end(), s3_id), list_wrap.end());
    free(list_data);

    // Close the file
    file.reset();

    // Open the file for reading  -------------------------------------------------------------
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    // Extract all Transport Sessions in the file
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_NE(list_data, nullptr);
    ASSERT_EQ(list_size, 3U); // 3 Transport Sessions are expected
    list_wrap = std::vector<fds_file_sid_t>(list_data, list_data + list_size);
    free(list_data);

    const auto exp_sessions = { // List of expected Transport Sessions
        std::make_pair(std::ref(s1_def), std::vector<uint32_t> {s1_odid1, s1_odid2}),
        std::make_pair(std::ref(s2_def), std::vector<uint32_t> {}),
        std::make_pair(std::ref(s3_def), std::vector<uint32_t> {s3_odid1, s3_odid2, s3_odid3})
    };

    for (const auto &pair : exp_sessions) {
        // Try to find each Transport Session Definition in the file
        const Session &session_ref = pair.first;
        auto lambda = [&file, &session_ref](fds_file_sid_t sid) -> bool {
            // Get a description of the given Session ID and compare it with the required one
            const struct fds_file_session *info;
            if (fds_file_session_get(file.get(), sid, &info) != FDS_OK) {
                return false;
            }
            return session_ref.cmp(info);
        };

        // Get an internal Transport Session ID that matches the current Transport Session definition
        auto result_it = std::find_if(list_wrap.begin(), list_wrap.end(), lambda);
        ASSERT_NE(result_it, list_wrap.end()) << "Transport Session description not found!";
        fds_file_sid_t sid = (*result_it);

        // List all Observation Domain IDs of this Transport Session
        ASSERT_EQ(fds_file_session_odids(file.get(), sid, &odid_data, &odid_size), FDS_OK);
        ASSERT_EQ(odid_size, pair.second.size());
        odid_wrap = std::vector<uint32_t>(odid_data, odid_data + odid_size);
        free(odid_data);

        for (uint32_t odid : pair.second) {
            // Try to find expected ODID in the list of ODIDs of this Transport Session
            SCOPED_TRACE("odid: " + std::to_string(odid));
            EXPECT_NE(std::find(odid_wrap.begin(), odid_wrap.end(), odid), odid_wrap.end());
        }
    }
}

// Use the same file handler to write and read the same file
TEST_P(FileAPI, reuseHandler)
{
    // Prepare a Transport Session and a Data Record
    Session s1_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_UDP};
    fds_file_sid_t s1_id;
    DRec_biflow rec(256);

    // Create a file for writing
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);

    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    // Add the Transport Session and Data Record
    ASSERT_EQ(fds_file_session_add(file.get(), s1_def.get(), &s1_id), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), s1_id, 0, 0), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec.tmplt_type(), rec.tmplt_data(),
        rec.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), rec.tmptl_id(), rec.rec_data(), rec.rec_size()),
        FDS_OK);

    // Open the file for reading using the same handler
    EXPECT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);

    // Get the Data Record
    struct fds_file_read_ctx rec_ctx;
    struct fds_drec rec_data;
    struct fds_drec_field field;

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(rec.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(rec.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, 0U);
    ASSERT_EQ(rec_ctx.exp_time, 0U);

    ASSERT_NE(fds_drec_find(&rec_data, 0, 1, &field), FDS_EOC); // octetDeltaCount
    if (m_load_iemgr) {
        ASSERT_NE(field.info->def, nullptr);
        EXPECT_NE(field.info->def->name, nullptr);
        EXPECT_EQ(field.info->def->data_type, FDS_ET_UNSIGNED_64);
        EXPECT_EQ(field.info->def->data_unit, FDS_EU_OCTETS);
    } else {
        EXPECT_EQ(field.info->def, nullptr);
    }

    EXPECT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);
}

// Try to (at least) partly read an empty file which is opened for writting by someone else
TEST_P(FileAPI, readEmptyFileWhichIsBeingWritten)
{
    // Open a file for writing and leave it opened
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file_write(fds_file_init(), &fds_file_close);
    ASSERT_NE(file_write, nullptr);
    ASSERT_EQ(fds_file_open(file_write.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file_write.get(), m_iemgr), FDS_OK);
    }

    // Try to open the file for reading
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file_read(fds_file_init(), &fds_file_close);
    ASSERT_NE(file_read, nullptr);
    ASSERT_EQ(fds_file_open(file_read.get(), m_filename.c_str(), m_flags_read), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file_read.get(), m_iemgr), FDS_OK);
    }

    // Try to get a Data Record
    struct fds_file_read_ctx rec_ctx;
    struct fds_drec rec_data;
    EXPECT_EQ(fds_file_read_rec(file_read.get(), &rec_data, &rec_ctx), FDS_EOC);

    // Try to list Transport Sessions
    fds_file_sid_t *list_data = nullptr;
    size_t list_size;
    ASSERT_EQ(fds_file_session_list(file_read.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 0U);
    free(list_data);
}

/* Try to (at least) partly read a non-empty file which is opened for writting by someone else.
 * To make sure that at least some records are written to the file, we will close it and reopen
 * in the append mode first. Then we will try to read it.
 */
TEST_P(FileAPI, readNonEmptyFileWhichIsBeingWritten)
{
    uint32_t s1_odid = 547; // random values
    uint32_t s2_odid = 8741;
    uint32_t exp_time = 165870;

    // Transport Session
    Session s1_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_UDP};
    Session s2_def {"192.168.10.12", "245.255.0.1", 10, 9999, FDS_FILE_SESSION_UDP};
    fds_file_sid_t s1_id, s2_id;

    // Prepare few Data Records
    DRec_simple s1_rec_a(256);
    DRec_opts s1_rec_b(300);
    DRec_biflow s2_rec(257);

    // Open a file for writing, add Transport Sessions and Data Records
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), s1_def.get(), &s1_id), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), s2_def.get(), &s2_id), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), s1_id, s1_odid, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), s1_rec_a.tmplt_type(), s1_rec_a.tmplt_data(),
        s1_rec_a.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), s1_rec_b.tmplt_type(), s1_rec_b.tmplt_data(),
        s1_rec_b.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s1_rec_a.tmptl_id(), s1_rec_a.rec_data(),
        s1_rec_a.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s1_rec_b.tmptl_id(), s1_rec_b.rec_data(),
        s1_rec_b.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), s2_id, s2_odid, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), s2_rec.tmplt_type(), s2_rec.tmplt_data(),
        s2_rec.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s2_rec.tmptl_id(), s2_rec.rec_data(),
        s2_rec.rec_size()), FDS_OK);

    // Reopen the file in append mode (all Transport Sessions and Data Records should be flushed)
    uint32_t append_flags = write2append_flag(m_flags_write);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), append_flags), FDS_OK);
    // Add few more Data Records ()
    ASSERT_EQ(fds_file_session_add(file.get(), s1_def.get(), &s1_id), FDS_OK); // Same as previous
    ASSERT_EQ(fds_file_write_ctx(file.get(), s1_id, s1_odid, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), s1_rec_a.tmplt_type(), s1_rec_a.tmplt_data(),
        s1_rec_a.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s1_rec_a.tmptl_id(), s1_rec_a.rec_data(),
        s1_rec_a.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s1_rec_a.tmptl_id(), s1_rec_a.rec_data(),
        s1_rec_a.rec_size()), FDS_OK);

    // Leave the file opened!

    // Try to open the file for reading
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file_read(fds_file_init(), &fds_file_close);
    ASSERT_NE(file_read, nullptr);
    ASSERT_EQ(fds_file_open(file_read.get(), m_filename.c_str(), m_flags_read), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file_read.get(), m_iemgr), FDS_OK);
    }

    // Get all Transport Session (all definitions should be known)
    fds_file_sid_t *list_data;
    size_t list_size;
    ASSERT_EQ(fds_file_session_list(file_read.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 2U); // 2 Transport Sessions
    EXPECT_NE(list_data[0], list_data[1]);

    for (size_t i = 0; i < list_size; ++i) {
        // We can determine Transport Session by comparison with previously added defintions
        const struct fds_file_session *info;
        ASSERT_EQ(fds_file_session_get(file_read.get(), list_data[i], &info), FDS_OK);
        if (s1_def.cmp(info)) {
            s1_id = list_data[i];
        } else if (s2_def.cmp(info)) {
            s2_id = list_data[i];
        } else {
            FAIL() << "Unexpected Transport Session defintion!";
        }
    }
    free(list_data);

    // Get Data Records (at least records written before appending must be available)
    struct fds_file_read_ctx rec_ctx;
    struct fds_drec rec_data;
    int rc;

    size_t s1_rec_a_cnt = 0;
    size_t s1_rec_b_cnt = 0;
    size_t s2_rec_cnt = 0;

    while ((rc = fds_file_read_rec(file_read.get(), &rec_data, &rec_ctx)) == FDS_OK) {
        // Determine Session
        if (rec_ctx.sid == s1_id) {
            // Transport Session 1
            if (s1_rec_a.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length)) {
                ASSERT_TRUE(s1_rec_a.cmp_record(rec_data.data, rec_data.size));
                ASSERT_EQ(rec_ctx.odid, s1_odid);
                ASSERT_EQ(rec_ctx.exp_time, exp_time);
                s1_rec_a_cnt++;
            } else if (s1_rec_b.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length)) {
                ASSERT_TRUE(s1_rec_b.cmp_record(rec_data.data, rec_data.size));
                ASSERT_EQ(rec_ctx.odid, s1_odid);
                ASSERT_EQ(rec_ctx.exp_time, exp_time);
                s1_rec_b_cnt++;
            } else {
                FAIL() << "Unexpected Data Record!";
            }
        } else if (rec_ctx.sid == s2_id) {
            // Transport Session 2
            ASSERT_TRUE(s2_rec.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
            ASSERT_TRUE(s2_rec.cmp_record(rec_data.data, rec_data.size));
            ASSERT_EQ(rec_ctx.odid, s2_odid);
            ASSERT_EQ(rec_ctx.exp_time, exp_time);
            s2_rec_cnt++;
        } else {
            FAIL() << "Unexpected Transport Session!";
        }
    }

    // Reader should finish normally
    EXPECT_EQ(rc, FDS_EOC);

    // Before flush (i.e. reopen for appending) few Data Records have been added
    EXPECT_GE(s1_rec_a_cnt, 1U);
    EXPECT_GE(s1_rec_b_cnt, 1U);
    EXPECT_GE(s2_rec_cnt, 1U);
}
