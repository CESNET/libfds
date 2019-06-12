/**
 * @file file_complex.cpp
 * @author Lukas Hutak (lukas.hutak@cesnet.cz)
 * @date June 2019
 * @brief
 *   Complex test cases using FDS File API
 *
 * The tests usually try to create large files with multiple different Data Records from
 * various Transport Sessions and/or Observation Domain IDs.
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 *
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
INSTANTIATE_TEST_CASE_P(Complex, FileAPI, product, &product_name);

/*
 * Write a lot of Data Records (based on the same Template) from a single Transport Session with
 * the same ODID and try to read them.
 *
 * The goal is to create a lot of Data Blocks in the file and test if the Data Blocks are stored
 * when the maximal capacity is reached.
 */
TEST_P(FileAPI, recordsFromSingleSourceAndODID)
{
    uint32_t odid = 9998894U;
    uint32_t exp_time = 0;

    // Create a Transport Session description
    Session session2write{"255.255.255.0", "10.10.10.10", 123, 789, FDS_FILE_SESSION_TCP};
    fds_file_sid_t session_sid;

    // Records based on the same Template
    uint16_t rec_tid = 1234;
    DRec_biflow rec1(rec_tid, "first", "eth0", 123, 789);
    DRec_biflow rec2(rec_tid, "second_string_is_slightly_longer", "eth1", 7891, 11, 6);

    // Open a file for writing, add the Transport Session and the IPFIX Template
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session2write.get(), &session_sid), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), session_sid, odid, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1.tmplt_type(), rec1.tmplt_data(), rec1.tmplt_size()), FDS_OK);

    // Add Data Records (change the record every iteration
    constexpr size_t cnt = 500000;
    for (size_t i = 0; i < cnt; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));
        // Update Export Time each 33 records
        if (i % 33 == 0) {
            ASSERT_EQ(fds_file_write_ctx(file.get(), session_sid, odid, ++exp_time), FDS_OK);
        }

        if (i % 2 == 0) {
            ASSERT_EQ(fds_file_write_rec(file.get(), rec_tid, rec1.rec_data(), rec1.rec_size()), FDS_OK);
        } else {
            ASSERT_EQ(fds_file_write_rec(file.get(), rec_tid, rec2.rec_data(), rec2.rec_size()), FDS_OK);
        }
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

    // Extract all Transport Sessions in the file (only one should be present)
    fds_file_sid_t *list_data;
    size_t list_size;
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 1U);
    fds_file_sid_t src_id = list_data[0]; // Get the internal ID of the Transport Session
    free(list_data);

    // Get a pointer to the Transport Session description
    const struct fds_file_session *src_desc;
    ASSERT_EQ(fds_file_session_get(file.get(), src_id, &src_desc), FDS_OK);
    EXPECT_TRUE(session2write.cmp(src_desc));

    // Try to get all Data Records
    struct fds_file_read_ctx rec_ctx;
    struct fds_drec rec_data;

    exp_time = 0;
    for (size_t i = 0; i < cnt; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));

        // Update expected Export Time
        if (i % 33 == 0) {
            ++exp_time;
        }
        // Update pointer to the expected record
        DRec_base *base_ptr = (i % 2 == 0) ? &rec1 : &rec2;

        // Clean the content of the record structures to detect if values are always set
        memset(&rec_data, 0, sizeof rec_data);
        memset(&rec_ctx, 0, sizeof rec_ctx);
        // Get the record and check its values
        ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
        EXPECT_TRUE(base_ptr->cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
        EXPECT_TRUE(base_ptr->cmp_record(rec_data.data, rec_data.size));
        EXPECT_EQ(rec_ctx.odid, odid);
        EXPECT_EQ(rec_ctx.exp_time, exp_time);
        EXPECT_EQ(rec_ctx.sid, src_id);
    }

    // No more Data Records
    EXPECT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);
}

/*
 * Write a lot of Data Records from a single Transport Session but with different ODIDs
 *
 * For each ODID, Templates of Data Records are different, but the same Template IDs are used.
 */
