find_package(LIBXML2 REQUIRED)
mark_as_advanced(LIBXML2_DIR)

# Configure a header file to pass some CMake variables
configure_file(
	"${PROJECT_SOURCE_DIR}/src/build_config.h.in"
	"${PROJECT_BINARY_DIR}/src/build_config.h"
)

# Header files for source code building
include_directories(
	"${PROJECT_SOURCE_DIR}/include/"
#   "${PROJECT_BINARY_DIR}/include/"  # Nothing useful now - disabled
	"${PROJECT_BINARY_DIR}/src/"      # for build_config.h
    ${LIBXML2_INCLUDE_DIRS}
)

# Core source files
#set(CORE_SOURCE
#	file.c
#)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Subdirectories with auxiliary components
add_subdirectory(dummy_dir)
add_subdirectory(xml_parser)

# Create a dynamic library from all source code
add_library(
	fds SHARED
	${CORE_SOURCE}
	$<TARGET_OBJECTS:dummy_obj>  # Dummy library
	$<TARGET_OBJECTS:xml_parser_obj>  # xml_parser library
)

target_link_libraries(fds ${LIBXML2_LIBRARIES})

# Set versions of the library
set_target_properties(fds PROPERTIES
	VERSION   "${LIBFDS_VERSION_MAJOR}.${LIBFDS_VERSION_MINOR}.${LIBFDS_VERSION_PATCH}"
	SOVERSION "${LIBFDS_VERSION_MAJOR}.${LIBFDS_VERSION_MINOR}"
)

# Installation targets
install(
	TARGETS fds LIBRARY
	DESTINATION lib
)
