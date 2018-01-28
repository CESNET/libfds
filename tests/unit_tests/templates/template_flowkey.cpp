#include <gtest/gtest.h>
#include <libfds.h>

// Template generator
#include <TGenerator.h>
#include <common_tests.h>
#include <cstring>
#include <TMock.h>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// Template definition for tests
struct fk_template_params {
    uint16_t id;
    enum fds_template_type type;
    fds_template_flag_t flags;
    uint16_t scope_fields;
};

// Template field definition for tests
struct fk_field_params {
    uint16_t id;
    uint32_t en;
    uint16_t len;
    fds_template_flag_t flags;
};

// Parsed template
using uniq_fds_tmplt = std::unique_ptr<struct fds_template, decltype(&::fds_template_destroy)>;

/**
 * \brief Create an auxiliary FDS template
 *
 * All flags will be cleared.
 * \param[in]  tmplt  Template parameters
 * \param[in]  fields Vector of field parameters
 * \param[out] out    Newly create template
 */
void
template_create(const fk_template_params &tmplt, const std::vector<struct fk_field_params> &fields,
    uniq_fds_tmplt &out)
{
    // Check parameters of the template
    ASSERT_TRUE(tmplt.type == FDS_TYPE_TEMPLATE || tmplt.type == FDS_TYPE_TEMPLATE_OPTS);
    if (tmplt.type == FDS_TYPE_TEMPLATE) {
        ASSERT_EQ(tmplt.scope_fields, 0);
    } else {
        ASSERT_GT(tmplt.scope_fields, 0);
    }

    // Prepare the template
    TGenerator tdata(tmplt.id, fields.size(), tmplt.scope_fields);
    for (const auto &field : fields) {
        tdata.append(field.id, field.len, field.en);
    }

    // Create the template
    uint16_t  tmplt_len = tdata.length();
    struct fds_template *tmplt_rec = nullptr;

    ASSERT_EQ(fds_template_parse(tmplt.type, tdata.get(), &tmplt_len, &tmplt_rec), FDS_OK);
    ASSERT_NE(tmplt_rec, nullptr);
    EXPECT_EQ(tmplt_len, tdata.length());
    out = uniq_fds_tmplt(tmplt_rec, &::fds_template_destroy);
}

/**
 * \brief Compare template and expected parameters
 *
 * Check if the flags are the same as expected.
 * \param[in] tmplt  Template parameters
 * \param[in] fields Vector of field parameters
 * \param[in] in Parsed FDS template
 */
void
template_tester(const fk_template_params &tmplt, const std::vector<struct fk_field_params> &fields,
    uniq_fds_tmplt &in)
{
    const struct fds_template *tmplt_rec = in.get();
    // Check test integrity
    ASSERT_EQ(tmplt_rec->fields_cnt_total, fields.size());
    ASSERT_EQ(tmplt_rec->fields_cnt_scope, tmplt.scope_fields);

    for (uint16_t i = 0; i <tmplt_rec->fields_cnt_total; ++i) {
        SCOPED_TRACE("Testing field ID: " + std::to_string(i));
        const fds_tfield *field = &tmplt_rec->fields[i];
        // Check test integrity
        ASSERT_EQ(field->id, fields[i].id);
        ASSERT_EQ(field->en, fields[i].en);
        ct_tfield_flags(field, fields[i].flags);
    }

    // Check template flags
    ct_template_flags(tmplt_rec, tmplt.flags);
}

// Test typical flow fields and a corresponding flow key
TEST(define, simple)
{
    struct fk_template_params tmplt;
    tmplt.id = 12345;
    tmplt.type = FDS_TYPE_TEMPLATE;
    tmplt.flags = FDS_TEMPLATE_HAS_FKEY;
    tmplt.scope_fields = 0;

    constexpr fds_template_flag_t flg_comm = FDS_TFIELD_LAST_IE;
    constexpr fds_template_flag_t flg_key = flg_comm | FDS_TFIELD_FLOW_KEY;
    const std::vector<struct fk_field_params> fields = {
        // id - en - len - type - flags
        {  8, 0, 4, flg_key},  // sourceIPv4Address
        { 12, 0, 4, flg_key},  // destinationIPv4Address
        {  7, 0, 2, flg_key},  // sourceTransportPort
        { 11, 0, 2, flg_key},  // destinationTransportPort
        {  4, 0, 1, flg_key},  // protocolIdentifier
        {  6, 0, 1, flg_comm},  // tcpControlBits
        {152, 0, 8, flg_comm},  // flowStartMilliseconds
        {153, 0, 8, flg_comm},  // flowEndMilliseconds
        {  2, 0, 4, flg_comm},  // packetDeltaCount
        {  1, 0, 4, flg_comm},  // octetDeltaCount
    };

    const uint64_t key = 31; // First 5 elements...

    // Create a template and define the flow key
    uniq_fds_tmplt aux_template(nullptr, &::fds_template_destroy);
    template_create(tmplt, fields, aux_template);
    ASSERT_EQ(fds_template_flowkey_define(aux_template.get(), key), FDS_OK);
    template_tester(tmplt, fields, aux_template);

    // Try to compare the key
    EXPECT_EQ(fds_template_flowkey_cmp(aux_template.get(), key), 0);
    EXPECT_NE(fds_template_flowkey_cmp(aux_template.get(), key + 1), 0);
    EXPECT_NE(fds_template_flowkey_cmp(aux_template.get(), 0), 0);
    EXPECT_NE(fds_template_flowkey_cmp(aux_template.get(), UINT64_MAX), 0);
}

