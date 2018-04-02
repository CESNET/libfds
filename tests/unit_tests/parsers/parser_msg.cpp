#include <string>
#include <memory>
#include <cstdlib>
#include <gtest/gtest.h>
#include <libfds.h>
#include <MsgGen.h>

// Unique pointer able to handle IPFIX Set
using msg_uniq = std::unique_ptr<fds_ipfix_msg_hdr, decltype(&free)>;
using uint8_uniq = std::unique_ptr<uint8_t, decltype(&free)>;

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

static const std::string NO_ERR_STRING = "No error.";

// Empty message (no sets, only valid header)
TEST(setIter, msgHeaderOnly)
{
    ipfix_msg msg{};
    msg_uniq hdr_msg(msg.release(), &free);

    fds_sets_iter iter;
    fds_sets_iter_init(&iter, hdr_msg.get());
    EXPECT_EQ(fds_sets_iter_next(&iter), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_sets_iter_err(&iter), NO_ERR_STRING);
}

// Message with one Set (even unknown types)
TEST(setIter, singleSet)
{
    for (uint32_t set_id = 0; set_id <= 512; ++set_id) { // First 512 Set IDs should be enough
        ipfix_set set{static_cast<uint16_t>(set_id)};
        set.add_padding(100); // "random" data

        ipfix_msg msg{};
        msg.add_set(set);
        msg_uniq hdr_msg(msg.release(), &free);

        fds_sets_iter iter;
        fds_sets_iter_init(&iter, hdr_msg.get());
        // First Set
        EXPECT_EQ(fds_sets_iter_next(&iter), FDS_OK);
        uint16_t set_len = ntohs(iter.set->length);
        EXPECT_EQ(set_len, set.size());
        uint16_t flowset_id = ntohs(iter.set->flowset_id);
        EXPECT_EQ(flowset_id, set_id);
        // End
        EXPECT_EQ(fds_sets_iter_next(&iter), FDS_ERR_NOTFOUND);
        EXPECT_EQ(fds_sets_iter_err(&iter), NO_ERR_STRING);
    }
}

// Message with multiple Sets
TEST(setIter, multipleSets)
{
    // Create Sets and add "fake" content
    ipfix_set set_tmplt_norm {FDS_IPFIX_SET_TMPLT};
    set_tmplt_norm.add_padding(20);
    ipfix_set set_tmplt_opts {FDS_IPFIX_SET_OPTS_TMPLT};
    set_tmplt_opts.add_padding(66);
    ipfix_set set_data1      {FDS_IPFIX_SET_MIN_DSET};
    set_data1.add_padding(1021);
    ipfix_set set_data2      {FDS_IPFIX_SET_MIN_DSET + 1};
    set_data2.add_padding(531);

    ipfix_msg msg;
    msg.add_set(set_tmplt_norm);
    msg.add_set(set_tmplt_opts);
    msg.add_set(set_data1);
    msg.add_set(set_data2);
    msg_uniq hdr_msg(msg.release(), &free);

    fds_sets_iter iter;
    fds_sets_iter_init(&iter, hdr_msg.get());
    // 1. Set
    EXPECT_EQ(fds_sets_iter_next(&iter), FDS_OK);
    uint16_t set1_len = ntohs(iter.set->length);
    uint16_t set1_id  = ntohs(iter.set->flowset_id);
    EXPECT_EQ(set1_len, set_tmplt_norm.size());
    EXPECT_EQ(set1_id, FDS_IPFIX_SET_TMPLT);
    // 2. Set
    EXPECT_EQ(fds_sets_iter_next(&iter), FDS_OK);
    uint16_t set2_len = ntohs(iter.set->length);
    uint16_t set2_id  = ntohs(iter.set->flowset_id);
    EXPECT_EQ(set2_len, set_tmplt_opts.size());
    EXPECT_EQ(set2_id, FDS_IPFIX_SET_OPTS_TMPLT);
    // 3. Set
    EXPECT_EQ(fds_sets_iter_next(&iter), FDS_OK);
    uint16_t set3_len = ntohs(iter.set->length);
    uint16_t set3_id  = ntohs(iter.set->flowset_id);
    EXPECT_EQ(set3_len, set_data1.size());
    EXPECT_EQ(set3_id, FDS_IPFIX_SET_MIN_DSET);
    // 4. Set
    EXPECT_EQ(fds_sets_iter_next(&iter), FDS_OK);
    uint16_t set4_len = ntohs(iter.set->length);
    uint16_t set4_id  = ntohs(iter.set->flowset_id);
    EXPECT_EQ(set4_len, set_data2.size());
    EXPECT_EQ(set4_id, FDS_IPFIX_SET_MIN_DSET + 1);
    // End
    EXPECT_EQ(fds_sets_iter_next(&iter), FDS_ERR_NOTFOUND);
    EXPECT_EQ(fds_sets_iter_err(&iter), NO_ERR_STRING);
}

// Malformed messages -----------------------------------------------------------------------------

// Set behind the end of the Message
TEST(setIterMalformed, setExceedsMsg)
{
    ipfix_set set{256};
    set.add_padding(100); // "random" data

    ipfix_msg msg{};
    msg.add_set(set);
    msg.set_len(msg.size() - 1); // Make the message shorter (the Set will exceed the message)
    msg_uniq hdr_msg(msg.release(), &free);

    fds_sets_iter iter;
    fds_sets_iter_init(&iter, hdr_msg.get());
    EXPECT_EQ(fds_sets_iter_next(&iter), FDS_ERR_FORMAT);
    EXPECT_NE(fds_sets_iter_err(&iter), NO_ERR_STRING);
}

// Length of a Set is shorter than an IPFIX Set header.
TEST(setIterMalformed, shortSetHeader)
{
    ipfix_set set{256};
    set.add_padding(3); // "random" data
    set.overwrite_len(FDS_IPFIX_SET_HDR_LEN - 1); // Shorted that valid header

    ipfix_msg msg{};
    msg.add_set(set);
    msg_uniq hdr_msg(msg.release(), &free);

    fds_sets_iter iter;
    fds_sets_iter_init(&iter, hdr_msg.get());
    EXPECT_EQ(fds_sets_iter_next(&iter), FDS_ERR_FORMAT);
    EXPECT_NE(fds_sets_iter_err(&iter), NO_ERR_STRING);
}

// Padding after the last set is not allowed
TEST(setIterMalformed, paddingAfterLastSet)
{
    ipfix_set set{256};
    set.add_padding(100); // "random" data

    ipfix_msg msg{};
    msg.add_set(set);
    msg.add_set(set);
    // Max. fake padding
    uint16_t max_padding = FDS_IPFIX_SET_HDR_LEN - 1; // i.e. less than IPFIX Set header size
    msg.set_len(msg.size() - set.size() + max_padding);
    msg_uniq hdr_msg(msg.release(), &free);

    fds_sets_iter iter;
    fds_sets_iter_init(&iter, hdr_msg.get());
    // 1. Set
    EXPECT_EQ(fds_sets_iter_next(&iter), FDS_OK);
    // "Fake" padding
    EXPECT_EQ(fds_sets_iter_next(&iter), FDS_ERR_FORMAT);
    EXPECT_NE(fds_sets_iter_err(&iter), NO_ERR_STRING);
}
