/**
 * \author Michal Režňák
 * \date   8. August 2017
 */
#pragma once

/** Folder with valid files hierarchy */
#define FILES_VALID   "test_files/valid/"

/** Folder with invalid files hierarchy */
#define FILES_INVALID "test_files/invalid/"

/** Error message when no error occurred*/
#define ERR_MSG "No error"

/** Check if NO error occurred */
#define EXPECT_NO_ERROR EXPECT_STREQ(fds_iemgr_last_err(mgr), ERR_MSG)

/** Check if error occurred */
#define EXPECT_ERROR    EXPECT_STRNE(fds_iemgr_last_err(mgr), ERR_MSG)

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

/**
 * \brief Only manager is created
 */
class Mgr: public ::testing::Test
{
protected:
    fds_iemgr_t* mgr = nullptr;
    void SetUp() override {
        mgr = fds_iemgr_create();
    }

    void TearDown() override {
        fds_iemgr_destroy(mgr);
    }
};

/**
 * \brief Manager is created and filled with individual scope
 */
class Fill: public ::testing::Test
{
protected:
    fds_iemgr_t* mgr = nullptr;
    void SetUp() override {
        mgr = fds_iemgr_create();
        fds_iemgr_read_file(mgr, FILES_VALID "individual.xml", true);
    }

    void TearDown() override {
        fds_iemgr_destroy(mgr);
    }
};

/**
 * \brief Manager is created and filled with individual scope and aliases
 */
class FillAndAlias: public ::testing::Test
{
protected:
    fds_iemgr_t* mgr = nullptr;
    void SetUp() override {
        mgr = fds_iemgr_create();
        fds_iemgr_read_file(mgr, FILES_VALID "individual.xml", true);
        fds_iemgr_alias_read_file(mgr, FILES_VALID "aliases.xml");
    }

    void TearDown() override {
        fds_iemgr_destroy(mgr);
    }
};
