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
 * @file src/mhd2/h2/h2_settings.h
 * @brief  HTTP2 SETTINGS identifiers, coding and decoding
 * @author Karlson2k (Evgeny Grin)
 *
 * SETTINGS parameters are defined in RFC 9113. Some additional identifiers
 * are taken from other RFCs (see comments in the code).
 */

#ifndef MHD_H2_SETTINGS_H
#define MHD_H2_SETTINGS_H 1

#include "mhd_sys_options.h"

#include "sys_base_types.h"

#include "mhd_assert.h"

#include "mhd_bithelpers.h"

#if defined(_MSC_FULL_VER)
#pragma warning(push)
/* Disable C4505 "unreferenced local function has been removed" */
#pragma warning(disable:4505)
#endif /* _MSC_FULL_VER */

/**
 * HTTP/2 SETTINGS identifiers
 *
 * Extracted from RFC 9113, Section 6.5.2; RFC 8441, Section 9.1;
 * RFC 9218, Section 16.
 */
enum mhd_H2SettingsID
mhd_ENUM_BASE_T (uint_least16_t)
{
  mhd_H2_STNGS_HEADER_TABLE_SIZE = 0x01u
  ,
  mhd_H2_STNGS_ENABLE_PUSH = 0x02u
  ,
  mhd_H2_STNGS_CONCURRENT_STREAMS = 0x03u
  ,
  mhd_H2_STNGS_INITIAL_WINDOW_SIZE = 0x04u
  ,
  mhd_H2_STNGS_MAX_FRAME_SIZE = 0x05u
  ,
  mhd_H2_STNGS_MAX_HEADER_LIST_SIZE = 0x06u
  ,
  mhd_H2_STNGS_ENABLE_CONNECT_PROTOCOL = 0x08u
  ,
  mhd_H2_STNGS_NO_RFC7540_PRIORITIES = 0x09u
#ifndef mhd_USE_ENUM_BASE_T
  ,
  /**
   * Not a real identifier, no not use
   */
  mhd_H2_STNGS_SENTINEL = 0xFFFFu
#endif /* ! mhd_USE_ENUM_BASE_T */
};


/**
 * HTTP/2 setting
 */
struct mhd_H2Setting
{
  /**
   * Setting's identifier
   */
  enum mhd_H2SettingsID identifier;
  /**
   * Setting's value
   */
  uint_least32_t value;
};

/**
 * The size of single HTTP/2 setting
 */
#define mhd_H2_SETTING_SIZE     (6u)

/**
 * Decode a SETTINGS parameter from "on-wire" 6-byte form.
 *
 * @param encoded the 6-byte array with the encoded parameter
 * @param[out] p_identifier receives the 16-bit setting identifier
 * @param[out] p_value receives the 32-bit setting value
 */
static inline MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_ (1)
MHD_FN_PAR_OUT_ (2) MHD_FN_PAR_OUT_ (3) void
mhd_h2_setting_decode3 (const uint8_t encoded[MHD_FN_PAR_FIX_ARR_SIZE_ (6)],
                        uint_least16_t *restrict p_identifier,
                        uint_least32_t *restrict p_value)
{
  *p_identifier = mhd_GET_16BIT_BE_UNALIGN (encoded);
  *p_value = mhd_GET_32BIT_BE_UNALIGN (encoded + 2u);
}


/**
 * Decode a SETTINGS parameter into a @ref mhd_H2Setting structure.
 *
 * @param encoded the 6-byte array with the encoded parameter
 * @param[out] setting receives the decoded identifier and value
 */
static inline MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_ (1)
MHD_FN_PAR_OUT_ (2) void
mhd_h2_setting_decode (const uint8_t encoded[MHD_FN_PAR_FIX_ARR_SIZE_ (6)],
                       struct mhd_H2Setting *restrict setting)
{
  uint_least16_t identifier;
  mhd_h2_setting_decode3 (encoded,
                          &identifier,
                          &(setting->value));
  setting->identifier = (enum mhd_H2SettingsID) identifier;
}


/**
 * Encode a SETTINGS parameter to on-wire 6-byte form.
 *
 * @param identifier the 16-bit setting identifier
 * @param value the 32-bit setting value
 * @param[out] encoded the destination 6-byte array
 */
static inline MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_ (3) void
mhd_h2_setting_encode3 (uint_least16_t identifier,
                        uint_least32_t value,
                        uint8_t encoded[MHD_FN_PAR_FIX_ARR_SIZE_ (6)])
{
  mhd_assert (identifier == (identifier & 0xFFFFu));
  mhd_assert (value == (value & 0xFFFFFFFFu));
  mhd_PUT_16BIT_BE_UNALIGN (encoded,
                            identifier);
  mhd_PUT_32BIT_BE_UNALIGN (encoded + 2u,
                            value);
}


/**
 * Encode a SETTINGS parameter from @ref mhd_H2Setting.
 *
 * @param setting the setting to encode
 * @param[out] encoded the destination 6-byte array
 */
static inline MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_ (1)
MHD_FN_PAR_OUT_ (2) void
mhd_h2_setting_encode (const struct mhd_H2Setting *restrict setting,
                       uint8_t encoded[MHD_FN_PAR_FIX_ARR_SIZE_ (6)])
{
  mhd_h2_setting_encode3 ((uint_least16_t) setting->identifier,
                          setting->value,
                          encoded);
}


#if defined(_MSC_FULL_VER)
/* Restore warnings */
#pragma warning(pop)
#endif /* _MSC_FULL_VER */

#endif /* ! MHD_H2_SETTINGS_H */
