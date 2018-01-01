//
// Created by lukashutak on 31.12.17.
//

#ifndef LIBFDS_COMMON_TESTS_H
#define LIBFDS_COMMON_TESTS_H

#include <libfds.h>

/**
 * \brief Test template flags
 * \param[in] tmplt Template
 * \param[in] exp   Expected flags
 */
void
ct_template_flags(const fds_template *tmplt, fds_template_flag_t exp);

/**
 * \brief Test template field flags
 * \param[in] tfield  Template field
 * \param[in] exp     Expected flags
 */
void
ct_tfield_flags(const fds_tfield *tfield, fds_template_flag_t exp);

#endif //LIBFDS_COMMON_TESTS_H
