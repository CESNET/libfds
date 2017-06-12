# Create a dummy "object" library
set(DUMMY_SRC
	dummy.c
)

add_library(dummy_obj OBJECT
	${DUMMY_SRC}
)
