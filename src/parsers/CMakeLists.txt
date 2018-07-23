# Create a Parser "object" library
set(PARSERS_SRC
	ipfix_parser.c
	list_parser.c)

add_library(parsers_obj OBJECT ${PARSERS_SRC})
