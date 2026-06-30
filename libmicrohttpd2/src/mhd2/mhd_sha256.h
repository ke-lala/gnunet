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
 * @file src/mhd2/mhd_sha256.h
 * @brief  Simple wrapper for selection of built-in/external SHA-256
 *         implementation
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_SHA256_H
#define MHD_SHA256_H 1

#include "mhd_sys_options.h"

#ifndef MHD_SUPPORT_SHA256
#error This file must be used only when SHA-256 is enabled
#endif
#ifndef MHD_SHA256_EXTR
#  include "sha256_int.h"
#else  /* MHD_SHA256_EXTR */
#  include "sha256_ext.h"
#endif /* MHD_SHA256_EXTR */

#ifndef mhd_SHA256_DIGEST_SIZE
/**
 * Size of SHA-256 resulting digest in bytes
 * This is the final digest size, not intermediate hash.
 */
#  define mhd_SHA256_DIGEST_SIZE (32)
#endif /* ! mhd_SHA256_DIGEST_SIZE */

#ifndef MHD_SHA256_EXTR
/**
 * Universal ctx type mapped for chosen implementation
 */
#  define mhd_Sha256Ctx mhd_Sha256CtxInt
#else  /* MHD_SHA256_EXTR */
/**
 * Universal ctx type mapped for chosen implementation
 */
#  define mhd_Sha256Ctx mhd_Sha256CtxExt
#endif /* MHD_SHA256_EXTR */

#ifndef mhd_SHA256_HAS_INIT_ONE_TIME
/**
 * Setup and prepare ctx for hash calculation
 */
#  define mhd_SHA256_init_one_time(ctx) mhd_SHA256_init (ctx)
#endif /* ! mhd_SHA256_HAS_INIT_ONE_TIME */

#ifndef mhd_SHA256_HAS_FINISH_RESET
/**
 * Re-use the same ctx for the new hashing after digest calculated
 */
#  define mhd_SHA256_reset(ctx) mhd_SHA256_init (ctx)
/**
 * Finalise SHA-512/256 calculation, return digest, reset hash calculation.
 */
#  define mhd_SHA256_finish_reset(ctx,digest) \
        (mhd_SHA256_finish (ctx,digest), mhd_SHA256_reset (ctx))
/**
 * Finalise hash calculation, return digest, de-initialise hash calculation.
 */
#  define mhd_SHA256_finish_deinit(ctx,digest) \
        (mhd_SHA256_finish (ctx,digest), mhd_SHA256_deinit (ctx))
#else  /* mhd_SHA256_HAS_FINISH_RESET */
#  define mhd_SHA256_reset(ctx) ((void) 0)
/**
 * Finalise hash calculation, return digest, de-initialise hash calculation.
 */
#  define mhd_SHA256_finish_deinit(ctx,digest) \
        (mhd_SHA256_finish_reset (ctx,digest), mhd_SHA256_deinit (ctx))
#endif /* mhd_SHA256_HAS_FINISH_RESET */

#ifndef mhd_SHA256_HAS_DEINIT
#  define mhd_SHA256_deinit(ctx) ((void) 0)
#endif /* HAVE_SHA256_DEINIT */

#ifdef mhd_SHA256_HAS_EXT_ERROR
#define mhd_SHA256_has_err(ctx) (0 != ((ctx)->ext_error))
#else  /* ! mhd_SHA256_HAS_EXT_ERROR */
#define mhd_SHA256_has_err(ctx) (((void) (ctx)), ! ! 0)
#endif /* ! mhd_SHA512_256_HAS_EXT_ERROR */

/* Sanity checks */

#if ! defined(mhd_SHA256_HAS_FINISH_RESET) && ! defined(mhd_SHA256_HAS_FINISH)
#error Required mhd_SHA256_finish_reset() or mhd_SHA256_finish()
#endif /* ! mhd_SHA256_HAS_FINISH_RESET && ! mhd_SHA256_HAS_FINISH */

#endif /* MHD_SHA256_H */
