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

// Try to set a flow key without defined time contest
TEST_P(Common, setFkeyEmpty)
{
    const uint16_t tid = 10000;
    const uint64_t fkey = 111;

    // Time context is not set
    EXPECT_EQ(fds_tmgr_template_set_fkey(tmgr, tid, fkey), FDS_ERR_ARG);

    // Set time context
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 10000), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_set_fkey(tmgr, tid, fkey), FDS_ERR_NOTFOUND);
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

TEST_P(Common, templateRemove)
{
    // Create a templates
    const uint32_t time1 = 5000;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time1), FDS_OK);

    const uint16_t tid1 = 52000;
    const uint16_t tid2 = 429;
    const uint16_t tid3 = 5000;
    const uint16_t tid4 = 700;

    struct fds_template *t1 = TMock::create(TMock::type::DATA_BASIC_FLOW, tid1);
    struct fds_template *t2 = TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid2);
    struct fds_template *t3 = TMock::create(TMock::type::OPTS_FKEY, tid3);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t1), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t2), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t3), FDS_OK);

    // Set new export time and create a snapshot
    const uint32_t time2 = time1 + 100;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time2), FDS_OK);
    const struct fds_tsnapshot *snap1 = nullptr;
    EXPECT_EQ(fds_tmgr_snapshot_get(tmgr, &snap1), FDS_OK);  // All templates available

    // Replace the template T2
    if (GetParam() != FDS_SESSION_TYPE_UDP) {
        EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid2, FDS_TYPE_TEMPLATE), FDS_OK);
    }
    struct fds_template *t2_new = TMock::create(TMock::type::OPTS_MPROC_RSTAT, tid2);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t2_new), FDS_OK);

    // Remove the template T2 and T3 from whole history
    EXPECT_EQ(fds_tmgr_template_remove(tmgr, tid2, FDS_TYPE_TEMPLATE_UNDEF), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_remove(tmgr, tid3, FDS_TYPE_TEMPLATE_OPTS), FDS_OK);
    // Remove a non-existing template T4
    EXPECT_EQ(fds_tmgr_template_remove(tmgr, tid4, FDS_TYPE_TEMPLATE_OPTS), FDS_OK);

    // Check availability of templates
    const fds_template *tmplt2check = nullptr;
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid1);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_ERR_NOTFOUND);

    // Snapshot should be unaffected
    ASSERT_NE(tmplt2check = fds_tsnapshot_template_get(snap1, tid1), nullptr);
    EXPECT_EQ(tmplt2check->id, tid1);
    ASSERT_NE(tmplt2check = fds_tsnapshot_template_get(snap1, tid2), nullptr);
    EXPECT_EQ(tmplt2check->id, tid2);
    EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE);
    ASSERT_NE(tmplt2check = fds_tsnapshot_template_get(snap1, tid3), nullptr);
    EXPECT_EQ(tmplt2check->id, tid3);
}


// Define a template and redefine it immediately without creating snapshot
TEST_P(Common, templateReplaceImmediately)
{
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 0), FDS_OK);
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 4000), FDS_OK);

    // Add templates
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 1000000), FDS_OK);
    const uint16_t tid1 = 45212;
    const uint16_t tid2 = 7382;
    struct fds_template *t1 = TMock::create(TMock::type::DATA_BASIC_FLOW, tid1);
    struct fds_template *t2 = TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid2);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t1), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t2), FDS_OK);

    // Replace T1 (withdraw first)
    if (GetParam() != FDS_SESSION_TYPE_UDP) {
        EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid1, FDS_TYPE_TEMPLATE_UNDEF), FDS_OK);
    }
    struct fds_template *t1_new = TMock::create(TMock::type::OPTS_MPROC_RSTAT, tid1);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t1_new), FDS_OK);

    // Check the template
    const struct fds_template *tmplt = nullptr;
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt), FDS_OK);
    EXPECT_EQ(tmplt->id, tid1);
    EXPECT_EQ(tmplt->type, FDS_TYPE_TEMPLATE_OPTS);
}

