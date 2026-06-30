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
 * @file src/mhd2/sys_ip_headers.h
 * @brief  The header for system headers related to TCP/IP
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_SYS_IP_HEADERS_H
#define MHD_SYS_IP_HEADERS_H 1

#include "mhd_sys_options.h"

#include "mhd_socket_type.h"
#include "sys_sockets_headers.h"

#ifdef MHD_SOCKETS_KIND_POSIX
#  ifdef HAVE_INETLIB_H
#    include <inetLib.h>
#  endif /* HAVE_INETLIB_H */
#  ifdef HAVE_NETINET_IN_H
#    include <netinet/in.h>
#  endif /* HAVE_NETINET_IN_H */
#  ifdef HAVE_ARPA_INET_H
#    include <arpa/inet.h>
#  endif
#  if ! defined(HAVE_NETINET_IN_H) && ! defined(HAVE_ARPA_INET_H) \
  && defined(HAVE_NETDB_H)
#    include <netdb.h>
#  endif
#  ifdef HAVE_NETINET_TCP_H
#    include <netinet/tcp.h>
#  endif
#else
#  include <ws2tcpip.h>
#endif

#if ! defined(HAVE_DCLR_IPV6_V6ONLY) && defined(IPV6_V6ONLY)
/* Mis-deteted by configure */
#  define HAVE_DCLR_IPV6_V6ONLY 1
#endif

#if ! defined(HAVE_DCLR_TCP_NODELAY) && defined(TCP_NODELAY)
/* Mis-deteted by configure */
#  define HAVE_DCLR_TCP_NODELAY 1
#endif

#if ! defined(HAVE_DCLR_TCP_FASTOPEN) && defined(TCP_FASTOPEN)
/* Mis-deteted by configure */
#  define HAVE_DCLR_TCP_FASTOPEN 1
#endif

#ifdef IPPROTO_TCP
#  if defined(TCP_CORK)
/**
 * Value of TCP_CORK or TCP_NOPUSH
 */
#    define mhd_TCP_CORK_NOPUSH TCP_CORK
#  elif defined(TCP_NOPUSH)
/**
 * Value of TCP_CORK or TCP_NOPUSH
 */
#    define mhd_TCP_CORK_NOPUSH TCP_NOPUSH
#  endif /* TCP_NOPUSH */
#endif /* IPPROTO_TCP */

#ifdef mhd_TCP_CORK_NOPUSH
#  ifdef __linux__
/**
 * Indicate that reset of TCP_CORK / TCP_NOPUSH push data to the network
 */
#    define mhd_CORK_RESET_PUSH_DATA 1
/**
 * Indicate that reset of TCP_CORK / TCP_NOPUSH push data to the network
 * even if TCP_CORK/TCP_NOPUSH was in switched off state.
 */
#    define mhd_CORK_RESET_PUSH_DATA_ALWAYS 1
#endif /* __linux__ */
#if (defined(__FreeBSD__) && \
  ((__FreeBSD__ + 0) >= 5 || (__FreeBSD_version + 0) >= 450000)) || \
  (defined(__FreeBSD_kernel_version) && \
  (__FreeBSD_kernel_version + 0) >= 450000)
/* FreeBSD pushes data to the network with reset of TCP_NOPUSH
 * starting from version 4.5. */
/**
 * Indicate that reset of TCP_CORK / TCP_NOPUSH push data to the network
 */
#define mhd_CORK_RESET_PUSH_DATA 1
#endif /* __FreeBSD_version >= 450000 */
#ifdef __OpenBSD__
/* OpenBSD took implementation from FreeBSD */
/**
 * Indicate that reset of TCP_CORK / TCP_NOPUSH push data to the network
 */
#define mhd_CORK_RESET_PUSH_DATA 1
#endif /* __OpenBSD__ */
#endif /* MHD_TCP_CORK_NOPUSH */

#ifdef __linux__
/**
 * Indicate that set of TCP_NODELAY push data to the network
 */
#  define mhd_NODELAY_SET_PUSH_DATA 1
/**
 * Indicate that set of TCP_NODELAY push data to the network even
 * if TCP_DELAY was already set and regardless of TCP_CORK / TCP_NOPUSH state
 */
#  define mhd_NODELAY_SET_PUSH_DATA_ALWAYS 1
#endif /* __linux__ */


#endif /* ! MHD_SYS_IP_HEADERS_H */
