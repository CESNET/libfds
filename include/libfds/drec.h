/**
 * \file libfds/drec.h
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \brief Data record in the IPFIX message (header file)
 * \date 2018
 */

/* Copyright (C) 2018 CESNET, z.s.p.o.
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

#ifndef LIBFDS_DREC_H
#define LIBFDS_DREC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "template.h"
#include "template_mgr.h"

/**
 * \defgroup fds_drec IPFIX Data Record
 * \ingroup publicAPIs
 * \brief Function for accessing IPFIX fields of an individual Data Record
 * \remark Based on RFC 7011, Section 3.4.3. (see https://tools.ietf.org/html/rfc7011#section-3.4.3)
 *
 * Following functions can be used to find an occurrence of a field in a Data Record that
 * belongs to a particular (Options) Template Record. Advanced features (such as accessing
 * multiple occurrences of the same element in the Data Record or distinction Data fields of
 * a Biflow Data Record by direction) are provided by Data Record iterator.
 *
 * @{
 */

/**
 * \brief Structure for parsed IPFIX Data record
 *
 * Represents a one parsed data record in the IPFIX packet.
 */
struct fds_drec {
    uint8_t *data;                     /**< Start of the record                     */
    uint16_t size;                     /**< Size of the record (in bytes)           */
    const struct fds_template *tmplt;  /**< Template (always must be defined)       */
    const fds_tsnapshot_t *snap;       /**< Template manager snapshot (can be NULL) */
};

/**
 * \brief Structure for a data field in a data record in a IPFIX message
 *
 * This structure is used by a lookup function or an iterator.
 */
struct fds_drec_field {
    /**
     * Pointer to the field in a data record. It always points to the beginning of Information
     * Element data (i.e. for variable-length elements behind octet prefix with real-length).
     */
    uint8_t *data;
    /** Real length of the field                                             */
    uint16_t size;
    /** Pointer to the field description (IDs, data types, etc.)             */
    const struct fds_tfield *info;
};

/**
 * \brief Get a field in a data record
 *
 * Try to find the first occurrence of the field defined by an Enterprise Number
 * and an Information Element ID in a data record.
 * \param[in]  rec   Pointer to the data record
 * \param[in]  pen   Private Enterprise Number
 * \param[in]  id    Information Element ID
 * \param[out] field Pointer to a variable where the result will be stored
 * \return If the field is present in the record, this function will fill \p field and return
 *   an index of the field in the record (the index starts from 0). Otherwise (the field is not
 *   present in the record) returns #FDS_EOC and the \p field is not filled.
 */
FDS_API int
fds_drec_find(struct fds_drec *rec, uint32_t pen, uint16_t id, struct fds_drec_field *field);

/** \brief Iterator over all data fields in a data record                                */
struct fds_drec_iter {
    /** Current field of an iterator                                                     */
    struct fds_drec_field field;

    /** FOR INTERNAL USE ONLY. DO NOT USE DIRECTLY!                                      */
    struct {
        struct fds_drec *rec;               /**< Pointer to the data record              */
        const struct fds_tfield *fields;    /**< Template fields                         */
        uint16_t next_offset;               /**< Offset of the next field                */
        uint16_t next_idx;                  /**< Index of the next field                 */
        uint16_t flags;                     /**< Iterator flags                          */
    } _private; /**< Internal field (implementation can be changed!)                     */
};

/** \brief Configuration flags of Data Record Iterator.                                  */
enum fds_drec_iter_flags {
    /**
     * \brief Skip fields with unknown definition of an Information Element
     */
    FDS_DREC_UNKNOWN_SKIP = (1 << 0),
    /**
     * \brief in case of a Biflow record, skip all reverse fields
     */
    FDS_DREC_REVERSE_SKIP = (1 << 1),
    /**
     * \brief In case of a Biflow record, show from Forward point of view
     */
    FDS_DREC_BIFLOW_FWD   = (1 << 2),
    /**
     * \brief In case of a Biflow record, show from Reverse point of view
     *
     * Template fields are remapped to represent fields in opposite direction using reverse
     * template fields (fds_template#fields_rev). In other words, common directional fields are
     * converted to opposite direction, for example, source IP address to destination IP address
     * and vice versa. Forward only fields to reverse only fields and vice versa.
     *
     * The flag can be combined with ::FDS_DREC_REVERSE_SKIP. In this case, the reverse filter
     * is applied on remapped template fields.
     */
    FDS_DREC_BIFLOW_REV   = (1 << 3),
    /**
     * \brief Do not skip Padding fields (PEN: 0, IE: 210, "paddingOctets")
     */
    FDS_DREC_PADDING_SHOW = (1 << 4)
};

