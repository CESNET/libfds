#include <string>
#include <memory>
#include <cstdlib>
#include <gtest/gtest.h>
#include <libfds.h>
#include <MsgGen.h>

// Unique pointer able to handle IPFIX Set
using set_uniq = std::unique_ptr<fds_ipfix_set_hdr, decltype(&free)>;
using uint8_uniq = std::unique_ptr<uint8_t, decltype(&free)>;

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

static const std::string NO_ERR_STRING = "No error.";

// Data Set (based on Template) with static fields only
TEST(dsetIter, staticFields)
{
    // For both types of Templates
    std::vector<enum fds_template_type> tmplt_types{FDS_TYPE_TEMPLATE, FDS_TYPE_TEMPLATE_OPTS};
    for (auto type : tmplt_types) {
        SCOPED_TRACE("Type " + std::to_string(type));
        // Prepare a template
        ipfix_trec tmplt_raw = (type == FDS_TYPE_TEMPLATE) ? ipfix_trec(256) : ipfix_trec(256, 2);
        tmplt_raw.add_field(10, 4);
        tmplt_raw.add_field(20, 8, 20);
        tmplt_raw.add_field(30, 3);
        tmplt_raw.add_field(40, 100);
        tmplt_raw.add_field(50, 1, 200);
        const uint16_t rec_size = 116; // -> Total length of a Data Record is 116 bytes

        uint16_t tmplt_size = tmplt_raw.size();
        uint8_uniq tmplt_data(tmplt_raw.release(), &free);
        struct fds_template *tmplt;
        ASSERT_EQ(fds_template_parse(type, tmplt_data.get(), &tmplt_size, &tmplt), FDS_OK);
        EXPECT_EQ(tmplt->data_length, rec_size);

        // Prepare a Data Set
        ipfix_drec rec1 {};
        rec1.append_uint(11, 4);
        rec1.append_uint(21, 8);
        rec1.append_uint(31, 3);
        rec1.append_string("41", 100);
        rec1.append_bool(true);

        ipfix_drec rec2 {};
        rec2.append_uint(12, 4);
        rec2.append_uint(22, 8);
        rec2.append_uint(32, 3);
        rec2.append_string("42", 100);
        rec2.append_bool(false);

        ipfix_drec rec3 {};
        rec3.append_uint(13, 4);
        rec3.append_uint(23, 8);
        rec3.append_uint(33, 3);
        rec3.append_string("43", 100);
        rec3.append_bool(true);

        // Data Set with only one record
        {
            ipfix_set set {256};
            set.add_rec(rec1);
            set_uniq hdr_set(set.release(), &free);
            fds_dset_iter iter;
            fds_dset_iter_init(&iter, hdr_set.get(), tmplt);

            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.size, rec_size);
            EXPECT_EQ(iter.rec, reinterpret_cast<uint8_t *>(hdr_set.get() + 1));
            // Try to get the first value
            uint64_t rec_value;
            EXPECT_EQ(fds_get_uint_be(iter.rec, 4, &rec_value), FDS_OK);
            EXPECT_EQ(rec_value, 11);

            // End has been reached
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_ERR_NOTFOUND);
            EXPECT_EQ(fds_dset_iter_err(&iter), NO_ERR_STRING);
        }

        // Data Set with multiple records
        {
            uint8_t *next_pos;
            uint64_t rec_value;

            ipfix_set set_multi {256};
            set_multi.add_rec(rec1);
            set_multi.add_rec(rec2);
            set_multi.add_rec(rec3);

            set_uniq hdr_set_multi(set_multi.release(), &free);
            fds_dset_iter iter;
            fds_dset_iter_init(&iter, hdr_set_multi.get(), tmplt);

            // 1. record
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.size, rec_size);
            EXPECT_EQ(iter.rec, reinterpret_cast<uint8_t *>(hdr_set_multi.get() + 1));
            next_pos = iter.rec + iter.size;
            EXPECT_EQ(fds_get_uint_be(iter.rec, 4, &rec_value), FDS_OK);
            EXPECT_EQ(rec_value, 11);

            // 2. record
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.size, rec_size);
            EXPECT_EQ(iter.rec, next_pos);
            next_pos = iter.rec + iter.size;
            EXPECT_EQ(fds_get_uint_be(iter.rec, 4, &rec_value), FDS_OK);
            EXPECT_EQ(rec_value, 12);
            // 3. record
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.size, rec_size);
            EXPECT_EQ(iter.rec, next_pos);
            EXPECT_EQ(fds_get_uint_be(iter.rec, 4, &rec_value), FDS_OK);
            EXPECT_EQ(rec_value, 13);

            // End has been reached
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_ERR_NOTFOUND);
            EXPECT_EQ(fds_dset_iter_err(&iter), NO_ERR_STRING);
        }

        // Data Set with padding
        {
            uint8_t *next_pos;
            uint64_t rec_value;

            ipfix_set set_multi {256};
            set_multi.add_rec(rec1);
            set_multi.add_rec(rec2);
            set_multi.add_padding(rec_size - 1); // The max. possible padding

            set_uniq hdr_set_multi(set_multi.release(), &free);
            fds_dset_iter iter;
            fds_dset_iter_init(&iter, hdr_set_multi.get(), tmplt);

            // 1. record
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.size, rec_size);
            EXPECT_EQ(iter.rec, reinterpret_cast<uint8_t *>(hdr_set_multi.get() + 1));
            next_pos = iter.rec + iter.size;
            EXPECT_EQ(fds_get_uint_be(iter.rec, 4, &rec_value), FDS_OK);
            EXPECT_EQ(rec_value, 11);
            // 2. record
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.size, rec_size);
            EXPECT_EQ(iter.rec, next_pos);
            EXPECT_EQ(fds_get_uint_be(iter.rec, 4, &rec_value), FDS_OK);
            EXPECT_EQ(rec_value, 12);

            // End has been reached
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_ERR_NOTFOUND);
            EXPECT_EQ(fds_dset_iter_err(&iter), NO_ERR_STRING);
        }

        fds_template_destroy(tmplt);
    }
}

