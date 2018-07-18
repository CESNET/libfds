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
#include <libfds/api.h>
#include "template.h"
#include "ipfix_structs.h"

/**
 * \defgroup fds_parsers IPFIX Parsers
 * \ingroup publicAPIs
 * \brief Parsers for IPFIX Message, Data Sets and (Options) Template Sets
 * \remark Based on RFC 7011, Section 3. (see https://tools.ietf.org/html/rfc7011#section-3)
 *
 * Parsers are implemented as simple iterators that also check correct consistency of IPFIX data
 * structures.
 *
 * @{
 */

/**
 * \defgroup fds_sets_iter IPFIX Sets iterator
 * \ingroup fds_parsers
 * \brief Iterator over IPFIX Sets in an IPFIX Message
 *
 * The iterator provides a simple way to go through all Sets in an IPFIX Message.
 * Every time a Sets is prepared by fds_sets_iter_next() it is guaranteed that the header and
 * the length of the Set is valid. Content of the Set is NOT checked for consistency because it is
 * provides by the \ref fds_dset_iter "IPFIX Data Set iterator" and the \ref fds_tset_iter
 * "IPFIX (Options) Template Set iterator".
 *
 * @{
 */

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
 *   if (rc != FDS_EOC) {
 *      fprintf(stderr, "Error: %s\n", fds_sets_iter_err(&it));
 *   }
 * \endcode
 * \param[in] it Pointer to the iterator
 * \return #FDS_OK on success an the next Set is ready to use.
 * \return #FDS_EOC if no more Sets are available.
 * \return #FDS_ERR_FORMAT if the format of the message is invalid (an appropriate error message
 *   is set - see fds_sets_iter_err()).
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

/**
 * @}
 *
 * \defgroup fds_dset_iter IPFIX Data Set iterator
 * \ingroup fds_parsers
 * \brief Iterator over IPFIX Data records in an IPFIX Data Set
 *
 * The iterator provides a simple way to go through all Data Records in an IPFIX Data Set.
 * Every time a Record is prepared by fds_dset_iter_next() it is guaranteed that the record
 * length is valid in a context of a corresponding IPFIX template. To iterate over the Data Record
 * or to find a specific field in it, you can use \ref fds_drec "Data record" tools.
 *
 * \warning
 *   Only for Data Sets, i.e. Set ID >= 256 (::FDS_IPFIX_SET_MIN_DSET). Using on different types
 *   of Sets cause undefined behaviour.
 *
 * @{
 */

/** Iterator over data records in an IPFIX Data Set  */
struct fds_dset_iter {
    /** Start of the data record           */
    uint8_t *rec;
    /** Size of the data record (in bytes) */
    uint16_t size;

    struct {
        /** Iterator flags                          */
        uint16_t flags;
        /** Template                                */
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
 *   if (rc != FDS_EOC) {
 *      fprintf(stderr, "Error: %s\n", fds_dset_iter_err(&it));
 *   }
 * \endcode
 * \param[in] it Pointer to the iterator
 * \return #FDS_OK on success and the next Record is ready to use.
 * \return #FDS_EOC if no more Records are available (the end of the Set has been reached).
 * \return #FDS_ERR_FORMAT if the format of the Data Set is invalid (an appropriate error message
 *   is set - see fds_dset_iter_err()).
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


/**
 * @}
 *
 * \defgroup fds_tset_iter IPFIX (Options) Template Set iterator
 * \ingroup fds_parsers
 * \brief Iterator over IPFIX (Options) Template Records in an IPFIX (Options) Template Set
 *
 * The iterator provides a simple way to go through all (Options) Template Records in an IPFIX
 * (Options) Template Set. Every time a Record is prepared by fds_tset_iter_next() it is
 * guaranteed that the record is fully valid. Each record can be then parsed using
 * fds_template_parse() function.
 *
 * Keep in mind that (Options) Template Set can contain also (Options) Template Withdrawals.
 * Therefore, you have to determine type of the template during later processing. Withdrawal
 * records always have zero Field count. On the other hand, (Options) Template definitions have
 * non-zero field count.
 * \note
 *   Template definitions and Withdrawals cannot be mixed within one (Options) Template Set
 *   and the iterator makes sure that this rule is followed correctly in the Set. Also, don't
 *   forget do distinguish (Options) Template Withdrawals and All (Options) Template Withdrawals.
 *
 * \warning
 *   Only for Template Sets and Options Template Sets, i.e. Set ID == 2 (::FDS_IPFIX_SET_TMPLT)
 *   or Set ID == 3 (::FDS_IPFIX_SET_OPTS_TMPLT). Using on different types of Sets cause undefined
 *   behaviour.
 *
 * @{
 */

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
        /** Iterator flags                          */
        uint16_t flags;
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
 * \note
 *   Set ID of the \p set MUST be 2 (::FDS_IPFIX_SET_TMPLT) or 3 (::FDS_IPFIX_SET_OPTS_TMPLT).
 *   Otherwise behavior of the parser is undefined.
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
 *   if (rc != FDS_EOC) {
 *      fprintf(stderr, "Error: %s\n", fds_tset_iter_err(&it));
 *   }
 * \endcode
 * \param[in] it Pointer to the iterator
 * \return #FDS_OK on success and the Template Record is ready to use.
 * \return #FDS_EOC if no more Template Records are available (the end of the Set has been reached).
 * \return #FDS_ERR_FORMAT if the format of the Template Set is invalid (an appropriate error
 *   message is set - see fds_tset_iter_err()).
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


