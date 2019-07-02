/**
 * \brief Test cases for all types of sessions
 */

#include <gtest/gtest.h>
#include <libfds.h>
#include <TGenerator.h>
#include <TMock.h>
#include <cstdint>

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
    ::testing::Values(FDS_SESSION_UDP, FDS_SESSION_TCP, FDS_SESSION_SCTP,
        FDS_SESSION_FILE));

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
    if (GetParam() != FDS_SESSION_UDP) {
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
    if (GetParam() != FDS_SESSION_UDP) {
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
    EXPECT_TRUE((tmplt2check->flags & FDS_TEMPLATE_FKEY) != 0);
    EXPECT_EQ(fds_template_flowkey_cmp(tmplt2check, fkey), 0);

    // Options templates can have flow key (really? not sure...)
    EXPECT_EQ(fds_tmgr_template_set_fkey(tmgr, tid2, fkey), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    EXPECT_NE(tmplt2check->flags & FDS_TEMPLATE_FKEY, 0);

    // Set new export time and create another snapshot
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 10), FDS_OK);
    const fds_tsnapshot_t *snap_with;
    EXPECT_EQ(fds_tmgr_snapshot_get(tmgr, &snap_with), FDS_OK);

    // Refresh the template and make sure that flow key is also set
    struct fds_template *t1_refresh = TMock::create(TMock::type::DATA_BASIC_FLOW, tid1);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t1_refresh), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid1);
    EXPECT_TRUE((tmplt2check->flags & FDS_TEMPLATE_FKEY) != 0);
    EXPECT_EQ(fds_template_flowkey_cmp(tmplt2check, fkey), 0);
    EXPECT_EQ(tmplt2check->time.first_seen, 0);
    EXPECT_EQ(tmplt2check->time.last_seen, 10);

    // Remove the flow key
    EXPECT_EQ(fds_tmgr_template_set_fkey(tmgr, tid1, 0), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_TRUE((tmplt2check->flags & FDS_TEMPLATE_FKEY) == 0);

    // Try to define flow key of non-existing template
    const uint16_t tid3 = 55555;
    EXPECT_EQ(fds_tmgr_template_set_fkey(tmgr, tid3, fkey), FDS_ERR_NOTFOUND);

    // Snapshot should be untouched
    ASSERT_NE(tmplt2check = fds_tsnapshot_template_get(snap_with, tid1), nullptr);
    EXPECT_TRUE((tmplt2check->flags & FDS_TEMPLATE_FKEY) != 0);

    ASSERT_NE(tmplt2check = fds_tsnapshot_template_get(snap_without, tid1), nullptr);
    EXPECT_TRUE((tmplt2check->flags & FDS_TEMPLATE_FKEY) == 0);
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
    EXPECT_EQ(tmplt->flags & FDS_TEMPLATE_FKEY, 0);
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
    if (GetParam() != FDS_SESSION_UDP && GetParam() != FDS_SESSION_FILE) {
        EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid1, FDS_TYPE_TEMPLATE_UNDEF), FDS_OK);
    }
    tmplt = TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid1);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, tmplt), FDS_OK);

    // Check the template
    const fds_template *tmplt2check;
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid1);
    EXPECT_EQ(tmplt2check->flags & FDS_TEMPLATE_FKEY, 0);
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
    if (GetParam() == FDS_SESSION_UDP) {
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
    if (GetParam() == FDS_SESSION_UDP || GetParam() == FDS_SESSION_FILE) {
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

// Define IE manager before first usage...
TEST_P(Common, ieManagerSimple)
{
    // Create a manager and load definitions
    fds_iemgr_t *iemgr = fds_iemgr_create();
    ASSERT_NE(iemgr, nullptr);
    ASSERT_EQ(fds_iemgr_read_file(iemgr, "./data/iana.xml", false), FDS_OK);

    // Assign the IE manager to the template manager
    fds_tmgr_set_iemgr(tmgr, iemgr);

    // Try to add few templates
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 0), FDS_OK);
    const uint16_t tid1 = 300;
    const uint16_t tid2 = 400;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid1)), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_FKEY, tid2)), FDS_OK);

    // Biflow can be detected only based on knowledge of IE definitions
    const struct fds_template *tmplt2check;
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_NE(tmplt2check->flags & FDS_TEMPLATE_BIFLOW, 0);
    for (uint16_t i = 0; i < tmplt2check->fields_cnt_total; ++i) {
        const struct fds_tfield *field = &tmplt2check->fields[i];
        EXPECT_NE(field->def, nullptr);
    }

    // The file with definitions doesn't include fields specific to Flow Key
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    for (uint16_t i = 0; i < tmplt2check->fields_cnt_total; ++i) {
        const struct fds_tfield *field = &tmplt2check->fields[i];
        EXPECT_EQ(field->def, nullptr); // Should be undefined
    }

    // Change export time
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 10), FDS_OK);

    // Try template refresh and make sure that definitions remains
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid1)), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->time.first_seen, 0);
    EXPECT_EQ(tmplt2check->time.last_seen, 10);
    EXPECT_NE(tmplt2check->flags & FDS_TEMPLATE_BIFLOW, 0);
    for (uint16_t i = 0; i < tmplt2check->fields_cnt_total; ++i) {
        const struct fds_tfield *field = &tmplt2check->fields[i];
        EXPECT_NE(field->def, nullptr);
    }

    // Try template redefinition and make sure that definitions will be set
    if (GetParam() != FDS_SESSION_UDP) {
        EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid2, FDS_TYPE_TEMPLATE_UNDEF), FDS_OK);
    }
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_FLOW, tid2)), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->time.first_seen, 10);
    EXPECT_EQ(tmplt2check->time.last_seen, 10);
    EXPECT_EQ(tmplt2check->flags & FDS_TEMPLATE_BIFLOW, 0);
    for (uint16_t i = 0; i < tmplt2check->fields_cnt_total; ++i) {
        const struct fds_tfield *field = &tmplt2check->fields[i];
        EXPECT_NE(field->def, nullptr);
    }

    // Try to define a new template
    const uint16_t tid3 = 500;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid3)), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_OK);
    EXPECT_NE(tmplt2check->flags & FDS_TEMPLATE_BIFLOW, 0);
    for (uint16_t i = 0; i < tmplt2check->fields_cnt_total; ++i) {
        const struct fds_tfield *field = &tmplt2check->fields[i];
        EXPECT_NE(field->def, nullptr);
    }

    // It should be safe to delete the IE manager here
    fds_iemgr_destroy(iemgr);
}