// A single variable-length Information Element
TEST(dsetIter, singleVarField)
{
    // For both types of Templates
    std::vector<enum fds_template_type> tmplt_types{FDS_TYPE_TEMPLATE, FDS_TYPE_TEMPLATE_OPTS};
    for (auto type : tmplt_types) {
        SCOPED_TRACE("Type " + std::to_string(type));
        // Prepare a template
        ipfix_trec tmplt_raw = (type == FDS_TYPE_TEMPLATE) ? ipfix_trec(256) : ipfix_trec(256, 1);
        tmplt_raw.add_field(10, ipfix_trec::SIZE_VAR);

        uint16_t tmplt_size = tmplt_raw.size();
        uint8_uniq tmplt_data(tmplt_raw.release(), &free);
        struct fds_template *tmplt;
        ASSERT_EQ(fds_template_parse(type, tmplt_data.get(), &tmplt_size, &tmplt), FDS_OK);

        std::string str1 = ""; // empty string
        std::string str2 = "0123456789";
        std::string str3 = "https://tools.ietf.org/html/rfc7011";

        // Prepare a Data Set
        ipfix_drec rec1 {};
        rec1.var_header(str1.length());         // Add header manually (short version) - empty

        ipfix_drec rec2 {};
        rec2.var_header(str2.length(), true);   // Add header manually (long version)
        rec2.append_string(str2, str2.length());

        ipfix_drec rec3 {};
        rec3.append_string(str3);               // Add header automatically (short version)

        // Data Set with only one record
        {
            ipfix_set set {256};
            set.add_rec(rec1);
            set_uniq hdr_set(set.release(), &free);
            fds_dset_iter iter;
            fds_dset_iter_init(&iter, hdr_set.get(), tmplt);

            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.size, rec1.size());
            EXPECT_EQ(iter.rec, reinterpret_cast<uint8_t *>(hdr_set.get() + 1));

            // End has been reached
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_ERR_NOTFOUND);
            EXPECT_EQ(fds_dset_iter_err(&iter), NO_ERR_STRING);
        }

        // Data Set with multiple records
        {
            uint8_t *next_pos;

            ipfix_set set_multi {256};
            set_multi.add_rec(rec1);
            set_multi.add_rec(rec2);
            set_multi.add_rec(rec3);

            set_uniq hdr_set_multi(set_multi.release(), &free);
            fds_dset_iter iter;
            fds_dset_iter_init(&iter, hdr_set_multi.get(), tmplt);

            // 1. record
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.size, rec1.size());
            EXPECT_EQ(iter.rec, reinterpret_cast<uint8_t *>(hdr_set_multi.get() + 1));
            next_pos = iter.rec + iter.size;

            // 2. record
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.size, rec2.size());
            EXPECT_EQ(iter.rec, next_pos);
            next_pos = iter.rec + iter.size;

            // 3. record
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.size, rec3.size());
            EXPECT_EQ(iter.rec, next_pos);

            // End has been reached
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_ERR_NOTFOUND);
            EXPECT_EQ(fds_dset_iter_err(&iter), NO_ERR_STRING);
        }

        // Data Set with padding -> no padding can be used in this situation
        fds_template_destroy(tmplt);
    }
}

