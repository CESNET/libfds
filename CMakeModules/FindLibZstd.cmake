#  LIBZSTD_FOUND - System has ZSTD
#  LIBZSTD_INCLUDE_DIRS - The ZSTD include directories
#  LIBZSTD_LIBRARIES - The libraries needed to use ZSTD

find_path(
    ZSTD_INCLUDE_DIR zstd.h
    PATH_SUFFIXES include
)

find_library(
    ZSTD_LIBRARY
    NAMES zstd libzstd
    PATH_SUFFIXES lib lib64
)

set(ZSTD_HEADER_FILE "${ZSTD_INCLUDE_DIR}/zstd.h")
if (ZSTD_INCLUDE_DIR AND EXISTS ${ZSTD_HEADER_FILE})
    # Try to extract library version from the header file
    file(STRINGS ${ZSTD_HEADER_FILE} zstd_major_define
        REGEX "^#define[\t ]+ZSTD_VERSION_MAJOR[\t ]+[0-9]+"
        LIMIT_COUNT 1
    )
    file(STRINGS ${ZSTD_HEADER_FILE} zstd_minor_define
        REGEX "^#define[\t ]+ZSTD_VERSION_MINOR[\t ]+[0-9]+"
        LIMIT_COUNT 1
    )
    file(STRINGS ${ZSTD_HEADER_FILE} zstd_release_define
        REGEX "^#define[\t ]+ZSTD_VERSION_RELEASE[\t ]+[0-9]+"
        LIMIT_COUNT 1
    )

    string(REGEX REPLACE "^#define[\t ]+ZSTD_VERSION_MAJOR[\t ]+([0-9]+).*" "\\1"
        zstd_major_num ${zstd_major_define})
    string(REGEX REPLACE "^#define[\t ]+ZSTD_VERSION_MINOR[\t ]+([0-9]+).*" "\\1"
        zstd_minor_num ${zstd_minor_define})
    string(REGEX REPLACE "^#define[\t ]+ZSTD_VERSION_RELEASE[\t ]+([0-9]+).*" "\\1"
        zstd_release_num ${zstd_release_define})

    set(ZSTD_VERSION_STRING "${zstd_major_num}.${zstd_minor_num}.${zstd_release_num}")
endif()
unset(ZSTD_HEADER_FILE)

# Handle the REQUIRED arguments and set LIBZSTD_FOUND to TRUE if all listed
# variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibZstd
    REQUIRED_VARS ZSTD_LIBRARY ZSTD_INCLUDE_DIR
    VERSION_VAR ZSTD_VERSION_STRING
)

set(LIBZSTD_LIBRARIES ${ZSTD_LIBRARY})
set(LIBZSTD_INCLUDE_DIRS ${ZSTD_INCLUDE_DIR})
mark_as_advanced(ZSTD_INCLUDE_DIR ZSTD_LIBRARY)
