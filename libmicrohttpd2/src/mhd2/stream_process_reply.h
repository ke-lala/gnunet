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
 * @file src/mhd2/stream_process_reply.h
 * @brief  The declarations of internal functions for forming and sending
 *         replies for requests
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_STREAM_PROCESS_REPLY_H
#define MHD_STREAM_PROCESS_REPLY_H 1

#include "mhd_sys_options.h"

#include "sys_bool_type.h"

struct MHD_Connection; /* forward declaration */


/**
 * Check whether Dynamic Content Creator cleanup callback is set and
 * call it, if needed.
 * Un-set cleanup callback after calling.
 * @param c the connection to process
 */
MHD_INTERNAL void
mhd_stream_call_dcc_cleanup_if_needed (struct MHD_Connection *restrict c)
MHD_FN_PAR_NONNULL_ALL_;

/**
 * Produce time stamp.
 *
 * Result is NOT null-terminated.
 * Result is always 29 bytes long.
 *
 * @param[out] date where to write the time stamp, with
 *             at least 29 bytes of available space.
 */
MHD_INTERNAL bool
mhd_build_date_str (char date[MHD_FN_PAR_FIX_ARR_SIZE_ (29)])
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_OUT_ (1);
/**
 * Allocate the connection's write buffer and fill it with all of the
 * headers from the response.
 * Required headers are added here.
 *
 * @param c the connection to process
 * @return 'true' if state has been update,
 *         'false' if connection is going to be aborted
 */
MHD_INTERNAL bool
mhd_stream_build_header_response (struct MHD_Connection *restrict c)
MHD_FN_PAR_NONNULL_ALL_;


/**
 * Prepare the unchunked response content of this connection for sending.
 *
 * @param c the connection
 * @return 'true' if connection new state could be processed now,
 *         'false' if no new state processing is needed.
 */
MHD_INTERNAL bool
mhd_stream_prep_unchunked_body (struct MHD_Connection *restrict c)
MHD_FN_PAR_NONNULL_ALL_;


/**
 * Prepare the chunked response content of this connection for sending.
 *
 * @param c the connection
 *
 * @return 'true' if connection new state could be processed now,
 *         'false' if no new state processing is needed.
 */
MHD_INTERNAL bool
mhd_stream_prep_chunked_body (struct MHD_Connection *restrict c)
MHD_FN_PAR_NONNULL_ALL_;


/**
 * Allocate the connection's write buffer (if necessary) and fill it
 * with response footers.
 *
 * @param c the connection
 *
 * @return 'true' if connection new state could be processed now,
 *         'false' if no new state processing is needed.
 */
MHD_INTERNAL bool
mhd_stream_prep_chunked_footer (struct MHD_Connection *restrict c)
MHD_FN_PAR_NONNULL_ALL_;

#endif /* ! MHD_STREAM_PROCESS_REPLY_H */
