# Build ZSTD static library
include(ExternalProject)
ExternalProject_Add(
    project_zstdlib
    SOURCE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/src/
    PREFIX          ${CMAKE_CURRENT_BINARY_DIR}/zstdlib
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
add_library(fds_zstd STATIC IMPORTED GLOBAL)
set_target_properties(
    fds_zstd PROPERTIES
    IMPORTED_LOCATION ${ZSTD_LIBRARY_PATH}
    #INTERFACE_LINK_LIBRARIES ${CMAKE_THREAD_LIBS_INIT}
)
add_dependencies(fds_zstd project_zstdlib)
