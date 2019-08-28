#include <libfds.h>
#include <gtest/gtest.h>
#include "filter_wrapper.h"

TEST_F(Filter, literals_int) {
    EXPECT_EQ(compile("1"), FDS_FILTER_OK);
    EXPECT_EQ(compile("-1"), FDS_FILTER_OK);
    EXPECT_EQ(compile("10000"), FDS_FILTER_OK);
    EXPECT_EQ(compile("465464894616548498"), FDS_FILTER_OK);
    EXPECT_EQ(compile("465464894a616548498"), FDS_FILTER_FAIL);
}

TEST_F(Filter, literals_int_bases) {
    EXPECT_EQ(compile("0x123"), FDS_FILTER_OK);
    EXPECT_EQ(compile("0xF123AF"), FDS_FILTER_OK);
    EXPECT_EQ(compile("-0xF123AF"), FDS_FILTER_OK);
    EXPECT_EQ(compile("0xF123AG"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("0xGF123AG"), FDS_FILTER_FAIL);

    EXPECT_EQ(compile("0b000"), FDS_FILTER_OK);
    EXPECT_EQ(compile("0b11"), FDS_FILTER_OK);
    EXPECT_EQ(compile("-0b11"), FDS_FILTER_OK);
    EXPECT_EQ(compile("0b12"), FDS_FILTER_FAIL);
}

TEST_F(Filter, literals_float) {
    EXPECT_EQ(compile("1.0"), FDS_FILTER_OK);
    EXPECT_EQ(compile("-1.0"), FDS_FILTER_OK);
    EXPECT_EQ(compile("10000.0"), FDS_FILTER_OK);
    EXPECT_EQ(compile("154.145489"), FDS_FILTER_OK);
    EXPECT_EQ(compile("1.2e+10"), FDS_FILTER_OK);
    EXPECT_EQ(compile("1.2E+10"), FDS_FILTER_OK);
    EXPECT_EQ(compile("1.2E-10"), FDS_FILTER_OK);
    EXPECT_EQ(compile("1.2E10"), FDS_FILTER_OK);
    EXPECT_EQ(compile("1.2e10"), FDS_FILTER_OK);
    EXPECT_EQ(compile(".2e10"), FDS_FILTER_OK);
    EXPECT_EQ(compile("1.e10"), FDS_FILTER_OK);
}

TEST_F(Filter, literals_string) {
    EXPECT_EQ(compile("\"aaaaaaaaaaaaa\""), FDS_FILTER_OK);
    EXPECT_EQ(compile("\"aaaaaaaaaaaaa"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("aaaaaaaaaaaaa\""), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("\"\""), FDS_FILTER_OK);
    EXPECT_EQ(compile("\"\\\"\""), FDS_FILTER_OK);
}

TEST_F(Filter, literals_ipv4_address) {
    EXPECT_EQ(compile("127.0.0.1"), FDS_FILTER_OK);
    EXPECT_EQ(compile("127.0.0.1/32"), FDS_FILTER_OK);
    EXPECT_EQ(compile("127.0.0.1/"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("127.0.0."), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("127.0..1"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("127...1"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile(".0.0.1"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("300.1.1.1"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("127.0.0.1.2"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("127.0.0.1/33"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("127.0.0.1/"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("127.0.0.1/32.0"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("127.0.0.1/-8"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("127.0.1/.8"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("256.6.6.6"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("254.-6.6.6"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("255.6.a.6"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("2554.6.1.6"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("0000.6.1.6"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("255.255.255.255"), FDS_FILTER_OK);
    EXPECT_EQ(compile("255.255.255.255/32"), FDS_FILTER_OK);
    EXPECT_EQ(compile("255.255.255.255/1"), FDS_FILTER_OK);
    EXPECT_EQ(compile("0.0.0.0"), FDS_FILTER_OK);
    EXPECT_EQ(compile("0.0.0.0/32"), FDS_FILTER_OK);
    EXPECT_EQ(compile("0.0.0.0/1"), FDS_FILTER_OK);
}

TEST_F(Filter, literals_ipv6_address) {
    EXPECT_EQ(compile("0011:2233:4455:6677:8899:aabb:ccdd:eeff"), FDS_FILTER_OK);
    EXPECT_EQ(compile("0011:2233:4455:6677:8899:aabb:ccdd:eeff/128"), FDS_FILTER_OK);
    EXPECT_EQ(compile("0011:2233:4455:6677:8899:AABB:CCDD:EEFF"), FDS_FILTER_OK);
    EXPECT_EQ(compile("0011:2233:4455:6677:8899:AabB:CcDd:eeFf"), FDS_FILTER_OK);
    EXPECT_EQ(compile("0011:2233:4455:6677:8899:AabB:CcDd::"), FDS_FILTER_OK);
    EXPECT_EQ(compile("0011:2233:4455:6677:8899:AabB:CcDd::/128"), FDS_FILTER_OK);
    EXPECT_EQ(compile("::2233:4455:6677:8899:AabB:CcDd:eeff"), FDS_FILTER_OK);
    EXPECT_EQ(compile("::2233:4455:6677:8899:AabB:CcDd:eeff/128"), FDS_FILTER_OK);
    EXPECT_EQ(compile("2233:4455:6677::8899:AabB:CcDd:eeff"), FDS_FILTER_OK);
    EXPECT_EQ(compile("aa:bb:cc:dd:ee:ff:11::"), FDS_FILTER_OK);
    EXPECT_EQ(compile("aa:0:bb:eeaa:faf:a11::"), FDS_FILTER_OK);
    EXPECT_EQ(compile("aa:0:bb:eeaa:faf:::a11:22"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("aa:0:bb:eeaa:faf::::a11:22"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("faf:0:bb:c:dd:eeaa::/128"), FDS_FILTER_OK);
    EXPECT_EQ(compile("aa:0:baaa:a11:22::faf"), FDS_FILTER_OK);
    EXPECT_EQ(compile("aa:faf:a11:22::faf/128"), FDS_FILTER_OK);
    EXPECT_EQ(compile("aa:bb:cc:dd:11:11222::"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("ff::ff::ff"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("ff::/200"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("ffah::"), FDS_FILTER_FAIL);
}

TEST_F(Filter, literals_mac_address) {
    EXPECT_EQ(compile("aa:bb:cc:dd:ee:ff"), FDS_FILTER_OK);
    EXPECT_EQ(compile("a2:11:cc:Dd:eE:FF"), FDS_FILTER_OK);
    EXPECT_EQ(compile("a2:11:cc:Dd:eE:FF:bb"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile(":a2:11:cc:Dd:eE:FF"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("a2:11:cc:Dd:eE:FF:"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("a2:11:cc:Dd:eE"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("a2:11:cc:Dd:eE:"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile(":a2:11:cc:Dd:eE"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("a2:11:cc:Dd:eE:gg"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("a2:-1:cc:Dd:eE:gg"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("111:44:55:66:77:88"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("1:44:55:66:77:88"), FDS_FILTER_FAIL);
}

TEST_F(Filter, comparsions_int) {
    EXPECT_EQ(evaluate("1 == 1"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("-1 != 1"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("-1 < 1"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1 > -1"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1 >= 1"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("-100 < -50"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("-100 <= -50"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("-100 <= -100"), FDS_FILTER_YES);
}

TEST_F(Filter, comparsions_uint) {
    EXPECT_EQ(evaluate("1u == 1u"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1u != 2u"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1u < 2u"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1u >= 1u"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("100u < 150u"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("100u <= 150u"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("100u <= 100u"), FDS_FILTER_YES);
}

TEST_F(Filter, comparsions_float) {
    EXPECT_EQ(evaluate("10.0 == 10.0"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("10.0 != 9.9"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("10.0 > 9.9"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("10.0 >= 9.9"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("-10.0 < 9.9"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("-10.0 <= 9.9"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("-10.0 <= -10.0"), FDS_FILTER_YES);
}

TEST_F(Filter, comparsions_string) {
    EXPECT_EQ(evaluate("\"hello\" == \"hello\""), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("\"hello world\" != \"hello\""), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("\"hello\" != \"world\""), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("\"\" == \"\""), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("\"hello\" != \"\""), FDS_FILTER_YES);
    EXPECT_EQ(compile("\"hello\" > \"world\""), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("\"hello\" < \"world\""), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("\"hello\" <= \"world\""), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("\"hello\" >= \"world\""), FDS_FILTER_FAIL);
}

TEST_F(Filter, comparsions_ipv4_address_simple) {
    EXPECT_EQ(evaluate("192.168.1.1 == 192.168.1.1"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("192.168.1.1/32 == 192.168.1.1/32"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("192.168.1.1/32 != 192.168.1.0/32"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("192.168.1.1/32 != 191.168.1.1/32"), FDS_FILTER_YES);
    EXPECT_EQ(compile("192.168.1.1 > 191.168.1.1"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("192.168.1.1 < 191.168.1.1"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("192.168.1.1 >= 191.168.1.1"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("192.168.1.1 <= 191.168.1.1"), FDS_FILTER_FAIL);
}

TEST_F(Filter, comparsions_ipv4_address_subnet) {
    EXPECT_EQ(evaluate("192.168.1.0/24 == 192.168.1.1/32"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("192.168.1.0/24 == 192.168.1.255/32"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("192.168.1.0/24 != 192.168.2.255/32"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("192.168.1.0/24 == 192.168.1.255/28"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("192.168.1.0/24 != 192.168.2.255/28"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("192.168.1.0/24 == 192.168.2.255/16"), FDS_FILTER_YES);
}

TEST_F(Filter, comparsions_ipv6_address_simple) {
    EXPECT_EQ(evaluate("1122:3344:5566:7788:99aa:bbcc:ddee:ff00 == 1122:3344:5566:7788:99aa:bbcc:ddee:ff00"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1122:3344:5566:7788:99aa:bbcc:ddee:ff00 != 1122:3344:5566:7788:99aa:bbcc:ddee:ffff"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1122:3344:5566:7788:99aa:bbcc:ddee:ff00 != 0122:3344:5566:7788:99aa:bbcc:ddee:ff00"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1122:3344:5566:7788:99aa:bbcc:ddee:ff00/128 == 1122:3344:5566:7788:99aa:bbcc:ddee:ff00"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1122:3344:5566:7788:99aa:bbcc:ddee:ff00/128 == 1122:3344:5566:7788:99aa:bbcc:ddee:ff00/128"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1122:: == 1122::"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("::ff == ::ff"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("ff:: != ::ff"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("ff::/128 != ::ff/128"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("ff::/128 == ff::/128"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("ff::f != ff::"), FDS_FILTER_YES);
}

TEST_F(Filter, comparsions_ipv6_address_subnet) {
    EXPECT_EQ(evaluate("1122:3344:5566:7788:0000:0000:0000:0000/64 == 1122:3344:5566:7788:99aa:bbcc:ddee:ff00/128"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1122:3344:5566:7788::/64 == 1122:3344:5566:7788:99aa:bbcc:ddee:ff00/128"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1122:3344:5566:7788::/64 == 1122:3344:5566:7788:99aa::/128"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1122:3344:5566:7788::/64 == 1122:3344:5566:7788:99aa::/96"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1122:3344:5566:7788::/64 == 1122:3344:5566:7788::/64"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1122:3344:5566:7788::/64 == 1122:3344::/32"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1122:3344:5566:7788::/64 != 0122:3344::/32"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1122:3344:5566:7788::/64 != ff::/128"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1122:3344:5566:7788::/64 != ff::/64"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1122:3344:5566:7788::/64 != ff::/16"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1122:3344:5566:7788::/64 == 1122::/16"), FDS_FILTER_YES);
}

TEST_F(Filter, comparsions_ipv4_with_ipv6_address) {
    EXPECT_EQ(evaluate("192.168.1.0 != ff::"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("255.255.255.0/24 != ffff:ffff:ffff:ffff::/24"), FDS_FILTER_YES);
}

TEST_F(Filter, comparsions_mac_address) {
    EXPECT_EQ(evaluate("00:11:22:33:44:55 == 00:11:22:33:44:55"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("00:11:22:33:44:55 != 00:11:22:33:44:66"), FDS_FILTER_YES);
}

TEST_F(Filter, number_suffixes) {
    EXPECT_EQ(evaluate("1ns == 1"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1us == 1000ns"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1ms == 1000us"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1s == 1000ms"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1m == 60s"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1m == 60000ms"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1h == 60m"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1h == 3600s"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1d == 24h"), FDS_FILTER_YES);

    EXPECT_EQ(evaluate("1B == 1"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1k == 1024B"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1M == 1024k"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1G == 1024M"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1T == 1024G"), FDS_FILTER_YES);

    EXPECT_EQ(evaluate("1.0ns == 1"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1.0us == 1000ns"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1.0ms == 1000us"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1.0s == 1000ms"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1.0m == 60s"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1.0m == 60000ms"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1.0h == 60m"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1.0h == 3600s"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1.0d == 24h"), FDS_FILTER_YES);

    EXPECT_EQ(evaluate("1.0k == 1024"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1.0M == 1024k"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1.0G == 1024M"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1.0T == 1024G"), FDS_FILTER_YES);
}

TEST_F(Filter, number_bases) {
    EXPECT_EQ(evaluate("0xFF == 255"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("0xFf == 255"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("0xfF == 255"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("0x0fF == 255"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("0b01111111 == 127"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("0b11111111 == 0xFF"), FDS_FILTER_YES);
}

TEST_F(Filter, float_extra) {
    EXPECT_EQ(evaluate(".2 == 0.2"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("2. == 2.0"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate(". == 0.0"), FDS_FILTER_FAIL);
    EXPECT_EQ(evaluate("0. == 0.0"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate(".0 == 0.0"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate(".e == 0.0"), FDS_FILTER_FAIL);
    EXPECT_EQ(evaluate("0.e == 0.0"), FDS_FILTER_FAIL);
    EXPECT_EQ(evaluate("1.2e1 == 12.0"), FDS_FILTER_OK);
    EXPECT_EQ(evaluate("1.2e2 == 120.0"), FDS_FILTER_OK);
    EXPECT_EQ(evaluate("1.2e3 == 1200.0"), FDS_FILTER_OK);
    EXPECT_EQ(evaluate("1.2e+3 == 1200.0"), FDS_FILTER_OK);
    EXPECT_EQ(evaluate("120.0e-2 == 1.2"), FDS_FILTER_OK);
    EXPECT_EQ(evaluate("120.0e-3 == 0.12"), FDS_FILTER_OK);
}

TEST_F(Filter, arithmetic) {
    EXPECT_EQ(evaluate("1 + 1 == 2"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1 - 1 == 0"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1 - 10 == -9"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("-1 + 1 == 0"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("-1 + 1 == 20 * 0"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("2 * 2 + 2 * 4 == (3 + 3) * 2"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("6 / 3 == 2"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("6 / 3 * 3 == 6"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("11 / 2 == 5"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("11.0 / 2 == 5.5"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1.0 + 1.0 == 2.0"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("3.0 + 2.0 < 3.0 * 2.0"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("3.0 + 2 < 3.0 * 2"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("-1 + 1 == -1.0 + 1.0"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("-1 - 1 == -1.0 - 1.0"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("-1 * 1 == -1.0 * 1.0"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("-1 / 1 == -1.0 / 1.0"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("-1 + 1.0 == -1 + 1.0"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("3.33 * 3 < 10"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("5 % 2 == 1"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("5.0 % 2 == 1"), FDS_FILTER_YES);
}

TEST_F(Filter, lists_numbers) {
    EXPECT_EQ(evaluate("1 inside [1, 2, 3, 4]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("2 inside [1, 2, 3, 4]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("3 inside [1, 2, 3, 4]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("4 inside [1, 2, 3, 4]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("5 inside [1, 2, 3, 4]"), FDS_FILTER_NO);
    EXPECT_EQ(evaluate("1 inside []"), FDS_FILTER_NO);

    EXPECT_EQ(evaluate("1.0 inside [1, 2, 3, 4]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1.0 inside [1, 2.0, 3, 4]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1 inside [1, 2.0, 3, 4]"), FDS_FILTER_YES);

    EXPECT_EQ(compile("1 inside 1, 2, 3, 4]"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("1 inside [1, 2, 3, 4"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("1 inside [1, 2 3, 4]"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("1 inside [1, 2, 3 4]"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("1 inside [1, 2, 3, 4,]"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("1 inside [,1, 2, 3, 4]"), FDS_FILTER_FAIL);
    EXPECT_EQ(compile("1 inside [1, 2. 3, 4]"), FDS_FILTER_FAIL);

    EXPECT_EQ(evaluate("1u inside [1, 2, 3, 4u]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1u inside [1, 2, 3, 4]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1 inside [1u, 2, 3, 4]"), FDS_FILTER_YES);
}

TEST_F(Filter, lists_strings) {
    EXPECT_EQ(evaluate("\"hello\" inside [\"hello\", \"world\"]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("not \"hello\" inside [\"hello \", \"world\"]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("not \"hello\" inside [\" hello\", \"world\"]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("\"world\" inside [\"hello\", \"world\"]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("\"world\" inside [\"hello\", \"world\", \"!\"]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("\"world\" inside [\"world\"]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("not \"world\" inside []"), FDS_FILTER_YES);
}

TEST_F(Filter, lists_ip_addresses) {
    EXPECT_EQ(evaluate("192.168.1.1 inside [192.168.1.0/24, 127.0.0.1/8, 10.0.0.0/8, 1.1.1.1, 8.8.8.8]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("not 192.168.0.1 inside [192.168.1.0/24, 127.0.0.1/8, 10.0.0.0/8, 1.1.1.1, 8.8.8.8]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("10.123.4.5 inside [192.168.1.0/24, 127.0.0.1/8, 10.0.0.0/8, 1.1.1.1, 8.8.8.8]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("not 11.2.2.2 inside [192.168.1.0/24, 127.0.0.1/8, 10.0.0.0/8, 1.1.1.1, 8.8.8.8]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1.1.1.1 inside [192.168.1.0/24, 127.0.0.1/8, 10.0.0.0/8, 1.1.1.1, 8.8.8.8]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("8.8.8.8 inside [192.168.1.0/24, 127.0.0.1/8, 10.0.0.0/8, 1.1.1.1, 8.8.8.8]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("not 1.1.1.2 inside [192.168.1.0/24, 127.0.0.1/8, 10.0.0.0/8, 1.1.1.1, 8.8.8.8]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("not 8.8.8.16 inside [192.168.1.0/24, 127.0.0.1/8, 10.0.0.0/8, 1.1.1.1, 8.8.8.8]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("not ff:: inside [192.168.1.0/24, 127.0.0.1/8, 10.0.0.0/8, 1.1.1.1, 8.8.8.8]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("192.168.1.0/28 inside [192.168.1.0/24, 127.0.0.1/8, 10.0.0.0/8, 1.1.1.1, 8.8.8.8]"), FDS_FILTER_YES);

    // ? should this be correct?
    EXPECT_EQ(evaluate("192.168.1.0/16 inside [192.168.1.0/24, 127.0.0.1/8, 10.0.0.0/8, 1.1.1.1, 8.8.8.8]"), FDS_FILTER_YES);
}

TEST_F(Filter, lists_mac_addresses) {
    EXPECT_EQ(evaluate("11:22:33:44:55:66 inside [11:22:33:44:55:66, 11:22:33:44:55:77]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("not 11:22:33:44:55:88 inside [11:22:33:44:55:66, 11:22:33:44:55:77]"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("11:22:33:44:55:66 inside [11:22:33:44:55:77, 11:22:33:44:55:66]"), FDS_FILTER_YES);
}

TEST_F(Filter, string_operations) {
    EXPECT_EQ(evaluate("\"hello\" + \" world\" == \"hello world\""), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("\"hello\" + \"\" == \"hello\""), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("\"\" + \"world\" == \"world\""), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("\"hello\" + \" world\" + \"!\" == \"hello world!\""), FDS_FILTER_YES);

    EXPECT_EQ(evaluate("\"hello world!\" contains \"hello\""), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("\"hello world!\" contains \"world\""), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("\"hello world!\" contains \" \""), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("\"hello world!\" contains \"\""), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("not \"hello world!\" contains \"foo\""), FDS_FILTER_YES);
}

TEST_F(Filter, bitwise_operations) {
    EXPECT_EQ(evaluate("0b11110000 | 0b01011111 == 0b11111111"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("0b11110000 ^ 0b01011111 == 0b10101111"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("0b11110000 & 0b01011111 == 0b01010000"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("~0b11110000 == 0b1111111111111111111111111111111111111111111111111111111100001111"), FDS_FILTER_YES);
}

TEST_F(Filter, bool_operations) {
    EXPECT_EQ(evaluate("1 and 1"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("not (1 and 0)"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("not (0 and 1)"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("0 or 1"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1 or 0"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("not (0 or 0)"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("not 0 or 0"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("not 0"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("0 or ((1 or 0) and 1)"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("(not (0 and 1)) or ((1 or 0) and 1)"), FDS_FILTER_YES);
}

TEST_F(Filter, flags) {
    constant("A", Value::UInt(0b100000), true);
    constant("S", Value::UInt(0b010000), true);
    constant("F", Value::UInt(0b001000), true);
    constant("R", Value::UInt(0b000100), true);
    constant("P", Value::UInt(0b000010), true);
    constant("U", Value::UInt(0b000001), true);
    constant("X", Value::UInt(0b111111), true);

    field("tcpflags", Value::UInt(0b111000), true);
    EXPECT_EQ(evaluate("tcpflags A"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("tcpflags S"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("tcpflags F"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("not tcpflags R"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("not tcpflags P"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("not tcpflags U"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("not tcpflags X"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("tcpflags A and tcpflags S"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("tcpflags A and tcpflags S and not tcpflags U"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("tcpflags 0b101000"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("tcpflags (A | S | F)"), FDS_FILTER_YES);

    reset_fields();
    field("tcpflags", Value::UInt(0b111111), true);
    EXPECT_EQ(evaluate("tcpflags A"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("tcpflags S"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("tcpflags F"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("tcpflags R"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("tcpflags P"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("tcpflags U"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("tcpflags X"), FDS_FILTER_YES);
}

TEST_F(Filter, number_mixed_types) {
    constant("a", Value::UInt(1));
    constant("b", Value::Int(2));
    constant("c", Value::Float(3));

    EXPECT_EQ(evaluate("a + a == b"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("a + b == c"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("a + 5 == 6"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("a + 5.0 == 6"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("a + 5 == 6.0"), FDS_FILTER_YES);

    EXPECT_EQ(evaluate("6u * 2u == 12u"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("6u % 2u == 0u"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("6u / 2u == 3u"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("6u - 2u == 4"), FDS_FILTER_YES);

    EXPECT_EQ(evaluate("0U > -1.0"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("0U < 1U"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("0U <= 1U"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1U >= 1U"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("1U != 0"), FDS_FILTER_YES);

    // TODO: should this be correct? it's how it works in C...
    EXPECT_EQ(evaluate("-1 < 1U"), FDS_FILTER_NO);
    EXPECT_EQ(evaluate("0U > -1"), FDS_FILTER_NO);
    EXPECT_EQ(evaluate("0U > -1U"), FDS_FILTER_NO);
}

TEST_F(Filter, implicit_compare) {
    constant("ip", Value::Ip("127.0.0.1"));
    constant("port", Value::UInt(80));

    EXPECT_EQ(evaluate("ip 127.0.0.1 and port 80"), FDS_FILTER_YES);
    EXPECT_EQ(evaluate("not ip 127.0.0.1 and not port 80"), FDS_FILTER_NO);
    EXPECT_EQ(evaluate("not ip 127.0.0.1 or not port 80"), FDS_FILTER_NO);
}


#if 0
TEST_F(Filter, sample) {
    expression("ip inside blacklist");
    field("ip",
        Value::Ip("127.0.0.1/32")
    );
    constant("blacklist",
        Value::List({
            Value::Ip("127.0.0.1/8"),
            Value::Ip("192.168.1.0/28"),
            Value::Ip("aabb:ccdd:eeff:0011::/64"),
            Value::Ip("aabb:ccdd:eeff:1122::/64"),
            Value::Ip("aabb:ccdd:eeff:2233::/64"),
        })
    );
    compile();
    EXPECT_TRUE(evaluate());
/*
    EXPECT_TRUE(evaluate("127.0.0.1/32 in blacklist"));
    EXPECT_TRUE(evaluate("aabb:ccdd:eeff:0011:1234:: in blacklist"));
    EXPECT_TRUE(evaluate("aabb:ccdd:eeff:1122:1234:: in blacklist"));
    EXPECT_TRUE(evaluate("aabb:ccdd:eeff:2233:1234:: in blacklist"));
    EXPECT_FALSE(evaluate("aabb:ccdd:eeff:2244:1234:: in blacklist"));
    EXPECT_FALSE(evaluate("aabb:ccdd:eeff:2255:1234:: in blacklist"));
    EXPECT_TRUE(evaluate("127.0.55.6 in blacklist"));
    EXPECT_FALSE(evaluate("128.0.55.6 in blacklist"));
*/
}
#endif

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}