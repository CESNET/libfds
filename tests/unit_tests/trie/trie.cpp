#include <gtest/gtest.h>
#include <libfds.h>
#include <vector>
#include <sstream>

struct IPAddress
{
    int version;
    int bit_length;
    uint32_t value[4] = { 0, 0, 0, 0 };

    void randomize() {
        if (std::rand() % 2) {
            version = 4;
            bit_length = std::rand() % 32 + 1;
        } else {
            version = 6;
            bit_length = std::rand() % 128 + 1;
        }
        value[0] = 0;
        value[1] = 0;
        value[2] = 0;
        value[3] = 0;
        for (int i = 0; i < bit_length; i += 32) {
            value[i / 32] = std::rand();
        }
        if (bit_length % 32 > 0) {
            value[(bit_length + 1) / 32] >>= 32 - bit_length % 32;
            value[(bit_length + 1) / 32] <<= 32 - bit_length % 32;
        }
    }

    bool fuzzy_equals(const IPAddress& other) {
        if (version != other.version) {
            return false;
        }
        int cmp_len = std::min(bit_length, other.bit_length);
        int word_idx = 0;
        while (cmp_len > 32) {
            if (value[word_idx] != other.value[word_idx]) {
                return false;
            }
            cmp_len -= 32;
            word_idx++;
        }
        if (cmp_len > 0) {
            if (value[word_idx] >> (32 - cmp_len) != other.value[word_idx] >> (32 - cmp_len)) {
                return false;
            }
        }
        return true;
    }

    std::string str() {
        std::stringstream ss;
        if (version == 4) {
            ss << ((value[0] >> 24) & 0xFF) << "."
               << ((value[0] >> 16) & 0xFF) << "."
               << ((value[0] >> 8) & 0xFF) << "."
               << (value[0] & 0xFF);
        } else {
            ss << std::hex << ((value[0] >> 24) & 0xFF) << ":"
               << std::hex << ((value[0] >> 16) & 0xFF) << ":"
               << std::hex << ((value[0] >> 8) & 0xFF) << ":"
               << std::hex << (value[0] & 0xFF) << ":";
            ss << std::hex << ((value[1] >> 24) & 0xFF) << ":"
               << std::hex << ((value[1] >> 16) & 0xFF) << ":"
               << std::hex << ((value[1] >> 8) & 0xFF) << ":"
               << std::hex << (value[1] & 0xFF) << ":";
            ss << std::hex << ((value[2] >> 24) & 0xFF) << ":"
               << std::hex << ((value[2] >> 16) & 0xFF) << ":"
               << std::hex << ((value[2] >> 8) & 0xFF) << ":"
               << std::hex << (value[2] & 0xFF) << ":";
            ss << std::hex << ((value[3] >> 24) & 0xFF) << ":"
               << std::hex << ((value[3] >> 16) & 0xFF) << ":"
               << std::hex << ((value[3] >> 8) & 0xFF) << ":"
               << std::hex << (value[3] & 0xFF);
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
    uint32_t address[4] = { };

    void SetUp() override {
        trie = fds_trie_create();
    }

    void TearDown() override {
        fds_trie_destroy(trie);
    }

    void set_ip4_address(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4) {
        address[0] = b4 << 28 | b3 << 16 | b2 << 8 | b1;
        address[1] = 0;
        address[2] = 0;
        address[3] = 0;
    }

    void set_ip6_address(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4,
                         uint8_t b5, uint8_t b6, uint8_t b7, uint8_t b8,
                         uint8_t b9, uint8_t b10, uint8_t b11, uint8_t b12,
                         uint8_t b13, uint8_t b14, uint8_t b15, uint8_t b16) {
        address[0] = b4 << 28 | b3 << 16 | b2 << 8 | b1;
        address[1] = b8 << 28 | b7 << 16 | b6 << 8 | b5;
        address[3] = b12 << 28 | b11 << 16 | b10 << 8 | b9;
        address[4] = b16 << 28 | b15 << 16 | b14 << 8 | b13;
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

    set_ip4_address(127, 0, 0, 2);
    EXPECT_FALSE(fds_trie_find(trie, 4, address, 32));
    set_ip4_address(128, 0, 0, 1);
    EXPECT_FALSE(fds_trie_find(trie, 4, address, 32));

    set_ip4_address(192, 168, 1, 1);
    EXPECT_FALSE(fds_trie_find(trie, 4, address, 32));
    fds_trie_add(trie, 4, address, 32);
    EXPECT_TRUE(fds_trie_find(trie, 4, address, 32));
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