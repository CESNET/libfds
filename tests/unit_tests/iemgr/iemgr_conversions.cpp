//
// Created by lukashutak on 07/09/18.
//

#include <ipfixcol2.h>
#include <gtest/gtest.h>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(conversion, str2type)
{
    EXPECT_EQ(FDS_ET_OCTET_ARRAY,            fds_iemgr_str2type("octetArray"));
    EXPECT_EQ(FDS_ET_UNSIGNED_8,             fds_iemgr_str2type("unsigned8"));
    EXPECT_EQ(FDS_ET_UNSIGNED_16,            fds_iemgr_str2type("unsigned16"));
    EXPECT_EQ(FDS_ET_UNSIGNED_32,            fds_iemgr_str2type("unsigned32"));
    EXPECT_EQ(FDS_ET_UNSIGNED_64,            fds_iemgr_str2type("unsigned64"));
    EXPECT_EQ(FDS_ET_SIGNED_8,               fds_iemgr_str2type("signed8"));
    EXPECT_EQ(FDS_ET_SIGNED_16,              fds_iemgr_str2type("signed16"));
    EXPECT_EQ(FDS_ET_SIGNED_32,              fds_iemgr_str2type("signed32"));
    EXPECT_EQ(FDS_ET_SIGNED_64,              fds_iemgr_str2type("signed64"));
    EXPECT_EQ(FDS_ET_FLOAT_32,               fds_iemgr_str2type("float32"));
    EXPECT_EQ(FDS_ET_FLOAT_64,               fds_iemgr_str2type("float64"));
    EXPECT_EQ(FDS_ET_BOOLEAN,                fds_iemgr_str2type("boolean"));
    EXPECT_EQ(FDS_ET_MAC_ADDRESS,            fds_iemgr_str2type("macAddress"));
    EXPECT_EQ(FDS_ET_STRING,                 fds_iemgr_str2type("string"));
    EXPECT_EQ(FDS_ET_DATE_TIME_SECONDS,      fds_iemgr_str2type("dateTimeSeconds"));
    EXPECT_EQ(FDS_ET_DATE_TIME_MILLISECONDS, fds_iemgr_str2type("dateTimeMilliseconds"));
    EXPECT_EQ(FDS_ET_DATE_TIME_MICROSECONDS, fds_iemgr_str2type("dateTimeMicroseconds"));
    EXPECT_EQ(FDS_ET_DATE_TIME_NANOSECONDS,  fds_iemgr_str2type("dateTimeNanoseconds"));
    EXPECT_EQ(FDS_ET_IPV4_ADDRESS,           fds_iemgr_str2type("ipv4Address"));
    EXPECT_EQ(FDS_ET_IPV6_ADDRESS,           fds_iemgr_str2type("ipv6Address"));
    EXPECT_EQ(FDS_ET_BASIC_LIST,             fds_iemgr_str2type("basicList"));
    EXPECT_EQ(FDS_ET_SUB_TEMPLATE_LIST,      fds_iemgr_str2type("subTemplateList"));
    EXPECT_EQ(FDS_ET_SUB_TEMPLATE_MULTILIST, fds_iemgr_str2type("subTemplateMultiList"));

    // Invalid examples
    EXPECT_EQ(FDS_ET_UNASSIGNED,             fds_iemgr_str2type("-- invalid --"));
    EXPECT_EQ(FDS_ET_UNASSIGNED,             fds_iemgr_str2type(""));
}

TEST(conversion, type2str)
{
    int cnt = 0;
    for (size_t i = 0; i < FDS_ET_UNASSIGNED; ++i) {
        enum fds_iemgr_element_type type = static_cast<enum fds_iemgr_element_type>(i);
        const char *str = fds_iemgr_type2str(type);
        if (str == nullptr) {
            continue;
        }

        cnt++;
        EXPECT_EQ(type, fds_iemgr_str2type(str));
    }

    EXPECT_GT(cnt, 0) << "No conversion called!";
}

