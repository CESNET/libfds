//
// Created by lukashutak on 31.12.17.
//

#include <gtest/gtest.h>
#include "common_tests.h"

void
ct_template_flags(const fds_template *tmplt, fds_template_flag_t exp)
{
    const std::vector<enum fds_template_features> flags = {
        FDS_TEMPLATE_HAS_MULTI_IE,
        FDS_TEMPLATE_HAS_DYNAMIC,
        FDS_TEMPLATE_HAS_REVERSE,
        FDS_TEMPLATE_HAS_STRUCT,
        FDS_TEMPLATE_HAS_FKEY
        // Add new flags here...
    };

    for (const auto &flag : flags) {
        SCOPED_TRACE("Template flag: " + std::to_string(flag));
        bool is_expected = (exp & flag) != 0;
        bool is_present = (tmplt->flags & flag) != 0;
        EXPECT_EQ(is_present, is_expected);
    }
}

void
ct_tfield_flags(const fds_tfield *tfield, fds_template_flag_t exp)
{
    const std::vector<enum fds_tfield_features> flags = {
        FDS_TFIELD_SCOPE,
        FDS_TFIELD_MULTI_IE,
        FDS_TFIELD_LAST_IE,
        FDS_TFIELD_STRUCTURED,
        FDS_TFIELD_REVERSE,
        FDS_TFIELD_FLOW_KEY
        // Add new flags here...
    };

    for (const auto &flag : flags) {
        SCOPED_TRACE("Field flag: " + std::to_string(flag));
        bool is_expected = (exp & flag) != 0;
        bool is_present = (tfield->flags & flag) != 0;
        EXPECT_EQ(is_present, is_expected);
    }
}
