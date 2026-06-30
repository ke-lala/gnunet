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
 * @file src/mhd2/mhd_tls_enums.h
 * @brief  The definition of internal enums used for TLS communication
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_TLS_ENUMS_H
#define MHD_TLS_ENUMS_H 1

#include "mhd_sys_options.h"

#ifndef MHD_SUPPORT_HTTPS
#error This header should be used only if HTTPS is enabled
#endif

/**
 * Result of performing TLS procedure
 */
enum MHD_FIXED_ENUM_ mhd_TlsProcedureResult
{
  /**
   * Completed successfully
   */
  mhd_TLS_PROCED_SUCCESS = 0
  ,
  /**
   * In progress, receive operation interrupted.
   * 'recv-ready' flag should NOT be cleared
   */
  mhd_TLS_PROCED_RECV_INTERRUPTED
  ,
  /**
   * In progress, send operation interrupted
   * 'send-ready' flag should NOT be cleared
   */
  mhd_TLS_PROCED_SEND_INTERRUPTED
  ,
  /**
   * In progress, need to receive more data
   * 'recv-ready' flag should be cleared
   */
  mhd_TLS_PROCED_RECV_MORE_NEEDED
  ,
  /**
   * In progress, need to send more data
   * 'send-ready' flag should be cleared
   */
  mhd_TLS_PROCED_SEND_MORE_NEEDED
  ,
  /**
   * Procedure failed
   */
  mhd_TLS_PROCED_FAILED
};

/**
 * Protocol selected by ALPN
 */
enum MHD_FIXED_ENUM_ mhd_TlsAlpnProt
{
  /**
   * The protocol was not selected by ALPN
   * ALPN is not used by the client or TLS backend does not support ALPN
   */
  mhd_TLS_ALPN_PROT_NOT_SELECTED
  ,
  mhd_TLS_ALPN_PROT_HTTP1_0
  ,
  mhd_TLS_ALPN_PROT_HTTP1_1
  ,
  mhd_TLS_ALPN_PROT_HTTP2
  ,
  mhd_TLS_ALPN_PROT_ERROR
};

#endif /* ! MHD_TLS_ENUMS_H */