// Define a flow key and make sure that it remain after template refresh
TEST_P(Common, setFlowKey)
{
    // Create and add templates
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 0), FDS_OK);
    const uint16_t tid1 = 1234;
    const uint16_t tid2 = 2000;
    struct fds_template *t1 = TMock::create(TMock::type::DATA_BASIC_FLOW, tid1);
    struct fds_template *t2 = TMock::create(TMock::type::OPTS_ERPOC_RSTAT, tid2);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t1), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t2), FDS_OK);

    // Create a snapshot
    const fds_tsnapshot_t *snap_without;
    EXPECT_EQ(fds_tmgr_snapshot_get(tmgr, &snap_without), FDS_OK);

    // Define flow key
    const uint64_t fkey = 31U;
    const struct fds_template *tmplt2check;
    EXPECT_EQ(fds_tmgr_template_set_fkey(tmgr, tid1, fkey), FDS_OK);
    // Make sure that the template has the key
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_TRUE((tmplt2check->flags & FDS_TEMPLATE_HAS_FKEY) != 0);
    EXPECT_EQ(fds_template_flowkey_cmp(tmplt2check, fkey), 0);

    // Options templates can have flow key (really? not sure...)
    EXPECT_EQ(fds_tmgr_template_set_fkey(tmgr, tid2, fkey), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    EXPECT_NE(tmplt2check->flags & FDS_TEMPLATE_HAS_FKEY, 0);

    // Set new export time and create another snapshot
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 10), FDS_OK);
    const fds_tsnapshot_t *snap_with;
    EXPECT_EQ(fds_tmgr_snapshot_get(tmgr, &snap_with), FDS_OK);

    // Refresh the template and make sure that flow key is also set
    struct fds_template *t1_refresh = TMock::create(TMock::type::DATA_BASIC_FLOW, tid1);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t1_refresh), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid1);
    EXPECT_TRUE((tmplt2check->flags & FDS_TEMPLATE_HAS_FKEY) != 0);
    EXPECT_EQ(fds_template_flowkey_cmp(tmplt2check, fkey), 0);
    EXPECT_EQ(tmplt2check->time.first_seen, 0);
    EXPECT_EQ(tmplt2check->time.last_seen, 10);

    // Remove the flow key
    EXPECT_EQ(fds_tmgr_template_set_fkey(tmgr, tid1, 0), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_TRUE((tmplt2check->flags & FDS_TEMPLATE_HAS_FKEY) == 0);

    // Try to define flow key of non-existing template
    const uint16_t tid3 = 55555;
    EXPECT_EQ(fds_tmgr_template_set_fkey(tmgr, tid3, fkey), FDS_ERR_NOTFOUND);

    // Snapshot should be untouched
    ASSERT_NE(tmplt2check = fds_tsnapshot_template_get(snap_with, tid1), nullptr);
    EXPECT_TRUE((tmplt2check->flags & FDS_TEMPLATE_HAS_FKEY) != 0);

    ASSERT_NE(tmplt2check = fds_tsnapshot_template_get(snap_without, tid1), nullptr);
    EXPECT_TRUE((tmplt2check->flags & FDS_TEMPLATE_HAS_FKEY) == 0);
}

// Try to define invalid flow key
TEST_P(Common, invalidFlowKey)
{
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 0), FDS_OK);
    const uint16_t tid1 = 10000;
    struct fds_template *t1 = TMock::create(TMock::type::DATA_BASIC_FLOW, tid1);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t1), FDS_OK);

    // Try to set invalid flow key
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 10), FDS_OK);
    const uint64_t fkey = 49184U;
    EXPECT_EQ(fds_tmgr_template_set_fkey(tmgr, tid1, fkey), FDS_ERR_ARG);

    // Check the template
    const struct fds_template *tmplt;
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt), FDS_OK);
    EXPECT_EQ(tmplt->id, tid1);
    EXPECT_EQ(tmplt->flags & FDS_TEMPLATE_HAS_FKEY, 0);
}

