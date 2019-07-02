#ifndef LIBFDS_FILTER_UTIL_H
#define LIBFDS_FILTER_UTIL_H

#include <libfds.h>

#ifndef NDEBUG
#define pdebug(fmt, ...) \
        do { fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__, __VA_ARGS__); } while (0)
#else
#define pdebug(fmt, ...) /* nothing */
#endif

#endif // LIBFDS_FILTER_UTIL_H