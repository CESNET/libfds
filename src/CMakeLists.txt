find_package(LibXml2 REQUIRED)
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
	"${LIBXML2_INCLUDE_DIR}"
)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Subdirectories with auxiliary components
add_subdirectory(xml_parser)
add_subdirectory(iemgr)
add_subdirectory(template_mgr)
add_subdirectory(drec)
add_subdirectory(converters)
add_subdirectory(parsers)

# Create a dynamic library from all source code
add_library(
	fds SHARED
	${CORE_SOURCE}
	$<TARGET_OBJECTS:xml_parser_obj>   # XML parser
	$<TARGET_OBJECTS:iemgr_obj>        # IE manager
	$<TARGET_OBJECTS:template_mgr_obj> # Template manager
	$<TARGET_OBJECTS:drec_obj>         # Data record
	$<TARGET_OBJECTS:converters_obj>   # Converters
	$<TARGET_OBJECTS:parsers_obj>      # Parsers

	${PROJECT_SOURCE_DIR}/include/libfds/
)

target_link_libraries(fds ${LIBXML2_LIBRARIES})

# Set versions of the library
set_target_properties(fds PROPERTIES
	VERSION   "${LIBFDS_VERSION_MAJOR}.${LIBFDS_VERSION_MINOR}.${LIBFDS_VERSION_PATCH}"
	SOVERSION "${LIBFDS_VERSION_MAJOR}"
)

# Installation targets
install(
	TARGETS fds LIBRARY
	DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)