TEST_P(FileAPI, recordsFromSingleSourceAndMultipleODIDs)
{
    // Prepare IPFIX Data Records
    constexpr uint32_t odid1 = 10;
    constexpr uint32_t odid2 = 5;
    constexpr uint32_t odid3 = 2000;
    uint16_t tid = 300; // Template ID
    uint32_t exp_time = 1000;

    DRec_simple rec1(tid);
    DRec_biflow rec2(tid);
    DRec_opts   rec3(tid);

    // Create a Transport Session description
    Session session2write{"fe80::f0b9:5fc4:1c28:aab2", "2001:67c::a371", 22, 23, FDS_FILE_SESSION_UDP};
    fds_file_sid_t session_sid;

    // Open a file for writing, add the Transport Session
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), session2write.get(), &session_sid), FDS_OK);

    // Define the Template for each ODID
    ASSERT_EQ(fds_file_write_ctx(file.get(), session_sid, odid1, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec1.tmplt_type(), rec1.tmplt_data(), rec1.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), session_sid, odid2, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec2.tmplt_type(), rec2.tmplt_data(), rec2.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), session_sid, odid3, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec3.tmplt_type(), rec3.tmplt_data(), rec3.tmplt_size()), FDS_OK);

    // Write some Data Records (change ODID after each write)
    constexpr size_t cnt = 300000;
    for (size_t i = 0; i < cnt; ++i) {
        SCOPED_TRACE("i:" + std::to_string(i));
        DRec_base *rec_ptr;
        uint32_t odid;

        switch (i % 3) {
        case 0:
            rec_ptr = &rec1;
            odid = odid1;
            break;
        case 1:
            rec_ptr = &rec2;
            odid = odid2;
            break;
        case 2:
            rec_ptr = &rec3;
            odid = odid3;
            break;
        default:
            FAIL() << "Invalid switch/case value";
        }

        if (i % 66 == 0) { // After 22 Data Records of each ODID
            exp_time += 11;
        }

        ASSERT_EQ(fds_file_write_ctx(file.get(), session_sid, odid, exp_time), FDS_OK);
        ASSERT_EQ(fds_file_write_rec(file.get(), tid, rec_ptr->rec_data(), rec_ptr->rec_size()), FDS_OK);
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

    // Check the Transport Session description
    fds_file_sid_t *list_data;
    size_t list_size;
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 1U);
    fds_file_sid_t session2get = list_data[0];
    free(list_data);

    const struct fds_file_session *session_desc;
    ASSERT_EQ(fds_file_session_get(file.get(), session2get, &session_desc), FDS_OK);
    EXPECT_TRUE(session2write.cmp(session_desc));

    // List all available ODIDs of the session
    uint32_t *odid_data;
    size_t odid_size;
    ASSERT_EQ(fds_file_session_odids(file.get(), session2get, &odid_data, &odid_size), FDS_OK);
    ASSERT_EQ(odid_size, 3U); // We previously added exactly 3 different ODIDs

    std::vector<uint32_t> odid_vector(odid_data, odid_data + odid_size);
    EXPECT_NE(std::find(odid_vector.begin(), odid_vector.end(), odid1), odid_vector.end());
    EXPECT_NE(std::find(odid_vector.begin(), odid_vector.end(), odid2), odid_vector.end());
    EXPECT_NE(std::find(odid_vector.begin(), odid_vector.end(), odid3), odid_vector.end());
    free(odid_data);

    /* Try to check all Data Records
     * Because Data Records from different ODIDs are not stored in the order, we must detect
     * ODID, check the Data Record and update a counter for the proper ODID.
     */
    std::map<uint32_t, unsigned int> counter;

    struct fds_drec rec_data;
    struct fds_file_read_ctx rec_ctx;
    exp_time = 1000;


    for (size_t i = 0; i < cnt; ++i) {
        SCOPED_TRACE("i:" + std::to_string(i));
        DRec_base *rec_ptr;

        // Clean the content of the record structures
        memset(&rec_data, 0, sizeof rec_data);
        memset(&rec_ctx, 0, sizeof rec_ctx);

        // Get the Data Record
        ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
        switch (rec_ctx.odid) {
        case odid1:
            rec_ptr = &rec1;
            break;
        case odid2:
            rec_ptr = &rec2;
            break;
        case odid3:
            rec_ptr = &rec3;
            break;
        default:
            FAIL() << "Unexpected ODID!";
        }

        // Compare the Data Record and Template
        ASSERT_TRUE(rec_ptr->cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
        ASSERT_TRUE(rec_ptr->cmp_record(rec_data.data, rec_data.size));
        ASSERT_EQ(rec_ctx.sid, session2get); // Transport Session ID

        // Calculate expected Export Time
        unsigned int count = counter[rec_ctx.odid]++;
        uint32_t expect_etime = 1011 + ((count / 22) * 11); // After each 22 Data Records + 11
        ASSERT_EQ(rec_ctx.exp_time, expect_etime);
    }

    // Check total number of read Data Records (expect 100000 for each ODID)
    ASSERT_EQ(counter.size(), 3U); // Number of counter should match number of ODIDs
    for (const auto count : counter) {
        SCOPED_TRACE("ODID:" + std::to_string(count.first));
        EXPECT_EQ(count.second, cnt / 3U);
    }

    // No more Data Records
    EXPECT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);
}

/*
 * Write a lot of Data Records (based on different Templates) from multiple Transport Sessions
 * with the same ODID.
 *
 * The goal is to make sure that Templates with the same ID are unique for a combination of
 * a Transport Session and ODID.
 */
