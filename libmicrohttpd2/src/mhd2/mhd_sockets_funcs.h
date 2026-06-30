/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2014-2024 Karlson2k (Evgeny Grin)

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
 * @file src/mhd2/mhd_sockets_funcs.h
 * @brief  Declarations for sockets manipulating functions
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_SOCKETS_FUNCS_H
#define MHD_SOCKETS_FUNCS_H 1

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "mhd_socket_type.h"


/**
 * Change socket options to be non-blocking.
 *
 * @param sckt the socket to manipulate
 * @return true on success, false otherwise
 */
MHD_INTERNAL bool
mhd_socket_nonblocking (MHD_Socket sckt);

/**
 * Change socket options to be non-inheritable.
 *
 * @param sckt the socket to manipulate
 * @return true on success, false otherwise
 */
MHD_INTERNAL bool
mhd_socket_noninheritable (MHD_Socket sckt);

/**
 * Change socket options to be closed "hard".
 *
 * @param sckt the socket to manipulate
 * @return true on success, false otherwise
 */
MHD_INTERNAL bool
mhd_socket_set_hard_close (MHD_Socket sckt);

/**
 * Shutdown sending on socket
 *
 * @param sckt the socket to manipulate
 * @return true on success, false otherwise
 */
MHD_INTERNAL bool
mhd_socket_shut_wr (MHD_Socket sckt);

/**
 * Control Nagle's algorithm on @a sock.
 *
 * @param sckt the socket to manipulate
 * @param on the value to use: true to set "no delay" (disable Nagle's
 *           algorithm), false to clear "no delay" (enable Nagle's algorithm)
 * @return true on success, false otherwise
 */
MHD_INTERNAL bool
mhd_socket_set_nodelay (MHD_Socket sckt,
                        bool on);


#ifndef HAVE_SOCKETPAIR

#  define mhd_socket_pair(fdarr_ptr) mhd_socket_pair_func (fdarr_ptr, false)
#  define mhd_socket_pair_nblk(fdarr_ptr) mhd_socket_pair_func (fdarr_ptr, true)


/**
 * Create pair of mutually connected sockets on loopback address
 * @param sockets_pair the array to receive resulted sockets
 * @param non_blk if set to true, sockets created in non-blocking mode
 *                otherwise sockets will be in blocking mode
 * @return true if succeeded, false otherwise
 */
MHD_INTERNAL bool
mhd_socket_pair_func (MHD_Socket sckt[2], bool non_blk);

#endif /* ! HAVE_SOCKETPAIR */


#endif /* ! MHD_SOCKETS_FUNCS_H */
