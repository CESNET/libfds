# Add tools
include_directories(../tools/)

set(AUX_TOOLS
    "../tools/MsgGen.cpp"
    "../tools/MsgGen.h"
)

# Copy auxiliary files for tests
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/data/iana.xml"
    "${CMAKE_CURRENT_BINARY_DIR}/data/iana.xml"
    COPYONLY
)

# List of tests
unit_tests_register_test(datetime.cpp)
unit_tests_register_test(numbers_be.cpp)
unit_tests_register_test(to_string.cpp)
unit_tests_register_test(json.cpp ${AUX_TOOLS})