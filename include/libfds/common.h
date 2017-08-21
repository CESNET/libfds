/**
 * \file   include/libfds/common.h
 * \author Michal Režňák <xrezna04@stud.fit.vutbr.cz>
 * \brief  Common definitions for libfds
 * \date   21. August 2017
 */

/* Copyright (C) 2016 CESNET, z.s.p.o.
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

/** Status code for success                                                   */
#define FDS_OK          (0)
/** Status code for memory allocation error                                   */
#define FDS_ERR_NOMEM  (-1)
/** Status code for the end of a context                                      */
#define FDS_EOC        (-2)
/** Invalid format description                                                */
#define FDS_ERR_FMT    (-3)
/** Status code for different modification time                               */
#define FDS_DIFF_MTIME (-4)
/** Status code when some element cannot be found                             */
#define FDS_NOT_FOUND  (-5)
