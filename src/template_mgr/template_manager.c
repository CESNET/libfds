/**
 * \file   src/template_mgr/template_manager.c
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \brief  Template manager (header file)
 * \date   February 2018
 */

/*
 * Copyright (C) 2017 CESNET, z.s.p.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is'', and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */

#include <stdint.h>
#include <libfds.h>
#include <stdlib.h>
#include <assert.h>

#include "garbage.h"
#include "snapshot.h"

/** Default snapshot lifetime if the history mod is enabled */
#define SNAPSHOT_DEF_LIFETIME 15

/** Template withdrawal modes */
enum withdraw_mod_e {
    /** Template withdrawal is not permitted operation */
    WITHDRAW_PROHIBITED,
    /** Template withdrawal is optional, but not required */
    WITHDRAW_OPTIONAL,
    /** Template withdrawal is required before changing definition of a template */
    WITHDRAW_REQUIRED
};

/**
 * \defgroup template_manager Template manager (internal)
 *
 * \brief Template manager internal structure
 *
 * Template manager is internally represented as a double linked list of snapshots ordered by
 * Export Time from the newest one to the oldest one. Validity range of a snapshot is defined
 * by a start time and an end time. In other words, the end time is a time when a snapshot was
 * replaced by a newer one. Each snapshot has only references to templates that are valid in its
 * context.
 *
 * Each template reference has a set of flags (Create, Delete, etc.).
 * "Create" flag represents the first snapshot that added a reference to a new template. This
 * flags vanishes with snapshot destruction. On the other hand, "Delete" flag represents the
 * newest snapshot that has a reference to a template. A snapshot with this flag is also responsible
 * for destruction of the template when it is not valid anymore. Therefore, every time a snapshot
 * is copied, all "Delete" flags are moved to the new copy. Every time a snapshot is removed from
 * the template manager's hierarchy, "Delete" flags (if possible) must be moved to another snapshot
 * first. Flags are only thing that can be modified on frozen snapshots! A newly added template
 * (a refreshed template is also a new one) inserted to the manager's snapshot always has both
 * flags ("Create" and "Delete") set.
 *
 * All template operations (adding/withdrawing/etc.) are always performed on a snapshot in the
 * hierarchy (usually on the newest one) that is in editable mode. When a reference to a template
 * in the snapshot or a reference to the snapshot itself is passed to user, the snapshot must be
 * frozen. Change of current Export Time usually also freeze all previous snapshots. Once the
 * snapshot is frozen, it is not possible to modify it anymore. If it is necessary to make
 * modifications of the snapshot, a new copy must be created first. Therefore, all snapshots
 * (except the oldest one) are create only by copying!
 *
 * See example of the template hierarchy below:
 * \verbatim
 *
 *     The oldest                                             The newest
 *   +------------+     +------------+    +------------+    +------------+
 *   |  Snapshot  |     |  Snapshot  |    |  Snapshot  |    |  Snapshot  |
 *   |   Time X   |     |  Time X+1  |    |  Time X+1  |    |  Time X+2  |
 *   +------------+     +------------+    +------------+    +------------+
 *         |C               |D   |C           |_               |D     |CD
 *         |                |    v            |                |      |
 *         |                |    T2 <---------+----------------+      |
 *         v                |                                         v
 *         T1 <-------------+                                         T1
 *
 *   Legend: C = Create flag, D = Delete flag, _ = no flags
 *
 * \endverbatim
 *
 * How the example above was probably created?
 * The snapshot at time X was created and a template T1 was added (flags CD).
 * At export time X+1, a user added the new template T2 with flags CD and the "Delete" flag of T1
 * was automatically moved to the new snapshot. The user probably wanted a reference to the
 * snapshot that disabled modification of the snapshot. Therefore, following withdrawing the
 * template T1 caused creating a new copy with the same time (without the reference to the
 * template T1) and moving "Delete" flag of T2 to the copy. At time X+2 a new template T1 was
 * defined.
 *
 * @{
 */

/**
 * Template manager
 */
struct fds_tmgr {
    /** Export Time of selected context             */
    uint32_t time_now;
    /** The newest time (the manager has even seen) */
    uint32_t time_newest;

    struct {
        /**
         * \brief Template lifetime of "normal" Templates (in seconds)
         * \note If the value is zero or the lifetime mod is disabled
         *   (fds_tmgr#cfg#en_lifetime), the lifetime of "normal" Templates is disabled.
         */
        uint32_t lifetime_normal;
        /**
         * \brief Template lifetime of Options Templates (in seconds)
         * \note If the value is zero or the lifetime mod is disabled
         *  (fds_tmgr#cfg#en_lifetime), the lifetime of Options Templates is disabled.
         */
        uint32_t lifetime_opts;
        /**
         * \brief Lifetime of valid historical templates (in seconds)
         * \note If the values is zero or the access to history is disabled
         *   (fds_tmgr#cfg#en_history_access), only the last snapshot is accessible and valid.
         */
        uint16_t lifetime_snapshot;
    } limits; /**< Timeouts */

    struct {
        /** Pointer to the oldest snapshot in the manager */
        struct fds_tsnapshot *newest;
        /** Pointer to the newest snapshot in the manager */
        struct fds_tsnapshot *oldest;
        /**
         * Pointer to the currently selected "working" snapshot (based on current time i.e.
         *  fds_tmgr#time_now)
         */
        struct fds_tsnapshot *current;
    } list; /**< Link to snapshots in a linked-list */

    struct {
        /** Type of session */
        enum fds_session_type session_type;
        /** Make historical snapshots available (only for unreliable communication) */
        bool en_history_access;
        /** Allow modification of historical snapshots and propagation of these changes */
        bool en_history_mod;
        /** Selected Template Withdrawal mode */
        enum withdraw_mod_e withdraw_mod;
    } cfg; /**< Behaviour configuration */

    /** Database of IPFIX Information Elements  */
    const fds_iemgr_t *ies_db;

    /** Garbage ready to throw away (old unreachable templates/snapshots/etc.) */
    fds_tgarbage_t *garbage;
};

/**
 * \brief Maximum of two integers
 * \param[in] a First integer
 * \param[in] b Second integer
 * \return Maximum
 */
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/**
 * \brief Compare snapshot timestamps (with timestamp wraparound support)
 * \param t1 First timestamp
 * \param t2 Second timestamp
 * \return  The  function  returns an integer less than, equal to, or greater than zero if the
 *   first timestamp \p t1 is found, respectively, to be less than, to match, or be greater than
 *   the second timestamp.
 */
static inline int
mgr_time_cmp(uint32_t t1, uint32_t t2)
{
    if (t1 == t2) {
        return 0;
    }

    if ((t1 - t2) & 0x80000000) { // test the "sign" bit
        return (-1);
    } else {
        return 1;
    }
}

/**
 * \brief Check if timestamp \p t1 is before \p t2
 * \param[in] t1 First timestamp
 * \param[in] t2 Second timestamp
 * \return True or false
 */
#define TIME_LT(t1, t2) (mgr_time_cmp((t1), (t2)) < 0)

/**
 * \brief Check if timestamp \p t1 is before \p t2 or the same
 * \param[in] t1 First timestamp
 * \param[in] t2 Second timestamp
 * \return True or false
 */
#define TIME_LE(t1, t2) (mgr_time_cmp((t1), (t2)) <= 0)

/**
 * \brief Check if timestamp \p t1 is the same as \p t2
 * \param[in] t1 First timestamp
 * \param[in] t2 Second timestamp
 * \return True or false
 */
#define TIME_EQ(t1, t2) ((t1) == (t2))

/**
 * \brief Check if timestamp \p t1 is NOT the same as \p t2
 * \param[in] t1 First timestamp
 * \param[in] t2 Second timestamp
 * \return True or false
 */
#define TIME_NE(t1, t2) ((t1) != (t2))

/**
 * \brief Check if timestamp \p t1 is after \p t2 or the same
 * \param[in] t1 First timestamp
 * \param[in] t2 Second timestamp
 * \return True or false
 */
#define TIME_GE(t1, t2) (mgr_time_cmp((t1), (t2)) >= 0)

/**
 * \brief Check if timestamp \p t1 is after \p t2
 * \param[in] t1 First timestamp
 * \param[in] t2 Second timestamp
 * \return True or false
 */
#define TIME_GT(t1, t2) (mgr_time_cmp((t1), (t2)) > 0)


/**
 * \brief Insert a new snapshot into hierarchy (as a newer snapshot)
 *
 * The new snapshot \p snap will be inserted into hierarchy as successor of the \p anchor. Global
 * pointers of the appropriate template manager will be modified if the \p new is the newest
 * snapshot in the manager.
 * \warning Only new snapshots can be inserted into a new position. Changing position of any
 *   snapshot, that is a part of the manager, is NOT permitted because the template's destruction
 *   mechanism depends on preserved order.
 * \param[in] anchor Snapshot that will be used as an anchor
 * \param[in] new    Snapshot to insert
 */
static inline void
mgr_link_newer(struct fds_tsnapshot *anchor, struct fds_tsnapshot *new)
{
    // Preserve time order
    assert(TIME_LE(anchor->start_time, new->start_time));
    assert(!anchor->link.newer || TIME_GT(anchor->link.newer->start_time, new->start_time));
    // Position of a snapshot cannot be changed. Only new snapshots can be placed into hierarchy.
    assert(new->link.older == NULL && new->link.newer == NULL);

    new->link.mgr = anchor->link.mgr;

    if (anchor->link.newer) {
        struct fds_tsnapshot *tmp = anchor->link.newer;
        new->link.newer = tmp;
        tmp->link.older = new;
    } else {
        struct fds_tmgr *mgr = anchor->link.mgr;
        assert(anchor == mgr->list.newest);
        mgr->list.newest = new;
    }

    anchor->link.newer = new;
    new->link.older = anchor;
}

