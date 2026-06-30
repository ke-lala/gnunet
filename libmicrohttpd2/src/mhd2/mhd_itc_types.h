/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2016-2024 Evgeny Grin (Karlson2k), Christian Grothoff

  GNU libmicrohttpd is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  GNU libmicrohttpd is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  Alternatively, you can redistribute GNU libmicrohttpd and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License, or (at your option) any later version, together
  with the eCos exception, as follows:

    As a special exception, if other files instantiate templates or
    use macros or inline functions from this file, or you compile this
    file and link it with other works to produce a work based on this
    file, this file does not by itself cause the resulting work to be
    covered by the GNU General Public License. However the source code
    for this file must still be made available in accordance with
    section (3) of the GNU General Public License v2.

    This exception does not invalidate any other reasons why a work
    based on this file might be covered by the GNU General Public
    License.

  You should have received copies of the GNU Lesser General Public
  License and the GNU General Public License along with this library;
  if not, see <https://www.gnu.org/licenses/>.
*/

/**
 * @file src/mhd2/mhd_itc_types.h
 * @brief  Types for platform-independent inter-thread communication
 * @author Karlson2k (Evgeny Grin)
 * @author Christian Grothoff
 *
 * Provides basic types for inter-thread communication.
 * Designed to be included by other headers.
 */
#ifndef MHD_ITC_TYPES_H
#define MHD_ITC_TYPES_H 1
#include "mhd_sys_options.h"

/* Force socketpair on native W32 */
#if defined(_WIN32) && ! defined(__CYGWIN__) && ! defined(MHD_ITC_SOCKETPAIR_)
#error MHD_ITC_SOCKETPAIR_ is not defined on native W32 platform
#endif /* _WIN32 && !__CYGWIN__ && !MHD_ITC_SOCKETPAIR_ */

#if defined(MHD_ITC_EVENTFD_)
/* **************** Optimised ITC implementation by eventfd ********** */

/**
 * Data type for a MHD ITC.
 */
struct mhd_itc
{
  int fd;
};

/**
 * Static initialiser for struct mhd_itc
 */
#  define mhd_ITC_STATIC_INIT_INVALID { -1 }


#elif defined(MHD_ITC_PIPE_)
/* **************** Standard UNIX ITC implementation by pipe ********** */

/**
 * Data type for a MHD ITC.
 */
struct mhd_itc
{
  int fd[2];
};

/**
 * Static initialiser for struct mhd_itc
 */
#  define mhd_ITC_STATIC_INIT_INVALID { { -1, -1 } }


#elif defined(MHD_ITC_SOCKETPAIR_)
/* **************** ITC implementation by socket pair ********** */

#  include "mhd_socket_type.h"

/**
 * Data type for a MHD ITC.
 */
struct mhd_itc
{
  MHD_Socket sk[2];
};

/**
 * Static initialiser for struct mhd_itc
 */
#  define mhd_ITC_STATIC_INIT_INVALID \
        { { MHD_INVALID_SOCKET, MHD_INVALID_SOCKET } }

#endif /* MHD_ITC_SOCKETPAIR_ */

#endif /* ! MHD_ITC_TYPES_H */
