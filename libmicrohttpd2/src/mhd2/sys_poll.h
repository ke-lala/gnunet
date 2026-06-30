/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2024 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/sys_poll.h
 * @brief  The header for the system 'poll()' function and related data types
 * @author Karlson2k (Evgeny Grin)
 *
 * This header includes system macros for 'poll()' and also has related
 * MHD macros.
 */

#ifndef MHD_SYS_POLL_H
#define MHD_SYS_POLL_H 1

#include "mhd_sys_options.h"

#ifdef MHD_SUPPORT_POLL
#  include "mhd_socket_type.h"
#  if defined(MHD_SOCKETS_KIND_POSIX)
#    include <poll.h>
#    define mhd_poll poll
#  elif defined(MHD_SOCKETS_KIND_WINSOCK)
#    include <winsock2.h>
#    define mhd_poll WSAPoll
#  else
#error Uknown sockets type
#  endif

#  if defined(HAVE_DCLR_POLLRDNORM) || defined(POLLRDNORM)
#    define MHD_POLL_IN POLLRDNORM
#  else
#    define MHD_POLL_IN POLLIN
#  endif

#  if defined(HAVE_DCLR_POLLWRNORM) || defined(POLLWRNORM)
#    define MHD_POLL_OUT POLLWRNORM
#  else
#    define MHD_POLL_OUT POLLOUT
#  endif

#  if defined(HAVE_DCLR_POLLRDBAND) || defined(POLLRDBAND)
#    define MHD_POLLRDBAND POLLRDBAND
#  else
#    define MHD_POLLRDBAND (0)
#  endif

#  if defined(HAVE_DCLR_POLLWRBAND) || defined(POLLWRBAND)
#    define MHD_POLLWRBAND POLLWRBAND
#  else
#    define MHD_POLLWRBAND (0)
#  endif

#  if defined(HAVE_DCLR_POLLPRI) || defined(POLLWRBAND)
#    define MHD_POLLPRI POLLPRI
#  else
#    define MHD_POLLPRI (0)
#  endif


#  if (defined(__APPLE__) && defined(__MACH__)) || defined(__CYGWIN__)
/* The platform incorrectly sets POLLHUP when remote use SHUT_WR.
   The correct behaviour must be POLLHUP only on remote close/disconnect */
#    define MHD_POLLHUP_ON_REM_SHUT_WR 1
#  endif

#endif /* MHD_SUPPORT_POLL */

#endif /* ! MHD_SYS_POLL_H */
