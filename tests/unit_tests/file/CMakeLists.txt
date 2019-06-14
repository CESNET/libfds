# Add header files of the Message generator
include_directories(../tools/)

set(AUX_TOOLS
    "../tools/MsgGen.cpp"
    "../tools/MsgGen.h"
    "../tools/TGenerator.cpp"
    "../tools/TGenerator.h"
    "../tools/TMock.cpp"
    "../tools/TMock.h"
)

# Copy auxiliary files for tests
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/data/iana.xml"
    "${CMAKE_CURRENT_BINARY_DIR}/data/iana.xml"
    COPYONLY
)

# List of tests
if (ENABLE_TESTS_INTERNAL)
    # Unit Tests that must have access to internal components and symbols
    unit_tests_register_test(Block_content.cpp)
    unit_tests_register_test(Block_data.cpp ${AUX_TOOLS})
    unit_tests_register_test(Block_session.cpp)
    unit_tests_register_test(Block_templates.cpp ${AUX_TOOLS})
    unit_tests_register_test(File_exception.cpp)
endif()

unit_tests_register_test(file_simple.cpp ${AUX_TOOLS})
unit_tests_register_test(file_append.cpp ${AUX_TOOLS})
unit_tests_register_test(file_complex.cpp ${AUX_TOOLS})
unit_tests_register_test(file_invalid.cpp ${AUX_TOOLS})