/**
 * \brief Insert a new snapshot into hierarchy (as older snapshot)
 *
 * The new snapshot \p snap will be inserted into hierarchy as predecessor of \p anchor. Global
 * pointers of the appropriate template manager will be modified if the \p new is the oldest
 * snapshot in the manager.
 * \warning Only new snapshots can be inserted into a new position. Changing position of any
 *   snapshot, that is a part of the manager, is NOT permitted because the template's destruction
 *   mechanism depends on preserved order.
 * \param[in] anchor Snapshot that will be used as an anchor
 * \param[in] new    Snapshot to insert
 */
static inline void
mgr_link_older(struct fds_tsnapshot *anchor, struct fds_tsnapshot *new)
{
    // Preserve time order
    assert(TIME_GT(anchor->start_time, new->start_time));
    assert(!anchor->link.older || TIME_LE(anchor->link.older->start_time, new->start_time));
    // Position of a snapshot cannot be changed. Only new snapshots can be placed into hierarchy.
    assert(new->link.older == NULL && new->link.newer == NULL);

    new->link.mgr = anchor->link.mgr;

    if (anchor->link.older) {
        struct fds_tsnapshot *tmp = anchor->link.older;
        tmp->link.newer = new;
        new->link.older = tmp;
    } else {
        struct fds_tmgr *mgr = anchor->link.mgr;
        assert(anchor == mgr->list.oldest);
        mgr->list.oldest = new;
    }

    new->link.newer = anchor;
    anchor->link.older = new;
}

/**
 * \brief Create a new empty snapshot
 *
 * Snapshot will be in editable mode and will be placed into the snapshot hierarchy as the oldest
 * snapshot.
 * \param[in] time Start time of validity
 * \return Pointer or NULL (memory allocation error)
 */
static struct fds_tsnapshot *
mgr_snap_create(struct fds_tmgr *mgr, uint32_t time)
{
    struct fds_tsnapshot *snap = snapshot_create();
    if (!snap) {
        return NULL;
    }

    snap->editable = true;
    snap->start_time = time;
    snap->link.older = NULL;
    snap->link.newer = NULL;

    // Place into hierarchy
    if (mgr->list.oldest) {
        assert(TIME_GT(mgr->list.oldest->start_time, time));
        mgr_link_older(mgr->list.oldest, snap);
        assert(mgr->list.oldest == snap);
    } else {
        // List is empty...
        mgr->list.oldest = snap;
        mgr->list.newest = snap;
        snap->link.mgr = mgr;
    }

    return snap;
}

/**
 * \brief Callback function that will free all templates with set "Delete" flag
 * \param[in] rec  Snapshot record
 * \param[in] data Not used
 * \return Always true.
 */
static bool
mgr_snap_destroy_cb(struct snapshot_rec *rec, void *data)
{
    (void) data; // Not used...
    if (rec->flags & SNAPSHOT_TF_DESTROY) {
        fds_template_destroy(rec->ptr);
    }
    return true;
}

/**
 * \brief Destroy a snapshot
 *
 * The function will iterate over all references to templates and it will free all templates
 * with set "Delete flag". Other templates will stay untouched. This function is suitable as
 * a callback for a garbage collector.
 * \warning Data will be freed immediately. Nothing is passed to a garbage collector.
 * \param[in] snap Snapshot to destroy
 */
static void
mgr_snap_destroy(struct fds_tsnapshot *snap)
{
    // Free all templates with the "Delete flag"
    snapshot_rec_for(snap, &mgr_snap_destroy_cb, NULL);
    snapshot_destroy(snap);
}

/**
 * \brief Try to move "Delete" flag
 *
 * If possible, the flag will be moved to the first snapshot older (predecessor) than this one
 * that has a reference to a template with the given ID.
 * \param[in] snap Snapshot
 * \param[in] id   Template ID
 * \return On success returns #FDS_OK. If the other snapshot is not found, the function will
 *   return #FDS_ERR_NOTFOUND and the flag will not be changed.
 */
static int
mgr_snap_dflag_move(struct fds_tsnapshot *snap, uint16_t id)
{
    // Make sure that the snapshot has the "Delete" flag
    struct snapshot_rec *snap_rec = snapshot_rec_find(snap, id);
    assert((snap_rec->flags & SNAPSHOT_TF_DESTROY) != 0);

    if (snap_rec->flags & SNAPSHOT_TF_CREATE) {
        // No one in the past can have a reference to this template
        return FDS_ERR_NOTFOUND;
    }

    struct fds_tsnapshot *ancestor = snap->link.older;
    struct snapshot_rec *ancestor_rec;
    while (ancestor != NULL) {
        ancestor_rec = snapshot_rec_find(ancestor, id);
        if (!ancestor_rec || ancestor_rec->ptr != snap_rec->ptr) {
            /* This snapshot doesn't have a pointer to the template or the pointer is different
             * due to history modification. We have to skip to the next ancestor.
             */
            ancestor = ancestor->link.older;
            continue;
        }

        // We found the snapshot
        break;
    }

    if (!ancestor) {
        // Not found
        return FDS_ERR_NOTFOUND;
    }

    // Transfer the flag
    snap_rec->flags &= ~SNAPSHOT_TF_DESTROY;
    ancestor_rec->flags |= SNAPSHOT_TF_DESTROY;
    return FDS_OK;
}

/**
 * \brief Try to pass "Delete" flag to a predecessor.
 *
 * The first predecessor with the reference to the template will be selected.
 * \param[in] rec  Snapshot record
 * \param[in] data Pointer to the snapshot from which the record comes
 * \return Always true
 */
static bool
mgr_snap_remove_cb(struct snapshot_rec *rec, void *data)
{
    struct fds_tsnapshot *snap = data;
    assert(snapshot_rec_find(snap, rec->id) == rec);

    // Process only records with "Delete" flag
    if (rec->flags & SNAPSHOT_TF_DESTROY) {
        /* Try to move the flag. It can remain here, if this is the last reference among snapshots
         * in the template manager and the template will be destroyed together with this snapshot.
         */
        mgr_snap_dflag_move(snap, rec->id);
    }

    return true;
}

// warning: this can remove pointer to the "current" snapshot selected by the "set time" function

/**
 * \brief Remove the snapshot
 *
 * Try to move ownership of "Delete" flags to predecessors, remove the snapshot from the hierarchy
 * (update links) and add this snapshot to the garbage.
 * \warning If the snapshot is "current" snapshot of the manager, the value will be set to NULL.
 * \param[in] snap Snapshot
 */
static void
mgr_snap_remove(struct fds_tsnapshot *snap)
{
    // Try to move all "Delete" flags to older (but still valid) snapshots in the manager
    snapshot_rec_for(snap, &mgr_snap_remove_cb, snap);

    // Update local and global pointers
    struct fds_tmgr *mgr = snap->link.mgr;

    if (snap->link.newer) {
        snap->link.newer->link.older = snap->link.older;
    } else {
        // This is the newest snapshot in the manager
        assert(mgr->list.newest == snap);
        mgr->list.newest = snap->link.older;
    }

    if (snap->link.older) {
        snap->link.older->link.newer = snap->link.newer;
    } else {
        // This is the oldest snapshot in the manager
        assert(mgr->list.oldest == snap);
        mgr->list.oldest = snap->link.newer;
    }

    if (mgr->list.current == snap) {
        mgr->list.current = NULL;
    }

    // Clear pointers (just for sure)
    snap->link.newer = snap->link.older = NULL;
    snap->link.mgr = NULL;

    // Move the snapshot to the garbage
    garbage_fn_t delete_fn = (garbage_fn_t) &mgr_snap_destroy;
    garbage_append(mgr->garbage, snap, delete_fn);
}

/**
 * \brief Clear "Create flag" of a snapshot record (callback function)
 * \param[in] rec  Snapshot record
 * \param[in] data Not used
 * \return Always true
 */
static bool
mgr_snap_clone_cflag_cb(struct snapshot_rec *rec, void *data)
{
    (void) data;
    rec->flags &= ~SNAPSHOT_TF_CREATE;
    return true;
}

/**
 * \brief Clear "Delete flag" of a snapshot record (callback function)
 * \param[in] rec  Snapshot record
 * \param[in] data Not used
 * \return Always true
 */
static bool
mgr_snap_clone_dflag_cb(struct snapshot_rec *rec, void *data)
{
    (void) data;
    rec->flags &= ~SNAPSHOT_TF_DESTROY;
    return true;
}

/** Auxiliary structure for mgr_snap_clone_remove_exp_cb() callback function */
struct mgr_snap_clone_remove_exp {
    /** Old snapshot         */
    struct fds_tsnapshot *old;
    /** New snapshot (clone) */
    struct fds_tsnapshot *new;

    /** New minimal lifetime value (calculated)              */
    uint32_t lifetime_min;
    /** True, if at least one template has enabled timeout   */
    bool lifetime_enabled;
};

/**
 * \brief Expired snapshot record checker (callback function)
 *
 * Check if a snapshot record has expired in the new snapshot and remove it. The function makes
 * sure that "Delete" flag will be moved back to the old snapshot.
 * \param[in] rec  Snapshot record
 * \param[in] data Structure mgr_snap_clone_remove_exp
 * \return Always true
 */
