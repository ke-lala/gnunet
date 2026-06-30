/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2024-2025 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/mhd_tls_internal.h
 * @brief  The TLS handling internal functions and data
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_TLS_INTERNAL_H
#define MHD_TLS_INTERNAL_H 1

#include "mhd_sys_options.h"

#ifndef MHD_SUPPORT_HTTPS
#error This header should be used only if HTTPS is enabled
#endif

#include "sys_sizet_type.h"

#include "mhd_str_macros.h"

#include "mhd_tls_enums.h"

/**
 * Registered ALPN value for HTTP/1.0
 */
#define mhd_ALPN_H1_0   "http/1.0"

/**
 * Registered ALPN value for HTTP/1.1
 */
#define mhd_ALPN_H1_1   "http/1.1"

/**
 * Registered ALPN value for HTTP/2
 */
#define mhd_ALPN_H2     "h2"

/**
 * Registered ALPN value for HTTP/3
 */
#define mhd_ALPN_H3     "h3"

/**
 * The length of #mhd_ALPN_H1_0
 */
#define mhd_ALPN_H1_0_LEN       mhd_SSTR_LEN (mhd_ALPN_H1_0)

/**
 * The length of #mhd_ALPN_H1_1
 */
#define mhd_ALPN_H1_1_LEN       mhd_SSTR_LEN (mhd_ALPN_H1_1)

/**
 * The length of #mhd_ALPN_H2_1
 */
#define mhd_ALPN_H2_LEN         mhd_SSTR_LEN (mhd_ALPN_H2)

/**
 * The length of #mhd_ALPN_H2_1
 */
#define mhd_ALPN_H3_LEN         mhd_SSTR_LEN (mhd_ALPN_H3)

/**
 * Decode provided ALPN identifier
 * @param alnp_id_size the size in bytes of the @a alpn_str
 * @param alnp_id the ALPN identifier, does not need to be zero-terminated,
 *                could be NULL
 * @return the decoded protocol,
 *         #mhd_TLS_ALPN_PROT_NOT_SELECTED if @a alnp_str is NULL,
 *         #mhd_TLS_ALPN_PROT_ERROR if @a alnp_str does not match any known
 *                                  string identifier,
 */
MHD_INTERNAL enum mhd_TlsAlpnProt
mhd_tls_alpn_decode_n (size_t alnp_id_size,
                       const unsigned char *alnp_id)
MHD_FN_PAR_IN_SIZE_ (2, 1);


#endif /* ! MHD_TLS_INTERNAL_H */
