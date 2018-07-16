#include <string>
#include <memory>
#include <cstdlib>
#include <gtest/gtest.h>
#include <libfds.h>
#include <MsgGen.h>

// Unique pointer able to handle IPFIX Set
using set_uniq = std::unique_ptr<fds_ipfix_set_hdr, decltype(&free)>;

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

static const std::string NO_ERR_STRING = "No error.";

// One Template definition
TEST(tsetIter, oneTemplate)
{
    // Prepare a simple template
    ipfix_trec rec {FDS_IPFIX_SET_MIN_DSET};
    rec.add_field(1, 4);      // bytes
    rec.add_field(2, 4);      // packets
    rec.add_field(1, 20, 10); // random enterprise IE
    rec.add_field(27, 16);    // source IPv6 address
    rec.add_field(82, ipfix_trec::SIZE_VAR);    // interface name
    rec.add_field(2, ipfix_trec::SIZE_VAR, 10); // another random enterprise IE

    // Prepare a Template Set
    ipfix_set tset {FDS_IPFIX_SET_TMPLT};
    tset.add_rec(rec);
    set_uniq hdr_set(tset.release(), &free);

    // Try to initialize iterator
    struct fds_tset_iter iter;
    fds_tset_iter_init(&iter, hdr_set.get());

    ASSERT_EQ(fds_tset_iter_next(&iter), FDS_OK);
    EXPECT_EQ(iter.scope_cnt, 0);
    EXPECT_EQ(iter.field_cnt, 6);
    // Size: 4 bytes Template header + 6x IEs (x 4B) + 2x Enterprise Extensions (x 4B)
    EXPECT_EQ(iter.size, 4 + 24 + 8);
    // Position of the template should be right behind the Set header
    fds_ipfix_trec *tmplt_pos = reinterpret_cast<fds_ipfix_trec *>(hdr_set.get() + 1);
    EXPECT_EQ(iter.ptr.trec, tmplt_pos);

    // The end of the Set
    EXPECT_EQ(fds_tset_iter_next(&iter), FDS_EOC);
    EXPECT_EQ(fds_tset_iter_err(&iter), NO_ERR_STRING);
}

// One Options Template definition
TEST(tsetIter, oneOptionsTemplate)
{
    // Prepare a simple template
    ipfix_trec rec {FDS_IPFIX_SET_MIN_DSET, 2}; // 2 scope fields
    rec.add_field(149, 4); // observationDomainID
    rec.add_field(143, 4); // meteringProcessId
    rec.add_field(41, 8);  // exportedMessageTotalCount
    rec.add_field(42, 8);  // exportedFlowRecordTotalCount
    rec.add_field(40, 8);  // exportedOctetTotalCount

    // Prepare an Options Template Set
    ipfix_set tset {FDS_IPFIX_SET_OPTS_TMPLT};
    tset.add_rec(rec);
    set_uniq hdr_set(tset.release(), &free);

    // Try to initialize iterator
    struct fds_tset_iter iter;
    fds_tset_iter_init(&iter, hdr_set.get());

    ASSERT_EQ(fds_tset_iter_next(&iter), FDS_OK);
    EXPECT_EQ(iter.scope_cnt, 2);
    EXPECT_EQ(iter.field_cnt, 5);
    // Size: 6 bytes Options Template header + 5x IEs (x 4B)
    EXPECT_EQ(iter.size, 6U + 20U);
    // Position of the template should be right behind the Set header
    fds_ipfix_opts_trec *tmplt_pos = reinterpret_cast<fds_ipfix_opts_trec *>(hdr_set.get() + 1);
    EXPECT_EQ(iter.ptr.opts_trec, tmplt_pos);

    // The end of the Set
    EXPECT_EQ(fds_tset_iter_next(&iter), FDS_EOC);
    EXPECT_EQ(fds_tset_iter_err(&iter), NO_ERR_STRING);
}

// One Template withdrawal
TEST(tsetIter, oneWithdrawal)
{
    ipfix_trec rec {1000}; // Withdraw Template ID 1000
    ipfix_set wset {FDS_IPFIX_SET_TMPLT};
    wset.add_rec(rec);
    set_uniq hdr_set(wset.release(), &free);

    // Try to initialize iterator
    struct fds_tset_iter iter;
    fds_tset_iter_init(&iter, hdr_set.get());

    ASSERT_EQ(fds_tset_iter_next(&iter), FDS_OK);
    EXPECT_EQ(iter.scope_cnt, 0);
    EXPECT_EQ(iter.field_cnt, 0);
    // Size: 4 bytes Template Withdrawal header
    EXPECT_EQ(iter.size, 4U);
    // Position of the template should be right behind the Set header
    fds_ipfix_wdrl_trec *tmplt_pos = reinterpret_cast<fds_ipfix_wdrl_trec *>(hdr_set.get() + 1);
    EXPECT_EQ(iter.ptr.wdrl_trec, tmplt_pos);

    // The end of the Set
    EXPECT_EQ(fds_tset_iter_next(&iter), FDS_EOC);
    EXPECT_EQ(fds_tset_iter_err(&iter), NO_ERR_STRING);
}

