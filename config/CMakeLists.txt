# System directory
install(
	DIRECTORY system
	DESTINATION "${CMAKE_INSTALL_FULL_SYSCONFDIR}/libfds"
)

# User directory (just create an empty directory)
install(
	DIRECTORY
	DESTINATION "${CMAKE_INSTALL_FULL_SYSCONFDIR}/libfds/user"
)
