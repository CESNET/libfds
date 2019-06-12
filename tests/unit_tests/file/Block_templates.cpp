
#include <gtest/gtest.h>
#include <libfds.h>
#include <TMock.h>

#include "../../../src/file/Block_templates.hpp"
#include "../../../src/file/File_exception.hpp"

using namespace fds_file;

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// Unique pointer types
using tmpfile_t = std::unique_ptr<FILE, decltype(&fclose)>;
using iemgr_t = std::unique_ptr<fds_iemgr_t, decltype(&fds_iemgr_destroy)>;
using tmplt_t = std::unique_ptr<fds_template, decltype(&fds_template_destroy)>;

// Simple function for generation of a temporary file that is automatically destroyed
static tmpfile_t
create_temp() {
    return std::move(tmpfile_t(tmpfile(), &fclose));
}

// Simple function for create a non-empty IE manager
static iemgr_t
create_iemgr() {
    static const char *ie_path = "data/iana.xml";
    iemgr_t mgr(fds_iemgr_create(), &fds_iemgr_destroy);
    if (fds_iemgr_read_file(mgr.get(), ie_path, true) != FDS_OK) {
        throw std::runtime_error("Failed to load IE manager");
    }
    return std::move(mgr);
}

// Create and destroy the Template Manager
TEST(Templates, createAndDestroy)
{
    Block_templates tmgr;
}

// Get number of templates in an empty Template manager
TEST(Templates, emptyCounter)
{
    Block_templates tmgr;
    EXPECT_EQ(tmgr.count(), 0);
}

// Get a template snapshot of an empty Template manager
TEST(Templates, emptySnapshot)
{
    Block_templates tmgr;
    auto tsnap = tmgr.snapshot();
    ASSERT_NE(tsnap, nullptr);
    EXPECT_EQ(fds_tsnapshot_template_get(tsnap, 256), nullptr);
}

// Set IE manager to an empty Template Manager
TEST(Templates, setIEManager)
{
    auto iemgr = create_iemgr();

    Block_templates tmgr;
    tmgr.ie_source(iemgr.get());
}

// Try to get a missing IPFIX Template
TEST(Templates, getAMissingTemplate)
{
    auto iemgr = create_iemgr();

    Block_templates tmgr;
    EXPECT_EQ(tmgr.get(256), nullptr);
    EXPECT_EQ(tmgr.get(10000), nullptr);

    tmgr.ie_source(iemgr.get());
    EXPECT_EQ(tmgr.get(256), nullptr);
    EXPECT_EQ(tmgr.get(10000), nullptr);
}

// Try to remove a missing IPFIX Template
TEST(Templates, removeAMissingTemplate)
{
    auto iemgr = create_iemgr();

    Block_templates tmgr;
    EXPECT_THROW(tmgr.remove(256), File_exception);
    EXPECT_THROW(tmgr.remove(22222), File_exception);
    tmgr.ie_source(iemgr.get());
    EXPECT_THROW(tmgr.remove(256), File_exception);
    EXPECT_THROW(tmgr.remove(22222), File_exception);
}

