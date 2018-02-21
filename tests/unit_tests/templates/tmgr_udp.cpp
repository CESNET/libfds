/**
 * \brief Test cases only for UDP sessions
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
class udp : public ::testing::TestWithParam<enum fds_session_type> {
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
INSTANTIATE_TEST_CASE_P(TemplateManager, udp,
    ::testing::Values(FDS_SESSION_UDP));

// Try to withdraw a template (not permitted operation)
TEST_P(udp, invalidWithdraw)
{
    const uint16_t tid1 = 256;
    const uint16_t tid2 = 5555;

    // Undefined time context
    EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid1, FDS_TYPE_TEMPLATE_UNDEF), FDS_ERR_ARG);
    EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid2, FDS_TYPE_TEMPLATE_UNDEF), FDS_ERR_ARG);

    // Set export time
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 9000000), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid1, FDS_TYPE_TEMPLATE_UNDEF), FDS_ERR_DENIED);

    // Add templates
    struct fds_template *t1 = TMock::create(TMock::type::DATA_BASIC_FLOW, tid1);
    struct fds_template *t2 = TMock::create(TMock::type::OPTS_MPROC_RSTAT, tid2);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t1), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t2), FDS_OK);

    // Try to withdraw a template
    EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid2, FDS_TYPE_TEMPLATE_OPTS), FDS_ERR_DENIED);

    // Set new export time and check template availability (templates should be available)
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 10000000), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid1, FDS_TYPE_TEMPLATE_UNDEF), FDS_ERR_DENIED);

    const struct fds_template *tmplt = nullptr;
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt), FDS_OK);
    EXPECT_EQ(tmplt->id, tid1);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt), FDS_OK);
    EXPECT_EQ(tmplt->id, tid2);
}

// Try to withdraw all template (not permitted operation)
TEST_P(udp, invalidWithdrawAll)
{
    // Undefined time context
    EXPECT_EQ(fds_tmgr_template_withdraw_all(tmgr, FDS_TYPE_TEMPLATE), FDS_ERR_ARG);
    EXPECT_EQ(fds_tmgr_template_withdraw_all(tmgr, FDS_TYPE_TEMPLATE_OPTS), FDS_ERR_ARG);
    EXPECT_EQ(fds_tmgr_template_withdraw_all(tmgr, FDS_TYPE_TEMPLATE_UNDEF), FDS_ERR_ARG);

    // Set export time
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 1000), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_withdraw_all(tmgr, FDS_TYPE_TEMPLATE_UNDEF), FDS_ERR_DENIED);

    // Add template
    const uint16_t tid1 = 65535;
    const uint16_t tid2 = 256;
    struct fds_template *t1 = TMock::create(TMock::type::OPTS_FKEY, tid1);
    struct fds_template *t2 = TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid2);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t1), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t2), FDS_OK);

    // Try to withdraw templates
    EXPECT_EQ(fds_tmgr_template_withdraw_all(tmgr, FDS_TYPE_TEMPLATE), FDS_ERR_DENIED);
    EXPECT_EQ(fds_tmgr_template_withdraw_all(tmgr, FDS_TYPE_TEMPLATE_OPTS), FDS_ERR_DENIED);
    EXPECT_EQ(fds_tmgr_template_withdraw_all(tmgr, FDS_TYPE_TEMPLATE_UNDEF), FDS_ERR_DENIED);

    // Template should be still available
    const struct fds_template *tmplt = nullptr;
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt), FDS_OK);
    EXPECT_EQ(tmplt->id, tid1);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt), FDS_OK);
    EXPECT_EQ(tmplt->id, tid2);
}

// Test template timeout
TEST_P(udp, templateTimeout)
{
    // Enable template timeout
    fds_tmgr_set_udp_timeouts(tmgr, 10, 10);
    fds_tmgr_set_snapshot_timeout(tmgr, 60);

    // Set export time and add a template
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 0), FDS_OK);
    const uint16_t tid1 = 256;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_FLOW, tid1)), FDS_OK);

    // Set new export time and add a new template
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 5), FDS_OK);
    const uint16_t tid2 = 257;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid2)), FDS_OK);

    // Set new export time and check availability of template (both templates should be available)
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 9), FDS_OK);
    const struct fds_template *tmplt2check;
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->time.first_seen, 0);
    EXPECT_EQ(tmplt2check->time.last_seen, 0);
    EXPECT_EQ(tmplt2check->time.end_of_life, 10); // Timeout is 10 seconds
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->time.first_seen, 5);
    EXPECT_EQ(tmplt2check->time.last_seen, 5);
    EXPECT_EQ(tmplt2check->time.end_of_life, 15); // Timeout is 10 seconds

    // Change the export time so the template T1 should be expired
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 11), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_ERR_NOTFOUND);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid2);

    // Change the export time so the template T2 should be expired
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 16), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_ERR_NOTFOUND);
}

// Test template timeouts and seeking in history backwards and forwards
TEST_P(udp, templateTimeoutAdvanced)
{
    // Enable template timeout
    fds_tmgr_set_udp_timeouts(tmgr, 10, 10);
    fds_tmgr_set_snapshot_timeout(tmgr, 60);

    // Set export time (500) and add a template
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 500), FDS_OK);
    const uint16_t tid1 = 256;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_FLOW, tid1)), FDS_OK);
    // Set export time and add another template
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 505), FDS_OK);
    const uint16_t tid2 = 257;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_MPROC_RSTAT, tid2)), FDS_OK);
    // Set export time and add another template
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 510), FDS_OK);
    const uint16_t tid3 = 258;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_ERPOC_RSTAT, tid3)), FDS_OK);

    // Set export time and check availability of templates
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 525), FDS_OK);
    const struct fds_template *tmplt2check;
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_ERR_NOTFOUND);

    // Go back and perform another check
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 519), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_ERR_NOTFOUND);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid3);

    // Go back again
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 512), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_ERR_NOTFOUND);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid2);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid3);

    // Go back even more
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 508), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid1);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->id, tid2);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_ERR_NOTFOUND);
}

// Try to refresh template in history and check propagation
TEST_P(udp, templateTimeoutRefresh)
{
    fds_tmgr_set_udp_timeouts(tmgr, 10, 10);
    fds_tmgr_set_snapshot_timeout(tmgr, 60);

    // Set export time and add a template
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 0), FDS_OK);
    const uint16_t tid1 = 1000;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid1)), FDS_OK);

    // Set new export time and check if the template has expired
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 11), FDS_OK);
    const struct fds_template *tmplt2check = nullptr;
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_ERR_NOTFOUND);
    // Create a snapshot
    const fds_tsnapshot_t *snap = nullptr;
    ASSERT_EQ(fds_tmgr_snapshot_get(tmgr, &snap), FDS_OK);

    // Go back and refresh the template
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 8), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid1)), FDS_OK);

    // Set new export time -> the template should be available
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 11), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(tmplt2check->time.first_seen, 0);
    EXPECT_EQ(tmplt2check->time.last_seen, 8);

    // Get garbage
    fds_tgarbage_t *garbage;
    EXPECT_EQ(fds_tmgr_garbage_get(tmgr, &garbage), FDS_OK);

    // Try to access the snapshot and make sure that template is not available
    EXPECT_EQ(tmplt2check = fds_tsnapshot_template_get(snap, tid1), nullptr);
    fds_tmgr_garbage_destroy(garbage);
}

// Different timeout for Data Templates and Options Templates
TEST_P(udp, differentTimeouts)
{
    fds_tmgr_set_udp_timeouts(tmgr, 10, 5);
    fds_tmgr_set_snapshot_timeout(tmgr, 30);

    // Set export time and add Data and Options Templates
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 0), FDS_OK);
    const uint16_t tid1 = 256;
    const uint16_t tid2 = 257;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_FLOW, tid1)), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_FKEY, tid2)), FDS_OK);
    // All templates should be available
    const struct fds_template *tmplt2check;
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);

    // Set new export time (the Options Template has expired)
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 6), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_ERR_NOTFOUND);

    // Set new export time (the Options Template has expired)
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 11), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_ERR_NOTFOUND);
}

// Enable UDP timeout later, earlier templates should be unaffected until they are refreshed
TEST_P(udp, enableTimeoutLater)
{
    // Make sure that UDP timeouts are disabled
    fds_tmgr_set_udp_timeouts(tmgr, 0, 0);
    fds_tmgr_set_snapshot_timeout(tmgr, 300);

    // Create templates
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 0), FDS_OK);
    const uint16_t tid1 = 256;
    const uint16_t tid2 = 257;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_FLOW, tid1)), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_FKEY, tid2)), FDS_OK);

    // Change export time (templates should be still available)
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 100), FDS_OK);
    const struct fds_template *tmplt2check;
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);

    // Enable timeout and define new templates
    fds_tmgr_set_udp_timeouts(tmgr, 20, 20);
    const uint16_t tid3 = 258;
    const uint16_t tid4 = 259;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_FLOW, tid3)), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_FKEY, tid4)), FDS_OK);

    // Change export time and check that OLD templates remain and the new ones have expired
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 125), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid4, &tmplt2check), FDS_ERR_NOTFOUND);

    // Redefine/refresh old templates
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_FLOW, tid1)), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_ERPOC_RSTAT, tid2)), FDS_OK);

    // Change export time and check that no templates are available
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 150), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid4, &tmplt2check), FDS_ERR_NOTFOUND);
}

// Disable UDP timeout later, earlier templates should be unaffected and expire
TEST_P(udp, disableTimeoutLater)
{
    // Enable timeouts
    fds_tmgr_set_udp_timeouts(tmgr, 20, 20);
    fds_tmgr_set_snapshot_timeout(tmgr, 300);

    // Create templates
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 0), FDS_OK);
    const uint16_t tid1 = 256;
    const uint16_t tid2 = 257;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_FLOW, tid1)), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_FKEY, tid2)), FDS_OK);

    // Disable timeouts
    fds_tmgr_set_udp_timeouts(tmgr, 0, 0);
    const uint16_t tid3 = 258;
    const uint16_t tid4 = 259;
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::DATA_BASIC_FLOW, tid3)), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, TMock::create(TMock::type::OPTS_FKEY, tid4)), FDS_OK);

    // Set export time and check availability
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 50), FDS_OK);
    const struct fds_template *tmplt2check = nullptr;
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt2check), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt2check), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt2check), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid4, &tmplt2check), FDS_OK);
}

// TODO: Test combination of snapshot timeouts and template timeouts (+ on demand snapshot)

