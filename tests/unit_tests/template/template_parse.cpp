#include <gtest/gtest.h>
#include <libfds.h>
#include <memory>
#include <cstring>

#include <TGenerator.h>
#include <common_tests.h>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

constexpr uint16_t VAR_IE = 65535;

/** Expected values of templates */
struct exp_template_params {
    enum fds_template_type type;
    uint32_t opts_types;
    uint16_t id;
    fds_template_flag_t flags;
    uint16_t fields_cnt_total;
    uint16_t fields_cnt_scope;
};

/** Expected values of template field */
struct exp_field_params {
    uint16_t id;
    uint16_t en;
    uint16_t len;
    fds_template_flag_t flags;
    const struct fds_iemgr_elem *elem;
};

/**
 * \brief Template tester
 *
 * Based on description the template will be build, parsed and checked.
 * \param[in] tmplt  Template description
 * \param[in] fields Field descriptions
 */
void
template_tester(const exp_template_params &tmplt, const std::vector<exp_field_params> &fields)
{
    TGenerator tdata(tmplt.id, tmplt.fields_cnt_total, tmplt.fields_cnt_scope);
    size_t exp_data_size = 0;

    for (const auto &field : fields) {
        tdata.append(field.id, field.len, field.en);
        if (field.len != VAR_IE) {
            exp_data_size += field.len;
        } else {
            // At least real length must be present
            exp_data_size += 1U;
        }

    }

    uint16_t  tmplt_len = tdata.length();
    struct fds_template *tmplt_rec = nullptr;

    ASSERT_EQ(fds_template_parse(tmplt.type, tdata.get(), &tmplt_len, &tmplt_rec), FDS_OK);
    ASSERT_NE(tmplt_rec, nullptr);
    EXPECT_EQ(tmplt_len, tdata.length());

    // Check RAW copy
    EXPECT_EQ(std::memcmp(tmplt_rec->raw.data, tdata.get(), tdata.length()), 0);
    EXPECT_EQ(tmplt_rec->raw.length, tdata.length());
    EXPECT_NE(tmplt_rec->raw.data, tdata.get());

    // Check global parameters
    EXPECT_EQ(tmplt_rec->id, tmplt.id);
    EXPECT_EQ(tmplt_rec->type, tmplt.type);
    EXPECT_EQ(tmplt_rec->data_length, exp_data_size);
    ASSERT_EQ(tmplt_rec->fields_cnt_total, tmplt.fields_cnt_total);
    EXPECT_EQ(tmplt_rec->fields_cnt_scope, tmplt.fields_cnt_scope);
    EXPECT_EQ(tmplt_rec->opts_types, tmplt.opts_types);
    // Flags
    ct_template_flags(tmplt_rec, tmplt.flags);

    // Check fields
    size_t idx = 0;
    size_t exp_offset = 0;
    for (const auto &field : fields) {
        SCOPED_TRACE("Field ID: " + std::to_string(idx));
        const fds_tfield *tfield = &tmplt_rec->fields[idx];
        EXPECT_EQ(tfield->id, field.id);
        EXPECT_EQ(tfield->length, field.len);
        EXPECT_EQ(tfield->en, field.en);
        EXPECT_EQ(tfield->offset, exp_offset);
        EXPECT_EQ(tfield->def, field.elem);
        ct_tfield_flags(tfield, field.flags);
        idx++;

        // Calculate next offset
        if (exp_offset == VAR_IE) {
            continue;
        }

        EXPECT_LT(exp_offset, VAR_IE);
        if (field.len == VAR_IE) {
            exp_offset = VAR_IE;
        } else {
            exp_offset += field.len;
        }
    }

    fds_template_destroy(tmplt_rec);
}

// Standard static fields
TEST(Parse, SimpleStatic)
{
    const std::vector<struct exp_field_params> fields = {
        //  id - en - len - flags - elem ;
        {  8, 0, 4, FDS_TFIELD_LAST_IE, nullptr}, // sourceIPv4Address
        { 12, 0, 4, FDS_TFIELD_LAST_IE, nullptr}, // destinationIPv4Address
        {  7, 0, 2, FDS_TFIELD_LAST_IE, nullptr}, // sourceTransportPort
        { 11, 0, 2, FDS_TFIELD_LAST_IE, nullptr}, // destinationTransportPort
        {  4, 0, 1, FDS_TFIELD_LAST_IE, nullptr}, // protocolIdentifier
        {  6, 0, 1, FDS_TFIELD_LAST_IE, nullptr}, // tcpControlBits
        {152, 0, 8, FDS_TFIELD_LAST_IE, nullptr}, // flowStartMilliseconds
        {153, 0, 8, FDS_TFIELD_LAST_IE, nullptr}, // flowEndMilliseconds
        {  2, 0, 4, FDS_TFIELD_LAST_IE, nullptr}, // packetDeltaCount
        {  1, 0, 4, FDS_TFIELD_LAST_IE, nullptr}  // octetDeltaCount
    };

    struct exp_template_params tmplt;
    tmplt.id =  256;
    tmplt.type = FDS_TYPE_TEMPLATE;
    tmplt.opts_types = 0;
    tmplt.fields_cnt_total = fields.size();
    tmplt.fields_cnt_scope = 0;
    tmplt.flags = 0;

    template_tester(tmplt, fields);
}

