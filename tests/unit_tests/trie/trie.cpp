#include <gtest/gtest.h>
#include <libfds.h>
#include <vector>
#include <sstream>
#include <cstring>

struct IPAddress
{
    int version;
    int bit_length;
    uint8_t value[16] = { 0, 0, 0, 0 };

    void randomize() {
        if (std::rand() % 2) {
            version = 4;
            bit_length = std::rand() % 32 + 1;
        } else {
            version = 6;
            bit_length = std::rand() % 128 + 1;
        }
        std::memset(value, 0, 16);
        for (int i = 0; i < bit_length; i += 8) {
            value[i / 8] = std::rand() % 256;
        }
        if (bit_length % 8 > 0) {
            value[bit_length / 8] >>= 8 - bit_length % 8;
            value[bit_length / 8] <<= 8 - bit_length % 8;
        }
    }

    bool fuzzy_equals(const IPAddress& other) {
        if (version != other.version) {
            return false;
        }
        int cmp_len = std::min(bit_length, other.bit_length);
        int word_idx = 0;
        while (cmp_len > 8) {
            if (value[word_idx] != other.value[word_idx]) {
                return false;
            }
            cmp_len -= 8;
            word_idx++;
        }
        if (cmp_len > 0) {
            if (value[word_idx] >> (8 - cmp_len) != other.value[word_idx] >> (8 - cmp_len)) {
                return false;
            }
        }
        return true;
    }

    std::string str() {
        std::stringstream ss;
        if (version == 4) {
            ss << std::dec << int(value[0]) << "." << int(value[1]) << "." << int(value[2]) << "." << int(value[3]);
        } else {
            ss << std::hex << value[0] << value[1] << ":"
                << value[2] << value[3] << ":"
                << value[4] << value[5] << ":"
                << value[6] << value[7] << ":"
                << value[8] << value[9] << ":"
                << value[10] << value[11] << ":"
                << value[12] << value[13] << ":"
                << value[14] << value[15];
        }
        ss << std::dec << "/" << bit_length;
        return ss.str();
    }
};



int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class Trie: public ::testing::Test
{
protected:
    fds_trie_t *trie = nullptr;
    uint8_t address[16] = { };

    void SetUp() override {
        trie = fds_trie_create();
    }

    void TearDown() override {
        fds_trie_destroy(trie);
    }

    void set_ip4_address(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4) {
        address[0] = b1;
        address[1] = b2;
        address[2] = b3;
        address[3] = b4;
    }

    void set_ip6_address(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4,
                         uint8_t b5, uint8_t b6, uint8_t b7, uint8_t b8,
                         uint8_t b9, uint8_t b10, uint8_t b11, uint8_t b12,
                         uint8_t b13, uint8_t b14, uint8_t b15, uint8_t b16) {
        address[0] = b1;
        address[1] = b2;
        address[2] = b3;
        address[3] = b4;
        address[4] = b5;
        address[5] = b6;
        address[6] = b7;
        address[7] = b8;
        address[8] = b9;
        address[9] = b10;
        address[10] = b11;
        address[11] = b12;
        address[12] = b13;
        address[13] = b14;
        address[14] = b15;
        address[15] = b16;
    }

    std::vector<IPAddress> positives;
    std::vector<IPAddress> negatives;

    bool is_unique(IPAddress &addr, std::vector<IPAddress> &addr_vec)
    {
        for (IPAddress &a : addr_vec) {
            if (a.fuzzy_equals(addr)) {
                return false;
            }
        }
        return true;
    }

    void generate_addresses(int n_pos, int n_neg)
    {
        for (int i = 0; i < n_pos; i++) {
            IPAddress addr;
            do {
                addr.randomize();
            } while (!is_unique(addr, positives));
            positives.push_back(addr);
        }
        for (int i = 0; i < n_neg; i++) {
            IPAddress addr;
            do {
                addr.randomize();
            } while (!is_unique(addr, positives) || !is_unique(addr, negatives));
            negatives.push_back(addr);
        }
    }

};

TEST_F(Trie, basic)
{
    set_ip4_address(127, 0, 0, 1);
    EXPECT_FALSE(fds_trie_find(trie, 4, address, 32));
    fds_trie_add(trie, 4, address, 32);
    EXPECT_TRUE(fds_trie_find(trie, 4, address, 32));
    fds_trie_print(trie);

    set_ip4_address(127, 0, 0, 2);
    EXPECT_FALSE(fds_trie_find(trie, 4, address, 32));
    set_ip4_address(128, 0, 0, 1);
    EXPECT_FALSE(fds_trie_find(trie, 4, address, 32));

    set_ip4_address(192, 168, 1, 1);
    EXPECT_FALSE(fds_trie_find(trie, 4, address, 32));
    fds_trie_add(trie, 4, address, 32);
    EXPECT_TRUE(fds_trie_find(trie, 4, address, 32));
    fds_trie_print(trie);
}

