#include <gtest/gtest.h>
#include <cstdio>
#include <stdexcept>
#include <tuple>
#include <array>
#include <utility>

#include <MsgGen.h>

#include <libfds.h>
#include "../../../src/file/Block_data_reader.hpp"
#include "../../../src/file/Block_data_writer.hpp"
#include "../../../src/file/File_exception.hpp"

// Namespace of the components to test
using namespace fds_file;

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// Type of parameters passed to Test Cases
using product_type = std::tuple<Io_factory::Type, enum fds_file_alg>;

// Class of parametrized Test Cases
class DBlock : public ::testing::TestWithParam<product_type> {
public:
    // Before the whole Test Suite (i.e. resources common for all Test Cases initialized only once)
    static void SetUpTestCase() {
        // Initialize and load Information Elements
        static const char *ie_path = "data/iana.xml";
        m_iemgr = fds_iemgr_create();
        if (m_iemgr == nullptr || fds_iemgr_read_file(m_iemgr, ie_path, true) != FDS_OK) {
            throw std::runtime_error("Faield to initialize IE manager");
        }
    }

    // After the whole Test Suite
    static void TearDownTestCase() {
        // Destroy the common IE manager
        fds_iemgr_destroy(m_iemgr);
        m_iemgr = nullptr;
    }
protected:
    // Before each Test Case
    void SetUp() override  {
        // Extract Test Case specific parameters (I/O type, compression alg. type)
        std::tie(m_param_io, m_param_alg) = GetParam();

        // Create a temporary file
        m_tmpfile = tmpfile();
        if (m_tmpfile == nullptr) {
            throw std::runtime_error("Failed to create a temporary file");
        }
        m_fd = fileno(m_tmpfile);
    }

    // After each Test Case
    void TearDown() override {
        // Close the temporary file
        fclose(m_tmpfile);
    }

    // IE manager (shared resource for all Test Cases)
    static fds_iemgr_t *m_iemgr;

    // Parsed Test Case parameters
    Io_factory::Type m_param_io;
    enum fds_file_alg m_param_alg;

    // Temporary file and its file descriptor
    FILE *m_tmpfile;
    int m_fd;
};

// Initialize the static class member(s)
fds_iemgr_t * DBlock::m_iemgr = nullptr;

// -------------------------------------------------------------------------------------------------

// IPFIX (Options) Template data holder
using tmplt_ptr_t = std::unique_ptr<uint8_t, decltype(&free)>;
// Combination of Template data and Template size
using tmplt_tuple_t = std::tuple<tmplt_ptr_t, uint16_t>;

// IPFIX Data Record holder
using rec_ptr_t = std::unique_ptr<uint8_t, decltype(&free)>;
// Combination of Data Record holder and Data size
using rec_tuple_t = std::tuple<rec_ptr_t, uint16_t>;

// Generator of a simple IPFIX Template (pattern 1)
tmplt_tuple_t
gen_t1_tmplt(uint16_t tid)
{
    ipfix_trec trec(tid);
    trec.add_field(  7, 2); // sourceTransportPort
    trec.add_field(  8, 4); // sourceIPv4Address
    trec.add_field( 11, 2); // destinationTransportPort
    trec.add_field( 12, 4); // destinationIPv4Address
    trec.add_field(  4, 1); // protocolIdentifier
    trec.add_field(210, 3); // -- paddingOctets
    trec.add_field(152, 8); // flowStartMilliseconds
    trec.add_field(153, 8); // flowEndMilliseconds
    trec.add_field(  1, 8); // octetDeltaCount
    trec.add_field(  2, 8); // packetDeltaCount

    uint16_t size = trec.size();
    auto ptr = tmplt_ptr_t(trec.release(), &free);
    return std::make_tuple(std::move(ptr), size);
}

// Generator of a partly parametrizable Data Record based on the simple IPFIX Template (pattern 1)
rec_tuple_t
gen_t1_rec(uint16_t src_p = 80, uint16_t dst_p = 48714, uint8_t proto = 17, uint64_t bytes = 1223,
    uint64_t pkts = 2)
{
    // Constant parameters
    const std::string VALUE_SRC_IP4  = "127.0.0.1";
    std::string VALUE_DST_IP4  = "1.1.1.1";
    uint64_t    VALUE_TS_FST   = 1522670362000ULL;
    uint64_t    VALUE_TS_LST   = 1522670372999ULL;

    ipfix_drec drec {};
    drec.append_uint(src_p, 2);
    drec.append_ip(VALUE_SRC_IP4);
    drec.append_uint(dst_p, 2);
    drec.append_ip(VALUE_DST_IP4);
    drec.append_uint(proto, 1);
    drec.append_uint(0, 3); // Padding
    drec.append_datetime(VALUE_TS_FST, FDS_ET_DATE_TIME_MILLISECONDS);
    drec.append_datetime(VALUE_TS_LST, FDS_ET_DATE_TIME_MILLISECONDS);
    drec.append_uint(bytes, 8);
    drec.append_uint(pkts, 8);

    uint16_t size = drec.size();
    auto ptr = rec_ptr_t(drec.release(), &free);
    return std::make_tuple(std::move(ptr), size);
}

// Generator of a biflow IPFIX Template with variable-length fields (pattern 2)
tmplt_tuple_t
gen_t2_tmplt(uint16_t tid)
{
    ipfix_trec trec {tid};
    trec.add_field(  7, 2);                    // sourceTransportPort
    trec.add_field(  8, 4);                    // sourceIPv4Address
    trec.add_field( 11, 2);                    // destinationTransportPort
    trec.add_field( 12, 4);                    // destinationIPv4Address
    trec.add_field(  4, 1);                    // protocolIdentifier
    trec.add_field(210, 3);                    // -- paddingOctets
    trec.add_field(152, 8);                    // flowStartMilliseconds
    trec.add_field(153, 8);                    // flowEndMilliseconds
    trec.add_field(152, 8, 29305);             // flowStartMilliseconds (reverse)
    trec.add_field(153, 8, 29305);             // flowEndMilliseconds   (reverse)
    trec.add_field( 96, ipfix_trec::SIZE_VAR); // applicationName
    trec.add_field( 94, ipfix_trec::SIZE_VAR); // applicationDescription
    trec.add_field(210, 5);                    // -- paddingOctets
    trec.add_field(  1, 8);                    // octetDeltaCount
    trec.add_field(  2, 8);                    // packetDeltaCount
    trec.add_field(100, 4, 10000);             // -- field with unknown definition --
    trec.add_field(  1, 8, 29305);             // octetDeltaCount (reverse)
    trec.add_field(  2, 8, 29305);             // packetDeltaCount (reverse)
    trec.add_field( 82, ipfix_trec::SIZE_VAR); // interfaceName
    trec.add_field( 82, ipfix_trec::SIZE_VAR); // interfaceName (second occurrence)

    uint16_t size = trec.size();
    auto ptr = tmplt_ptr_t(trec.release(), &free);
    return std::make_tuple(std::move(ptr), size);
}