// Standard static and dynamic fields
TEST(Parse, SimpleDynamic)
{
    const std::vector<struct exp_field_params> fields = {
        //  id - en - len - flags - elem;
        {  8, 0,      4, FDS_TFIELD_LAST_IE, nullptr}, // sourceIPv4Address
        { 12, 0,      4, FDS_TFIELD_LAST_IE, nullptr}, // destinationIPv4Address
        {  7, 0,      2, FDS_TFIELD_LAST_IE, nullptr}, // sourceTransportPort
        { 11, 0,      2, FDS_TFIELD_LAST_IE, nullptr}, // destinationTransportPort
        {460, 0, VAR_IE, FDS_TFIELD_LAST_IE, nullptr}, // httpRequestHost
        {461, 0, VAR_IE, FDS_TFIELD_LAST_IE, nullptr}, // httpRequestTarget
        {  4, 0,      1, FDS_TFIELD_LAST_IE, nullptr}, // protocolIdentifier
        {468, 0, VAR_IE, FDS_TFIELD_LAST_IE, nullptr}, // httpUserAgent
        {  2, 0,      4, FDS_TFIELD_LAST_IE, nullptr}, // packetDeltaCount
        {  1, 0,      4, FDS_TFIELD_LAST_IE, nullptr}  // octetDeltaCount
    };

    struct exp_template_params tmplt;
    tmplt.id =  1000;
    tmplt.type = FDS_TYPE_TEMPLATE;
    tmplt.opts_types = 0;
    tmplt.fields_cnt_total = fields.size();
    tmplt.fields_cnt_scope = 0;
    tmplt.flags = FDS_TEMPLATE_HAS_DYNAMIC;

    template_tester(tmplt, fields);
}

// Nonstandard fields
TEST(Parse, EnpterpriseSimple)
{
    const std::vector<struct exp_field_params> fields = {
        //  id - en - len - flags - elem;
        {  8, 0,      4, FDS_TFIELD_LAST_IE, nullptr}, // sourceIPv4Address
        { 12, 0,      4, FDS_TFIELD_LAST_IE, nullptr}, // destinationIPv4Address
        {  7, 0,      2, FDS_TFIELD_LAST_IE, nullptr}, // sourceTransportPort
        { 11, 2,      2, FDS_TFIELD_LAST_IE, nullptr}, // -- something enterprise
        { 10, 2, VAR_IE, FDS_TFIELD_LAST_IE, nullptr}, // -- something enterprise
        { 12, 2, VAR_IE, FDS_TFIELD_LAST_IE, nullptr}, // -- something enterprise
        {  4, 0,      1, FDS_TFIELD_LAST_IE, nullptr}, // protocolIdentifier
        {468, 0, VAR_IE, FDS_TFIELD_LAST_IE, nullptr}, // httpUserAgent
    };

    struct exp_template_params tmplt;
    tmplt.id =  40000;
    tmplt.type = FDS_TYPE_TEMPLATE;
    tmplt.opts_types = 0;
    tmplt.fields_cnt_total = fields.size();
    tmplt.fields_cnt_scope = 0;
    tmplt.flags = FDS_TEMPLATE_HAS_DYNAMIC;

    template_tester(tmplt, fields);
}

// Multiple definitions of the same element
TEST(Parse, MultiIE)
{
    const fds_template_flag_t flg_last = FDS_TFIELD_LAST_IE;
    const fds_template_flag_t flg_multi = FDS_TFIELD_MULTI_IE;
    const fds_template_flag_t flg_both = flg_last | flg_multi;

    const std::vector<struct exp_field_params> fields = {
        //  id - en - len - flags - elem;
        {  2, 0,      4, flg_last,  nullptr}, // packetDeltaCount
        {  1, 0,      4, flg_last,  nullptr}, // octetDeltaCount
        {  8, 0,      4, flg_multi, nullptr}, // sourceIPv4Address
        { 12, 0,      4, flg_multi, nullptr}, // destinationIPv4Address
        {  8, 0,      4, flg_both,  nullptr}, // sourceIPv4Address
        { 12, 0,      4, flg_both,  nullptr}, // destinationIPv4Address
        {468, 0, VAR_IE, flg_multi, nullptr}, // httpUserAgent
        {468, 0, VAR_IE, flg_both,  nullptr}, // httpUserAgent
        {152, 0,      8, flg_last,  nullptr}, // flowStartMilliseconds
        {153, 0,      8, flg_last,  nullptr}  // flowEndMilliseconds
    };

    struct exp_template_params tmplt;
    tmplt.id =  40000;
    tmplt.type = FDS_TYPE_TEMPLATE;
    tmplt.opts_types = 0;
    tmplt.fields_cnt_total = fields.size();
    tmplt.fields_cnt_scope = 0;
    tmplt.flags = FDS_TEMPLATE_HAS_DYNAMIC | FDS_TEMPLATE_HAS_MULTI_IE;

    template_tester(tmplt, fields);
}