// Multiple Templates definitions
TEST(tsetIter, multipleTemplates)
{
    fds_ipfix_trec *tmplt_pos, *next_pos;

    // Static size fields only
    ipfix_trec rec1 {256};
    rec1.add_field(1, 1);
    rec1.add_field(2, 2);
    // Dynamic size fields only
    ipfix_trec rec2 {257};
    rec2.add_field(3, ipfix_trec::SIZE_VAR);
    rec2.add_field(4, ipfix_trec::SIZE_VAR);
    // Enterprise fields only
    ipfix_trec rec3 {258};
    rec3.add_field(5, 8, 1);
    rec3.add_field(6, 2, 1);
    // Combination of above
    ipfix_trec rec4 {259};
    rec4.add_field(6, 2, 1);
    rec4.add_field(4, ipfix_trec::SIZE_VAR);
    rec4.add_field(2, 2);
    rec4.add_field(3, ipfix_trec::SIZE_VAR);
    rec4.add_field(5, 8, 1);
    rec4.add_field(1, 1);

    // Prepare a Template Set
    ipfix_set tset {FDS_IPFIX_SET_TMPLT};
    tset.add_rec(rec1);
    tset.add_rec(rec2);
    tset.add_rec(rec3);
    tset.add_rec(rec4);
    set_uniq hdr_set(tset.release(), &free);

    // Try to initialize iterator
    struct fds_tset_iter iter;
    fds_tset_iter_init(&iter, hdr_set.get());

    // The first record
    ASSERT_EQ(fds_tset_iter_next(&iter), FDS_OK);
    EXPECT_EQ(iter.scope_cnt, 0);
    EXPECT_EQ(iter.field_cnt, 2);
    // Size: 4 bytes Template header + 2x IEs (x 4B)
    EXPECT_EQ(iter.size, 4 + 8);
    // Position of the template should be right behind the Set header
    tmplt_pos = reinterpret_cast<fds_ipfix_trec *>(hdr_set.get() + 1);
    next_pos = (fds_ipfix_trec *)(((uint8_t *) tmplt_pos) + iter.size);
    EXPECT_EQ(iter.ptr.trec, tmplt_pos);

    // The second record
    ASSERT_EQ(fds_tset_iter_next(&iter), FDS_OK);
    EXPECT_EQ(iter.scope_cnt, 0);
    EXPECT_EQ(iter.field_cnt, 2);
    // Size: 4 bytes Template header + 2x IEs (x 4B)
    EXPECT_EQ(iter.size, 4 + 8);
    tmplt_pos = next_pos;
    next_pos = (fds_ipfix_trec *)(((uint8_t *) tmplt_pos) + iter.size);
    EXPECT_EQ(iter.ptr.trec, tmplt_pos);

    // The third record
    ASSERT_EQ(fds_tset_iter_next(&iter), FDS_OK);
    EXPECT_EQ(iter.scope_cnt, 0);
    EXPECT_EQ(iter.field_cnt, 2);
    // Size: 4 bytes Template header + 2x IEs (x 4B) + 2x Enterprise field (x4)
    EXPECT_EQ(iter.size, 4 + 8 + 8);
    tmplt_pos = next_pos;
    next_pos = (fds_ipfix_trec *)(((uint8_t *) tmplt_pos) + iter.size);
    EXPECT_EQ(iter.ptr.trec, tmplt_pos);

    // The last record
    ASSERT_EQ(fds_tset_iter_next(&iter), FDS_OK);
    EXPECT_EQ(iter.scope_cnt, 0);
    EXPECT_EQ(iter.field_cnt, 6);
    // Size: 4 bytes Template header + 6x IEs (x 4B) + 2x Enterprise field (x4)
    EXPECT_EQ(iter.size, 4 + 24 + 8);
    tmplt_pos = next_pos;
    EXPECT_EQ(iter.ptr.trec, tmplt_pos);

    // The end of the Set
    EXPECT_EQ(fds_tset_iter_next(&iter), FDS_EOC);
    EXPECT_EQ(fds_tset_iter_err(&iter), NO_ERR_STRING);
}

// Multiple Options Template definitions
TEST(tsetIter, multipleOptionsTemplates)
{
    fds_ipfix_trec *tmplt_pos, *next_pos;

    // Static size fields only
    ipfix_trec rec1 {10000, 2};
    rec1.add_field(1, 1);
    rec1.add_field(2, 2);
    // Dynamic size fields only
    ipfix_trec rec2 {10001, 1};
    rec2.add_field(3, ipfix_trec::SIZE_VAR);
    rec2.add_field(4, ipfix_trec::SIZE_VAR);
    // Enterprise fields only
    ipfix_trec rec3 {10002, 2};
    rec3.add_field(5, 8, 1);
    rec3.add_field(6, 2, 1);
    // Combination of above
    ipfix_trec rec4 {9999, 3};
    rec4.add_field(6, 2, 1);
    rec4.add_field(4, ipfix_trec::SIZE_VAR);
    rec4.add_field(2, 2);
    rec4.add_field(3, ipfix_trec::SIZE_VAR);
    rec4.add_field(5, 8, 1);
    rec4.add_field(1, 1);

    // Prepare a Template Set
    ipfix_set tset {FDS_IPFIX_SET_OPTS_TMPLT};
    tset.add_rec(rec1);
    tset.add_rec(rec2);
    tset.add_rec(rec3);
    tset.add_rec(rec4);
    set_uniq hdr_set(tset.release(), &free);

    // Try to initialize iterator
    struct fds_tset_iter iter;
    fds_tset_iter_init(&iter, hdr_set.get());

    // The first record
    ASSERT_EQ(fds_tset_iter_next(&iter), FDS_OK);
    EXPECT_EQ(iter.scope_cnt, 2);
    EXPECT_EQ(iter.field_cnt, 2);
    // Size: 6 bytes Options Template header + 2x IEs (x 4B)
    EXPECT_EQ(iter.size, 6 + 8);
    // Position of the template should be right behind the Set header
    tmplt_pos = reinterpret_cast<fds_ipfix_trec *>(hdr_set.get() + 1);
    next_pos = (fds_ipfix_trec *)(((uint8_t *) tmplt_pos) + iter.size);
    EXPECT_EQ(iter.ptr.trec, tmplt_pos);

    // The second record
    ASSERT_EQ(fds_tset_iter_next(&iter), FDS_OK);
    EXPECT_EQ(iter.scope_cnt, 1);
    EXPECT_EQ(iter.field_cnt, 2);
    // Size: 6 bytes Options Template header + 2x IEs (x 4B)
    EXPECT_EQ(iter.size, 6 + 8);
    tmplt_pos = next_pos;
    next_pos = (fds_ipfix_trec *)(((uint8_t *) tmplt_pos) + iter.size);
    EXPECT_EQ(iter.ptr.trec, tmplt_pos);

    // The third record
    ASSERT_EQ(fds_tset_iter_next(&iter), FDS_OK);
    EXPECT_EQ(iter.scope_cnt, 2);
    EXPECT_EQ(iter.field_cnt, 2);
    // Size: 6 bytes Options Template header + 2x IEs (x 4B) + 2x Enterprise field (x4)
    EXPECT_EQ(iter.size, 6 + 8 + 8);
    tmplt_pos = next_pos;
    next_pos = (fds_ipfix_trec *)(((uint8_t *) tmplt_pos) + iter.size);
    EXPECT_EQ(iter.ptr.trec, tmplt_pos);

    // The last record
    ASSERT_EQ(fds_tset_iter_next(&iter), FDS_OK);
    EXPECT_EQ(iter.scope_cnt, 3);
    EXPECT_EQ(iter.field_cnt, 6);
    // Size: 6 bytes Options Template header + 6x IEs (x 4B) + 2x Enterprise field (x4)
    EXPECT_EQ(iter.size, 6 + 24 + 8);
    tmplt_pos = next_pos;
    EXPECT_EQ(iter.ptr.trec, tmplt_pos);

    // The end of the Set
    EXPECT_EQ(fds_tset_iter_next(&iter), FDS_EOC);
    EXPECT_EQ(fds_tset_iter_err(&iter), NO_ERR_STRING);
}