TEST_P(FileAPI, recordsFromDifferentSourcesAndSameODID)
{
    uint32_t odid = 5;
    uint32_t exp_time = 0;
    fds_file_sid_t s1_id, s2_id, s3_id;

    // Create Transport Session descriptions
    Session s1_def {"192.168.10.12", "245.255.0.1", 10, 9999, FDS_FILE_SESSION_SCTP};
    Session s2_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_UDP};
    Session s3_def {"192.168.10.12", "245.255.0.1", 10, 9999, FDS_FILE_SESSION_TCP};
    const auto all_defs = {std::ref(s1_def), std::ref(s2_def), std::ref(s3_def)};

    // Prepare Data Records and Template IDs
    std::vector<uint16_t> s1_tmplt = {256, 1000, 12345};  // Template IDs used by TS S1
    std::vector<uint16_t> s2_tmplt = {12345, 1000, 7897};
    std::vector<uint16_t> s3_tmplt = {256, 65530, 45712};

    std::vector<std::unique_ptr<DRec_base>> s1_recs; // Data Records used by TS S1
    s1_recs.emplace_back(new DRec_biflow(s1_tmplt[0]));
    s1_recs.emplace_back(new DRec_simple(s1_tmplt[1]));
    s1_recs.emplace_back(new DRec_opts(s1_tmplt[2]));

    std::vector<std::unique_ptr<DRec_base>> s2_recs;
    s2_recs.emplace_back(new DRec_biflow(s2_tmplt[0], "session2_rec1"));
    s2_recs.emplace_back(new DRec_biflow(s2_tmplt[1], "session2_rec2"));
    s2_recs.emplace_back(new DRec_simple(s2_tmplt[2], 1234, 457, 6, 10001, 78));

    std::vector<std::unique_ptr<DRec_base>> s3_recs;
    s3_recs.emplace_back(new DRec_simple(s3_tmplt[0], 12345, 12, 6));
    s3_recs.emplace_back(new DRec_simple(s3_tmplt[1], 8797, 16547, 17));
    s3_recs.emplace_back(new DRec_simple(s3_tmplt[2], 11, 24, 14, 7894, 124));

    // Open a file for writing
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);

    // Add Transport Session S1, its Templates and few Data Records (10000 per Data Record type)
    ASSERT_EQ(fds_file_session_add(file.get(), s1_def.get(), &s1_id), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), s1_id, odid, exp_time), FDS_OK);
    for (const auto &rec: s1_recs) {
        ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec->tmplt_type(), rec->tmplt_data(),
            rec->tmplt_size()), FDS_OK);
    }
    constexpr size_t cnt_base = 30000;
    for (size_t i = 0; i < cnt_base; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));

        size_t idx = i % 3;
        const DRec_base *rec_data = s1_recs[idx].get();
        const uint16_t rec_tid = s1_tmplt[idx];
        ASSERT_EQ(fds_file_write_rec(file.get(), rec_tid, rec_data->rec_data(), rec_data->rec_size()), FDS_OK);
    }

    // Add Transport Session S2, its Templates and few Data Records (10000 per Data Record type)
    ASSERT_EQ(fds_file_session_add(file.get(), s2_def.get(), &s2_id), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), s2_id, odid, exp_time), FDS_OK);
    for (const auto &rec: s2_recs) {
        ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec->tmplt_type(), rec->tmplt_data(),
            rec->tmplt_size()), FDS_OK);
    }
    for (size_t i = 0; i < cnt_base; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));

        size_t idx = i % 3;
        const DRec_base *rec_data = s2_recs[idx].get();
        const uint16_t rec_tid = s2_tmplt[idx];
        ASSERT_EQ(fds_file_write_rec(file.get(), rec_tid, rec_data->rec_data(), rec_data->rec_size()), FDS_OK);
    }

    // Add Transport Session S3, its Templates and few Data Records (10000 per Data Record type)
    ASSERT_EQ(fds_file_session_add(file.get(), s3_def.get(), &s3_id), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), s3_id, odid, exp_time), FDS_OK);
    for (const auto &rec: s3_recs) {
        ASSERT_EQ(fds_file_write_tmplt_add(file.get(), rec->tmplt_type(), rec->tmplt_data(),
            rec->tmplt_size()), FDS_OK);
    }
    for (size_t i = 0; i < cnt_base; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));

        size_t idx = i % 3;
        const DRec_base *rec_data = s3_recs[idx].get();
        const uint16_t rec_tid = s3_tmplt[idx];
        ASSERT_EQ(fds_file_write_rec(file.get(), rec_tid, rec_data->rec_data(), rec_data->rec_size()), FDS_OK);
    }

    if (m_load_iemgr) {
        // Define Information Elements slightly later and check if there are not any problems
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    // Add extra Data Records from the Transport Session (mixed order)
    const std::vector<std::pair<fds_file_sid_t, DRec_base *>>order = {
        std::make_pair(s1_id, s1_recs[0].get()), std::make_pair(s2_id, s2_recs[0].get()),
        std::make_pair(s3_id, s3_recs[0].get()), std::make_pair(s3_id, s3_recs[1].get()),
        std::make_pair(s2_id, s2_recs[1].get()), std::make_pair(s1_id, s1_recs[1].get()),
        std::make_pair(s2_id, s2_recs[2].get()), std::make_pair(s1_id, s1_recs[2].get()),
        std::make_pair(s3_id, s3_recs[2].get()),
    };
    ASSERT_EQ(order.size(), 9U);

    constexpr size_t cnt_extra = 90000; // 10000 per Data Record type (3 Sessions x 3 Templates)
    exp_time = 10;
    for (size_t i = 0; i < cnt_extra; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));

        const auto &pair = order[i % 9];
        DRec_base *rec = pair.second;
        // Set appropriate writer context and add the Data Record
        ASSERT_EQ(fds_file_write_ctx(file.get(), pair.first, odid, exp_time), FDS_OK);
        ASSERT_EQ(fds_file_write_rec(file.get(), rec->tmptl_id(), rec->rec_data(), rec->rec_size()), FDS_OK);
    }

    // Close the file
    file.reset();

    // Open the file for reading
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);

    // Check the Transport Session descriptions
    fds_file_sid_t *list_data;
    size_t list_size;
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 3U); // We have 3 different Transport Sessions

    // For each Transport Session in the file
    for (size_t i = 0; i < list_size; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));
        // Try to find the description and compare it with the previously written one
        const struct fds_file_session *session_desc;
        ASSERT_EQ(fds_file_session_get(file.get(), list_data[i], &session_desc), FDS_OK);

        auto session_cmp = [session_desc](const Session &s) -> bool {return s.cmp(session_desc);};
        auto match = std::find_if(all_defs.begin(), all_defs.end(), session_cmp);
        EXPECT_NE(match, all_defs.end()) << "Transport Session description not found!";

        // List all ODIDs of the Transport Session
        uint32_t *odid_data;
        size_t odid_size;
        ASSERT_EQ(fds_file_session_odids(file.get(), list_data[i], &odid_data, &odid_size), FDS_OK);
        ASSERT_EQ(odid_size, 1U); // Only one ODID was used
        EXPECT_EQ(odid_data[0], odid);
        free(odid_data);
    }

    free(list_data);

    /* Try to check all Data Records
     * Because Data Records from different Transport Sessions are not stored in the order,
     * we must detect Transport Session, check the Data Record and update the counter.
     */
    std::map<fds_file_sid_t, std::map<uint16_t, unsigned int>> counter; // SID -> Template ID -> cnt
    struct fds_drec rec_data;
    struct fds_file_read_ctx rec_ctx;

    for (size_t i = 0; i < (3 * cnt_base) + cnt_extra; ++i) {
        SCOPED_TRACE("i: " + std::to_string(i));

        // Clean the content of the record structures
        memset(&rec_data, 0, sizeof rec_data);
        memset(&rec_ctx, 0, sizeof rec_ctx);

        // Get the Data Record
        ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);

        // Try to find the expected Data Record content
        std::vector<uint16_t> *tmplts;
        std::vector<std::unique_ptr<DRec_base>> *recs;

        fds_file_sid_t session_id = rec_ctx.sid;
        if (session_id == s1_id) {
            tmplts = &s1_tmplt;
            recs = &s1_recs;
        } else if (session_id == s2_id) {
            tmplts = &s2_tmplt;
            recs = &s2_recs;
        } else if (session_id == s3_id) {
            tmplts = &s3_tmplt;
            recs = &s3_recs;
        } else {
            FAIL() << "Unknown Transport Session ID!";
        }

        // Based on the Template ID, try to find the Data Record
        const uint16_t tmplt_id = rec_data.tmplt->id;
        const auto tmplt_pos = std::find(tmplts->begin(), tmplts->end(), tmplt_id);
        ASSERT_NE(tmplt_pos, tmplts->end());
        size_t tmplt_idx = std::distance(tmplts->begin(), tmplt_pos);
        DRec_base *rec_ptr = (*recs)[tmplt_idx].get();

        // Compare the Data Records
        ASSERT_TRUE(rec_ptr->cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
        ASSERT_TRUE(rec_ptr->cmp_record(rec_data.data, rec_data.size));
        EXPECT_EQ(rec_ctx.odid, odid);

        /* Calculate expected Export Time
         * Before mixing insertion, cnt_base number of records has been added with Export Time 0
         * to this combination of a Transport Session and ODID. However, 3 types of Data Records
         * were inserted, therefore, we must divide it by 3. Later (i.e. mixing insertion) all Data
         * Records had Export Time == 10.
         */
        unsigned int count = counter[session_id][tmplt_id]++;
        uint32_t exp_time = (count < (cnt_base / 3)) ? 0 : 10;
        EXPECT_EQ(rec_ctx.exp_time, exp_time);
    }

    // Check counters
    const unsigned int exp_occur = (cnt_base / 3U) + (cnt_extra / 9U); // Expected occurrence per Data Record

    EXPECT_EQ(counter.size(), 3U); // 3 Transport Sessions expected
    for (const auto &map : counter) {
        EXPECT_EQ(map.second.size(), 3U); // 3 Templates per Transport Session expected
        for (auto count : map.second) {
            EXPECT_EQ(count.second, exp_occur); // Number of occurrences
        }
    }

    // No more Data Records
    EXPECT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);

    // Rewind and try to configure Transport Session filter to return only Data Record from S1
    ASSERT_EQ(fds_file_read_rewind(file.get()), FDS_OK);
    ASSERT_EQ(fds_file_read_sfilter(file.get(), &s1_id, nullptr), FDS_OK);

    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    unsigned int s1_cnt = 0;
    while (fds_file_read_rec(file.get(), &rec_data, &rec_ctx) == FDS_OK) {
        SCOPED_TRACE("s1_cnt: " + std::to_string(s1_cnt));
        s1_cnt++;
        ASSERT_EQ(rec_ctx.sid, s1_id);
        ASSERT_EQ(rec_ctx.odid, odid);
    }

    EXPECT_EQ(s1_cnt, 3U * exp_occur);

    // Rewind again and add S3 to the Transport Session filter
    ASSERT_EQ(fds_file_read_rewind(file.get()), FDS_OK);
    ASSERT_EQ(fds_file_read_sfilter(file.get(), &s3_id, nullptr), FDS_OK);

    unsigned int s13_cnt = 0;
    while (fds_file_read_rec(file.get(), &rec_data, &rec_ctx) == FDS_OK) {
        SCOPED_TRACE("s13_cnt: " + std::to_string(s13_cnt));
        s13_cnt++;
        ASSERT_TRUE(rec_ctx.sid == s1_id || rec_ctx.sid == s3_id);
        ASSERT_EQ(rec_ctx.odid, odid);
    }

    EXPECT_EQ(s13_cnt, 6U * exp_occur);
}

