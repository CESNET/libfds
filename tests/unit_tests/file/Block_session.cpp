
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <gtest/gtest.h>
#include <libfds.h>

#include "../../../src/file/Block_session.hpp"
#include "../../../src/file/Block_templates.hpp"
#include "../../../src/file/File_base.hpp"
#include "../../../src/file/File_exception.hpp"

using namespace fds_file;

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// Unique pointer type(s)
using tmpfile_t = std::unique_ptr<FILE, decltype(&fclose)>;

// Simple function for generation of a temporary file that is automatically destroyed
static tmpfile_t
create_temp() {
    return std::move(tmpfile_t(tmpfile(), &fclose));
}

// Create a dummy Transport Session (all parameters set to 0) and check getters
TEST(Session, createAndDestroy)
{
    struct fds_file_session dummy;
    memset(&dummy, 0, sizeof dummy);

    uint16_t sid = 236;
    Block_session session(sid, &dummy);

    const fds_file_session &ref = session.get_struct();
    EXPECT_EQ(session.get_sid(), sid);
    EXPECT_EQ(ref.port_dst, dummy.port_dst);
    EXPECT_EQ(ref.port_src, dummy.port_src);
    EXPECT_EQ(ref.proto, dummy.proto);
    EXPECT_EQ(memcmp(ref.ip_src, dummy.ip_src, sizeof ref.ip_src), 0);
    EXPECT_EQ(memcmp(ref.ip_dst, dummy.ip_dst, sizeof ref.ip_dst), 0);
    EXPECT_TRUE(session == dummy);
    EXPECT_TRUE(dummy == session);
}

// Store and load Transport Session as a Session Block
TEST(Session, storeAndLoad)
{
    auto ip_list = {
        std::make_pair("::FFFF:0102:0304", "::FFFF:C0A8:0001"), // IPv4 mapped addresses
        std::make_pair("fe80::fea9:7ac4:2f18:cab3", "::1")
    };

    auto proto_list = {
        FDS_FILE_SESSION_UNKNOWN,
        FDS_FILE_SESSION_UDP,
        FDS_FILE_SESSION_TCP,
        FDS_FILE_SESSION_SCTP
    };

    // For each combination of IP addresses
    for (const auto &addrs: ip_list) {
        SCOPED_TRACE(addrs.first + std::string(" -> ") + addrs.second);
        // For each combination of protocol types
        for (const auto stype : proto_list) {
            SCOPED_TRACE("stype: " + std::to_string(stype));

            // Create and fill a Data Structure
            struct fds_file_session session_orig;
            session_orig.proto = stype;
            ASSERT_EQ(inet_pton(AF_INET6, addrs.first, session_orig.ip_src), 1);
            ASSERT_EQ(inet_pton(AF_INET6, addrs.second, session_orig.ip_dst), 1);
            session_orig.port_src = 123;
            session_orig.port_dst = 10002;

            // Create a Session
            uint16_t sid = 1;
            Block_session session_writer(sid, &session_orig);

            // Check it before storing to a file
            const fds_file_session &ref_writer = session_writer.get_struct();
            EXPECT_EQ(session_writer.get_sid(), sid);
            EXPECT_EQ(ref_writer.port_dst, session_orig.port_dst);
            EXPECT_EQ(ref_writer.port_src, session_orig.port_src);
            EXPECT_EQ(ref_writer.proto, session_orig.proto);
            EXPECT_EQ(memcmp(ref_writer.ip_src, session_orig.ip_src, sizeof ref_writer.ip_src), 0);
            EXPECT_EQ(memcmp(ref_writer.ip_dst, session_orig.ip_dst, sizeof ref_writer.ip_dst), 0);
            EXPECT_TRUE(session_writer == session_orig);
            EXPECT_TRUE(session_orig == session_writer);

            // Store it to the file
            tmpfile_t file = create_temp();
            int file_fd = fileno(file.get());
            off_t offset = 128;
            uint64_t wsize = session_writer.write_to_file(file_fd, offset);
            EXPECT_GT(wsize, 0U);

            // Try to read it and check parameters
            Block_session session_reader1(file_fd, offset);
            const fds_file_session &ref_reader1 = session_reader1.get_struct();
            EXPECT_EQ(session_reader1.get_sid(), sid);
            EXPECT_EQ(ref_reader1.port_dst, session_orig.port_dst);
            EXPECT_EQ(ref_reader1.port_src, session_orig.port_src);
            EXPECT_EQ(ref_reader1.proto, session_orig.proto);
            EXPECT_EQ(memcmp(ref_reader1.ip_src, session_orig.ip_src, sizeof ref_reader1.ip_src), 0);
            EXPECT_EQ(memcmp(ref_reader1.ip_dst, session_orig.ip_dst, sizeof ref_reader1.ip_dst), 0);
            EXPECT_TRUE(session_reader1 == session_orig);
            EXPECT_TRUE(session_orig == session_reader1);

            // Try to load it using "load" function (and overwrite dummy structure)
            struct fds_file_session dummy;
            memset(&dummy, 0, sizeof dummy);
            Block_session session_reader2(0, &dummy);
            uint64_t rwise = session_reader2.load_from_file(file_fd, offset);
            EXPECT_EQ(rwise, wsize);
            // Check parameters
            const fds_file_session &ref_reader2 = session_reader2.get_struct();
            EXPECT_EQ(session_reader2.get_sid(), sid);
            EXPECT_EQ(ref_reader2.port_dst, session_orig.port_dst);
            EXPECT_EQ(ref_reader2.port_src, session_orig.port_src);
            EXPECT_EQ(ref_reader2.proto, session_orig.proto);
            EXPECT_EQ(memcmp(ref_reader2.ip_src, session_orig.ip_src, sizeof ref_reader2.ip_src), 0);
            EXPECT_EQ(memcmp(ref_reader2.ip_dst, session_orig.ip_dst, sizeof ref_reader2.ip_dst), 0);
            EXPECT_TRUE(session_reader2 == session_orig);
            EXPECT_TRUE(session_orig == session_reader2);
        }
    }
}

// Try to load a Session Block from an empty file
TEST(Session, readEmpty)
{
    tmpfile_t file = create_temp();
    int file_fd = fileno(file.get());

    EXPECT_THROW(Block_session session(file_fd, 0), File_exception);
}

// Try to read a Template block as a Session block
TEST(Session, readTemplateBlockAsSessionBlock)
{
    tmpfile_t file = create_temp();
    int file_fd = fileno(file.get());

    // Create a Template Block
    Block_templates tmgr;
    uint64_t wsize = tmgr.write_to_file(file_fd, 0, 0, 0);
    ASSERT_GT(wsize, 0U);

    // Try to read it
    EXPECT_THROW(Block_session session_constr(file_fd, 0), File_exception);
}

// Try to load incomplete Data Block
TEST(Session, tooShort)
{
    tmpfile_t file = create_temp();
    int file_fd = fileno(file.get());

    // Create a Session block
    struct fds_file_session session_orig;
    memset(&session_orig, 0, sizeof session_orig);
    Block_session session_writer(123, &session_orig);
    uint64_t wsize = session_writer.write_to_file(file_fd, 0);

    // Truncate the file
    ASSERT_EQ(ftruncate(file_fd, wsize - 1), 0);

    // Try to load the Data Block
    EXPECT_THROW(Block_session session_reader(file_fd, 0), File_exception);
    EXPECT_THROW(session_writer.load_from_file(file_fd, 0), File_exception);
}