// Generator of a partly parametrizable Data Record based on the biflow IPFIX Template (pattern 2)
rec_tuple_t
gen_t2_rec(std::string app_name = "ipfixcol2", std::string ifc_name = "eth0", uint16_t sp = 65145,
    uint16_t dp = 53, uint8_t proto = 6, uint64_t bts = 87984121, uint64_t pkts = 251)
{
    // Constant parameters
    std::string VALUE_SRC_IP4  = "127.0.0.1";
    std::string VALUE_DST_IP4  = "8.8.8.8";
    uint64_t    VALUE_TS_FST   = 226710362000ULL;
    uint64_t    VALUE_TS_LST   = 226710372999ULL;
    uint64_t    VALUE_TS_FST_R = 226710363123ULL;
    uint64_t    VALUE_TS_LST_R = 226710369000ULL;
    double      VALUE_UNKNOWN  = 3.1416f;
    std::string rev_app_name(app_name.rbegin(), app_name.rend());

    ipfix_drec drec {};
    drec.append_uint(sp, 2);
    drec.append_ip(VALUE_SRC_IP4);
    drec.append_uint(dp, 2);
    drec.append_ip(VALUE_DST_IP4);
    drec.append_uint(proto, 1);
    drec.append_uint(0, 3); // Padding
    drec.append_datetime(VALUE_TS_FST, FDS_ET_DATE_TIME_MILLISECONDS);
    drec.append_datetime(VALUE_TS_LST, FDS_ET_DATE_TIME_MILLISECONDS);
    drec.append_datetime(VALUE_TS_FST_R, FDS_ET_DATE_TIME_MILLISECONDS);
    drec.append_datetime(VALUE_TS_LST_R, FDS_ET_DATE_TIME_MILLISECONDS);
    drec.append_string(app_name);       // Adds variable head automatically (short version)
    drec.var_header(rev_app_name.length(), true); // Adds variable head manually (long version)
    drec.append_string(rev_app_name, rev_app_name.length());
    drec.append_uint(0, 5); // Padding
    drec.append_uint(bts, 8);
    drec.append_uint(pkts, 8);
    drec.append_float(VALUE_UNKNOWN, 4);
    drec.append_uint(UINT64_MAX - bts, 8);
    drec.append_uint(UINT64_MAX - pkts, 8);
    drec.var_header(0, false); // empty string (only header)
    drec.append_string(ifc_name);

    uint16_t size = drec.size();
    auto ptr = rec_ptr_t(drec.release(), &free);
    return std::make_tuple(std::move(ptr), size);
}

// Generator of an Options IPFIX Template(pattern 3)
tmplt_tuple_t
gen_t3_tmplt(uint16_t tid)
{
    ipfix_trec trec {tid, 2}; // 2 scope fields
    trec.add_field(149, 4); // observationDomainID
    trec.add_field(143, 4); // meteringProcessId
    trec.add_field(41, 8);  // exportedMessageTotalCount
    trec.add_field(42, 8);  // exportedFlowRecordTotalCount
    trec.add_field(40, 8);  // exportedOctetTotalCount

    uint16_t size = trec.size();
    auto ptr = tmplt_ptr_t(trec.release(), &free);
    return std::make_tuple(std::move(ptr), size);
}

// Generator of a parametrizable Data Record based on the Options IPFIX Template (pattern 3)
rec_tuple_t
gen_t3_rec(uint32_t odid = 4, uint32_t mpid = 1554, uint64_t msg_cnt = 171141,
    uint64_t flow_cnt = 212457447U, uint64_t octet_cnt = 2245744700U)
{
    ipfix_drec drec {};
    drec.append_uint(odid, 4);
    drec.append_uint(mpid, 4);
    drec.append_uint(msg_cnt, 8);
    drec.append_uint(flow_cnt, 8);
    drec.append_uint(octet_cnt, 8);

    uint16_t size = drec.size();
    auto ptr = rec_ptr_t(drec.release(), &free);
    return std::make_tuple(std::move(ptr), size);
}

// Compare records
static void
rec_cmp(const rec_tuple_t &orig, const uint8_t *data, uint16_t size)
{
    const rec_ptr_t &orig_ptr = std::get<0>(orig);
    uint16_t orig_size = std::get<1>(orig);

    EXPECT_EQ(orig_size, size);
    EXPECT_EQ(memcmp(orig_ptr.get(), data, size), 0);
}

// -------------------------------------------------------------------------------------------------

// Auxiliary function that generates human readable string of test case parameters
std::string
product_name(::testing::TestParamInfo<product_type> product)
{
    Io_factory::Type param_io_type;
    enum fds_file_alg param_calg;
    std::tie(param_io_type, param_calg)= product.param;

    // Warning: the returned string can contain only alphanumeric characters (i.e. no whitespaces)!
    std::string str;
    switch (param_io_type) {
    case Io_factory::Type::IO_ASYNC:
        str += "AsyncIO";
        break;
    case Io_factory::Type::IO_SYNC:
        str += "SyncIO";
        break;
    case Io_factory::Type::IO_DEFAULT:
        str += "DefaultIO";
        break;
    default:
        throw std::runtime_error("Undefined type");
    }

    str += "And";

    switch (param_calg) {
    case FDS_FILE_CALG_NONE:
        str += "NoCompression";
        break;
    case FDS_FILE_CALG_LZ4:
        str += "LZ4";
        break;
    case FDS_FILE_CALG_ZSTD:
        str += "ZSTD";
        break;
    default:
        throw std::runtime_error("Undefined type");
    }

    return str;
}

