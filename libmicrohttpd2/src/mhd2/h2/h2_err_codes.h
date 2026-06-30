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
 * @file src/mhd2/h2/h2_err_codes.h
 * @brief  Definition of HTTP/2 error codes
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_H2_ERR_CODES_H
#define MHD_H2_ERR_CODES_H 1

#include "mhd_sys_options.h"

#ifdef mhd_USE_ENUM_BASE_T
#  include "sys_base_types.h"
#endif

/**
 * HTTP/2 error codes
 *
 * Extracted from RFC 9113, Section 7
 */
enum mhd_H2ErrorCode
mhd_ENUM_BASE_T (uint_least32_t)
{
  mhd_H2_ERR_NO_ERROR = 0x00u
  ,
  mhd_H2_ERR_PROTOCOL_ERROR = 0x01u
  ,
  mhd_H2_ERR_INTERNAL_ERROR = 0x02u
  ,
  mhd_H2_ERR_FLOW_CONTROL_ERROR = 0x03u
  ,
  mhd_H2_ERR_SETTINGS_TIMEOUT = 0x04u
  ,
  mhd_H2_ERR_STREAM_CLOSED = 0x05u
  ,
  mhd_H2_ERR_FRAME_SIZE_ERROR = 0x06u
  ,
  mhd_H2_ERR_REFUSED_STREAM = 0x07u
  ,
  mhd_H2_ERR_CANCEL = 0x08u
  ,
  mhd_H2_ERR_COMPRESSION_ERROR = 0x09u
  ,
  mhd_H2_ERR_CONNECT_ERROR = 0x0Au
  ,
  mhd_H2_ERR_ENHANCE_YOUR_CALM = 0x0Bu
  ,
  mhd_H2_ERR_INADEQUATE_SECURITY = 0x0Cu
  ,
  mhd_H2_ERR_HTTP_1_1_REQUIRED = 0x0Du
#ifndef mhd_USE_ENUM_BASE_T
  ,
  /**
   * Not a real error code, no not use
   */
  mhd_H2_ERR_SENTINEL = 0x7FFFFFFFu
#endif /* ! mhd_USE_ENUM_BASE_T */
};


#endif /* ! MHD_H2_ERR_CODES_H */