// Try to add IPFIX Templates (Normal and Options)
TEST(Templates, addTemplates)
{
    // Create an IE manager and auxiliary templates
    auto iemgr = create_iemgr();

    uint16_t tid1 = 300;
    uint16_t tid2 = 12345;
    auto tmplt1 = tmplt_t(TMock::create(TMock::type::DATA_BASIC_FLOW, tid1), &fds_template_destroy);
    auto tmplt2 = tmplt_t(TMock::create(TMock::type::OPTS_MPROC_RSTAT, tid2), &fds_template_destroy);

    Block_templates tmgr;
    tmgr.ie_source(iemgr.get());
    EXPECT_EQ(tmgr.get(tid1), nullptr);
    EXPECT_EQ(tmgr.get(tid2), nullptr);
    EXPECT_EQ(tmgr.count(), 0U);

    // Add the first Template
    tmgr.add(FDS_TYPE_TEMPLATE, tmplt1->raw.data, tmplt1->raw.length);
    const struct fds_template *tptr = tmgr.get(tid1);
    ASSERT_NE(tptr, nullptr);
    EXPECT_EQ(tptr->id, tid1);
    EXPECT_EQ(tptr->raw.length, tmplt1->raw.length);
    EXPECT_EQ(memcmp(tptr->raw.data, tmplt1->raw.data, tmplt1->raw.length), 0);
    EXPECT_NE(tptr->raw.data, tmplt1->raw.data);
    // The seconds template must not be present
    EXPECT_EQ(tmgr.get(tid2), nullptr);
    EXPECT_EQ(tmgr.count(), 1U);

    // Add the second Template
    tmgr.add(FDS_TYPE_TEMPLATE_OPTS, tmplt2->raw.data, tmplt2->raw.length);
    tptr = tmgr.get(tid2);
    ASSERT_NE(tptr, nullptr);
    EXPECT_EQ(tptr->id, tid2);
    EXPECT_EQ(tptr->raw.length, tmplt2->raw.length);
    EXPECT_EQ(memcmp(tptr->raw.data, tmplt2->raw.data, tmplt2->raw.length), 0);
    EXPECT_NE(tptr->raw.data, tmplt2->raw.data);
    // The first template must be also available
    tptr = tmgr.get(tid1);
    ASSERT_NE(tptr, nullptr);
    EXPECT_EQ(tptr->id, tid1);
    EXPECT_EQ(tmgr.count(), 2U);

    // Create a snapshot and find these Templates
    auto snapshot = tmgr.snapshot();
    ASSERT_NE(snapshot, nullptr);
    tptr = fds_tsnapshot_template_get(snapshot, tid1);
    ASSERT_NE(tptr, nullptr);
    EXPECT_EQ(tptr->id, tid1);
    tptr = fds_tsnapshot_template_get(snapshot, tid2);
    ASSERT_NE(tptr, nullptr);
    EXPECT_EQ(tptr->id, tid2);

    // Clear
    tmgr.clear();
    EXPECT_EQ(tmgr.count(), 0U);
    EXPECT_EQ(tmgr.get(tid1), nullptr);
    EXPECT_EQ(tmgr.get(tid2), nullptr);
}

// Try to add a Template withdrawal
TEST(Templates, addWithdrawal)
{
    // Create an IE manager and auxiliary templates
    auto iemgr = create_iemgr();
    uint16_t tid = 256;
    Block_templates tmgr;
    tmgr.ie_source(iemgr.get());

    auto tmplt_w = tmplt_t(TMock::create(TMock::type::DATA_WITHDRAWAL, tid), &fds_template_destroy);
    auto tmplt_def = tmplt_t(TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid), &fds_template_destroy);

    // Try to add a Template withdrawal
    EXPECT_THROW(tmgr.add(FDS_TYPE_TEMPLATE, tmplt_w->raw.data, tmplt_w->raw.length), File_exception);
    const struct fds_template *tptr = tmgr.get(tid);
    EXPECT_EQ(tptr, nullptr);

    // Try to add a Template definition with the same Template ID
    tmgr.add(FDS_TYPE_TEMPLATE, tmplt_def->raw.data, tmplt_def->raw.length);
    tptr = tmgr.get(tid);
    ASSERT_NE(tptr, nullptr);
    EXPECT_EQ(memcmp(tptr->raw.data, tmplt_def->raw.data, tmplt_def->raw.length), 0);
}

