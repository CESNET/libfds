# List of header files
set(TOP_HEADERS
	libfds.h
)

set(SUB_HEADERS
	libfds/api.h
	libfds/file.h
	libfds/xml_parser.h
	libfds/iemgr.h
	libfds/common.h
)

# Installation targets
install(
	FILES ${TOP_HEADERS}
	DESTINATION include
)

install(
	FILES ${SUB_HEADERS}
	DESTINATION include/libfds/
)