// Test longer key then number of elements (template should be untouched)
TEST(define, invalidKey)
{
    struct fk_template_params tmplt;
    tmplt.id = 12345;
    tmplt.type = FDS_TYPE_TEMPLATE;
    tmplt.flags = 0;
    tmplt.scope_fields = 0;

    constexpr fds_template_flag_t flg_comm = FDS_TFIELD_LAST_IE;
    const std::vector<struct fk_field_params> fields = {
        // id - en - len - type - flags
        {  8, 0, 4, flg_comm},  // sourceIPv4Address
        { 12, 0, 4, flg_comm},  // destinationIPv4Address
        {  7, 0, 2, flg_comm},  // sourceTransportPort
        { 11, 0, 2, flg_comm},  // destinationTransportPort
        {  4, 0, 1, flg_comm},  // protocolIdentifier
        {  6, 0, 1, flg_comm},  // tcpControlBits
        {152, 0, 8, flg_comm},  // flowStartMilliseconds
        {153, 0, 8, flg_comm},  // flowEndMilliseconds
        {  2, 0, 4, flg_comm},  // packetDeltaCount
        {  1, 0, 4, flg_comm},  // octetDeltaCount
    };

    const uint64_t key = 1055; // First 5 elements + 11th element...

    // Create a template and define the flow key
    uniq_fds_tmplt aux_template(nullptr, &::fds_template_destroy);
    template_create(tmplt, fields, aux_template);
    ASSERT_EQ(fds_template_flowkey_define(aux_template.get(), key), FDS_ERR_FORMAT);
    // Template should be untouched
    template_tester(tmplt, fields, aux_template);

    // to compare the key
    EXPECT_EQ(fds_template_flowkey_cmp(aux_template.get(), 0), 0);
    EXPECT_NE(fds_template_flowkey_cmp(aux_template.get(), key), 0);
}

// Test removing template flow key
TEST(define, remove)
{
    struct fk_template_params tmplt;
    tmplt.id = 256;
    tmplt.type = FDS_TYPE_TEMPLATE;
    tmplt.flags = FDS_TEMPLATE_HAS_MULTI_IE;
    tmplt.scope_fields = 0;

    constexpr fds_template_flag_t flg_comm = FDS_TFIELD_LAST_IE;
    constexpr fds_template_flag_t flg_multi = FDS_TFIELD_MULTI_IE;
    constexpr fds_template_flag_t flg_multi_last = flg_multi | FDS_TFIELD_LAST_IE;

    const std::vector<struct fk_field_params> fields = {
        // id - en - len - type - flags
        {  8, 0, 4, flg_comm}, // sourceIPv4Address
        { 12, 0, 4, flg_comm}, // destinationIPv4Address
        {  7, 0, 2, flg_comm}, // sourceTransportPort
        { 11, 0, 2, flg_comm}, // destinationTransportPort
        {  4, 0, 1, flg_comm}, // protocolIdentifier
        {  6, 0, 1, flg_multi}, // tcpControlBits
        {152, 0, 8, flg_multi}, // flowStartMilliseconds
        {153, 0, 8, flg_multi}, // flowEndMilliseconds
        {  2, 0, 4, flg_multi}, // packetDeltaCount
        {  1, 0, 4, flg_multi}, // octetDeltaCount
        {  6, 0, 1, flg_multi_last}, // tcpControlBits
        {152, 0, 8, flg_multi_last}, // flowStartMilliseconds
        {153, 0, 8, flg_multi_last}, // flowEndMilliseconds
        {  2, 0, 4, flg_multi_last}, // packetDeltaCount
        {  1, 0, 4, flg_multi_last}, // octetDeltaCount
    };

    uniq_fds_tmplt aux_template(nullptr, &::fds_template_destroy);
    template_create(tmplt, fields, aux_template);
    // Add flow key
    EXPECT_EQ(fds_template_flowkey_define(aux_template.get(), 21845), FDS_OK);
    EXPECT_TRUE((aux_template.get()->flags & FDS_TEMPLATE_HAS_FKEY) != 0);
    // Remove flow key
    EXPECT_EQ(fds_template_flowkey_define(aux_template.get(), 0), FDS_OK);
    template_tester(tmplt, fields, aux_template);
}