// Use the Transport Session and ODID filter to skip all flows
TEST_P(FileAPI, filterOutAllFlows)
{
    // Prepare various Transport Sessions and ODIDs
    uint32_t odid1 = 1;
    uint32_t odid2 = 8;
    uint32_t odid3 = 4;
    uint32_t odid_inv = 50;
    uint32_t exp_time = 10;

    // Create Transport Session descriptions
    Session s1_def {"192.168.10.12", "245.255.0.1", 10, 9999, FDS_FILE_SESSION_SCTP};
    Session s2_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_TCP};
    Session s3_def {"192.168.10.12", "245.255.0.1", 10, 9999, FDS_FILE_SESSION_TCP};
    fds_file_sid_t s1_id, s2_id, s3_id;

    // Prepare Data Records
    DRec_biflow s1_rec(256);
    DRec_opts s2_rec(300);
    DRec_simple s3_rec(256);

    // Open a file for writting and add all Transport Sessions
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }
    ASSERT_EQ(fds_file_session_add(file.get(), s1_def.get(), &s1_id), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), s2_def.get(), &s2_id), FDS_OK);
    ASSERT_EQ(fds_file_session_add(file.get(), s3_def.get(), &s3_id), FDS_OK);

    // Add the Data Records
    ASSERT_EQ(fds_file_write_ctx(file.get(), s1_id, odid1, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), s1_rec.tmplt_type(), s1_rec.tmplt_data(),
        s1_rec.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s1_rec.tmptl_id(), s1_rec.rec_data(),
        s1_rec.rec_size()), FDS_OK);

    ASSERT_EQ(fds_file_write_ctx(file.get(), s2_id, odid2, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), s2_rec.tmplt_type(), s2_rec.tmplt_data(),
        s2_rec.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s2_rec.tmptl_id(), s2_rec.rec_data(),
        s2_rec.rec_size()), FDS_OK);

    ASSERT_EQ(fds_file_write_ctx(file.get(), s3_id, odid3, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), s3_rec.tmplt_type(), s3_rec.tmplt_data(),
        s3_rec.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s3_rec.tmptl_id(), s3_rec.rec_data(),
        s3_rec.rec_size()), FDS_OK);

    // Close the file
    file.reset();

    // Open the file for reading
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);
    if (m_load_iemgr) {
        // Load Information Elements
        EXPECT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    }

    // Initialize the filter and expect no Data Records to read
    ASSERT_EQ(fds_file_read_sfilter(file.get(), nullptr, &odid_inv), FDS_OK);
    struct fds_file_read_ctx rec_ctx;
    struct fds_drec rec_data;
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);

    // Try to disable the filter and read all Data Records (expect automatic rewind)
    size_t cnt = 0;
    ASSERT_EQ(fds_file_read_sfilter(file.get(), nullptr, nullptr), FDS_OK);
    while (fds_file_read_rec(file.get(), &rec_data, &rec_ctx) == FDS_OK) {
        cnt++;
    }

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);
    EXPECT_EQ(cnt, 3U);

    // Try to read Data Records from nonexisting combination of Transport Session and ODID
    fds_file_sid_t *list_data;
    size_t list_size;
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 3U); // 3 Transport Sessions
    ASSERT_EQ(fds_file_read_sfilter(file.get(), &list_data[0], &odid_inv), FDS_OK);
    free(list_data);

    EXPECT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);
}

