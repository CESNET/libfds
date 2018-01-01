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

    fds_template_destroy(tmplt);
    fds_template_destroy(copy);
}