/**
 * @}
 *
 * \defgroup fds_blist_iter IPFIX Basic list iterator
 * \ingroup fds_parsers
 * \brief Iterator over Fields in Basic list
 *
 * The iterator provides a simple way to go trough all the fields in Basic list data type.
 * This iterator can be used only on the Basic list data type which represents
 * a list of zero or more instances of an Information Element.
 * If Information Element manager is provided during the initialization of the iterator, the definition
 * of the field is automatically filled.
 *
 *
 * @{
 */

/** Iterator over basic list fields in an IPFIX Basic list data type  */
struct fds_blist_iter {
    /** Current field     */
    struct fds_drec_field field;

    struct {
        /** Start of the basic list                 */
        struct fds_ipfix_blist *blist;

        struct fds_tfield *info;
        /** Pointer to the start of the next record */
        uint8_t *field_next;
        /** End of the basic list                   */
        uint8_t *blist_end;
        /** Error buffer                            */
        const char *err_msg;

    } _private; /**< Internal structure (dO NOT use directly!)  */
};

/**
 * \brief Initialize Basic list data type iterator
 *
 * \warning
 *   After initialization the iterator has initialized only internal structures but public part
 *   is still undefined i.e. doesn't point to the first Field in the list. To get the first field
 *   see fds_blist_iter_next().
 * \param[in] it Uninitialized structure of iterator
 * \param[in] field Field with data type of Basic list
 * \param[in] ie_mgr IE manager for finding field definition (can be NULL)
 * \return #FDS_OK Successful initialization
 * \return #FDS_ERR_FORMAT Field is shorter than minimal size of Basic list
 */
int
fds_blist_iter_init(struct fds_blist_iter *it, struct fds_drec_field *field,  fds_iemgr_t *ie_mgr);

/**
 * \brief Get the next field in the Basic list
 *
 * Move the iterator to the next Field in the order, If this function was not previously called
 * after initialization by fds_blist_iter_init(), then the iterator will point to the first field
 * in the Basic list.
 *
 * \code{.c}
 *  struct fds_blist_iter blist_it;
 *  fds_blist_iter_init(&blist_it, field, ie_manager[or NULL]);
 *
 *  int rc;
 *  while ((rc = fds_blist_iter_next(&blist_it)) == FDS_OK) {
 *      // Add your code here...
 *  }
 *
 *  if (rc != FDS_EOC) {
 *      // error message fds_blist_iter_err();
 *  }
 * \endcode
 *
 * \param[in] it Basic list iterator
 * \return #FDS_OK if the next Field is prepared.
 * \return #FDS_EOC if no more Fields are available.
 * \return #FDS_ERR_FORMAT otherwise and fills an error message (see fds_blist_iter_err()).
 */
int
fds_blist_iter_next(struct fds_blist_iter *it);

/**
 * \brief Get the last error message
 * \note The message is statically allocated string that can be passed to other function even
 *   when the iterator doesn't exist anymore.
 * \param[in] it Iterator
 * \return The error message
 */
const char *
fds_blist_iter_err(const struct fds_blist_iter *it);


#ifdef __cplusplus
}
#endif

#endif // LIBFDS_IPFIX_PARSERS_H

/**
 * @}
 * @}
 */
