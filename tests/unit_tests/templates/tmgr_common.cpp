/**
 * \brief Test cases for all types of sessions
 */

#include <gtest/gtest.h>
#include <libfds.h>
#include <TGenerator.h>
#include <TMock.h>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// Create main class for parameterized test
class Common : public ::testing::TestWithParam<enum fds_session_type> {
protected:
    fds_tmgr_t *tmgr = nullptr;
    /** \brief Prepare a template manager*/
    void SetUp() override {
        tmgr = fds_tmgr_create(GetParam());
        if (!tmgr) {
            throw std::runtime_error("Failed to create a template manager!");
        }
    }

    /** \brief Destroy a template manager */
    void TearDown() override {
        if (tmgr == nullptr) {
            return;
        }
        fds_tmgr_destroy(tmgr);
    }
};

// Define parameters of parametrized test
INSTANTIATE_TEST_CASE_P(TemplateManager, Common,
    ::testing::Values(FDS_SESSION_TYPE_UDP, FDS_SESSION_TYPE_TCP, FDS_SESSION_TYPE_SCTP,
        FDS_SESSION_TYPE_IPFIX_FILE));

// Try to create and immediately destroy the manager
TEST_P(Common, createAndDestroy)
{
    ASSERT_NE(tmgr, nullptr);
}

// Try to clear an empty manager
TEST_P(Common, clearEmpty)
{
    fds_tmgr_clear(tmgr);

    // Try to get garbage
    fds_tgarbage_t *garbage = nullptr;
    EXPECT_EQ(fds_tmgr_garbage_get(tmgr, &garbage), FDS_OK);
    EXPECT_EQ(garbage, nullptr);
}

// Try to get snapshot from an empty manager
TEST_P(Common, getSnapshotEmpty)
{
    // Time context is not set
    const fds_tsnapshot_t *snap = nullptr;
    EXPECT_EQ(fds_tmgr_snapshot_get(tmgr, &snap), FDS_ERR_ARG);
    EXPECT_EQ(snap, nullptr);

    // Set context and try again
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 10000), FDS_OK);
    ASSERT_EQ(fds_tmgr_snapshot_get(tmgr, &snap), FDS_OK);
    ASSERT_NE(snap, nullptr);

    // Try to find a non-existing template in the snapshot
    EXPECT_EQ(fds_tsnapshot_template_get(snap, 54541), nullptr);
}

// Try to get a garbage from an empty manager
TEST_P(Common, getGarbageEmpty)
{
    // Time context is not set
    fds_tgarbage_t *garbage = nullptr;
    EXPECT_EQ(fds_tmgr_garbage_get(tmgr, &garbage), FDS_OK);
    EXPECT_EQ(garbage, nullptr);

    // Set time context
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 123456789), FDS_OK);
    EXPECT_EQ(fds_tmgr_garbage_get(tmgr, &garbage), FDS_OK);
    EXPECT_EQ(garbage, nullptr);
}

// Try to get a non-existing template in an empty manager
TEST_P(Common, getTemplateEmpty)
{
    // Time context is not set
    const struct fds_template *tmplt = nullptr;
    EXPECT_EQ(fds_tmgr_template_get(tmgr, 12345, &tmplt), FDS_ERR_ARG);
    EXPECT_EQ(tmplt, nullptr);

    // Set time context
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 272642144), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, 12345, &tmplt), FDS_ERR_NOTFOUND);
    EXPECT_EQ(tmplt, nullptr);
}

// Try to add and find a simple template
TEST_P(Common, addAndFind)
{
    struct fds_template *t256 = TMock::create(TMock::type::DATA_BASIC_FLOW, 256);
    // Set current time and add the template
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 10), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t256), FDS_OK);

    // Get the template
    const struct fds_template *result;
    ASSERT_EQ(fds_tmgr_template_get(tmgr, 256, &result), FDS_OK);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->id, 256);
}

// Add templates in whole range
TEST_P(Common, maxTemplates)
{
    // Set time context and add templates
    ASSERT_EQ(fds_tmgr_set_time(tmgr, 0), FDS_OK);
    const int step = 17;

    struct fds_template *aux_tmplt = nullptr;
    for (unsigned int i = 256; i <= UINT16_MAX; i += step) {
        TMock::type type = (i % 2)
            ? TMock::type::DATA_BASIC_FLOW
            : TMock::type::DATA_BASIC_BIFLOW;
        aux_tmplt = TMock::create(type, i);
        ASSERT_NE(aux_tmplt, nullptr);
        EXPECT_EQ(fds_tmgr_template_add(tmgr, aux_tmplt), FDS_OK);
    }

    const struct fds_template *const_ref = nullptr;
    for (unsigned int i = 256; i <= UINT16_MAX; i += step) {
        ASSERT_EQ(fds_tmgr_template_get(tmgr, i, &const_ref), FDS_OK);
        ASSERT_NE(const_ref, nullptr);
        EXPECT_EQ(const_ref->id, i);
        if (i % 2) {
            EXPECT_EQ(const_ref->fields_cnt_total, 10);
        } else {
            EXPECT_EQ(const_ref->fields_cnt_total, 15);
        }
    }
}

