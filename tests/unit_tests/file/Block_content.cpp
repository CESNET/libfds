#include <unistd.h>
#include <sys/types.h>

#include <gtest/gtest.h>
#include <libfds.h>
#include <utility>

#include "../../../src/file/Block_content.hpp"
#include "../../../src/file/Block_templates.hpp"
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

// Try to create and destroy class instance immediately
TEST(BContent, createAndDestroy)
{
    Block_content block();
}

// Try to write and read empty Content Table
TEST(BContent, writeAndReadEmpty)
{
    tmpfile_t file = create_temp();
    int file_fd = fileno(file.get());

    Block_content content_writer;
    size_t wsize = content_writer.write_to_file(file_fd, 0);
    EXPECT_GT(wsize, 0U);

    Block_content content_reader;
    size_t rsize = content_reader.load_from_file(file_fd, 0);
    EXPECT_GT(rsize, 0U);

    EXPECT_EQ(wsize, rsize);
}

// Try to write and read Content Table with only Sessions records
TEST(BContent, writeAndReadSessions)
{
    const auto list = {
        std::make_tuple(165UL, 10UL, 0),    std::make_tuple(UINT64_MAX, UINT64_MAX, UINT16_MAX),
        std::make_tuple(10UL, 500UL, 6547), std::make_tuple(5464987UL, 654UL, 567)
    };

    // Try to insert different number of sessions
    for (size_t cnt = 1; cnt < list.size(); ++cnt) {
        SCOPED_TRACE("cnt: " + std::to_string(cnt));
        tmpfile_t file = create_temp();
        int file_fd = fileno(file.get());

        // Insert records to the table
        Block_content content_writer;
        auto it = list.begin();
        for (size_t i = 0; i < cnt; ++i) {
            content_writer.add_session(std::get<0>(*it), std::get<1>(*it), std::get<2>(*it));
            it++;
        }

        EXPECT_EQ(content_writer.get_sessions().size(), cnt);
        EXPECT_EQ(content_writer.get_data_blocks().size(), 0U);

        // Write to the file
        size_t wsize = content_writer.write_to_file(file_fd, 0);
        EXPECT_GT(wsize, 0U);

        // Try to load it from the file
        Block_content content_reader;
        EXPECT_EQ(content_reader.get_sessions().size(), 0U);
        EXPECT_EQ(content_reader.get_data_blocks().size(), 0U);
        size_t rsize = content_reader.load_from_file(file_fd, 0);
        EXPECT_EQ(rsize, wsize);
        ASSERT_EQ(content_writer.get_sessions().size(), cnt);
        EXPECT_EQ(content_writer.get_data_blocks().size(), 0U);

        // Check if the values match
        const auto &records = content_reader.get_sessions();
        it = list.begin();
        for (size_t i = 0; i < cnt; ++i) {
            SCOPED_TRACE("i: " + std::to_string(i));
            const struct Block_content::info_session &session = records[i];
            EXPECT_EQ(session.offset, std::get<0>(*it));
            EXPECT_EQ(session.len, std::get<1>(*it));
            EXPECT_EQ(session.session_id, std::get<2>(*it));
            it++;
        }
    }
}

// Try to write and read Content Table with only Data Block records
TEST(BContent, writeAndReadDataBlocks)
{
    // Assert note: Data Block offset (1. value) must be behind its Template Block offset (3.value)
    const auto list = {
        std::make_tuple(100UL, 21324UL, 50UL, 32U, 0U),
        std::make_tuple(UINT64_MAX, UINT64_MAX, UINT64_MAX - 1, UINT32_MAX, static_cast<unsigned int>(UINT16_MAX)),
        std::make_tuple(1234567UL, 879745UL, 100UL, 12154U, 21U),
        std::make_tuple(987981234UL, 49879UL, 54657UL, 87987154U, 5654U)
    };

    // Try to insert different number of sessions
    for (size_t cnt = 1; cnt < list.size(); ++cnt) {
        SCOPED_TRACE("cnt: " + std::to_string(cnt));
        tmpfile_t file = create_temp();
        int file_fd = fileno(file.get());

        // Insert records to the table
        Block_content content_writer;
        auto it = list.begin();
        for (size_t i = 0; i < cnt; ++i) {
            content_writer.add_data_block(std::get<0>(*it), std::get<1>(*it), std::get<2>(*it),
                std::get<3>(*it), std::get<4>(*it));
            it++;
        }

        EXPECT_EQ(content_writer.get_sessions().size(), 0U);
        EXPECT_EQ(content_writer.get_data_blocks().size(), cnt);

        // Write to the file
        size_t wsize = content_writer.write_to_file(file_fd, 0);
        EXPECT_GT(wsize, 0U);

        // Try to load it from the file
        Block_content content_reader;
        EXPECT_EQ(content_reader.get_sessions().size(), 0U);
        EXPECT_EQ(content_reader.get_data_blocks().size(), 0U);
        size_t rsize = content_reader.load_from_file(file_fd, 0);
        EXPECT_EQ(rsize, wsize);
        EXPECT_EQ(content_writer.get_sessions().size(), 0U);
        ASSERT_EQ(content_writer.get_data_blocks().size(), cnt);

        // Check if the values match
        const auto &records = content_reader.get_data_blocks();
        it = list.begin();
        for (size_t i = 0; i < cnt; ++i) {
            SCOPED_TRACE("i: " + std::to_string(i));
            const struct Block_content::info_data_block &dblock = records[i];
            EXPECT_EQ(dblock.offset, std::get<0>(*it));
            EXPECT_EQ(dblock.len, std::get<1>(*it));
            EXPECT_EQ(dblock.tmplt_offset, std::get<2>(*it));
            EXPECT_EQ(dblock.odid, std::get<3>(*it));
            EXPECT_EQ(dblock.session_id, std::get<4>(*it));
            it++;
        }
    }
}

