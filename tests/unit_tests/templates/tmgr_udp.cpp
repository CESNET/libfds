/**
 * \brief Test cases only for TCP, SCTP and IPFIX FILE sessions
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
    ::testing::Values(FDS_SESSION_TYPE_UDP));

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
    //EXPECT_EQ(fds_tmgr_template_get())
}

// TODO: enable template timeout later... older should remain until they are refreshed/redefined
// TODO: disable template timeout later...
// TODO: create a snapshot on demand (due to template expiration)...
