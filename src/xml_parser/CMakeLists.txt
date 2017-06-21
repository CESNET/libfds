# Create a xml_parser "object" library
set(XML_PARSER_SRC
	xml_parser.cpp
)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(xml_parser_obj OBJECT ${XML_PARSER_SRC})