TEST(tsetIter, multipleWithdrawals)
{
    ipfix_trec rec1 {256};
    ipfix_trec rec2 {1000};
    ipfix_trec rec3 {65535};
    ipfix_trec rec4 {15000};

    ipfix_set wset {FDS_IPFIX_SET_OPTS_TMPLT};
    wset.add_rec(rec1);
    wset.add_rec(rec2);
    wset.add_rec(rec3);
    wset.add_rec(rec4);
    set_uniq hdr_set(wset.release(), &free);

    // Try to initialize iterator
    struct fds_tset_iter iter;
    fds_tset_iter_init(&iter, hdr_set.get());

    fds_ipfix_wdrl_trec *tmplt_pos = reinterpret_cast<fds_ipfix_wdrl_trec *>(hdr_set.get() + 1);
    for (int i = 0; i < 4; ++i) {
        ASSERT_EQ(fds_tset_iter_next(&iter), FDS_OK);
        EXPECT_EQ(iter.field_cnt, 0);
        EXPECT_EQ(iter.scope_cnt, 0);
        EXPECT_EQ(iter.size, 4U);
        EXPECT_EQ(iter.ptr.wdrl_trec, tmplt_pos);

        tmplt_pos = (fds_ipfix_wdrl_trec *)(((uint8_t *) tmplt_pos) + iter.size);
    }

    // The end of the Set
    EXPECT_EQ(fds_tset_iter_next(&iter), FDS_EOC);
    EXPECT_EQ(fds_tset_iter_err(&iter), NO_ERR_STRING);
}

TEST(tsetIter, allTemplatesWithdrawal)
{
    ipfix_trec rec {FDS_IPFIX_SET_TMPLT};
    ipfix_set wset {FDS_IPFIX_SET_TMPLT};
    wset.add_rec(rec);

    set_uniq hdr_set(wset.release(), &free);

    // Try to initialize iterator
    struct fds_tset_iter iter;
    fds_tset_iter_init(&iter, hdr_set.get());

    ASSERT_EQ(fds_tset_iter_next(&iter), FDS_OK);
    EXPECT_EQ(iter.field_cnt, 0);
    EXPECT_EQ(iter.scope_cnt, 0);
    EXPECT_EQ(iter.size, 4U);
    fds_ipfix_wdrl_trec *tmplt_pos = reinterpret_cast<fds_ipfix_wdrl_trec *>(hdr_set.get() + 1);
    EXPECT_EQ(iter.ptr.wdrl_trec, tmplt_pos);

    // The end of the Set
    EXPECT_EQ(fds_tset_iter_next(&iter), FDS_EOC);
    EXPECT_EQ(fds_tset_iter_err(&iter), NO_ERR_STRING);
}

TEST(tsetIter, allOptionsTemplatesWithdrawal)
{
    ipfix_trec rec {FDS_IPFIX_SET_OPTS_TMPLT};
    ipfix_set wset {FDS_IPFIX_SET_OPTS_TMPLT};
    wset.add_rec(rec);
    set_uniq hdr_set(wset.release(), &free);

    // Try to initialize iterator
    struct fds_tset_iter iter;
    fds_tset_iter_init(&iter, hdr_set.get());

    ASSERT_EQ(fds_tset_iter_next(&iter), FDS_OK);
    EXPECT_EQ(iter.field_cnt, 0);
    EXPECT_EQ(iter.scope_cnt, 0);
    EXPECT_EQ(iter.size, 4U);
    fds_ipfix_wdrl_trec *tmplt_pos = reinterpret_cast<fds_ipfix_wdrl_trec *>(hdr_set.get() + 1);
    EXPECT_EQ(iter.ptr.wdrl_trec, tmplt_pos);

    // The end of the Set
    EXPECT_EQ(fds_tset_iter_next(&iter), FDS_EOC);
    EXPECT_EQ(fds_tset_iter_err(&iter), NO_ERR_STRING);
}