TEST(Parse, Withdrawal)
{
    // Standard template
    const std::vector<struct exp_field_params> fields = {};

    struct exp_template_params tmplt;
    tmplt.id =  40000;
    tmplt.type = FDS_TYPE_TEMPLATE;
    tmplt.opts_types = 0;
    tmplt.fields_cnt_total = 0;
    tmplt.fields_cnt_scope = 0;
    tmplt.flags = 0;
    template_tester(tmplt, fields);

    // Options template
    tmplt.type = FDS_TYPE_TEMPLATE_OPTS;
    template_tester(tmplt, fields);
}

// Simple Options Template
TEST(Parse, SimpleOptions)
{
    const std::vector<struct exp_field_params> fields = {
        //  id - en - len - flags - elem;
        {  8, 0, 4, FDS_TFIELD_LAST_IE | FDS_TFIELD_SCOPE,  nullptr}, // sourceIPv4Address
        {  2, 0, 4, FDS_TFIELD_LAST_IE,                     nullptr}, // packetDeltaCount
        {  1, 0, 4, FDS_TFIELD_LAST_IE,                     nullptr}, // octetDeltaCount
    };

    struct exp_template_params tmplt;
    tmplt.id =  65535;
    tmplt.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt.opts_types = 0; // Unknown type
    tmplt.fields_cnt_total = fields.size();
    tmplt.fields_cnt_scope = 1;
    tmplt.flags = 0;

    template_tester(tmplt, fields);
}

TEST(Parse, OptionsMeteringProcessStat)
{
    // Basic version (1 scope field)
    const std::vector<struct exp_field_params> fields_basic = {
        //  id - en - len - flags - elem
        {149, 0, 4, FDS_TFIELD_LAST_IE | FDS_TFIELD_SCOPE, nullptr}, // observationDomainId
        { 40, 0, 8, FDS_TFIELD_LAST_IE,                    nullptr}, // exportedOctetTotalCount
        { 41, 0, 8, FDS_TFIELD_LAST_IE,                    nullptr}, // exportedMessageTotalCount
        { 42, 0, 8, FDS_TFIELD_LAST_IE,                    nullptr}, // exportedFlowRecordTotalCount
        {164, 0, 4, FDS_TFIELD_LAST_IE,                    nullptr}  // (extra) ignoredPacketTotalCount
    };

    struct exp_template_params tmplt_basic;
    tmplt_basic.id =  65535;
    tmplt_basic.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt_basic.opts_types = FDS_OPTS_MPROC_STAT;
    tmplt_basic.fields_cnt_total = fields_basic.size();
    tmplt_basic.fields_cnt_scope = 1;
    tmplt_basic.flags = 0;
    {
        SCOPED_TRACE("Basic version (ODID non-zero)");
        template_tester(tmplt_basic, fields_basic);
    }

    // Basic version (1 scope field)
    const std::vector<struct exp_field_params> fields_basic2 = {
        //  id - en - len - flags - elem
        {143, 0, 4, FDS_TFIELD_LAST_IE | FDS_TFIELD_SCOPE, nullptr}, // meteringProcessId
        { 40, 0, 8, FDS_TFIELD_LAST_IE,                    nullptr}, // exportedOctetTotalCount
        { 41, 0, 8, FDS_TFIELD_LAST_IE,                    nullptr}, // exportedMessageTotalCount
        { 42, 0, 8, FDS_TFIELD_LAST_IE,                    nullptr}, // exportedFlowRecordTotalCount
        {164, 0, 4, FDS_TFIELD_LAST_IE,                    nullptr}  // (extra) ignoredPacketTotalCount
    };

    struct exp_template_params tmplt_basic2;
    tmplt_basic2.id =  65535;
    tmplt_basic2.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt_basic2.opts_types = FDS_OPTS_MPROC_STAT;
    tmplt_basic2.fields_cnt_total = fields_basic2.size();
    tmplt_basic2.fields_cnt_scope = 1;
    tmplt_basic2.flags = 0;
    {
        SCOPED_TRACE("Basic version (ODID zero)");
        template_tester(tmplt_basic2, fields_basic2);
    }


    // Extended version (2 scope fields)
    const std::vector<struct exp_field_params> fields_long = {
        //  id - en - len - flags - elem
        {149, 0, 4, FDS_TFIELD_LAST_IE | FDS_TFIELD_SCOPE, nullptr}, // observationDomainId
        {143, 0, 4, FDS_TFIELD_LAST_IE | FDS_TFIELD_SCOPE, nullptr}, // meteringProcessId
        { 40, 0, 8, FDS_TFIELD_LAST_IE,                    nullptr}, // exportedOctetTotalCount
        { 41, 0, 8, FDS_TFIELD_LAST_IE,                    nullptr}, // exportedMessageTotalCount
        { 42, 0, 8, FDS_TFIELD_LAST_IE,                    nullptr}, // exportedFlowRecordTotalCount
    };

    struct exp_template_params tmplt_long;
    tmplt_long.id = 300;
    tmplt_long.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt_long.opts_types = FDS_OPTS_MPROC_STAT;
    tmplt_long.fields_cnt_total = fields_long.size();
    tmplt_long.fields_cnt_scope = 2;
    tmplt_long.flags = 0;
    {
        SCOPED_TRACE("Extended version");
        template_tester(tmplt_long, fields_long);
    }

    // Invalid scope field
    const std::vector<struct exp_field_params> fields_err_scope1 = {
        //  id - en - len - flags - elem
        { 40, 0, 8, FDS_TFIELD_LAST_IE | FDS_TFIELD_SCOPE, nullptr}, // exportedOctetTotalCount
        {149, 0, 4, FDS_TFIELD_LAST_IE,                    nullptr}, // observationDomainId
        { 41, 0, 8, FDS_TFIELD_LAST_IE,                    nullptr}, // exportedMessageTotalCount
        { 42, 0, 8, FDS_TFIELD_LAST_IE,                    nullptr}, // exportedFlowRecordTotalCount
        {164, 0, 4, FDS_TFIELD_LAST_IE,                    nullptr}  // (extra) ignoredPacketTotalCount
    };

    struct exp_template_params tmplt_err_scope1;
    tmplt_err_scope1.id =  5004;
    tmplt_err_scope1.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt_err_scope1.opts_types = 0;
    tmplt_err_scope1.fields_cnt_total = fields_err_scope1.size();
    tmplt_err_scope1.fields_cnt_scope = 1;
    tmplt_err_scope1.flags = 0;
    {
        SCOPED_TRACE("Invalid scope field");
        template_tester(tmplt_err_scope1, fields_err_scope1);
    }

    // Missing scope field
    const std::vector<struct exp_field_params> fields_err_scope2 = {
        //  id - en - len - flags - elem
        {149, 0, 4, FDS_TFIELD_LAST_IE | FDS_TFIELD_SCOPE, nullptr}, // observationDomainId
        {143, 0, 4, FDS_TFIELD_LAST_IE,                    nullptr}, // meteringProcessId
        { 40, 0, 8, FDS_TFIELD_LAST_IE,                    nullptr}, // exportedOctetTotalCount
        { 41, 0, 8, FDS_TFIELD_LAST_IE,                    nullptr}, // exportedMessageTotalCount
        { 42, 0, 8, FDS_TFIELD_LAST_IE,                    nullptr}, // exportedFlowRecordTotalCount
        {164, 0, 4, FDS_TFIELD_LAST_IE,                    nullptr}  // (extra) ignoredPacketTotalCount
    };

    struct exp_template_params tmplt_err_scope2;
    tmplt_err_scope2.id =  37241;
    tmplt_err_scope2.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt_err_scope2.opts_types = 0;
    tmplt_err_scope2.fields_cnt_total = fields_err_scope2.size();
    tmplt_err_scope2.fields_cnt_scope = 1;
    tmplt_err_scope2.flags = 0;
    {
        SCOPED_TRACE("Missing scope field");
        template_tester(tmplt_err_scope2, fields_err_scope2);
    }
}

