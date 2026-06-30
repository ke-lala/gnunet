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
 * @file src/mhd2/events_process.h
 * @brief  The declarations of events processing functions
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_EVENTS_PROCESS_H
#define MHD_EVENTS_PROCESS_H 1

#include "mhd_sys_options.h"

#include "sys_base_types.h"

#ifdef MHD_SUPPORT_THREADS
#  include "sys_thread_entry_type.h"
#endif /* MHD_SUPPORT_THREADS */

struct MHD_Daemon; /* forward declaration */

#ifdef MHD_SUPPORT_THREADS

/**
 * The entry point for the daemon worker thread
 * @param cls the closure
 */
mhd_THRD_RTRN_TYPE mhd_THRD_CALL_SPEC
mhd_worker_all_events (void *cls);

/**
 * The entry point for the daemon listening thread
 * @param cls the closure
 */
mhd_THRD_RTRN_TYPE mhd_THRD_CALL_SPEC
mhd_worker_listening_only (void *cls);

/**
 * The entry point for the connection thread for thread-per-connection mode
 * @param cls the closure
 */
mhd_THRD_RTRN_TYPE mhd_THRD_CALL_SPEC
mhd_worker_connection (void *cls);

#endif /* MHD_SUPPORT_THREADS */

/**
 * Get maximum wait time for the daemon
 * @param d the daemon to check
 * @return the maximum wait time,
 *         #MHD_WAIT_INDEFINITELY if wait time is not limited
 */
MHD_INTERNAL uint_fast64_t
mhd_daemon_get_wait_max (const struct MHD_Daemon *restrict d)
MHD_FN_PAR_NONNULL_ALL_;

/**
 * Close all daemon connections.
 * Must not be called when any other connections processing function is running
 * @param d the daemon to use
 */
MHD_INTERNAL void
mhd_daemon_close_all_conns (struct MHD_Daemon *d)
MHD_FN_PAR_NONNULL_ALL_;

#endif /* ! MHD_EVENTS_PROCESS_H */
