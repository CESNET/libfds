cmake_minimum_required(VERSION 2.8.8)
project(libfds)

# Description of the project
set(PROJECT_DESCRIPTION
	"Flow Data Storage library is set of tools for processing flow records."
)

# Check the platform
if (NOT UNIX)
	message(FATAL_ERROR "Only Unix like systems are supported.")
endif()

# Versions and other informations
set(LIBFDS_VERSION_MAJOR 0)
set(LIBFDS_VERSION_MINOR 6)
set(LIBFDS_VERSION_PATCH 0)
set(LIBFDS_VERSION
	${LIBFDS_VERSION_MAJOR}.${LIBFDS_VERSION_MINOR}.${LIBFDS_VERSION_PATCH})

# Include modules
include(CMakeModules/git_info.cmake)
include(CMakeModules/install_dirs.cmake)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

# Include custom FindXXX modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules")

CHECK_C_COMPILER_FLAG(-std=gnu11 COMPILER_SUPPORT_GNU11)
if (NOT COMPILER_SUPPORT_GNU11)
	message(FATAL_ERROR "Compiler does NOT support C11 with GNU extension")
endif()

CHECK_CXX_COMPILER_FLAG(-std=gnu++11 COMPILER_SUPPORT_GNUXX11)
if (NOT COMPILER_SUPPORT_GNUXX11)
	message(FATAL_ERROR "Compiler does NOT support C++11 with GNU extension")
endif()

# ------------------------------------------------------------------------------
# Set default build type if not specified by user
set(DEFAULT_BUILD_TYPE "Release")
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    # Use the default build type if not specified
    message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE
        STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

option(ENABLE_DOC            "Enable documentation building"            OFF)
option(ENABLE_TESTS          "Build Unit tests (make test)"             OFF)
option(ENABLE_TESTS_INTERNAL "Build Unit tests that don't use public API" OFF)
option(ENABLE_TESTS_VALGRIND "Build Unit tests with Valgrind Memcheck"  OFF)
option(ENABLE_TESTS_COVERAGE "Enable support for code coverage"         OFF)
option(PACKAGE_BUILDER_RPM   "Enable RPM package builder (make rpm)"    OFF)
option(PACKAGE_BUILDER_DEB   "Enable DEB package builder (make deb)"    OFF)
option(USE_SYSTEM_LZ4        "Use system-installed LZ4 library"         ON)
option(USE_SYSTEM_ZSTD       "Use system-installed ZSTD library"        ON)

## -----------------------------------------------------------------------------
# Hard coded definitions

if (NOT ENABLE_TESTS_INTERNAL)
	# Hide functions that doesn't belong to the public API
	set(CMAKE_C_FLAGS        "${CMAKE_C_FLAGS} -fvisibility=hidden")
	set(CMAKE_CXX_FLAGS      "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
else()
	message(WARNING "Internal Unit Tests enabled! Do not use this version in production!")
endif()

set(CMAKE_C_FLAGS            "${CMAKE_C_FLAGS} -std=gnu11")
set(CMAKE_C_FLAGS_RELEASE    "-O2 -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG      "-g -O0 -Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS          "${CMAKE_CXX_FLAGS} -std=gnu++11")
set(CMAKE_CXX_FLAGS_RELEASE  "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG    "-g -O0 -Wall -Wextra -pedantic")

if (ENABLE_TESTS AND ENABLE_TESTS_COVERAGE)
	# Additional flags for code coverage
	set(CMAKE_C_FLAGS    "${CMAKE_C_FLAGS}   --coverage -fprofile-arcs -ftest-coverage")
	set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
endif()

# ------------------------------------------------------------------------------
# Project components
add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(docs)
add_subdirectory(config)
add_subdirectory(pkg)

if (ENABLE_TESTS)
	enable_testing()
	add_subdirectory(tests/unit_tests)
endif()

# ------------------------------------------------------------------------------
# Status messages
string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE_UPPER)
MESSAGE(STATUS
	"\n\n"
	"libfds version...:   ${LIBFDS_VERSION}\n"
	"Install prefix...:   ${CMAKE_INSTALL_PREFIX}\n"
	"Build type.......:   ${BUILD_TYPE_UPPER}\n"
	"C Compiler.......:   ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}\n"
	"C Flags..........:   ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_${BUILD_TYPE_UPPER}}\n"
	"C++ Compiler.....:   ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}\n"
	"C++ Flags........:   ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${BUILD_TYPE_UPPER}}\n"
	"Doxygen..........:   ${DOXYGEN_FOUND}\n"
)