TEST_F(Trie, basic_ipv6) {
    set_ip6_address(0xaa, 0xbb, 0xcc, 0xdd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
    EXPECT_FALSE(fds_trie_find(trie, 6, address, 128));
    fds_trie_add(trie, 6, address, 128);
    EXPECT_TRUE(fds_trie_find(trie, 6, address, 128));
}

TEST_F(Trie, mixed) {
    struct address_info {
        int version;
        int prefix_len;
        uint8_t bytes[16];
    };

    std::vector<address_info> addresses_to_add = {
        { 4, 32, { 127, 0, 0, 1 } },
        { 4, 32, { 192, 168, 1, 25 } },
        { 4, 32, { 85, 132, 197, 60 } },
        { 4, 32, { 1, 1, 1, 1 } },
        { 4, 32, { 8, 8, 8, 8 } },
        { 4, 32, { 4, 4, 4, 4 } },
        { 6, 128, { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 } }
    };

    std::vector<address_info> addresses_to_test = {
        { 6, 128, { 0xaa, 0xbb, 0xcc, 0xdd, 0x00 } },
        { 6, 128, { 0x11, 0x22, 0x33, 0x44, 0x55 } },
        { 6, 128, { 0xff, 0xff, 0xff, 0xff, 0xff } }
    };

    for (auto addr : addresses_to_add) {
        EXPECT_FALSE(
            fds_trie_find(trie, addr.version, addr.bytes, addr.prefix_len)
        );
        fds_trie_add(trie, addr.version, addr.bytes, addr.prefix_len);
        EXPECT_TRUE(
            fds_trie_find(trie, addr.version, addr.bytes, addr.prefix_len)
        );
    }

    for (auto addr : addresses_to_test) {
        EXPECT_FALSE(
            fds_trie_find(trie, addr.version, addr.bytes, addr.prefix_len)
        );
    }

    for (auto addr : addresses_to_add) {
        EXPECT_TRUE(
            fds_trie_find(trie, addr.version, addr.bytes, addr.prefix_len)
        );
    }

}

TEST_F(Trie, subnets)
{
    set_ip4_address(127, 0, 0, 1);
    EXPECT_FALSE(fds_trie_find(trie, 4, address, 32));
    EXPECT_FALSE(fds_trie_find(trie, 4, address, 30));
    EXPECT_FALSE(fds_trie_find(trie, 4, address, 25));
    EXPECT_FALSE(fds_trie_find(trie, 4, address, 24));
    fds_trie_add(trie, 4, address, 24);
    EXPECT_TRUE(fds_trie_find(trie, 4, address, 32));
    EXPECT_TRUE(fds_trie_find(trie, 4, address, 30));
    EXPECT_TRUE(fds_trie_find(trie, 4, address, 25));
    EXPECT_TRUE(fds_trie_find(trie, 4, address, 24));
    EXPECT_FALSE(fds_trie_find(trie, 4, address, 23));
    EXPECT_FALSE(fds_trie_find(trie, 4, address, 12));
    EXPECT_FALSE(fds_trie_find(trie, 4, address, 8));

    set_ip4_address(192, 168, 1, 1);
    EXPECT_FALSE(fds_trie_find(trie, 4, address, 32));
    EXPECT_FALSE(fds_trie_find(trie, 4, address, 30));
    EXPECT_FALSE(fds_trie_find(trie, 4, address, 25));
    EXPECT_FALSE(fds_trie_find(trie, 4, address, 24));
    fds_trie_add(trie, 4, address, 24);
    EXPECT_TRUE(fds_trie_find(trie, 4, address, 32));
    EXPECT_TRUE(fds_trie_find(trie, 4, address, 30));
    EXPECT_TRUE(fds_trie_find(trie, 4, address, 25));
    EXPECT_TRUE(fds_trie_find(trie, 4, address, 24));
    EXPECT_FALSE(fds_trie_find(trie, 4, address, 23));
    EXPECT_FALSE(fds_trie_find(trie, 4, address, 12));
    EXPECT_FALSE(fds_trie_find(trie, 4, address, 8));
}

TEST_F(Trie, randomly_generated_addresses)
{
    generate_addresses(10000, 3000);

    for (IPAddress &addr : positives) {
        EXPECT_FALSE(fds_trie_find(trie, addr.version, addr.value, addr.bit_length));
        fds_trie_add(trie, addr.version, addr.value, addr.bit_length);
        EXPECT_TRUE(fds_trie_find(trie, addr.version, addr.value, addr.bit_length));
    }
    for (IPAddress &addr : negatives) {
        EXPECT_FALSE(fds_trie_find(trie, addr.version, addr.value, addr.bit_length));
    }
}