TEST(Parse, OptionsMeteringProcessReliabilityStat)
{
    const fds_template_flag_t flg_multi = FDS_TFIELD_MULTI_IE;
    const fds_template_flag_t flg_last = FDS_TFIELD_LAST_IE;
    const fds_template_flag_t flg_ml = flg_last | flg_multi;

    // Basic version (1 scope field)
    const std::vector<struct exp_field_params> fields_basic = {
        //  id - en - len - flags - elem
        {149, 0, 4, flg_last | FDS_TFIELD_SCOPE, nullptr}, // observationDomainId
        {164, 0, 8, flg_last,                    nullptr}, // ignoredPacketTotalCount
        {165, 0, 8, flg_last,                    nullptr}, // ignoredOctetTotalCount
        {323, 0, 8, flg_multi,                   nullptr}, // observationTimeMilliseconds
        {323, 0, 8, flg_ml,                      nullptr}, // observationTimeMilliseconds
        {166, 0, 8, flg_last,                    nullptr}  // (extra) notSentFlowTotalCount
    };

    struct exp_template_params tmplt_basic;
    tmplt_basic.id =  65535;
    tmplt_basic.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt_basic.opts_types = FDS_OPTS_MPROC_RELIABILITY_STAT;
    tmplt_basic.fields_cnt_total = fields_basic.size();
    tmplt_basic.fields_cnt_scope = 1;
    tmplt_basic.flags = FDS_TEMPLATE_HAS_MULTI_IE;
    {
        SCOPED_TRACE("Basic version (ODID non-zero)");
        template_tester(tmplt_basic, fields_basic);
    }

    // Basic version (1 scope field)
    const std::vector<struct exp_field_params> fields_basic2 = {
        //  id - en - len - flags - elem
        {143, 0, 4, flg_last | FDS_TFIELD_SCOPE, nullptr}, // meteringProcessId
        {164, 0, 8, flg_last,                    nullptr}, // ignoredPacketTotalCount
        {165, 0, 8, flg_last,                    nullptr}, // ignoredOctetTotalCount
        {325, 0, 8, flg_multi,                   nullptr}, // observationTimeNanoseconds
        {325, 0, 8, flg_ml,                      nullptr}, // observationTimeNanoseconds
        {166, 0, 8, flg_last,                    nullptr}  // (extra) notSentFlowTotalCount
    };

    struct exp_template_params tmplt_basic2;
    tmplt_basic2.id =  53722;
    tmplt_basic2.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt_basic2.opts_types = FDS_OPTS_MPROC_RELIABILITY_STAT;
    tmplt_basic2.fields_cnt_total = fields_basic2.size();
    tmplt_basic2.fields_cnt_scope = 1;
    tmplt_basic2.flags = FDS_TEMPLATE_HAS_MULTI_IE;
    {
        SCOPED_TRACE("Basic version (ODID zero)");
        template_tester(tmplt_basic2, fields_basic2);
    }

    // Extended version (2 scope fields)
    const std::vector<struct exp_field_params> fields_long = {
        //  id - en - len - flags - elem
        {149, 0, 4, flg_last | FDS_TFIELD_SCOPE, nullptr}, // observationDomainId
        {143, 0, 4, flg_last | FDS_TFIELD_SCOPE, nullptr}, // meteringProcessId
        {164, 0, 8, flg_last,                    nullptr}, // ignoredPacketTotalCount
        {165, 0, 8, flg_last,                    nullptr}, // ignoredOctetTotalCount
        {322, 0, 4, flg_last,                    nullptr}, // observationTimeSeconds
        {324, 0, 8, flg_last,                    nullptr}  // observationTimeMicroseconds
    };

    struct exp_template_params tmplt_long;
    tmplt_long.id = 42731;
    tmplt_long.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt_long.opts_types = FDS_OPTS_MPROC_RELIABILITY_STAT;
    tmplt_long.fields_cnt_total = fields_long.size();
    tmplt_long.fields_cnt_scope = 2;
    tmplt_long.flags = 0;
    {
        SCOPED_TRACE("Extended version");
        template_tester(tmplt_long, fields_long);
    }

    // Invalid scope field
    const std::vector<struct exp_field_params> fields_err_scope1 = {
        //  id - en - len - flags - elem
        {164, 0, 8, flg_last | FDS_TFIELD_SCOPE, nullptr}, // ignoredPacketTotalCount
        {149, 0, 4, flg_last,                    nullptr}, // observationDomainId
        {165, 0, 8, flg_last,                    nullptr}, // ignoredOctetTotalCount
        {323, 0, 8, flg_multi,                   nullptr}, // observationTimeMilliseconds
        {323, 0, 8, flg_ml,                      nullptr}, // observationTimeMilliseconds
        {166, 0, 8, flg_last,                    nullptr}  // (extra) notSentFlowTotalCount
    };

    struct exp_template_params tmplt_err_scope1;
    tmplt_err_scope1.id =  62611;
    tmplt_err_scope1.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt_err_scope1.opts_types = 0;
    tmplt_err_scope1.fields_cnt_total = fields_err_scope1.size();
    tmplt_err_scope1.fields_cnt_scope = 1;
    tmplt_err_scope1.flags = FDS_TEMPLATE_HAS_MULTI_IE;
    {
        SCOPED_TRACE("Invalid scope field");
        template_tester(tmplt_err_scope1, fields_err_scope1);
    }

    // Missing scope field
    const std::vector<struct exp_field_params> fields_err_scope2 = {
        //  id - en - len - flags - elem
        {149, 0, 4, flg_last | FDS_TFIELD_SCOPE, nullptr}, // observationDomainId
        {143, 0, 4, flg_last,                    nullptr}, // meteringProcessId
        {164, 0, 8, flg_last,                    nullptr}, // ignoredPacketTotalCount
        {165, 0, 8, flg_last,                    nullptr}, // ignoredOctetTotalCount
        {323, 0, 8, flg_multi,                   nullptr}, // observationTimeMilliseconds
        {323, 0, 8, flg_ml,                      nullptr}, // observationTimeMilliseconds
    };

    struct exp_template_params tmplt_err_scope2;
    tmplt_err_scope2.id =  37241;
    tmplt_err_scope2.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt_err_scope2.opts_types = 0;
    tmplt_err_scope2.fields_cnt_total = fields_err_scope2.size();
    tmplt_err_scope2.fields_cnt_scope = 1;
    tmplt_err_scope2.flags = FDS_TEMPLATE_HAS_MULTI_IE;
    {
        SCOPED_TRACE("Missing scope field");
        template_tester(tmplt_err_scope2, fields_err_scope2);
    }
}

