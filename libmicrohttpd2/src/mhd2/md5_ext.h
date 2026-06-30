/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2022-2024 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/md5_ext.h
 * @brief  Wrapper declarations for MD5 calculation performed by TLS library
 * @author Karlson2k (Evgeny Grin)
 */
#ifndef MHD_MD5_EXT_H
#define MHD_MD5_EXT_H 1

#include "mhd_sys_options.h"

#include "sys_base_types.h"

/**
 * Size of MD5 resulting digest in bytes
 * This is the final digest size, not intermediate hash.
 */
#define mhd_MD5_DIGEST_SIZE (16)

#ifndef MHD_MD5_Context
#define MHD_MD5_Context void
#endif

/**
 * Indicates that struct mhd_Md5CtxExt has 'ext_error'
 */
#define mhd_MD5_HAS_EXT_ERROR 1

/**
 * MD5 calculation context
 */
struct mhd_Md5CtxExt
{
  MHD_MD5_Context *handle; /**< Hash calculation handle */
  int ext_error; /**< Non-zero if external error occurs during init or hashing */
};

/**
 * Indicates that mhd_MD5_init_one_time() function is present.
 */
#define mhd_MD5_HAS_INIT_ONE_TIME 1

/**
 * Initialise structure for MD5 calculation, allocate resources.
 *
 * This function must not be called more than one time for @a ctx.
 *
 * @param ctx the calculation context
 */
void
mhd_MD5_init_one_time (struct mhd_Md5CtxExt *ctx);


/**
 * MD5 process portion of bytes.
 *
 * @param ctx the calculation context
 * @param size number of bytes in @a data, must not be 0
 * @param data bytes to add to hash
 */
void
mhd_MD5_update (struct mhd_Md5CtxExt *restrict ctx,
                size_t size,
                const uint8_t *restrict data);


/**
 * Indicates that mhd_MD5_finish_reset() function is available
 */
#define mhd_MD5_HAS_FINISH_RESET 1

/**
 * Finalise MD5 calculation, return digest, reset hash calculation.
 *
 * @param ctx the calculation context
 * @param[out] digest set to the hash, must be #mhd_MD5_DIGEST_SIZE bytes
 */
void
mhd_MD5_finish_reset (struct mhd_Md5CtxExt *restrict ctx,
                      uint8_t digest[mhd_MD5_DIGEST_SIZE]);

/**
 * Indicates that mhd_MD5_deinit() function is present
 */
#define mhd_MD5_HAS_DEINIT 1

/**
 * Free allocated resources.
 *
 * @param[in] ctx the calculation context
 */
void
mhd_MD5_deinit (struct mhd_Md5CtxExt *ctx);

#endif /* MHD_MD5_EXT_H */
