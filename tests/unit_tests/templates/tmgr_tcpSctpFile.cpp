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
class tcpSctpFile : public ::testing::TestWithParam<enum fds_session_type> {
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
INSTANTIATE_TEST_CASE_P(TemplateManager, tcpSctpFile,
    ::testing::Values(FDS_SESSION_TYPE_TCP, FDS_SESSION_TYPE_SCTP, FDS_SESSION_TYPE_IPFIX_FILE));

// Test template withdrawal mechanism
TEST_P(tcpSctpFile, withdrawal)
{
    // Create templates and add templates
    const uint16_t tid1 = 256;
    const uint16_t tid2 = 257;
    const uint16_t tid3 = 258;
    struct fds_template *t1 = TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid1);
    struct fds_template *t2 = TMock::create(TMock::type::OPTS_MPROC_RSTAT, tid2);
    struct fds_template *t3 = TMock::create(TMock::type::OPTS_FKEY, tid3);

    const uint32_t time1 = 10;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time1), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t1), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t2), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t3), FDS_OK);

    // Immediately withdraw template T1
    EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid1, FDS_TYPE_TEMPLATE), FDS_OK);

    // Set new export time and check availability of templates
    const uint32_t time2 = 20;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time2), FDS_OK);

    const struct fds_tsnapshot *snap = nullptr;
    const struct fds_template *tmplt = nullptr;
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt), FDS_OK);
    EXPECT_EQ(tmplt->id, tid2);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt), FDS_OK);
    EXPECT_EQ(tmplt->id, tid3);
    ASSERT_EQ(fds_tmgr_snapshot_get(tmgr, &snap), FDS_OK);

    // Withdraw template T2
    EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid2, FDS_TYPE_TEMPLATE_UNDEF), FDS_OK);

    // Set new export time and check availability of templates
    const uint32_t time3 = 30;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time3), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid1, &tmplt), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid2, &tmplt), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid3, &tmplt), FDS_OK);
    EXPECT_EQ(tmplt->id, tid3);

    // Snapshot should be unchanged and template T2 should be available
    tmplt = fds_tsnapshot_template_get(snap, tid1);
    EXPECT_EQ(tmplt, nullptr);
    tmplt = fds_tsnapshot_template_get(snap, tid2);
    EXPECT_EQ(tmplt->id, tid2);
    tmplt = fds_tsnapshot_template_get(snap, tid3);
    EXPECT_EQ(tmplt->id, tid3);
}

// Try to withdraw a template with a different type then expected
TEST_P(tcpSctpFile, withdrawInvalidType)
{
    const struct fds_template *tmplt = nullptr;

    // Set new export time
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 123456), FDS_OK);

    const uint16_t tid_data = 256;
    const uint16_t tid_opts1 = 12345;
    const uint16_t tid_opts2 = 45520;

    // Add few templates
    struct fds_template *t_data = TMock::create(TMock::type::DATA_BASIC_FLOW, tid_data);
    struct fds_template *t_opts1 = TMock::create(TMock::type::OPTS_MPROC_STAT, tid_opts1);
    struct fds_template *t_opts2 = TMock::create(TMock::type::OPTS_FKEY, tid_opts2);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t_data), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t_opts1), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t_opts2), FDS_OK);

    // Try to immediately withdraw T_OPTS1
    EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid_opts1, FDS_TYPE_TEMPLATE), FDS_ERR_ARG);
    // The template should be still available
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid_opts1, &tmplt), FDS_OK);
    // Try to withdraw template T_DATA, but it should remain in the manager
    EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid_data, FDS_TYPE_TEMPLATE_OPTS), FDS_ERR_ARG);

    // Set new export time, remove another template and check availability
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 23456789), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, tid_opts2, FDS_TYPE_TEMPLATE), FDS_ERR_ARG);

    // Remove old garbage...
    fds_tgarbage_t *garbage = nullptr;
    EXPECT_EQ(fds_tmgr_garbage_get(tmgr, &garbage), FDS_OK);
    if (garbage != nullptr) {
        fds_tmgr_garbage_destroy(garbage);
    }

    // The templates should be still available
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid_data, &tmplt), FDS_OK);
    EXPECT_EQ(tmplt->type, FDS_TYPE_TEMPLATE);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid_opts1, &tmplt), FDS_OK);
    EXPECT_EQ(tmplt->type, FDS_TYPE_TEMPLATE_OPTS);
    EXPECT_TRUE((tmplt->opts_types & FDS_OPTS_MPROC_STAT) != 0);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid_opts2, &tmplt), FDS_OK);
    EXPECT_EQ(tmplt->type, FDS_TYPE_TEMPLATE_OPTS);
    EXPECT_TRUE((tmplt->opts_types & FDS_OPTS_FKEYS) != 0);
}

