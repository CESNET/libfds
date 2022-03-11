#pragma once

#include <libfds/iemgr.h>
#include "iemgr_common.h"

int
read_aliases_file(fds_iemgr_t *mgr, const char *path);

void
aliases_destroy(fds_iemgr_t *mgr);

int
aliases_copy(const fds_iemgr_t *old_mgr, fds_iemgr_t *new_mgr);

fds_iemgr_alias *
find_alias_by_name(fds_iemgr *mgr, const char *name);