// Try maximum number of fields in a Template definition
TEST(tsetIter, maxTemplate)
{
    /*
     * SOURCE: https://tools.ietf.org/html/rfc5471#section-3.5.3
     *    The total length field in the IP header is 16 bits, allowing a length
     * up to 65535 octets in one application-level datagram.  This limits
     * the number of Information Elements one can specify in an IPFIX
     * Template when using UDP export.  SCTP and TCP are streaming
     * protocols, so they do not impose much restriction on the packet
     * level.  UDP requires 20 octets for a minimal IPv4 header, 8 octets
     * for the UDP header, 16 octets for the IPFIX header, 4 octets for the
     * Set header, and 4 octets for the Template header, so the Template
     * definition may be up to (65535 - 20 - 8 - 16 - 4 - 4) = 65483 octets
     * long.  The minimum IPFIX Information Element specification requires 4
     * octets: two for the Information Element ID and two for the field
     * length.  Therefore, the maximum number of IPFIX Information Elements
     * in a single Template is 65483 / 4 = 16370.  With this many
     * Information Elements, the Template will be 65480 octets long, while
     * the entire packet will be 65532 octets long.
     */

    const uint16_t REC_CNT = 16370;
    ipfix_trec rec {10000};
    for (uint16_t i = 0; i < REC_CNT; ++i) {
        rec.add_field(i, 2);
    }

    ipfix_set set {FDS_IPFIX_SET_TMPLT};
    set.add_rec(rec);
    set_uniq hdr_set(set.release(), &free);

    // Try to initialize iterator
    struct fds_tset_iter iter;
    fds_tset_iter_init(&iter, hdr_set.get());

    ASSERT_EQ(fds_tset_iter_next(&iter), FDS_OK);
    EXPECT_EQ(iter.size, 4 + 4 * REC_CNT); // header + IEs
    EXPECT_EQ(iter.scope_cnt, 0);
    EXPECT_EQ(iter.field_cnt, REC_CNT);
    EXPECT_EQ(iter.ptr.trec, reinterpret_cast<fds_ipfix_trec *>(hdr_set.get() + 1));

    // The end of the Set
    EXPECT_EQ(fds_tset_iter_next(&iter), FDS_EOC);
    EXPECT_EQ(fds_tset_iter_err(&iter), NO_ERR_STRING);
}

// Try maximum number of fields in an Options Template definition
TEST(tsetIter, maxOptionsTemplate)
{
    /* Similar as above
     * (65535 - 20 - 8 - 16 - 4 - 6) = 65481 octets
     * 65481 / 4 = 16370
     */
    const uint16_t REC_CNT = 16370;
    ipfix_trec rec {10000, REC_CNT / 2};
    for (uint16_t i = 0; i < REC_CNT; ++i) {
        rec.add_field(i, 2);
    }

    ipfix_set set {FDS_IPFIX_SET_OPTS_TMPLT};
    set.add_rec(rec);
    set_uniq hdr_set(set.release(), &free);

    // Try to initialize iterator
    struct fds_tset_iter iter;
    fds_tset_iter_init(&iter, hdr_set.get());

    ASSERT_EQ(fds_tset_iter_next(&iter), FDS_OK);
    EXPECT_EQ(iter.size, 6 + 4 * REC_CNT); // header + IEs
    EXPECT_EQ(iter.scope_cnt, REC_CNT / 2);
    EXPECT_EQ(iter.field_cnt, REC_CNT);
    EXPECT_EQ(iter.ptr.opts_trec, reinterpret_cast<fds_ipfix_opts_trec *>(hdr_set.get() + 1));

    // The end of the Set
    EXPECT_EQ(fds_tset_iter_next(&iter), FDS_EOC);
    EXPECT_EQ(fds_tset_iter_err(&iter), NO_ERR_STRING);
}

// Maximum number of Template definitions in one Set
TEST(tsetIter, maxTemplatesInSet)
{
    /* Max. IPFIX size: 65535
     * IPFIX Message header size: 16
     * IPFIX Set header size: 4
     * SIZE = 65535 - 16 - 4 = 65515
     *
     * Size of min. Template definition (1 field):  4 + 4
     * Definitions: 65515 / 8 = 8189
     */
    const uint16_t REC_CNT = 8189;
    ipfix_set set {FDS_IPFIX_SET_TMPLT};
    for (uint16_t i = 0; i < REC_CNT; ++i) {
        ipfix_trec rec {static_cast<uint16_t>(FDS_IPFIX_SET_MIN_DSET + i)};
        rec.add_field(i, i + 1);
        set.add_rec(rec);
    }
    set_uniq hdr_set(set.release(), &free);

    // Try to initialize iterator
    struct fds_tset_iter iter;
    fds_tset_iter_init(&iter, hdr_set.get());

    for (uint16_t i = 0; i < REC_CNT; ++i) {
        ASSERT_EQ(fds_tset_iter_next(&iter), FDS_OK);
        EXPECT_EQ(iter.size, 8U);
        EXPECT_EQ(iter.field_cnt, 1U);
        EXPECT_EQ(iter.scope_cnt, 0);
    }

    // The end of the Set
    EXPECT_EQ(fds_tset_iter_next(&iter), FDS_EOC);
    EXPECT_EQ(fds_tset_iter_err(&iter), NO_ERR_STRING);
}