TEST(conversion, str2semantic)
{
    EXPECT_EQ(FDS_ES_DEFAULT,        fds_iemgr_str2semantic("default"));
    EXPECT_EQ(FDS_ES_QUANTITY,       fds_iemgr_str2semantic("quantity"));
    EXPECT_EQ(FDS_ES_TOTAL_COUNTER,  fds_iemgr_str2semantic("totalCounter"));
    EXPECT_EQ(FDS_ES_DELTA_COUNTER,  fds_iemgr_str2semantic("deltaCounter"));
    EXPECT_EQ(FDS_ES_IDENTIFIER,     fds_iemgr_str2semantic("identifier"));
    EXPECT_EQ(FDS_ES_FLAGS,          fds_iemgr_str2semantic("flags"));
    EXPECT_EQ(FDS_ES_LIST,           fds_iemgr_str2semantic("list"));
    EXPECT_EQ(FDS_ES_SNMP_COUNTER,   fds_iemgr_str2semantic("snmpCounter"));
    EXPECT_EQ(FDS_ES_SNMP_GAUGE,     fds_iemgr_str2semantic("snmpGauge"));

    // Invalid examples
    EXPECT_EQ(FDS_ES_UNASSIGNED,     fds_iemgr_str2semantic("-- invalid --"));
    EXPECT_EQ(FDS_ES_UNASSIGNED,     fds_iemgr_str2semantic(""));
}

TEST(conversion, semantic2str)
{
    int cnt = 0;
    for (size_t i = 0; i < FDS_ES_UNASSIGNED; ++i) {
        enum fds_iemgr_element_semantic sem = static_cast<enum fds_iemgr_element_semantic>(i);
        const char *str = fds_iemgr_semantic2str(sem);
        if (str == nullptr) {
            continue;
        }

        cnt++;
        EXPECT_EQ(sem, fds_iemgr_str2semantic(str));
    }

    EXPECT_GT(cnt, 0) << "No conversion called!";
}

TEST(conversion, str2unit)
{
    EXPECT_EQ(FDS_EU_NONE,          fds_iemgr_str2unit("none"));
    EXPECT_EQ(FDS_EU_BITS,          fds_iemgr_str2unit("bits"));
    EXPECT_EQ(FDS_EU_OCTETS,        fds_iemgr_str2unit("octets"));
    EXPECT_EQ(FDS_EU_PACKETS,       fds_iemgr_str2unit("packets"));
    EXPECT_EQ(FDS_EU_FLOWS,         fds_iemgr_str2unit("flows"));
    EXPECT_EQ(FDS_EU_SECONDS,       fds_iemgr_str2unit("seconds"));
    EXPECT_EQ(FDS_EU_MILLISECONDS,  fds_iemgr_str2unit("milliseconds"));
    EXPECT_EQ(FDS_EU_MICROSECONDS,  fds_iemgr_str2unit("microseconds"));
    EXPECT_EQ(FDS_EU_NANOSECONDS,   fds_iemgr_str2unit("nanoseconds"));
    EXPECT_EQ(FDS_EU_4_OCTET_WORDS, fds_iemgr_str2unit("4-octet words"));
    EXPECT_EQ(FDS_EU_MESSAGES,      fds_iemgr_str2unit("messages"));
    EXPECT_EQ(FDS_EU_HOPS,          fds_iemgr_str2unit("hops"));
    EXPECT_EQ(FDS_EU_ENTRIES,       fds_iemgr_str2unit("entries"));
    EXPECT_EQ(FDS_EU_FRAMES,        fds_iemgr_str2unit("frames"));
    EXPECT_EQ(FDS_EU_PORTS,         fds_iemgr_str2unit("ports"));
    EXPECT_EQ(FDS_EU_INFERRED,      fds_iemgr_str2unit("inferred"));

    // Invalid examples
    EXPECT_EQ(FDS_EU_UNASSIGNED,    fds_iemgr_str2unit("-- invalid --"));
    EXPECT_EQ(FDS_EU_UNASSIGNED,    fds_iemgr_str2unit(""));
}

TEST(conversion, unit2str)
{
    int cnt = 0;
    for (size_t i = 0; i < FDS_EU_UNASSIGNED; ++i) {
        enum fds_iemgr_element_unit unit = static_cast<enum fds_iemgr_element_unit>(i);
        const char *str = fds_iemgr_unit2str(unit);
        if (str == nullptr) {
            continue;
        }

        cnt++;
        EXPECT_EQ(unit, fds_iemgr_str2unit(str));
    }

    EXPECT_GT(cnt, 0) << "No conversion called!";
}