/* A variable-length Information Element followed by a fixed-length Information Element
 * and a fixed-length Information Element followed by a variable-length Information Element
 */
TEST(dsetIter, mixVarAndFixed)
{
    // For both types of Templates
    std::vector<enum fds_template_type> tmplt_types{FDS_TYPE_TEMPLATE, FDS_TYPE_TEMPLATE_OPTS};
    for (auto type : tmplt_types) {
        SCOPED_TRACE("Type " + std::to_string(type));
        // Prepare a template
        ipfix_trec tmplt_raw = (type == FDS_TYPE_TEMPLATE) ? ipfix_trec(256) : ipfix_trec(256, 1);
        tmplt_raw.add_field(10, 4);
        tmplt_raw.add_field(20, ipfix_trec::SIZE_VAR);
        tmplt_raw.add_field(30, 8);

        uint16_t tmplt_size = tmplt_raw.size();
        uint8_uniq tmplt_data(tmplt_raw.release(), &free);
        struct fds_template *tmplt;
        ASSERT_EQ(fds_template_parse(type, tmplt_data.get(), &tmplt_size, &tmplt), FDS_OK);

        std::string str1 = ""; // empty string
        std::string str2 = "Ultra Mega Giga . . . string";

        // Prepare a Data Set
        ipfix_drec rec1 {};
        rec1.append_uint(11, 4);
        rec1.var_header(str1.length());         // Add header manually (short version) - empty
        rec1.append_uint(31, 8);

        ipfix_drec rec2 {};
        rec2.append_uint(12, 4);
        rec2.var_header(str2.length(), true);   // Add header manually (long version)
        rec2.append_string(str2, str2.length());
        rec2.append_uint(32, 8);

        ipfix_drec rec3 {};
        rec3.append_uint(13, 4);
        rec3.append_string(str2);               // Add header automatically (short version)
        rec3.append_uint(32, 8);

        // Data Set with only one record
        {
            uint64_t rec_value;

            ipfix_set set {256};
            set.add_rec(rec1);
            set_uniq hdr_set(set.release(), &free);
            fds_dset_iter iter;
            fds_dset_iter_init(&iter, hdr_set.get(), tmplt);

            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.rec, reinterpret_cast<uint8_t *>(hdr_set.get() + 1));
            EXPECT_EQ(iter.size, rec1.size());
            EXPECT_EQ(fds_get_uint_be(iter.rec, 4, &rec_value), FDS_OK);
            EXPECT_EQ(rec_value, 11);

            // End has been reached
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_ERR_NOTFOUND);
            EXPECT_EQ(fds_dset_iter_err(&iter), NO_ERR_STRING);
        }

        // Data Set with multiple records
        {
            uint8_t *next_pos;
            uint64_t rec_value;

            ipfix_set set_multi {256};
            set_multi.add_rec(rec1);
            set_multi.add_rec(rec2);
            set_multi.add_rec(rec3);

            set_uniq hdr_set_multi(set_multi.release(), &free);
            fds_dset_iter iter;
            fds_dset_iter_init(&iter, hdr_set_multi.get(), tmplt);

            // 1. record
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.rec, reinterpret_cast<uint8_t *>(hdr_set_multi.get() + 1));
            EXPECT_EQ(iter.size, rec1.size());
            next_pos = iter.rec + iter.size;
            EXPECT_EQ(fds_get_uint_be(iter.rec, 4, &rec_value), FDS_OK);
            EXPECT_EQ(rec_value, 11);

            // 2. record
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.rec, next_pos);
            EXPECT_EQ(iter.size, rec2.size());
            next_pos = iter.rec + iter.size;
            EXPECT_EQ(fds_get_uint_be(iter.rec, 4, &rec_value), FDS_OK);
            EXPECT_EQ(rec_value, 12);
            // 3. record
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.rec, next_pos);
            EXPECT_EQ(iter.size, rec3.size());
            EXPECT_EQ(fds_get_uint_be(iter.rec, 4, &rec_value), FDS_OK);
            EXPECT_EQ(rec_value, 13);

            // End has been reached
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_ERR_NOTFOUND);
            EXPECT_EQ(fds_dset_iter_err(&iter), NO_ERR_STRING);
        }

        // Data Set with padding
        {
            uint8_t *next_pos;
            uint64_t rec_value;
            uint16_t min_rec_size = tmplt->data_length;

            ipfix_set set_multi {256};
            set_multi.add_rec(rec1);
            set_multi.add_rec(rec2);
            set_multi.add_padding(min_rec_size - 1); // The max. possible padding

            set_uniq hdr_set_multi(set_multi.release(), &free);
            fds_dset_iter iter;
            fds_dset_iter_init(&iter, hdr_set_multi.get(), tmplt);

            // 1. record
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.rec, reinterpret_cast<uint8_t *>(hdr_set_multi.get() + 1));
            EXPECT_EQ(iter.size, rec1.size());
            next_pos = iter.rec + iter.size;
            EXPECT_EQ(fds_get_uint_be(iter.rec, 4, &rec_value), FDS_OK);
            EXPECT_EQ(rec_value, 11);
            // 2. record
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.size, rec2.size());
            EXPECT_EQ(iter.rec, next_pos);
            EXPECT_EQ(fds_get_uint_be(iter.rec, 4, &rec_value), FDS_OK);
            EXPECT_EQ(rec_value, 12);

            // End has been reached
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_ERR_NOTFOUND);
            EXPECT_EQ(fds_dset_iter_err(&iter), NO_ERR_STRING);
        }

        fds_template_destroy(tmplt);
    }
}