// Maximum number of Options Template definitions in one Set
TEST(tsetIter, maxOptionsTemplatesInSet)
{
    /* Max. IPFIX size: 65535
     * IPFIX Message header size: 16
     * IPFIX Set header size: 4
     * SIZE = 65535 - 16 - 4 = 65515
     *
     * Size of min. Options Template definition (1 field, 1 scope):  6 + 4
     * Definitions: 65515 / 10 = 6551
     */
    const uint16_t REC_CNT = 6551;
    ipfix_set set {FDS_IPFIX_SET_OPTS_TMPLT};
    for (uint16_t i = 0; i < REC_CNT; ++i) {
        ipfix_trec rec {static_cast<uint16_t>(FDS_IPFIX_SET_MIN_DSET + i), 1};
        rec.add_field(i, i + 1);
        set.add_rec(rec);
    }
    set_uniq hdr_set(set.release(), &free);

    // Try to initialize iterator
    struct fds_tset_iter iter;
    fds_tset_iter_init(&iter, hdr_set.get());

    for (uint16_t i = 0; i < REC_CNT; ++i) {
        ASSERT_EQ(fds_tset_iter_next(&iter), FDS_OK);
        EXPECT_EQ(iter.size, 10U);
        EXPECT_EQ(iter.field_cnt, 1U);
        EXPECT_EQ(iter.scope_cnt, 1U);
    }

    // The end of the Set
    EXPECT_EQ(fds_tset_iter_next(&iter), FDS_EOC);
    EXPECT_EQ(fds_tset_iter_err(&iter), NO_ERR_STRING);
}

// Maximum number of Withdrawals in one Set
TEST(tsetIter, maxWithdrawalsInSet)
{
    /* Max. IPFIX size: 65535
     * IPFIX Message header size: 16
     * IPFIX Set header size: 4
     * SIZE = 65535 - 16 - 4 = 65515
     *
     * Size of Template Withdrawal (0 fields):  4
     * Definitions: 65515 / 4 = 16378
     */

    const uint16_t REC_CNT = 16378;
    ipfix_set set {FDS_IPFIX_SET_OPTS_TMPLT};
    for (uint16_t i = 0; i < REC_CNT; ++i) {
        ipfix_trec rec {static_cast<uint16_t>(FDS_IPFIX_SET_MIN_DSET + i)};
        set.add_rec(rec);
    }
    set_uniq hdr_set(set.release(), &free);

    // Try to initialize iterator
    struct fds_tset_iter iter;
    fds_tset_iter_init(&iter, hdr_set.get());

    for (uint16_t i = 0; i < REC_CNT; ++i) {
        ASSERT_EQ(fds_tset_iter_next(&iter), FDS_OK);
        EXPECT_EQ(iter.size, 4U);
        EXPECT_EQ(iter.field_cnt, 0U);
        EXPECT_EQ(iter.scope_cnt, 0U);
    }

    // The end of the Set
    EXPECT_EQ(fds_tset_iter_next(&iter), FDS_EOC);
    EXPECT_EQ(fds_tset_iter_err(&iter), NO_ERR_STRING);
}

TEST(tsetIter, templateSetPadding)
{
    const uint16_t max_padding = 3; // 3 bytes
    for (uint16_t padding = 0; padding <= max_padding; ++padding) {
        // Prepare a simple template
        ipfix_trec rec {FDS_IPFIX_SET_MIN_DSET};
        rec.add_field(1, 4);      // bytes
        rec.add_field(1, 20, 10); // random enterprise IE

        // Prepare a Template Set
        ipfix_set tset {FDS_IPFIX_SET_TMPLT};
        tset.add_rec(rec);
        tset.add_padding(padding);  // << Add padding
        set_uniq hdr_set(tset.release(), &free);

        // Set header + Tmplt header + 2 fields + 1x Enterprise Num + padding
        uint16_t real_set_len = ntohs(hdr_set.get()->length);
        EXPECT_EQ(real_set_len, 4 + 4 + 8 + 4 + padding);

        // Try to initialize iterator
        struct fds_tset_iter iter;
        fds_tset_iter_init(&iter, hdr_set.get());

        ASSERT_EQ(fds_tset_iter_next(&iter), FDS_OK);
        EXPECT_EQ(iter.scope_cnt, 0);
        EXPECT_EQ(iter.field_cnt, 2);
        // Size: 4 bytes Template header + 2x IEs (x 4B) + 1x Enterprise Extensions (x 4B)
        EXPECT_EQ(iter.size, 4 + 8 + 4);
        // Position of the template should be right behind the Set header
        fds_ipfix_trec *tmplt_pos = reinterpret_cast<fds_ipfix_trec *>(hdr_set.get() + 1);
        EXPECT_EQ(iter.ptr.trec, tmplt_pos);

        // The end of the Set
        EXPECT_EQ(fds_tset_iter_next(&iter), FDS_EOC);
        EXPECT_EQ(fds_tset_iter_err(&iter), NO_ERR_STRING);
    }
}

