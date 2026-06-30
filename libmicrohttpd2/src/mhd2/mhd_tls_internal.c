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
 * @file src/mhd2/mhd_tls_internal.c
 * @brief  The TLS handling internal functions
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "sys_null_macro.h"
#include "sys_sizet_type.h"

#include <string.h>

#include "mhd_assert.h"
#include "mhd_unreachable.h"

#include "mhd_tls_internal.h"

MHD_INTERNAL
MHD_FN_PAR_IN_SIZE_ (2, 1) enum mhd_TlsAlpnProt
mhd_tls_alpn_decode_n (size_t alnp_id_size,
                       const unsigned char *alnp_id)
{
  mhd_assert ((NULL != alnp_id) || (0u == alnp_id_size));

  if (NULL == alnp_id)
    return mhd_TLS_ALPN_PROT_NOT_SELECTED;

#ifdef MHD_SUPPORT_HTTP2
  if ((alnp_id_size == mhd_ALPN_H2_LEN) &&
      (0 == memcmp (alnp_id,
                    mhd_ALPN_H2,
                    mhd_ALPN_H2_LEN)))
    return mhd_TLS_ALPN_PROT_HTTP2;
#endif /* MHD_SUPPORT_HTTP2 */

  if ((alnp_id_size == mhd_ALPN_H1_1_LEN) &&
      (0 == memcmp (alnp_id,
                    mhd_ALPN_H1_1,
                    mhd_ALPN_H1_1_LEN)))
    return mhd_TLS_ALPN_PROT_HTTP1_1;

  if ((alnp_id_size == mhd_ALPN_H1_0_LEN) &&
      (0 == memcmp (alnp_id,
                    mhd_ALPN_H1_0,
                    mhd_ALPN_H1_0_LEN)))
    return mhd_TLS_ALPN_PROT_HTTP1_0;

  mhd_UNREACHABLE_D ("ALPN can negotiate only one of the provided values");

  return mhd_TLS_ALPN_PROT_ERROR;
}
