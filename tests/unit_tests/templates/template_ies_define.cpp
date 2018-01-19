#include <gtest/gtest.h>
#include <libfds.h>

// Template generator
#include <TGenerator.h>
#include <common_tests.h>

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

struct exp_template_params {
    uint16_t id;
    enum fds_template_type type;
    fds_template_flag_t flags;
    uint16_t scope_fields;
};

struct exp_field_params {
    uint16_t id;
    uint32_t en;
    uint16_t len;
    fds_template_flag_t flags;
    fds_iemgr_element_type type;
};

void
template_tester(const fds_iemgr_t *ies, const exp_template_params &tmplt,
    const std::vector<struct exp_field_params> &fields)
{
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

    // Define IEs and check flags
    fds_template_ies_define(tmplt_rec, ies, false);
    for (uint16_t i = 0; i <tmplt_rec->fields_cnt_total; ++i) {
        const fds_tfield *field = &tmplt_rec->fields[i];
        if (fields[i].type == FDS_ET_UNASSIGNED) {
            continue;
        }

        ASSERT_NE(field->def, nullptr);
        EXPECT_EQ(field->def->data_type, fields[i].type);
        ct_tfield_flags(field, fields[i].flags);
    }

    // Check template flags
    ct_template_flags(tmplt_rec, tmplt.flags);

    // Cleanup
    fds_template_destroy(tmplt_rec);
}

TEST_F(IEs, StandardFlow)
{
    constexpr fds_template_flag_t flg_comm = FDS_TFIELD_LAST_IE;
    const std::vector<struct exp_field_params> fields = {
        // id - en - len - type - flags
        {  8, 0, 4, flg_comm, FDS_ET_IPV4_ADDRESS}, // sourceIPv4Address
        { 12, 0, 4, flg_comm, FDS_ET_IPV4_ADDRESS}, // destinationIPv4Address
        {  7, 0, 2, flg_comm, FDS_ET_UNSIGNED_16}, // sourceTransportPort
        { 11, 0, 2, flg_comm, FDS_ET_UNSIGNED_16}, // destinationTransportPort
        {  4, 0, 1, flg_comm, FDS_ET_UNSIGNED_8}, // protocolIdentifier
        {  6, 0, 1, flg_comm, FDS_ET_UNSIGNED_16}, // tcpControlBits
        {152, 0, 8, flg_comm, FDS_ET_DATE_TIME_MILLISECONDS}, // flowStartMilliseconds
        {153, 0, 8, flg_comm, FDS_ET_DATE_TIME_MILLISECONDS}, // flowEndMilliseconds
        {  2, 0, 4, flg_comm, FDS_ET_UNSIGNED_64}, // packetDeltaCount
        {  1, 0, 4, flg_comm, FDS_ET_UNSIGNED_64}  // octetDeltaCount
    };

    struct exp_template_params tmplt;
    tmplt.id = 256;
    tmplt.type = FDS_TYPE_TEMPLATE;
    tmplt.flags = 0;
    tmplt.scope_fields = 0;

    template_tester(ie_mgr, tmplt, fields);
}

TEST_F(IEs, Biflow)
{
    constexpr fds_template_flag_t flg_comm = FDS_TFIELD_LAST_IE;
    constexpr fds_template_flag_t flg_rev = flg_comm | FDS_TFIELD_REVERSE;

    const std::vector<struct exp_field_params> fields = {
        // id - en - len - type - flags
        {  8, 0, 4, flg_comm, FDS_ET_IPV4_ADDRESS}, // sourceIPv4Address
        { 12, 0, 4, flg_comm, FDS_ET_IPV4_ADDRESS}, // destinationIPv4Address
        {  7, 0, 2, flg_comm, FDS_ET_UNSIGNED_16}, // sourceTransportPort
        { 11, 0, 2, flg_comm, FDS_ET_UNSIGNED_16}, // destinationTransportPort
        {  4, 0, 1, flg_comm, FDS_ET_UNSIGNED_8}, // protocolIdentifier
        {  6, 0, 1, flg_comm, FDS_ET_UNSIGNED_16}, // tcpControlBits
        {152, 0, 8, flg_comm, FDS_ET_DATE_TIME_MILLISECONDS}, // flowStartMilliseconds
        {153, 0, 8, flg_comm, FDS_ET_DATE_TIME_MILLISECONDS}, // flowEndMilliseconds
        {  2, 0, 4, flg_comm, FDS_ET_UNSIGNED_64}, // packetDeltaCount
        {  1, 0, 4, flg_comm, FDS_ET_UNSIGNED_64}, // octetDeltaCount
        {  6, 29305, 1, flg_rev, FDS_ET_UNSIGNED_16}, // tcpControlBits (reverse)
        {152, 29305, 8, flg_rev, FDS_ET_DATE_TIME_MILLISECONDS}, // flowStartMilliseconds (reverse)
        {153, 29305, 8, flg_rev, FDS_ET_DATE_TIME_MILLISECONDS}, // flowEndMilliseconds (reverse)
        {  2, 29305, 4, flg_rev, FDS_ET_UNSIGNED_64}, // packetDeltaCount (reverse)
        {  1, 29305, 4, flg_rev, FDS_ET_UNSIGNED_64}  // octetDeltaCount (reverse)
    };

    struct exp_template_params tmplt;
    tmplt.id = 256;
    tmplt.type = FDS_TYPE_TEMPLATE;
    tmplt.flags = FDS_TEMPLATE_HAS_REVERSE;
    tmplt.scope_fields = 0;

    template_tester(ie_mgr, tmplt, fields);
}