// Make sure that a flow key is not inherited if a template is redefined
TEST_P(Common, doNotInheritFlowKey)
{
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 12345678), FDS_OK);
    const uint16_t tid1 = 12345;
    const uint64_t fkey = 31;

    // Create a template and define flow key
    struct fds_template *tmplt = TMock::create(TMock::type::DATA_BASIC_FLOW, tid1);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, tmplt), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_set_fkey(tmgr, tid1, fkey), FDS_OK);

    // Redefine the template
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 12345680), FDS_OK);
    if (GetParam() != FDS_SESSION_TYPE_UDP && GetParam() != FDS_SESSION_TYPE_IPFIX_FILE) {
        EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid1, FDS_TYPE_TEMPLATE_UNDEF), FDS_OK);
    }
    tmplt = TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid1);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, tmplt), FDS_OK);

    // Check the template
    const fds_template *tmplt2check;
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid1);
    EXPECT_EQ(tmplt2check->flags & FDS_TEMPLATE_HAS_FKEY, 0);
    EXPECT_EQ(fds_template_flowkey_cmp(tmplt2check, 0), 0);
}

// Simple time wraparound test
TEST_P(Common, timeWraparound)
{
    // Set export time and add few templates
    const uint32_t time1 = UINT32_MAX - 10;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time1), FDS_OK);

    const uint16_t tid1 = 1234;
    const uint16_t tid2 = 1235;
    const uint16_t tid3 = 12222;
    struct fds_template *t1 = TMock::create(TMock::type::DATA_BASIC_FLOW, tid1);
    struct fds_template *t2 = TMock::create(TMock::type::OPTS_MPROC_STAT, tid2);
    struct fds_template *t3 = TMock::create(TMock::type::OPTS_FKEY, tid3);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t1), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t2), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t3), FDS_OK);

    // Set new export time
    const uint32_t time2 = 10;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time2), FDS_OK);
    // Create a snapshot and check that all templates are accessible
    const fds_tsnapshot_t *snap = nullptr;
    EXPECT_EQ(fds_tmgr_snapshot_get(tmgr, &snap), FDS_OK);

    const struct fds_template *tmplt2check = nullptr;
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid1);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid2);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid3);

    // Try to redefine template T1
    if (GetParam() == FDS_SESSION_TYPE_UDP) {
        EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid1, FDS_TYPE_TEMPLATE_UNDEF), FDS_ERR_DENIED);
        ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
        EXPECT_EQ(tmplt2check->id, tid1);
    } else {
        EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid1, FDS_TYPE_TEMPLATE_UNDEF), FDS_OK);
        EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_ERR_NOTFOUND);
    }
    struct fds_template *t1_new = TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid1);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t1_new), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid1);

    // Try to remove T2 from whole history
    EXPECT_EQ(fds_tmgr_template_remove(tmgr, tid2, FDS_TYPE_TEMPLATE_OPTS), FDS_OK);

    // Set new export time and check templates
    const uint32_t time3 = 20;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time3), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid1);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_ERR_NOTFOUND);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid3);

    // Check snapshot
    ASSERT_NE(tmplt2check = fds_tsnapshot_template_get(snap, tid1), nullptr);
    EXPECT_EQ(tmplt2check->id, tid1);
    ASSERT_NE(tmplt2check = fds_tsnapshot_template_get(snap, tid2), nullptr);
    EXPECT_EQ(tmplt2check->id, tid2);
    EXPECT_EQ(tmplt2check->time.first_seen, time1);
    ASSERT_NE(tmplt2check = fds_tsnapshot_template_get(snap, tid3), nullptr);
    EXPECT_EQ(tmplt2check->id, tid3);
}

// Try to add already added template without creating snapshot
TEST_P(Common, addAlreadyAddedTemplate)
{
    // Set time and add a template
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 1000), FDS_OK);
    uint16_t tid1 = 2222;
    struct fds_template *t1 = TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid1);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t1), FDS_OK);

    const struct fds_template *tmplt2check = nullptr;
    struct fds_template *t1_new = TMock::create(TMock::type::OPTS_MPROC_RSTAT, tid1);
    if (GetParam() == FDS_SESSION_TYPE_UDP || GetParam() == FDS_SESSION_TYPE_IPFIX_FILE) {
        // We should be able to redefine the template
        EXPECT_EQ(fds_tmgr_template_add(tmgr, t1_new), FDS_OK);
        ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
        EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE_OPTS);
    } else {
        // We should NOT be able to redefine the template
        ASSERT_EQ(fds_tmgr_template_add(tmgr, t1_new), FDS_ERR_DENIED);
        fds_template_destroy(t1_new);
        ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
        EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE);
    }
}

// TODO: ie manager ... set/remove

