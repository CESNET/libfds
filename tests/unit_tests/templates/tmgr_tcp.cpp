#include <gtest/gtest.h>
#include <libfds.h>
#include <TGenerator.h>
#include <TMock.h>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

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
TEST_F(TCP, createAndDestroy)
{
    ASSERT_NE(tmgr, nullptr);
}

// Try to add and remove simple template
TEST_F(TCP, addAndWithdraw)
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

    // Remove the template
    EXPECT_EQ(fds_tmgr_template_withdraw(tmgr, 256, FDS_TYPE_TEMPLATE_UNDEF), FDS_OK);
    const struct fds_template *aux = nullptr;
    EXPECT_EQ(fds_tmgr_template_get(tmgr, 256, &aux), FDS_ERR_NOTFOUND);
    EXPECT_EQ(aux, nullptr);

    // The template should still be available
    EXPECT_EQ(result->id, 256);
}


