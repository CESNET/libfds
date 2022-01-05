#pragma once

#include <libfds/iemgr.h>
#include "iemgr_common.h"

void
mappings_destroy(fds_iemgr_t *mgr);

int
mappings_copy(const fds_iemgr_t *old_mgr, fds_iemgr_t *new_mgr);

const struct fds_iemgr_mapping_item *
find_mapping_in_elem(const fds_iemgr_elem *elem, const char *key);