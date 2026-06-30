/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2025 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/extr_events_funcs.h
 * @brief  The definition of the external events internal helper functions
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_EXTR_EVENTS_FUNCS_H
#define MHD_EXTR_EVENTS_FUNCS_H 1

#include "mhd_sys_options.h"

#ifdef MHD_USE_TRACE_POLLING_FDS
#  include <stdio.h>
#  include "mhd_daemon.h"
#  include "mhd_assert.h"
#  include "sys_null_macro.h"
#endif /* MHD_USE_TRACE_POLLING_FDS */

#ifdef MHD_SUPPORT_LOG_FUNCTIONALITY

struct MHD_Daemon; /* forward declaration */

/**
 * Log message about failed de-registration of FDs
 */
MHD_INTERNAL void
mhd_log_extr_event_dereg_failed (struct MHD_Daemon *restrict d);

#else  /* ! MHD_SUPPORT_LOG_FUNCTIONALITY */

/**
 * Log message about failed de-registration of FDs (no-op implementation)
 */
#define mhd_log_extr_event_dereg_failed(d) ((void) 0)

#endif /* ! MHD_SUPPORT_LOG_FUNCTIONALITY */

#ifdef MHD_USE_TRACE_POLLING_FDS
/**
 * Call application event registration callback
 * @param d the daemon to use
 * @param fd the FD to register
 * @param watch_for events/statuses to watch for
 * @param app_cntx_old the previous application FD context
 * @param ecb_cntx the MHD FD context
 */
mhd_static_inline
MHD_FN_PAR_NONNULL_ (1) void *
mhd_daemon_extr_event_reg (struct MHD_Daemon *d,
                           MHD_Socket fd,
                           enum MHD_FdState watch_for,
                           void *app_cntx_old,
                           struct MHD_EventUpdateContext *ecb_cntx)
{
  void *res;
  char state_str[] = "x:x:x";
  const char *reg_type;
  const char *fd_rel;

  mhd_assert (mhd_WM_INT_HAS_EXT_EVENTS (d->wmode_int));
  mhd_assert (mhd_POLL_TYPE_EXT == d->events.poll_type);
  mhd_assert (mhd_SOCKET_REL_MARKER_EMPTY != (mhd_SockRelMarker) ecb_cntx);

  res =
    d->events.data.extr.cb_data.cb (d->events.data.extr.cb_data.cls,
                                    fd,
                                    watch_for,
                                    app_cntx_old,
                                    ecb_cntx);

  if (NULL == app_cntx_old)
    reg_type = "   Registration";
  else if (MHD_FD_STATE_NONE == watch_for)
    reg_type = "De-registration";
  else
    reg_type = "Re-registration";

  state_str[0] = MHD_FD_STATE_IS_SET_RECV (watch_for) ? 'R' : '-';
  state_str[2] = MHD_FD_STATE_IS_SET_SEND (watch_for) ? 'W' : '-';
  state_str[4] = MHD_FD_STATE_IS_SET_EXCEPT (watch_for) ? 'E' : '-';

  switch ((mhd_SockRelMarker) ecb_cntx)
  {
  case mhd_SOCKET_REL_MARKER_ITC:
    fd_rel = "ITC: ";
    break;
  case mhd_SOCKET_REL_MARKER_LISTEN:
    fd_rel = "lstn:";
    break;
  default:
    fd_rel = "conn:";
    break;
  }
  fprintf (stderr,
           "### %s callback (cls, [%s %2llu], %s, 0x%08llX, ptr) "
           "-> 0x%08llX\n",
           reg_type,
           fd_rel,
           (unsigned long long) fd,
           state_str,
           (unsigned long long) app_cntx_old,
           (unsigned long long) res);

  return res;
}
#else  /* ! MHD_USE_TRACE_POLLING_FDS */
#  define mhd_daemon_extr_event_reg(d,fd,w_for,app_cntx_old,ecb_cntx) \
        d->events.data.extr.cb_data.cb (d->events.data.extr.cb_data.cls, \
                                        fd, w_for, app_cntx_old, ecb_cntx)

#endif /* ! MHD_USE_TRACE_POLLING_FDS */


#endif /* ! MHD_EXTR_EVENTS_FUNCS_H */
