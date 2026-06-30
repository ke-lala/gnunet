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
 * @file src/mhd2/conn_timeout.h
 * @brief  The declarations of connection timeout handling functions
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_CONN_TIMEOUT_H
#define MHD_CONN_TIMEOUT_H 1

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

struct MHD_Connection; /* Forward declaration */


/**
 * Get number of milliseconds before connection will time out.
 * @param c the connection to check
 * @return zero if connection is timed out already,
 *         number of millisecond left before connection will time out,
 *         #UINT64_MAX if connection has no time out
 */
MHD_INTERNAL uint_fast64_t
mhd_conn_get_timeout_left (const struct MHD_Connection *restrict c,
                           uint_fast64_t cur_milsec)
MHD_FN_PAR_NONNULL_ALL_;


/**
 * Check whether connection's timeout is expired.
 * @param c the connection to check
 * @return 'true' if connection timeout expired and the connection needs to be
 *         closed,
 *         'false' otherwise
 */
MHD_INTERNAL bool
mhd_conn_is_timeout_expired (struct MHD_Connection *restrict c)
MHD_FN_PAR_NONNULL_ALL_;

/**
 * Initialise activity timeout data.
 * Add connection to the timeouts list if needed; set activity mark, if needed.
 * @param c the connection to initialise
 * @param timeout the connection timeout value to use
 */
MHD_INTERNAL void
mhd_conn_init_activity_timeout (struct MHD_Connection *restrict c,
                                uint_fast32_t timeout)
MHD_FN_PAR_NONNULL_ALL_;


/**
 * Remove connection from time-out lists
 * @param c the connection to process
 */
MHD_INTERNAL void
mhd_conn_deinit_activity_timeout (struct MHD_Connection *restrict c)
MHD_FN_PAR_NONNULL_ALL_;


/**
 * Update last activity mark to the current time.
 * @param c the connection to update
 */
MHD_INTERNAL void
mhd_conn_update_activity_mark (struct MHD_Connection *restrict c)
MHD_FN_PAR_NONNULL_ALL_;

#endif /* ! MHD_CONN_TIMEOUT_H */