// Try to redefine and remove a Template
TEST(Templates, redefineAndRemove)
{
    // Create an IE manager and auxiliary templates
    auto iemgr = create_iemgr();
    uint16_t tid = 256;
    Block_templates tmgr;
    tmgr.ie_source(iemgr.get());

    auto tmplt_basic = tmplt_t(TMock::create(TMock::type::DATA_BASIC_FLOW, tid), &fds_template_destroy);
    auto tmplt_biflow = tmplt_t(TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid), &fds_template_destroy);

    // Add the basic IPFIX Template
    tmgr.add(FDS_TYPE_TEMPLATE, tmplt_basic->raw.data, tmplt_basic->raw.length);
    EXPECT_NE(tmgr.get(tid), nullptr);
    EXPECT_EQ(tmgr.count(), 1U);

    // Replace it with the biflow IPFIX Template
    tmgr.add(FDS_TYPE_TEMPLATE, tmplt_biflow->raw.data, tmplt_biflow->raw.length);
    EXPECT_EQ(tmgr.count(), 1U);
    const struct fds_template *tptr = tmgr.get(tid);
    ASSERT_NE(tptr, nullptr);
    EXPECT_EQ(memcmp(tptr->raw.data, tmplt_biflow->raw.data, tmplt_biflow->raw.length), 0);

    // Remove the Template
    tmgr.remove(tid);
    EXPECT_EQ(tmgr.count(), 0U);
    EXPECT_EQ(tmgr.get(tid), nullptr);
}

// Try to add "undefined" type of Template
TEST(Templates, addUndefinedTemplateType)
{
    uint16_t tid = 15534;
    auto tmplt = tmplt_t(TMock::create(TMock::type::OPTS_FKEY, tid), &fds_template_destroy);

    Block_templates tmgr;
    EXPECT_THROW(tmgr.add(FDS_TYPE_TEMPLATE_UNDEF, tmplt->raw.data, tmplt->raw.length), File_exception);
    EXPECT_EQ(tmgr.count(), 0U);
    EXPECT_EQ(tmgr.get(tid), nullptr);
}

// Try to add malformed Templates
TEST(Templates, addMalforemdTemplates)
{
    // Create an IE manager and auxiliary templates
    auto iemgr = create_iemgr();
    uint16_t tid = 260;

    Block_templates tmgr;
    tmgr.ie_source(iemgr.get());

    // Create a shorted IPFIX Templates
    auto tmplt1 = tmplt_t(TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid), &fds_template_destroy);
    for (uint16_t i = tmplt1->raw.length - 1; i > 0; --i) {
        SCOPED_TRACE("Size: " + std::to_string(i));
        EXPECT_THROW(tmgr.add(FDS_TYPE_TEMPLATE, tmplt1->raw.data, i), File_exception);
    }
    EXPECT_EQ(tmgr.count(), 0U);

    // Create a longer IPFIX Options Template
    auto tmplt2 = tmplt_t(TMock::create(TMock::type::OPTS_MPROC_STAT, tid), &fds_template_destroy);
    uint16_t new_size = tmplt2->raw.length + 16U; // Try to prolong the definition with garbage
    std::unique_ptr<uint8_t[]> new_mem(new uint8_t[new_size]);
    memset(new_mem.get(), 0xFF, new_size);
    memcpy(new_mem.get(), tmplt2->raw.data, tmplt2->raw.length);

    for (uint16_t i = tmplt2->raw.length + 1; i <= new_size; ++i) {
        SCOPED_TRACE("Size: " + std::to_string(i));
        EXPECT_THROW(tmgr.add(FDS_TYPE_TEMPLATE_OPTS, new_mem.get(), i), File_exception);
    }
    EXPECT_EQ(tmgr.count(), 0U);

    // Create a Template and change its Template ID to invalid value
    auto tmplt3 = tmplt_t(TMock::create(TMock::type::DATA_BASIC_FLOW, tid), &fds_template_destroy);
    const uint16_t tmplt3_size = tmplt3->raw.length;
    std::unique_ptr<uint8_t[]> tmplt3_cpy(new uint8_t[tmplt3_size]);
    memcpy(tmplt3_cpy.get(), tmplt3->raw.data, tmplt3_size);

    for (uint16_t tid_inv : {0, 1, 2, 50, 100, 157, 213, 255}) { // All IDs below 256 are invalid
        auto data_ptr = reinterpret_cast<fds_ipfix_trec *>(tmplt3_cpy.get());
        data_ptr->template_id = htons(tid_inv);
        EXPECT_THROW(tmgr.add(FDS_TYPE_TEMPLATE, tmplt3_cpy.get(), tmplt3_size), File_exception);
    }
}

