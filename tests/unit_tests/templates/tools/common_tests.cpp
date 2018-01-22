//
// Created by lukashutak on 31.12.17.
//

#include <gtest/gtest.h>
#include <string>
#include "common_tests.h"

#define DEF_FLAG_PAIR(_pair_) \
    {(_pair_), #_pair_}

struct flag_pair {
    fds_template_flag_t value;
    const char *name;
};

void
ct_template_flags(const fds_template *tmplt, fds_template_flag_t exp)
{
    const std::vector<struct flag_pair> flags = {
        DEF_FLAG_PAIR(FDS_TEMPLATE_HAS_MULTI_IE),
        DEF_FLAG_PAIR(FDS_TEMPLATE_HAS_DYNAMIC),
        DEF_FLAG_PAIR(FDS_TEMPLATE_HAS_REVERSE),
        DEF_FLAG_PAIR(FDS_TEMPLATE_HAS_STRUCT),
        DEF_FLAG_PAIR(FDS_TEMPLATE_HAS_FKEY)
        // Add new flags here...
    };

    for (const auto &flag : flags) {
        SCOPED_TRACE("Testing template flag: " + std::string(flag.name));
        bool is_expected = (exp & flag.value) != 0;
        bool is_present = (tmplt->flags & flag.value) != 0;
        EXPECT_EQ(is_present, is_expected);
        exp &= ~flag.value;
    }

    EXPECT_EQ(exp, 0) << "Unexpected flag(s) not tested. Add it to the list...";
}

void
ct_tfield_flags(const fds_tfield *tfield, fds_template_flag_t exp)
{
    const std::vector<struct flag_pair> flags = {
        DEF_FLAG_PAIR(FDS_TFIELD_SCOPE),
        DEF_FLAG_PAIR(FDS_TFIELD_MULTI_IE),
        DEF_FLAG_PAIR(FDS_TFIELD_LAST_IE),
        DEF_FLAG_PAIR(FDS_TFIELD_FLOW_KEY),
        DEF_FLAG_PAIR(FDS_TFIELD_STRUCTURED),
        DEF_FLAG_PAIR(FDS_TFIELD_REVERSE),
        DEF_FLAG_PAIR(FDS_TFIEDL_BKEY_COM),
        DEF_FLAG_PAIR(FDS_TFIELD_BKEY_SRC),
        DEF_FLAG_PAIR(FDS_TFIELD_BKEY_DST)
        // Add new flags here...
    };

    for (const auto &flag : flags) {
        SCOPED_TRACE("Testing field flag: " + std::string(flag.name));
        bool is_expected = (exp & flag.value) != 0;
        bool is_present = (tfield->flags & flag.value) != 0;
        EXPECT_EQ(is_present, is_expected);
        exp &= ~flag.value;
    }

    EXPECT_EQ(exp, 0) << "Unexpected flag(s) not tested. Add it to the list...";
}
