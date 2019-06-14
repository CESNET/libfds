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
    } _private; /**< Internal structure (do NOT use directly!)  */
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
 * \param[in] it  Iterator structure to initialize
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
 * \brief Iterator over IPFIX Data Records in an IPFIX Data Set
 *
 * The iterator provides a simple way to go through all Data Records in an IPFIX Data Set.
 * Every time a Record is prepared by fds_dset_iter_next() it is guaranteed that the record
 * length is valid in a context of a corresponding IPFIX template. To iterate over the Data Record
 * or to find a specific field in it, you can use \ref fds_drec "Data Record" tools.
 *
 * \warning
 *   Only for Data Sets, i.e. Set ID >= 256 (::FDS_IPFIX_SET_MIN_DSET). Using on different types
 *   of Sets cause undefined behaviour.
 *
 * @{
 */

/** Iterator over Data Records in an IPFIX Data Set  */
struct fds_dset_iter {
    /** Start of the Data Record           */
    uint8_t *rec;
    /** Size of the Data Record (in bytes) */
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
    } _private; /**< Internal structure (do NOT use directly!)  */
};

/**
 * \brief Initialize IPFIX Data Records iterator
 *
 * \warning
 *   After initialization the iterator has initialized only internal structures but public part
 *   is still undefined i.e. doesn't point to the first Record in the Data Set. To get the first
 *   field see fds_dset_iter_next().
 * \warning
 *   Make sure that the length of allocated memory of the Set is at least the same as the length
 *   from the Set header before using the iterator. Not required if you got the Set from
 *   fds_sets_iter_next().
 * \param[in] it    Uninitialized structure
 * \param[in] set   Data set header to iterate through
 * \param[in] tmplt Parsed template of Data Records
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

/** Iterator over template records in an IPFIX (Options) Template Set  */
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
 * \brief Initialize IPFIX (Options) Template records iterator
 *
 * \note
 *   Set ID of the \p set MUST be 2 (::FDS_IPFIX_SET_TMPLT) or 3 (::FDS_IPFIX_SET_OPTS_TMPLT).
 *   Otherwise behavior of the parser is undefined.
 * \warning
 *   After initialization the iterator has initialized only internal structures but public part
 *   is still undefined i.e. doesn't point to the first Record in the Set. To get the first field
 *   see fds_tset_iter_next().
 * \warning
 *   Make sure that the length of allocated memory of the Set is at least the same as the length
 *   from the Set header before using the iterator. Not required if you got the Set from
 *   fds_sets_iter_next().
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
 * \defgroup fds_blist_iter IPFIX basicList iterator
 * \ingroup fds_parsers
 * \brief Iterator over Fields in basicList
 *
 * The iterator provides a simple way to go trough all the fields in basicList data type.
 * This iterator can be used only on basicList data type which represents a list of zero or
 * more instances of an Information Element. If an Information Element manager is provided during
 * the initialization of the iterator, the definition of the field is automatically filled.
 *
 * @{
 */

/** Iterator over fields in an IPFIX basiclist data type  */
struct fds_blist_iter {
    /** Prepared field                              */
    struct fds_drec_field field;
    /** Semantic of the basicList                   */
    enum fds_ipfix_list_semantics semantic;

    struct {
        /** Start of the basicList                     */
        struct fds_ipfix_blist *blist;
        /** Info about the single field in the list    */
        struct fds_tfield info;
        /** Pointer to the start of the next record    */
        uint8_t *field_next;
        /** First byte after the end of this basicList */
        uint8_t *blist_end;
        /** Error buffer                               */
        const char *err_msg;
        /** Last error code                            */
        int err_code;
    } _private; /**< Internal structure (do NOT use directly!)  */
};

/**
 * \brief Initialize basicList data type iterator
 *
 * \warning
 *   After initialization the iterator has initialized only private structures but in the public
 *   part only semantic is specified. However, the content of the Data Field is still undefined i.e.
 *   it doesn't point to the first field in the list. To get the first field see
 *   fds_blist_iter_next().
 * \note
 *   If Information Manager \p ie_mgr is defined, the iterator will try to find definition of the
 *   field and properly fill \ref fds_tfield.def pointer. However, keep on mind that the definition
 *   could be missing in the manager. In this case (or if \p ie_mgr is NULL), the reference is set
 *   to NULL.
 * \param[in] it     Iterator structure to initialize
 * \param[in] field  Field with data type of
 * \param[in] ie_mgr IE manager for finding field definition (can be NULL)
 */
FDS_API void
fds_blist_iter_init(struct fds_blist_iter *it, struct fds_drec_field *field,
    const fds_iemgr_t *ie_mgr);

/**
 * \brief Get the next field in the basicList
 *
 * Move the iterator to the next field in the order, If this function was not previously called
 * after initialization by fds_blist_iter_init(), then the iterator will point to the first field
 * in the basicList.
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
 * \param[in] it basicList iterator
 * \return #FDS_OK if the next field is prepared.
 * \return #FDS_EOC if no more fields are available.
 * \return #FDS_ERR_FORMAT otherwise and fills an error message (see fds_blist_iter_err()).
 */
FDS_API int
fds_blist_iter_next(struct fds_blist_iter *it);

/**
 * \brief Get the last error message
 * \note The message is statically allocated string that can be passed to other function even
 *   when the iterator doesn't exist anymore.
 * \param[in] it Iterator
 * \return The error message
 */
FDS_API const char *
fds_blist_iter_err(const struct fds_blist_iter *it);


/**
 * @}
 *
 * \defgroup fds_stlist_iter IPFIX subTemplateList and subTemplateMultiList iterator
 * \ingroup fds_parsers
 * \brief Iterators over Data Records in subTemplateList and subTemplateMultiList
 *
 * The iterators provide a simple way to go trough all the Records in subTemplateList and
 * subTemplateMultiList data type.
 *
 * The type "subTemplateList" represents a list of zero or more instances of a structured data
 * type, where the data type of each list element is the same and corresponds with a single
 * Template Record. There is also semantic information which represent the relationship among
 * the Data Records.
 *
 * The type "subTemplateMultiList" represents a list of zero or more instances of a structured
 * data type, where the data type of each list element can be different and corresponds with
 * different Template definitions. Internally, the list is divided into blocks of Data Records with
 * the same Template ID (similar to Data Set structure). There is also semantic information which
 * represent the top-level relationship among the blocks.
 *
 * @{
 */

/**
 * \brief Flags for the iterator over subTemplateList and subTemplateMultiList
 */
enum fds_stl_flags {
    /**
     * \brief Report a missing template
     *
     * If a template necessary for decoding Data Records is not found, the Data Records
     * in the list won't be skipped but iterator will return #FDS_ERR_NOTFOUND.
     */
    FDS_STL_REPORT              = (1 << 0),
};

/**
 * \brief Iterator over subTemplateList data type
 */
struct fds_stlist_iter {
    /** Current Data Record                         */
    struct fds_drec rec;
    /** Template ID of all records in the list      */
    uint16_t tid;
    /** The relationship among the different Data Records within this list */
    enum fds_ipfix_list_semantics semantic;

    struct {
        /** Next record in the list                 */
        uint8_t *rec_next;
        /** First byte after the end of the list    */
        uint8_t *list_end;
        /** Flags that has been set up during init  */
        uint16_t flags;
        /** Error code                              */
        int err_code;
        /** Error message                           */
        const char *err_msg;
    } private__; /**< Internal structure (do NOT use directly!)  */
};

/**
 * \brief Initialize iterator of subTemplateList data type
 *
 * \warning
 *   After initialization the iterator has initialized only private structures but in the public
 *   part only semantic, and Template ID are specified. However, the content of the Data Record is
 *   still partly undefined i.e. doesn't point to the first Field in the list. To get the first
 *   Data Record see fds_stlist_iter_next().
 *
 * \param[in] it    Iterator structure to initialize
 * \param[in] field Field which is encoded as subTemplateList
 * \param[in] snap  Template snapshot for IPFIX Template lookup
 * \param[in] flags Iterator flags (default 0)
 */
FDS_API void
fds_stlist_iter_init(struct fds_stlist_iter *it, struct fds_drec_field *field,
    const fds_tsnapshot_t *snap, uint16_t flags);

/**
 * \brief Get the next Data Record from the list
 *
 * Move the iterator to the next Data Record in the order, If this function was not previously
 * called after initialization by fds_stlist_iter_init(), then the iterator will point to the
 * first Data Record in the list.
 *
 * \note
 *   If values doesn't change between records (such as a Template reference, Template ID, etc.)
 *   the iterator doesn't update them.
 *
 * \note
 *   By default, the iterator automatically skips Data Records if a Template required to interpret
 *   them is not present in the Template snapshot. In other words, because all Templates in the
 *   subTemplateList are based on the same Template, if the Template is not found,
 *   this function will immediately return #FDS_EOC. However, if a user wants to be notified
 *   that the Template is missing, the iterator must be initialized with #FDS_STL_REPORT flag.
 *   In that case, error code #FDS_ERR_NOTFOUND is returned and Template ID
 *   (i.e. \ref fds_stlist_iter.tid ) of the Template is set appropriately.
 *
 * \code{.c}
 *  struct fds_drec *record = ...;
 *  struct fds_drec_field *field = ...; // A one of fields from the record
 *
 *  struct fds_stlist_iter stlist_it;
 *  fds_stlist_iter_init(&stlist_it, field, record->snapshot, 0);
 *
 *  int rc;
 *  while ((rc = fds_stlist_iter_next(&stlist_it)) == FDS_OK) {
 *      // Add your code here...
 *  }
 *
 *  if (rc != FDS_EOC) {
 *      // error message fds_stlist_iter_err();
 *  }
 * \endcode
 *
 * \param[in] it Iterator which will be updated to point to the next Data Record
 * \return #FDS_OK if the iterator was successfully updated.
 * \return #FDS_EOC if there are no more Data Records to read.
 * \return #FDS_ERR_NOTFOUND if #FDS_STL_REPORT flag has been set during iterator initialization
 *   and the iterator is unable to find a Template to interpret Data Record(s).
 * \return #FDS_ERR_FORMAT if the format of the list is invalid and the iterator cannot continue.
 *   (an appropriate error message is set, see fds_stlist_iter_err())
 */
FDS_API int
fds_stlist_iter_next(struct fds_stlist_iter *it);

/**
 * \brief Get the last error message
 * \note The message is statically allocated string that can be passed to other function even
 *   when the iterator doesn't exist anymore.
 * \param[in] it Iterator
 * \return The error message
 */
FDS_API const char *
fds_stlist_iter_err(struct fds_stlist_iter *it);


/**
 * \brief Iterator over subTemplateMultiList data type
 */
struct fds_stmlist_iter {
    /** Current Data Record in the current block (inner Data Set)      */
    struct fds_drec rec;
    /** Template ID of the all records in the current block            */
    uint16_t tid;
    /**
     * The top-level relationship among blocks (the series of Data Records) corresponding to
     * the different Template Records within this list.
     */
    enum fds_ipfix_list_semantics semantic;

    struct {
        /** Start of the next record in the current block              */
        uint8_t *rec_next;
        /** Start of the next block (inner Data Set)                   */
        uint8_t *block_next;
        /** First byte after the end of the subTemplateMultiList       */
        uint8_t *list_end;
        /** Template snapshot                                          */
        const fds_tsnapshot_t *snap;
        /** Template used to decode the current block (inner Data Set) */
        const struct fds_template *tmplt;
        /** Flags that has been set up during init                     */
        uint16_t flags;
        /** Error code                                                 */
        int err_code;
        /** Error message                                              */
        const char *err_msg;
    } private__; /**< Internal structure (do NOT use directly!)  */
};

/**
 * \brief Initialize iterator of subTemplateMultiList data type
 *
 * \warning
 *   After initialization the iterator has fully initialized only private structures but in the
 *   public part only semantic is specified. However, the content of the Data Record and Template
 *   ID is undefined. To get the first Record see fds_stmlist_iter_next_block() and
 *   fds_stmlist_iter_next_rec().
 *
 * \param[in] it    Iterator structure to initialize
 * \param[in] field Field which is encoded as subTemplteMultiList
 * \param[in] snap  Template snapshot for IPFIX Template lookup
 * \param[in] flags Iterator flags (default 0)
 */
FDS_API void
fds_stmlist_iter_init(struct fds_stmlist_iter *it, struct fds_drec_field *field,
    const fds_tsnapshot_t *snap, uint16_t flags);

/**
 * \brief Get the next block of Data Records
 *
 * Move the iterator to the next block (of Data Records with the same Template) in the order.
 * There is a semantic specified among these blocks, which is filled in the public part.
 * If this function wasn't previously called after initialization by fds_stmlist_iter_init(), then
 * the iterator will point to the first block in the list.
 *
 * \note
 *   By default, the iterator automatically skips blocks if a Template required to interpret
 *   them is not present in the Template snapshot. However, if the user wants to be notified,
 *   the iterator must be initialized with #FDS_STL_REPORT flag. In that case whenever a required
 *   Template is missing, error code #FDS_ERR_NOTFOUND is returned and Template ID
 *   (i.e. \ref fds_stmlist_iter.tid ) is set. These uninterpretable blocks can be skipped
 *   by calling this function again.
 *
 * \warning
 *   After calling this function, the Data Record in the public part is still undefined!
 *   To get records in the block you have to use fds_stmlist_iter_next_rec(). However, the public
 *   Template ID is already determined at the moment.
 *
 * \param[in] it Iterator which will be updated to point to the next block.
 * \return #FDS_OK if the iterator was successfully updated.
 * \return #FDS_EOC if there are no more blocks in the list.
 * \return #FDS_ERR_NOTFOUND if #FDS_STL_REPORT flag has been set during iterator initialization
 *   and the iterator is unable to find an IPFIX Template to interpret the next block of Data
 *   Record(s). This is not a fatal error i.e. the user can continue to iterate and ignore this
 *   warning.
 * \return #FDS_ERR_FORMAT if the format of the list is invalid and the iterator cannot continue.
 *   (an appropriate error message is set, see fds_stmlist_iter_err())
 */
FDS_API int
fds_stmlist_iter_next_block(struct fds_stmlist_iter *it);

/**
 * \brief Get the next Data Record from the current block
 *
 * Move the iterator to the next Data Record in the order. If this function was not previously
 * called after moving to the next block by fds_stmlist_iter_next_block(), then the iterator will
 * point to the first Data Record in the current block.
 *
 * Keep on mind that the block could be hypothetically empty, in that case, this function will
 * immediately return #FDS_EOC.
 *
 * \code{.c}
 *  struct fds_drec *record = ...;
 *  struct fds_drec_field *field = ...; // A one of fields from the above record
 *
 *  struct fds_stmlist_iter it;
 *  fds_stlist_iter_init(&it, field, record->snapshot, 0);
 *
 *  int rc_block, rc_rec;
 *  // For each block in the list
 *  while ((rc_block = fds_stmlist_iter_next_block(&it)) == FDS_OK) {
 *      // For each Data Record in the block
 *      while ((rc_rec = fds_stmlist_iter_next_rec(&it)) == FDS_OK) {
 *          // Do something with the Data Record
 *      }
 *
 *      if (rc_rec != FDS_EOC) {
 *          // Something went wrong...
 *          break;
 *      }
 *  }
 *
 *  if (rc_block != FDS_EOC || rc_rec != FDS_EOC) {
 *      // error message from fds_stmlist_iter_err();
 *  }
 * \endcode
 *
 * \warning
 *   If the fds_stmlist_iter_next_block() hasn't been called after iterator initialization,
 *   the results of this functions are undefined!
 *
 * \param[in] it Iterator which will be updated to point to the next Data Record
 * \return #FDS_OK if the iterator was successfully updated.
 * \return #FDS_EOC if there are no more Data Records in the block to read.
 * \return #FDS_ERR_FORMAT if the format of the list is invalid and the iterator cannot continue.
 *   (an appropriate error message is set, see fds_stmlist_iter_err())
 */
FDS_API int
fds_stmlist_iter_next_rec(struct fds_stmlist_iter *it);

/**
 * \brief Get the last error message
 * \note The message is statically allocated string that can be passed to other function even
 *   when the iterator doesn't exist anymore.
 * \param[in] it Iterator
 * \return The error message
 */
FDS_API const char *
fds_stmlist_iter_err(struct fds_stmlist_iter *it);

/**
 * @}
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif // LIBFDS_IPFIX_PARSERS_H
