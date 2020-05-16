#include <gtest/gtest.h>
#include <libfds.h>

// Template generator
#include <TGenerator.h>
#include <common_tests.h>
#include <cstring>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class IEs : public ::testing::Test {
private:
    // File with few IANA elements
    const char *ie_path = "data/iana.xml";
protected:

    fds_iemgr_t *ie_mgr = nullptr;
    /** \brief Prepare IE DB */
    void SetUp() override {
        ie_mgr = fds_iemgr_create();
        if (!ie_mgr) {
            throw std::runtime_error("IPFIX IE Manager is not ready!");
        }

        if (fds_iemgr_read_file(ie_mgr, ie_path, true) != FDS_OK) {
            throw std::runtime_error("Failed to load Information Elements: "
                + std::string(fds_iemgr_last_err(ie_mgr)));
        }
    }

    /** \brief Destroy IE DB */
    void TearDown() override {
        fds_iemgr_destroy(ie_mgr);
    }
};

constexpr uint16_t VAR_IE = 65535;

// Template definition for tests
struct ie_template_params {
    uint16_t id;
    enum fds_template_type type;
    fds_template_flag_t flags;
    uint16_t scope_fields;
};

// Template field definition for tests
struct ie_field_params {
    uint16_t id;
    uint32_t en;
    uint16_t len;
    fds_template_flag_t flags;
    fds_iemgr_element_type type;
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
template_create(const ie_template_params &tmplt, const std::vector<struct ie_field_params> &fields,
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
 * \param[in] tmplt   Template parameters
 * \param[in] fields  Vector of field parameters
 * \param[in] in      Parsed FDS template
 * \param[in] reverse Test reverse template fields (biflow template only)
 */
void
template_tester(const ie_template_params &tmplt, const std::vector<struct ie_field_params> &fields,
    uniq_fds_tmplt &in, bool reverse = false)
{
    const struct fds_template *tmplt_rec = in.get();
    if (reverse && tmplt_rec->rev_dir == nullptr) {
        FAIL() << "Reverse template fields are not defined!";
    }

    // Check test integrity
    ASSERT_EQ(tmplt_rec->fields_cnt_total, fields.size());
    ASSERT_EQ(tmplt_rec->fields_cnt_scope, tmplt.scope_fields);

    for (uint16_t i = 0; i <tmplt_rec->fields_cnt_total; ++i) {
        SCOPED_TRACE("Testing field ID: " + std::to_string(i));
        const fds_tfield *field = (!reverse)
            ? &tmplt_rec->fields[i]
            : &tmplt_rec->rev_dir->fields[i];
        // Check test integrity
        ASSERT_EQ(field->id, fields[i].id);
        ASSERT_EQ(field->en, fields[i].en);

        if (fields[i].type != FDS_ET_UNASSIGNED) {
            ASSERT_NE(field->def, nullptr);
            EXPECT_EQ(field->def->data_type, fields[i].type);
        }

        ct_tfield_flags(field, fields[i].flags);
    }

    // Check template flags
    ct_template_flags(tmplt_rec, tmplt.flags);
}

// Basic uniflow template
TEST_F(IEs, StandardFlow)
{
    struct ie_template_params tmplt;
    tmplt.id = 256;
    tmplt.type = FDS_TYPE_TEMPLATE;
    tmplt.flags = 0;
    tmplt.scope_fields = 0;

    constexpr fds_template_flag_t flg_comm = FDS_TFIELD_LAST_IE;
    const std::vector<struct ie_field_params> fields = {
        // id - en - len - type - flags
        {  8, 0, 4, flg_comm, FDS_ET_IPV4_ADDRESS}, // sourceIPv4Address
        { 12, 0, 4, flg_comm, FDS_ET_IPV4_ADDRESS}, // destinationIPv4Address
        {  7, 0, 2, flg_comm, FDS_ET_UNSIGNED_16}, // sourceTransportPort
        { 11, 0, 2, flg_comm, FDS_ET_UNSIGNED_16}, // destinationTransportPort
        {  4, 0, 1, flg_comm, FDS_ET_UNSIGNED_8},  // protocolIdentifier
        {  6, 0, 1, flg_comm, FDS_ET_UNSIGNED_16}, // tcpControlBits
        {152, 0, 8, flg_comm, FDS_ET_DATE_TIME_MILLISECONDS}, // flowStartMilliseconds
        {153, 0, 8, flg_comm, FDS_ET_DATE_TIME_MILLISECONDS}, // flowEndMilliseconds
        {  2, 0, 4, flg_comm, FDS_ET_UNSIGNED_64}, // packetDeltaCount
        {  1, 0, 4, flg_comm, FDS_ET_UNSIGNED_64}  // octetDeltaCount
    };

    uniq_fds_tmplt aux_template(nullptr, &::fds_template_destroy);
    template_create(tmplt, fields, aux_template);
    // Add definitions
    fds_template_ies_define(aux_template.get(), ie_mgr, false);
    template_tester(tmplt, fields, aux_template);
    // Biflow should be undefined
    EXPECT_EQ(aux_template.get()->rev_dir, nullptr);
}

// Basic biflow template
TEST_F(IEs, Biflow)
{
    struct ie_template_params tmplt;
    tmplt.id = 256;
    tmplt.type = FDS_TYPE_TEMPLATE;
    tmplt.flags = FDS_TEMPLATE_BIFLOW;
    tmplt.scope_fields = 0;

    constexpr fds_template_flag_t flg_comm = FDS_TFIELD_LAST_IE;
    constexpr fds_template_flag_t flg_rev = flg_comm | FDS_TFIELD_REVERSE;
    constexpr fds_template_flag_t flg_key = flg_comm | FDS_TFIELD_BKEY;

    const std::vector<struct ie_field_params> fields = {
        // id - en - len - type - flags
        {  8, 0, 4, flg_key,     FDS_ET_IPV4_ADDRESS}, // sourceIPv4Address
        { 12, 0, 4, flg_key,     FDS_ET_IPV4_ADDRESS}, // destinationIPv4Address
        {  7, 0, 2, flg_key,     FDS_ET_UNSIGNED_16},  // sourceTransportPort
        { 11, 0, 2, flg_key,     FDS_ET_UNSIGNED_16},  // destinationTransportPort
        {  4, 0, 1, flg_key,     FDS_ET_UNSIGNED_8},   // protocolIdentifier
        {  6, 0, 1, flg_comm,    FDS_ET_UNSIGNED_16},  // tcpControlBits
        {152, 0, 8, flg_comm,    FDS_ET_DATE_TIME_MILLISECONDS}, // flowStartMilliseconds
        {153, 0, 8, flg_comm,    FDS_ET_DATE_TIME_MILLISECONDS}, // flowEndMilliseconds
        {  2, 0, 4, flg_comm,    FDS_ET_UNSIGNED_64},  // packetDeltaCount
        {  1, 0, 4, flg_comm,    FDS_ET_UNSIGNED_64},  // octetDeltaCount
        {  6, 29305, 1, flg_rev, FDS_ET_UNSIGNED_16},  // tcpControlBits (reverse)
        {152, 29305, 8, flg_rev, FDS_ET_DATE_TIME_MILLISECONDS}, // flowStartMilliseconds (reverse)
        {153, 29305, 8, flg_rev, FDS_ET_DATE_TIME_MILLISECONDS}, // flowEndMilliseconds (reverse)
        {  2, 29305, 4, flg_rev, FDS_ET_UNSIGNED_64},  // packetDeltaCount (reverse)
        {  1, 29305, 4, flg_rev, FDS_ET_UNSIGNED_64},  // octetDeltaCount (reverse)
        // Also add a random field with unknown definition -> should be marked as common
        { 50, 10000, 4, flg_key, FDS_ET_UNASSIGNED}
    };

    uniq_fds_tmplt aux_template(nullptr, &::fds_template_destroy);
    template_create(tmplt, fields, aux_template);
    // Add definitions
    fds_template_ies_define(aux_template.get(), ie_mgr, false);
    template_tester(tmplt, fields, aux_template);

    // Reverse template (biflow)
    const std::vector<struct ie_field_params> fields_rev = {
        // id - en - len - type - flags
        { 12, 0, 4, flg_key,     FDS_ET_IPV4_ADDRESS}, // destinationIPv4Address
        {  8, 0, 4, flg_key,     FDS_ET_IPV4_ADDRESS}, // sourceIPv4Address
        { 11, 0, 2, flg_key,     FDS_ET_UNSIGNED_16},  // destinationTransportPort
        {  7, 0, 2, flg_key,     FDS_ET_UNSIGNED_16},  // sourceTransportPort
        {  4, 0, 1, flg_key,     FDS_ET_UNSIGNED_8},   // protocolIdentifier
        {  6, 29305, 1, flg_rev, FDS_ET_UNSIGNED_16},  // tcpControlBits (reverse)
        {152, 29305, 8, flg_rev, FDS_ET_DATE_TIME_MILLISECONDS}, // flowStartMilliseconds (reverse)
        {153, 29305, 8, flg_rev, FDS_ET_DATE_TIME_MILLISECONDS}, // flowEndMilliseconds (reverse)
        {  2, 29305, 4, flg_rev, FDS_ET_UNSIGNED_64},  // packetDeltaCount (reverse)
        {  1, 29305, 4, flg_rev, FDS_ET_UNSIGNED_64},   // octetDeltaCount (reverse)
        {  6, 0, 1, flg_comm,    FDS_ET_UNSIGNED_16},  // tcpControlBits
        {152, 0, 8, flg_comm,    FDS_ET_DATE_TIME_MILLISECONDS}, // flowStartMilliseconds
        {153, 0, 8, flg_comm,    FDS_ET_DATE_TIME_MILLISECONDS}, // flowEndMilliseconds
        {  2, 0, 4, flg_comm,    FDS_ET_UNSIGNED_64},  // packetDeltaCount
        {  1, 0, 4, flg_comm,    FDS_ET_UNSIGNED_64},  // octetDeltaCount
        // Also add a random field with unknown definition -> should be marked as common
        { 50, 10000, 4, flg_key, FDS_ET_UNASSIGNED}
    };

    template_tester(tmplt, fields_rev, aux_template, true);
}

// Test preservation and removing of all templates
TEST_F(IEs, preserveAndRemove)
{
    struct ie_template_params tmplt;
    tmplt.id = 1000;
    tmplt.type = FDS_TYPE_TEMPLATE;
    tmplt.flags = FDS_TEMPLATE_BIFLOW;
    tmplt.scope_fields = 0;

    constexpr fds_template_flag_t flg_comm = FDS_TFIELD_LAST_IE;
    constexpr fds_template_flag_t flg_rev = flg_comm | FDS_TFIELD_REVERSE;
    constexpr fds_template_flag_t flg_key = flg_comm | FDS_TFIELD_BKEY;

    const std::vector<struct ie_field_params> fields = {
        // id - en - len - type - flags
        {8,  0, 4, flg_key, FDS_ET_IPV4_ADDRESS}, // sourceIPv4Address
        {12, 0, 4, flg_key, FDS_ET_IPV4_ADDRESS}, // destinationIPv4Address
        {7,  0, 2, flg_key, FDS_ET_UNSIGNED_16},  // sourceTransportPort
        {11, 0, 2, flg_key, FDS_ET_UNSIGNED_16},  // destinationTransportPort
        {4,  0, 1, flg_key, FDS_ET_UNSIGNED_8},   // protocolIdentifier
        {2,  0, 4, flg_comm,    FDS_ET_UNSIGNED_64},  // packetDeltaCount
        {2, 29305, 4, flg_rev,  FDS_ET_UNSIGNED_64}   // packetDeltaCount (reverse)
    };

    // Create a template and add definitions
    uniq_fds_tmplt aux_template(nullptr, &::fds_template_destroy);
    template_create(tmplt, fields, aux_template);
    fds_template_ies_define(aux_template.get(), ie_mgr, false);
    template_tester(tmplt, fields, aux_template);

    // Definitions and flags should be preserved
    fds_template_ies_define(aux_template.get(), nullptr, true);
    template_tester(tmplt, fields, aux_template);

    // Definitions and flags should be removed
    fds_template_ies_define(aux_template.get(), nullptr, false);
    for (uint16_t i = 0; i < aux_template.get()->fields_cnt_total; ++i) {
        const struct fds_tfield *field = &aux_template.get()->fields[i];
        ct_tfield_flags(field, flg_comm); // Remove flags connected to definitions
    }
    ct_template_flags(aux_template.get(), 0);
}

// Try to add new definition but preserve the old ones
TEST_F(IEs, extend)
{
    // Create a private manager with new Information Elements
    fds_iemgr_t *private_mgr = fds_iemgr_create();
    if (!private_mgr) {
        throw std::runtime_error("Failed to initialize IE manager");
    }

    struct fds_iemgr_elem elemA; // ID: 1001, PEN: 1000
    std::memset(&elemA, 0, sizeof(elemA));
    elemA.id = 1001;
    elemA.name = const_cast<char *>("myFirstElement");
    elemA.data_type = FDS_ET_FLOAT_32;
    elemA.status = FDS_ST_CURRENT;
    ASSERT_EQ(fds_iemgr_elem_add(private_mgr, &elemA, 1000, false), FDS_OK);

    struct fds_iemgr_elem elemB; // ID: 8, PEN: 0
    std::memset(&elemB, 0, sizeof(elemB));
    elemB.id = 8;
    elemB.name = const_cast<char*>("myHiddenElement");
    elemB.data_type = FDS_ET_BASIC_LIST;
    elemB.status = FDS_ST_CURRENT;
    ASSERT_EQ(fds_iemgr_elem_add(private_mgr, &elemB, 0, false), FDS_OK);

    // Create a test template
    struct ie_template_params tmplt;
    tmplt.id = 5641;
    tmplt.type = FDS_TYPE_TEMPLATE;
    tmplt.flags = 0;
    tmplt.scope_fields = 0;

    constexpr fds_template_flag_t flg_comm = FDS_TFIELD_LAST_IE;
    std::vector<struct ie_field_params> fields = {
        // id - en - len - type - flags
        { 152,    0, 8, flg_comm, FDS_ET_DATE_TIME_MILLISECONDS}, // flowStartMilliseconds
        { 153,    0, 8, flg_comm, FDS_ET_DATE_TIME_MILLISECONDS}, // flowEndMilliseconds
        {   2,    0, 4, flg_comm, FDS_ET_UNSIGNED_64}, // packetDeltaCount
        {   1,    0, 4, flg_comm, FDS_ET_UNSIGNED_64}, // octetDeltaCount
        {1001, 1000, 4, flg_comm, FDS_ET_UNASSIGNED},  // myFirstElement <--- unknown here
        {   8,    0, 4, flg_comm, FDS_ET_IPV4_ADDRESS} // sourceIPv4Address
    };

    uniq_fds_tmplt aux_template(nullptr, &::fds_template_destroy);
    template_create(tmplt, fields, aux_template);
    // Add definitions from standard list
    fds_template_ies_define(aux_template.get(), ie_mgr, false);
    template_tester(tmplt, fields, aux_template);

    // Element should be unknown here
    EXPECT_EQ(aux_template.get()->fields[4].def, nullptr);

    // Defined new elements and preserve previous definitions
    fds_template_ies_define(aux_template.get(), private_mgr, true);
    // New element should be defined now
    EXPECT_NE(aux_template.get()->fields[4].def, nullptr);
    EXPECT_STREQ(aux_template.get()->fields[4].def->name, elemA.name);
    // Old element should be preserved
    EXPECT_STRNE(aux_template.get()->fields[5].def->name, elemB.name);
    EXPECT_NE(aux_template.get()->fields[5].def->data_type, FDS_ET_BASIC_LIST);

    fields[4].type = FDS_ET_FLOAT_32;
    template_tester(tmplt, fields, aux_template);
    fds_iemgr_destroy(private_mgr);
}

// Try to replace definitions with the new ones
TEST_F(IEs, replace)
{
    // Create a private manager with new Information Elements
    fds_iemgr_t *private_mgr = fds_iemgr_create();
    if (!private_mgr) {
        throw std::runtime_error("Failed to initialize IE manager");
    }

    struct fds_iemgr_elem elemA; // ID: 1001, PEN: 1000
    std::memset(&elemA, 0, sizeof(elemA));
    elemA.id = 1001;
    elemA.name = const_cast<char *>("myFirstElement");
    elemA.data_type = FDS_ET_FLOAT_32;
    elemA.status = FDS_ST_CURRENT;
    ASSERT_EQ(fds_iemgr_elem_add(private_mgr, &elemA, 1000, false), FDS_OK);

    struct fds_iemgr_elem elemB; // ID: 8, PEN: 0
    std::memset(&elemB, 0, sizeof(elemB));
    elemB.id = 8;
    elemB.name = const_cast<char*>("myHiddenElement");
    elemB.data_type = FDS_ET_BASIC_LIST;
    elemB.status = FDS_ST_CURRENT;
    ASSERT_EQ(fds_iemgr_elem_add(private_mgr, &elemB, 0, false), FDS_OK);

    // Create a test template
    struct ie_template_params tmplt;
    tmplt.id = 5641;
    tmplt.type = FDS_TYPE_TEMPLATE;
    tmplt.flags = 0;
    tmplt.scope_fields = 0;

    constexpr fds_template_flag_t flg_comm = FDS_TFIELD_LAST_IE;
    std::vector<struct ie_field_params> fields = {
        // id - en - len - type - flags
        { 152,    0, 8, flg_comm, FDS_ET_DATE_TIME_MILLISECONDS}, // flowStartMilliseconds
        { 153,    0, 8, flg_comm, FDS_ET_DATE_TIME_MILLISECONDS}, // flowEndMilliseconds
        {   2,    0, 4, flg_comm, FDS_ET_UNSIGNED_64}, // packetDeltaCount
        {   1,    0, 4, flg_comm, FDS_ET_UNSIGNED_64}, // octetDeltaCount
        {1001, 1000, 4, flg_comm, FDS_ET_UNASSIGNED},  // myFirstElement <--- unknown here
        {   8,    0, 4, flg_comm, FDS_ET_IPV4_ADDRESS} // sourceIPv4Address
    };

    uniq_fds_tmplt aux_template(nullptr, &::fds_template_destroy);
    template_create(tmplt, fields, aux_template);
    // Add definitions from standard IE manager
    fds_template_ies_define(aux_template.get(), ie_mgr, false);
    template_tester(tmplt, fields, aux_template);

    // Element should be unknown here
    EXPECT_EQ(aux_template.get()->fields[4].def, nullptr);

    // Redefine new elements
    fds_template_ies_define(aux_template.get(), private_mgr, false); // << redefined
    // Newly expected structure and flags
    std::vector<struct ie_field_params> fields_new = {
        // id - en - len - type - flags
        { 152,    0, 8, flg_comm, FDS_ET_UNASSIGNED}, // unknown
        { 153,    0, 8, flg_comm, FDS_ET_UNASSIGNED}, // unknown
        {   2,    0, 4, flg_comm, FDS_ET_UNASSIGNED}, // unknown
        {   1,    0, 4, flg_comm, FDS_ET_UNASSIGNED}, // unknown
        {1001, 1000, 4, flg_comm, FDS_ET_FLOAT_32},  // myFirstElement <--- unknown here
        {   8,    0, 4, flg_comm | FDS_TFIELD_STRUCT, FDS_ET_BASIC_LIST} // sourceIPv4Address
    };

    // Now test it
    tmplt.flags = FDS_TEMPLATE_STRUCT;
    template_tester(tmplt, fields_new, aux_template);
    // First 4 elements should have unknown definitions
    for (uint16_t i = 0; i < 4; ++i) {
        const struct fds_tfield *field = &aux_template.get()->fields[i];
        EXPECT_EQ(field->def, nullptr);
    }

    fds_iemgr_destroy(private_mgr);
}

// Add definition of a reverse IE -> Add biflow flags
TEST_F(IEs, addReverse)
{
    // Create a copy of the manager and remove few reverse elements
    fds_iemgr_t *ie_copy = fds_iemgr_copy(ie_mgr);
    ASSERT_NE(ie_copy, nullptr);
    EXPECT_EQ(fds_iemgr_elem_remove(ie_copy, 29305, 1), FDS_OK);
    EXPECT_EQ(fds_iemgr_elem_remove(ie_copy, 29305, 2), FDS_OK);

    // Create a test template
    struct ie_template_params tmplt;
    tmplt.id = 11111;
    tmplt.type = FDS_TYPE_TEMPLATE_OPTS;
    tmplt.flags = 0;
    tmplt.scope_fields = 2;

    constexpr fds_template_flag_t flg_comm = FDS_TFIELD_LAST_IE;
    const std::vector<struct ie_field_params> fields = {
        // id - en - len - type - flags
        { 8, 0, 4, flg_comm | FDS_TFIELD_SCOPE, FDS_ET_IPV4_ADDRESS}, // sourceIPv4Address
        {12, 0, 4, flg_comm | FDS_TFIELD_SCOPE, FDS_ET_IPV4_ADDRESS}, // destinationIPv4Address
        { 2, 0, 4, flg_comm, FDS_ET_UNSIGNED_64},  // packetDeltaCount
        { 1, 0, 4, flg_comm, FDS_ET_UNSIGNED_64},  // octetDeltaCount
        { 2, 29305, 4, flg_comm, FDS_ET_UNASSIGNED}, // packetDeltaCount <-- unknown here
        { 1, 29305, 4, flg_comm, FDS_ET_UNASSIGNED}  // octetDeltaCount  <-- unknown here
    };

    // Create a template and run tests
    uniq_fds_tmplt aux_template(nullptr, &::fds_template_destroy);
    template_create(tmplt, fields, aux_template);
    fds_template_ies_define(aux_template.get(), ie_copy, false);
    {
        SCOPED_TRACE("Phase I. Without known reverse elements");
        template_tester(tmplt, fields, aux_template);
        EXPECT_EQ(aux_template.get()->rev_dir, nullptr); // Reverse template undefined
    }

    // Prepare new definitions of elements with known reverse elements
    constexpr fds_template_flag_t flg_bkey = flg_comm | FDS_TFIELD_BKEY;
    constexpr fds_template_flag_t flg_rev = flg_comm | FDS_TFIELD_REVERSE;
    const std::vector<struct ie_field_params> fields_new = {
        // id - en - len - type - flags
        { 8, 0, 4, flg_bkey | FDS_TFIELD_SCOPE, FDS_ET_IPV4_ADDRESS}, // sourceIPv4Address
        {12, 0, 4, flg_bkey | FDS_TFIELD_SCOPE, FDS_ET_IPV4_ADDRESS}, // destinationIPv4Address
        { 2, 0, 4, flg_comm,    FDS_ET_UNSIGNED_64}, // packetDeltaCount
        { 1, 0, 4, flg_comm,    FDS_ET_UNSIGNED_64}, // octetDeltaCount
        { 2, 29305, 4, flg_rev, FDS_ET_UNSIGNED_64}, // packetDeltaCount (reverse)
        { 1, 29305, 4, flg_rev, FDS_ET_UNSIGNED_64}  // octetDeltaCount (reverse)
    };
    const std::vector<struct ie_field_params> fields_new_rev = {
        // id - en - len - type - flags
        {12, 0, 4, flg_bkey | FDS_TFIELD_SCOPE, FDS_ET_IPV4_ADDRESS}, // destinationIPv4Address
        { 8, 0, 4, flg_bkey | FDS_TFIELD_SCOPE, FDS_ET_IPV4_ADDRESS}, // sourceIPv4Address
        { 2, 29305, 4, flg_rev, FDS_ET_UNSIGNED_64}, // packetDeltaCount (reverse)
        { 1, 29305, 4, flg_rev, FDS_ET_UNSIGNED_64}, // octetDeltaCount (reverse)
        { 2, 0, 4, flg_comm,    FDS_ET_UNSIGNED_64}, // packetDeltaCount
        { 1, 0, 4, flg_comm,    FDS_ET_UNSIGNED_64}  // octetDeltaCount
    };
    tmplt.flags |= FDS_TEMPLATE_BIFLOW;

    // Add new definitions with reverse elements (use original IE manager)
    fds_template_ies_define(aux_template.get(), ie_mgr, false);
    {
        SCOPED_TRACE("Phase II. Added definitions of reverse elements");
        template_tester(tmplt, fields_new, aux_template);
        EXPECT_NE(aux_template.get()->rev_dir, nullptr); // Reverse template defined
        template_tester(tmplt, fields_new_rev, aux_template, true);
    }

    // Now try to remove definition reverse Information Elements -> flags should be cleared
    fds_template_ies_define(aux_template.get(), ie_copy, false);
    tmplt.flags &= ~(fds_template_flag_t) FDS_TEMPLATE_BIFLOW;
    {
        SCOPED_TRACE("Phase III. Remove definitions of reverse elements");
        template_tester(tmplt, fields, aux_template);
        EXPECT_EQ(aux_template.get()->rev_dir, nullptr); // Reverse template should be removed
    }

    fds_iemgr_destroy(ie_copy);
}

// Add biflow fields as secondary source of fields -> reverse fields should be ignored
TEST_F(IEs, biflowSecondary)
{
    // Create a copy of the manager and remove few reverse elements
    fds_iemgr_t *ie_copy = fds_iemgr_copy(ie_mgr);
    ASSERT_NE(ie_copy, nullptr);
    EXPECT_EQ(fds_iemgr_elem_remove(ie_copy, 29305, 1), FDS_OK);
    EXPECT_EQ(fds_iemgr_elem_remove(ie_copy, 29305, 2), FDS_OK);

    // Create a test template
    struct ie_template_params tmplt;
    tmplt.id = 55000;
    tmplt.type = FDS_TYPE_TEMPLATE;
    tmplt.flags = 0;
    tmplt.scope_fields = 0;

    constexpr fds_template_flag_t flg_comm = FDS_TFIELD_LAST_IE;
    const std::vector<struct ie_field_params> fields = {
        // id - en - len - type - flags
        { 8, 0, 4, flg_comm, FDS_ET_IPV4_ADDRESS}, // sourceIPv4Address
        {12, 0, 4, flg_comm, FDS_ET_IPV4_ADDRESS}, // destinationIPv4Address
        { 2, 0, 4, flg_comm, FDS_ET_UNSIGNED_64},  // packetDeltaCount
        { 1, 0, 4, flg_comm, FDS_ET_UNSIGNED_64},  // octetDeltaCount
        { 2, 29305, 4, flg_comm, FDS_ET_UNASSIGNED}, // packetDeltaCount <-- unknown here
        { 1, 29305, 4, flg_comm, FDS_ET_UNASSIGNED}  // octetDeltaCount  <-- unknown here
    };

    // Create a template and run tests
    uniq_fds_tmplt aux_template(nullptr, &::fds_template_destroy);
    template_create(tmplt, fields, aux_template);
    fds_template_ies_define(aux_template.get(), ie_copy, false); // IEs without biflow
    {
        SCOPED_TRACE("Phase I. Without known reverse elements");
        template_tester(tmplt, fields, aux_template);
        EXPECT_EQ(aux_template.get()->rev_dir, nullptr); // Reverse template undefined
    }

    // Add new definitions with reverse elements (use original IE manager)
    fds_template_ies_define(aux_template.get(), ie_mgr, true); // <-- perform ONLY update of unknown
    {
        SCOPED_TRACE("Phase II. Added definitions of reverse elements (should be ignored)");
        template_tester(tmplt, fields, aux_template);
        EXPECT_EQ(aux_template.get()->rev_dir, nullptr); // Reverse template still undefined
    }

    fds_iemgr_destroy(ie_copy);
}
