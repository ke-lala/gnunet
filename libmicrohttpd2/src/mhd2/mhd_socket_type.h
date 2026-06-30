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
 * @file src/mhd2/mhd_sockets_types.h
 * @brief  The header for MHD_Socket types and relevant macros
 * @author Karlson2k (Evgeny Grin)
 *
 * This header provides 'MHD_Socket' type and 'MHD_INVALID_SOCKET' value.
 */

#ifndef MHD_SOCKET_TYPE_H
#define MHD_SOCKET_TYPE_H 1

#include "mhd_sys_options.h"

#ifndef MHD_INVALID_SOCKET
#  if ! defined(_WIN32) || defined(_SYS_TYPES_FD_SET)
#    define MHD_SOCKETS_KIND_POSIX 1 /* The POSIX-style sockets are used */
/**
 * MHD_Socket is type for socket FDs
 *
 * This type is always 'int' on POSIX platforms.
 */
typedef int MHD_Socket;
/**
 * Invalid value for MHD_Socket
 */
#    define MHD_INVALID_SOCKET (-1)
#  else /* !defined(_WIN32) || defined(_SYS_TYPES_FD_SET) */
#    define MHD_SOCKETS_KIND_WINSOCK 1 /* The WinSock-style sockets are used */
#    include <winsock2.h>
/**
 * MHD_Socket is type for socket FDs
 */
typedef SOCKET MHD_Socket;
/**
 * Invalid value for MHD_Socket
 */
#    define MHD_INVALID_SOCKET (INVALID_SOCKET)
#  endif /* !defined(_WIN32) || defined(_SYS_TYPES_FD_SET) */
#endif /* MHD_INVALID_SOCKET */

#endif /* ! MHD_SOCKET_TYPE_H */