static bool
mgr_snap_clone_remove_exp_cb(struct snapshot_rec *rec, void *data)
{
    struct mgr_snap_clone_remove_exp *info = data;

    if ((rec->flags & SNAPSHOT_TF_TIMEOUT) == 0) {
        // Template doesn't have timeout
        assert(TIME_EQ(rec->ptr->time.last_seen, rec->ptr->time.end_of_life)); // Must be the same
        return true;
    }

    // The record is from the new snapshot
    assert(snapshot_rec_find(info->new, rec->id) == rec);
    // Check that lifetime is really enabled
    assert(TIME_NE(rec->ptr->time.last_seen, rec->ptr->time.end_of_life)); // Must be different

    if (TIME_GE(rec->lifetime, info->new->start_time)) {
        // Template is still valid, calculate new minimal lifetime
        if (TIME_LT(rec->lifetime, info->lifetime_min)) {
            info->lifetime_min = rec->lifetime;
        }

        info->lifetime_enabled = true;
        return true;
    }

    // Remove the record
    if (rec->flags & SNAPSHOT_TF_DESTROY) {
        // Move "Delete" flag back
        struct snapshot_rec *old_rec = snapshot_rec_find(info->old, rec->id);
        assert(old_rec != NULL);
        old_rec->flags |= SNAPSHOT_TF_DESTROY;
    }
    snapshot_rec_remove(info->new, rec->id);
    return true;
}

/**
 * \brief Create a clone of a snapshot and insert it into hierarchy
 *
 * Create a copy of the source snapshot and move all "Delete" flags from the source to the clone.
 * A start time of the clone MUST be the same or greater than a start time of the source snapshot.
 * If the start time of the clone is different, all expired snapshots will NOT be part of the newly
 * created clone.
 *
 * The clone will be inserted into hierarchy as a successor of the source snapshot.
 * \warning If the source snapshot already has a successor, the start time of the clone MUST be
 *   less than a start time of the old successor. Otherwise order could not be preserved!
 * \note The function doesn't check editability of the source snapshot, because this method is,
 *   among other things, necessary for creating a snapshot between two historical snapshots
 *   when at least one template has expired.
 *
 * \param[in]  src   Source snapshot
 * \param[out] dst   New snapshot
 * \param[in]  start Start time of the new snapshot (the same or greater)
 * \return #FDS_OK or #FDS_ERR_NOMEM.
 */
static int
mgr_snap_clone(struct fds_tsnapshot *src, struct fds_tsnapshot **dst, uint32_t start)
{
    // Only closed snapshots can be cloned
    assert(src->editable == false);
    // We cannot add copy of this snapshots into the history
    assert(TIME_LE(src->start_time, start));
    // Check history consistency
    assert(!src->link.newer || TIME_GT(src->link.newer->start_time, start));

    // Make a copy of the snapshot
    struct fds_tsnapshot *new_snap = snapshot_copy(src);
    if (!new_snap) {
        return FDS_ERR_NOMEM;
    }

    new_snap->editable = true;
    new_snap->start_time = start;
    new_snap->link.newer = NULL;
    new_snap->link.older = NULL;

    // Insert into snapshot hierarchy
    mgr_link_newer(src, new_snap);

    /*
     * Transfer ownership of templates i.e. old snapshot records will not have "Delete" flag
     * anymore. Also remove "Create" flags that must remain in the parent.
     */
    snapshot_rec_for(src, &mgr_snap_clone_dflag_cb, NULL);
    snapshot_rec_for(new_snap,  &mgr_snap_clone_cflag_cb, NULL);

    // Check if there is a template that has expired...
    fds_tmgr_t *mgr = src->link.mgr;
    if (TIME_NE(src->start_time, start) && src->lifetime.enabled
            && TIME_LE(src->lifetime.min_value, start)) {
        // Remove expired templates and calculate new lifetime
        const uint32_t max_timeout = MAX(mgr->limits.lifetime_normal, mgr->limits.lifetime_opts);
        const uint32_t max_lifetime = new_snap->start_time + max_timeout;

        struct mgr_snap_clone_remove_exp data = {src, new_snap, max_lifetime, false};
        snapshot_rec_for(new_snap, mgr_snap_clone_remove_exp_cb, &data);

        new_snap->lifetime.enabled = data.lifetime_enabled;
        new_snap->lifetime.min_value = data.lifetime_min + 1;
    }

    /* TODO: optimization enable/disable???
    if (src->start_time == start) {
        // Move the source snapshot into the garbage  because it cannot be accessible anymore
        mgr_snap_remove(src);
    }
    */

    *dst = new_snap;
    return FDS_OK;
}

/**
 * \brief Make a snapshot editable
 *
 * If the snapshot is editable, the function will return the original snapshot. If the snapshot
 * is NOT editable, a new clone of the snapshot will be created unless this is a historical
 * snapshot and history modification is disabled.
 *
 * \param[in] src Source snapshot
 * \param[in] dst Editable snapshot (the same as the source or a new clone)
 * \return #FDS_OK on success. In case of memory allocation error returns #FDS_ERR_NOMEM.
 *   If modification is not permitted, returns #FDS_ERR_DENIED.
 */
static int
mgr_snap_edit(struct fds_tsnapshot *src, struct fds_tsnapshot **dst)
{
    if (src->link.newer == NULL) {
        // This is the newest snapshot
        if (src->editable) {
            // It is already editable -> nothing to do
            *dst = src;
            return FDS_OK;
        }

        // Return a copy
        return mgr_snap_clone(src, dst, src->start_time);
    }

    // We would like to make the historical snapshot editable
    if (src->link.mgr->cfg.en_history_mod == false) {
        // Modification of history is not permitted!
        return FDS_ERR_DENIED;
    }

    if (src->editable) {
        // History modification is enabled and the snapshot is already editable
        *dst = src;
        return FDS_OK;
    }

    // Return a copy
    return mgr_snap_clone(src, dst, src->start_time);
}

/**
 * \brief Add a reference to a template into a snapshot (auxiliary function)
 *
 * This will create a new snapshot record with the reference to the template and user defined flags.
 * \note Expiration of the snapshot (minimal lifetime) will be recalculated based on a validity
 *   of the template, if necessary.
 * \note Flag ::SNAPSHOT_TF_TIMEOUT will be set automatically if the fds_template#time#last_seen
 *   and fds_template#time#end_of_life are different.
 * \warning All fds_template#time variables must be already set!
 * \param[in] snap  Snapshot
 * \param[in] tmplt Template
 * \param[in] flags Flags of the snapshot record
 * \return #FDS_OK or #FDS_ERR_NOMEM
 */
static int
mgr_snap_template_add_ref(struct fds_tsnapshot *snap, struct fds_template *tmplt, uint16_t flags)
{
    // Do NOT overwrite template references
    assert(snapshot_rec_find(snap, tmplt->id) == NULL);

    // This flag should be set only by this function
    assert((flags & SNAPSHOT_TF_TIMEOUT) == 0);

    if (TIME_NE(tmplt->time.last_seen, tmplt->time.end_of_life)) {
        // Timeout of this template is enabled
        assert(TIME_LT(tmplt->time.last_seen, tmplt->time.end_of_life));
        flags |= SNAPSHOT_TF_TIMEOUT;
        const uint32_t invalid_time = tmplt->time.end_of_life + 1;

        if (!snap->lifetime.enabled) {
            snap->lifetime.enabled = true;
            snap->lifetime.min_value = invalid_time;
        } else {
            if (TIME_LT(invalid_time, snap->lifetime.min_value)) {
                snap->lifetime.min_value = invalid_time;
            }
        }

        assert(TIME_GT(snap->lifetime.min_value, snap->start_time));
    }

    // Add the template to the snapshot
    struct snapshot_rec new_rec;
    new_rec.id = tmplt->id;
    new_rec.flags = flags;
    new_rec.lifetime = tmplt->time.end_of_life;
    new_rec.ptr = tmplt;

    return snapshot_rec_add(snap, &new_rec);
}

// Function prototype
static int
mgr_snap_template_remove(struct fds_tsnapshot *snap, uint16_t id);

/**
 * \brief Add a template to a snapshot
 *
 * Based on the session type, try to add the template in the snapshot. References to
 * IE definitions will be added to template's elements and the lifetime of the template will
 * be configured appropriately to the manager configuration.
 *
 * \note In case of adding the template into a historical snapshot, the template will not be
 *   immediately propagated to the future snapshot. Changes will be propagated on the snapshot
 *   freeze, see mgr_snap_freeze().
 * \note In case of withdrawing the template in a historical snapshot, the modification
 *   will be propagated immediately. If any future snapshot is not editable, a new copy will
 *   be created.
 *
 * \warning The snapshot MUST be editable! The function does NOT check historical editability.
 * \param[in] snap  Snapshot
 * \param[in] tmplt Template to be added
 * \return On success returns #FDS_OK and the manager will take responsibility for the template.
 *   In case of any following error, the template will not be part of the manager and it must
 *   be freed by the user. On memory allocation error returns #FDS_ERR_NOMEM. If the operation
 *   is not allowed for this template and session combination returns #FDS_ERR_DENIED.
 */
