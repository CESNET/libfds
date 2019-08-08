include(${CMAKE_SOURCE_DIR}/CMakeModules/unit_tests.cmake)

# Include public headers
include_directories(
	"${PROJECT_SOURCE_DIR}/include/"
	"${PROJECT_BINARY_DIR}/include/" # libfds/api.h
)

# List of tests
add_subdirectory(xml_parser)
add_subdirectory(iemgr)
add_subdirectory(templates)
add_subdirectory(converters)
add_subdirectory(parsers)
add_subdirectory(drec)
add_subdirectory(lists)
add_subdirectory(file)

unit_tests_register_test(api.cpp)
# >> Add your new tests or test subdirectories HERE <<
add_subdirectory(filter)
add_subdirectory(trie)

# Enable code coverage target (i.e. make coverage) when appropriate build
# type is chosen.
if (ENABLE_TESTS AND ENABLE_TESTS_COVERAGE)
	coverage_add_target()
endif()

