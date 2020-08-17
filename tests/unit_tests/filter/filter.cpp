#include <libfds.h>
#include <gtest/gtest.h>
#include <iostream>

class Filter : public ::testing::Test {
public:
    fds_filter_opts_t *opts = nullptr;
    fds_filter_t *filter = nullptr;
    void *user_ctx = nullptr;

    Filter() {
        opts = fds_filter_create_default_opts();
        assert(opts);
    }

    Filter(Filter &) = delete;
    Filter(Filter &&) = delete;

    ~Filter() {
        fds_filter_destroy(filter);
        fds_filter_destroy_opts(opts);
    }


    int compile(const char *expr) {
        fds_filter_destroy(filter);
        filter = nullptr;
        fds_filter_opts_set_user_ctx(opts, user_ctx);
        int ret = fds_filter_create(&filter, expr, opts);
        if (ret != FDS_OK) {
            return ret;
        }
        return FDS_OK;
    }

    bool evaluate(const char *expr) {
        int ret = compile(expr);
        assert(ret == FDS_OK);
        return fds_filter_eval(filter, nullptr);
    }

    int evaluate() {
        return fds_filter_eval(filter, nullptr);
    }
};

TEST_F(Filter, literals_int) {
    EXPECT_EQ(compile("1"), FDS_OK);
    EXPECT_EQ(compile("-1"), FDS_OK);
    EXPECT_EQ(compile("10000"), FDS_OK);
    EXPECT_EQ(compile("465464894616548498"), FDS_OK);
    EXPECT_LT(compile("465464894a616548498"), FDS_OK);
}

TEST_F(Filter, literals_int_bases) {
    EXPECT_EQ(compile("0x123"), FDS_OK);
    EXPECT_EQ(compile("0xF123AF"), FDS_OK);
    EXPECT_EQ(compile("-0xF123AF"), FDS_OK);
    EXPECT_LT(compile("0xF123AG"), FDS_OK);
    EXPECT_LT(compile("0xGF123AG"), FDS_OK);

    EXPECT_EQ(compile("0b000"), FDS_OK);
    EXPECT_EQ(compile("0b11"), FDS_OK);
    EXPECT_EQ(compile("-0b11"), FDS_OK);
    EXPECT_LT(compile("0b12"), FDS_OK);
}

TEST_F(Filter, literals_float) {
    EXPECT_EQ(compile("1.0"), FDS_OK);
    EXPECT_EQ(compile("-1.0"), FDS_OK);
    EXPECT_EQ(compile("10000.0"), FDS_OK);
    EXPECT_EQ(compile("154.145489"), FDS_OK);
    EXPECT_EQ(compile("1.2e+10"), FDS_OK);
    EXPECT_EQ(compile("1.2E+10"), FDS_OK);
    EXPECT_EQ(compile("1.2E-10"), FDS_OK);
    EXPECT_EQ(compile("1.2E10"), FDS_OK);
    EXPECT_EQ(compile("1.2e10"), FDS_OK);
    EXPECT_EQ(compile(".2e10"), FDS_OK);
    EXPECT_EQ(compile("1.e10"), FDS_OK);
}

TEST_F(Filter, literals_string) {
    EXPECT_EQ(compile("\"aaaaaaaaaaaaa\""), FDS_OK);
    EXPECT_LT(compile("\"aaaaaaaaaaaaa"), FDS_OK);
    EXPECT_LT(compile("aaaaaaaaaaaaa\""), FDS_OK);
    EXPECT_EQ(compile("\"\""), FDS_OK);
    EXPECT_EQ(compile("\"\\\"\""), FDS_OK);
}

