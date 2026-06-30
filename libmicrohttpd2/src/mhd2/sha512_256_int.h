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
 * @file src/mhd2/sha512_256.h
 * @brief  Calculation of SHA-512/256 digest
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_SHA512_256_INT_H
#define MHD_SHA512_256_INT_H 1

#include "mhd_sys_options.h"

#include "sys_base_types.h"


/**
 * Number of bits in single SHA-512/256 word.
 */
#define mhd_SHA512_256_WORD_SIZE_BITS 64

/**
 * Number of bytes in single SHA-512/256 word.
 */
#define mhd_SHA512_256_BYTES_IN_WORD (mhd_SHA512_256_WORD_SIZE_BITS / 8)

/**
 * Hash is kept internally as 8 64-bit words.
 * This is intermediate hash size, used during computing the final digest.
 */
#define mhd_SHA512_256_HASH_SIZE_WORDS 8

/**
 * Size of SHA-512/256 resulting digest in words.
 * This is the final digest size, not intermediate hash.
 */
#define mhd_SHA512_256_DIGEST_SIZE_WORDS (mhd_SHA512_256_HASH_SIZE_WORDS  / 2)

/**
 * Size of SHA-512/256 resulting digest in bytes.
 * This is the final digest size, not intermediate hash.
 */
#define mhd_SHA512_256_DIGEST_SIZE \
        (mhd_SHA512_256_DIGEST_SIZE_WORDS * mhd_SHA512_256_BYTES_IN_WORD)

/**
 * Size of SHA-512/256 single processing block in bits.
 */
#define mhd_SHA512_256_BLOCK_SIZE_BITS 1024

/**
 * Size of SHA-512/256 single processing block in bytes.
 */
#define mhd_SHA512_256_BLOCK_SIZE (mhd_SHA512_256_BLOCK_SIZE_BITS / 8)

/**
 * Size of SHA-512/256 single processing block in words.
 */
#define mhd_SHA512_256_BLOCK_SIZE_WORDS \
        (mhd_SHA512_256_BLOCK_SIZE_BITS / mhd_SHA512_256_WORD_SIZE_BITS)


/**
 * SHA-512/256 calculation context
 */
struct mhd_Sha512_256CtxInt
{
  uint64_t H[mhd_SHA512_256_HASH_SIZE_WORDS];       /**< Intermediate hash value  */
  uint64_t buffer[mhd_SHA512_256_BLOCK_SIZE_WORDS]; /**< SHA512_256 input data buffer */
  /**
   * The number of bytes, lower part
   */
  uint64_t count;
  /**
   * The number of bits, high part.
   * Unlike lower part, this counts the number of bits, not bytes.
   */
  uint64_t count_bits_hi;
};

/**
 * Initialise structure for SHA-512/256 calculation.
 *
 * @param ctx the calculation context
 */
MHD_INTERNAL void
mhd_SHA512_256_init (struct mhd_Sha512_256CtxInt *ctx)
MHD_FN_PAR_NONNULL_ALL_;


/**
 * Process portion of bytes.
 *
 * @param ctx the calculation context
 * @param size number of bytes in @a data, must not be 0
 * @param data bytes to add to hash
 */
MHD_INTERNAL void
mhd_SHA512_256_update (struct mhd_Sha512_256CtxInt *restrict ctx,
                       size_t size,
                       const uint8_t *restrict data)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_IN_SIZE_ (3, 2);


/**
 * Finalise SHA-512/256 calculation, return digest.
 *
 * @param ctx the calculation context
 * @param[out] digest set to the hash, must be #mhd_SHA512_256_DIGEST_SIZE bytes
 */
MHD_INTERNAL void
mhd_SHA512_256_finish (struct mhd_Sha512_256CtxInt *restrict ctx,
                       uint8_t digest[mhd_SHA512_256_DIGEST_SIZE])
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_OUT_ (2);

/**
 * Indicates that function mhd_SHA512_256_finish() (without context reset) is
 * available
 */
#define mhd_SHA512_256_HAS_FINISH 1

#endif /* MHD_SHA512_256_H */
