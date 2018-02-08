# Create a Data record "object" library
set(DREC_SRC
	iterator.c
)

add_library(drec_obj OBJECT ${DREC_SRC})