TEST_F(Filter, literals_ipv4_address) {
    EXPECT_EQ(compile("127.0.0.1"), FDS_OK);
    EXPECT_EQ(compile("127.0.0.1/32"), FDS_OK);
    EXPECT_LT(compile("127.0.0.1/"), FDS_OK);
    EXPECT_LT(compile("127.0.0."), FDS_OK);
    EXPECT_LT(compile("127.0..1"), FDS_OK);
    EXPECT_LT(compile("127...1"), FDS_OK);
    EXPECT_LT(compile(".0.0.1"), FDS_OK);
    EXPECT_LT(compile("300.1.1.1"), FDS_OK);
    EXPECT_LT(compile("127.0.0.1.2"), FDS_OK);
    EXPECT_LT(compile("127.0.0.1/33"), FDS_OK);
    EXPECT_LT(compile("127.0.0.1/"), FDS_OK);
    EXPECT_LT(compile("127.0.0.1/32.0"), FDS_OK);
    EXPECT_LT(compile("127.0.0.1/-8"), FDS_OK);
    EXPECT_LT(compile("127.0.1/.8"), FDS_OK);
    EXPECT_LT(compile("256.6.6.6"), FDS_OK);
    EXPECT_LT(compile("254.-6.6.6"), FDS_OK);
    EXPECT_LT(compile("255.6.a.6"), FDS_OK);
    EXPECT_LT(compile("2554.6.1.6"), FDS_OK);
    EXPECT_LT(compile("0000.6.1.6"), FDS_OK);
    EXPECT_EQ(compile("255.255.255.255"), FDS_OK);
    EXPECT_EQ(compile("255.255.255.255/32"), FDS_OK);
    EXPECT_EQ(compile("255.255.255.255/1"), FDS_OK);
    EXPECT_EQ(compile("0.0.0.0"), FDS_OK);
    EXPECT_EQ(compile("0.0.0.0/32"), FDS_OK);
    EXPECT_EQ(compile("0.0.0.0/1"), FDS_OK);
}

TEST_F(Filter, literals_ipv6_address) {
    EXPECT_EQ(compile("0011:2233:4455:6677:8899:aabb:ccdd:eeff"), FDS_OK);
    EXPECT_EQ(compile("0011:2233:4455:6677:8899:aabb:ccdd:eeff/128"), FDS_OK);
    EXPECT_EQ(compile("0011:2233:4455:6677:8899:AABB:CCDD:EEFF"), FDS_OK);
    EXPECT_EQ(compile("0011:2233:4455:6677:8899:AabB:CcDd:eeFf"), FDS_OK);
    EXPECT_EQ(compile("0011:2233:4455:6677:8899:AabB:CcDd::"), FDS_OK);
    EXPECT_EQ(compile("0011:2233:4455:6677:8899:AabB:CcDd::/128"), FDS_OK);
    EXPECT_EQ(compile("::2233:4455:6677:8899:AabB:CcDd:eeff"), FDS_OK);
    EXPECT_EQ(compile("::2233:4455:6677:8899:AabB:CcDd:eeff/128"), FDS_OK);
    EXPECT_EQ(compile("2233:4455:6677::8899:AabB:CcDd:eeff"), FDS_OK);
    EXPECT_EQ(compile("aa:bb:cc:dd:ee:ff:11::"), FDS_OK);
    EXPECT_EQ(compile("aa:0:bb:eeaa:faf:a11::"), FDS_OK);
    EXPECT_LT(compile("aa:0:bb:eeaa:faf:::a11:22"), FDS_OK);
    EXPECT_LT(compile("aa:0:bb:eeaa:faf::::a11:22"), FDS_OK);
    EXPECT_EQ(compile("faf:0:bb:c:dd:eeaa::/128"), FDS_OK);
    EXPECT_EQ(compile("aa:0:baaa:a11:22::faf"), FDS_OK);
    EXPECT_EQ(compile("aa:faf:a11:22::faf/128"), FDS_OK);
    EXPECT_LT(compile("aa:bb:cc:dd:11:11222::"), FDS_OK);
    EXPECT_LT(compile("ff::ff::ff"), FDS_OK);
    EXPECT_LT(compile("ff::/200"), FDS_OK);
    EXPECT_LT(compile("ffah::"), FDS_OK);
}

TEST_F(Filter, literals_mac_address) {
    EXPECT_EQ(compile("aa:bb:cc:dd:ee:ff"), FDS_OK);
    EXPECT_EQ(compile("a2:11:cc:Dd:eE:FF"), FDS_OK);
    EXPECT_LT(compile("a2:11:cc:Dd:eE:FF:bb"), FDS_OK);
    EXPECT_LT(compile(":a2:11:cc:Dd:eE:FF"), FDS_OK);
    EXPECT_LT(compile("a2:11:cc:Dd:eE:FF:"), FDS_OK);
    EXPECT_LT(compile("a2:11:cc:Dd:eE"), FDS_OK);
    EXPECT_LT(compile("a2:11:cc:Dd:eE:"), FDS_OK);
    EXPECT_LT(compile(":a2:11:cc:Dd:eE"), FDS_OK);
    EXPECT_LT(compile("a2:11:cc:Dd:eE:gg"), FDS_OK);
    EXPECT_LT(compile("a2:-1:cc:Dd:eE:gg"), FDS_OK);
    EXPECT_LT(compile("111:44:55:66:77:88"), FDS_OK);
    EXPECT_LT(compile("1:44:55:66:77:88"), FDS_OK);
}