// Run all tests independently for all following combinations of compression algorithms and I/Os
Io_factory::Type io_list[] = {Io_factory::Type::IO_SYNC, Io_factory::Type::IO_ASYNC};
enum fds_file_alg calg_list[] = {FDS_FILE_CALG_NONE, FDS_FILE_CALG_LZ4};
auto product = ::testing::Combine(::testing::ValuesIn(io_list), ::testing::ValuesIn(calg_list));
INSTANTIATE_TEST_CASE_P(MainInstance, DBlock, product, &product_name);

// Create and destroy Data Block reader and writer
TEST_P(DBlock, createAndDestroy)
{
    // Create the writer
    uint32_t odid = 123;
    Block_data_writer writer(odid, m_param_alg);
    EXPECT_EQ(writer.count(), 0U);

    // Create the reader
    Block_data_reader reader(m_param_alg);
}

// Try to write an empty Data Block (nothing should be written)
TEST_P(DBlock, writeEmptyBlock)
{
    // Some parameters
    uint32_t odid = 10;
    uint16_t sid = 5;
    off_t offset = 0;
    uint32_t exp_time = 15654587U;
    uint64_t tmplt_offset = 0;

    // Write an empty Data Block
    Block_data_writer writer(odid, m_param_alg);
    EXPECT_EQ(writer.write_to_file(m_fd, offset, sid, tmplt_offset, m_param_io), 0U);
    EXPECT_EQ(writer.count(), 0U);
    writer.set_etime(exp_time);
    EXPECT_EQ(writer.write_to_file(m_fd, offset, sid, tmplt_offset, m_param_io), 0U);
    EXPECT_EQ(writer.count(), 0U);

    // Check that the Temporary file is empty
    EXPECT_EQ(lseek(m_fd, 0, SEEK_END), 0);
}

// Write a Data Block with a single Data Record and try to read it
TEST_P(DBlock, writeSingleRecord)
{
    // Some random parameters of the Data Block
    uint32_t odid = 213244;
    uint16_t sid = 1;
    off_t offset = 0;
    uint32_t exp_time = 45789114U;
    uint64_t tmplt_offset = 589715;

    Block_data_writer writer(odid, m_param_alg);
    Block_data_reader reader(m_param_alg);

    // Create a Template manager and add a Template
    uint16_t t1_tid = 256;
    tmplt_tuple_t t1_tuple = gen_t1_tmplt(t1_tid);
    tmplt_ptr_t &t1_ptr = std::get<0>(t1_tuple);
    uint16_t t1_size = std::get<1>(t1_tuple);

    Block_templates tmgr;
    tmgr.ie_source(m_iemgr);
    tmgr.add(FDS_TYPE_TEMPLATE, t1_ptr.get(), t1_size);
    const struct fds_template *tmplt = tmgr.get(t1_tid);
    ASSERT_NE(tmplt, nullptr);

    // Generate a Data Record based on the Template, add it to the Data Block and write it
    rec_tuple_t r1_tuple = gen_t1_rec();
    rec_ptr_t &r1_ptr = std::get<0>(r1_tuple);
    uint16_t r1_size = std::get<1>(r1_tuple);

    writer.set_etime(exp_time);
    EXPECT_GT(writer.remains(), r1_size);
    writer.add(r1_ptr.get(), r1_size, tmplt);
    EXPECT_EQ(writer.count(), 1U);
    uint64_t wsize = writer.write_to_file(m_fd, offset, sid, tmplt_offset, m_param_io);
    EXPECT_GT(wsize, 0U);
    EXPECT_EQ(writer.count(), 0U);

    // Wait for the writer to finish current I/O operation (required for async. I/O)
    writer.write_wait();

    // Try to read the Data Block
    reader.set_templates(tmgr.snapshot());
    reader.load_from_file(m_fd, offset, 0, m_param_io);
    const struct fds_file_bdata *block_hdr = reader.get_block_header();
    ASSERT_NE(block_hdr, nullptr);
    EXPECT_EQ(le64toh(block_hdr->hdr.length), wsize);
    EXPECT_EQ(le16toh(block_hdr->hdr.type), FDS_FILE_BTYPE_DATA);
    EXPECT_EQ(le16toh(block_hdr->session_id), sid);
    EXPECT_EQ(le32toh(block_hdr->odid), odid);
    EXPECT_EQ(le64toh(block_hdr->offset_tmptls), tmplt_offset);

    EXPECT_EQ(reader.next_block_hdr(), nullptr);

    // Check the Data Record
    struct fds_drec drec;
    struct fds_file_read_ctx ctx;
    ASSERT_EQ(reader.next_rec(&drec, &ctx), FDS_OK);
    EXPECT_EQ(ctx.odid, odid);
    EXPECT_EQ(ctx.exp_time, exp_time);
    EXPECT_EQ(ctx.sid, sid);
    ASSERT_NE(drec.data, nullptr);
    ASSERT_NE(drec.snap, nullptr);
    ASSERT_NE(drec.tmplt, nullptr);
    ASSERT_GT(drec.size, 0);
    rec_cmp(r1_tuple, drec.data, drec.size);
    // Check also referenced IPFIX Template
    EXPECT_EQ(fds_template_cmp(tmplt, drec.tmplt), 0);
    // Reference to IE should be also available
    struct fds_drec_field dfield;
    ASSERT_GE(fds_drec_find(&drec, 0, 1, &dfield), 0);
    ASSERT_NE(dfield.info, nullptr);
    ASSERT_NE(dfield.info->def, nullptr);
    EXPECT_EQ(dfield.info->def->id, 1);

    // The next record should not be available
    EXPECT_EQ(reader.next_rec(&drec, &ctx), FDS_EOC);
    // But after rewind...
    reader.rewind();
    EXPECT_EQ(reader.next_rec(&drec, &ctx), FDS_OK);
    rec_cmp(r1_tuple, drec.data, drec.size);
}

