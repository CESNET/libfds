/**
 * \file src/template_mgr/snapshot.h
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \brief Snapshot structure and auxiliary functions (internal header file)
 * \date 2017
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
 * This software is provided ``as is``, and any express or implied
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

#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <libfds.h>

// Declaration (to avoid cross include)
struct fds_tmgr;

/** L1 and L2 table size (must be a power of 2) */
#define SNAPSHOT_TABLE_SIZE (256U)
/** \brief Bits per an item of an index array */
#define SNAPSHOT_BITSET_BPI (8 * sizeof(uint32_t))

/** Snapshot L1 and L2 bitset (able to handle up to 256 bits) */
typedef struct snapshot_bitset {
    /** Bit array */
    uint32_t set[8]; // 8 * 32 == 256 a.k.a. SNAPSHOT_TABLE_SIZE
} snapshot_bitset_t;

/** \brief Snapshot record features                                                              */
enum snapshot_rec_flags {
    /**
     *  \brief Create flag
     *
     *  If this flag is set, this snapshot has added this template. In other words, there is no
     *  one older with a reference to this template.
     *  \warning The flag must NOT be moved to another snapshot (for example, during snapshot
     *    cloning etc.)!
     */
    SNAPSHOT_TF_CREATE = (1 << 0),
    /**
     * \brief Destroy flag
     *
     * If this flag is set, this snapshot is responsible for destroying this template.
     * \warning In case of removing the snapshot from a template manager, this flag MUST be moved
     *   to the newest snapshot that has a reference to this template and is still in the manager.
     *   If this is the last snapshot with the reference to the template, the flag MUST remain
     *   and the template will be deleted together with the snapshot.
     */
    SNAPSHOT_TF_DESTROY = (1 << 1),
    /**
     * \brief Timeout enabled
     *
     * If this flag is set, a referenced template has limited lifetime that is described by
     * variable snapshot_rec#lifetime.
     */
    SNAPSHOT_TF_TIMEOUT = (1 << 2)
};

/** Snapshot record (a.k.a. reference to a template)                                             */
struct snapshot_rec {
    /** Template ID (must be >= 256)           */
    uint16_t id;
    /**
     * \brief Features specific for this record
     *
     * The flags argument contains a bitwise OR of zero or more of the flags defined in
     * #snapshot_rec_flags enumeration.
     */
    uint16_t flags;
    /**
     * \brief Template lifetime
     *
     * Time after which this template is not valid anymore, exclusive. This values is valid only
     * if the lifetime flag (::SNAPSHOT_TF_TIMEOUT) is set.
     */
    uint32_t lifetime;
    /** Reference to a corresponding template  */
    struct fds_template *ptr;
};

/** Snapshot L1 table */
struct snapshot_l1_table {
    /** Array of L2 tables        */
    struct snapshot_l2_table *tables[SNAPSHOT_TABLE_SIZE];
    /** Bitset of used L2 tables  */
    snapshot_bitset_t bitset;
};

/** Snapshot L2 table */
struct snapshot_l2_table {
    /** Bitset of valid records   */
    snapshot_bitset_t bitset;
    /** Records in the array      */
    uint16_t rec_cnt;
    /** Array of records          */
    struct snapshot_rec recs[SNAPSHOT_TABLE_SIZE];
};

/**
 * \brief Snapshot of valid templates at specific time
 * \warning User can safely manipulate directly with every parameter except #l1_table.
 *   To add/remove or find a template always use snapshot functions \ref snapshot_aux_func.
 */
struct fds_tsnapshot {
    /** Start time of validity (the Export Time of an IPFIX message) */
    uint32_t start_time;

    struct {
        /** Pointer to a newer snapshot (successor)  */
        struct fds_tsnapshot *newer;
        /** Pointer to a old snapshot (predecessor)  */
        struct fds_tsnapshot *older;
        /** Parent manager                           */
        struct fds_tmgr *mgr;
    } link; /**< Valid only when the snapshot is inside the template manager!   */

    struct {
        /**
         * \brief Minimal value of template lifetime
         *
         * If the lifetime is enabled, this value represents an Export Time when at least
         * one template will not be valid anymore i.e. this snapshot cannot be used after this
         * time, inclusive.
         */
        uint32_t min_value;
        /** Snapshot lifetime is enabled when at least one template has enabled lifetime */
        bool enabled;
    } lifetime; /**< Snapshot lifetime */

