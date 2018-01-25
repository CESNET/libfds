/**
 * \file src/template_mgr/snapshot.c
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \brief Snapshot structure and auxiliary functions (source file)
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

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>  // memcpy

#include "../ipfix_structures.h"
#include "snapshot.h"

/**
 * \brief Set L1 index flag at a user defined position
 * \param[in] set Snapshot
 * \param[in] bit  Bit index (start from 0)
 */
static inline void
snapshot_bit_set(snapshot_bitset_t *set, uint16_t bit)
{
    assert(bit < SNAPSHOT_TABLE_SIZE);
    set->set[bit / SNAPSHOT_BITSET_BPI] |= ((uint32_t) 1U) << (bit % SNAPSHOT_BITSET_BPI);
}

/**
 * \brief Clear L1 index flag at a user defined position
 * \param[in] set Snapshot
 * \param[in] bit  Bit index (start from 0)
 */
static inline void
snapshot_bit_clear(snapshot_bitset_t *set, uint16_t bit)
{
    assert(bit < SNAPSHOT_TABLE_SIZE);
    set->set[bit / SNAPSHOT_BITSET_BPI] &= ~(((uint32_t) 1U) << (bit % SNAPSHOT_BITSET_BPI));
}

/**
 * \brief Get the index of the next set bit in the L1 index
 * \param[in]  set   Snapshot
 * \param[in]  start Start search from this position (inclusive)
 * \param[out] bit   Position of the bit (only if return code is #FDS_OK)
 * \return #FDS_OK or #FDS_ERR_NOTFOUND
 */
static inline int
snapshot_bit_next(snapshot_bitset_t *set, uint16_t start, uint16_t *bit)
{
    if (start >= SNAPSHOT_TABLE_SIZE) {
        return FDS_ERR_NOTFOUND;
    }

    size_t arr_idx = start / SNAPSHOT_BITSET_BPI;
    uint32_t arr_data =  set->set[arr_idx];
    arr_data >>= (start % SNAPSHOT_BITSET_BPI);
    size_t result = start;

    while (true) {
        // Check this block of bitset
        if (arr_data != 0) {
            if (arr_data & 0x1) {
                // Found
                break;
            }

            arr_data >>= 1;
            result++;
            continue;
        }

        // Go the next block
        arr_idx++;
        result = arr_idx * SNAPSHOT_BITSET_BPI;
        if (result < SNAPSHOT_TABLE_SIZE) {
            arr_data = set->set[arr_idx];
            continue;
        }

        // Not found
        return FDS_ERR_NOTFOUND;
    }

    *bit = result;
    return FDS_OK;
}

fds_tsnapshot_t *
snapshot_create() {
    struct fds_tsnapshot *snap = calloc(1, sizeof(*snap));
    if (!snap) {
        return NULL;
    }

    // L1 table and its bitset will be clear after calloc() -> nothing to do
    return snap;
}

void
snapshot_destroy(struct fds_tsnapshot *snap)
{
    // Delete all L2 tables
    uint16_t idx = 0;
    while (snapshot_bit_next(&snap->l1_table.bitset, idx, &idx) == FDS_OK) {
        free(snap->l1_table.tables[idx]);
        idx++;
    }

    // Delete the snapshot itself
    free(snap);
}

struct fds_tsnapshot *
snapshot_copy(const struct fds_tsnapshot *snap)
{
    // Copy snapshot
    struct fds_tsnapshot *new_snap = malloc(sizeof(*new_snap));
    if (!new_snap) {
        return NULL;
    }

    memcpy(new_snap, snap, sizeof(*new_snap));

    // Copy all L2 tables
    uint16_t copy_idx = 0;
    bool failed = false;

    snapshot_bitset_t *bitset = &new_snap->l1_table.bitset;
    while (snapshot_bit_next(bitset, copy_idx, &copy_idx) == FDS_OK) {
        const struct snapshot_l2_table *old_l2 = snap->l1_table.tables[copy_idx];
        if (old_l2->rec_cnt == 0) {
            // Do not copy empty tables
            new_snap->l1_table.tables[copy_idx] = NULL;
            snapshot_bit_clear(bitset, copy_idx);
            copy_idx++;
            continue;
        }

        struct snapshot_l2_table *new_l2 = malloc(sizeof(*new_l2));
        if (!new_l2) {
            failed = true;
            break;
        }
        memcpy(new_l2, old_l2, sizeof(*new_l2));
        new_snap->l1_table.tables[copy_idx] = new_l2;
        copy_idx++;
    }

    if (failed) {
        // Free successfully copied tables
        uint16_t idx = 0;
        while (snapshot_bit_next(bitset, idx, &idx) == FDS_OK && idx < copy_idx) {
            free(new_snap->l1_table.tables[idx]);
            idx++;
        }

        free(new_snap);
        return NULL;
    }

    return new_snap;
}