// Combination of two options templates together
TEST(Parse, OptionsMeteringStatCombination)
{
    const fds_template_flag_t flg_multi = FDS_TFIELD_MULTI_IE;
    const fds_template_flag_t flg_last = FDS_TFIELD_LAST_IE;
    const fds_template_flag_t flg_ml = flg_last | flg_multi;

    // Basic version (1 scope field)
    const std::vector<struct exp_field_params> fields_basic = {
        //  id - en - len - flags - elem
        {149, 0, 4, flg_last | FDS_TFIELD_SCOPE, nullptr}, // observationDomainId
        {164, 0, 8, flg_last,                    nullptr}, // ignoredPacketTotalCount
        {165, 0, 8, flg_last,                    nullptr}, // ignoredOctetTotalCount
        { 40, 0, 8, flg_last,                    nullptr}, // exportedOctetTotalCount
        { 41, 0, 8, flg_last,                    nullptr}, // exportedMessageTotalCount
        { 42, 0, 8, flg_last,                    nullptr}, // exportedFlowRecordTotalCount
        {323, 0, 8, flg_multi,                   nullptr}, // observationTimeMilliseconds
        {323, 0, 8, flg_ml,                      nullptr}, // observationTimeMilliseconds
        {166, 0, 8, flg_last,                    nullptr}  // (extra) notSentFlowTotalCount
    };

    struct exp_template_params tmplt_basic;
    tmplt_basic.id =  25253;
    tmplt_basic.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt_basic.opts_types = FDS_OPTS_MPROC_RELIABILITY_STAT | FDS_OPTS_MPROC_STAT;
    tmplt_basic.fields_cnt_total = fields_basic.size();
    tmplt_basic.fields_cnt_scope = 1;
    tmplt_basic.flags = FDS_TEMPLATE_HAS_MULTI_IE;
    {
        SCOPED_TRACE("Basic version (ODID non-zero)");
        template_tester(tmplt_basic, fields_basic);
    }

    // Extended version (2 scope fields)
    const std::vector<struct exp_field_params> fields_long = {
        //  id - en - len - flags - elem
        {143, 0, 4, flg_last | FDS_TFIELD_SCOPE, nullptr}, // meteringProcessId
        {149, 0, 4, flg_last | FDS_TFIELD_SCOPE, nullptr}, // observationDomainId
        {164, 0, 8, flg_last,                    nullptr}, // ignoredPacketTotalCount
        {165, 0, 8, flg_last,                    nullptr}, // ignoredOctetTotalCount
        { 40, 0, 8, flg_last,                    nullptr}, // exportedOctetTotalCount
        { 41, 0, 8, flg_last,                    nullptr}, // exportedMessageTotalCount
        { 42, 0, 8, flg_last,                    nullptr}, // exportedFlowRecordTotalCount
        {323, 0, 8, flg_multi,                   nullptr}, // observationTimeMilliseconds
        {323, 0, 8, flg_ml,                      nullptr}, // observationTimeMilliseconds
        {166, 0, 8, flg_last,                    nullptr}  // (extra) notSentFlowTotalCount
    };

    struct exp_template_params tmplt_long;
    tmplt_long.id = 42731;
    tmplt_long.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt_long.opts_types = FDS_OPTS_MPROC_RELIABILITY_STAT | FDS_OPTS_MPROC_STAT;
    tmplt_long.fields_cnt_total = fields_long.size();
    tmplt_long.fields_cnt_scope = 2;
    tmplt_long.flags = FDS_TEMPLATE_HAS_MULTI_IE;
    {
        SCOPED_TRACE("Extended version");
        template_tester(tmplt_long, fields_long);
    }
}

