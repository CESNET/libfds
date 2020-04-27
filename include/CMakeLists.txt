# Configure a header file to pass library version to the source code
configure_file(
	"${PROJECT_SOURCE_DIR}/include/libfds/api.h.in"
	"${PROJECT_BINARY_DIR}/include/libfds/api.h"
	)

# List of header files
set(TOP_HEADERS
	libfds.h
)

set(SUB_HEADERS
	"${PROJECT_BINARY_DIR}/include/libfds/api.h"
	libfds/converters.h
	libfds/iemgr.h
	libfds/drec.h
	libfds/file.h
	libfds/filter.h
	libfds/ipfix_parsers.h
	libfds/ipfix_structs.h
	libfds/ipfix_filter.h
	libfds/template.h
	libfds/template_mgr.h
	libfds/trie.h
	libfds/xml_parser.h
)

# Installation targets
install(
	FILES ${TOP_HEADERS}
	DESTINATION ${INSTALL_DIR_INCLUDE}
)

install(
	FILES ${SUB_HEADERS}
	DESTINATION "${INSTALL_DIR_INCLUDE}/libfds/"
)
