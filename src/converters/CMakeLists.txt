# Create a Converter "object" library
set(CONVERTERS_SRC
	converters.c
	branchlut2.h
)

add_library(converters_obj OBJECT ${CONVERTERS_SRC})