TEST(tsetIter, optionsTemplateSetPadding)
{
    const uint16_t max_padding = 3; // 3 bytes
    for (uint16_t padding = 0; padding <= max_padding; ++padding) {
        // Prepare a simple template
        ipfix_trec rec {FDS_IPFIX_SET_MIN_DSET, 1};
        rec.add_field(1, 20, 10); // random enterprise IE
        rec.add_field(1, 4);      // bytes

        // Prepare a Template Set
        ipfix_set tset {FDS_IPFIX_SET_OPTS_TMPLT};
        tset.add_rec(rec);
        tset.add_padding(padding);  // << Add padding
        set_uniq hdr_set(tset.release(), &free);

        // Set header + Options Tmplt header + 2 fields + 1x Enterprise Num + padding
        uint16_t real_set_len = ntohs(hdr_set.get()->length);
        EXPECT_EQ(real_set_len, 4 + 6 + 8 + 4 + padding);

        // Try to initialize iterator
        struct fds_tset_iter iter;
        fds_tset_iter_init(&iter, hdr_set.get());

        ASSERT_EQ(fds_tset_iter_next(&iter), FDS_OK);
        EXPECT_EQ(iter.scope_cnt, 1);
        EXPECT_EQ(iter.field_cnt, 2);
        // Size: 4 bytes Template header + 2x IEs (x 4B) + 1x Enterprise Extensions (x 4B)
        EXPECT_EQ(iter.size, 6 + 8 + 4);
        // Position of the template should be right behind the Set header
        fds_ipfix_opts_trec *tmplt_pos = reinterpret_cast<fds_ipfix_opts_trec *>(hdr_set.get() + 1);
        EXPECT_EQ(iter.ptr.opts_trec, tmplt_pos);

        // The end of the Set
        EXPECT_EQ(fds_tset_iter_next(&iter), FDS_EOC);
        EXPECT_EQ(fds_tset_iter_err(&iter), NO_ERR_STRING);
    }
}

TEST(tsetIter, withdrawalSetPadding)
{
    const uint16_t max_padding = 3; // 3 bytes

    // Try Template Withdrawal and Options Template Withdrawal
    std::vector<uint16_t> set_ids{FDS_IPFIX_SET_TMPLT, FDS_IPFIX_SET_OPTS_TMPLT};
    for (const uint16_t &set_id : set_ids) {
        for (uint16_t padding = 0; padding <= max_padding; ++padding) {
            // Prepare a Template Withdrawal record
            ipfix_trec rec {FDS_IPFIX_SET_MIN_DSET};

            // Prepare a Set
            ipfix_set tset {set_id};
            tset.add_rec(rec);
            tset.add_padding(padding);  // << Add padding
            set_uniq hdr_set(tset.release(), &free);

            // Set header + Withdrawal header + padding
            uint16_t real_set_len = ntohs(hdr_set.get()->length);
            EXPECT_EQ(real_set_len, 4 + 4 + padding);
            SCOPED_TRACE("Set ID " + std::to_string(set_id) + ", padding "
                + std::to_string(padding) + " byte(s)");

            // Try to initialize iterator
            struct fds_tset_iter iter;
            fds_tset_iter_init(&iter, hdr_set.get());

            ASSERT_EQ(fds_tset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.scope_cnt, 0);
            EXPECT_EQ(iter.field_cnt, 0);
            // Size: 4 bytes Withdrawal header
            EXPECT_EQ(iter.size, 4);
            // Position of the template should be right behind the Set header
            fds_ipfix_wdrl_trec *tmplt_pos = reinterpret_cast<fds_ipfix_wdrl_trec *>(hdr_set.get() + 1);
            EXPECT_EQ(iter.ptr.wdrl_trec, tmplt_pos);

            // The end of the Set
            EXPECT_EQ(fds_tset_iter_next(&iter), FDS_EOC);
            EXPECT_EQ(fds_tset_iter_err(&iter), NO_ERR_STRING);
        }
    }
}

// Malformed (Options) Templates ------------------------------------------------------------------

// Try to parse a malformed Set with a single template
void
failtest(ipfix_set &set)
{
    struct fds_tset_iter iter;
    set_uniq hdr_norm(set.release(), &free);

    fds_tset_iter_init(&iter, hdr_norm.get());
    EXPECT_EQ(fds_tset_iter_next(&iter), FDS_ERR_FORMAT);
    EXPECT_NE(fds_tset_iter_err(&iter), NO_ERR_STRING);
}

// Try to parse a malformed Set with multiple templates
void
failtest_multi(ipfix_set &set)
{
    set_uniq hdr_norm(set.release(), &free);

    struct fds_tset_iter iter;
    fds_tset_iter_init(&iter, hdr_norm.get());

    int rc;
    while ((rc = fds_tset_iter_next(&iter)) == FDS_OK) {
        // Skip valid records
    }

    // Expect that the processing fails
    EXPECT_EQ(rc, FDS_ERR_FORMAT);
}

// Try to parse empty set
TEST(tsetIterMalformed, emptySet)
{
    // Note: Each Set should consist of at least one template but it is not fatal failure...
    // Normal Template
    ipfix_set set_norm {FDS_IPFIX_SET_TMPLT};
    failtest(set_norm);

    // Options Template
    ipfix_set set_opts {FDS_IPFIX_SET_OPTS_TMPLT};
    failtest(set_opts);
}

// Unexpected end of Set (after (Options) Template header)
TEST(tsetIterMalformed, unexpectedSetEndBeforeFirstField)
{
    // Normal Template
    ipfix_trec rec_norm {10000};
    rec_norm.overwrite_field_cnt(10); // Just Template header without fields

    ipfix_set set_norm {FDS_IPFIX_SET_TMPLT};
    set_norm.add_rec(rec_norm);
    failtest(set_norm);

    // Options Template
    ipfix_trec rec_opts {10000, 2};
    rec_opts.overwrite_field_cnt(10); // Just header without any fields

    ipfix_set set_opts {FDS_IPFIX_SET_OPTS_TMPLT};
    set_opts.add_rec(rec_opts);
    failtest(set_opts);
}

