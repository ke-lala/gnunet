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
 * @file src/mhd2/h2/h2_proc_conn.h
 * @brief  Declarations of HTTP/2 connection processing functions
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_H2_PROC_CONN_H
#define MHD_H2_PROC_CONN_H 1

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include "h2_err_codes.h"

struct MHD_Connection; /* forward declaration */
struct mhd_Buffer; /* forward declaration */
union mhd_H2FrameUnion; /* forward declaration */

/**
 * Process the first incoming frame
 * @param c the connection to process
 * @return 'true' if successfully processed, HTTP/2 connection is fully ready;
 *         'false' if HTTP/2 connection cannot be used (not ready or broken)
 */
MHD_INTERNAL bool
mhd_h2_conn_process_first_fr (struct MHD_Connection *restrict c)
MHD_FN_PAR_NONNULL_ALL_;


/**
 * Process all changes in the connection.
 * Create outgoing control frames as necessary, handle errors.
 * @param c the connection to process
 * @return 'true' if successfully processed, HTTP/2 processing may continue
 *         'false' if not enough output buffer to send the control frames or
 *                 if connection is broken, HTTP/2 procession may not continue
 */
MHD_INTERNAL bool
mhd_h2_conn_process_changes (struct MHD_Connection *restrict c)
MHD_FN_PAR_NONNULL_ALL_;


MHD_INTERNAL bool
mhd_h2_conn_process_in_goaway (struct MHD_Connection *restrict c,
                               uint_least32_t last_stream_id,
                               enum mhd_H2ErrorCode err)
MHD_FN_PAR_NONNULL_ALL_;


MHD_INTERNAL bool
mhd_h2_conn_update_stream_init_window (struct MHD_Connection *restrict c,
                                       uint_least32_t init_wind_size)
MHD_FN_PAR_NONNULL_ALL_;

/**
 * Transition connection to closing state, queue relevant frame for peer.
 * @param c the connection to process
 * @param err the error code to use
 * @param forced if 'true' connection will start closing even if there is
 *               no space to form peer notification frame
 * @return 'true' if succeed,
 *         'false' if output buffer has no space
 */
MHD_INTERNAL bool
mhd_h2_conn_finish (struct MHD_Connection *restrict c,
                    enum mhd_H2ErrorCode err,
                    bool forced)
MHD_FN_PAR_NONNULL_ALL_;

#endif /* ! MHD_H2_PROC_CONN_H */