// Check if a Template contains references to the IE definitions
TEST(Templates, templateIEReferences)
{
    const struct fds_template *tptr;
    const struct fds_tfield *tfield;

    auto iemgr = create_iemgr();
    uint16_t tid = 55556;
    auto tmplt = tmplt_t(TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid), &fds_template_destroy);
    const uint16_t ie_id_src_ipv4 = 8;

    // Add a template (IEs should be undefined)
    Block_templates tmgr;
    tmgr.add(FDS_TYPE_TEMPLATE, tmplt->raw.data, tmplt->raw.length);
    tptr = tmgr.get(tid);
    ASSERT_NE(tptr, nullptr);
    tfield = fds_template_cfind(tptr, 0, ie_id_src_ipv4);
    ASSERT_NE(tfield, nullptr);
    EXPECT_EQ(tfield->def, nullptr);

    // Define IE source
    tmgr.ie_source(iemgr.get());
    tptr = tmgr.get(tid);
    ASSERT_NE(tptr, nullptr);
    tfield = fds_template_cfind(tptr, 0, ie_id_src_ipv4);
    ASSERT_NE(tfield, nullptr);
    ASSERT_NE(tfield->def, nullptr);
    EXPECT_EQ(tfield->def->id, ie_id_src_ipv4);
    EXPECT_EQ(tfield->def->data_type, FDS_ET_IPV4_ADDRESS);

    // Remove the the IE manager
    tmgr.ie_source(nullptr);
    tptr = tmgr.get(tid);
    ASSERT_NE(tptr, nullptr);
    tfield = fds_template_cfind(tptr, 0, ie_id_src_ipv4);
    ASSERT_NE(tfield, nullptr);
    EXPECT_EQ(tfield->def, nullptr);
}

// Check if a Template in a Template Snapshot contains reference to the IE definitions
TEST(Templates, snapshotIEReference)
{
    const fds_tsnapshot_t *tsnap;
    const struct fds_template *tptr;
    const struct fds_tfield *tfield;

    auto iemgr = create_iemgr();
    const uint16_t ie_id_bytes = 1;
    const uint16_t ie_id_nsf_cnt = 166;

    // Create a Template manager with 2 Templates and create a snapshot
    uint16_t tid1 = 65535;
    uint16_t tid2 = 256;
    auto tmplt1 = tmplt_t(TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid1), &fds_template_destroy);
    auto tmplt2 = tmplt_t(TMock::create(TMock::type::OPTS_ERPOC_RSTAT, tid2), &fds_template_destroy);

    Block_templates tmgr;
    tmgr.ie_source(iemgr.get());
    tmgr.add(FDS_TYPE_TEMPLATE, tmplt1->raw.data, tmplt1->raw.length);
    tmgr.add(FDS_TYPE_TEMPLATE_OPTS, tmplt2->raw.data, tmplt2->raw.length);
    EXPECT_EQ(tmgr.count(), 2U);

    tsnap = tmgr.snapshot();
    ASSERT_NE(tsnap, nullptr);

    // Check if IE references are available
    tptr = fds_tsnapshot_template_get(tsnap, tid1);
    ASSERT_NE(tptr, nullptr);
    tfield = fds_template_cfind(tptr, 0, ie_id_bytes);
    ASSERT_NE(tfield, nullptr);
    ASSERT_NE(tfield->def, nullptr);
    EXPECT_EQ(tfield->def->id, ie_id_bytes);

    tptr = fds_tsnapshot_template_get(tsnap, tid2);
    ASSERT_NE(tptr, nullptr);
    tfield = fds_template_cfind(tptr, 0, ie_id_nsf_cnt);
    ASSERT_NE(tfield, nullptr);
    ASSERT_NE(tfield->def, nullptr);
    EXPECT_EQ(tfield->def->id, ie_id_nsf_cnt);
}

