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
 * @file src/mhd2/stream_process_states.h
 * @brief  The declarations of internal functions for processing
 *         stream states
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_STREAM_PROCESS_STATES_H
#define MHD_STREAM_PROCESS_STATES_H 1

#include "mhd_sys_options.h"

#include "sys_bool_type.h"

struct MHD_Connection; /* forward declaration */

/**
 * Update event loop state for the connection.
 * Event loop state indicates current stage: sending or receiving the data.
 * The underlying connection layer (plain TCP or TLS) must be already connected.
 * @param c the connection to process
 */
MHD_INTERNAL void
mhd_conn_event_loop_state_update (struct MHD_Connection *restrict c)
MHD_FN_PAR_NONNULL_ALL_;

/**
 * Process states and the data for the connection.
 * For HTTP/1.1: connection is equal stream.
 * The function updates the states of the connection.
 * The underlying connection layer (plain TCP or TLS) must be already connected.
 * @param c the connection to process
 * @return true if states and data has been successfully processed,
 *         false if connection needs to be closed
 */
MHD_INTERNAL bool
mhd_conn_process_data (struct MHD_Connection *restrict c)
MHD_FN_PAR_NONNULL_ALL_;

#endif /* ! MHD_STREAM_PROCESS_STATES_H */