TEST_F(Filter, comparsions_int) {
    EXPECT_EQ(evaluate("1 == 1"), true);
    EXPECT_EQ(evaluate("-1 != 1"), true);
    EXPECT_EQ(evaluate("-1 < 1"), true);
    EXPECT_EQ(evaluate("1 > -1"), true);
    EXPECT_EQ(evaluate("1 >= 1"), true);
    EXPECT_EQ(evaluate("-100 < -50"), true);
    EXPECT_EQ(evaluate("-100 <= -50"), true);
    EXPECT_EQ(evaluate("-100 <= -100"), true);
}

TEST_F(Filter, comparsions_uint) {
    EXPECT_EQ(evaluate("1u == 1u"), true);
    EXPECT_EQ(evaluate("1u != 2u"), true);
    EXPECT_EQ(evaluate("1u < 2u"), true);
    EXPECT_EQ(evaluate("1u >= 1u"), true);
    EXPECT_EQ(evaluate("100u < 150u"), true);
    EXPECT_EQ(evaluate("100u <= 150u"), true);
    EXPECT_EQ(evaluate("100u <= 100u"), true);
}

TEST_F(Filter, comparsions_float) {
    EXPECT_EQ(evaluate("10.0 == 10.0"), true);
    EXPECT_EQ(evaluate("10.0 != 9.9"), true);
    EXPECT_EQ(evaluate("10.0 > 9.9"), true);
    EXPECT_EQ(evaluate("10.0 >= 9.9"), true);
    EXPECT_EQ(evaluate("-10.0 < 9.9"), true);
    EXPECT_EQ(evaluate("-10.0 <= 9.9"), true);
    EXPECT_EQ(evaluate("-10.0 <= -10.0"), true);
}

TEST_F(Filter, comparsions_string) {
    EXPECT_EQ(evaluate("\"hello\" == \"hello\""), true);
    EXPECT_EQ(evaluate("\"hello world\" != \"hello\""), true);
    EXPECT_EQ(evaluate("\"hello\" != \"world\""), true);
    EXPECT_EQ(evaluate("\"\" == \"\""), true);
    EXPECT_EQ(evaluate("\"hello\" != \"\""), true);
    EXPECT_LT(compile("\"hello\" > \"world\""), FDS_OK);
    EXPECT_LT(compile("\"hello\" < \"world\""), FDS_OK);
    EXPECT_LT(compile("\"hello\" <= \"world\""), FDS_OK);
    EXPECT_LT(compile("\"hello\" >= \"world\""), FDS_OK);
}

TEST_F(Filter, comparsions_ipv4_address_simple) {
    EXPECT_EQ(evaluate("192.168.1.1 == 192.168.1.1"), true);
    EXPECT_EQ(evaluate("192.168.1.1/32 == 192.168.1.1/32"), true);
    EXPECT_EQ(evaluate("192.168.1.1/32 != 192.168.1.0/32"), true);
    EXPECT_EQ(evaluate("192.168.1.1/32 != 191.168.1.1/32"), true);
    EXPECT_LT(compile("192.168.1.1 > 191.168.1.1"), FDS_OK);
    EXPECT_LT(compile("192.168.1.1 < 191.168.1.1"), FDS_OK);
    EXPECT_LT(compile("192.168.1.1 >= 191.168.1.1"), FDS_OK);
    EXPECT_LT(compile("192.168.1.1 <= 191.168.1.1"), FDS_OK);
}

