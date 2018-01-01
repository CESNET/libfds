# Add header files of the Template generator
include_directories(./tools/)

set(AUX_TOOLS
	"./tools/TGenerator.cpp"
	"./tools/common_tests.cpp"
)

# List of tests
unit_tests_register_test(template_parse.cpp ${AUX_TOOLS})