# Create a xml_parser "object" library
set(IEMGR_SRC
	iemgr_common.cpp
	iemgr_element.cpp
	iemgr_scope.cpp
	iemgr.cpp
	fds_iemgr_todo_name.h
)

add_library(iemgr_obj OBJECT ${IEMGR_SRC})