TEST_F(Filter, comparsions_ipv4_address_subnet) {
    EXPECT_EQ(evaluate("192.168.1.0/24 == 192.168.1.1/32"), true);
    EXPECT_EQ(evaluate("192.168.1.0/24 == 192.168.1.255/32"), true);
    EXPECT_EQ(evaluate("192.168.1.0/24 != 192.168.2.255/32"), true);
    EXPECT_EQ(evaluate("192.168.1.0/24 == 192.168.1.255/28"), true);
    EXPECT_EQ(evaluate("192.168.1.0/24 != 192.168.2.255/28"), true);
    EXPECT_EQ(evaluate("192.168.1.0/24 == 192.168.2.255/16"), true);
}

TEST_F(Filter, comparsions_ipv6_address_simple) {
    EXPECT_EQ(evaluate("1122:3344:5566:7788:99aa:bbcc:ddee:ff00 == 1122:3344:5566:7788:99aa:bbcc:ddee:ff00"), true);
    EXPECT_EQ(evaluate("1122:3344:5566:7788:99aa:bbcc:ddee:ff00 != 1122:3344:5566:7788:99aa:bbcc:ddee:ffff"), true);
    EXPECT_EQ(evaluate("1122:3344:5566:7788:99aa:bbcc:ddee:ff00 != 0122:3344:5566:7788:99aa:bbcc:ddee:ff00"), true);
    EXPECT_EQ(evaluate("1122:3344:5566:7788:99aa:bbcc:ddee:ff00/128 == 1122:3344:5566:7788:99aa:bbcc:ddee:ff00"), true);
    EXPECT_EQ(evaluate("1122:3344:5566:7788:99aa:bbcc:ddee:ff00/128 == 1122:3344:5566:7788:99aa:bbcc:ddee:ff00/128"), true);
    EXPECT_EQ(evaluate("1122:: == 1122::"), true);
    EXPECT_EQ(evaluate("::ff == ::ff"), true);
    EXPECT_EQ(evaluate("ff:: != ::ff"), true);
    EXPECT_EQ(evaluate("ff::/128 != ::ff/128"), true);
    EXPECT_EQ(evaluate("ff::/128 == ff::/128"), true);
    EXPECT_EQ(evaluate("ff::f != ff::"), true);
}

TEST_F(Filter, comparsions_ipv6_address_subnet) {
    EXPECT_EQ(evaluate("1122:3344:5566:7788:0000:0000:0000:0000/64 == 1122:3344:5566:7788:99aa:bbcc:ddee:ff00/128"), true);
    EXPECT_EQ(evaluate("1122:3344:5566:7788::/64 == 1122:3344:5566:7788:99aa:bbcc:ddee:ff00/128"), true);
    EXPECT_EQ(evaluate("1122:3344:5566:7788::/64 == 1122:3344:5566:7788:99aa::/128"), true);
    EXPECT_EQ(evaluate("1122:3344:5566:7788::/64 == 1122:3344:5566:7788:99aa::/96"), true);
    EXPECT_EQ(evaluate("1122:3344:5566:7788::/64 == 1122:3344:5566:7788::/64"), true);
    EXPECT_EQ(evaluate("1122:3344:5566:7788::/64 == 1122:3344::/32"), true);
    EXPECT_EQ(evaluate("1122:3344:5566:7788::/64 != 0122:3344::/32"), true);
    EXPECT_EQ(evaluate("1122:3344:5566:7788::/64 != ff::/128"), true);
    EXPECT_EQ(evaluate("1122:3344:5566:7788::/64 != ff::/64"), true);
    EXPECT_EQ(evaluate("1122:3344:5566:7788::/64 != ff::/16"), true);
    EXPECT_EQ(evaluate("1122:3344:5566:7788::/64 == 1122::/16"), true);
}

TEST_F(Filter, comparsions_ipv4_with_ipv6_address) {
    EXPECT_EQ(evaluate("192.168.1.0 != ff::"), true);
    EXPECT_EQ(evaluate("255.255.255.0/24 != ffff:ffff:ffff:ffff::/24"), true);
}

TEST_F(Filter, comparsions_mac_address) {
    EXPECT_EQ(evaluate("00:11:22:33:44:55 == 00:11:22:33:44:55"), true);
    EXPECT_EQ(evaluate("00:11:22:33:44:55 != 00:11:22:33:44:66"), true);
}