int
snapshot_rec_add(struct fds_tsnapshot *snap, struct snapshot_rec *rec)
{
    assert(rec->id >= IPFIX_SET_MIN_DATA_SET_ID);

    const uint16_t l1_idx = rec->id / SNAPSHOT_TABLE_SIZE;
    struct snapshot_l2_table *l2_table = snap->l1_table.tables[l1_idx];
    if (!l2_table) {
        // Create L2 table
        l2_table = calloc(1, sizeof(*l2_table));
        if (!l2_table) {
            return FDS_ERR_NOMEM;
        }

        snap->l1_table.tables[l1_idx] = l2_table;
        snapshot_bit_set(&snap->l1_table.bitset, l1_idx);
    }

    const uint16_t l2_idx = rec->id % SNAPSHOT_TABLE_SIZE;
    struct snapshot_rec *l2_rec = &l2_table->recs[l2_idx];
    if (l2_rec->id == 0) {
        // New record
        snapshot_bit_set(&l2_table->bitset, l2_idx);
        l2_table->rec_cnt++;
        snap->rec_cnt++;
    }

    *l2_rec = *rec;
    return FDS_OK;
}

int
snapshot_rec_remove(struct fds_tsnapshot *snap, uint16_t id)
{
    assert(id >= IPFIX_SET_MIN_DATA_SET_ID);

    // Find the record
    const uint16_t l1_idx = id / SNAPSHOT_TABLE_SIZE;
    struct snapshot_l2_table *l2_table = snap->l1_table.tables[l1_idx];
    if (!l2_table) {
        return FDS_ERR_NOTFOUND;
    }

    const uint16_t l2_idx = id % SNAPSHOT_TABLE_SIZE;
    struct snapshot_rec *l2_rec = &l2_table->recs[l2_idx];
    if (l2_rec->id == 0) {
        return FDS_ERR_NOTFOUND;
    }

    // Remove it
    assert(l2_rec->id == id);
    assert(l2_table->rec_cnt > 0);
    assert(snap->rec_cnt > 0);

    *l2_rec = (struct snapshot_rec) {0, 0, 0, NULL};
    snapshot_bit_clear(&l2_table->bitset, l2_idx);
    l2_table->rec_cnt--;
    snap->rec_cnt--;

    /* We don't want to free empty L2 table here.
     * Someone can iterate over the table (i.e. snapshot_rec_for()) and by removing the last
     * record of the table (snapshot_rec_remove()) and therefore L2 table itself, we would remove
     * the L2 table while the iterator is still using it.
     */
    return FDS_OK;
}

const struct snapshot_rec *
snapshot_rec_cfind(const struct fds_tsnapshot *snap, uint16_t id)
{
    // Find the record
    const uint16_t l1_idx = id / SNAPSHOT_TABLE_SIZE;
    struct snapshot_l2_table *l2_table = snap->l1_table.tables[l1_idx];
    if (!l2_table) {
        return NULL;
    }

    const uint16_t l2_idx = id % SNAPSHOT_TABLE_SIZE;
    struct snapshot_rec *l2_rec = &l2_table->recs[l2_idx];
    if (l2_rec->id == 0) {
        return NULL;
    }

    return l2_rec;
}

struct snapshot_rec *
snapshot_rec_find(struct fds_tsnapshot *snap, uint16_t id)
{
    return (struct snapshot_rec *) snapshot_rec_cfind(snap, id);
}

void
snapshot_rec_for(struct fds_tsnapshot *snap, snapshot_rec_cb cb, void *data)
{
    uint16_t l1_idx = 0;
    snapshot_bitset_t *l1_bitset = &snap->l1_table.bitset;

    // For each L2 table
    while (snapshot_bit_next(l1_bitset, l1_idx, &l1_idx) == FDS_OK) {
        struct snapshot_l2_table *l2_table = snap->l1_table.tables[l1_idx];
        snapshot_bitset_t *l2_bitset = &l2_table->bitset;

        // For each record in the L2 table
        uint16_t l2_idx = 0;
        while (snapshot_bit_next(l2_bitset, l2_idx, &l2_idx) == FDS_OK) {
            // Call user defined function
            struct snapshot_rec *l2_rec = &l2_table->recs[l2_idx];
            if (!cb(l2_rec, data)) {
                return;
            }
            l2_idx++;
        }

        l1_idx++;
    }
}

