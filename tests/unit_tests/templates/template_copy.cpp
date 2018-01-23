#include <gtest/gtest.h>
#include <libfds.h>
#include <memory>
#include <cstring>

#include <TGenerator.h>

constexpr uint16_t VAR_IE = 65535;

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(Copy, simpleCheck)
{
    TGenerator tdata(256, 10, 0);
    tdata.append(  8,      4); // sourceIPv4Address
    tdata.append( 12,      4); // destinationIPv4Address
    tdata.append(  7,      2); // sourceTransportPort
    tdata.append( 11,      2); // destinationTransportPort
    tdata.append(460, VAR_IE); // httpRequestHost
    tdata.append(461, VAR_IE); // httpRequestTarget
    tdata.append(  4,      1); // protocolIdentifier
    tdata.append(468, VAR_IE); // httpUserAgent
    tdata.append(  2,      4); // packetDeltaCount
    tdata.append(  1,      4); // octetDeltaCount

    struct fds_template *tmplt, *copy;
    enum fds_template_type type = FDS_TYPE_TEMPLATE;
    uint16_t len = tdata.length();
    ASSERT_EQ(fds_template_parse(type, tdata.get(), &len, &tmplt), FDS_OK);

    // Create a copy
    copy = fds_template_copy(tmplt);
    ASSERT_NE(copy, nullptr);
    ASSERT_NE(copy, tmplt);

    // Check internals
    EXPECT_EQ(copy->type, tmplt->type);
    EXPECT_EQ(copy->opts_types, tmplt->opts_types);
    EXPECT_EQ(copy->id, tmplt->id);
    EXPECT_EQ(copy->flags, tmplt->flags);
    EXPECT_EQ(copy->data_length, tmplt->data_length);
    EXPECT_EQ(copy->fields_cnt_total, tmplt->fields_cnt_total);
    EXPECT_EQ(copy->fields_cnt_scope, tmplt->fields_cnt_scope);

    // Check timestamps
    EXPECT_EQ(copy->time.first_seen, tmplt->time.first_seen);
    EXPECT_EQ(copy->time.last_seen, tmplt->time.last_seen);
    EXPECT_EQ(copy->time.end_of_life, tmplt->time.end_of_life);

    // Check raw fields
    EXPECT_NE(copy->raw.data, tmplt->raw.data);
    EXPECT_EQ(copy->raw.length, tmplt->raw.length);

    // Check fields
    size_t fields_size = tmplt->fields_cnt_total * sizeof(struct fds_tfield);
    EXPECT_EQ(std::memcmp(copy->fields, tmplt->fields, fields_size), 0);

    // Compare
    EXPECT_EQ(fds_template_cmp(tmplt, copy), 0);

    fds_template_destroy(tmplt);
    fds_template_destroy(copy);
}

TEST(compare, simple)
{
    TGenerator tdata1(256, 3, 0);
    tdata1.append(1, 2);
    tdata1.append(2, 4);
    tdata1.append(3, 8);

    TGenerator tdata2(256, 3, 0); // Different fields
    tdata2.append(3, 8);
    tdata2.append(2, 4);
    tdata2.append(1, 2);

    TGenerator tdata3(256, 3, 1); // Options template
    tdata3.append(1, 2);
    tdata3.append(2, 4);
    tdata3.append(3, 8);

    TGenerator tdata4(256, 2, 0); // Just 2 elements
    tdata4.append(1, 2);
    tdata4.append(2, 4);

    struct fds_template *t1, *t2, *t3, *t4;
    uint16_t t1_len = tdata1.length();
    uint16_t t2_len = tdata2.length();
    uint16_t t3_len = tdata3.length();
    uint16_t t4_len = tdata4.length();
    ASSERT_EQ(fds_template_parse(FDS_TYPE_TEMPLATE, tdata1.get(), &t1_len, &t1), FDS_OK);
    ASSERT_EQ(fds_template_parse(FDS_TYPE_TEMPLATE, tdata2.get(), &t2_len, &t2), FDS_OK);
    ASSERT_EQ(fds_template_parse(FDS_TYPE_TEMPLATE_OPTS, tdata3.get(), &t3_len, &t3), FDS_OK);
    ASSERT_EQ(fds_template_parse(FDS_TYPE_TEMPLATE, tdata4.get(), &t4_len, &t4), FDS_OK);

    EXPECT_EQ(fds_template_cmp(t1, t1), 0);
    EXPECT_NE(fds_template_cmp(t1, t2), 0);
    EXPECT_NE(fds_template_cmp(t1, t3), 0);
    EXPECT_NE(fds_template_cmp(t1, t4), 0);

    fds_template_destroy(t1);
    fds_template_destroy(t2);
    fds_template_destroy(t3);
    fds_template_destroy(t4);
}