TEST_F(Filter, number_suffixes) {
    EXPECT_EQ(evaluate("1ns == 1"), true);
    EXPECT_EQ(evaluate("1us == 1000ns"), true);
    EXPECT_EQ(evaluate("1ms == 1000us"), true);
    EXPECT_EQ(evaluate("1s == 1000ms"), true);
    EXPECT_EQ(evaluate("1m == 60s"), true);
    EXPECT_EQ(evaluate("1m == 60000ms"), true);
    EXPECT_EQ(evaluate("1h == 60m"), true);
    EXPECT_EQ(evaluate("1h == 3600s"), true);
    EXPECT_EQ(evaluate("1d == 24h"), true);

    EXPECT_EQ(evaluate("1B == 1"), true);
    EXPECT_EQ(evaluate("1k == 1024B"), true);
    EXPECT_EQ(evaluate("1M == 1024k"), true);
    EXPECT_EQ(evaluate("1G == 1024M"), true);
    EXPECT_EQ(evaluate("1T == 1024G"), true);

    EXPECT_EQ(evaluate("1.0ns == 1"), true);
    EXPECT_EQ(evaluate("1.0us == 1000ns"), true);
    EXPECT_EQ(evaluate("1.0ms == 1000us"), true);
    EXPECT_EQ(evaluate("1.0s == 1000ms"), true);
    EXPECT_EQ(evaluate("1.0m == 60s"), true);
    EXPECT_EQ(evaluate("1.0m == 60000ms"), true);
    EXPECT_EQ(evaluate("1.0h == 60m"), true);
    EXPECT_EQ(evaluate("1.0h == 3600s"), true);
    EXPECT_EQ(evaluate("1.0d == 24h"), true);

    EXPECT_EQ(evaluate("1.0k == 1024"), true);
    EXPECT_EQ(evaluate("1.0M == 1024k"), true);
    EXPECT_EQ(evaluate("1.0G == 1024M"), true);
    EXPECT_EQ(evaluate("1.0T == 1024G"), true);
}

TEST_F(Filter, number_bases) {
    EXPECT_EQ(evaluate("0xFF == 255"), true);
    EXPECT_EQ(evaluate("0xFf == 255"), true);
    EXPECT_EQ(evaluate("0xfF == 255"), true);
    EXPECT_EQ(evaluate("0x0fF == 255"), true);
    EXPECT_EQ(evaluate("0b01111111 == 127"), true);
    EXPECT_EQ(evaluate("0b11111111 == 0xFF"), true);
}

TEST_F(Filter, float_extra) {
    EXPECT_EQ(evaluate(".2 == 0.2"), true);
    EXPECT_EQ(evaluate("2. == 2.0"), true);
    EXPECT_LT(compile(". == 0.0"), FDS_OK);
    EXPECT_EQ(evaluate("0. == 0.0"), true);
    EXPECT_EQ(evaluate(".0 == 0.0"), true);
    EXPECT_LT(compile(".e == 0.0"), FDS_OK);
    EXPECT_LT(compile("0.e == 0.0"), FDS_OK);
    EXPECT_EQ(evaluate("1.2e1 == 12.0"), true);
    EXPECT_EQ(evaluate("1.2e2 == 120.0"), true);
    EXPECT_EQ(evaluate("1.2e3 == 1200.0"), true);
    EXPECT_EQ(evaluate("1.2e+3 == 1200.0"), true);
    EXPECT_EQ(evaluate("120.0e-2 == 1.2"), true);
    EXPECT_EQ(evaluate("120.0e-3 == 0.12"), true);
}