// Redefine IE Manager while writing Data Records
TEST_P(FileAPI, redefineIEManagerWhileWriting)
{
    if (!m_load_iemgr) {
        // Nothing to do
        return;
    }

    uint32_t exp_time = 1023;

    // Prepare a copy of the IE manager without octetDeltaCount
    std::unique_ptr<fds_iemgr_t, decltype(&fds_iemgr_destroy)> iemgr_mod(fds_iemgr_copy(m_iemgr),
        &fds_iemgr_destroy);
    ASSERT_NE(iemgr_mod, nullptr);
    EXPECT_NE(fds_iemgr_elem_find_name(iemgr_mod.get(), "iana:octetDeltaCount"), nullptr);
    ASSERT_EQ(fds_iemgr_elem_remove(iemgr_mod.get(), 0, 1), FDS_OK);
    EXPECT_EQ(fds_iemgr_elem_find_name(iemgr_mod.get(), "iana:octetDeltaCount"), nullptr);

    // Prepare few Transport Sessions and Data Records
    Session s1_def {"192.168.10.12", "245.255.0.1", 10, 9999, FDS_FILE_SESSION_SCTP};
    Session s2_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_TCP};
    fds_file_sid_t s1_id, s2_id;
    uint32_t s1_odid_a = 10;
    uint32_t s1_odid_b = 8;
    uint32_t s2_odid = 10;

    DRec_biflow s1_a_rec1(256); // Transport Session 1 - ODID A
    DRec_simple s1_a_rec2(257);
    DRec_simple s1_b_rec1(256); // Transport Session 1 - ODID B
    DRec_opts s1_b_rec2(257);
    DRec_opts s2_rec1(256);     // Transport Session 2
    DRec_biflow s2_rec2(257);

    // Open file for writing
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);

    // Add few Data Records based on the Transport Session 2
    ASSERT_EQ(fds_file_session_add(file.get(), s2_def.get(), &s2_id), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), s2_id, s2_odid, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), s2_rec1.tmplt_type(), s2_rec1.tmplt_data(),
        s2_rec1.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), s2_rec2.tmplt_type(), s2_rec2.tmplt_data(),
        s2_rec2.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s2_rec1.tmptl_id(), s2_rec1.rec_data(),
        s2_rec1.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s2_rec2.tmptl_id(), s2_rec2.rec_data(),
        s2_rec2.rec_size()), FDS_OK);

    // Add few Data Records based on the Transport Session 1 - ODID A
    ASSERT_EQ(fds_file_session_add(file.get(), s1_def.get(), &s1_id), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), s1_id, s1_odid_a, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), s1_a_rec1.tmplt_type(), s1_a_rec1.tmplt_data(),
        s1_a_rec1.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), s1_a_rec2.tmplt_type(), s1_a_rec2.tmplt_data(),
        s1_a_rec2.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s1_a_rec1.tmptl_id(), s1_a_rec1.rec_data(),
        s1_a_rec1.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s1_a_rec2.tmptl_id(), s1_a_rec2.rec_data(),
        s1_a_rec2.rec_size()), FDS_OK);

    // Configure an IE manager (use the original one for now)
    ASSERT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);

    // Try to write few more Data Records (the context must be still the same)
    ASSERT_EQ(fds_file_write_rec(file.get(), s1_a_rec1.tmptl_id(), s1_a_rec1.rec_data(),
        s1_a_rec1.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s1_a_rec2.tmptl_id(), s1_a_rec2.rec_data(),
        s1_a_rec2.rec_size()), FDS_OK);

    // Change the context (Transport Session - ODID B) and add few Data Records
    ASSERT_EQ(fds_file_write_ctx(file.get(), s1_id, s1_odid_b, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), s1_b_rec1.tmplt_type(), s1_b_rec1.tmplt_data(),
        s1_b_rec1.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), s1_b_rec2.tmplt_type(), s1_b_rec2.tmplt_data(),
        s1_b_rec2.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s1_b_rec1.tmptl_id(), s1_b_rec1.rec_data(),
        s1_b_rec1.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s1_b_rec2.tmptl_id(), s1_b_rec2.rec_data(),
        s1_b_rec2.rec_size()), FDS_OK);

    // Change the IE manager to the modified one
    ASSERT_EQ(fds_file_set_iemgr(file.get(), iemgr_mod.get()), FDS_OK);

    // Try to add few more Data Records (no context change!)
    ASSERT_EQ(fds_file_write_rec(file.get(), s1_b_rec1.tmptl_id(), s1_b_rec1.rec_data(),
        s1_b_rec1.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s1_b_rec2.tmptl_id(), s1_b_rec2.rec_data(),
        s1_b_rec2.rec_size()), FDS_OK);

    // Remove the IE manager and free it
    ASSERT_EQ(fds_file_set_iemgr(file.get(), nullptr), FDS_OK);
    iemgr_mod.reset();

    // Try to add few more Data Records (no context change!)
    ASSERT_EQ(fds_file_write_rec(file.get(), s1_b_rec1.tmptl_id(), s1_b_rec1.rec_data(),
        s1_b_rec1.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s1_b_rec2.tmptl_id(), s1_b_rec2.rec_data(),
        s1_b_rec2.rec_size()), FDS_OK);

    // Change context and try to add more Data Records to the Transport Session 2
    ASSERT_EQ(fds_file_session_add(file.get(), s2_def.get(), &s2_id), FDS_OK); // no action expected
    ASSERT_EQ(fds_file_write_ctx(file.get(), s2_id, s2_odid, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s2_rec1.tmptl_id(), s2_rec1.rec_data(),
        s2_rec1.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s2_rec2.tmptl_id(), s2_rec2.rec_data(),
        s2_rec2.rec_size()), FDS_OK);

    // Try to reopen the file for reading and check all Data Records -------------------------------
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);

    // First of all, get the list of all Transport Sessions
    fds_file_sid_t *list_data;
    size_t list_size;
    ASSERT_EQ(fds_file_session_list(file.get(), &list_data, &list_size), FDS_OK);
    ASSERT_EQ(list_size, 2U); // Only 2 Transport Sessions

    // Determine which one is Transport Session 1 and which one is Transport Session 2
    for (size_t i = 0; i < list_size; ++i) {
        // Get the list of ODIDs
        uint32_t *odid_data;
        size_t odid_size;

        ASSERT_EQ(fds_file_session_odids(file.get(), list_data[i], &odid_data, &odid_size), FDS_OK);
        switch (odid_size) {
        case 1U:
            s2_id = list_data[i]; // Transport Session 2 has only 1 ODID
            break;
        case 2U:
            s1_id = list_data[i]; // Transport Session 1 has only 2 ODIDs
            break;
        default:
            FAIL() << "Unexpected Transport Session";
            break;
        }

        free(odid_data);
    }
    free(list_data);

    struct fds_file_read_ctx rec_ctx;
    struct fds_drec rec_data;

    // Use Transport Session and ODID filter to check records from Transport Session 1 - ODID A
    ASSERT_EQ(fds_file_read_sfilter(file.get(), &s1_id, &s1_odid_a), FDS_OK);

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(s1_a_rec1.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(s1_a_rec1.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, s1_odid_a);
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(s1_a_rec2.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(s1_a_rec2.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(s1_a_rec1.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(s1_a_rec1.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(s1_a_rec2.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(s1_a_rec2.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);

    // Use Transport Session and ODID filter to check records from Transport Session 1 - ODID B
    ASSERT_EQ(fds_file_read_sfilter(file.get(), nullptr, nullptr), FDS_OK);
    ASSERT_EQ(fds_file_read_sfilter(file.get(), &s1_id, &s1_odid_b), FDS_OK);

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(s1_b_rec1.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(s1_b_rec1.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, s1_odid_b);
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(s1_b_rec2.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(s1_b_rec2.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(s1_b_rec1.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(s1_b_rec1.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(s1_b_rec2.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(s1_b_rec2.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(s1_b_rec1.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(s1_b_rec1.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(s1_b_rec2.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(s1_b_rec2.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);

    // Use Transport Session and ODID filter to check records from Transport Session 2
    ASSERT_EQ(fds_file_read_sfilter(file.get(), nullptr, nullptr), FDS_OK);
    ASSERT_EQ(fds_file_read_sfilter(file.get(), &s2_id, &s2_odid), FDS_OK);

    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(s2_rec1.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(s2_rec1.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(rec_ctx.odid, s2_odid);
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(s2_rec2.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(s2_rec2.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(s2_rec1.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(s2_rec1.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
    ASSERT_TRUE(s2_rec2.cmp_template(rec_data.tmplt->raw.data, rec_data.tmplt->raw.length));
    ASSERT_TRUE(s2_rec2.cmp_record(rec_data.data, rec_data.size));
    ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);
}


// Redefine IE Manager while reading Data Records
TEST_P(FileAPI, redefineIEManagerWhileReading)
{
    if (!m_load_iemgr) {
        // Nothing to do
        return;
    }

    uint32_t exp_time = 1023;

    // Prepare a copy of the IE manager without octetDeltaCount
    std::unique_ptr<fds_iemgr_t, decltype(&fds_iemgr_destroy)> iemgr_mod(fds_iemgr_copy(m_iemgr),
        &fds_iemgr_destroy);
    ASSERT_NE(iemgr_mod, nullptr);
    EXPECT_NE(fds_iemgr_elem_find_name(iemgr_mod.get(), "iana:octetDeltaCount"), nullptr);
    ASSERT_EQ(fds_iemgr_elem_remove(iemgr_mod.get(), 0, 1), FDS_OK);
    EXPECT_EQ(fds_iemgr_elem_find_name(iemgr_mod.get(), "iana:octetDeltaCount"), nullptr);

    // Prepare few Transport Sessions and Data Records
    Session s1_def {"192.168.10.12", "245.255.0.1", 10, 9999, FDS_FILE_SESSION_SCTP};
    Session s2_def {"10.0.10.12", "127.0.0.1", 879, 11324, FDS_FILE_SESSION_TCP};
    fds_file_sid_t s1_id, s2_id;
    uint32_t s1_odid_a = 10;
    uint32_t s1_odid_b = 8;
    uint32_t s2_odid = 10;

    DRec_biflow s1_a_rec1(256); // Transport Session 1 - ODID A
    DRec_simple s1_a_rec2(257);
    DRec_simple s1_b_rec1(256); // Transport Session 1 - ODID B
    DRec_opts s1_b_rec2(257);
    DRec_opts s2_rec1(256);     // Transport Session 2
    DRec_biflow s2_rec2(257);

    // Open file for writing and add few Data Records from different Transport Sessions
    std::unique_ptr<fds_file_t, decltype(&fds_file_close)> file(fds_file_init(), &fds_file_close);
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_write), FDS_OK);
    // Data Records based on the Transport Session 1 - ODID A
    ASSERT_EQ(fds_file_session_add(file.get(), s1_def.get(), &s1_id), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), s1_id, s1_odid_a, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), s1_a_rec1.tmplt_type(), s1_a_rec1.tmplt_data(),
        s1_a_rec1.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), s1_a_rec2.tmplt_type(), s1_a_rec2.tmplt_data(),
        s1_a_rec2.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s1_a_rec1.tmptl_id(), s1_a_rec1.rec_data(),
        s1_a_rec1.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s1_a_rec2.tmptl_id(), s1_a_rec2.rec_data(),
        s1_a_rec2.rec_size()), FDS_OK);
    // Data Records based on the Transport Session 1 - ODID B
    ASSERT_EQ(fds_file_write_ctx(file.get(), s1_id, s1_odid_b, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), s1_b_rec1.tmplt_type(), s1_b_rec1.tmplt_data(),
        s1_b_rec1.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), s1_b_rec2.tmplt_type(), s1_b_rec2.tmplt_data(),
        s1_b_rec2.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s1_b_rec1.tmptl_id(), s1_b_rec1.rec_data(),
        s1_b_rec1.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s1_b_rec2.tmptl_id(), s1_b_rec2.rec_data(),
        s1_b_rec2.rec_size()), FDS_OK);
    // Data Records based on the Transport Session 2
    ASSERT_EQ(fds_file_session_add(file.get(), s2_def.get(), &s2_id), FDS_OK);
    ASSERT_EQ(fds_file_write_ctx(file.get(), s2_id, s2_odid, exp_time), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), s2_rec1.tmplt_type(), s2_rec1.tmplt_data(),
        s2_rec1.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_tmplt_add(file.get(), s2_rec2.tmplt_type(), s2_rec2.tmplt_data(),
        s2_rec2.tmplt_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s2_rec1.tmptl_id(), s2_rec1.rec_data(),
        s2_rec1.rec_size()), FDS_OK);
    ASSERT_EQ(fds_file_write_rec(file.get(), s2_rec2.tmptl_id(), s2_rec2.rec_data(),
        s2_rec2.rec_size()), FDS_OK);

    // Open the file in the reader mode ------------------------------------------------------------
    file.reset(fds_file_init());
    ASSERT_EQ(fds_file_open(file.get(), m_filename.c_str(), m_flags_read), FDS_OK);

    // First, of all try to read few Data Records without definition of IE manager
    struct fds_file_read_ctx rec_ctx;
    struct fds_drec rec_data;
    struct fds_drec_field field;

    size_t cnt = 6U;
    for (size_t i = 0; i < cnt; ++i) {
        ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
        if (rec_data.tmplt->type != FDS_TYPE_TEMPLATE) {
            // Skip IPFIX Options Templates
            continue;
        }

        // Try to find a definition of octetDeltaCount
        ASSERT_NE(fds_drec_find(&rec_data, 0, 1, &field), FDS_EOC); // octetDeltaCount
        EXPECT_EQ(field.info->def, nullptr); // definition NOT available
    }
    EXPECT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);

    // Try to use the default IE manager (reader must be automatically rewind)
    ASSERT_EQ(fds_file_set_iemgr(file.get(), m_iemgr), FDS_OK);
    for (size_t i = 0; i < cnt; ++i) {
        ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
        if (rec_data.tmplt->type != FDS_TYPE_TEMPLATE) {
            // Skip IPFIX Options Templates
            continue;
        }

        // Try to find a definition of octetDeltaCount
        ASSERT_NE(fds_drec_find(&rec_data, 0, 1, &field), FDS_EOC); // octetDeltaCount
        ASSERT_NE(field.info->def, nullptr); // definition available
        EXPECT_NE(field.info->def->name, nullptr);
        EXPECT_EQ(field.info->def->data_type, FDS_ET_UNSIGNED_64);
        EXPECT_EQ(field.info->def->data_unit, FDS_EU_OCTETS);
    }
    EXPECT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);

    // Try to use the modified IE manager (reader must be automatically rewind)
    ASSERT_EQ(fds_file_set_iemgr(file.get(), iemgr_mod.get()), FDS_OK);
    for (size_t i = 0; i < cnt; ++i) {
        ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
        if (rec_data.tmplt->type != FDS_TYPE_TEMPLATE) {
            // Skip IPFIX Options Templates
            continue;
        }

        // Try to find a definition of octetDeltaCount
        ASSERT_NE(fds_drec_find(&rec_data, 0, 1, &field), FDS_EOC); // octetDeltaCount
        EXPECT_EQ(field.info->def, nullptr); // definition NOT available

        // Try to find a definition of packetDeltaCount
        ASSERT_NE(fds_drec_find(&rec_data, 0, 2, &field), FDS_EOC); // packetDeltaCount
        ASSERT_NE(field.info->def, nullptr); // definition available
        EXPECT_NE(field.info->def->name, nullptr);
        EXPECT_EQ(field.info->def->data_type, FDS_ET_UNSIGNED_64);
        EXPECT_EQ(field.info->def->data_unit, FDS_EU_PACKETS);
    }
    EXPECT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);

    // Try to remove the IE manager and destroy it
    ASSERT_EQ(fds_file_set_iemgr(file.get(), nullptr), FDS_OK);
    iemgr_mod.reset();

    for (size_t i = 0; i < cnt; ++i) {
        ASSERT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_OK);
        if (rec_data.tmplt->type != FDS_TYPE_TEMPLATE) {
            // Skip IPFIX Options Templates
            continue;
        }

        // Try to find a definition of octetDeltaCount
        ASSERT_NE(fds_drec_find(&rec_data, 0, 1, &field), FDS_EOC); // octetDeltaCount
        EXPECT_EQ(field.info->def, nullptr); // definition NOT available
    }
    EXPECT_EQ(fds_file_read_rec(file.get(), &rec_data, &rec_ctx), FDS_EOC);
}


