#include <cstdint>
#include <cstring>
#include <vector>
#include <functional>
#include <libfds.h>
#include <gtest/gtest.h>

struct address_t {
    int version;
    int bit_length;
    std::uint8_t bytes[16];
};

std::vector<address_t> addresses {
#include "ip_list.inc"
};

std::vector<address_t> blacklist;
std::vector<address_t> testlist;

int main(int argc, char **argv)
{
    int i = 0;
    for (; i < addresses.size() / 2; i++) {
        blacklist.push_back(addresses[i]);
    }
    for (; i < addresses.size(); i++) {
        testlist.push_back(addresses[i]);
    }
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

enum class identifier : int {
    ip = 1,
    blacklist = 1,
};

int lookup_callback(const char *name, void *user_context, struct fds_filter_identifier_attributes *attributes)
{
    if (std::strcmp(name, "ip") == 0) {
        attributes->id = int(identifier::ip);
        attributes->identifier_type = FDS_FILTER_IDENTIFIER_FIELD;
        attributes->type = FDS_FILTER_TYPE_IP_ADDRESS;
        return FDS_FILTER_OK;
    } else if (std::strcmp(name, "blacklist") == 0) {
        attributes->id = int(identifier::blacklist);
        attributes->identifier_type = FDS_FILTER_IDENTIFIER_CONST;
        attributes->type = FDS_FILTER_TYPE_LIST;
        attributes->subtype = FDS_FILTER_TYPE_IP_ADDRESS;
        return FDS_FILTER_OK;
    } else {
        return FDS_FILTER_FAIL;
    }
}

void const_callback(int id, void *user_context, union fds_filter_value *value)
{
    switch (id) {
    case int(identifier::blacklist): {
        value->list.length = blacklist.size();
        value->list.items = (union fds_filter_value *)malloc(sizeof(union fds_filter_value) * value->list.length);
        ASSERT_TRUE(value->list.items != NULL);
        for (int i = 0; i < blacklist.size(); i++) {
            value->list.items[i].ip_address.version = blacklist[i].version;
            value->list.items[i].ip_address.mask = blacklist[i].bit_length;
            std::memcpy(value->list.items[i].ip_address.bytes, blacklist[i].bytes, 16);
        }
    } break;
    default: assert(0);
    }
}

int field_callback(int id, void *user_context_, int reset_flag, void *input_data, union fds_filter_value *value)
{
    address_t *address = reinterpret_cast<address_t *>(input_data);
    switch (id) {
    case int(identifier::ip): {
        value->ip_address.version = address->version;
        value->ip_address.mask = address->bit_length;
        std::memcpy(value->ip_address.bytes, address->bytes, 16);
        return FDS_FILTER_OK;
    } break;
    default: assert(0);
    }
}

TEST(Filter, ip_list) {
    fds_filter_t *filter = fds_filter_create();
    ASSERT_TRUE(filter != NULL);

    fds_filter_set_lookup_callback(filter, lookup_callback);
    fds_filter_set_const_callback(filter, const_callback);
    fds_filter_set_field_callback(filter, field_callback);

    ASSERT_TRUE(
        fds_filter_compile(filter, "ip in blacklist") == FDS_FILTER_OK
    );

    for (auto &address : testlist) {
        EXPECT_FALSE(
            fds_filter_evaluate(filter, &address)
        );
    }

    for (auto &address : blacklist) {
        EXPECT_TRUE(
            fds_filter_evaluate(filter, &address)
        );
    }

    fds_filter_destroy(filter);
}