// Unexpected end of Set (next field definition)
TEST(tsetIterMalformed, unexpectedSetEndBeforeFieldDef)
{
    // Normal Template
    ipfix_trec rec_norm {20000};
    rec_norm.add_field(5,  4);
    rec_norm.add_field(10, ipfix_trec::SIZE_VAR);
    rec_norm.overwrite_field_cnt(3); // Configure invalid number of fields

    ipfix_set set_norm {FDS_IPFIX_SET_TMPLT};
    set_norm.add_rec(rec_norm);
    failtest(set_norm);

    // Options Template
    ipfix_trec rec_opts {10000, 3};
    rec_opts.add_field(5, 4);
    rec_opts.add_field(10, ipfix_trec::SIZE_VAR);
    rec_opts.overwrite_field_cnt(3); // Just header without any fields

    ipfix_set set_opts {FDS_IPFIX_SET_OPTS_TMPLT};
    set_opts.add_rec(rec_opts);
    failtest(set_opts);
}

// Unexpected end of Set (enterprise number definition)
TEST(tsetIterMalformed, unepectedSetEndBeforeEnterpriseNum)
{
    // Normal Template
    ipfix_trec rec_norm {20000};
    rec_norm.add_field(5,  4);
    rec_norm.add_field(10 | 0x8000, 8); // Add "enterprise" bit to the ID

    ipfix_set set_norm {FDS_IPFIX_SET_TMPLT};
    set_norm.add_rec(rec_norm);
    failtest(set_norm);

    // Options Template
    ipfix_trec rec_opts {10000, 2};
    rec_opts.add_field(5, 4);
    rec_opts.add_field(10 | 0x8000, 8); // Add "enterprise" bit to the ID

    ipfix_set set_opts {FDS_IPFIX_SET_OPTS_TMPLT};
    set_opts.add_rec(rec_opts);
    failtest(set_opts);
}

// Template data length 0
TEST(tsetIterMalformed, zeroDataLength)
{
    // Normal Template
    ipfix_trec rec_norm {12345};
    rec_norm.add_field(5,  0);
    rec_norm.add_field(10, 0);

    ipfix_set set_norm {FDS_IPFIX_SET_TMPLT};
    set_norm.add_rec(rec_norm);
    failtest(set_norm);

    // Options Template
    ipfix_trec rec_opts {12345, 1};
    rec_opts.add_field(5,  0);
    rec_opts.add_field(10, 0);

    ipfix_set set_opts {FDS_IPFIX_SET_OPTS_TMPLT};
    set_opts.add_rec(rec_opts);
    failtest(set_opts);
}

// Template that defined too long data record
TEST(tsetIterMalformed, outOfRangeDataLength)
{
    /* Max. IPFIX MSG size:   65535
     * IPFIX MSG header size: 16
     * IPFIX Set header size: 4
     * -> Max data rec size = 65535 - 16 - 4 = 65515 bytes
     */
    const uint16_t max_size = 65515;

    // Normal Template
    ipfix_trec rec_norm {12345};
    rec_norm.add_field(100, max_size);
    rec_norm.add_field(200, 1);  // Extra byte

    ipfix_set set_norm {FDS_IPFIX_SET_TMPLT};
    set_norm.add_rec(rec_norm);
    failtest(set_norm);

    // Options Template
    ipfix_trec rec_opts {12345, 1};
    rec_opts.add_field(100, 1);
    rec_opts.add_field(200, max_size);

    ipfix_set set_opts {FDS_IPFIX_SET_OPTS_TMPLT};
    set_opts.add_rec(rec_opts);
    failtest(set_opts);
}

// Options Template with zero scope cnt
TEST(tsetIterMalformed, zeroScopeFields)
{
    ipfix_trec rec_opts {12345, 0};
    rec_opts.add_field(12345, 16);
    rec_opts.add_field(10, ipfix_trec::SIZE_VAR);

    ipfix_set set_opts {FDS_IPFIX_SET_OPTS_TMPLT};
    set_opts.add_rec(rec_opts);
    failtest(set_opts);
}

// More scope fields than total number of fields
TEST(tsetIterMalformed, tooManyScopeFields)
{
    ipfix_trec rec_opts {256, 2}; // 2 scope fields
    rec_opts.add_field(1, 1);     // but only one field defined

    ipfix_set set_opts {FDS_IPFIX_SET_OPTS_TMPLT};
    set_opts.add_rec(rec_opts);
    failtest(set_opts);
}

// Template and Options Template definitions in the same Set
TEST(tsetIterMalformed, mixTemplatesInSet)
{
    ipfix_trec rec_norm {400};
    rec_norm.add_field(100, 16);
    rec_norm.add_field(200, 256);

    ipfix_trec rec_opts {500, 2};
    rec_opts.add_field(20000, 16);
    rec_opts.add_field(30000, 4);
    rec_opts.add_field(25000, 4);

    // Normal Set
    {
        SCOPED_TRACE("Template Set 1");
        ipfix_set set_norm1 {FDS_IPFIX_SET_TMPLT};
        set_norm1.add_rec(rec_norm);
        set_norm1.add_rec(rec_opts);
        failtest_multi(set_norm1);
    }

    {
        SCOPED_TRACE("Template Set 2");
        ipfix_set set_norm2 {FDS_IPFIX_SET_TMPLT};
        set_norm2.add_rec(rec_opts);
        set_norm2.add_rec(rec_norm);
        failtest_multi(set_norm2);
    }


    // Options Set
    {
        SCOPED_TRACE("Options Template Set 1");
        ipfix_set set_opts1 {FDS_IPFIX_SET_OPTS_TMPLT};
        set_opts1.add_rec(rec_norm);
        set_opts1.add_rec(rec_opts);
        failtest_multi(set_opts1);
    }

    {
        SCOPED_TRACE("Options Template Set 2");
        ipfix_set set_opts2{FDS_IPFIX_SET_OPTS_TMPLT};
        set_opts2.add_rec(rec_opts);
        set_opts2.add_rec(rec_norm);
        failtest_multi(set_opts2);
    }
}

