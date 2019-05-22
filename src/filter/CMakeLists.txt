CMAKE_MINIMUM_REQUIRED(VERSION 2.8) 

FIND_PACKAGE(BISON)
FIND_PACKAGE(FLEX)

set(bison_source ${CMAKE_BINARY_DIR}/parser.c)
set(bison_header ${CMAKE_BINARY_DIR}/parser.h)
set(flex_source ${CMAKE_BINARY_DIR}/scanner.c)
set(flex_header ${CMAKE_BINARY_DIR}/scanner.h)

if (BISON_FOUND)
	add_custom_command(
		OUTPUT ${bison_source} ${bison_header}
		COMMAND ${BISON_EXECUTABLE} 
			--output-file=${bison_source} 
			--defines=${bison_header} 
			-t -d --warnings=all 
			${CMAKE_CURRENT_SOURCE_DIR}/parser.y
		COMMENT "Generating parser files"
	)
endif()

if (FLEX_FOUND)
	add_custom_command(
		OUTPUT ${flex_source} ${flex_header}
		COMMAND ${FLEX_EXECUTABLE} 
			--outfile=${flex_source}
			--header-file=${flex_header} 
			-d
			${CMAKE_CURRENT_SOURCE_DIR}/scanner.l
		DEPENDS ${bison_header}
		COMMENT "Generating scanner files"
	)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})

add_library(filter_obj OBJECT
	filter.c
	ast.c ast.h
	${flex_source} ${flex_header}	
	${bison_source} ${bison_header}
)