// Write multiple different Data Records with different Export Times
TEST_P(DBlock, writeDifferentRecords)
{
    // Some random parameters of the Data Block
    uint32_t odid = 125;
    uint16_t sid = 1;
    off_t offset = 160;
    uint64_t tmplt_offset = 0U;

    Block_data_writer writer(odid, m_param_alg);
    Block_data_reader reader(m_param_alg);

    // Create a Template manager and add a Template
    uint16_t t1_tid = 256, t2_tid = 300, t3_tid =65535;
    tmplt_tuple_t t1_tuple = gen_t1_tmplt(t1_tid);
    tmplt_tuple_t t2_tuple = gen_t2_tmplt(t2_tid);
    tmplt_tuple_t t3_tuple = gen_t3_tmplt(t3_tid);

    Block_templates tmgr; // With undefined IE source
    tmgr.add(FDS_TYPE_TEMPLATE, std::get<0>(t1_tuple).get(), std::get<1>(t1_tuple));
    tmgr.add(FDS_TYPE_TEMPLATE, std::get<0>(t2_tuple).get(), std::get<1>(t2_tuple));
    tmgr.add(FDS_TYPE_TEMPLATE_OPTS, std::get<0>(t3_tuple).get(), std::get<1>(t3_tuple));
    const struct fds_template *t1_parsed = tmgr.get(t1_tid);
    const struct fds_template *t2_parsed = tmgr.get(t2_tid);
    const struct fds_template *t3_parsed = tmgr.get(t3_tid);

    // Generate few Data Records and them to the Data Block
    rec_tuple_t r1_tuple = gen_t1_rec();
    rec_tuple_t r2_tuple = gen_t2_rec();
    rec_tuple_t r3_tuple = gen_t3_rec();
    uint32_t remains_start = writer.remains();
    // By default, Export Time should be 0
    writer.add(std::get<0>(r1_tuple).get(), std::get<1>(r1_tuple), t1_parsed);
    writer.add(std::get<0>(r2_tuple).get(), std::get<1>(r2_tuple), t2_parsed);
    writer.set_etime(10);
    writer.add(std::get<0>(r3_tuple).get(), std::get<1>(r3_tuple), t3_parsed);
    writer.add(std::get<0>(r3_tuple).get(), std::get<1>(r3_tuple), t3_parsed);
    writer.add(std::get<0>(r2_tuple).get(), std::get<1>(r2_tuple), t2_parsed);
    writer.set_etime(50);
    EXPECT_EQ(writer.count(), 5U);
    uint32_t remains_end = writer.remains();
    EXPECT_GT(remains_start, remains_end);

    uint64_t wsize = writer.write_to_file(m_fd, offset, sid, tmplt_offset, m_param_io);
    EXPECT_GT(wsize, 0U);
    EXPECT_EQ(writer.count(), 0U);

    // Wait for the writer to finish current I/O operation (required for async. I/O)
    writer.write_wait();

    // Try to read the Data Block
    reader.set_templates(tmgr.snapshot());
    reader.load_from_file(m_fd, offset, 0, m_param_io);
    EXPECT_EQ(reader.next_block_hdr(), nullptr);
    const struct fds_file_bdata *block_hdr = reader.get_block_header();
    ASSERT_NE(block_hdr, nullptr);
    EXPECT_EQ(le64toh(block_hdr->hdr.length), wsize);
    EXPECT_EQ(le16toh(block_hdr->hdr.type), FDS_FILE_BTYPE_DATA);
    EXPECT_EQ(le16toh(block_hdr->session_id), sid);
    EXPECT_EQ(le32toh(block_hdr->odid), odid);
    EXPECT_EQ(le64toh(block_hdr->offset_tmptls), tmplt_offset);

    // Check records
    struct fds_drec drec;
    struct fds_file_read_ctx ctx;

    ASSERT_EQ(reader.next_rec(&drec, &ctx), FDS_OK);
    EXPECT_EQ(ctx.odid, odid);
    EXPECT_EQ(ctx.exp_time, 0U);
    EXPECT_EQ(ctx.sid, sid);
    EXPECT_EQ(fds_template_cmp(drec.tmplt, t1_parsed), 0);
    rec_cmp(r1_tuple, drec.data, drec.size);

    ASSERT_EQ(reader.next_rec(&drec, &ctx), FDS_OK);
    EXPECT_EQ(ctx.odid, odid);
    EXPECT_EQ(ctx.exp_time, 0U);
    EXPECT_EQ(ctx.sid, sid);
    EXPECT_EQ(fds_template_cmp(drec.tmplt, t2_parsed), 0);
    rec_cmp(r2_tuple, drec.data, drec.size);

    for (int i : {1, 2}) {
        SCOPED_TRACE("i = " + std::to_string(i));
        ASSERT_EQ(reader.next_rec(&drec, &ctx), FDS_OK);
        EXPECT_EQ(ctx.odid, odid);
        EXPECT_EQ(ctx.exp_time, 10U);
        EXPECT_EQ(ctx.sid, sid);
        EXPECT_EQ(fds_template_cmp(drec.tmplt, t3_parsed), 0);
        rec_cmp(r3_tuple, drec.data, drec.size);
    }

    ASSERT_EQ(reader.next_rec(&drec, &ctx), FDS_OK);
    EXPECT_EQ(ctx.odid, odid);
    EXPECT_EQ(ctx.exp_time, 10U);
    EXPECT_EQ(ctx.sid, sid);
    EXPECT_EQ(fds_template_cmp(drec.tmplt, t2_parsed), 0);
    rec_cmp(r2_tuple, drec.data, drec.size);

    EXPECT_EQ(reader.next_rec(&drec, &ctx), FDS_EOC);
}

