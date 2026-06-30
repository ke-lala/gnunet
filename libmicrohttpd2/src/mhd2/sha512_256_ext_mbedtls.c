/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2025 Christian Grothoff

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
 * @file microhttpd/sha512_256_ext_mbedtls.c
 * @brief  Wrapper for SHA-512/256 calculation performed by mbedTLS library
 * @author Christian Grothoff
 */
#include <stdlib.h>
#include <string.h>
#define MBEDTLS_ALLOW_PRIVATE_ACCESS 1
#include <mbedtls/build_info.h>
#include <mbedtls/sha512.h>
#define MHD_SHA512_256_Context mbedtls_sha512_context
#include "sha512_256_ext.h"
#include "mhd_assert.h"


/**
 * Initialize mbedtls context for SHA-512/256.
 * Since SHA-512/256 is not natively supported by
 * mbedTLS, we initialize for SHA-512 and then
 * override the state with the SHA-512/256 IV.
 *
 * @param[in,out] ctx hash context to initialize
 */
static void
init512_256 (struct mhd_Sha512_256CtxExt *ctx)
{
  static const uint64_t iv_sha512_256[8] = {
    0x22312194FC2BF72CULL, 0x9F555FA3C84C64C2ULL,
    0x2393B86B6F53B151ULL, 0x963877195940EABDULL,
    0x96283EE2A88EFFE3ULL, 0xBE5E1E2553863992ULL,
    0x2B0199FC2C85B8AAULL, 0x0EB72DDC81C52CA2ULL
  };

  mbedtls_sha512_init (ctx->handle);
  /* is384=0 for SHA-512 */
#if MBEDTLS_VERSION_MAJOR >= 4
  ctx->ext_error = mbedtls_sha512_starts_ret (ctx->handle,
                                              0);
  if (0 != ctx->ext_error)
  {
    mbedtls_sha512_free (ctx->handle);
    free (ctx->handle);
    ctx->handle = NULL;
    return;
  }
  mhd_assert (sizeof (ctx->handle.state) ==
              sizeof (iv_sha512_256));
  memcpy (ctx->handle.state,
          iv_sha512_256,
          sizeof (iv_sha512_256));
#else
  mbedtls_sha512_starts (ctx->handle,
                         0);
  mhd_assert (sizeof (ctx->handle->state) ==
              sizeof (iv_sha512_256));
  memcpy (ctx->handle->state,
          iv_sha512_256,
          sizeof (iv_sha512_256));
#endif
}


/**
 * Initialise structure for SHA-512/256 calculation, allocate resources.
 *
 * This function must not be called more than one time for @a ctx.
 *
 * @param ctx the calculation context
 */
void
mhd_SHA512_256_init_one_time (struct mhd_Sha512_256CtxExt *ctx)
{
  ctx->ext_error = 0;
  ctx->handle = (mbedtls_sha512_context *) malloc (
    sizeof (mbedtls_sha512_context));
  if (NULL == ctx->handle)
  {
    ctx->ext_error = 1; /* Allocation failure */
    return;
  }

  init512_256 (ctx);

  /* If handle is NULL, the error must be set */
  mhd_assert ((NULL != ctx->handle) || (0 != ctx->ext_error));
  /* If error is set, the handle must be NULL */
  mhd_assert ((0 == ctx->ext_error) || (NULL == ctx->handle));
}


/**
 * Process portion of bytes.
 *
 * @param ctx the calculation context
 * @param data bytes to add to hash
 * @param length number of bytes in @a data
 */
void
mhd_SHA512_256_update (struct mhd_Sha512_256CtxExt *ctx,
                       size_t size,
                       const uint8_t *data)
{
  mhd_assert (0 != size);
#if MBEDTLS_VERSION_MAJOR >= 4
  if (0 == ctx->ext_error)
    ctx->ext_error = mbedtls_sha512_update_ret (ctx->handle,
                                                data,
                                                size);
#else
  mbedtls_sha512_update (ctx->handle,
                         data,
                         size);
#endif
}


/**
 * Finalise SHA-512/256 calculation, return digest, reset hash calculation.
 *
 * @param ctx the calculation context
 * @param[out] digest set to the hash, must be #mhd_SHA512_256_DIGEST_SIZE bytes
 */
void
mhd_SHA512_256_finish_reset (struct mhd_Sha512_256CtxExt *ctx,
                             uint8_t digest[mhd_SHA512_256_DIGEST_SIZE])
{
  uint8_t full_digest[64]; /* SHA-512 produces 64 bytes */

  if (0 == ctx->ext_error)
  {
#if MBEDTLS_VERSION_MAJOR >= 4
    ctx->ext_error = mbedtls_sha512_finish_ret (ctx->handle,
                                                full_digest);
#else
    mbedtls_sha512_finish (ctx->handle,
                           full_digest);
#endif
    if (0 == ctx->ext_error)
    {
      /* SHA-512/256 uses first 32 bytes of SHA-512 with different IV */
      memcpy (digest,
              full_digest,
              mhd_SHA512_256_DIGEST_SIZE);

      /* Reset for potential reuse */
      init512_256 (ctx);
    }
  }
}


/**
 * Free allocated resources.
 *
 * @param ctx the calculation context
 */
void
mhd_SHA512_256_deinit (struct mhd_Sha512_256CtxExt *ctx)
{
  if (NULL != ctx->handle)
  {
    mbedtls_sha512_free (ctx->handle);
    free (ctx->handle);
  }
}
