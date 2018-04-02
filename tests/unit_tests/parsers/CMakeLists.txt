# Add header files of the Template generator
include_directories(../tools/)

set(AUX_TOOLS
	"../tools/MsgGen.cpp"
	"../tools/MsgGen.h"
)

unit_tests_register_test(parser_msg.cpp ${AUX_TOOLS})
unit_tests_register_test(parser_tset.cpp ${AUX_TOOLS})
unit_tests_register_test(parser_dset.cpp ${AUX_TOOLS})