    /**
     * \brief Editability of the snapshot
     *
     * If the snapshot is editable, there is no reference to the template outside of this manager.
     * Thus, templates could be added, withdrawn and/or removed. If the snapshot is not editable,
     * modification of this snapshot is STRICTLY prohibited!
     */
    bool editable;

    /** Number of records in the snapshot */
    uint16_t rec_cnt;

    /**
     * \brief Table of templates 2-level table of templates.
     * \warning Do NOT use directly.
     */
    struct snapshot_l1_table l1_table;
};

/**
 * \defgroup snapshot_aux_func Snapshot auxiliary functions
 * \brief Function for elementary manipulation with snapshot
 *
 * Following functions do NOT manipulate with the parameters of the structure except adding
 * and removing snapshot records (a.k.a. a reference to template). Snapshot logic must be
 * therefor implemented by somewhere else, in this case in the template manager.
 *
 * @{
 */

/**
 * \brief Create a new snapshot structure
 *
 * Values of the function will be set to default i.e. zero values and the internal array of the
 * templates will be prepared for further insertion.
 * \return Pointer or NULL in case of memory error.
 */
fds_tsnapshot_t *
snapshot_create();

/**
 * \brief Destroy a snapshot
 *
 * Referenced templates will NOT be freed. If you need to free them, manually iterate over
 * the reference array first.
 * \param[in] snap Snapshot
 */
void
snapshot_destroy(struct fds_tsnapshot *snap);

/**
 * \brief Make a copy of a snapshot
 *
 * The new copy of the snapshot will have its own copy of the array of template references, but the
 * templates will NOT be copied.
 * \param[in] snap Snapshot
 * \return Pointer or NULL (in case of memory error)
 */
struct fds_tsnapshot *
snapshot_copy(const struct fds_tsnapshot *snap);

/**
 * \brief Add a snapshot record
 *
 * In other words, this function can be used to add a reference to a template.
 * \param[in] snap  Snapshot
 * \param[in] rec   Snapshot record
 * \warning If there is already a record with the same ID, it will be rewritten with the new one.
 * \return On success returns #FDS_OK. Otherwise returns #FDS_ERR_NOMEM and the record is not
 *   added.
 */
int
snapshot_rec_add(struct fds_tsnapshot *snap, struct snapshot_rec *rec);

/**
 * \brief Remove a snapshot record
 *
 * In other words, this function can be used to remove a reference to a template (i.e. the template
 * will not be freed)
 * \param[in] snap Snapshot
 * \param[in] id   Template ID (from the snapshot record)
 * \return #FDS_OK or #FDS_ERR_NOTFOUND
 */
int
snapshot_rec_remove(struct fds_tsnapshot *snap, uint16_t id);

/**
 * \brief Get a snapshot record of a template
 *
 * \param[in] snap Snapshot
 * \param[in] id   Template ID (from the snapshot record)
 * \return Pointer of NULL (if template doesn't exist in the snapshot)
 */
struct snapshot_rec *
snapshot_rec_find(struct fds_tsnapshot *snap, uint16_t id);

/**
 * \brief Const alternative of snapshot_rec_find()
 * \copydetails snapshot_rec_find
 */
const struct snapshot_rec *
snapshot_rec_cfind(const struct fds_tsnapshot *snap, uint16_t id);

/**
 * \brief Function callback for a snapshot record
 *
 * \param[in] snap Snapshot
 * \param[in] data User defined data for the callback (optional)
 * \return Continue iteration
 */
typedef bool (*snapshot_rec_cb)(struct snapshot_rec *rec, void *data);

/**
 * \brief Call a function on each snapshot record in a snapshot
 *
 * It is guaranteed that records will be processed in the order given by their Template ID
 * in ascending order. It is also safe to call snapshot_rec_remove() from the callback on
 * this record.
 * \param[in] snap Snapshot
 * \param[in] cb   Callback function
 * \param[in] data User defined data that will be passed to the callback
 */
void
snapshot_rec_for(struct fds_tsnapshot *snap, snapshot_rec_cb cb, void *data);

/** @} */ // end of the group

#endif // SNAPSHOT_H
