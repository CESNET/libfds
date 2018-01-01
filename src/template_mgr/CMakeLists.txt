# Create Template manager "object" library
set(TMGR_SRC
	garbage.c
	garbage.h
	snapshot.c
	snapshot.h
	template.c
	template_manager.c
)

add_library(template_mgr_obj OBJECT ${TMGR_SRC})