// Multiple variable-length Information Elements
TEST(dsetIter, multipeVarFields)
{
    // For both types of Templates
    std::vector<enum fds_template_type> tmplt_types{FDS_TYPE_TEMPLATE, FDS_TYPE_TEMPLATE_OPTS};
    for (auto type : tmplt_types) {
        SCOPED_TRACE("Type " + std::to_string(type));
        // Prepare a template
        ipfix_trec tmplt_raw = (type == FDS_TYPE_TEMPLATE) ? ipfix_trec(256) : ipfix_trec(256, 2);
        tmplt_raw.add_field(10, ipfix_trec::SIZE_VAR);
        tmplt_raw.add_field(20, ipfix_trec::SIZE_VAR);
        tmplt_raw.add_field(30, ipfix_trec::SIZE_VAR);

        uint16_t tmplt_size = tmplt_raw.size();
        uint8_uniq tmplt_data(tmplt_raw.release(), &free);
        struct fds_template *tmplt;
        ASSERT_EQ(fds_template_parse(type, tmplt_data.get(), &tmplt_size, &tmplt), FDS_OK);

        std::string str1 = "exampleShowString";
        std::string str2 = ""; // Empty string
        std::string str3 = "veryLongStringThatCannotBeLongerOrCanBe?";
        std::string str4 = "12345";

        // Prepare a Data Set
        ipfix_drec rec1 {};
        rec1.var_header(str1.length());       // Add header manually (short version)
        rec1.append_string(str1, str1.length());
        rec1.var_header(str3.length(), true); // Add header manually (long version)
        rec1.append_string(str3, str3.length());
        rec1.var_header(str2.length(), true); // Add header manually (long version) - empty

        ipfix_drec rec2 {};
        rec2.append_string(str1);       // Add header automatically (short version)
        rec2.var_header(str2.length()); // Add header manually      (short version) - empty
        rec2.append_string(str3);       // Add header automatically (short version)

        ipfix_drec rec3 {};
        rec3.var_header(str2.length());       // Add header manually (short version) - empty
        rec3.append_string(str4);             // Add header automatically (short version)
        rec3.var_header(str4.length(), true); // Add header manually (long version)
        rec3.append_string(str4, str4.length());

        // Data Set with only one record
        {
            ipfix_set set {256};
            set.add_rec(rec1);
            set_uniq hdr_set(set.release(), &free);
            fds_dset_iter iter;
            fds_dset_iter_init(&iter, hdr_set.get(), tmplt);

            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.rec, reinterpret_cast<uint8_t *>(hdr_set.get() + 1));
            EXPECT_EQ(iter.size, rec1.size());

            // End has been reached
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_ERR_NOTFOUND);
            EXPECT_EQ(fds_dset_iter_err(&iter), NO_ERR_STRING);
        }

        // Data Set with multiple records
        {
            uint8_t *next_pos;

            ipfix_set set_multi {256};
            set_multi.add_rec(rec1);
            set_multi.add_rec(rec2);
            set_multi.add_rec(rec3);

            set_uniq hdr_set_multi(set_multi.release(), &free);
            fds_dset_iter iter;
            fds_dset_iter_init(&iter, hdr_set_multi.get(), tmplt);

            // 1. record
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.rec, reinterpret_cast<uint8_t *>(hdr_set_multi.get() + 1));
            EXPECT_EQ(iter.size, rec1.size());
            next_pos = iter.rec + iter.size;

            // 2. record
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.rec, next_pos);
            EXPECT_EQ(iter.size, rec2.size());
            next_pos = iter.rec + iter.size;

            // 3. record
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.size, rec3.size());
            EXPECT_EQ(iter.rec, next_pos);

            // End has been reached
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_ERR_NOTFOUND);
            EXPECT_EQ(fds_dset_iter_err(&iter), NO_ERR_STRING);
        }

        // Data Set with padding
        {
            uint8_t *next_pos;

            ipfix_set set_multi {256};
            set_multi.add_rec(rec1);
            set_multi.add_rec(rec2);
            /* Because the template has 3 variable fields (each min. size is 1 byte), minimal valid
             * record length is 3 bytes -> padding max 2 bytes
             */
            set_multi.add_padding(2);

            set_uniq hdr_set_multi(set_multi.release(), &free);
            fds_dset_iter iter;
            fds_dset_iter_init(&iter, hdr_set_multi.get(), tmplt);

            // 1. record
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.rec, reinterpret_cast<uint8_t *>(hdr_set_multi.get() + 1));
            EXPECT_EQ(iter.size, rec1.size());
            next_pos = iter.rec + iter.size;
            // 2. record
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_OK);
            EXPECT_EQ(iter.rec, next_pos);
            EXPECT_EQ(iter.size, rec2.size());
            next_pos = iter.rec + iter.size;

            // End has been reached
            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_ERR_NOTFOUND);
            EXPECT_EQ(fds_dset_iter_err(&iter), NO_ERR_STRING);
        }

        // Data Set with padding -> no padding can be used in this situation
        fds_template_destroy(tmplt);
    }
}

