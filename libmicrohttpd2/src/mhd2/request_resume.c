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
 * @file src/mhd2/request_resume.c
 * @brief  The implementation of MHD_request_resume() function
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "mhd_cntnr_ptr.h"

#ifdef MHD_USE_TRACE_SUSPEND_RESUME
#  include <stdio.h>
#endif /* MHD_USE_TRACE_SUSPEND_RESUME */

#include "mhd_daemon.h"
#include "mhd_connection.h"

#include "daemon_funcs.h"

#include "mhd_public_api.h"

MHD_EXTERN_ MHD_FN_PAR_NONNULL_ALL_ void
MHD_request_resume (struct MHD_Request *request)
{
  struct MHD_Connection *c = mhd_CNTNR_PTR (request, \
                                            struct MHD_Connection, \
                                            rq);
  struct MHD_Daemon *d = c->daemon;

  if (! c->suspended)
  {
#ifdef MHD_USE_TRACE_SUSPEND_RESUME
    fprintf (stderr,
             "%%%%%% Requested conn resume, FD: %2llu -> "
             "failed as not suspended\n",
             (unsigned long long) c->sk.fd);
#endif /* MHD_USE_TRACE_SUSPEND_RESUME */
    return;
  }
  c->resuming = true;
#ifdef MHD_USE_TRACE_SUSPEND_RESUME
  fprintf (stderr,
           "%%%%%% Requested conn resume, FD: %2llu\n",
           (unsigned long long) c->sk.fd);
#endif /* MHD_USE_TRACE_SUSPEND_RESUME */
  d->events.act_req.resume = true;
  (void) mhd_daemon_trigger_itc (d);
}
