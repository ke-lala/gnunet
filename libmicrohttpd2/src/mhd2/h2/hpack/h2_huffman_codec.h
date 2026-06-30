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
 * @file src/mhd2/h2/hpack/h2_huffman_codec.h
 * @brief  The declaration for HTTP/2 Huffman encoding and decoding function
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_H2_HUFFMAN_CODEC_H
#define MHD_H2_HUFFMAN_CODEC_H 1

#include "mhd_sys_options.h"

#include "sys_sizet_type.h"

/**
 * Perform one-time initialisation of H2 static Huffman decoding tables
 *
 * Must be always be called once before calling #mhd_h2_huffman_decode().
 */
MHD_INTERNAL void
mhd_h2_huffman_init (void);


/**
 * Perform H2 static Huffman encoding
 * @param str_len the size of the @a str, must not be zero
 * @param str the pointer to the data to encoded,
 *            does not need to be zero-terminated
 * @param out_buf_size the size of the @a out_buf; when used inside MHD
 *                     must not be larger than @a str_len
 * @param out_buf the output buffer to put the encoded data. May be altered
 *            even if encoding  failed.
 * @return non-zero size of the encoded data placed to the @a out_buf on
 *         success,
 *         zero if output buffer is not large enough,
 */
MHD_INTERNAL size_t
mhd_h2_huffman_encode (size_t str_len,
                       const char *restrict str,
                       size_t out_buf_size,
                       void *restrict out_buf)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_IN_SIZE_ (2,1) MHD_FN_PAR_OUT_SIZE_ (4,3);


/**
 * Result of H2 static Huffman decoding
 */
enum MHD_FIXED_ENUM_ mhd_H2HuffDecodeRes
{
  /**
   * Decoded successfully
   */
  MHD_H2_HUFF_DEC_RES_OK = 0
  ,
  /**
   * The output buffer is too small
   */
  MHD_H2_HUFF_DEC_RES_NO_SPACE
  ,
  /**
   * Encoded data is invalid (malformed or wrong padding)
   */
  MHD_H2_HUFF_DEC_RES_BROKEN_DATA
};

/**
 * Perform H2 static Huffman decoding
 * @param encoded_size the size of the data pointed by the @a encoded pointer,
 *                     must not be zero
 * @param encoded the pointer to the data to decode
 * @param out_buf_size the size of the @a out_buf
 * @param out_buf the output buffer to put the decoded data. The decoded
 *                data is NOT zero-terminated. May be altered even if decoding
 *                failed.
 * @param decode_result the pointer to variable to set to the decoding status,
 *                      always set to #MHD_H2_HUFF_DEC_RES_OK if returned
 *                      value is not zero.
 * @return non-zero size of the decoded data placed to the @a out_buf
 *         on success,
 *         zero if decoding is failed (see @a decode_result).
 */
MHD_INTERNAL size_t
mhd_h2_huffman_decode (size_t encoded_size,
                       const void *restrict encoded,
                       size_t out_buf_size,
                       char *restrict out_buf,
                       enum mhd_H2HuffDecodeRes *restrict decode_result)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_IN_SIZE_ (2,1) MHD_FN_PAR_OUT_SIZE_ (4,3);


#endif /* ! MHD_H2_HUFFMAN_CODEC_H */