// Malformed sets --------------------------------------------------------------------------------

// Empty Data set
TEST(dsetIterMalformed, empty)
{
    // For both types of Templates
    std::vector<enum fds_template_type> tmplt_types{FDS_TYPE_TEMPLATE, FDS_TYPE_TEMPLATE_OPTS};
    for (auto type : tmplt_types) {
        SCOPED_TRACE("Type " + std::to_string(type));
        // Prepare a template
        ipfix_trec tmplt_raw = (type == FDS_TYPE_TEMPLATE) ? ipfix_trec(256) : ipfix_trec(256, 1);
        tmplt_raw.add_field(10, 4);
        tmplt_raw.add_field(20, ipfix_trec::SIZE_VAR);
        tmplt_raw.add_field(30, 8);

        uint16_t tmplt_size = tmplt_raw.size();
        uint8_uniq tmplt_data(tmplt_raw.release(), &free);
        struct fds_template *tmplt;
        ASSERT_EQ(fds_template_parse(type, tmplt_data.get(), &tmplt_size, &tmplt), FDS_OK);

        // Create an empty Set
        {
            ipfix_set set {256};
            set_uniq hdr_set(set.release(), &free);
            fds_dset_iter iter;
            fds_dset_iter_init(&iter, hdr_set.get(), tmplt);

            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_ERR_FORMAT);
            EXPECT_NE(fds_dset_iter_err(&iter), NO_ERR_STRING);
        }

        // Create an empty Set with padding
        {
            ipfix_set set {256};
            set.add_padding(tmplt->data_length - 1);
            set_uniq hdr_set(set.release(), &free);
            fds_dset_iter iter;
            fds_dset_iter_init(&iter, hdr_set.get(), tmplt);

            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_ERR_FORMAT);
            EXPECT_NE(fds_dset_iter_err(&iter), NO_ERR_STRING);
        }

        fds_template_destroy(tmplt);
    }
}

