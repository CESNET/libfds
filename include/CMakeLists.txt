# List of header files
set(TOP_HEADERS
	libfds.h
)

set(SUB_HEADERS
	libfds/api.h
	libfds/xml_parser.h
	libfds/iemgr.h
	libfds/template.h
	libfds/template_mgr.h
	libfds/drec.h
	libfds/converters.h
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