// Try to write maximum possible number of Data Records until the buffer is completely full
// + try to load Data Block with invalid hint size
// + try to decode Data Block without definition of a Template Manager
TEST_P(DBlock, fullBuffer)
{
    // Some pseudo random parameters
    uint32_t odid = 0;
    uint16_t sid = 0;
    off_t offset = 0;
    uint64_t tmplt_offset = 12U;

    // Create a Template manager and add few Templates
    uint16_t t2_tid = 546, t3_tid = 25112;
    tmplt_tuple_t t2_tuple = gen_t2_tmplt(t2_tid);
    tmplt_tuple_t t3_tuple = gen_t3_tmplt(t3_tid);

    Block_templates tmgr_writer;
    tmgr_writer.add(FDS_TYPE_TEMPLATE, std::get<0>(t2_tuple).get(), std::get<1>(t2_tuple));
    tmgr_writer.add(FDS_TYPE_TEMPLATE_OPTS, std::get<0>(t3_tuple).get(), std::get<1>(t3_tuple));
    const struct fds_template *t2_parsed = tmgr_writer.get(t2_tid);
    const struct fds_template *t3_parsed = tmgr_writer.get(t3_tid);

    // Add multiple Data Records until the buffer is full
    const char *str1 = "someRandomString", *str2 = "another string that can be slightly longer...";
    rec_tuple_t r2_tuple_v1 = gen_t2_rec(str1);
    rec_tuple_t r2_tuple_v2 = gen_t2_rec(str2);
    rec_tuple_t r3_tuple = gen_t3_rec(odid);

    std::array<const rec_tuple_t *, 3> rec_arr = {&r2_tuple_v1, &r2_tuple_v2, &r3_tuple};
    std::array<const struct fds_template *, 3> tmplt_arr = {t2_parsed, t2_parsed, t3_parsed};

    // For different size of IPFIX Messages generated by the writer
    for (uint16_t max_msg_size : {64, 512, 1400, 3000, 6500, 65535}) {
        SCOPED_TRACE("MAX MSG SIZE: " + std::to_string(max_msg_size));

        Block_data_writer writer(odid, m_param_alg, max_msg_size);
        Block_data_reader reader(m_param_alg);

        unsigned int idx = 0;
        const rec_tuple_t *rec2add = nullptr;
        const struct fds_template *tmpl2add = nullptr;
        uint64_t recs_added = 0;

        while (true) {
            SCOPED_TRACE("recs_added: " + std::to_string(recs_added));
            if (idx >= rec_arr.size()) {
                idx = 0;
            }

            if (recs_added % 100U == 0) {
                // Increment Export Time
                writer.set_etime(recs_added / 100U);
            }

            rec2add = rec_arr[idx];
            tmpl2add = tmplt_arr[idx];
            if (writer.remains() < std::get<1>(*rec2add)) {
                break;
            }

            // Add the record
            writer.add(std::get<0>(*rec2add).get(), std::get<1>(*rec2add), tmpl2add);
            ++recs_added;
            ++idx;
        }

        EXPECT_EQ(writer.count(), recs_added);

        // The next record should not fit
        EXPECT_THROW(writer.add(std::get<0>(*rec2add).get(), std::get<1>(*rec2add), tmpl2add), File_exception);
        // Write the Data Block to the file
        uint64_t wsize = writer.write_to_file(m_fd, offset, sid, tmplt_offset, m_param_io);
        EXPECT_GT(wsize, 0U);
        EXPECT_EQ(writer.count(), 0U);

        writer.write_wait();

        // Create different Template manager for reader with references to IE definitions
        Block_templates tmgr_reader;
        tmgr_reader.ie_source(m_iemgr);
        tmgr_reader.add(FDS_TYPE_TEMPLATE, std::get<0>(t2_tuple).get(), std::get<1>(t2_tuple));
        tmgr_reader.add(FDS_TYPE_TEMPLATE_OPTS, std::get<0>(t3_tuple).get(), std::get<1>(t3_tuple));
        t2_parsed = tmgr_reader.get(t2_tid);
        t3_parsed = tmgr_reader.get(t3_tid);

        // Try to read all records (use load_from_file with hint, ignore the Data Block header check)
        reader.set_templates(tmgr_reader.snapshot());
        reader.load_from_file(m_fd, offset, wsize, m_param_io);

        idx = 0;
        uint64_t recs_read = 0;

        while (recs_read < recs_added) {
            SCOPED_TRACE("recs_read: " + std::to_string(recs_read));
            if (idx >= rec_arr.size()) {
                idx = 0;
            }

            rec2add = rec_arr[idx];
            tmpl2add = tmplt_arr[idx];

            // Check the record
            struct fds_drec drec;
            struct fds_file_read_ctx ctx;
            ASSERT_EQ(reader.next_rec(&drec, &ctx), FDS_OK);

            // Check the context
            ASSERT_EQ(ctx.odid, odid);
            ASSERT_EQ(ctx.exp_time, recs_read / 100U);
            ASSERT_EQ(ctx.sid, sid);
            // Check the content
            EXPECT_EQ(fds_template_cmp(drec.tmplt, tmpl2add), 0);
            rec_cmp(*rec2add, drec.data, drec.size);
            ++recs_read;
            ++idx;
        }

        // No more Data Records
        struct fds_drec drec;
        struct fds_file_read_ctx ctx;
        EXPECT_EQ(reader.next_rec(&drec, &ctx), FDS_EOC);

        // --- FAIL TESTS ----

        // Try to read the Data Block with invalid hint (i.e. Data Block size doesn't match)
        Block_data_reader reader_invalid1(m_param_alg);
        reader_invalid1.load_from_file(m_fd, offset, wsize - 1, m_param_io);
        EXPECT_THROW(reader_invalid1.get_block_header(), File_exception);
        Block_data_reader reader_invalid2(m_param_alg);
        reader_invalid2.load_from_file(m_fd, offset, wsize + 1, m_param_io);
        EXPECT_THROW(reader_invalid1.get_block_header(), File_exception);

        // Try to load and parse the Data Block without definition of a Template Manager
        Block_data_reader reader_no_tmgr(m_param_alg);
        reader_no_tmgr.load_from_file(m_fd, offset, wsize, m_param_io);
        ASSERT_NE(reader_no_tmgr.get_block_header(), nullptr);
        EXPECT_THROW(reader_no_tmgr.next_rec(&drec, &ctx), File_exception);
    }
}

