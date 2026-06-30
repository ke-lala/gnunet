/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2025-2026 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/mhd_dbg_print.h
 * @brief  The declarations of internal debug-print/trace helpers
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_DBG_PRINT_H
#define MHD_DBG_PRINT_H 1

#include "mhd_sys_options.h"

#ifdef MHD_USE_TRACE_POLLING_FDS
#  include "sys_bool_type.h"
#  include "mhd_socket_type.h"
#  include "sys_kqueue.h"
#endif /* MHD_USE_TRACE_POLLING_FDS */


#ifdef MHD_USE_TRACE_POLLING_FDS
/**
 * Debug-printf request of FD polling/monitoring
 * @param fd_name the name of FD ("ITC", "lstn" or "conn")
 * @param fd the FD value
 * @param r_ready the request for read (or receive) readiness
 * @param w_ready the request for write (or send) readiness
 * @param e_ready the request for exception (or error) readiness
 * @note Implemented in src/mhd2/events_process.c
 */
MHD_INTERNAL void
mhd_dbg_print_fd_mon_req (const char *fd_name,
                          MHD_Socket fd,
                          bool r_ready,
                          bool w_ready,
                          bool e_ready)
MHD_FN_PAR_NONNULL_ALL_;

#  ifdef MHD_SUPPORT_KQUEUE
/**
 * Debug print kqueue event request update or event report
 * @param fd_name the name of FD ("ITC", "lstn" or "conn")
 * @param ke the pointer to kevent
 * @param update_req set to 'true' if @a ke is in the change list,
 *                   set to 'false' if @a ke is in the reported events list
 */
MHD_INTERNAL void
mhd_dbg_print_kevent (const char *fd_name,
                      const struct kevent *ke,
                      bool update_req)
MHD_FN_PAR_NONNULL_ALL_;

#  endif /* MHD_SUPPORT_KQUEUE */

#else  /* ! MHD_USE_TRACE_POLLING_FDS */
#  define mhd_dbg_print_fd_mon_req(fd_n,fd,r_ready,w_ready,e_ready) ((void) 0)
#  ifdef MHD_SUPPORT_KQUEUE
#    define mhd_dbg_print_kevent(fd_name,ke,update_req)     ((void) 0)
#  endif /* MHD_SUPPORT_KQUEUE */
#endif /* ! MHD_USE_TRACE_POLLING_FDS */

#ifdef MHD_SUPPORT_KQUEUE
/**
 * Debug print kqueue event request update
 * @param fd_name the name of FD ("ITC", "lstn" or "conn")
 * @param ke the pointer to kevent
 */
#  define mhd_dbg_print_kevent_change(fd_name,ke) \
        mhd_dbg_print_kevent ((fd_name),(ke),true)

/**
 * Debug print kqueue event report
 * @param fd_name the name of FD ("ITC", "lstn" or "conn")
 * @param ke the pointer to kevent
 */
#  define mhd_dbg_print_kevent_report(fd_name,ke) \
        mhd_dbg_print_kevent ((fd_name),(ke),false)
#endif /* MHD_SUPPORT_KQUEUE */

#endif /* ! MHD_DBG_PRINT_H */