TEST(Parse, OptionsExportingProcessReliabilityStat)
{
    const fds_template_flag_t flg_multi = FDS_TFIELD_MULTI_IE;
    const fds_template_flag_t flg_last = FDS_TFIELD_LAST_IE;
    const fds_template_flag_t flg_ml = flg_last | flg_multi;

    // IPv4 version (IPv4 address as the scope field)
    const std::vector<struct exp_field_params> fields_ipv4 = {
        //  id - en - len - flags - elem
        {130, 0, 4, flg_last | FDS_TFIELD_SCOPE, nullptr}, // exporterIPv4Address
        {166, 0, 8, flg_last,                    nullptr}, // notSentFlowTotalCount
        {167, 0, 8, flg_last,                    nullptr}, // notSentPacketTotalCount
        {168, 0, 8, flg_last,                    nullptr}, // notSentOctetTotalCount
        {323, 0, 8, flg_multi,                   nullptr}, // observationTimeMilliseconds
        {323, 0, 8, flg_ml,                      nullptr}, // observationTimeMilliseconds
        {164, 0, 4, FDS_TFIELD_LAST_IE,          nullptr}  // (extra) ignoredPacketTotalCount
    };

    struct exp_template_params tmplt_ipv4;
    tmplt_ipv4.id =  2242;
    tmplt_ipv4.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt_ipv4.opts_types = FDS_OPTS_EPROC_RELIABILITY_STAT;
    tmplt_ipv4.fields_cnt_total = fields_ipv4.size();
    tmplt_ipv4.fields_cnt_scope = 1;
    tmplt_ipv4.flags = FDS_TEMPLATE_HAS_MULTI_IE;
    {
        SCOPED_TRACE("IPv4 version");
        template_tester(tmplt_ipv4, fields_ipv4);
    }

    // IPv6 version (IPv6 address as the scope field)
    const std::vector<struct exp_field_params> fields_ipv6 = {
        //  id - en - len - flags - elem
        {131, 0, 16, flg_last | FDS_TFIELD_SCOPE, nullptr}, // exporterIPv6Address
        {166, 0,  8, flg_last,                    nullptr}, // notSentFlowTotalCount
        {167, 0,  8, flg_last,                    nullptr}, // notSentPacketTotalCount
        {168, 0,  8, flg_last,                    nullptr}, // notSentOctetTotalCount
        {324, 0,  8, flg_multi,                   nullptr}, // observationTimeMicroseconds
        {324, 0,  8, flg_ml,                      nullptr}, // observationTimeMicroseconds
    };

    struct exp_template_params tmplt_ipv6;
    tmplt_ipv6.id =  26112;
    tmplt_ipv6.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt_ipv6.opts_types = FDS_OPTS_EPROC_RELIABILITY_STAT;
    tmplt_ipv6.fields_cnt_total = fields_ipv6.size();
    tmplt_ipv6.fields_cnt_scope = 1;
    tmplt_ipv6.flags = FDS_TEMPLATE_HAS_MULTI_IE;
    {
        SCOPED_TRACE("IPv6 version");
        template_tester(tmplt_ipv6, fields_ipv6);
    }

    // Exporting Process version
    const std::vector<struct exp_field_params> fields_exproc = {
        //  id - en - len - flags - elem
        {144, 0,  4, flg_last | FDS_TFIELD_SCOPE, nullptr}, // exportingProcessId
        {324, 0,  8, flg_last,                    nullptr}, // observationTimeMicroseconds
        {325, 0,  8, flg_last,                    nullptr}, // observationTimeNanoseconds
        {166, 0,  8, flg_last,                    nullptr}, // notSentFlowTotalCount
        {167, 0,  8, flg_last,                    nullptr}, // notSentPacketTotalCount
        {168, 0,  8, flg_last,                    nullptr}, // notSentOctetTotalCount

    };

    struct exp_template_params tmplt_exproc;
    tmplt_exproc.id = 300;
    tmplt_exproc.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt_exproc.opts_types = FDS_OPTS_EPROC_RELIABILITY_STAT;
    tmplt_exproc.fields_cnt_total = fields_exproc.size();
    tmplt_exproc.fields_cnt_scope = 1;
    tmplt_exproc.flags = 0;
    {
        SCOPED_TRACE("Exporting Process version");
        template_tester(tmplt_exproc, fields_exproc);
    }

    // Missing timestamp
    const std::vector<struct exp_field_params> fields_err_ts = {
        //  id - en - len - flags - elem
        {144, 0,  4, flg_last | FDS_TFIELD_SCOPE, nullptr}, // exportingProcessId
        {325, 0,  8, flg_last,                    nullptr}, // observationTimeNanoseconds
        {166, 0,  8, flg_last,                    nullptr}, // notSentFlowTotalCount
        {167, 0,  8, flg_last,                    nullptr}, // notSentPacketTotalCount
        {168, 0,  8, flg_last,                    nullptr}, // notSentOctetTotalCount
    };

    struct exp_template_params tmplt_err_ts;
    tmplt_err_ts.id =  11221;
    tmplt_err_ts.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt_err_ts.opts_types = 0;
    tmplt_err_ts.fields_cnt_total = fields_err_ts.size();
    tmplt_err_ts.fields_cnt_scope = 1;
    tmplt_err_ts.flags = 0;
    {
        SCOPED_TRACE("Missing timestamp");
        template_tester(tmplt_err_ts, fields_err_ts);
    }
}