// Writer multiple Data Block in file and try to read them
TEST_P(DBlock, multipleBlocks)
{
    // Some pseudo random parameters
    uint32_t odid1 = 0, odid2 = 1;
    uint16_t sid = 0;
    off_t offset = 0;
    uint64_t tmplt_offset = 10020U;

    Block_data_writer writer1(odid1, m_param_alg);
    Block_data_writer writer2(odid2, m_param_alg, 3000);
    Block_data_reader reader1(m_param_alg);
    Block_data_reader reader2(m_param_alg);

    // Create a Template manager and add few Templates
    uint16_t t1_tid = 65530, t2_tid = 48000;
    tmplt_tuple_t t1_tuple = gen_t1_tmplt(t1_tid);
    tmplt_tuple_t t2_tuple = gen_t2_tmplt(t2_tid);

    Block_templates tmgr;
    tmgr.add(FDS_TYPE_TEMPLATE, std::get<0>(t1_tuple).get(), std::get<1>(t1_tuple));
    tmgr.add(FDS_TYPE_TEMPLATE, std::get<0>(t2_tuple).get(), std::get<1>(t2_tuple));
    const struct fds_template *t1_parsed = tmgr.get(t1_tid);
    const struct fds_template *t2_parsed = tmgr.get(t2_tid);

    // Generate few Data Records
    rec_tuple_t r1_tuple = gen_t1_rec();
    rec_tuple_t r2_tuple_v1 = gen_t2_rec("randomString");
    rec_tuple_t r2_tuple_v2 = gen_t2_rec();

    // Create 10 Data Blocks using different writers in different order
    std::array<Block_data_writer *, 10> arr_writer = {
        &writer1, &writer2, &writer1, &writer1, &writer1,
        &writer2, &writer2, &writer1, &writer2, &writer1
    };

    unsigned int idx = 0;
    for (Block_data_writer *writer_ptr : arr_writer) {
        SCOPED_TRACE("idx: " + std::to_string(idx));

        // Fill the Data Block and write the content to the file
        writer_ptr->set_etime(idx * 10);
        writer_ptr->add(std::get<0>(r2_tuple_v1).get(), std::get<1>(r2_tuple_v1), t2_parsed);
        writer_ptr->add(std::get<0>(r1_tuple).get(), std::get<1>(r1_tuple), t1_parsed);
        writer_ptr->add(std::get<0>(r2_tuple_v2).get(), std::get<1>(r2_tuple_v2), t2_parsed);
        uint64_t wsize = writer_ptr->write_to_file(m_fd, offset, sid, tmplt_offset, m_param_io);
        ASSERT_GT(wsize, 0U);

        offset += wsize;
        ++idx;
        // Don't wait here until write I/0 is complete... we want to test internal blocking
    }

    // Wait for writers to complete I/O (we don't want to read unwritten Data Blocks)
    writer1.write_wait();
    writer2.write_wait();

    reader1.set_templates(tmgr.snapshot());
    reader2.set_templates(tmgr.snapshot());

    // Prepare readers (one is used for getting records, the second one will start loading)
    Block_data_reader *reader_main = &reader1;
    Block_data_reader *reader_second = &reader2;
    // Initialize load operation on the main reader
    reader_main->load_from_file(m_fd, 0, 0, m_param_io);

    idx = 0;
    offset = 0;

    // Try to read all Data Blocks
    while (idx < arr_writer.size()) {
        SCOPED_TRACE("idx: " + std::to_string(idx));

        if (idx < 10 - 1) { // The last block doesn't have successor
            // Determine position of the next block
            const fds_file_bdata *my_hdr = reader_main->get_block_header();
            ASSERT_NE(my_hdr, nullptr);
            uint64_t my_size = le64toh(my_hdr->hdr.length);
            offset += my_size;

            // Start (a)synchronous read of the next block using secondary reader
            const fds_file_bhdr *next_hdr = reader_main->next_block_hdr();
            ASSERT_NE(next_hdr, nullptr);
            ASSERT_EQ(le16toh(next_hdr->type), FDS_FILE_BTYPE_DATA);
            uint64_t next_size = le64toh(next_hdr->length);
            ASSERT_NE(next_size, 0U);

            reader_second->load_from_file(m_fd, offset, next_size, m_param_io);
        } else {
            // The last block
            EXPECT_EQ(reader_main->next_block_hdr(), nullptr);
        }

        // Check Data Block header
        const struct fds_file_bdata *block_hdr = reader_main->get_block_header();
        ASSERT_NE(block_hdr, nullptr);
        EXPECT_EQ(le16toh(block_hdr->session_id), sid);
        EXPECT_EQ(le64toh(block_hdr->offset_tmptls), tmplt_offset);
        uint32_t exp_odid = (arr_writer[idx] == &writer1) ? odid1 : odid2;
        EXPECT_EQ(block_hdr->odid, exp_odid);

        // Check Data Records in the Data Block
        struct fds_drec drec;
        struct fds_file_read_ctx ctx;

        ASSERT_EQ(reader_main->next_rec(&drec, &ctx), FDS_OK);
        ASSERT_EQ(ctx.odid, exp_odid);
        ASSERT_EQ(ctx.exp_time, idx * 10);
        ASSERT_EQ(ctx.sid, sid);
        ASSERT_EQ(fds_template_cmp(drec.tmplt, t2_parsed), 0);
        rec_cmp(r2_tuple_v1, drec.data, drec.size);

        ASSERT_EQ(reader_main->next_rec(&drec, &ctx), FDS_OK);
        ASSERT_EQ(ctx.odid, exp_odid);
        ASSERT_EQ(ctx.exp_time, idx * 10);
        ASSERT_EQ(ctx.sid, sid);
        ASSERT_EQ(fds_template_cmp(drec.tmplt, t1_parsed), 0);
        rec_cmp(r1_tuple, drec.data, drec.size);

        ASSERT_EQ(reader_main->next_rec(&drec, &ctx), FDS_OK);
        ASSERT_EQ(ctx.odid, exp_odid);
        ASSERT_EQ(ctx.exp_time, idx * 10);
        ASSERT_EQ(ctx.sid, sid);
        ASSERT_EQ(fds_template_cmp(drec.tmplt, t2_parsed), 0);
        rec_cmp(r2_tuple_v2, drec.data, drec.size);

        EXPECT_EQ(reader_main->next_rec(&drec, &ctx), FDS_EOC);

        // Swap readers every iteration
        std::swap(reader_main, reader_second);
        ++idx;
    }
}

// Try to load Data Block from an empty file
TEST_P(DBlock, readEmptyFile)
{
    // Without hint -> load should fail immediately
    Block_data_reader reader_no_hint(m_param_alg);
    EXPECT_THROW(reader_no_hint.load_from_file(m_fd, 0, 0, m_param_io), File_exception);

    // With hint -> load should fail during the first access
    uint64_t hint = 1024U;
    Block_data_reader reader_with_hint(m_param_alg);
    reader_with_hint.load_from_file(m_fd, 0, hint, m_param_io);
    EXPECT_THROW(reader_with_hint.get_block_header(), File_exception);
}