// Try to redefine IE manager
TEST_P(Common, ieManagerRedefine)
{
    const struct fds_template *tmplt2check = nullptr;

    // Create an simple manager and add manually few definitions
    fds_iemgr_t *iemgr_simple = fds_iemgr_create();
    ASSERT_NE(iemgr_simple, nullptr);

    struct fds_iemgr_elem elem_bytes;
    memset(&elem_bytes, 0, sizeof(elem_bytes));
    elem_bytes.id = 1;
    elem_bytes.name = const_cast<char *>("bytes");
    elem_bytes.data_type = FDS_ET_UNSIGNED_64;
    elem_bytes.data_unit = FDS_EU_OCTETS;
    ASSERT_EQ(fds_iemgr_elem_add(iemgr_simple, &elem_bytes, 0, false), FDS_OK);

    struct fds_iemgr_elem elem_fkeyind;
    memset(&elem_fkeyind, 0, sizeof(elem_fkeyind));
    elem_fkeyind.id = 173;
    elem_fkeyind.name = const_cast<char *>("fKeyID");
    elem_fkeyind.data_type = FDS_ET_UNSIGNED_64;
    ASSERT_EQ(fds_iemgr_elem_add(iemgr_simple, &elem_fkeyind, 0, false), FDS_OK);

    // Set export time and define few templates
    ASSERT_EQ(fds_tmgr_set_iemgr(tmgr, iemgr_simple), FDS_OK);
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 100), FDS_OK);

    const uint16_t tid1 = 256;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_FLOW, tid1)), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    for (uint16_t i = 0; i < tmplt2check->fields_cnt_total; ++i) {
        // Everything except PEN:0, ID:1 (a.k.a. "bytes") should be undefined
        const struct fds_tfield *field = &tmplt2check->fields[i];
        if (field->id == 1 && field->en == 0) {
            ASSERT_NE(field->def, nullptr);
            EXPECT_STREQ(field->def->name, elem_bytes.name);
            continue;
        }

        EXPECT_EQ(field->def, nullptr);
    }

    const uint16_t tid2 = 257;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_FKEY, tid2)), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    const struct fds_tfield *field = fds_template_cfind(tmplt2check, 0, elem_fkeyind.id);
    ASSERT_NE(field, nullptr);
    ASSERT_NE(field->def, nullptr);
    EXPECT_STREQ(field->def->name, elem_fkeyind.name);

    // Set different export time and define another template
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 110), FDS_OK);
    const uint16_t tid3 = 258;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid3)), FDS_OK);
    // Withdraw the T1 template (expect UDP)
    if (GetParam() != FDS_SESSION_UDP) {
        EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid1, FDS_TYPE_TEMPLATE_UNDEF), FDS_OK);
    }

    // Create a new IE manager, load definitions and apply changes
    fds_iemgr_t *iemgr_file = fds_iemgr_create();
    ASSERT_NE(iemgr_file, nullptr);
    ASSERT_EQ(fds_iemgr_read_file(iemgr_file, "./data/iana.xml", false), FDS_OK);
    ASSERT_EQ(fds_tmgr_set_iemgr(tmgr, iemgr_file), FDS_OK);
    // We don't need the old manager anymore...
    fds_iemgr_destroy(iemgr_simple);

    // Time context has been lost -> define it again
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 110), FDS_OK);
    // Check templates (T1 is available only for UDP)
    if (GetParam() != FDS_SESSION_UDP) {
        EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_ERR_NOTFOUND);
    } else {
        // UDP only
        ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
        for (uint16_t i = 0; i < tmplt2check->fields_cnt_total; ++i) {
            const struct fds_tfield *field = &tmplt2check->fields[i];
            EXPECT_NE(field->def, nullptr);
            EXPECT_EQ(field->def->id, field->id);
        }
    }

    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE_OPTS);
    field = fds_template_cfind(tmplt2check, 0, elem_fkeyind.id);
    ASSERT_NE(field, nullptr);
    ASSERT_EQ(field->def, nullptr); // The Information Element is not defined in the file

    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE);
    EXPECT_NE(tmplt2check->flags & FDS_TEMPLATE_BIFLOW, 0);
    field = fds_template_cfind(tmplt2check, 0, elem_bytes.id);
    ASSERT_NE(field, nullptr);
    ASSERT_NE(field->def, nullptr);
    EXPECT_STRNE(field->def->name, elem_bytes.name);

    // Check history (except TCP)
    if (GetParam() != FDS_SESSION_TCP) {
        EXPECT_EQ(fds_tmgr_set_time(tmgr, 105), FDS_OK);
        // Template T1 should be available and all fields should have a reference to an IE def.
        ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
        for (uint16_t i = 0; i < tmplt2check->fields_cnt_total; ++i) {
            const struct fds_tfield *field = &tmplt2check->fields[i];
            EXPECT_NE(field->def, nullptr);
            EXPECT_EQ(field->def->id, field->id);
        }
    }

    // Try to create a new template
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 110), FDS_OK);
    const uint16_t tid4 = 259;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_FLOW, tid4)), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid4, &tmplt2check), FDS_OK);
    // Check that all fields have known definitions
    for (uint16_t i = 0; i < tmplt2check->fields_cnt_total; ++i) {
        const struct fds_tfield *field = &tmplt2check->fields[i];
        EXPECT_NE(field->def, nullptr);
        EXPECT_EQ(field->def->id, field->id);
    }

    // Remove all definitions
    ASSERT_EQ(fds_tmgr_set_iemgr(tmgr, nullptr), FDS_OK);
    fds_iemgr_destroy(iemgr_file);

    // Check that all definitions are not available and derived features have been removed
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 115), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE);
    EXPECT_EQ(tmplt2check->flags & FDS_TEMPLATE_BIFLOW, 0); // Biflow flag should be lost...
    for (uint16_t i = 0; i < tmplt2check->fields_cnt_total; ++i) {
        const struct fds_tfield *field = &tmplt2check->fields[i];
        EXPECT_EQ(field->def, nullptr);
    }
}