TEST(Parse, OptionsFlowKey)
{
    const std::vector<struct exp_field_params> fields_ok = {
        //  id - en - len - flags - elem
        {145, 0, 2, FDS_TFIELD_LAST_IE | FDS_TFIELD_SCOPE, nullptr}, // templateId
        {173, 0, 8, FDS_TFIELD_LAST_IE,                    nullptr}, // flowKeyIndicator
    };

    struct exp_template_params tmplt_ok;
    tmplt_ok.id =  36621;
    tmplt_ok.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt_ok.opts_types = FDS_OPTS_FKEYS;
    tmplt_ok.fields_cnt_total = fields_ok.size();
    tmplt_ok.fields_cnt_scope = 1;
    tmplt_ok.flags = 0;
    {
        SCOPED_TRACE("Correct template");
        template_tester(tmplt_ok, fields_ok);
    }

    // Missing key
    const std::vector<struct exp_field_params> fields_err = {
        //  id - en - len - flags - elem
        {145, 0, 2, FDS_TFIELD_LAST_IE | FDS_TFIELD_SCOPE, nullptr}, // templateId
    };

    struct exp_template_params tmplt_err;
    tmplt_err.id =  36621;
    tmplt_err.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt_err.opts_types = 0;
    tmplt_err.fields_cnt_total = fields_err.size();
    tmplt_err.fields_cnt_scope = 1;
    tmplt_err.flags = 0;
    {
        SCOPED_TRACE("Invalid Flow key template");
        template_tester(tmplt_err, fields_err);
    }
}

