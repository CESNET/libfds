# Create a IE manager "object" library
set(IEMGR_SRC
	iemgr_common.cpp
	iemgr_element.cpp
	iemgr_scope.cpp
	iemgr_alias.cpp
	iemgr_mapping.cpp
	iemgr.cpp
	iemgr_common.h
	iemgr_scope.h
	iemgr_element.h
	iemgr_alias.h
	iemgr_mapping.h
)

add_library(iemgr_obj OBJECT ${IEMGR_SRC})
