find_package(LibXml2 REQUIRED)
mark_as_advanced(LIBXML2_DIR)

include_directories(${LIBXML2_INCLUDE_DIRS})

# List of test files
# unit_tests_register_test(main.cpp)
unit_tests_register_test(xml_parser_create.cpp)
unit_tests_register_test(xml_parser_destroy.cpp)
unit_tests_register_test(xml_parser_set_args.cpp)
unit_tests_register_test(xml_parser_parse.cpp)
unit_tests_register_test(xml_parser_next.cpp)
unit_tests_register_test(xml_parser_rewind.cpp)
unit_tests_register_test(xml_parser_last_err.cpp)
