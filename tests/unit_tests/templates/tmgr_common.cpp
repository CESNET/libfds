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
    /** \brief Prepare IE DB */
    void SetUp() override {
        tmgr = fds_tmgr_create(GetParam());
        if (!tmgr) {
            throw std::runtime_error("Failed to create a template manager!");
        }
    }

    /** \brief Destroy IE DB */
    void TearDown() override {
        fds_tmgr_destroy(tmgr);
    }
};

// Define parameters of parametrized test
INSTANTIATE_TEST_CASE_P(TemplateManager, Common,
    ::testing::Values(FDS_SESSION_TYPE_UDP, FDS_SESSION_TYPE_TCP, FDS_SESSION_TYPE_SCTP,
        FDS_SESSION_TYPE_IPFIX_FILE));





class TCP : public ::testing::Test {
protected:
    fds_tmgr_t *tmgr = nullptr;
    /** \brief Prepare IE DB */
    void SetUp() override {
        tmgr = fds_tmgr_create(FDS_SESSION_TYPE_TCP);
        if (!tmgr) {
            throw std::runtime_error("Failed to create a template manager!");
        }
    }

    /** \brief Destroy IE DB */
    void TearDown() override {
        fds_tmgr_destroy(tmgr);
    }
};



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
    struct fds_template *t256 = TMock::create(TMock::type::NORM_BASIC_FLOW, 256);
    // Set current time and add the template
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 10), FDS_OK);
    EXPECT_EQ(fds_tmgr_template_add(tmgr, t256), FDS_OK);

    // Get the template
    const struct fds_template *result;
    ASSERT_EQ(fds_tmgr_template_get(tmgr, 256, &result), FDS_OK);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->id, 256);
}