// Try to read a Template Block as Data Block (must fail)
TEST_P(DBlock, readTemplateBlockAsDataBlock)
{
    // Create a Template Manager, fill it, and write it to a file
    uint16_t t1_tid = 256, t2_tid = 257, t3_tid = 258;
    tmplt_tuple_t t1_tuple = gen_t1_tmplt(t1_tid);
    tmplt_tuple_t t2_tuple = gen_t2_tmplt(t2_tid);
    tmplt_tuple_t t3_tuple = gen_t3_tmplt(t3_tid);

    Block_templates tmgr;
    tmgr.add(FDS_TYPE_TEMPLATE, std::get<0>(t1_tuple).get(), std::get<1>(t1_tuple));
    tmgr.add(FDS_TYPE_TEMPLATE, std::get<0>(t2_tuple).get(), std::get<1>(t2_tuple));
    tmgr.add(FDS_TYPE_TEMPLATE_OPTS, std::get<0>(t3_tuple).get(), std::get<1>(t3_tuple));
    uint64_t wsize = tmgr.write_to_file(m_fd, 0, 0, 0);
    ASSERT_GT(wsize, 0U);

    // Try to read it as Data Block (with and without hint)
    Block_data_reader reader_no_hint(m_param_alg);
    EXPECT_THROW(reader_no_hint.load_from_file(m_fd, 0, 0, m_param_io), File_exception);

    Block_data_reader reader_with_hint(m_param_alg);
    reader_with_hint.load_from_file(m_fd, 0, wsize, m_param_io);
    reader_with_hint.set_templates(tmgr.snapshot());

    struct fds_drec drec;
    struct fds_file_read_ctx ctx;
    EXPECT_THROW(reader_with_hint.next_rec(&drec, &ctx), File_exception);
}

// Try to use various functions that depend on loaded data without loading any Data Block
TEST_P(DBlock, parseWithoutLoad)
{
    Block_data_reader reader1(m_param_alg);
    EXPECT_THROW(reader1.get_block_header(), File_exception);

    Block_data_reader reader2(m_param_alg);
    EXPECT_THROW(reader2.next_block_hdr(), File_exception);

    struct fds_drec drec;
    struct fds_file_read_ctx ctx;
    Block_data_reader reader3(m_param_alg);
    EXPECT_THROW(reader3.next_rec(&drec, &ctx), File_exception);
}

// Try to decompress Data Block using incorrect decompression algorithm
TEST_P(DBlock, incorrectDecompressionAlg)
{
    // Prepare a Template manager
    uint16_t tid1 = 10000, tid2 = 12345;
    tmplt_tuple_t t1_tuple = gen_t1_tmplt(tid1);
    tmplt_tuple_t t2_tuple = gen_t2_tmplt(tid2);

    Block_templates tmgr;
    tmgr.add(FDS_TYPE_TEMPLATE, std::get<0>(t1_tuple).get(), std::get<1>(t1_tuple));
    tmgr.add(FDS_TYPE_TEMPLATE, std::get<0>(t2_tuple).get(), std::get<1>(t2_tuple));
    const struct fds_template *t1_parsed = tmgr.get(tid1);
    const struct fds_template *t2_parsed = tmgr.get(tid2);

    // Prepare Data Block compressed using current TestCase parameter
    rec_tuple_t r1_tuple = gen_t1_rec();
    rec_tuple_t r2_tuple = gen_t2_rec();

    Block_data_writer writer(0, m_param_alg);
    writer.set_etime(10);
    writer.add(std::get<0>(r2_tuple).get(), std::get<1>(r2_tuple), t2_parsed);
    writer.add(std::get<0>(r1_tuple).get(), std::get<1>(r1_tuple), t1_parsed);
    writer.add(std::get<0>(r2_tuple).get(), std::get<1>(r2_tuple), t2_parsed);
    uint64_t wsize = writer.write_to_file(m_fd, 0, 0, 0, m_param_io);
    ASSERT_GT(wsize, 0U);

    // Wait for the Data Block to be written
    writer.write_wait();

    for (enum fds_file_alg calg : calg_list) {
        SCOPED_TRACE("Decompression algorithm: " + std::to_string(calg));

        if (calg == m_param_alg) {
            // Skip situation when compression and decompression uses the same algorithm
            continue;
        }

        struct fds_drec drec;
        struct fds_file_read_ctx ctx;

        for (uint64_t hint : {static_cast<uint64_t>(0), wsize}) { // Without and with size hint
            SCOPED_TRACE("Hint: " + std::to_string(hint));
            Block_data_reader reader(calg);
            reader.set_templates(tmgr.snapshot());

            if (m_param_alg == FDS_FILE_CALG_NONE) {
                /* Compression was NOT used during Data Block writing (i.e. the compression flag of
                 * the block is not set -> the configured decompression algorithm should be ignored)
                 * Therefore, the Data Block can be processed!
                 */
                reader.load_from_file(m_fd, 0, hint, m_param_io);
                EXPECT_EQ(reader.next_rec(&drec, &ctx), FDS_OK);
                EXPECT_EQ(reader.next_rec(&drec, nullptr), FDS_OK);
                EXPECT_EQ(reader.next_rec(&drec, &ctx), FDS_OK);
                EXPECT_EQ(reader.next_rec(&drec, &ctx), FDS_EOC);
            } else {
                // Compression was used during Data Block writing, therefore, it should fail
                EXPECT_THROW({
                    // It can throw anytime during loading or start of Data Record processing
                    reader.load_from_file(m_fd, 0, hint, m_param_io);
                    reader.next_rec(&drec, &ctx);
                }, File_exception);
            }
        }
    }
}

