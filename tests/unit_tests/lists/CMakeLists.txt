# Add header files of the Message generator
include_directories(../tools/)

set(AUX_TOOLS
	"../tools/MsgGen.cpp"
	"../tools/MsgGen.h"
)

# Copy auxiliary files for tests
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/data/iana.xml"
	"${CMAKE_CURRENT_BINARY_DIR}/data/iana.xml"
	COPYONLY
)

unit_tests_register_test(blist.cpp ${AUX_TOOLS})
unit_tests_register_test(new_stlist.cpp ${AUX_TOOLS})