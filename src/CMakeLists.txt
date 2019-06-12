find_package(LibXml2 REQUIRED)
mark_as_advanced(LIBXML2_DIR)

# Find librt (required for implementation of asynchronous I/O)
find_library(LIBRT rt)

# Configure a header file to pass some CMake variables
configure_file(
	"${PROJECT_SOURCE_DIR}/src/build_config.h.in"
	"${PROJECT_BINARY_DIR}/src/build_config.h"
)

# Header files for source code building
include_directories(
	"${PROJECT_SOURCE_DIR}/include/"
    "${PROJECT_BINARY_DIR}/include/"  # for api.h
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
add_subdirectory(file)

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

	${PROJECT_SOURCE_DIR}/include/libfds/
)

target_link_libraries(fds ${LIBXML2_LIBRARIES})
if (LIBRT)
	target_link_libraries(fds ${LIBRT})
endif()

# -----------------------------------------------------------------------------
# Build ZSTD static library
include(ExternalProject)
ExternalProject_Add(
    project_zstdlib
    SOURCE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/file/zstd/
    PREFIX          ${CMAKE_CURRENT_BINARY_DIR}/file/zstdlib
    UPDATE_COMMAND  "" # Do nothing
    INSTALL_COMMAND "" # Do nothing
    LOG_CONFIGURE   ON
    LOG_BUILD       ON
)

# Extract information about the ZSTD static library
ExternalProject_Get_Property(project_zstdlib source_dir)
set(ZSTD_INCLUDE_DIRS ${source_dir}/lib)
ExternalProject_Get_Property(project_zstdlib binary_dir)
set(ZSTD_LIBRARY_PATH ${binary_dir}/lib/${CMAKE_FIND_LIBRARY_PREFIXES}zstd.a)

# Create "wrapper" for the static library
set(ZSTD_NAME "zstdlib")
add_library(${ZSTD_NAME} STATIC IMPORTED)
set_target_properties(
    ${ZSTD_NAME} PROPERTIES
    IMPORTED_LOCATION ${ZSTD_LIBRARY_PATH}
    #INTERFACE_LINK_LIBRARIES ${CMAKE_THREAD_LIBS_INIT}
)
add_dependencies(${ZSTD_NAME} project_zstdlib)

# Add dependency on ZSTD static library to file_obj
#target_include_directories($<TARGET_OBJECTS:file_obj> PUBLIC ${ZSTD_INCLUDE_DIRS})
target_link_libraries(fds ${ZSTD_NAME})
# -----------------------------------------------------------------------------

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