// Try to read malformed Data Block (too short)
TEST_P(DBlock, readTooShortDataBlock)
{
    // Create a Template manager and an IPFIX Template
    uint16_t tid = 300;
    Block_templates tmgr;
    tmplt_tuple_t t1_tuple = gen_t1_tmplt(tid);
    tmgr.add(FDS_TYPE_TEMPLATE, std::get<0>(t1_tuple).get(), std::get<1>(t1_tuple));
    const struct fds_template *t1_parsed = tmgr.get(tid);
    // Create a record
    rec_tuple_t r1_tuple = gen_t1_rec(100, 1000);

    // Create a Data Block
    Block_data_writer writer(0, m_param_alg);
    writer.set_etime(100);
    for (unsigned int i = 0; i < 10; ++i) {
        writer.add(std::get<0>(r1_tuple).get(), std::get<1>(r1_tuple), t1_parsed);
    }
    uint64_t wsize = writer.write_to_file(m_fd, 0, 0, 0, m_param_io);
    EXPECT_GT(wsize, 0U);
    writer.write_wait();

    // Reduce size of the file -> remove the last byte
    ASSERT_EQ(ftruncate(m_fd, wsize - 1), 0);

    // Try to load it (with and without hint)
    for (uint64_t hint : {uint64_t(0), wsize}) {
        SCOPED_TRACE("hint: " + std::to_string(hint));
        Block_data_reader reader(m_param_alg);

        EXPECT_THROW({
            // It can throw anything during loading or start of Data Record processing
            reader.load_from_file(m_fd, 0, hint, m_param_io);
            reader.get_block_header();
        }, File_exception);
    }
}

// Try to write malformed Data Record (e.g. zero size, too short, too long, wrong template)
TEST_P(DBlock, writeInvalidDataRecord)
{
    // Prepare a Template manager
    uint16_t tid1 = 10000, tid2 = 12345, tid3 = 456;
    tmplt_tuple_t t1_tuple = gen_t1_tmplt(tid1);
    tmplt_tuple_t t2_tuple = gen_t2_tmplt(tid2);
    tmplt_tuple_t t3_tuple = gen_t3_tmplt(tid3);

    Block_templates tmgr;
    tmgr.add(FDS_TYPE_TEMPLATE, std::get<0>(t1_tuple).get(), std::get<1>(t1_tuple));
    tmgr.add(FDS_TYPE_TEMPLATE, std::get<0>(t2_tuple).get(), std::get<1>(t2_tuple));
    tmgr.add(FDS_TYPE_TEMPLATE_OPTS, std::get<0>(t3_tuple).get(), std::get<1>(t3_tuple));
    const struct fds_template *t1_parsed = tmgr.get(tid1);
    const struct fds_template *t2_parsed = tmgr.get(tid2);
    const struct fds_template *t3_parsed = tmgr.get(tid3);

    // Prepare few Data Records
    rec_tuple_t r1_tuple = gen_t1_rec();
    uint16_t r1_size = std::get<1>(r1_tuple);
    rec_tuple_t r2_tuple = gen_t2_rec();
    uint16_t r2_size = std::get<1>(r2_tuple);
    rec_tuple_t r3_tuple = gen_t3_rec();
    uint16_t r3_size = std::get<1>(r3_tuple);

    // Try to write zero length Data Record
    {
        Block_data_writer writer(0, m_param_alg);
        writer.set_etime(10);
        EXPECT_THROW(writer.add(std::get<0>(r1_tuple).get(), 0, t1_parsed), File_exception);
        // It should fail without accessing any data
        EXPECT_THROW(writer.add(nullptr, 0, t1_parsed), File_exception);
        EXPECT_EQ(writer.count(), 0U);
    }

    // Try to write shorted Data Records
    {
        Block_data_writer writer(0, m_param_alg);
        writer.set_etime(10);
        EXPECT_THROW(writer.add(std::get<0>(r1_tuple).get(), r1_size - 1, t1_parsed), File_exception);
        EXPECT_THROW(writer.add(std::get<0>(r2_tuple).get(), r2_size - 1, t2_parsed), File_exception);
        EXPECT_THROW(writer.add(std::get<0>(r3_tuple).get(), r3_size - 1, t3_parsed), File_exception);
        EXPECT_EQ(writer.count(), 0U);
    }

    // Try to writer longer Data Records
    {
        uint16_t r1_size_long = r1_size + 1;
        std::unique_ptr<uint8_t[]> r1_longer(new uint8_t[r1_size_long]);
        memset(r1_longer.get(), 0, r1_size_long);
        memcpy(r1_longer.get(), std::get<0>(r1_tuple).get(), r1_size);

        uint16_t r2_size_long = r2_size + 1;
        std::unique_ptr<uint8_t[]> r2_longer(new uint8_t[r2_size_long]);
        memset(r2_longer.get(), 0, r2_size_long);
        memcpy(r2_longer.get(), std::get<0>(r2_tuple).get(), r2_size);

        uint16_t r3_size_long = r3_size + 1;
        std::unique_ptr<uint8_t[]> r3_longer(new uint8_t[r3_size_long]);
        memset(r3_longer.get(), 0, r3_size_long);
        memcpy(r3_longer.get(), std::get<0>(r3_tuple).get(), r3_size);

        Block_data_writer writer(0, m_param_alg);
        EXPECT_THROW(writer.add(r1_longer.get(), r1_size_long, t1_parsed), File_exception);
        EXPECT_THROW(writer.add(r2_longer.get(), r2_size_long, t2_parsed), File_exception);
        EXPECT_THROW(writer.add(r3_longer.get(), r3_size_long, t3_parsed), File_exception);
        EXPECT_EQ(writer.count(), 0U);
    }

    // Try to writer using different IPFIX (Options) Template
    {
        Block_data_writer writer(0, m_param_alg);

        EXPECT_THROW(writer.add(std::get<0>(r1_tuple).get(), std::get<1>(r1_tuple), t2_parsed), File_exception);
        EXPECT_THROW(writer.add(std::get<0>(r1_tuple).get(), std::get<1>(r1_tuple), t3_parsed), File_exception);
        EXPECT_THROW(writer.add(std::get<0>(r2_tuple).get(), std::get<1>(r2_tuple), t1_parsed), File_exception);
        EXPECT_THROW(writer.add(std::get<0>(r2_tuple).get(), std::get<1>(r2_tuple), t3_parsed), File_exception);
        EXPECT_THROW(writer.add(std::get<0>(r3_tuple).get(), std::get<1>(r3_tuple), t1_parsed), File_exception);
        EXPECT_THROW(writer.add(std::get<0>(r3_tuple).get(), std::get<1>(r3_tuple), t2_parsed), File_exception);
        EXPECT_EQ(writer.count(), 0U);
    }
}
