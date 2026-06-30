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
 * @file src/mhd2/conn_timeout.c
 * @brief  The definitions of connection timeout handling functions
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "sys_bool_type.h"

#include "mhd_assert.h"

#include "mhd_constexpr.h"

#include "mhd_dlinked_list.h"

#include "mhd_daemon.h"
#include "mhd_connection.h"

#include "daemon_funcs.h"

#include "conn_timeout.h"

MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ uint_fast64_t
mhd_conn_get_timeout_left (const struct MHD_Connection *restrict c,
                           uint_fast64_t cur_milsec)
{
  mhd_constexpr uint_fast64_t uifast64_hibit =
    (uint_fast64_t) (~(((uint_fast64_t) (~((uint_fast64_t) 0))) >> 1u));
  const uint_fast64_t timeout = c->timeout.milsec;
  uint_fast64_t timedout_time;
  uint_fast64_t timeout_left;

  mhd_assert (! c->suspended);

  if (0u == timeout)
    return (uint_fast64_t) 0xFFFFFFFFFFFFFFFFu;

  /* The logic used in these calculations allows values wrap over the maximum
     value (taking into account that maximum timeout time is limited) */
  timedout_time =
    c->timeout.last_act + c->timeout.milsec + 1u; /* '+1' to not expire at exactly 'timeout' milliseconds */
  /* Time before timed out moment */
  timeout_left = timedout_time - cur_milsec;

  if (0u == (timeout_left & uifast64_hibit))
  {
    /* "Timeout left" must be always less or equal the timeout value (plus one)
       unless the system clock jumped back. */
    mhd_assert ((timeout_left <= (c->timeout.milsec + 1u)) ||
                ((cur_milsec - c->timeout.last_act)
                 > (c->timeout.last_act - cur_milsec)));

    return timeout_left;
  }

  return 0u; /* Already timed out */
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_conn_is_timeout_expired (struct MHD_Connection *restrict c)
{
  struct MHD_Daemon *const restrict d = c->daemon;

  return (0u == mhd_conn_get_timeout_left (c,
                                           mhd_daemon_get_milsec_counter (d)));
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ void
mhd_conn_init_activity_timeout (struct MHD_Connection *restrict c,
                                uint_fast32_t timeout)
{
  struct MHD_Daemon *const restrict d = c->daemon;

#if defined(MHD_SUPPORT_THREADS)
  mhd_assert (! mhd_D_HAS_WORKERS (d));
#endif /* MHD_SUPPORT_THREADS */

  mhd_assert (! c->suspended);
  mhd_assert (! c->timeout.in_cstm_tmout_list);

  c->timeout.milsec = timeout;

  if (0u == timeout)
    return;

  c->timeout.last_act = mhd_daemon_get_milsec_counter (d);

  if (mhd_D_HAS_THR_PER_CONN (d))
    return;

  if (timeout == d->conns.cfg.timeout_milsec)
  {
    mhd_DLINKEDL_INS_FIRST_D (&(d->conns.def_timeout),
                              c,
                              timeout.tmout_list);
  }
  else
  {
    mhd_DLINKEDL_INS_FIRST_D (&(d->conns.cust_timeout),
                              c,
                              timeout.tmout_list);
    c->timeout.in_cstm_tmout_list = true;
  }
}


MHD_INTERNAL
MHD_FN_PAR_NONNULL_ALL_ void
mhd_conn_deinit_activity_timeout (struct MHD_Connection *restrict c)
{
  struct MHD_Daemon *const restrict d = c->daemon;

#if defined(MHD_SUPPORT_THREADS)
  mhd_assert (! mhd_D_HAS_WORKERS (d));
#endif /* MHD_SUPPORT_THREADS */

  if (0u == c->timeout.milsec)
  {
    mhd_assert (! c->timeout.in_cstm_tmout_list);
    return;
  }

  if (mhd_D_HAS_THR_PER_CONN (d))
  {
    mhd_assert (! c->timeout.in_cstm_tmout_list);
    return;
  }

  if (! c->timeout.in_cstm_tmout_list)
    mhd_DLINKEDL_DEL_D (&(d->conns.def_timeout),
                        c,
                        timeout.tmout_list);
  else
    mhd_DLINKEDL_DEL_D (&(d->conns.cust_timeout), \
                        c,
                        timeout.tmout_list);

  c->timeout.in_cstm_tmout_list = false;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ void
mhd_conn_update_activity_mark (struct MHD_Connection *restrict c)
{
  struct MHD_Daemon *const restrict d = c->daemon;
  uint_fast64_t prev_mark;

#if defined(MHD_SUPPORT_THREADS)
  mhd_assert (! mhd_D_HAS_WORKERS (d));
#endif /* MHD_SUPPORT_THREADS */

  mhd_assert (! c->suspended);

  if (0u == c->timeout.milsec)
  {
    mhd_assert (! c->timeout.in_cstm_tmout_list);
    return;
  }

  prev_mark = c->timeout.last_act;
  c->timeout.last_act = mhd_daemon_get_milsec_counter (d);

  if (c->timeout.last_act == prev_mark)
    return; /* Nothing to update */

  if (mhd_D_HAS_THR_PER_CONN (d))
  {
    mhd_assert (! c->timeout.in_cstm_tmout_list);
    return; /* each connection has personal timeout */
  }

  if (c->timeout.in_cstm_tmout_list)
    return; /* custom timeout, no need to move it in "normal" DLL */

  /* move connection to head of timeout list */
  mhd_DLINKEDL_MOVE_TO_FIRST_D (&(d->conns.def_timeout),
                                c,
                                timeout.tmout_list);
}
