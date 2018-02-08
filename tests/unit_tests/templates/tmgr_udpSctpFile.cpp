/**
 * \brief Test cases only for UDP, SCTP and IPFIX FILE sessions
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
class udpSctpFile : public ::testing::TestWithParam<enum fds_session_type> {
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
INSTANTIATE_TEST_CASE_P(TemplateManager, udpSctpFile,
    ::testing::Values(FDS_SESSION_TYPE_UDP, FDS_SESSION_TYPE_SCTP, FDS_SESSION_TYPE_IPFIX_FILE));

// Try to access templates defined in history
TEST_P(udpSctpFile, historyAccess)
{
    fds_tmgr_set_snapshot_timeout(tmgr, 30);

    // Add a template
    const uint32_t time10 = 10;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time10), FDS_OK);

    const uint16_t tid1 = 256;
    struct fds_template *t1 = TMock::create(TMock::type::DATA_BASIC_FLOW, tid1);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t1), FDS_OK);

    // Change export time and add another template
    const uint32_t time15 = 15;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time15), FDS_OK);

    const uint16_t tid2 = 257;
    struct fds_template *t2 = TMock::create(TMock::type::DATA_BASIC_FLOW, tid2);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t2), FDS_OK);

    // Change export time again adn add another template
    const uint32_t time20 = 20;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time20), FDS_OK);

    const uint16_t tid3 = 258;
    struct fds_template *t3 = TMock::create(TMock::type::DATA_BASIC_FLOW, tid3);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t3), FDS_OK);

    // Now back and check availability of templates
    const struct fds_template *tmplt2check;
    // Time: 10
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time10), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid1);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_ERR_NOTFOUND);
    // Time: 20
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time20), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid1);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid2);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid3);
    // Time: 15
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time15), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid1);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid2);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_ERR_NOTFOUND);
}

// Add a template in history and make sure that it will be propagated
TEST_P(udpSctpFile, historyAdd)
{
    fds_tmgr_set_snapshot_timeout(tmgr, 30);

    // Set export time and add templates
    const uint32_t time100 = 100;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time100), FDS_OK);
    const uint16_t tid1 = 256;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid1)), FDS_OK);

    // Change export time again and add new templates
    const uint32_t time102 = 102;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time102), FDS_OK);
    const uint16_t tid2 = 257;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_MPROC_STAT, tid2)), FDS_OK);

    // Go back and define a template T3
    const uint32_t time101 = 101;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time101), FDS_OK);
    const uint16_t tid3 = 258;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_FKEY, tid3)), FDS_OK);

    // Change export time and Check if the template has been propagated
    const struct fds_template *tmplt2check;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time102), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->time.first_seen, time101);

    // Go back to history and check availability
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time101), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_OK);

    EXPECT_EQ(fds_tmgr_set_time(tmgr, time100), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_ERR_NOTFOUND);
}

// Redefine a template in history and make sure that modification will be propagated
TEST_P(udpSctpFile, historyRedefinition)
{
    fds_tmgr_set_snapshot_timeout(tmgr, 30);

    // Set export time and add templates
    const uint32_t time10 = 10;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time10), FDS_OK);
    const uint16_t tid1 = 256;
    const uint16_t tid2 = 257;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_FLOW, tid1)), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_MPROC_STAT, tid2)), FDS_OK);

    // Change export time and check availability of templates
    const uint32_t time20 = 20;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time20), FDS_OK);
    const struct fds_template *tmplt2check = nullptr;
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE_OPTS);

    // Add a new template and create a snapshot
    const uint16_t tid3 = 258;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid3)), FDS_OK);
    const fds_tsnapshot_t *snap;
    EXPECT_EQ(fds_tmgr_snapshot_get(tmgr, &snap), FDS_OK);

    // Go back and change template T1
    const uint32_t time19 = 19;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time19), FDS_OK);
    if (GetParam() == FDS_SESSION_TYPE_SCTP) {
        EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid1, FDS_TYPE_TEMPLATE_UNDEF), FDS_OK);
    }
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_ERPOC_RSTAT, tid1)), FDS_OK);

    // Change the export time and check if the template T1 has been propagated
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time20), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid1);
    EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE_OPTS); // Type must be different!
    EXPECT_NE(tmplt2check->opts_types & FDS_OPTS_EPROC_RELIABILITY_STAT, 0);
    EXPECT_EQ(tmplt2check->time.first_seen, time19);
    // T2 + T3 should be still available
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid2);
    EXPECT_EQ(tmplt2check->time.first_seen, time10);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->time.first_seen, time20);

    // Create a garbage and make sure that the snapshot is still usable
    fds_tgarbage_t *garbage;
    EXPECT_EQ(fds_tmgr_garbage_get(tmgr, &garbage), FDS_OK);
    ASSERT_NE(tmplt2check = fds_tsnapshot_template_get(snap, tid1), nullptr);
    EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE);
    // Now we can destroy the garbage and the snapshot should not be accessible
    fds_tmgr_garbage_destroy(garbage);

    // Try go back to the history and check that the previous T1 is still there
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time10), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid1);
    EXPECT_EQ(tmplt2check->time.first_seen, time10);
    EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE);
}

// Refresh template in history + flow key propagation
TEST_P(udpSctpFile, refreshPropagation)
{
    fds_tmgr_set_snapshot_timeout(tmgr, 20);

    // Add a template
    const uint32_t time200 = 200;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time200), FDS_OK);
    const uint16_t tid1 = 511;
    const uint16_t tid2 = 512;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid1)), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_FKEY, tid2)), FDS_OK);

    // Change export time and add a new template
    const uint32_t time210 = 210;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time210), FDS_OK);
    const uint16_t tid3 = 513;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_FLOW, tid3)), FDS_OK);

    // Go back and refresh T1
    const uint32_t time205 = 205;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time205), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid1)), FDS_OK);
    const struct fds_template *tmplt2check = nullptr;
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->time.first_seen, time200);
    EXPECT_EQ(tmplt2check->time.last_seen, time205);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_ERR_NOTFOUND);

    // Check if the template has been propagated
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time210), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->time.first_seen, time200);
    EXPECT_EQ(tmplt2check->time.last_seen, time205);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->time.first_seen, time200);
    EXPECT_EQ(tmplt2check->time.last_seen, time200);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->time.first_seen, time210);
    EXPECT_EQ(tmplt2check->time.last_seen, time210);
}

// Try to go to deeply into history (behind snapshot limit)
TEST_P(udpSctpFile, goEmptyHistory)
{
    fds_tmgr_set_snapshot_timeout(tmgr, 20);
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 100), FDS_OK); // First usage of the manager
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 80),  FDS_OK);
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 50),  FDS_ERR_NOTFOUND); // Too far in history
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 0),   FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 100), FDS_OK);

    // Clear the manager
    fds_tmgr_clear(tmgr);
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 50),  FDS_OK);
}

// Check if templates in history (behind snapshot limit) are no longer available, but a snapshot
// still can be used.
TEST_P(udpSctpFile, historyLimitAutoRemove)
{
    fds_tmgr_set_snapshot_timeout(tmgr, 20);
    // Note: UDP template timeouts are disabled by default...

    // Set export time and add few templates
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 500), FDS_OK);
    const uint16_t tid1 = 1024;
    const uint16_t tid2 = 1025;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid1)), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_FKEY, tid2)), FDS_OK);

    // Create a snapshot
    const fds_tsnapshot_t *snap;
    ASSERT_EQ(fds_tmgr_snapshot_get(tmgr, &snap), FDS_OK);

    // Change export time and Remove/redefine templates
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 600), FDS_OK);
    if (GetParam() == FDS_SESSION_TYPE_SCTP) {
        // Remove template T1
        EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid1, FDS_TYPE_TEMPLATE), FDS_OK);
    } else {
        // Redefine template T2
        EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_FKEY, tid1)), FDS_OK);
    }

    // Go to the future, cleanup manager and check availability of templates
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 700), FDS_OK);
    fds_tgarbage_t *garbage;
    EXPECT_EQ(fds_tmgr_garbage_get(tmgr, &garbage), FDS_OK);

    const struct fds_template *tmplt2check;
    if (GetParam() == FDS_SESSION_TYPE_SCTP) {
        EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_ERR_NOTFOUND);
    } else {
        ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
        EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE_OPTS);
    }

    // Access to history should not be possible
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 500), FDS_ERR_NOTFOUND);
    // ...but the snapshot should be still usable
    ASSERT_NE(tmplt2check = fds_tsnapshot_template_get(snap, tid1), nullptr);
    EXPECT_EQ(tmplt2check->id, tid1);
    EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE);

    fds_tmgr_garbage_destroy(garbage);
}

// Try to refresh template in history, but new there is already newer definition (stop propagation)
TEST_P(udpSctpFile, stopPropagation)
{
    fds_tmgr_set_snapshot_timeout(tmgr, 30);
    // Note: UDP template timeouts are disabled by default...

    // Set export time and add few templates
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 0), FDS_OK);
    const uint16_t tid1 = 256;
    const uint16_t tid2 = 257;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_FLOW, tid1)), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_MPROC_STAT, tid2)), FDS_OK);

    // Set export time and refresh the template T1
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 20), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_FLOW, tid1)), FDS_OK);
    const struct fds_template *tmplt2check;
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->time.first_seen, 0);
    EXPECT_EQ(tmplt2check->time.last_seen, 20);

    // Go back in time and refresh both templates
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 10), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_FLOW, tid1)), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_MPROC_STAT, tid2)), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->time.first_seen, 0);
    EXPECT_EQ(tmplt2check->time.last_seen, 10);

    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->time.first_seen, 0);
    EXPECT_EQ(tmplt2check->time.last_seen, 10);

    // Return export time
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 20), FDS_OK);
    // Template T1 should not be changed
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->time.first_seen, 0);
    EXPECT_EQ(tmplt2check->time.last_seen, 20); // <- 20
    // Template T2 should be refreshed
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->time.first_seen, 0);
    EXPECT_EQ(tmplt2check->time.last_seen, 10); // <- 10
}

// Try to withdraw a template in history but the template doesn't exist anymore
TEST_P(udpSctpFile, withdrawInHistory)
{
    fds_tmgr_set_snapshot_timeout(tmgr, 30);

    // Set export time and define few templates
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 50), FDS_OK);
    const uint16_t tid1 = 256;
    const uint16_t tid2 = 257;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_FLOW, tid1)), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_FKEY, tid2)), FDS_OK);

    // Set export time, withdraw the template T1 and define new one
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 70), FDS_OK);
    if (GetParam() != FDS_SESSION_TYPE_UDP) {
        ASSERT_EQ(fds_tmgr_template_withdraw(tmgr, tid1, FDS_TYPE_TEMPLATE), FDS_OK);
    }
    ASSERT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_MPROC_STAT, tid1)), FDS_OK);

    // Go back and try to withdraw the template again (should not affect redefined template)
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 65), FDS_OK);
    const struct fds_template *tmplt2check = nullptr;
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid1);

    if (GetParam() != FDS_SESSION_TYPE_UDP) {
        ASSERT_EQ(fds_tmgr_template_withdraw(tmgr, tid1, FDS_TYPE_TEMPLATE_UNDEF), FDS_OK);
    }

    // Go forward and check that the redefined template T1 is still available
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 70), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid1);
    EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE_OPTS);
    EXPECT_EQ(tmplt2check->time.first_seen, 70);

    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid2);
    EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE_OPTS);

}

// Try to redefine a template in history but the template already has new definition in future
TEST_P(udpSctpFile, redefineInHistory)
{
    fds_tmgr_set_snapshot_timeout(tmgr, 30);
    const struct fds_template *tmplt2check;

    // Set export time and define few templates
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 50), FDS_OK);
    const uint16_t tid1 = 256;
    const uint16_t tid2 = 257;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_FLOW, tid1)), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_FKEY, tid2)), FDS_OK);
    // Get a snapshot
    const fds_tsnapshot_t *snap;
    ASSERT_EQ(fds_tmgr_snapshot_get(tmgr, &snap), FDS_OK);

    // Go back and redefine the template T1
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 45), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_MPROC_STAT, tid1)), FDS_OK);

    // Change export time and check if the template T1 has been changed (MUST be unchanged)
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 55), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid1);
    EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE);
    EXPECT_EQ(tmplt2check->time.first_seen, 50);

    // Redefine the template T2
    if (GetParam() == FDS_SESSION_TYPE_SCTP) {
        ASSERT_EQ(fds_tmgr_template_withdraw(tmgr, tid2, FDS_TYPE_TEMPLATE_UNDEF), FDS_OK);
    }
    EXPECT_EQ(fds_tmgr_template_add(tmgr,TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid2)), FDS_OK);

    // Go back and refresh the template T2
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 52), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_FKEY, tid2)), FDS_OK);

    // Change the export time (go back to the future) and check the template (MUST be unchanged)
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 55), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid2);
    EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE);
    EXPECT_EQ(tmplt2check->time.first_seen, 55);

    // Check that the old snapshot hasn't been modified
    ASSERT_NE(tmplt2check = fds_tsnapshot_template_get(snap, tid1), nullptr);
    EXPECT_EQ(tmplt2check->id, tid1);
    EXPECT_EQ(tmplt2check->time.first_seen, 50);
    EXPECT_EQ(tmplt2check->time.last_seen, 50);
    EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE);

    ASSERT_NE(tmplt2check = fds_tsnapshot_template_get(snap, tid2), nullptr);
    EXPECT_EQ(tmplt2check->id, tid2);
    EXPECT_EQ(tmplt2check->time.first_seen, 50);
    EXPECT_EQ(tmplt2check->time.last_seen, 50);
    EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE_OPTS);
}

// Define a flow key in history and check modifications
TEST_P(udpSctpFile, flowKeyPropagation)
{
    fds_tmgr_set_snapshot_timeout(tmgr, 30);
    const struct fds_template *tmplt2check;

    // Set export time and add few template
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 1000), FDS_OK);
    const uint16_t tid1 = 512;
    const uint16_t tid2 = 513;
    const uint16_t tid3 = 514;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid1)), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_FLOW, tid2)), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid3)), FDS_OK);

    // Change export time and add a template T4
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 1003), FDS_OK);
    const uint16_t tid4 = 515;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_FLOW, tid4)), FDS_OK);

    // Change export time and refresh the template T1 and redefine T2
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 1005), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid1)), FDS_OK);
    if (GetParam() != FDS_SESSION_TYPE_UDP) {
        EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid2, FDS_TYPE_TEMPLATE), FDS_OK);
        EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid3, FDS_TYPE_TEMPLATE), FDS_OK);
    }
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_FLOW, tid2)), FDS_OK);
    // Create a snapshot
    const fds_tsnapshot_t *snap;
    EXPECT_EQ(fds_tmgr_snapshot_get(tmgr, &snap), FDS_OK);

    // Go back and define flow key...
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 1000), FDS_OK);
    const uint64_t tid1_key = 31;
    const uint64_t tid2_key = 15; // just examples
    const uint64_t tid3_key = 19;
    const uint64_t tid4_key = 31;
    EXPECT_EQ(fds_tmgr_template_set_fkey(tmgr, tid1, tid1_key), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_set_fkey(tmgr, tid2, tid2_key), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_set_fkey(tmgr, tid3, tid3_key), FDS_OK);
    // Try to assign a flow key to a non-existing template
    EXPECT_EQ(fds_tmgr_template_set_fkey(tmgr, tid4, tid4_key), FDS_ERR_NOTFOUND);

    // Change export time and check modifications
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 1005), FDS_OK);
    // Template T1 should have the flow key (it was just refreshed)
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(fds_template_flowkey_cmp(tmplt2check, tid1_key), 0);
    // Template T2 should not have the flow key (it was redefined) (except UDP)
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    if (GetParam() != FDS_SESSION_TYPE_UDP) {
        EXPECT_EQ(fds_template_flowkey_cmp(tmplt2check, 0), 0);
        EXPECT_EQ(tmplt2check->flags & FDS_TEMPLATE_FKEY, 0);
    } else {
        // In case of UDP, the flow key should be propagated because the T2 hasn't been withdrawn.
        EXPECT_EQ(fds_template_flowkey_cmp(tmplt2check, tid2_key), 0);
        EXPECT_NE(tmplt2check->flags & FDS_TEMPLATE_FKEY, 0);
    }
    // Template T3 should not be available (expect UDP)
    if (GetParam() != FDS_SESSION_TYPE_UDP) {
        ASSERT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_ERR_NOTFOUND);
    } else {
        ASSERT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_OK);
        EXPECT_EQ(fds_template_flowkey_cmp(tmplt2check, tid3_key), 0);
    }
    // Template T4 should not have the flow key
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid4, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->flags & FDS_TEMPLATE_FKEY, 0);

    // Check the snapshot
    // T1
    ASSERT_NE(tmplt2check = fds_tsnapshot_template_get(snap, tid1), nullptr);
    EXPECT_EQ(tmplt2check->time.first_seen, 1000);
    EXPECT_EQ(tmplt2check->time.last_seen, 1005);
    EXPECT_EQ(fds_template_flowkey_cmp(tmplt2check, 0), 0);
    // T2
    ASSERT_NE(tmplt2check = fds_tsnapshot_template_get(snap, tid2), nullptr);
    EXPECT_EQ(fds_template_flowkey_cmp(tmplt2check, 0), 0);
    if (GetParam() != FDS_SESSION_TYPE_UDP) {
        EXPECT_EQ(tmplt2check->time.first_seen, 1005);
        EXPECT_EQ(tmplt2check->time.last_seen, 1005);
    } else {
        EXPECT_EQ(tmplt2check->time.first_seen, 1000);
        EXPECT_EQ(tmplt2check->time.last_seen, 1005);
    }
    // T3
    if (GetParam() != FDS_SESSION_TYPE_UDP) {
        EXPECT_EQ(tmplt2check = fds_tsnapshot_template_get(snap, tid3), nullptr);
    } else {
        ASSERT_NE(tmplt2check = fds_tsnapshot_template_get(snap, tid3), nullptr);
        EXPECT_EQ(fds_template_flowkey_cmp(tmplt2check, 0), 0);
        EXPECT_EQ(tmplt2check->time.first_seen, 1000);
        EXPECT_EQ(tmplt2check->time.last_seen, 1000);
    }
    // T4
    ASSERT_NE(tmplt2check = fds_tsnapshot_template_get(snap, tid4), nullptr);
    EXPECT_EQ(fds_template_flowkey_cmp(tmplt2check, 0), 0);
    EXPECT_EQ(tmplt2check->time.first_seen, 1003);
    EXPECT_EQ(tmplt2check->time.last_seen, 1003);
}

// Try to collect and clear garbage in history
TEST_P(udpSctpFile, clearGarbageInHistory)
{
    fds_tmgr_set_snapshot_timeout(tmgr, 10);
    const struct fds_template *tmplt2check;
    const fds_tsnapshot_t *dummy_snap;

    // Set export time and add few templates
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 0), FDS_OK);
    const uint16_t tid1 = 256;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_FLOW, tid1)), FDS_OK);
    // Get a snapshot (this should fix the current snapshot)
    EXPECT_EQ(fds_tmgr_snapshot_get(tmgr, &dummy_snap), FDS_OK);

    // Add a new template
    const uint16_t tid2 = 257;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid2)), FDS_OK);
    // Get another snapshot
    EXPECT_EQ(fds_tmgr_snapshot_get(tmgr, &dummy_snap), FDS_OK);

    // Redefine a template
    if (GetParam() == FDS_SESSION_TYPE_SCTP) {
        EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid1, FDS_TYPE_TEMPLATE_UNDEF), FDS_OK);
    }
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_FKEY, tid1)), FDS_OK);

    // Change export time and add a new template
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 10), FDS_OK);
    const uint16_t tid3 = 258;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_MPROC_RSTAT, tid3)), FDS_OK);

    // Go back and remove garbage
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 0), FDS_OK);
    fds_tgarbage_t *garbage;
    EXPECT_EQ(fds_tmgr_garbage_get(tmgr, &garbage), FDS_OK);
    fds_tmgr_garbage_destroy(garbage);

    // Check availability of templates
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE_OPTS);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE);

    // Change time
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 10), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE_OPTS);
}

// Try to add/remove templates in history
TEST_P(udpSctpFile, timeWrapparound)
{
    fds_tmgr_set_snapshot_timeout(tmgr, 30);
    const struct fds_template *tmplt2check;

    // Set export time and add a template
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 5), FDS_OK);
    const uint16_t tid1 = 256;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid1)), FDS_OK);

    // Go back in time
    EXPECT_EQ(fds_tmgr_set_time(tmgr, UINT32_MAX - 5), FDS_OK);
    const uint16_t tid2 = 257;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_FKEY, tid2)), FDS_OK);
    // Remove T1
    EXPECT_EQ(fds_tmgr_template_remove(tmgr, tid1, FDS_TYPE_TEMPLATE_UNDEF), FDS_OK);

    // Set export time and check templates
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 5), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_ERR_NOTFOUND);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->type, FDS_TYPE_TEMPLATE_OPTS);
}