static int
mgr_snap_template_add(struct fds_tsnapshot *snap, struct fds_template *tmplt)
{
    // We can modify only editable snapshots
    assert(snap->editable == true);
    assert(tmplt->type == FDS_TYPE_TEMPLATE || tmplt->type == FDS_TYPE_TEMPLATE_OPTS);
    // This function doesn't support templates withdrawal
    assert(tmplt->fields_cnt_total != 0);

    int ret_code;
    struct fds_tmgr *mgr = snap->link.mgr;

    // Is a template with the same ID already in the snapshot?
    bool is_refresh = false;
    struct snapshot_rec *snap_rec = snapshot_rec_find(snap, tmplt->id);
    if (snap_rec != NULL) {
        is_refresh = (fds_template_cmp(snap_rec->ptr, tmplt) == 0);
        if (!is_refresh && mgr->cfg.withdraw_mod == WITHDRAW_REQUIRED) {
           /* The template tries to replace a different template with the same ID, but a template
            * withdrawal is required first.
            */
            return FDS_ERR_DENIED;
        }
    }

    struct fds_template *tmplt2add;
    if (is_refresh) {
        /* We received exactly the same template, so we can just copy the old one and preserve
         * first seen timestamp, Flow key and other settings...
         */
        tmplt2add = fds_template_copy(snap_rec->ptr);
        if (!tmplt2add) {
            return FDS_ERR_NOMEM;
        }
    } else {
        // We received a new template or a template that is different then the previous one
        tmplt2add = tmplt;
        tmplt2add->time.first_seen = mgr->time_now;

        // Update IE definitions
        if ((ret_code = fds_template_ies_define(tmplt2add, mgr->ies_db, false)) != FDS_OK) {
            return ret_code;
        }
    }

    if (snap_rec != NULL) {
        // Remove the old template from the snapshot. This can eventually move "Delete flag"...
        mgr_snap_template_remove(snap, tmplt2add->id);
    }

    // Update timestamp info
    uint32_t lifetime = (tmplt2add->type == FDS_TYPE_TEMPLATE)
        ? mgr->limits.lifetime_normal
        : mgr->limits.lifetime_opts;
    tmplt2add->time.last_seen = mgr->time_now;
    tmplt2add->time.end_of_life = mgr->time_now + lifetime;

    // Add a reference of the template to the snapshot
    const uint16_t flags = SNAPSHOT_TF_CREATE | SNAPSHOT_TF_DESTROY; // First owner of the template
    if ((ret_code = mgr_snap_template_add_ref(snap, tmplt2add, flags)) != FDS_OK) {
        // Failed to add the record
        if (is_refresh) {
            fds_template_destroy(tmplt2add);
        }

        return ret_code;
    }

    if (is_refresh) {
        // Now everything is OK and we can safely free the copy of the refreshed template
        fds_template_destroy(tmplt);
    }

    return FDS_OK;
}

/**
 * \brief Remove a template from a snapshot
 *
 * First, try to move ownership of the template (i.e. "Delete" flag) to the newest predecessor
 * that has a reference to the same template. Finally, remove the template from the snapshot.
 * \warning The snapshot MUST be editable.
 * \param[in] snap Snapshot
 * \param[in] id   Template ID to remove
 * \return On success returns #FDS_OK. #FDS_ERR_NOTFOUND if the template is not present.
 */
static int
mgr_snap_template_remove(struct fds_tsnapshot *snap, uint16_t id)
{
    // Is the template is the snapshot
    struct snapshot_rec *snap_rec = snapshot_rec_find(snap, id);
    if (!snap_rec) {
        return FDS_ERR_NOTFOUND;
    }

    // Make sure that the snapshot is editable
    assert(snap->editable);

    if (snap->lifetime.enabled && snap->rec_cnt == 1) {
        // The last record in the snapshot -> disable lifetime
        snap->lifetime.enabled = false;
    }

    if ((snap_rec->flags & SNAPSHOT_TF_DESTROY) == 0) {
        // The snapshot is not responsible for destruction of the template -> remove the record
        return snapshot_rec_remove(snap, id);
    }

    // We have the "Delete" flag
    if (snap_rec->flags & SNAPSHOT_TF_CREATE) {
        /* We have the "Create" and "Delete" flags at the same time.
         * The template has been added to this snapshot and immediately we want to remove it.
         * In other words, this snapshot hasn't been frozen yet, thus, no one can have a reference
         * to the template (we can directly free the template).
         */
        fds_template_destroy(snap_rec->ptr);
        return snapshot_rec_remove(snap, id);
    }

    /* Let's try to move the delete flag to the first snapshot in the past that also has
     * a reference to this snapshot.
     */
    if (mgr_snap_dflag_move(snap, id) == FDS_ERR_NOTFOUND) {
        // This snapshot has the last reference -> move the template to the garbage
        fds_tgarbage_t *gc = snap->link.mgr->garbage;
        garbage_fn_t delete_fn = (garbage_fn_t) &fds_template_destroy;
        garbage_append(gc, snap_rec->ptr, delete_fn);
    }

    return snapshot_rec_remove(snap, id);
}

/**
 * \brief Withdraw a template from a snapshot
 *
 * In case of withdrawing the template in a historical snapshot, the modification will be
 * propagated immediately. If any future snapshot is not editable, a new copy with modification
 * will be created and the old one will not be accessible for seek operation anymore.
 *
 * \warning The snapshot MUST be editable!
 * \note If \p type is ::FDS_TYPE_TEMPLATE_UNDEF, type check is skipped.
 * \param[in] snap Snapshot
 * \param[in] id   Template ID to withdraw
 * \param[in] type Expected type of the template
 * \return #FDS_OK on success.
 *   #FDS_ERR_NOMEM, if a memory allocation error has occurred.
 *   #FDS_ERR_NOTFOUND. if the template is not present in the snapshot.
 *   #FDS_ERR_DENIED, if the operation is not permitted for this session.
 *   #FDS_ERR_ARG, if \p type of the template does NOT match.
 */
static int
mgr_snap_template_withdraw(struct fds_tsnapshot *snap, uint16_t id, enum fds_template_type type)
{
    struct fds_tmgr *mgr = snap->link.mgr;
    assert(mgr->cfg.withdraw_mod != WITHDRAW_PROHIBITED);

    if (snap->link.newer != NULL && mgr->cfg.en_history_mod == false) {
        // We try to modify a historical snapshot, but this operation is disabled for this session
        return FDS_ERR_DENIED;
    }

    struct snapshot_rec *snap_rec = snapshot_rec_find(snap, id);
    if (!snap_rec) {
        // The template not found
        return FDS_ERR_NOTFOUND;
    }

    // Check the template type
    if (type != FDS_TYPE_TEMPLATE_UNDEF && type != snap_rec->ptr->type) {
        // Type of withdrawal request doesn't match the type of the template in the snapshot
        return FDS_ERR_ARG;
    }

    // Make sure that the snapshot is editable
    assert(snap->editable);

    // Remove a reference to the template from this snapshot and all its future descendants
    int ret_code;

    for (struct fds_tsnapshot *ptr = snap; ptr != NULL; ptr = ptr->link.newer) {
        struct snapshot_rec *rec = snapshot_rec_find(ptr, id);
        if (!rec) {
            /* Snapshot doesn't have a reference to the template, but there can be still anyone
             * else in the future due to history modification that caused this "gap".
             */
            continue;
        }

        if (TIME_GT(rec->ptr->time.last_seen, mgr->time_now)) {
            /* We found a new future definition of the template that we want to remove. We have
             * to stop here because we cannot remove template from the future by withdrawal request
             * from the past.
             */
            break;
        }

        // Warning: we cannot change the order of these 2 operations!
        if (ptr->link.newer != NULL && TIME_EQ(ptr->link.newer->start_time, ptr->start_time)) {
            /* This snapshot has the same start time as its descendant. Because this node is not
             * accessible for seek operation anymore (it is hidden by the descendant), we don't
             * have to modify it.
             */
            continue;
        }

        // Make sure that the snapshot is editable and remove the template
        if ((ret_code = mgr_snap_edit(ptr, &ptr)) != FDS_OK) {
            return ret_code;
        }


        if ((ret_code = mgr_snap_template_remove(ptr, id)) != FDS_OK) {
            return ret_code;
        }
    }

    return FDS_OK;
}

/** \brief Auxiliary function for mgr_snap_freeze_cb() */
struct mgr_snap_freeze {
    /** Snapshot                    */
    struct fds_tsnapshot *snap;
    /** Operation result            */
    int ret_code;
};

/**
 * \brief Propagate newly created templates to descendants (auxiliary function)
 *
 * \param[in] rec  Snapshot record
 * \param[in] data Callback data
 * \return True on success. On error returns false and a new return code is set.
 */