/**
 * \brief Initialize an iterator to data fields in a data record
 * \warning
 *   After initialization the iterator only has initialized internal structures, but public part
 *   is still undefined i.e. does NOT point to the first field in the record. To get the first
 *   field and for code example, see fds_drec_iter_next().
 * \note The \p flags argument contains a bitwise OR of zero or more of the flags defined in
     #fds_drec_iter_flags enumeration.
 * \param[out] iter   Pointer to the iterator to initialize
 * \param[in]  record Pointer to the data record
 * \param[in]  flags  Iterator flags (see #fds_drec_iter_flags)
 */
FDS_API void
fds_drec_iter_init(struct fds_drec_iter *iter, struct fds_drec *record, uint16_t flags);

/**
 * \brief Get the next field in the record
 *
 * Move the iterator to the next field in the order. If this function was NOT previously called
 * after initialization by fds_drec_iter_init(), then the iterator will point to the first field
 * in a data record.
 *
 * \code{.c}
 *  // How to use iterators...
 *  struct fds_drec_iter it;
 *  fds_drec_iter_init(&it, record, 0);
 *
 *  while (fds_drec_iter_next(&it) != FDS_EOC) {
 *      // Add your code here, for example:
 *      const struct fds_tfield *field = it.field.info;
 *      printf("en: %" PRIu32 " & id: %" PRIu16 "\n", field->en, field->id);
 *  }
 * \endcode
 *
 * \note Padding fields (PEN: 0, IE: 210, "paddingOctets") are automatically skipped.
 * \param[in,out] iter Pointer to the iterator
 * \return If the next field is prepared, returns an index of the field in the record (starts
 *   from 0). Otherwise (no more field in the record) returns #FDS_EOC.
 */
FDS_API int
fds_drec_iter_next(struct fds_drec_iter *iter);

/**
 * \brief Find a field in the record
 *
 * Search starts from the successor of the current field. Therefore, after the iterator
 * initialization or rewind it will start searching from the first field. This function can be
 * also used to find multiple occurrences of the same Information element in the record. See
 * the code example below.
 *
 * \code{.c}
 *  // How to use iterator to find multiple occurrences of the same Information Element
 *  uint32_t pen = ...;
 *  uint16_t id  = ...;
 *
 *  struct fds_drec_iter it;
 *  fds_drec_iter_init(&it, record, 0);
 *
 *  while (fds_drec_iter_find(&it, pen, id) != FDS_EOC) {
 *      // Add your code here...
 *  }
 * \endcode
 *
 * \note Iterator flags ::FDS_DREC_UNKNOWN_SKIP and ::FDS_DREC_REVERSE_SKIP are ignored.
 * \note If one of Biflow direction flags (i.e. ::FDS_DREC_BIFLOW_FWD or ::FDS_DREC_BIFLOW_REV) is
 *   set, fields are searched from the point of view of the selected direction. In case of
 *   the forward flag (::FDS_DREC_BIFLOW_FWD) fields are search as present in an IPFIX template.
 *   In case of reverse flag (::FDS_DREC_BIFLOW_REV) fields are remapped to represent fields in
 *   opposite direction using reverse template fields (fds_template#fields_rev). In other words,
 *   common directional fields are converted to opposite direction, for example, source IP address
 *   to destination IP address and vice versa. Forward only fields to reverse only fields and vice
 *   versa.
 * \param[in,out] iter Pointer to the iterator
 * \param[in]     pen  Private Enterprise Number
 * \param[in]     id   Information Element ID
 * \return On success returns an index of the field in the record (the index starts from 0).
 *   Otherwise returns #FDS_EOC.
 */
FDS_API int
fds_drec_iter_find(struct fds_drec_iter *iter, uint32_t pen, uint16_t id);

/**
 * \brief Rewind an iterator to the beginning (before the first record in a record)
 *
 * The record field (fds_drec_iter#field) will be undefined and the function fds_drec_iter_next()
 * or fds_drec_iter_find() must be used to define it. Flags of the iterator will be preserved.
 * \param[out] iter Pointer to the iterator
 */
FDS_API void
fds_drec_iter_rewind(struct fds_drec_iter *iter);

#ifdef __cplusplus
}
#endif

#endif // LIBFDS_DREC_H

/**
 * @}
 */
