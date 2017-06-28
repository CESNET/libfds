find_package(LIBXML2 REQUIRED)
mark_as_advanced(LIBXML2_DIR)

include_directories(${LIBXML2_INCLUDE_DIRS})

# List of test files
unit_tests_register_test(main.cpp)
