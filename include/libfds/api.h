/**
 * \file include/libfds/api.h
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \brief Definitions for API functions
 * \date 2017
 */

/* Copyright (C) 2017 CESNET, z.s.p.o.
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

#ifndef LIBFDS_API_H_
#define LIBFDS_API_H_

/**
 * \def FDS_API
 * \brief Make an interface public outside
 *
 * If the compiler supports attribute to mark objects as hidden, mark all
 * objects as hidden and export only objects explicitly marked to be part of
 * the public API.
 */
#define FDS_API __attribute__((visibility("default")))

/** Status code for success                                                   */
#define FDS_OK                  (0)
/** Status code for the end of a context                                      */
#define FDS_EOC                 (-1)
/** Status code for memory allocation error                                   */
#define FDS_ERR_NOMEM           (-2)
/** Status code for invalid format of data                                    */
#define FDS_ERR_FORMAT          (-3)
/** Status code for an invalid argument or a combination of arguments         */
#define FDS_ERR_ARG             (-4)
/** Status code for not found                                                 */
#define FDS_ERR_NOTFOUND        (-5)
/** Status code for a truncation of a value                                   */
#define FDS_ERR_TRUNC           (-6)
/** Status code for insufficient buffer size for a result                     */
#define FDS_ERR_BUFFER          (-7)
/** Status code for denied operation                                          */
#define FDS_ERR_DENIED          (-8)
/** Status code for modification error                                         */
#define FDS_ERR_DIFF            (-9)

#endif /* LIBFDS_API_H_ */