// Write an empty Template manager and try to read it
TEST(Templates, writeEmpty)
{
    // Create a temporary file
    auto file_ptr = create_temp();
    int fd = fileno(file_ptr.get());

    // Common parameters
    off_t offset = 31; // Odd offset is intentional
    uint16_t sid = 10, sid_parsed;
    uint32_t odid = 303030, odid_parsed;

    // Write an empty Template Block to a file
    Block_templates writer;
    uint64_t written = writer.write_to_file(fd, offset, sid, odid);
    ASSERT_GT(written, 0U);

    // Read an empty Template Block from a file
    Block_templates reader;
    uint64_t read = reader.load_from_file(fd, offset, &sid_parsed, &odid_parsed);
    EXPECT_EQ(read, written);
    EXPECT_EQ(reader.count(), 0U);
    EXPECT_EQ(sid_parsed, sid);
    EXPECT_EQ(odid_parsed, odid);
}

// Write a Template Block with 2 (Options) Templates and load them later
TEST(Templates, writeAndReadTemplates)
{
    // Create a temporary files and load IE definitions
    auto file_ptr = create_temp();
    int fd = fileno(file_ptr.get());
    auto iemgr = create_iemgr();

    // Common parameters
    off_t offset = 0;
    uint16_t sid = 6546, sid_parsed;
    uint32_t odid = 10, odid_parsed;

    uint16_t tid1 = 56312;
    uint16_t tid2 = 12555;
    auto tmplt1 = tmplt_t(TMock::create(TMock::type::DATA_BASIC_FLOW, tid1), &fds_template_destroy);
    auto tmplt2 = tmplt_t(TMock::create(TMock::type::OPTS_MPROC_STAT, tid2), &fds_template_destroy);

    // Create a Template manager, add IPFIX (Options) Templates and write the TManager into a file
    Block_templates tmgr_writer;
    tmgr_writer.add(FDS_TYPE_TEMPLATE, tmplt1->raw.data, tmplt1->raw.length);
    tmgr_writer.add(FDS_TYPE_TEMPLATE_OPTS, tmplt2->raw.data, tmplt2->raw.length);
    uint64_t written = tmgr_writer.write_to_file(fd, offset, sid, odid);
    EXPECT_GT(written, 0U);

    // Load the Template Block from the file
    Block_templates tmgr_reader;
    tmgr_reader.ie_source(iemgr.get());
    uint64_t read = tmgr_reader.load_from_file(fd, offset, &sid_parsed, &odid_parsed);
    EXPECT_EQ(read, written);
    EXPECT_EQ(tmgr_reader.count(), 2U);
    EXPECT_EQ(sid_parsed, sid);
    EXPECT_EQ(odid_parsed, odid);

    // Try to get the (Options) Templates and check if IE definitions are available
    const struct fds_template *tptr;
    const struct fds_tfield *tfield;
    const uint16_t ie_id_bytes = 1;
    const uint16_t ie_id_exp_bytes = 40;

    tptr = tmgr_reader.get(tid1);
    ASSERT_NE(tptr, nullptr);
    tfield = fds_template_cfind(tptr, 0, ie_id_bytes);
    ASSERT_NE(tfield, nullptr);
    ASSERT_NE(tfield->def, nullptr);
    EXPECT_EQ(tfield->def->id, ie_id_bytes);
    EXPECT_TRUE(fds_iemgr_is_type_unsigned(tfield->def->data_type));

    tptr = tmgr_reader.get(tid2);
    ASSERT_NE(tptr, nullptr);
    tfield = fds_template_cfind(tptr, 0, ie_id_exp_bytes);
    ASSERT_NE(tfield, nullptr);
    ASSERT_NE(tfield->def, nullptr);
    EXPECT_EQ(tfield->def->id, ie_id_exp_bytes);
    EXPECT_TRUE(fds_iemgr_is_type_unsigned(tfield->def->data_type));
}