TEST_F(Filter, arithmetic) {
    EXPECT_EQ(evaluate("1 + 1 == 2"), true);
    EXPECT_EQ(evaluate("1 - 1 == 0"), true);
    EXPECT_EQ(evaluate("1 - 10 == -9"), true);
    EXPECT_EQ(evaluate("-1 + 1 == 0"), true);
    EXPECT_EQ(evaluate("-1 + 1 == 20 * 0"), true);
    EXPECT_EQ(evaluate("2 * 2 + 2 * 4 == (3 + 3) * 2"), true);
    EXPECT_EQ(evaluate("6 / 3 == 2"), true);
    EXPECT_EQ(evaluate("6 / 3 * 3 == 6"), true);
    EXPECT_EQ(evaluate("11 / 2 == 5"), true);
    EXPECT_EQ(evaluate("11.0 / 2 == 5.5"), true);
    EXPECT_EQ(evaluate("1.0 + 1.0 == 2.0"), true);
    EXPECT_EQ(evaluate("3.0 + 2.0 < 3.0 * 2.0"), true);
    EXPECT_EQ(evaluate("3.0 + 2 < 3.0 * 2"), true);
    EXPECT_EQ(evaluate("-1 + 1 == -1.0 + 1.0"), true);
    EXPECT_EQ(evaluate("-1 - 1 == -1.0 - 1.0"), true);
    EXPECT_EQ(evaluate("-1 * 1 == -1.0 * 1.0"), true);
    EXPECT_EQ(evaluate("-1 / 1 == -1.0 / 1.0"), true);
    EXPECT_EQ(evaluate("-1 + 1.0 == -1 + 1.0"), true);
    EXPECT_EQ(evaluate("3.33 * 3 < 10"), true);
    EXPECT_EQ(evaluate("5 % 2 == 1"), true);
    EXPECT_EQ(evaluate("5.0 % 2 == 1"), true);
}

TEST_F(Filter, lists_numbers) {
    EXPECT_EQ(evaluate("1 in [1, 2, 3, 4]"), true);
    EXPECT_EQ(evaluate("2 in [1, 2, 3, 4]"), true);
    EXPECT_EQ(evaluate("3 in [1, 2, 3, 4]"), true);
    EXPECT_EQ(evaluate("4 in [1, 2, 3, 4]"), true);
    EXPECT_EQ(evaluate("5 in [1, 2, 3, 4]"), false);
    EXPECT_EQ(evaluate("1 in []"), false);

    EXPECT_EQ(evaluate("1.0 in [1, 2, 3, 4]"), true);
    EXPECT_EQ(evaluate("1.0 in [1, 2.0, 3, 4]"), true);
    EXPECT_EQ(evaluate("1 in [1, 2.0, 3, 4]"), true);

    EXPECT_LT(compile("1 in 1, 2, 3, 4]"), FDS_OK);
    EXPECT_LT(compile("1 in [1, 2, 3, 4"), FDS_OK);
    EXPECT_LT(compile("1 in [1, 2 3, 4]"), FDS_OK);
    EXPECT_LT(compile("1 in [1, 2, 3 4]"), FDS_OK);
    //EXPECT_LT(compile("1 in [1, 2, 3, 4,]"), FDS_OK);
    EXPECT_LT(compile("1 in [,1, 2, 3, 4]"), FDS_OK);
    EXPECT_LT(compile("1 in [1, 2. 3, 4]"), FDS_OK);

    EXPECT_EQ(evaluate("1u in [1, 2, 3, 4u]"), true);
    EXPECT_EQ(evaluate("1u in [1, 2, 3, 4]"), true);
    EXPECT_EQ(evaluate("1 in [1u, 2, 3, 4]"), true);
}

TEST_F(Filter, lists_strings) {
    EXPECT_EQ(evaluate("\"hello\" in [\"hello\", \"world\"]"), true);
    EXPECT_EQ(evaluate("not \"hello\" in [\"hello \", \"world\"]"), true);
    EXPECT_EQ(evaluate("not \"hello\" in [\" hello\", \"world\"]"), true);
    EXPECT_EQ(evaluate("\"world\" in [\"hello\", \"world\"]"), true);
    EXPECT_EQ(evaluate("\"world\" in [\"hello\", \"world\", \"!\"]"), true);
    EXPECT_EQ(evaluate("\"world\" in [\"world\"]"), true);
    EXPECT_EQ(evaluate("not \"world\" in []"), true);
}

