# Add header files of the Template generator
include_directories(./tools/)

set(AUX_TOOLS
	"./tools/TGenerator.cpp"
	"./tools/common_tests.cpp"
	"./tools/TMock.cpp"
)

# Copy auxiliary files for tests
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/data/iana.xml"
	"${CMAKE_CURRENT_BINARY_DIR}/data/iana.xml"
	COPYONLY
)

# List of tests
unit_tests_register_test(template_parse.cpp ${AUX_TOOLS})
unit_tests_register_test(template_copy.cpp ${AUX_TOOLS})
unit_tests_register_test(template_ies_define.cpp ${AUX_TOOLS})
unit_tests_register_test(template_flowkey.cpp ${AUX_TOOLS})

unit_tests_register_test(tmgr_common.cpp ${AUX_TOOLS})
unit_tests_register_test(tmgr_tcp.cpp ${AUX_TOOLS})
unit_tests_register_test(tmgr_tcpSctp.cpp ${AUX_TOOLS})
unit_tests_register_test(tmgr_tcpSctpFile.cpp ${AUX_TOOLS})
unit_tests_register_test(tmgr_udp.cpp ${AUX_TOOLS})
unit_tests_register_test(tmgr_udpSctpFile.cpp ${AUX_TOOLS})