// Try to add withdrawal templates (not permitted)
TEST_P(Common, refuseWithdrawalTemplate)
{
    struct fds_template *data_with = TMock::create(TMock::type::DATA_WITHDRAWAL, 12345);
    struct fds_template *opts_with = TMock::create(TMock::type::OPTS_WITHDRAWAL, 25647);

    // Try to add withdrawal templates
    EXPECT_EQ(fds_tmgr_template_add(tmgr, data_with), FDS_ERR_ARG);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, opts_with), FDS_ERR_ARG);

    // Set time context
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 500), FDS_OK);

    // Try again
    EXPECT_EQ(fds_tmgr_template_add(tmgr, data_with), FDS_ERR_ARG);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, opts_with), FDS_ERR_ARG);

    fds_template_destroy(data_with);
    fds_template_destroy(opts_with);
}

// Add the same template every 60 seconds (refresh)
TEST_P(Common, templateRefresh)
{
    const uint16_t tid = 50000;
    struct fds_template *aux_tmplt = TMock::create(TMock::type::OPTS_MPROC_STAT, tid);
    ASSERT_NE(aux_tmplt, nullptr);

    const uint32_t time_start = 302515242UL;

    // Try to refresh the template 10 times
    for (unsigned int i = 0; i < 10; ++i) {
        const uint32_t time_now = time_start + (i * 60);
        // Set export time and add a template
        EXPECT_EQ(fds_tmgr_set_time(tmgr, time_now), FDS_OK);
        struct fds_template *tmplt2add = fds_template_copy(aux_tmplt);
        ASSERT_NE(tmplt2add, nullptr);
        EXPECT_EQ(fds_tmgr_template_add(tmgr, tmplt2add), FDS_OK);

        // Check the template
        const struct fds_template *tmplt2check = nullptr;
        ASSERT_EQ(fds_tmgr_template_get(tmgr, tid, &tmplt2check), FDS_OK);
        ASSERT_NE(tmplt2check, nullptr);

        // Start time should be still the same. Last time should be modified.
        EXPECT_EQ(tmplt2check->time.first_seen, time_start);
        EXPECT_EQ(tmplt2check->time.last_seen, time_now);
    }

    fds_template_destroy(aux_tmplt);
}

// Add a template, get a snapshot, create a garbage and remove the manager
TEST_P(Common, templateInGarbage)
{
    const uint16_t tid1 = 1000;
    const uint16_t tid2 = 5000;
    const uint16_t tid3 = 48712;

    // Set export time and add templates
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 1000), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_ERPOC_RSTAT, tid1)), FDS_OK);
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 1000), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_MPROC_STAT, tid2)), FDS_OK);
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 1001), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_MPROC_RSTAT, tid3)), FDS_OK);

    // Set different export time in the future and get the snapshot
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 2000), FDS_OK);
    const struct fds_tsnapshot *snap = nullptr;
    ASSERT_EQ(fds_tmgr_snapshot_get(tmgr, &snap), FDS_OK);
    ASSERT_NE(snap, nullptr);

    // Move everything (all snapshots and templates) to the garbage
    fds_tmgr_clear(tmgr);
    fds_tgarbage_t *tgarbage = nullptr;
    ASSERT_EQ(fds_tmgr_garbage_get(tmgr, &tgarbage), FDS_OK);
    ASSERT_NE(tgarbage, nullptr);

    // Destroy the template manager
    fds_tmgr_destroy(tmgr);
    tmgr = nullptr;

    // The snapshot and templates must be still available
    const struct fds_template *tmplt2check;
    tmplt2check = fds_tsnapshot_template_get(snap, tid1);
    ASSERT_NE(tmplt2check, nullptr);
    EXPECT_EQ(tmplt2check->id, tid1);

    tmplt2check = fds_tsnapshot_template_get(snap, tid2);
    ASSERT_NE(tmplt2check, nullptr);
    EXPECT_EQ(tmplt2check->id, tid2);

    tmplt2check = fds_tsnapshot_template_get(snap, tid3);
    ASSERT_NE(tmplt2check, nullptr);
    EXPECT_EQ(tmplt2check->id, tid3);

    // Destroy the garbage
    fds_tmgr_garbage_destroy(tgarbage);
}


// TODO: template remove (snapshot should be untouched...)
// TODO: Time wraparound...
// TODO: add already added template without creating snapshot

