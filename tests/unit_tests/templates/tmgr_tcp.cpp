/**
 * \brief Test cases only for TCP sessions
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
class tcp : public ::testing::TestWithParam<enum fds_session_type> {
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
INSTANTIATE_TEST_CASE_P(TemplateManager, tcp,
    ::testing::Values(FDS_SESSION_TCP));


// TODO: try to access history...
// TODO: try to configure history timeout and access it (should fail)

TEST_P(tcp, goEmptyHistory)
{
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 10), FDS_OK);
    EXPECT_EQ(fds_tmgr_set_time(tmgr, 0), FDS_ERR_DENIED);
}