// Try to withdraw undefined template
TEST_P(tcpSctpFile, withdrawUndefined)
{
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 128), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, 1000, FDS_TYPE_TEMPLATE_UNDEF), FDS_ERR_NOTFOUND);

    // Try to add at least one template
    struct fds_template *tmplt = TMock::create(TMock::type::DATA_BASIC_BIFLOW, 5000);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, tmplt), FDS_OK);

    // Withdraw undefined template again
    EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, 1000, FDS_TYPE_TEMPLATE_UNDEF), FDS_ERR_NOTFOUND);
}

// Withdraw all templates
TEST_P(tcpSctpFile, withdrawAll)
{
    const struct fds_template *tmplt = nullptr;
    const struct fds_tsnapshot *snap = nullptr;

    // Set new export time
    const uint32_t time1 = 1516872285UL;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time1), FDS_OK);

    // Add few templates
    const uint16_t tid_data1 = 1000;
    const uint16_t tid_opts1 = 2000;
    struct fds_template *t_data1 = TMock::create(TMock::type::DATA_BASIC_FLOW, tid_data1);
    struct fds_template *t_opts1 = TMock::create(TMock::type::OPTS_MPROC_STAT, tid_opts1);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t_data1), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t_opts1), FDS_OK);

    // Try to withdraw all options template NOW
    EXPECT_EQ(fds_tmgr_template_withdraw_all(tmgr, FDS_TYPE_TEMPLATE_OPTS), FDS_OK);

    // Set new export time and check availability of templates
    const uint32_t time2 = time1 + 3600;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time2), FDS_OK);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid_data1, &tmplt), FDS_OK);
    EXPECT_EQ(tmplt->id, tid_data1);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid_opts1, &tmplt), FDS_ERR_NOTFOUND);

    // Add a new template and create a snapshot
    const uint16_t tid_opts2 = 2002;
    struct fds_template *t_opts2 = TMock::create(TMock::type::OPTS_ERPOC_RSTAT, tid_opts2);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t_opts2), FDS_OK);
    EXPECT_EQ(fds_tmgr_snapshot_get(tmgr, &snap), FDS_OK);

    // Set new export time and remove all data templates
    const uint32_t time3 = time2 + 100;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time3), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_withdraw_all(tmgr, FDS_TYPE_TEMPLATE), FDS_OK);

    // Check template availability (only OPT2 should be present)
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid_data1, &tmplt), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid_opts1, &tmplt), FDS_ERR_NOTFOUND);
    ASSERT_EQ(fds_tmgr_template_get(tmgr, tid_opts2, &tmplt), FDS_OK);
    EXPECT_EQ(tmplt->id, tid_opts2);

    // Withdraw all templates (both types)
    EXPECT_EQ(fds_tmgr_template_withdraw_all(tmgr, FDS_TYPE_TEMPLATE_UNDEF), FDS_OK);
    // Add a new data template
    const uint16_t tid_data2 = 1001;
    struct fds_template *t_data2 = TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid_data2);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t_data2), FDS_OK);

    // Set new export time and check availability of templates (only DATA2 is accessible)
    const uint32_t time4 = time3 + 7200;
    EXPECT_EQ(fds_tmgr_set_time(tmgr, time4), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid_data1, &tmplt), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid_data2, &tmplt), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid_opts1, &tmplt), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_tmgr_template_get(tmgr, tid_opts2, &tmplt), FDS_ERR_NOTFOUND);

    // Snapshot should have valid references to DATA1 and OPTS2
    tmplt = fds_tsnapshot_template_get(snap, tid_data1);
    ASSERT_NE(tmplt, nullptr);
    EXPECT_EQ(tmplt->id, tid_data1);
    tmplt = fds_tsnapshot_template_get(snap, tid_opts2);
    ASSERT_NE(tmplt, nullptr);
    EXPECT_EQ(tmplt->id, tid_opts2);

    EXPECT_EQ(fds_tsnapshot_template_get(snap, tid_opts1), nullptr);
    EXPECT_EQ(fds_tsnapshot_template_get(snap, tid_data2), nullptr);
}

// Try to withdraw all templates from empty manager
TEST_P(tcpSctpFile, withdrawAllEmpty)
{
    // Undefined time context
    EXPECT_EQ(fds_tmgr_template_withdraw_all(tmgr, FDS_TYPE_TEMPLATE), FDS_ERR_ARG);
    EXPECT_EQ(fds_tmgr_template_withdraw_all(tmgr, FDS_TYPE_TEMPLATE_OPTS), FDS_ERR_ARG);
    EXPECT_EQ(fds_tmgr_template_withdraw_all(tmgr, FDS_TYPE_TEMPLATE_UNDEF), FDS_ERR_ARG);

    // Set new export time
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 100000), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_withdraw_all(tmgr, FDS_TYPE_TEMPLATE), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_withdraw_all(tmgr, FDS_TYPE_TEMPLATE_OPTS), FDS_OK);
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 100001), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_withdraw_all(tmgr, FDS_TYPE_TEMPLATE_UNDEF), FDS_OK);
}