static bool
mgr_snap_freeze_cb(struct snapshot_rec *rec, void *data)
{
    struct mgr_snap_freeze *info = data;
    struct fds_tmgr *mgr = info->snap->link.mgr;
    assert(snapshot_rec_find(info->snap, rec->id) == rec);

    /* We are only interested into records that has been added into this snapshot (i.e. with
     * "Create" flag)
     */
    if ((rec->flags & SNAPSHOT_TF_CREATE) == 0) {
        // Skip
        return true;
    }

    /* Because the snapshot hasn't been frozen yet, the added template (with "Create" flag) cannot
     * be referenced by any other snapshot, therefore, there MUST be also present "Delete" flag.
     */
    assert((rec->flags & SNAPSHOT_TF_DESTROY) != 0);
    // We are trying to modify history, make sure that we have rights...
    assert(mgr->cfg.en_history_mod);

    // Remember the last successful insertion
    struct fds_tsnapshot *last_insert = NULL;
    // We would like to propagate this template to all descendants
    for (struct fds_tsnapshot *dsc = info->snap->link.newer; dsc != NULL; dsc = dsc->link.newer) {
        // Is the template still valid in this snapshot (and descendants)?
        if ((rec->flags & SNAPSHOT_TF_TIMEOUT) != 0 && TIME_LT(rec->lifetime, dsc->start_time)) {
            // Stop propagation
            break;
        }

        // Does the descent's snapshot has a template with the same ID?
        struct snapshot_rec *dsc_rec = snapshot_rec_find(dsc, rec->id);
        if (dsc_rec != NULL) {
            const uint32_t dsc_seen = dsc_rec->ptr->time.last_seen;
            const uint32_t rec_seen = rec->ptr->time.last_seen;

            if (TIME_LT(rec_seen, dsc_seen)) {
                /* The descendant's template is newer. We cannot rewrite the newer template
                 * with a template from the past. Stop propagation...
                 */
                break;
            }
        }

        if (dsc->link.newer != NULL && TIME_EQ(dsc->link.newer->start_time, dsc->start_time)) {
            /* This snapshot has the same start time as its descendant. Because this node is not
             * accessible for seek operation anymore (it is hidden by the descendant), we don't
             * have to modify it.
             */
            continue;
        }

        // Make sure that the snapshot is editable and add/replace the template
        int ret_code;
        if ((ret_code = mgr_snap_edit(dsc, &dsc)) != FDS_OK) {
            info->ret_code = ret_code;
            return false;
        }

        if (dsc_rec != NULL && (ret_code = mgr_snap_template_remove(dsc, dsc_rec->id)) != FDS_OK) {
            // Failed to remove the template
            info->ret_code = ret_code;
            return false;
        }

        // Add a reference to the template into this snapshot. This time without any flags!
        if ((ret_code = mgr_snap_template_add_ref(dsc, rec->ptr, 0)) != FDS_OK) {
            info->ret_code = ret_code;
            return false;
        }

        last_insert = dsc;
    }

    if (last_insert != NULL) {
        /* Move the "Delete" flag (responsibility to destroy the template) to the last modified
         * snapshot.
         */
        struct snapshot_rec *last_rec = snapshot_rec_find(last_insert, rec->id);
        assert(last_rec->ptr == rec->ptr);
        assert((last_rec->flags & (SNAPSHOT_TF_CREATE | SNAPSHOT_TF_DESTROY)) == 0);

        last_rec->flags |= SNAPSHOT_TF_DESTROY;
        rec->flags &= ~SNAPSHOT_TF_DESTROY;
    }

    return true;
}

/**
 * \brief Freeze a snapshot (disable modifications)
 *
 * If the snapshot is historical (there is at last one newer snapshot) and it includes newly added
 * templates, the templates will be propagated to future snapshots.
 *
 * \param[in] snap Snapshot
 * \return #FDS_OK on success. #FDS_ERR_NOMEM, if a memory allocation error has occurred.
 */
static int
mgr_snap_freeze(struct fds_tsnapshot *snap)
{
    if (snap->editable == false) {
        // Nothing to do
        return FDS_OK;
    }

    snap->editable = false;
    if (!snap->link.newer) {
        // This is the newest snapshot -> we don't have to propagate changes
        return FDS_OK;
    }

    /* This is historical snapshot with modification.
     * We have to make sure that all newly added templates (i.e. with "Create" flag) will be
     * propagated into newer snapshots. Template withdrawals are propagated differently, so we
     * don't have to take care of them now.
     */
    struct mgr_snap_freeze data;
    data.snap = snap;
    data.ret_code = FDS_OK;
    snapshot_rec_for(snap, &mgr_snap_freeze_cb, &data);

    /* TODO: enable/disable this optimization
    if (snap->link.older != NULL && snap->link.older->start_time == snap->start_time) {
        mgr_snap_remove(snap->link.older);
    };
    */

    return data.ret_code;
}

/**
 * \brief Hierarchy cleanup
 *
 * Move old and inaccessible snapshots into a garbage collector. At least one snapshot (the newest
 * one) will always survive.
 * \warning The pointer to current snapshot i.e. fds_tmgr#list#current will be set to NULL.
 * \param[in] mgr Template manager
 */
static void
mgr_cleanup(struct fds_tmgr *mgr)
{
    if (mgr->list.newest == NULL) {
        // The manager is empty
        assert(mgr->list.oldest == NULL);
        return;
    }

    // Optimization
    if (!mgr->cfg.en_history_access || mgr->limits.lifetime_snapshot == 0) {
        // Remove all snapshots except the newest one...
        garbage_fn_t delete_fn = (garbage_fn_t) &mgr_snap_destroy;
        struct fds_tsnapshot *next = mgr->list.newest->link.older;
        while (next) {
            struct fds_tsnapshot *ptr = next;
            next = next->link.older;
            garbage_append(mgr->garbage, ptr, delete_fn);

            // Clear pointers (just for sure)
            ptr->link.newer = ptr->link.older = NULL;
            ptr->link.mgr = NULL;
        }

        // Remove all links from the newest
        mgr->list.newest->link.older = NULL;
        assert(mgr->list.newest->link.newer == NULL);

        // Modify global pointers
        mgr->list.oldest = mgr->list.newest;
        mgr->list.current = NULL;
        return;
    }

    // Proceed from the oldest to the newest snapshot
    struct fds_tsnapshot *next = mgr->list.oldest;
    const uint32_t newest_time = mgr->list.newest->start_time;

    while (next) {
        struct fds_tsnapshot *ptr = next;
        next = next->link.newer;

        // Make sure that the newest snapshot will remain
        if (ptr->link.newer == NULL) {
            break;
        }

        // Is this snapshot reachable for seek operation? (i.e. remove duplicities)
        if (TIME_EQ(ptr->start_time, ptr->link.newer->start_time)) {
            // No, it isn't because it is hidden by the newer with the same start time
            mgr_snap_remove(ptr);
            continue;
        }

        // Get the end time of validity of this snapshot
        assert(TIME_NE(ptr->start_time, ptr->link.newer->start_time));
        uint32_t end_time = ptr->link.newer->start_time - 1U;
        if (TIME_LT(end_time + mgr->limits.lifetime_snapshot, newest_time)) {
            // Historical snapshot is not valid anymore
            mgr_snap_remove(ptr);
            continue;
        }
    }

    mgr->list.current = NULL;
}

/**
 * \brief Seek for a snapshot (in the future)
 *
 * Try to find a valid snapshot that belongs to the \p time. If necessary (for example,
 * template timeout), the new snapshot will be created.
 *
 * \warning The current pointer MUST be set and \p time MUST the same or greater than the time
 *   of the current pointer.
 * \param[in] tmgr Template manager
 * \param[in] time Time required
 * \return On success returns #FDS_OK and the current pointer is set. Otherwise returns
 *   #FDS_ERR_NOMEM and the current pointer is undefined!
 */
static int
mgr_seek_forwards(fds_tmgr_t *tmgr, uint32_t time)
{
    assert(tmgr->list.current);

    int ret_code;
    struct fds_tsnapshot *snap = tmgr->list.current;

    // Find a snapshot where "start time" >= time and "end time" <= time
    while (snap) {
        // Make sure that we don't go too far
        assert(TIME_LE(snap->start_time, time));

        if (!snap->link.newer || TIME_GT(snap->link.newer->start_time, time)) {
            // This is the last snapshot or the last within the required range
            break;
        }

        // Move to the next snapshot, but make sure that all snapshots in the past are frozen
        if (snap->editable && (ret_code = mgr_snap_freeze(snap)) != FDS_OK) {
            tmgr->list.current = NULL;
            return ret_code;
        }

        snap = snap->link.newer;
    }

    if (TIME_EQ(snap->start_time, time)) {
        // We found the exact match -> return (possibly editable) snapshot
        tmgr->list.current = snap;
        return FDS_OK;
    }

    // We don't have exact match, therefore, the snapshot must be frozen first
    assert(TIME_LT(snap->start_time, time));
    if ((ret_code = mgr_snap_freeze(snap)) != FDS_OK) {
        tmgr->list.current = NULL;
        return ret_code;
    }

    if (snap->lifetime.enabled && TIME_LE(snap->lifetime.min_value, time)) {
        // At least one template will expire -> create a new snapshot without these templates
        if ((ret_code = mgr_snap_clone(snap, &snap, time)) != FDS_OK) {
            tmgr->list.current = NULL;
            return ret_code;
        }
    }

    tmgr->list.current = snap;
    return FDS_OK;
}

/**
 * \brief Seek for a snapshot (in the past)
 *
 * Try to find a valid snapshot that belongs to the \p time. If necessary (for example,
 * template timeout), the new snapshot will be created.
 *
 * \warning The current pointer MUST be set and \p time MUST the less than the time of the
 *   current pointer.
 * \param[in] tmgr Template manager
 * \param[in] time Time required
 * \return On success returns #FDS_OK and the current pointer is set. Otherwise returns
 *   #FDS_ERR_NOMEM and the current pointer is undefined!
 */
