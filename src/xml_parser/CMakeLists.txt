# Create a XML parser "object" library
set(XML_PARSER_SRC
	xml_parser.cpp
)

add_library(xml_parser_obj OBJECT ${XML_PARSER_SRC})

