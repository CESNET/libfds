#ifndef FDS_FILTER_COMMON_H
#define FDS_FILTER_COMMON_H

#include <assert.h>

#define ASSERT_UNREACHABLE()    assert(0 && "UNREACHABLE")
#define CONST_ARR_SIZE(x)   (sizeof(x) / sizeof((x)[0]))

#endif
