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

unit_tests_register_test(api.cpp)
# >> Add your new tests or test subdirectories HERE <<

# Enable code coverage target (i.e. make coverage) when appropriate build
# type is chosen.
if (${BUILD_TYPE_UPPER} STREQUAL "COVERAGE")
	coverage_add_target()
endif()