// Auxiliary data structure used in a callback function of a snapshot iterator
struct snapshot_iterator_data {
    // List of visited Template IDs
    std::set<uint16_t> ids;
    // Callback function fails (i.e. returns false) if the limit is equal to zero
    ssize_t limit;
};

// Auxiliary callback function
bool snapshot_iterator_cb(const struct fds_template *tmplt, void *data)
{
    auto my_data = reinterpret_cast<snapshot_iterator_data *>(data);

    // Check whether the callback function should "fail"
    EXPECT_GE(my_data->limit, 0) << "Called with negative limit value!";
    if (my_data->limit <= 0) {
        my_data->limit = -1;
        return false;
    }
    my_data->limit--;

    if (tmplt->id % 2 == 0) {
        // Biflow Template
        EXPECT_EQ(tmplt->type, FDS_TYPE_TEMPLATE);
    } else {
        // Options Template
        EXPECT_EQ(tmplt->type, FDS_TYPE_TEMPLATE_OPTS);
    }

    my_data->ids.emplace(tmplt->id);
    return true;
}

// Try to iterate over a Template snapshot
TEST_P(Common, snapshotIterator)
{
    // Add few Templates to the manager
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 123456), FDS_OK);

    // The list must be sorted and all values must be unique!
    std::initializer_list<uint16_t> list = {256, 257, 511, 512, 513, 564, 1000, 1457, 2234, 65535};
    for (auto it : list) {
        struct fds_template *tmplt_ptr;
        if (it % 2 == 0) { // Even Template IDs belong to biflow template
            tmplt_ptr = TMock::create(TMock::type::DATA_BASIC_BIFLOW, it);
        } else { // Odd Template IDs belong to an options template
            tmplt_ptr = TMock::create(TMock::type::OPTS_MPROC_STAT, it);
        }
        EXPECT_EQ(fds_tmgr_template_add(tmgr, tmplt_ptr), FDS_OK);
    }

    // Get the snapshot
    const struct fds_tsnapshot *snap = nullptr;
    ASSERT_EQ(fds_tmgr_snapshot_get(tmgr, &snap), FDS_OK);
    ASSERT_NE(snap, nullptr);

    // Call the snapshot function for each Template in the snapshot
    struct snapshot_iterator_data cb_data;
    cb_data.limit = list.size() + 1; // Enough to process all Templates and limit should remain 1U

    fds_tsnapshot_for(snap, &snapshot_iterator_cb, &cb_data);
    // Check if all Templates have been visited
    EXPECT_EQ(cb_data.limit, 1U);
    EXPECT_EQ(cb_data.ids.size(), list.size());
    for (uint16_t item : list) {
        SCOPED_TRACE("item: " + std::to_string(item));
        EXPECT_NE(cb_data.ids.find(item), cb_data.ids.end());
    }

    // Try to use the iterator again, but make sure that the callback will not process all Templates
    ssize_t tmplt_cnt = 5;
    cb_data.ids.clear();
    cb_data.limit = tmplt_cnt; // return "false" after 5 processed templates

    fds_tsnapshot_for(snap, &snapshot_iterator_cb, &cb_data);
    EXPECT_EQ(cb_data.limit, -1);
    EXPECT_EQ(cb_data.ids.size(), size_t(tmplt_cnt));

    auto list_pos = list.begin();
    std::advance(list_pos, tmplt_cnt);
    for (auto it = list.begin(); it < list_pos; ++it) {
        SCOPED_TRACE("item: " + std::to_string(*it));
        EXPECT_NE(cb_data.ids.find(*it), cb_data.ids.end());
    }
}


// TODO: Multiple updates of the same template at the same time + cleanup

// TODO: Test snapshot lifetime (TO ALL except TCP)
