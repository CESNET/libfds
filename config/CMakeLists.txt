# System directory
install(
	DIRECTORY system
	DESTINATION "${INSTALL_DIR_SYSCONF}/libfds"
)

# User directory (just create an empty directory)
install(
	DIRECTORY
	DESTINATION "${INSTALL_DIR_SYSCONF}/libfds/user"
)

install(
	DIRECTORY
	DESTINATION "${INSTALL_DIR_SYSCONF}/libfds/user/elements"
)
