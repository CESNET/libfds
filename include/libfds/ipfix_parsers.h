/**
 * \file include/libfds/ipfix_parsers.h
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \brief Simple parsers of an IPFIX Message
 * \date 2018
 */
/*
 * Copyright (C) 2018 CESNET, z.s.p.o.
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

#ifndef LIBFDS_IPFIX_PARSERS_H
#define LIBFDS_IPFIX_PARSERS_H

#ifdef __cplusplus
extern "C" {
#endif

/** Size of an internal error buffer */
#include <stdint.h>
#include "api.h"
#include "template.h"
#include "ipfix_structs.h"

/** Iterator over IPFIX Sets in an IPFIX Message                */
struct fds_sets_iter {
    /** Pointer to the current IPFIX Set header                 */
    struct fds_ipfix_set_hdr *set;

    struct {
        /** Pointer to the start of the next IPFIX Set          */
        uint8_t *set_next;
        /** First byte after the end of the message             */
        uint8_t *msg_end;
        /** Error buffer                                        */
        const char *err_msg;
    } _private; /**< Internal structure (dO NOT use directly!)  */
};

/**
 * \brief Initialize IPFIX Set iterator
 * \warning
 *   After initialization the iterator has initialized only internal structures but public part
 *   is still undefined i.e. doesn't point to the first Set in the Message. To get the first field
 *   see fds_sets_iter_next().
 * \warning
 *   The IPFIX Message header is not checked. Make sure that its length is at
 *   least >= ::FDS_IPFIX_MSG_HDR_LEN.
 * \param[in] it  Uninitialized iterator structure
 * \param[in] msg IPFIX Message to iterate through
 */
FDS_API void
fds_sets_iter_init(struct fds_sets_iter *it, struct fds_ipfix_msg_hdr *msg);

/**
 * \brief Get the next Set in the Message
 *
 * Move the iterator to the next Set in the order, If this function was not previously called after
 * initialization by fds_sets_iter_init(), then the iterator will point to the first Set in the
 * Message.
 *
 * \code{.c}
 *   struct fds_ipfix_msg_hdr *ipfix_msg = ...;
 *   struct fds_sets_iter it;
 *   fds_sets_iter_init(&it, ipfix_msg);
 *
 *   int rc;
 *   while ((rc = fds_sets_iter_next(&it)) == FDS_OK) {
 *      // Add your code here...
 *   }
 *
 *   if (rc != FDS_ERR_NOTFOUND) {
 *      fprintf(stderr, "Error: %s\n", fds_sets_iter_err(&it));
 *   }
 * \endcode
 * \param[in] it Pointer to the iterator
 * \return If the next Set is prepared, returns #FDS_OK. If no more Sets are available, returns
 *   #FDS_ERR_NOTFOUND. Otherwise (invalid Message format) returns #FDS_ERR_FORMAT and fills
 *   an error message (see fds_sets_iter_err()).
 */
FDS_API int
fds_sets_iter_next(struct fds_sets_iter *it);

/**
 * \brief Get the last error message
 * \note The message is statically allocated string that can be passed to other function even
 *   when the iterator doesn't exist anymore.
 * \param[in] it Iterator
 * \return The error message.
 */
FDS_API const char *
fds_sets_iter_err(const struct fds_sets_iter *it);

// -------------------------------------------------------------------------------------------------

/** Iterator over data records in an IPFIX Data Set  */
struct fds_dset_iter {
    /** Start of the data record           */
    uint8_t *rec;
    /** Size of the data record (in bytes) */
    uint16_t size;

    struct {
        /** Template */
        const struct fds_template *tmplt;
        /** Pointer to the start of the next record */
        uint8_t *rec_next;
        /** First byte after end of the data set    */
        uint8_t *set_end;
        /** Error buffer                            */
        const char *err_msg;
    } _private; /**< Internal structure (dO NOT use directly!)  */
};

/**
 * \brief Initialize IPFIX Data records iterator
 *
 * \warning
 *   After initialization the iterator has initialized only internal structures but public part
 *   is still undefined i.e. doesn't point to the first Set in the Message. To get the first field
 *   see fds_dset_iter_next().
 * \warning
 *   Make sure that the length of allocated memory of the Set is at least the same as the length
 *   from the Set header before using the iterator.
 * \param[in] it    Uninitialized structure
 * \param[in] set   Data set header to iterate through
 * \param[in] tmplt Parsed template of data records
 */
FDS_API void
fds_dset_iter_init(struct fds_dset_iter *it, struct fds_ipfix_set_hdr *set,
    const struct fds_template *tmplt);

