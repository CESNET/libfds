/**
 * \brief Test cases only for UDP and IPFIX FILE sessions
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
class udpFile : public ::testing::TestWithParam<enum fds_session_type> {
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
INSTANTIATE_TEST_CASE_P(TemplateManager, udpFile,
    ::testing::Values(FDS_SESSION_TYPE_UDP, FDS_SESSION_TYPE_IPFIX_FILE));

TEST_P(udpFile, basic)
{
    // Do nothing
}
