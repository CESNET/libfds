# List of header files
set(TOP_HEADERS
	libfds.h
)

set(SUB_HEADERS
	libfds/api.h
	libfds/converters.h
	libfds/drec.h
	libfds/iemgr.h
	libfds/ipfix_parsers.h
	libfds/ipfix_structs.h
	libfds/template.h
	libfds/template_mgr.h
	libfds/xml_parser.h
	)

# Installation targets
install(
	FILES ${TOP_HEADERS}
	DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}
)

install(
	FILES ${SUB_HEADERS}
	DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}/libfds/"
)