TEST(Parse, OptionsIEType)
{
    const std::vector<struct exp_field_params> fields_full = {
        //  id - en - len - flags - elem
        {303, 0,      2, FDS_TFIELD_LAST_IE | FDS_TFIELD_SCOPE, nullptr}, // informationElementId
        {346, 0,      4, FDS_TFIELD_LAST_IE | FDS_TFIELD_SCOPE, nullptr}, // privateEnterpriseNumber
        {339, 0,      1, FDS_TFIELD_LAST_IE,                    nullptr}, // informationElementDataType
        {344, 0,      1, FDS_TFIELD_LAST_IE,                    nullptr}, // informationElementSemantics
        {345, 0,      1, FDS_TFIELD_LAST_IE,                    nullptr}, // informationElementUnits
        {342, 0,      8, FDS_TFIELD_LAST_IE,                    nullptr}, // informationElementRangeBegin
        {343, 0,      8, FDS_TFIELD_LAST_IE,                    nullptr}, // informationElementRangeEnd
        {341, 0, VAR_IE, FDS_TFIELD_LAST_IE,                    nullptr}, // informationElementName
        {340, 0, VAR_IE, FDS_TFIELD_LAST_IE,                    nullptr}, // informationElementDescription
    };

    struct exp_template_params tmplt_full;
    tmplt_full.id =  333;
    tmplt_full.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt_full.opts_types = FDS_OPTS_IE_TYPE;
    tmplt_full.fields_cnt_total = fields_full.size();
    tmplt_full.fields_cnt_scope = 2;
    tmplt_full.flags = FDS_TEMPLATE_HAS_DYNAMIC;
    {
        SCOPED_TRACE("Full template");
        template_tester(tmplt_full, fields_full);
    }

    // Only required fields
    const std::vector<struct exp_field_params> fields_min = {
        //  id - en - len - flags - elem
        {346, 0,      4, FDS_TFIELD_LAST_IE | FDS_TFIELD_SCOPE, nullptr}, // privateEnterpriseNumber
        {303, 0,      2, FDS_TFIELD_LAST_IE | FDS_TFIELD_SCOPE, nullptr}, // informationElementId
        {339, 0,      1, FDS_TFIELD_LAST_IE,                    nullptr}, // informationElementDataType
        {344, 0,      1, FDS_TFIELD_LAST_IE,                    nullptr}, // informationElementSemantics
        {341, 0, VAR_IE, FDS_TFIELD_LAST_IE,                    nullptr}, // informationElementName
    };

    struct exp_template_params tmplt_min;
    tmplt_min.id =  9892;
    tmplt_min.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt_min.opts_types = FDS_OPTS_IE_TYPE;
    tmplt_min.fields_cnt_total = fields_min.size();
    tmplt_min.fields_cnt_scope = 2;
    tmplt_min.flags = FDS_TEMPLATE_HAS_DYNAMIC;
    {
        SCOPED_TRACE("Minimal template");
        template_tester(tmplt_min, fields_min);
    }

    // Missing IE ID
    const std::vector<struct exp_field_params> fields_err_ie = {
        //  id - en - len - flags - elem
        {346, 0,      4, FDS_TFIELD_LAST_IE | FDS_TFIELD_SCOPE, nullptr}, // privateEnterpriseNumber
        {339, 0,      1, FDS_TFIELD_LAST_IE,                    nullptr}, // informationElementDataType
        {344, 0,      1, FDS_TFIELD_LAST_IE,                    nullptr}, // informationElementSemantics
        {341, 0, VAR_IE, FDS_TFIELD_LAST_IE,                    nullptr}, // informationElementName
    };

    struct exp_template_params tmplt_ok;
    tmplt_ok.id =  8881;
    tmplt_ok.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt_ok.opts_types = 0;
    tmplt_ok.fields_cnt_total = fields_err_ie.size();
    tmplt_ok.fields_cnt_scope = 1;
    tmplt_ok.flags = FDS_TEMPLATE_HAS_DYNAMIC;
    {
        SCOPED_TRACE("Missing IE ID");
        template_tester(tmplt_ok, fields_err_ie);
    }

    // Missing Enterprise ID
    const std::vector<struct exp_field_params> fields_err = {
        //  id - en - len - flags - elem
        {303, 0,      2, FDS_TFIELD_LAST_IE | FDS_TFIELD_SCOPE, nullptr}, // informationElementId
        {339, 0,      1, FDS_TFIELD_LAST_IE,                    nullptr}, // informationElementDataType
        {344, 0,      1, FDS_TFIELD_LAST_IE,                    nullptr}, // informationElementSemantics
        {341, 0, VAR_IE, FDS_TFIELD_LAST_IE,                    nullptr}, // informationElementName
    };

    struct exp_template_params tmplt_err;
    tmplt_err.id =  7722;
    tmplt_err.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt_err.opts_types = 0;
    tmplt_err.fields_cnt_total = fields_err.size();
    tmplt_err.fields_cnt_scope = 1;
    tmplt_err.flags = FDS_TEMPLATE_HAS_DYNAMIC;
    {
        SCOPED_TRACE("Missing Private Enterprise Number");
        template_tester(tmplt_err, fields_err);
    }
}

// INVALID TEMPLATES ===============================================================================

// TODO
// Invalid header ID

// Too short header

// Too short template

// Missing fields

// Missing enterprise number

// Too long data section

// Too many elements