TEST_F(Filter, lists_ip_addresses) {
    EXPECT_EQ(evaluate("192.168.1.1 in [192.168.1.0/24, 127.0.0.1/8, 10.0.0.0/8, 1.1.1.1, 8.8.8.8]"), true);
    EXPECT_EQ(evaluate("not 192.168.0.1 in [192.168.1.0/24, 127.0.0.1/8, 10.0.0.0/8, 1.1.1.1, 8.8.8.8]"), true);
    EXPECT_EQ(evaluate("10.123.4.5 in [192.168.1.0/24, 127.0.0.1/8, 10.0.0.0/8, 1.1.1.1, 8.8.8.8]"), true);
    EXPECT_EQ(evaluate("not 11.2.2.2 in [192.168.1.0/24, 127.0.0.1/8, 10.0.0.0/8, 1.1.1.1, 8.8.8.8]"), true);
    EXPECT_EQ(evaluate("1.1.1.1 in [192.168.1.0/24, 127.0.0.1/8, 10.0.0.0/8, 1.1.1.1, 8.8.8.8]"), true);
    EXPECT_EQ(evaluate("8.8.8.8 in [192.168.1.0/24, 127.0.0.1/8, 10.0.0.0/8, 1.1.1.1, 8.8.8.8]"), true);
    EXPECT_EQ(evaluate("not 1.1.1.2 in [192.168.1.0/24, 127.0.0.1/8, 10.0.0.0/8, 1.1.1.1, 8.8.8.8]"), true);
    EXPECT_EQ(evaluate("not 8.8.8.16 in [192.168.1.0/24, 127.0.0.1/8, 10.0.0.0/8, 1.1.1.1, 8.8.8.8]"), true);
    EXPECT_EQ(evaluate("not ff:: in [192.168.1.0/24, 127.0.0.1/8, 10.0.0.0/8, 1.1.1.1, 8.8.8.8]"), true);
    EXPECT_EQ(evaluate("192.168.1.0/28 in [192.168.1.0/24, 127.0.0.1/8, 10.0.0.0/8, 1.1.1.1, 8.8.8.8]"), true);
}

TEST_F(Filter, lists_mac_addresses) {
    EXPECT_EQ(evaluate("11:22:33:44:55:66 in [11:22:33:44:55:66, 11:22:33:44:55:77]"), true);
    EXPECT_EQ(evaluate("not 11:22:33:44:55:88 in [11:22:33:44:55:66, 11:22:33:44:55:77]"), true);
    EXPECT_EQ(evaluate("11:22:33:44:55:66 in [11:22:33:44:55:77, 11:22:33:44:55:66]"), true);
}

TEST_F(Filter, string_operations) {
    EXPECT_EQ(evaluate("\"hello world!\" contains \"hello\""), true);
    EXPECT_EQ(evaluate("\"hello world!\" contains \"world\""), true);
    EXPECT_EQ(evaluate("\"hello world!\" contains \" \""), true);
    EXPECT_EQ(evaluate("\"hello world!\" contains \"\""), true);
    EXPECT_EQ(evaluate("not \"\" contains \"hello\""), true);
    EXPECT_EQ(evaluate("not \"hello world!\" contains \"foo\""), true);
}

TEST_F(Filter, bitwise_operations) {
    EXPECT_EQ(evaluate("0b11110000 | 0b01011111 == 0b11111111"), true);
    EXPECT_EQ(evaluate("0b11110000 ^ 0b01011111 == 0b10101111"), true);
    EXPECT_EQ(evaluate("0b11110000 & 0b01011111 == 0b01010000"), true);
    EXPECT_EQ(evaluate("~0b11110000 == 0b1111111111111111111111111111111111111111111111111111111100001111"), true);
}

TEST_F(Filter, bool_operations) {
    EXPECT_EQ(evaluate("1 and 1"), true);
    EXPECT_EQ(evaluate("not (1 and 0)"), true);
    EXPECT_EQ(evaluate("not (0 and 1)"), true);
    EXPECT_EQ(evaluate("0 or 1"), true);
    EXPECT_EQ(evaluate("1 or 0"), true);
    EXPECT_EQ(evaluate("not (0 or 0)"), true);
    EXPECT_EQ(evaluate("not 0 or 0"), true);
    EXPECT_EQ(evaluate("not 0"), true);
    EXPECT_EQ(evaluate("0 or ((1 or 0) and 1)"), true);
    EXPECT_EQ(evaluate("(not (0 and 1)) or ((1 or 0) and 1)"), true);
}