static int
mgr_seek_backwards(struct fds_tmgr *tmgr, uint32_t time)
{
    assert(tmgr->list.current && TIME_GT(tmgr->list.current->start_time, time));

    // Find a snapshot where "start time" >= time and "end time" <= time
    struct fds_tsnapshot *snap = tmgr->list.current->link.older; // Start from the previous snap
    while (snap) {
        // Because we are in the past, all snapshots must be frozen
        assert(snap->editable == false);

        // Is it the required snapshot?
        if (TIME_LE(snap->start_time, time)) {
            // This is the first snapshot in required range
            assert(TIME_GT(snap->link.newer->start_time, time));
            break;
        }

        snap = snap->link.older;
    }

    if (!snap) {
        // The snapshot not found! Create an empty one and insert it into the hierarchy
        snap = mgr_snap_create(tmgr, time);
        if (!snap) {
            tmgr->list.current = NULL;
            return FDS_ERR_NOMEM;
        }

        tmgr->list.current = snap;
        return FDS_OK;
    }

    // Snapshot found...
    if (snap->lifetime.enabled && TIME_LE(snap->lifetime.min_value, time)) {
        // At least one template will expire -> create a new snapshot without these templates
        assert(TIME_LT(snap->start_time, time));

        int ret_code;
        if ((ret_code = mgr_snap_clone(snap, &snap, time)) != FDS_OK) {
            tmgr->list.current = NULL;
            return ret_code;
        }
    }

    tmgr->list.current = snap;
    return FDS_OK;
}

fds_tmgr_t *
fds_tmgr_create(enum fds_session_type type)
{
    fds_tmgr_t *mgr = calloc(1, sizeof(*mgr));
    if (!mgr) {
        return NULL;
    }

    mgr->garbage = garbage_create();
    if (!mgr->garbage) {
        fds_tmgr_destroy(mgr);
        return NULL;
    }

    // Prepare configuration based on the type of a session
    mgr->limits.lifetime_snapshot = SNAPSHOT_DEF_LIFETIME;
    mgr->cfg.session_type = type;

    switch (type) {
    case FDS_SESSION_TCP:
        mgr->cfg.en_history_access = false; // All records MUST be send reliably
        mgr->cfg.en_history_mod = false;    // Everything is reliable -> no history required
        mgr->cfg.withdraw_mod = WITHDRAW_REQUIRED;
        break;
    case FDS_SESSION_UDP:
        /* By default, template timeouts are disabled (values are 0), user must manually specify
         * timeouts using fds_tmgr_set_udp_timeouts().
         */
        mgr->cfg.en_history_access = true;
        mgr->cfg.en_history_mod = true;
        mgr->cfg.withdraw_mod = WITHDRAW_PROHIBITED;
        break;
    case FDS_SESSION_SCTP:
        mgr->cfg.en_history_access = true; // Data records can be send unreliably
        /* Templates MUST be send reliably (ordered delivery), but they can be send on any SCTP
         * stream. Different streams can have different export times, therefore, history
         * modification must be enabled.
         */
        mgr->cfg.en_history_mod = true;
        mgr->cfg.withdraw_mod = WITHDRAW_REQUIRED;
        break;
    case FDS_SESSION_FILE:
        mgr->cfg.en_history_access = true;
        mgr->cfg.en_history_mod = true;
        mgr->cfg.withdraw_mod = WITHDRAW_OPTIONAL;
        break;
    }

    return mgr;
}

void
fds_tmgr_destroy(fds_tmgr_t *tmgr)
{
    // Destroy all snapshots and templates
    struct fds_tsnapshot *snap = tmgr->list.oldest;
    while (snap) {
        struct fds_tsnapshot *tmp = snap;
        snap = snap->link.newer;
        mgr_snap_destroy(tmp);
    }

    // Destroy garbage
    if (tmgr->garbage) {
        garbage_destroy(tmgr->garbage);
    }

    // Finally destroy the manager
    free(tmgr);
}

void
fds_tmgr_clear(fds_tmgr_t *tmgr)
{
    // Move all snapshots to garbage
    garbage_fn_t delete_fn = (garbage_fn_t) &mgr_snap_destroy;
    struct fds_tsnapshot *ptr = tmgr->list.oldest;

    while (ptr) {
        garbage_append(tmgr->garbage, ptr, delete_fn);
        ptr = ptr->link.newer;
    }

    // Modify global pointers
    tmgr->list.oldest = tmgr->list.newest = tmgr->list.current = NULL;
    tmgr->time_newest = tmgr->time_now = 0;
}

int
fds_tmgr_garbage_get(fds_tmgr_t *tmgr, fds_tgarbage_t **gc)
{
    mgr_cleanup(tmgr);

    // Set the current pointer again...
    int ret_code;
    if ((ret_code = fds_tmgr_set_time(tmgr, tmgr->time_now)) != FDS_OK) {
        return ret_code;
    }

    if (garbage_empty(tmgr->garbage)) {
        *gc = NULL;
        return FDS_OK;
    }

    fds_tgarbage_t *ret_val = tmgr->garbage;
    tmgr->garbage = garbage_create();
    if (!tmgr->garbage) {
        tmgr->garbage = ret_val;
        return FDS_ERR_NOMEM;
    }

    *gc = ret_val;
    return FDS_OK;
}

void
fds_tmgr_garbage_destroy(fds_tgarbage_t *gc)
{
    garbage_destroy(gc);
}

int
fds_tmgr_set_time(fds_tmgr_t *tmgr, uint32_t exp_time)
{
    // Let's go to the past, but...
    if (TIME_LT(exp_time, tmgr->time_now)) {
        if (tmgr->list.newest != NULL) {
            if (tmgr->cfg.en_history_access == false) {
                // The manager is not empty and history is disabled
                return FDS_ERR_DENIED;
            }

            if (TIME_LT(exp_time + tmgr->limits.lifetime_snapshot, tmgr->time_newest)) {
                // Do not allow going back more that snapshot lifetime
                return FDS_ERR_NOTFOUND;
            }
        } else {
            // The manager is empty (no snapshots)
            tmgr->time_newest = exp_time;
        }
    }

    tmgr->time_now = exp_time;
    if (TIME_GT(exp_time, tmgr->time_newest)) {
        tmgr->time_newest = exp_time;
    }

    if (!tmgr->list.current) {
        //  The current snapshot is unknown
        if (tmgr->list.oldest) {
            /* The manager is not empty, so we have to start seeking from the oldest possible
             * snapshot because in the manager can be a non-frozen snapshot without propagated
             * modifications.
             */
            assert(tmgr->list.newest);
            tmgr->list.current = tmgr->list.oldest;
        } else {
            // The manager is empty -> just add an empty snapshot
            assert(tmgr->list.oldest == NULL);
            assert(tmgr->list.newest == NULL);
            tmgr->list.current = mgr_snap_create(tmgr, exp_time);
            if (!tmgr->list.current) {
                return FDS_ERR_NOMEM;
            }

            return FDS_OK;
        }
    }

    if (TIME_LT(exp_time, tmgr->list.current->start_time)) {
        // Select snapshot in the past
        return mgr_seek_backwards(tmgr, exp_time);
    } else {
        // Select snapshot in the present or the future
        return mgr_seek_forwards(tmgr, exp_time);
    }
}

/**
 * \brief Prepare the current snapshot for modification
 *
 * If the snapshot is not editable, the function will create a new copy and update the pointer
 * to the current snapshot. In case of historical snapshot, it also makes sure that history
 * modification mode is enabled.
 * \warning The current snapshot MUST be set.
 * \param[in] tmgr Template manager
 * \return On success returns #FDS_OK. If history modification is NOT enabled and the snapshot
 *   is historical returns #FDS_ERR_DENIED. If a memory allocation error has occurred returns
 *   #FDS_ERR_NOMEM.
 */
static inline int
mgr_modify_prepare(struct fds_tmgr *tmgr)
{
    struct fds_tsnapshot *snap = tmgr->list.current;
    // Check history consistency
    assert(TIME_GE(tmgr->time_now, snap->start_time));
    assert(!snap->link.newer || TIME_LT(tmgr->time_now, snap->link.newer->start_time));

    if (snap->link.newer != NULL && tmgr->cfg.en_history_mod == false) {
        // We try to modify a historical snapshot, but this operation is disabled for this session
        return FDS_ERR_DENIED;
    }

    int ret_code;
    if (TIME_EQ(tmgr->time_now, snap->start_time) && snap->editable == false) {
        // Make the snapshot editable
        if ((ret_code = mgr_snap_edit(snap, &snap)) != FDS_OK) {
            return ret_code;
        }

        // Update the current snapshot
        tmgr->list.current = snap;
    } else if (TIME_GT(tmgr->time_now, snap->start_time)) {
        // We would like to add/withdraw/remove a template, but the start is in the past
        if ((ret_code = mgr_snap_clone(snap, &snap, tmgr->time_now)) != FDS_OK) {
            return ret_code;
        }

        // Update the current snapshot
        tmgr->list.current = snap;
    }

    return FDS_OK;
}


int
fds_tmgr_template_add(fds_tmgr_t *tmgr, struct fds_template *tmplt)
{
    if (!tmgr->list.current) {
        // Undefined snapshot
        return FDS_ERR_ARG;
    }

    if (tmplt->fields_cnt_total == 0) {
        // Templates withdrawals cannot be added
        return FDS_ERR_ARG;
    }

    int ret_code;
    if ((ret_code = mgr_modify_prepare(tmgr)) != FDS_OK) {
        return ret_code;
    }

    struct fds_tsnapshot *snap = tmgr->list.current;
    return mgr_snap_template_add(snap, tmplt);
}


int
fds_tmgr_template_withdraw(fds_tmgr_t *tmgr, uint16_t id, enum fds_template_type type)
{
    if (!tmgr->list.current) {
        // Undefined snapshot
        return FDS_ERR_ARG;
    }

    if (tmgr->cfg.withdraw_mod == WITHDRAW_PROHIBITED) {
        return FDS_ERR_DENIED;
    }

    int ret_code;
    if ((ret_code = mgr_modify_prepare(tmgr)) != FDS_OK) {
        return ret_code;
    }

    struct fds_tsnapshot *snap = tmgr->list.current;
    return mgr_snap_template_withdraw(snap, id, type);
}

