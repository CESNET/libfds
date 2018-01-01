# Add header files of the Template generator
include_directories(../TEST_TOOLS/Templates)

set(AUX_TOOLS
	"../TEST_TOOLS/Templates/TGenerator.cpp"
	"../TEST_TOOLS/Templates/common_tests.cpp"
)

# List of tests
unit_tests_register_test(template_parse.cpp ${AUX_TOOLS})