// With a single Template manager try to write Templates to a file and load them later
TEST(Templates, writeAndReadSingleManager)
{
    // Create a temporary files
    auto file_ptr = create_temp();
    int fd = fileno(file_ptr.get());

    // Common parameters
    off_t offset = 64;
    uint16_t sid = 0, sid_parsed;
    uint32_t odid = 0, odid_parsed;

    uint16_t tid1 = 56312;
    auto tmplt1 = tmplt_t(TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid1), &fds_template_destroy);

    // Create a manager, add a template and write a Template Block to the file
    Block_templates tmgr;
    tmgr.add(FDS_TYPE_TEMPLATE, tmplt1->raw.data, tmplt1->raw.length);
    uint64_t written = tmgr.write_to_file(fd, offset, sid, odid);
    EXPECT_GT(written, 0U);
    EXPECT_EQ(tmgr.count(), 1U);

    // Remove all Templates from the manager
    tmgr.clear();
    EXPECT_EQ(tmgr.count(), 0U);
    EXPECT_EQ(tmgr.get(tid1), nullptr);

    // Add a new Template
    uint16_t tid2 = 2567;
    auto tmplt2 = tmplt_t(TMock::create(TMock::type::DATA_BASIC_FLOW, tid2), &fds_template_destroy);
    tmgr.add(FDS_TYPE_TEMPLATE, tmplt2->raw.data, tmplt2->raw.length);
    EXPECT_EQ(tmgr.count(), 1U);
    EXPECT_NE(tmgr.get(tid2), nullptr);

    // Load the Template Block from the file (it should clear the Template manager)
    uint64_t read = tmgr.load_from_file(fd, offset, &sid_parsed, &odid_parsed);
    EXPECT_EQ(read, written);
    EXPECT_EQ(tmgr.count(), 1U);
    EXPECT_EQ(sid_parsed, sid);
    EXPECT_EQ(odid_parsed, odid);

    // Only the Template from the Block should be available
    ASSERT_NE(tmgr.get(tid1), nullptr);
    EXPECT_EQ(tmgr.get(tid2), nullptr);

    auto tptr = tmgr.get(tid1);
    EXPECT_EQ(tptr->id, tmplt1->id);
    EXPECT_EQ(memcmp(tptr->raw.data, tmplt1->raw.data, tmplt1->raw.length), 0);
}