/**
 * \brief Get the next Data Record in the Data Set
 *
 * Move the iterator to the next Record in the order, If this function was not previously called
 * after initialization by fds_dset_iter_init(), then the iterator will point to the first Record
 * in the Data Set.
 *
 * \code{.c}
 *   struct fds_ipfix_set_hdr *data_set = ...;
 *   const struct fds_template *tmplt = ...;
 *   struct fds_dset_iter it;
 *   fds_dset_iter_init(&it, data_set, tmplt);
 *
 *   int rc;
 *   while ((rc = fds_dset_iter_next(&it)) == FDS_OK) {
 *      // Add your code here...
 *   }
 *
 *   if (rc != FDS_ERR_NOTFOUND) {
 *      fprintf(stderr, "Error: %s\n", fds_dset_iter_err(&it));
 *   }
 * \endcode
 * \param[in] it Pointer to the iterator
 * \return If the next Record is prepared, returns #FDS_OK. If no more Records are available,
 *   returns #FDS_ERR_NOTFOUND. Otherwise (invalid Data Set format) returns #FDS_ERR_FORMAT and
 *   fills an error message (see fds_dset_iter_err()).
 */
FDS_API int
fds_dset_iter_next(struct fds_dset_iter *it);

/**
 * \brief Get the last error message
 * \note The message is statically allocated string that can be passed to other function even
 *   when the iterator doesn't exist anymore.
 * \param[in] it Iterator
 * \return The error message
 */
FDS_API const char *
fds_dset_iter_err(const struct fds_dset_iter *it);

// -------------------------------------------------------------------------------------------------


// TODO: sepsat, ze kdyz kontoruje vsechny chyby... parsuje i vsechny typy
// TODO: tj. withdrawal, all withdrawal... atd. Vraci jen pokud data jsou zkontrolovana...

/** Iterator over template records in an IPFIX Template Set  */
struct fds_tset_iter {
    union {
        /** Start of the Template Record            (field_cnt > 0 && scope_cnt == 0) */
        struct fds_ipfix_trec      *trec;
        /** Start of the Options Template Record    (field_cnt > 0 && scope_cnt > 0)  */
        struct fds_ipfix_opts_trec *opts_trec;
        /** Start of the Template Withdrawal Record (field_cnt == 0)                  */
        struct fds_ipfix_wdrl_trec *wdrl_trec;
    } ptr; /**< Pointer to the start of the template record                           */
    /** Size of the template record (in bytes)      */
    uint16_t size;
    /** Field count                                 */
    uint16_t field_cnt;
    /** Scope field count                           */
    uint16_t scope_cnt;

    struct {
        /** Type of templates                       */
        uint16_t type;
        /** Pointer to the start of the next record */
        uint8_t *rec_next;
        /** Start of the (Options) Template Set     */
        struct fds_ipfix_set_hdr *set_begin;
        /** First byte after end of the data set    */
        uint8_t *set_end;
        /** Error buffer                            */
        const char *err_msg;
    } _private; /**< Internal structure (dO NOT use directly!)  */
};

/**
 * \brief Initialize IPFIX Template records iterator
 *
 * \note Set ID of the \p set must be 2 (::FDS_IPFIX_SET_TMPLT) or 3 (::FDS_IPFIX_SET_OPTS_TMPLT).
 * \warning
 *   After initialization the iterator has initialized only internal structures but public part
 *   is still undefined i.e. doesn't point to the first Set in the Message. To get the first field
 *   see fds_tset_iter_next().
 * \warning
 *   Make sure that the length of allocated memory of the Set is at least the same as the length
 *   from the Set header before using the iterator.
 * \param[in] it  Uninitialized structure
 * \param[in] set (Options) Template Set header to iterate through
 */
FDS_API void
fds_tset_iter_init(struct fds_tset_iter *it, struct fds_ipfix_set_hdr *set);

/**
 * \brief Get the next (Options) Template Record in the Data Set
 *
 * Move the iterator to the next Record in the order, If this function was not previously called
 * after initialization by fds_dset_iter_init(), then the iterator will point to the first Record
 * in the (Options) Template Set.
 *
 * \code{.c}
 *   struct fds_ipfix_set_hdr *tmplt_set = ...;
 *   struct fds_tset_iter it;
 *   fds_tset_iter_init(&it, tmplt_set);
 *
 *   int rc;
 *   while ((rc = fds_tset_iter_next(&it)) == FDS_OK) {
 *      // Add your code here...
 *   }
 *
 *   if (rc != FDS_ERR_NOTFOUND) {
 *      fprintf(stderr, "Error: %s\n", fds_tset_iter_err(&it));
 *   }
 * \endcode
 * \param[in] it Pointer to the iterator
 * \return If the next Record is prepared, returns #FDS_OK. If no more Records are available,
 *   returns #FDS_ERR_NOTFOUND. Otherwise (invalid (Options) Template Set format) returns
 *   #FDS_ERR_FORMAT and fills an error message (see fds_tset_iter_err()).
 */
FDS_API int
fds_tset_iter_next(struct fds_tset_iter *it);

/**
 * \brief Get the last error message
 * \note The message is statically allocated string that can be passed to other function even
 *   when the iterator doesn't exist anymore.
 * \param[in] it Iterator
 * \return The error message
 */
FDS_API const char *
fds_tset_iter_err(const struct fds_tset_iter *it);

#ifdef __cplusplus
}
#endif

#endif // LIBFDS_IPFIX_PARSERS_H