/** \brief Auxiliary structure for mgr_template_withdraw_all_cb() */
struct mgr_template_withdrawal_all {
    /** Snapshot                    */
    struct fds_tsnapshot *snap;
    /** Type of templates to remove */
    enum fds_template_type type;

    /** Operation result            */
    int ret_code;
};

/**
 * \brief Apply withdrawal operation on a snapshot record (auxiliary function)
 * \param[in] rec  Snapshot record
 * \param[in] data Auxiliary data structure
 * \return On success returns true. Otherwise returns false and sets an error code appropriately.
 */
static bool
mgr_template_withdraw_all_cb(struct snapshot_rec *rec, void *data)
{
    struct mgr_template_withdrawal_all *info = data;
    assert(snapshot_rec_find(info->snap, rec->id) == rec);

    if (info->type != FDS_TYPE_TEMPLATE_UNDEF && info->type != rec->ptr->type) {
        // Skip this template (we are removing a different type of templates)
        return true;
    }

    int ret_code;
    if ((ret_code = mgr_snap_template_withdraw(info->snap, rec->id, info->type)) != FDS_OK) {
        // Something went wrong -> stop processing
        info->ret_code = ret_code;
        return false;
    }

    return true;
}

int
fds_tmgr_template_withdraw_all(fds_tmgr_t *tmgr, enum fds_template_type type)
{
    if (!tmgr->list.current) {
        // Undefined snapshot
        return FDS_ERR_ARG;
    }

    if (tmgr->cfg.withdraw_mod == WITHDRAW_PROHIBITED) {
        return FDS_ERR_DENIED;
    }

    int ret_code;
    if ((ret_code = mgr_modify_prepare(tmgr)) != FDS_OK) {
        return ret_code;
    }

    struct fds_tsnapshot *snap = tmgr->list.current;
    struct mgr_template_withdrawal_all data;
    data.snap = snap;
    data.type = type;
    data.ret_code = FDS_OK; // Expected return code. can be changed by the callback function

    // For each valid template of the given type, call the withdraw function
    snapshot_rec_for(snap, &mgr_template_withdraw_all_cb, &data);
    return data.ret_code;
}

int
fds_tmgr_template_remove(fds_tmgr_t *tmgr, uint16_t id, enum fds_template_type type)
{
    int ret_code;

    /* Iterate over all accessible snapshots (from the oldest to the newest one) and remove all
     * templates with the given Template ID and type.
     */
    for (struct fds_tsnapshot *ptr = tmgr->list.oldest; ptr != NULL; ptr = ptr->link.newer) {
        // Check history consistency
        assert(!ptr->link.newer || TIME_LE(ptr->start_time, ptr->link.newer->start_time));

        struct snapshot_rec *rec = snapshot_rec_find(ptr, id);
        if (!rec) {
            // Not found -> skip
            continue;
        }

        // Check the template type
        if (type != FDS_TYPE_TEMPLATE_UNDEF && type != rec->ptr->type) {
            // The type doesn't match
            continue;
        }

        if (ptr->link.newer != NULL && TIME_EQ(ptr->link.newer->start_time, ptr->start_time)) {
            /* This snapshot has the same start time as its descendant. Because this node is not
             * accessible for seek operation anymore (it is hidden by the descendant), we don't
             * have to modify it.
             */
            continue;
        }

        // Make sure that the snapshot is editable and remove the template
        if (!ptr->editable && (ret_code = mgr_snap_clone(ptr, &ptr, ptr->start_time)) != FDS_OK) {
            return ret_code;
        }

        if ((ret_code = mgr_snap_template_remove(ptr, id)) != FDS_OK) {
            return ret_code;
        }

        /* If the history is not modifiable (TCP, SCTP), immediately freeze manually all new
         * snapshots except the newest one, otherwise the manager can have problems with asserts.
         * (If modification of history is disabled, only the newest snapshot can be editable)
         */
        if (tmgr->cfg.en_history_mod == false && ptr->link.newer != NULL) {
            /* History modification is not enabled, so we don't have anything to propagate.
             * In other words, there is no record with "Create" flag in the new clone.
             */
            ptr->editable = false;
        }
    }

    // The current node is not accessible anymore
    tmgr->list.current = NULL;

    // Find the current node and close several snapshots (older then the current time)
    return fds_tmgr_set_time(tmgr, tmgr->time_now);
}

int
fds_tmgr_snapshot_get(const fds_tmgr_t *tmgr, const fds_tsnapshot_t **snap)
{
    struct fds_tsnapshot *current = tmgr->list.current;
    if (!current) {
        return FDS_ERR_ARG;
    }

    // Check if the snapshot is properly selected
    assert(TIME_LE(current->start_time, tmgr->time_now));
    assert(!current->link.newer || TIME_GT(current->link.newer->start_time, tmgr->time_now));

    // Check if all templates in the snapshot are still valid
    assert(!current->lifetime.enabled || TIME_GT(current->lifetime.min_value, tmgr->time_now));

    if (current->editable) {
        // The snapshot is still editable...
        int ret_code;
        if ((ret_code = mgr_snap_freeze(current)) != FDS_OK) {
            return ret_code;
        }
    }

    *snap = current;
    return FDS_OK;
}

const struct fds_template *
fds_tsnapshot_template_get(const fds_tsnapshot_t *snap, uint16_t id)
{
    const struct snapshot_rec *rec = snapshot_rec_cfind(snap, id);
    assert(rec == NULL || rec->ptr->id == id);
    return (rec != NULL) ? rec->ptr : NULL;
}

/// Auxiliary internal data structure for public snapshot iterator
struct tsnapshot_cb_data {
    /// User defined callback
    fds_tsnapshot_for_cb cb;
    /// User data for the user callback
    void *data;
};

/**
 * @brief Auxiliary callback wrapper for public snapshot iterator
 *
 * The wrapper makes sure that the user defined callback gets only access to the
 * IPFIX (Options) Template. Other internal structures of the snapshot are hidden
 * for users.
 * @param[in] rec  Snapshot record to process
 * @param[in] data Internal data structure of the public snapshot iterator
 * @return Return value of the user defined callback after its execution
 */
static bool
tsnapshot_cb_aux(struct snapshot_rec *rec, void *data)
{
    struct tsnapshot_cb_data *for_data = (struct tsnapshot_cb_data *) data;
    return for_data->cb(rec->ptr, for_data->data);
}

void
fds_tsnapshot_for(const fds_tsnapshot_t *snap, fds_tsnapshot_for_cb cb, void *data)
{
    // Ugly, but the callback cannot modify the snapshot, so it should be OK
    fds_tsnapshot_t *snap_orig = (fds_tsnapshot_t *) snap;

    struct tsnapshot_cb_data for_data = {cb, data};
    snapshot_rec_for(snap_orig, &tsnapshot_cb_aux, &for_data);
}

int
fds_tmgr_template_get(fds_tmgr_t *tmgr, uint16_t id, const struct fds_template **tmplt)
{
    const struct fds_tsnapshot *snap;
    int ret_code;

    if ((ret_code = fds_tmgr_snapshot_get(tmgr, &snap)) != FDS_OK) {
        return ret_code;
    }

    const struct snapshot_rec *rec = snapshot_rec_cfind(snap, id);
    if (!rec) {
        *tmplt = NULL;
        return FDS_ERR_NOTFOUND;
    } else {
        *tmplt = rec->ptr;
        return FDS_OK;
    }
}

int
fds_tmgr_set_udp_timeouts(fds_tmgr_t *tmgr, uint16_t tl_data, uint16_t tl_opts)
{
    if (tmgr->cfg.session_type != FDS_SESSION_UDP) {
        return FDS_ERR_ARG;
    }

    tmgr->limits.lifetime_normal = tl_data;
    tmgr->limits.lifetime_opts = tl_opts;
    return FDS_OK;
}

void
fds_tmgr_set_snapshot_timeout(fds_tmgr_t *tmgr, uint16_t timeout)
{
    tmgr->limits.lifetime_snapshot = timeout;
}

/** \brief Auxiliary data structure for fds_tmgr_set_iemgr_cb() */
struct fds_tmgr_set_iemgr_data {
    /** Snapshot that is modified      */
    struct fds_tsnapshot *snap;
    /** New IE manager                 */
    const struct fds_iemgr *ie_defs;
    /** Status code of whole operation */
    int ret_code;
};

/**
 * \brief Create an updated copy of a template and propagate it (auxiliary function)
 *
 * A new copy will be created only if \p rec is the owner of the template (i.e. it has "Delete"
 * flag) to prevent multiple duplications.
 *
 * If an error has occurred during modification, a status code will be changed from #FDS_OK to the
 * corresponding error. If the error code is different from the #FDS_OK, the snapshot record \p rec
 * will be removed. Therefore, if this callback function is called on all snapshot records in
 * a snapshot and an error has occurred during processing, in the snapshot will remain only
 * successfully copied templates.
 * \param[in] rec  Snapshot record
 * \param[in] data Auxiliary data structure
 * \return Always true
 */