TEST_F(Filter, vars) {
    int n;
    user_ctx = &n;
    fds_filter_opts_set_lookup_cb(opts,
    [](void *user_ctx, const char *name, const char *other_name, int *out_id, int *out_datatype, int *out_flags) -> int
    {
        if (strcmp(name, "ip") == 0) {
            *out_id = 1;
            *out_datatype = FDS_FDT_IP;
            return FDS_OK;
        }
        if (strcmp(name, "port") == 0) {
            *out_id = 2;
            *out_datatype = FDS_FDT_UINT;
            return FDS_OK;
        }
        if (strcmp(name, "bytes") == 0) {
            *out_id = 3;
            *out_datatype = FDS_FDT_UINT;
            return FDS_OK;
        }
        if (strcmp(name, "url") == 0) {
            *out_id = 4;
            *out_datatype = FDS_FDT_STR;
            return FDS_OK;
        }
        assert(false);
    });
    fds_filter_opts_set_const_cb(opts,
    [](void *user_ctx, int id, fds_filter_value_u *out_value) -> void
    {

    });
    fds_filter_opts_set_data_cb(opts,
    [](void *user_ctx, bool reset_ctx, int id, void *data, fds_filter_value_u *out_value) -> int
    {
        int *n = static_cast<int *>(user_ctx);
        if (reset_ctx) {
            *n = 0;
        }

        if (id == 1) { // ip
            switch (*n) {
            case 0:
                out_value->ip.prefix = 32;
                out_value->ip.version = 4;
                out_value->ip.addr[0] = 127;
                out_value->ip.addr[1] = 0;
                out_value->ip.addr[2] = 0;
                out_value->ip.addr[3] = 1;
                (*n)++;
                return FDS_OK_MORE;
            case 1:
                out_value->ip.prefix = 32;
                out_value->ip.version = 4;
                out_value->ip.addr[0] = 10;
                out_value->ip.addr[1] = 0;
                out_value->ip.addr[2] = 0;
                out_value->ip.addr[3] = 1;
                (*n)++;
                return FDS_OK_MORE;
            case 2:
                return FDS_ERR_NOTFOUND;
            }
        } else if (id == 2) { // port
            switch (*n) {
            case 0:
                out_value->u = 80;
                (*n)++;
                return FDS_OK_MORE;
            case 1:
                out_value->u = 443;
                (*n)++;
                return FDS_OK_MORE;
            case 2:
                return FDS_ERR_NOTFOUND;
            }
        } else if (id == 3) { // bytes
            switch (*n) {
            case 0:
                (*n)++;
                out_value->u = 1024;
                return FDS_OK_MORE;
            case 1:
                (*n)++;
                out_value->u = 2048;
                return FDS_OK;
            default:
                return FDS_ERR_NOTFOUND;
            }
        } else if (id == 4) { // url
            out_value->str.chars = NULL;
            out_value->str.len = 0;
            return FDS_ERR_NOTFOUND;
        } else {
            assert(false);
        }
    });
    EXPECT_EQ(evaluate("ip 127.0.0.1"), true);
    EXPECT_EQ(evaluate("not ip 127.0.0.0"), true);
    EXPECT_EQ(evaluate("ip 10.0.0.1"), true);
    EXPECT_EQ(evaluate("not ip 10.0.0.2"), true);
    EXPECT_EQ(evaluate("port 80"), true);
    EXPECT_EQ(evaluate("port 443"), true);
    EXPECT_EQ(evaluate("not port 22"), true);
    EXPECT_EQ(evaluate("not port 1234"), true);
    EXPECT_EQ(evaluate("ip 127.0.0.1 and port 80"), true);
    EXPECT_EQ(evaluate("ip 127.0.0.1 and port 443"), true);
    EXPECT_EQ(evaluate("ip 10.0.0.1 and port 80"), true);
    EXPECT_EQ(evaluate("ip 10.0.0.1 and port 443"), true);
    EXPECT_EQ(evaluate("not url \"google.com\""), true);
    EXPECT_EQ(evaluate("not exists url"), true);
    EXPECT_EQ(evaluate("exists port"), true);
    EXPECT_EQ(evaluate("exists ip"), true);
    EXPECT_EQ(evaluate("url \"\""), true);
    EXPECT_EQ(evaluate("exists url or url \"\""), true);
    EXPECT_EQ(evaluate("bytes > 1024"), true);
    EXPECT_EQ(evaluate("bytes < 2048"), true);
    EXPECT_EQ(evaluate("bytes + 1 == 1025"), true);
    EXPECT_EQ(evaluate("bytes + 1 == 2049"), true);
    EXPECT_EQ(evaluate("bytes != 1024"), true);
    EXPECT_EQ(evaluate("bytes * 2 == 2048"), true);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
