# Create a xml_parser "object" library
set(IEMGR_SRC
	iemgr_common.cpp
	iemgr_element.cpp
	iemgr_scope.cpp
	iemgr.cpp
	iemgr_common.h
	iemgr_scope.h
	iemgr_element.h
)

add_library(iemgr_obj OBJECT ${IEMGR_SRC})