// Test change of already defined key
TEST(define, redefine)
{
    struct fk_template_params tmplt;
    tmplt.id = 8879;
    tmplt.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt.flags = FDS_TEMPLATE_HAS_FKEY;
    tmplt.scope_fields = 2;

    constexpr fds_template_flag_t flg_comm = FDS_TFIELD_LAST_IE;
    constexpr fds_template_flag_t flg_comm_fk = flg_comm | FDS_TFIELD_FLOW_KEY;
    constexpr fds_template_flag_t flg_scope = flg_comm | FDS_TFIELD_SCOPE;

    const std::vector<struct fk_field_params> fields = {
        // id - en - len - type - flags
        {  8, 0, 4, flg_scope}, // sourceIPv4Address
        { 12, 0, 4, flg_scope}, // destinationIPv4Address
        {  7, 0, 2, flg_comm_fk}, // sourceTransportPort
        { 11, 0, 2, flg_comm}, // destinationTransportPort
        {  4, 0, 1, flg_comm}, // protocolIdentifier
        {  6, 0, 1, flg_comm_fk},  // tcpControlBits
        {152, 0, 8, flg_comm},  // flowStartMilliseconds
        {153, 0, 8, flg_comm_fk},  // flowEndMilliseconds
        {  2, 0, 4, flg_comm},  // packetDeltaCount
        {  1, 0, 4, flg_comm}   // octetDeltaCount
    };

    const uint64_t key_old = 859;
    const uint64_t key_new = 164;

    uniq_fds_tmplt aux_template(nullptr, &::fds_template_destroy);
    template_create(tmplt, fields, aux_template);

    // Define the flow key
    EXPECT_EQ(fds_template_flowkey_define(aux_template.get(), key_old), FDS_OK);
    EXPECT_EQ(fds_template_flowkey_cmp(aux_template.get(), key_old), 0);
    EXPECT_NE(fds_template_flowkey_cmp(aux_template.get(), key_new), 0);

    // Redefine the flow key
    EXPECT_EQ(fds_template_flowkey_define(aux_template.get(), key_new), FDS_OK);
    EXPECT_NE(fds_template_flowkey_cmp(aux_template.get(), key_old), 0);
    EXPECT_EQ(fds_template_flowkey_cmp(aux_template.get(), key_new), 0);

    template_tester(tmplt, fields, aux_template);
}

TEST(applicable, valid)
{
    const uint64_t fkey = 31U;
    struct fds_template *t1 = TMock::create(TMock::type::DATA_BASIC_FLOW, 256);
    EXPECT_EQ(fds_template_flowkey_applicable(t1, fkey), FDS_OK);

    // The template should be untouched
    EXPECT_TRUE((t1->flags & FDS_TEMPLATE_HAS_FKEY) == 0);
    for (uint16_t i = 0; i < t1->fields_cnt_total; ++i) {
        EXPECT_TRUE((t1->fields[i].flags & FDS_TFIELD_FLOW_KEY) == 0);
    }

    fds_template_destroy(t1);
}

TEST(applicable, invalid)
{
    /*
    // Try to use it on Options Template
    struct fds_template *t_otps = TMock::create(TMock::type::OPTS_MPROC_RSTAT, 256);
    EXPECT_EQ(fds_template_flowkey_applicable(t_otps, 3), FDS_ERR_ARG);
    EXPECT_TRUE((t_otps->flags & FDS_TEMPLATE_HAS_FKEY) != 0);
    fds_template_destroy(t_otps);
    */

    // Try too long flow key (definition of non-existing fields)
    struct fds_template *t_short = TMock::create(TMock::type::DATA_BASIC_FLOW, 257);
    EXPECT_EQ(fds_template_flowkey_applicable(t_short, 2047), FDS_ERR_FORMAT);
    EXPECT_TRUE((t_short->flags & FDS_TEMPLATE_HAS_FKEY) == 0);
    fds_template_destroy(t_short);
}