// Try to write and read a Template Block with maximum possible number of Templates
TEST(Templates, writeMaxTemplates)
{
    // Create a temporary files
    auto file_ptr = create_temp();
    int fd = fileno(file_ptr.get());
    auto iemgr = create_iemgr();

    // Create auxiliary Templates for modification
    uint16_t tid1 = 256;
    uint16_t tid2 = 257;
    auto tmplt1 = tmplt_t(TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid1), &fds_template_destroy);
    auto tmplt2 = tmplt_t(TMock::create(TMock::type::OPTS_MPROC_RSTAT, tid2), &fds_template_destroy);
    uint16_t tmplt1_size = tmplt1->raw.length;
    uint16_t tmplt2_size = tmplt2->raw.length;
    std::unique_ptr<uint8_t[]> tmplt1_cpy(new uint8_t[tmplt1_size]);
    std::unique_ptr<uint8_t[]> tmplt2_cpy(new uint8_t[tmplt2_size]);
    memcpy(tmplt1_cpy.get(), tmplt1->raw.data, tmplt1_size);
    memcpy(tmplt2_cpy.get(), tmplt2->raw.data, tmplt2_size);
    auto tmplt1_struct = reinterpret_cast<struct fds_ipfix_trec *>(tmplt1_cpy.get());
    auto tmplt2_struct = reinterpret_cast<struct fds_ipfix_opts_trec *>(tmplt2_cpy.get());

    Block_templates tmgr_writer;
    for (uint32_t tid = FDS_IPFIX_SET_MIN_DSET; tid <= UINT16_MAX; ++tid) {
        // If TID is even, add the first Template type, otherwise add the second type
        SCOPED_TRACE("TID: " + std::to_string(tid));

        if (tid % 2 == 0) {
            tmplt1_struct->template_id = htons(tid);
            tmgr_writer.add(FDS_TYPE_TEMPLATE, tmplt1_cpy.get(), tmplt1_size);
        } else {
            tmplt2_struct->template_id = htons(tid);
            tmgr_writer.add(FDS_TYPE_TEMPLATE_OPTS, tmplt2_cpy.get(), tmplt2_size);
        }
    }

    // The manager should be full and all templates should be available
    EXPECT_EQ(tmgr_writer.count(), UINT16_MAX - FDS_IPFIX_SET_MIN_DSET + 1);
    for (uint32_t tid = FDS_IPFIX_SET_MIN_DSET; tid <= UINT16_MAX; ++tid) {
        SCOPED_TRACE("TID: " + std::to_string(tid));
        ASSERT_NE(tmgr_writer.get(tid), nullptr);
    }

    // Write all Templates to a Template Block
    off_t offset = 60;
    uint16_t sid = 14789;
    uint32_t odid = 125464678U, odid_parsed;
    uint64_t wsize = tmgr_writer.write_to_file(fd, offset, sid, odid);

    // Try load the Template Block
    Block_templates tmgr_reader;
    uint64_t rsize = tmgr_reader.load_from_file(fd, offset, nullptr, &odid_parsed);
    EXPECT_EQ(rsize, wsize);
    EXPECT_EQ(tmgr_reader.count(), UINT16_MAX - FDS_IPFIX_SET_MIN_DSET + 1);
    EXPECT_EQ(odid_parsed, odid);

    // Try to compare all Templates
    const fds_tsnapshot_t *tsnap = tmgr_reader.snapshot();
    for (uint32_t tid = FDS_IPFIX_SET_MIN_DSET; tid <= UINT16_MAX; ++tid) {
        SCOPED_TRACE("TID: " + std::to_string(tid));
        const struct fds_template *tptr = fds_tsnapshot_template_get(tsnap, tid);
        ASSERT_NE(tptr, nullptr);

        if (tid % 2 == 0) {
            tmplt1_struct->template_id = htons(tid);
            ASSERT_EQ(memcmp(tmplt1_cpy.get(), tptr->raw.data, tptr->raw.length), 0);
            ASSERT_EQ(tmplt1_size, tptr->raw.length);
        } else {
            tmplt2_struct->template_id = htons(tid);
            ASSERT_EQ(memcmp(tmplt2_cpy.get(), tptr->raw.data, tptr->raw.length), 0);
            ASSERT_EQ(tmplt2_size, tptr->raw.length);
        }
    }
}

// Try to read the Block from an empty file
TEST(Templates, readEmptyFile)
{
    // Create a temporary files
    auto file_ptr = create_temp();
    int fd = fileno(file_ptr.get());

    uint16_t sid;
    uint32_t odid;

    Block_templates tmgr;
    EXPECT_THROW(tmgr.load_from_file(fd, 0, &sid, &odid), File_exception);
    EXPECT_THROW(tmgr.load_from_file(fd, 128, &sid, &odid), File_exception);
}

// Try to read an incomplete Template Block
TEST(Templates, readTooShortBlock)
{
    // Create a temporary files
    auto file_ptr = create_temp();
    int fd = fileno(file_ptr.get());

    // Write a simple Table Block into a file
    Block_templates tmgr_writer;
    uint16_t tid = 257;
    auto tmplt = tmplt_t(TMock::create(TMock::type::DATA_BASIC_BIFLOW, tid), &fds_template_destroy);
    tmgr_writer.add(FDS_TYPE_TEMPLATE, tmplt->raw.data, tmplt->raw.length);
    uint64_t wsize = tmgr_writer.write_to_file(fd, 0, 0, 0);

    // Truncate the block
    ASSERT_EQ(ftruncate(fd, wsize - 1), 0);

    // Try to read the block
    Block_templates tmgr_reader;
    EXPECT_THROW(tmgr_reader.load_from_file(fd, 0, nullptr, nullptr), File_exception);
}

