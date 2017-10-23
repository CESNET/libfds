# System directory
install(
	DIRECTORY system
	DESTINATION etc/fds
)

# User directory (just create an empty directory)
install(
	DIRECTORY
	DESTINATION etc/fds/user
)
