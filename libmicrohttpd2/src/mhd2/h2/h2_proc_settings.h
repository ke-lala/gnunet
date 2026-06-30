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
 * @file src/mhd2/h2/h2_proc_settings.h
 * @brief  Declarations of HTTP/2 connection settings processing functions
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_H2_PROC_SETTINGS_H
#define MHD_H2_PROC_SETTINGS_H 1

#include "mhd_sys_options.h"

#include "sys_bool_type.h"

/**
 * Default value of Initial Window Size
 */
#define mhd_H2_STNG_DEF_INIT_WIN_SIZE   (65535u)

/**
 * Minimal allowed value of Maximum Frame Size
 */
#define mhd_H2_STNG_MIN_MAX_FRAME_SIZE  (16384u)

/**
 * Maximum allowed value of Maximum Frame Size
 */
#define mhd_H2_STNG_MAX_MAX_FRAME_SIZE  (16777215u)

/**
 * Default value of Maximum Frame Size
 */
#define mhd_H2_STNG_DEF_MAX_FRAME_SIZE  mhd_H2_STNG_MIN_MAX_FRAME_SIZE

struct MHD_Connection; /* forward declaration */
struct mhd_Buffer; /* forward declaration */

enum mhd_H2SettingsProcessResult
{
  /**
   * Settings processed.
   * Settings ACK queued.
   */
  mhd_H2_STNGS_PROC_OK = 0
  ,
  /**
   * No output buffer space to queue settings ACK.
   * The frame should be processed later again
   */
  mhd_H2_STNGS_PROC_NO_OUT_BUFF
  ,
  /**
   * Settings error.
   * GOAWAY frame queued, connection marked as closing
   */
  mhd_H2_STNGS_PROC_STNGS_ERR
};

/**
 * Process first settings frame sent by peer
 * @param c the connection to use
 * @param stngs_payload the payload of the settings frame
 * @return 'true' if setting successfully processed;
 *         'false' on failure, GOAWAY queued if possible and connection
 *                 is marked as "closing" or "broken".
 */
MHD_INTERNAL bool
mhd_h2_proc_first_settings (struct MHD_Connection *restrict c,
                            const struct mhd_Buffer *restrict stngs_payload)
MHD_FN_PAR_NONNULL_ALL_;

MHD_INTERNAL enum mhd_H2SettingsProcessResult
mhd_h2_proc_new_settings (struct MHD_Connection *restrict c,
                          const struct mhd_Buffer *restrict stngs_payload)
MHD_FN_PAR_NONNULL_ALL_;


MHD_INTERNAL bool
mhd_h2_q_settings_first_fr (struct MHD_Connection *restrict c)
MHD_FN_PAR_NONNULL_ALL_;

#endif /* ! MHD_H2_PROC_SETTINGS_H */