// Template definition within Options Template Set
TEST(tsetIterMalformed, templateInOptionsTemplateSet)
{
    ipfix_trec rec_norm {256};
    rec_norm.add_field(100, 16);
    rec_norm.add_field(200, 256);

    ipfix_set set_opts {FDS_IPFIX_SET_OPTS_TMPLT};
    set_opts.add_rec(rec_norm);
    failtest(set_opts);
}

// Options Template within Template Set
TEST(tsetIterMalformed, optionsTemplateInTemplateSet)
{
    // Options Template
    ipfix_trec rec_opts {256, 2};
    rec_opts.add_field(65000, 16);
    rec_opts.add_field(10000, 256);

    ipfix_set set_norm {FDS_IPFIX_SET_TMPLT};
    set_norm.add_rec(rec_opts);
    failtest(set_norm);
}

// All (Options) Template Withdrawal inside invalid Set ID
TEST(tsetIterMalformed, allWithdrawalSetMismatch)
{
    // All Templates Withdrawal within Options Template Set
    ipfix_trec rec_norm {FDS_IPFIX_SET_TMPLT};
    ipfix_set set_norm {FDS_IPFIX_SET_OPTS_TMPLT};
    set_norm.add_rec(rec_norm);
    failtest(set_norm);

    // All Options Template Withdrawal within Template Set
    ipfix_trec rec_opts {FDS_IPFIX_SET_OPTS_TMPLT};
    ipfix_set set_opts {FDS_IPFIX_SET_TMPLT};
    set_opts.add_rec(rec_opts);
    failtest(set_opts);
}

// Combination of All (Options) Templates Withdrawal and other withdrawals
TEST(tsetIterMalformed, allWithdrawalAndOthers)
{
    ipfix_trec rec_other {256};

    // All Template Withdrawal
    ipfix_trec rec_all_norm {FDS_IPFIX_SET_TMPLT};
    ipfix_set set_norm {FDS_IPFIX_SET_TMPLT};
    set_norm.add_rec(rec_all_norm);
    set_norm.add_rec(rec_other);
    failtest(set_norm);

    // All Options Template Withdrawal
    ipfix_trec rec_all_opts {FDS_IPFIX_SET_OPTS_TMPLT};
    ipfix_set set_opts {FDS_IPFIX_SET_OPTS_TMPLT};
    set_opts.add_rec(rec_all_opts);
    set_opts.add_rec(rec_other);
    failtest(set_opts);
}

// Combination of (Options) Template Withdrawals and All (Options) Withdrawal
TEST(tsetIterMalformed, withdrawalsAndAllWithdrawals)
{
    ipfix_trec rec_all_norm{FDS_IPFIX_SET_TMPLT};
    ipfix_trec rec_all_opts{FDS_IPFIX_SET_OPTS_TMPLT};
    ipfix_trec rec_with1{256};
    ipfix_trec rec_with2{300};

    {
        SCOPED_TRACE("Example 1");
        ipfix_set set {FDS_IPFIX_SET_TMPLT};
        set.add_rec(rec_with1);
        set.add_rec(rec_all_norm);
        failtest_multi(set);
    }

    {
        SCOPED_TRACE("Example 2");
        ipfix_set set {FDS_IPFIX_SET_TMPLT};
        set.add_rec(rec_with1);
        set.add_rec(rec_all_opts);
        failtest_multi(set);
    }

    {
        SCOPED_TRACE("Example 3");
        ipfix_set set {FDS_IPFIX_SET_TMPLT};
        set.add_rec(rec_all_norm);
        set.add_rec(rec_with2);
        failtest_multi(set);
    }

    {
        SCOPED_TRACE("Example 4");
        ipfix_set set {FDS_IPFIX_SET_OPTS_TMPLT};
        set.add_rec(rec_with1);
        set.add_rec(rec_all_opts);
        failtest_multi(set);
    }

    {
        SCOPED_TRACE("Example 5");
        ipfix_set set {FDS_IPFIX_SET_OPTS_TMPLT};
        set.add_rec(rec_with1);
        set.add_rec(rec_all_norm);
        failtest_multi(set);
    }

    {
        SCOPED_TRACE("Example 6");
        ipfix_set set {FDS_IPFIX_SET_OPTS_TMPLT};
        set.add_rec(rec_all_opts);
        set.add_rec(rec_with2);
        failtest_multi(set);
    }
}

// Test invalid Template ID
TEST(tsetIterMalformed, invalidTmpltDefID)
{
    for (uint16_t id = 0; id < FDS_IPFIX_SET_MIN_DSET; ++id) {
        SCOPED_TRACE("Template ID: " + std::to_string(id));
        // Normal Template
        ipfix_trec rec_norm {id};
        rec_norm.add_field(5,  8);
        rec_norm.add_field(10, 4);

        ipfix_set set_norm {FDS_IPFIX_SET_TMPLT};
        set_norm.add_rec(rec_norm);
        failtest(set_norm);
    }

    for (uint16_t id = 0; id < FDS_IPFIX_SET_MIN_DSET; ++id) {
        // Options Template
        ipfix_trec rec_opts {id, 1};
        rec_opts.add_field(5,  16);
        rec_opts.add_field(10, 4);

        ipfix_set set_opts {FDS_IPFIX_SET_OPTS_TMPLT};
        set_opts.add_rec(rec_opts);
        failtest(set_opts);
    }
}