static bool
fds_tmgr_set_iemgr_cb(struct snapshot_rec *rec, void *data)
{
    struct fds_tmgr_set_iemgr_data *info = data;
    assert(snapshot_rec_find(info->snap, rec->id) == rec);

    // Something went wrong -> we have to remove all remaining references
    if (info->ret_code != FDS_OK) {
        snapshot_rec_remove(info->snap, rec->id);
        return true;
    }

    // Is the record responsible for the template?
    if ((rec->flags & SNAPSHOT_TF_DESTROY) == 0) {
        // Skip
        return true;
    }

    // Create a copy of the template
    struct fds_template *ptr_old = rec->ptr;
    struct fds_template *ptr_new = fds_template_copy(rec->ptr);
    if (!ptr_new) {
        // Failed!
        info->ret_code = FDS_ERR_NOMEM;
        return true;
    }

    // Add new definitions
    info->ret_code = fds_template_ies_define(ptr_new, info->ie_defs, false);
    if (info->ret_code != FDS_OK) {
        fds_template_destroy(ptr_new);
        return true;
    }

    rec->ptr = ptr_new;

    // Now propagate the pointer to predecessors
    struct fds_tsnapshot *snap_ptr = info->snap->link.older;
    struct snapshot_rec *snap_rec;

    while (snap_ptr) {
        snap_rec = snapshot_rec_find(snap_ptr, rec->id);
        snap_ptr = snap_ptr->link.older; // For the next iteration

        if (!snap_rec || snap_rec->ptr != ptr_old) {
            continue;
        }

        // Replace the old pointer
        snap_rec->ptr = ptr_new;
        if (snap_rec->flags & SNAPSHOT_TF_CREATE) {
            // This is the oldest owner of the template
            break;
        }
    }

    return true;
}

int
fds_tmgr_set_iemgr(fds_tmgr_t *tmgr, const fds_iemgr_t *iemgr)
{
    // To make it faster, first clean up hierarchy
    mgr_cleanup(tmgr);

    if (tmgr->list.newest == NULL) {
        // The manager is empty -> nothing to do
        assert(tmgr->list.oldest == NULL);
        tmgr->ies_db = iemgr;
        return FDS_OK;
    }

    // Copy whole hierarchy (from the head to the tail)
    struct fds_tsnapshot *new_head = NULL, *new_tail, *new_last = NULL;
    struct fds_tsnapshot *tmp_old, *tmp_new;
    bool failed = false;

    tmp_old = tmgr->list.newest;
    while (tmp_old) {
        tmp_new = snapshot_copy(tmp_old);
        if (!tmp_new) {
            failed = true;
            break;
        }

        // Update pointers
        tmp_new->link.newer = new_last;
        if (new_last == NULL) {
            // This is the newest snapshot
            new_head = tmp_new;
        } else {
            new_last->link.older = tmp_new;
        }

        // Get ready for the next iteration
        new_last = tmp_new;
        tmp_old = tmp_old->link.older;
    }

    new_tail = new_last;
    if (new_last) {
        new_last->link.older = NULL;
    }

    if (failed) {
        // Destroy all snapshots
        struct fds_tsnapshot *ptr = new_tail;
        while (ptr) {
            struct fds_tsnapshot *tmp = ptr;
            ptr = ptr->link.newer;
            snapshot_destroy(tmp);
        }

        return FDS_ERR_NOMEM;
    }

    // From the newest to the oldest snapshot
    struct fds_tsnapshot *edit_ptr;
    for (edit_ptr = new_head; edit_ptr != NULL; edit_ptr = edit_ptr->link.older) {
        // Duplicate all templates with "Delete flags" and propagate them
        struct fds_tmgr_set_iemgr_data data;
        data.snap = edit_ptr;
        data.ie_defs = iemgr;
        data.ret_code = FDS_OK;

        snapshot_rec_for(edit_ptr, &fds_tmgr_set_iemgr_cb, &data);
        if (data.ret_code != FDS_OK) {
            // Something went wrong
            failed = true;
            break;
        }
    }

    if (failed) {
        // Delete snapshots WITHOUT ownership of the new templates
        struct fds_tsnapshot *ptr = new_tail;
        while (ptr != edit_ptr) {
            struct fds_tsnapshot *tmp = ptr;
            ptr = ptr->link.newer;
            snapshot_destroy(tmp);
        }

        // Delete snapshots WITH ownership of the new templates
        assert(ptr == edit_ptr);
        while (ptr != NULL) {
            struct fds_tsnapshot *tmp = ptr;
            ptr = ptr->link.newer;
            mgr_snap_destroy(tmp);
        }

        return FDS_ERR_NOMEM;
    }

    // Ufff, everything is ready -> replace whole hierarchy
    fds_tmgr_clear(tmgr);
    tmgr->list.oldest = new_tail;
    tmgr->list.newest = new_head;
    tmgr->ies_db = iemgr;

    return FDS_OK;
}

int
fds_tmgr_template_set_fkey(fds_tmgr_t *tmgr, uint16_t id, uint64_t key)
{
    // Check the record
    struct fds_tsnapshot *snap = tmgr->list.current;
    if (!snap) {
        return FDS_ERR_ARG;
    }

    struct snapshot_rec *snap_rec = snapshot_rec_find(snap, id);
    if (!snap_rec) {
        return FDS_ERR_NOTFOUND;
    }

    if (fds_template_flowkey_applicable(snap_rec->ptr, key) != FDS_OK) {
        // Flow key can be assigned only to Template Records and flow key cannot be too long
        return FDS_ERR_ARG;
    }

    if (!fds_template_flowkey_cmp(snap_rec->ptr, key)) {
        // The same flow key is already set -> skip
        return FDS_OK;
    }

    // Preparation for modifications
    int ret_code;
    if ((ret_code = mgr_modify_prepare(tmgr)) != FDS_OK) {
        return ret_code;
    }

    snap = tmgr->list.current; // The prepare operation can change the current pointer
    assert(snap->editable);

    // Check consistency
    assert(TIME_EQ(tmgr->time_now, snap->start_time));
    assert(!snap->link.newer || TIME_LT(tmgr->time_now, snap->link.newer->start_time));

    // Replace the template in this snapshot and in its descendants
    const uint32_t first_seen = snap_rec->ptr->time.first_seen;

    const struct fds_template *tmplt_orig = NULL;
    struct fds_template *tmplt_new = NULL;
    struct snapshot_rec *rec_last_modif = NULL;

    for (struct fds_tsnapshot *it = snap; it != NULL; it = it->link.newer) {
        struct snapshot_rec *rec = snapshot_rec_find(it, id);
        if (!rec) {
            /* Snapshot doesn't have a reference to the template, but there can be still anyone
             * else in the future due to history modification that caused this "gap".
             */
            continue;
        }

        assert(rec->id == id && rec->ptr->id == id);
        if (TIME_GT(rec->ptr->time.first_seen, first_seen)) {
            /* The found a future definition of the template that has a newer start time.
             * Because the start time is newer, the definition must be also different (it cannot be
             * template update) -> We have to stop propagation here.
             */
            break;
        }

        if (it->link.newer != NULL && TIME_EQ(it->link.newer->start_time, it->start_time)) {
            /* This snapshot has the same start time as its descendant. Because this node is not
             * accessible for seek operation anymore (it is hidden by the descendant), we don't
             * have to modify it.
             */
            continue;
        }

        // Make sure that the snapshot is editable and remove the template
        if (!it->editable) {
            if ((ret_code = mgr_snap_edit(it, &it)) != FDS_OK) {
                return ret_code;
            }

            // We have to find the new snapshot record
            rec = snapshot_rec_find(it, id);
            assert(rec != NULL);
        }

        // Just check if the templates are the same
        assert(tmplt_new == NULL || fds_template_cmp(tmplt_new, rec->ptr) == 0);

        if (rec->ptr != tmplt_orig) {
            // Create a new copy of the template
            tmplt_new = fds_template_copy(rec->ptr);
            if (!tmplt_new) {
                return FDS_ERR_NOMEM;
            }

            // Apply modifications
            if ((ret_code = fds_template_flowkey_define(tmplt_new, key)) != FDS_OK) {
                return ret_code;
            }

            // Replace the record
            const struct fds_template *old_ptr = rec->ptr; // Don't use for access (address only)
            if ((ret_code = mgr_snap_template_remove(it, id)) != FDS_OK) {
                return ret_code;
            }

            /* First owner of the template should have "Create" and "Delete" flag, but "Create"
             * flag would cause (in case of history modification) later propagation of the template.
             * Unfortunately we do propagation right here, because we have to propagate the flow
             * key even if the template was later refreshed (this cannot be done by normal
             * propagation mechanism). The solution is not to use "Create" flag at all. Missing
             * "Create" flag can only cause longer seeking in history in case of some operations.
             */
            const uint16_t flags = SNAPSHOT_TF_DESTROY; // First owner
            if ((ret_code = mgr_snap_template_add_ref(it, tmplt_new, flags)) != FDS_OK) {
                return ret_code;
            }

            // We have to find the record again because remove/add operation can change address
            rec_last_modif = snapshot_rec_find(it, id);
            tmplt_orig = old_ptr;
        } else {
            // Just replace the old one with the new one
            assert(snapshot_rec_find(it, id)->ptr == tmplt_orig);
            if ((ret_code = mgr_snap_template_remove(it, id)) != FDS_OK) {
                return ret_code;
            }

            const uint16_t flags = SNAPSHOT_TF_DESTROY;
            if ((ret_code = mgr_snap_template_add_ref(it, tmplt_new, flags)) != FDS_OK) {
                return ret_code;
            }

            // Remove the destroy flag from the previous record
            assert(rec_last_modif->flags & SNAPSHOT_TF_DESTROY);
            rec_last_modif->flags &= ~SNAPSHOT_TF_DESTROY;

            // We have to find the record again because remove/add operation can change address
            rec_last_modif = snapshot_rec_find(it, id);
        }
    }

    return FDS_OK;
}

/**
 * @}
 */
