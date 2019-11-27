#ifndef FDS_FILTER_COMMON_H
#define FDS_FILTER_COMMON_H

#include <stdio.h>

#define ASSERT_UNREACHABLE()    assert(0 && "UNREACHABLE")
#define CONST_ARR_SIZE(x)   (sizeof(x) / sizeof((x)[0]))
#define UNUSED(x)   (void)(x)
#ifndef NDEBUG
#define PTRACE(fmt, ...) \
    do { fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__,##__VA_ARGS__); } while (0)
#define PDEBUG(fmt, ...) \
    do { fprintf(stderr, fmt "\n",##__VA_ARGS__); } while (0)
#else
#define PTRACE(fmt, ...) /* nothing */
#define PDEBUG(fmt, ...) /* nothing */
#endif

#endif
