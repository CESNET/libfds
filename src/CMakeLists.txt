find_package(LibXml2 REQUIRED)
mark_as_advanced(LIBXML2_DIR)

# Find librt (required for implementation of asynchronous I/O)
find_library(LIBRT rt)
mark_as_advanced(LIBRT)

# Configure a header file to pass some CMake variables
configure_file(
	"${PROJECT_SOURCE_DIR}/src/build_config.h.in"
	"${PROJECT_BINARY_DIR}/src/build_config.h"
)

# Header files for source code building
include_directories(
	"${PROJECT_SOURCE_DIR}/include/"
	"${PROJECT_SOURCE_DIR}/src/3rd_party" # for 3party libraries
	"${PROJECT_BINARY_DIR}/include/"      # for api.h
	"${PROJECT_BINARY_DIR}/src/"          # for build_config.h
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
add_subdirectory(file)
add_subdirectory(filter)
add_subdirectory(trie)

# Create a dynamic library from all source code
add_library(
	fds SHARED
	api.c
	$<TARGET_OBJECTS:xml_parser_obj>   # XML parser
	$<TARGET_OBJECTS:iemgr_obj>        # IE manager
	$<TARGET_OBJECTS:template_mgr_obj> # Template manager
	$<TARGET_OBJECTS:drec_obj>         # Data record
	$<TARGET_OBJECTS:converters_obj>   # Converters
	$<TARGET_OBJECTS:parsers_obj>      # Parsers
	$<TARGET_OBJECTS:file_obj>         # File
	$<TARGET_OBJECTS:filter_obj>       # Filter
	$<TARGET_OBJECTS:trie_obj>         # Trie
	${PROJECT_SOURCE_DIR}/include/libfds/
)

target_link_libraries(fds ${LIBXML2_LIBRARIES})
if (LIBRT)
	target_link_libraries(fds ${LIBRT})
endif()

if (USE_SYSTEM_LZ4)
	# Try to find library in the system and link it to the library
	find_package(LibLz4 REQUIRED)
	include_directories(${LIBLZ4_INCLUDE_DIRS})
	target_link_libraries(fds ${LIBLZ4_LIBRARIES})
else()
	# Build static version of LZ4 and add it to the shared library
	add_subdirectory(3rd_party/lz4)
	target_link_libraries(fds fds_lz4)
endif()

if (USE_SYSTEM_ZSTD)
	# Try to find library in the system and link it to the library
	find_package(LibZstd REQUIRED)
	include_directories(${LIBZSTD_INCLUDE_DIRS})
	target_link_libraries(fds ${LIBZSTD_LIBRARIES})
else()
	# Build static version of ZSTD and add it to the shared library
	add_subdirectory(3rd_party/zstd)
	target_link_libraries(fds fds_zstd)
endif()

# Set versions of the library
set_target_properties(fds PROPERTIES
	VERSION   "${LIBFDS_VERSION_MAJOR}.${LIBFDS_VERSION_MINOR}.${LIBFDS_VERSION_PATCH}"
	SOVERSION "${LIBFDS_VERSION_MAJOR}"
)

# Installation targets
install(
	TARGETS fds LIBRARY
	DESTINATION ${INSTALL_DIR_LIB}
)
