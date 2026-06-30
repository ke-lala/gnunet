/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2014-2024 Evgeny Grin (Karlson2k)
  Copyright (C) 2007-2018 Daniel Pittman and Christian Grothoff

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
 * @file src/mhd2/daemon_add_conn.h
 * @brief  The declaration of internal functions for adding new connections
 * @author Karlson2k (Evgeny Grin)
 * @author Daniel Pittman
 * @author Christian Grothoff
 */

#ifndef MHD_DAEMON_ADD_CONN
#define MHD_DAEMON_ADD_CONN 1

#include "mhd_sys_options.h"

#include "sys_bool_type.h"

struct MHD_Daemon; /* Forward declaration */
struct MHD_Connection; /* Forward declaration */

/**
 * The result of the accepting of the new connection
 */
enum mhd_DaemonAcceptResult
{
  /**
   * New connection has been accepted successfully.
   * Probably more connections are pending to be accepted.
   */
  mhd_DAEMON_ACCEPT_SUCCESS = 0
  ,
  /**
   * New connection has been skipped for some reason.
   * It is OK to try to accept more connections right now.
   */
  mhd_DAEMON_ACCEPT_SKIPPED = 1 << 0
  ,
  /**
   * No more new connections are pending, the listen backlog is empty
   */
  mhd_DAEMON_ACCEPT_NO_MORE_PENDING = 1 << 1
  ,
  /**
   * Connection accept failed, but the listen backlog could be not empty.
   * Do not try to accept more connection right now.
   */
  mhd_DAEMON_ACCEPT_FAILED = 1 << 2
};


/**
 * Accept an incoming connection and create the MHD_Connection object for
 * it.  This function also enforces policy by way of checking with the
 * accept policy callback.
 * @remark To be called only from thread that process
 * daemon's select()/poll()/etc.
 *
 * @param daemon handle with the listen socket
 * @return a #mhd_DaemonAcceptResult value
 */
MHD_INTERNAL enum mhd_DaemonAcceptResult
mhd_daemon_accept_connection (struct MHD_Daemon *restrict daemon)
MHD_FN_PAR_NONNULL_ALL_;


/**
 * Remove all referenced to the connection from the daemon.
 * Must be performed only when connection thread (for thread-per-connection)
 * has stopped.
 * @param c the connection to remove
 */
MHD_INTERNAL void
mhd_conn_remove_from_daemon (struct MHD_Connection *restrict c)
MHD_FN_PAR_NONNULL_ALL_;


/**
 * Finally close and deallocate connection.
 * Must be performed only when connection thread (for thread-per-connection)
 * has stopped.
 * The connection data deallocated by this function and cannot be used anymore.
 * This function must be the last function called for connection object.
 * @param c the connection to close
 */
MHD_INTERNAL void
mhd_conn_close_final (struct MHD_Connection *restrict c)
MHD_FN_PAR_NONNULL_ALL_;


/**
 * Process externally added connections (if any).
 * If the daemon at the connection limit, then any pending externally
 * added connections will be closed without processing.
 * @param d the daemon handle to use
 */
MHD_INTERNAL void
mhd_daemon_process_ext_added_conns (struct MHD_Daemon *restrict d)
MHD_FN_PAR_NONNULL_ALL_;

#endif /* ! MHD_DAEMON_ADD_CONN */