// Variable-length record is longer than its enclosing Set
TEST(dsetIterMalformed, tooLongVar)
{
    // For both types of Templates
    std::vector<enum fds_template_type> tmplt_types{FDS_TYPE_TEMPLATE, FDS_TYPE_TEMPLATE_OPTS};
    for (auto type : tmplt_types) {
        SCOPED_TRACE("Type " + std::to_string(type));
        // Prepare a template
        ipfix_trec tmplt_raw = (type == FDS_TYPE_TEMPLATE) ? ipfix_trec(256) : ipfix_trec(256, 1);
        tmplt_raw.add_field(10, 4);
        tmplt_raw.add_field(20, ipfix_trec::SIZE_VAR);
        tmplt_raw.add_field(30, ipfix_trec::SIZE_VAR);
        tmplt_raw.add_field(40, 8); // -> min. valid record is 4 + 1 + 1 + 8 = 14 bytes long

        uint16_t tmplt_size = tmplt_raw.size();
        uint8_uniq tmplt_data(tmplt_raw.release(), &free);
        struct fds_template *tmplt;
        ASSERT_EQ(fds_template_parse(type, tmplt_data.get(), &tmplt_size, &tmplt), FDS_OK);

        // Example 1 (short var. len. header, -1 byte)
        {
            ipfix_drec rec;
            rec.append_uint(4, 4);
            rec.append_string("Some random string"); // Automatically add var. header (short)
            rec.append_string("Some random string"); // Automatically add var. header (short)
            rec.append_uint(8, 8);

            ipfix_set set{256};
            set.add_rec(rec);
            set.overwrite_len(set.size() - 1); // Overwrite size of the Set (minus 1 byte)

            set_uniq hdr_set(set.release(), &free);
            fds_dset_iter iter;
            fds_dset_iter_init(&iter, hdr_set.get(), tmplt);

            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_ERR_FORMAT);
            EXPECT_NE(fds_dset_iter_err(&iter), NO_ERR_STRING);
        }

        // Example 2 (short var.len. header, variable-length field ends behind its enclosing Set)
        {
            ipfix_drec rec;
            rec.append_uint(4, 4);
            rec.append_string("Some random string"); // Automatically add var. header (short)
            rec.append_string("Some random string"); // Automatically add var. header (short)
            rec.append_uint(8, 8);

            ipfix_set set{256};
            set.add_rec(rec);
            set.overwrite_len(set.size() - 10); // Overwrite size of the Set (minus 10 bytes)

            set_uniq hdr_set(set.release(), &free);
            fds_dset_iter iter;
            fds_dset_iter_init(&iter, hdr_set.get(), tmplt);

            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_ERR_FORMAT);
            EXPECT_NE(fds_dset_iter_err(&iter), NO_ERR_STRING);
        }

        // Example 3 (short var.len. header, variable-length header corrupted - behind Set end)
        {
            ipfix_drec rec;
            std::string str = "Some random string";
            rec.append_uint(4, 4);
            rec.var_header(20, false);              // Manually add var. header (short)
            rec.append_string("Random string", 20);
            rec.append_string(str);                 // Automatically add var. header (short)
            rec.append_uint(8, 8);

            ipfix_set set{256};
            set.add_rec(rec);
            // Make the variable-length header behind the Set end (+ 4 + 20 == size of the first 2 fields)
            uint16_t new_set_len = FDS_IPFIX_SET_HDR_LEN + 4 + 20;
            set.overwrite_len(new_set_len);

            set_uniq hdr_set(set.release(), &free);
            fds_dset_iter iter;
            fds_dset_iter_init(&iter, hdr_set.get(), tmplt);

            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_ERR_FORMAT);
            EXPECT_NE(fds_dset_iter_err(&iter), NO_ERR_STRING);
        }

        // Example 4 (long var. len. header, variable-length header corrupted - behind the Set end))
        {
            ipfix_drec rec;
            std::string str = "Some random string";
            rec.append_uint(4, 4);
            rec.var_header(20, false);              // Manually add var. header (short)
            rec.append_string("Random string", 20);
            rec.var_header(str.length(), true);     // Add manually long header
            rec.append_string(str, str.length());
            rec.append_uint(8, 8);

            ipfix_set set{256};
            set.add_rec(rec);
            // Make the second part of the long variable-length header partly behind the Set end (+2)
            uint16_t new_set_len = FDS_IPFIX_SET_HDR_LEN + 4 + 20 + 2;
            set.overwrite_len(new_set_len);

            set_uniq hdr_set(set.release(), &free);
            fds_dset_iter iter;
            fds_dset_iter_init(&iter, hdr_set.get(), tmplt);

            EXPECT_EQ(fds_dset_iter_next(&iter), FDS_ERR_FORMAT);
            EXPECT_NE(fds_dset_iter_err(&iter), NO_ERR_STRING);
        }

        fds_template_destroy(tmplt);
    }
}
