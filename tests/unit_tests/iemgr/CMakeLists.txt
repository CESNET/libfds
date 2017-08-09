# List of test files
unit_tests_register_test(iemgr_create.cpp)
unit_tests_register_test(iemgr_copy.cpp)
unit_tests_register_test(iemgr_clear.cpp)
unit_tests_register_test(iemgr_destroy.cpp)
unit_tests_register_test(iemgr_compare_timestamps.cpp)
unit_tests_register_test(iemgr_read.cpp)
unit_tests_register_test(iemgr_find.cpp)
unit_tests_register_test(iemgr_err.cpp)

file(COPY test_files DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