// Try to write and read Content Table with all types of records
TEST(BContent, writeAndReadAllBlocks)
{
    const auto list_sessions = {
        std::make_tuple(165UL, 10UL, 0),    std::make_tuple(UINT64_MAX, UINT64_MAX, UINT16_MAX),
        std::make_tuple(10UL, 500UL, 6547), std::make_tuple(5464987UL, 654UL, 567)
    };
    const auto list_dblocks = {
        std::make_tuple(100UL, 21324UL, 50UL, 32U, 0U),
        std::make_tuple(UINT64_MAX, UINT64_MAX, UINT64_MAX - 1, UINT32_MAX, static_cast<unsigned int>(UINT16_MAX)),
        std::make_tuple(1234567UL, 879745UL, 100UL, 12154U, 21U),
        std::make_tuple(987981234UL, 49879UL, 54657UL, 87987154U, 5654U)
    };
    const auto test_cases = {
        std::make_tuple(1,1), std::make_tuple(2,2), std::make_tuple(3,3), std::make_tuple(4,4),
        std::make_tuple(1,2), std::make_tuple(2,4), std::make_tuple(3,1), std::make_tuple(4,2),
        std::make_tuple(1,4), std::make_tuple(2,1), std::make_tuple(3,4), std::make_tuple(4,3),
    };

    // For each test combination
    for (const auto &tcase : test_cases) {
        const size_t cnt_sessions = std::get<0>(tcase);
        const size_t cnt_dblocks = std::get<1>(tcase);
        SCOPED_TRACE("Sessions: " + std::to_string(cnt_sessions));
        SCOPED_TRACE("DBlocks: " + std::to_string(cnt_dblocks));

        tmpfile_t file = create_temp();
        int file_fd = fileno(file.get());

        // Insert records to the table
        Block_content content_writer;
        auto it_sessions = list_sessions.begin();
        auto it_dblocks = list_dblocks.begin();

        for (size_t i = 0; i < cnt_dblocks; ++i) {
            content_writer.add_data_block(std::get<0>(*it_dblocks), std::get<1>(*it_dblocks),
                std::get<2>(*it_dblocks), std::get<3>(*it_dblocks), std::get<4>(*it_dblocks));
            it_dblocks++;
        }
        for (size_t i = 0; i < cnt_sessions; ++i) {
            content_writer.add_session(std::get<0>(*it_sessions), std::get<1>(*it_sessions),
                std::get<2>(*it_sessions));
            it_sessions++;
        }

        // Write to the file
        size_t wsize = content_writer.write_to_file(file_fd, 0);
        EXPECT_GT(wsize, 0U);

        // Try to load it from the file
        Block_content content_reader;
        size_t rsize = content_reader.load_from_file(file_fd, 0);
        EXPECT_EQ(rsize, wsize);
        ASSERT_EQ(content_writer.get_sessions().size(), cnt_sessions);
        ASSERT_EQ(content_writer.get_data_blocks().size(), cnt_dblocks);

        // Check if the values match
        const auto &dblock_records = content_reader.get_data_blocks();
        it_dblocks = list_dblocks.begin();
        for (size_t i = 0; i < cnt_dblocks; ++i) {
            SCOPED_TRACE("i: " + std::to_string(i));
            const struct Block_content::info_data_block &dblock = dblock_records[i];
            EXPECT_EQ(dblock.offset, std::get<0>(*it_dblocks));
            EXPECT_EQ(dblock.len, std::get<1>(*it_dblocks));
            EXPECT_EQ(dblock.tmplt_offset, std::get<2>(*it_dblocks));
            EXPECT_EQ(dblock.odid, std::get<3>(*it_dblocks));
            EXPECT_EQ(dblock.session_id, std::get<4>(*it_dblocks));
            it_dblocks++;
        }

        const auto &session_records = content_reader.get_sessions();
        it_sessions = list_sessions.begin();
        for (size_t i = 0; i < cnt_sessions; ++i) {
            SCOPED_TRACE("i: " + std::to_string(i));
            const struct Block_content::info_session &session = session_records[i];
            EXPECT_EQ(session.offset, std::get<0>(*it_sessions));
            EXPECT_EQ(session.len, std::get<1>(*it_sessions));
            EXPECT_EQ(session.session_id, std::get<2>(*it_sessions));
            it_sessions++;
        }
    }
}

// Try to load Template Block as Content Table
TEST(BContent, tryToLoadTemplateBlock)
{
    tmpfile_t file = create_temp();
    int file_fd = fileno(file.get());

    Block_templates tmptls;
    ASSERT_GT(tmptls.write_to_file(file_fd, 0, 0, 0), 0U);

    Block_content content;
    EXPECT_THROW(content.load_from_file(file_fd, 0), File_exception);
}

// Try to load Content Table from an empty file
TEST(BContent, loadFromEmptyFile)
{
    tmpfile_t file = create_temp();
    int file_fd = fileno(file.get());

    Block_content content;
    EXPECT_THROW(content.load_from_file(file_fd, 0), File_exception);
}

// Try to load trimmed Content Table from a file
TEST(BContent, shortBlock)
{
    tmpfile_t file = create_temp();
    int file_fd = fileno(file.get());

    Block_content content_writer;
    content_writer.add_session(123U, 567U, 1U); // dummy record
    size_t wsize = content_writer.write_to_file(file_fd, 0);
    ASSERT_GT(wsize, 0U);

    // Truncate the file
    ASSERT_EQ(ftruncate(file_fd, wsize - 1), 0);

    Block_content content_loader;
    EXPECT_THROW(content_loader.load_from_file(file_fd, 0), File_exception);
}
