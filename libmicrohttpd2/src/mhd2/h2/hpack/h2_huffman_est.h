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
 * @file src/mhd2/h2/hpack/h2_huffman_est.h
 * @brief  HTTP/2 Huffman encoding size estimation functions
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_H2_HUFFMAN_EST_H
#define MHD_H2_HUFFMAN_EST_H 1

#include "mhd_sys_options.h"

#include "sys_base_types.h"

/**
 * Estimate minimal possible size of H2 Huffman encoder output
 * @param str_len the length of the original string to encode, must be
 *                within 32 bit value
 * @return the estimated minimal size of the encoded string; the real
 *         encoded size is most probably larger
 */
mhd_static_inline uint_least32_t
mhd_h2_huffman_est_min_size (uint_fast32_t str_len)
{
  const uint_fast64_t min_bits = ((uint_fast64_t) str_len) * 5u;
  mhd_assert (str_len == (str_len & 0xFFFFFFFFu));

  return (uint_least32_t) ((min_bits + 7u) / 8u);
}


/**
 * Estimate average expected size of H2 Huffman encoder output
 * @param str_len the length of the original string to encode, must be
 *                within 32 bit value
 * @return the estimated average size of the encoded string; the real
 *         encoded size may be larger or smaller
 */
mhd_static_inline uint_least32_t
mhd_h2_huffman_est_avg_size (uint_fast32_t str_len)
{
  uint_fast64_t est_bits;
  mhd_assert (str_len == (str_len & 0xFFFFFFFFu));

  /* Assume 6.5 bits per symbol in average */
  est_bits = ((uint_fast64_t) str_len) * 6u;
  est_bits += str_len / 2;
  return (uint_least32_t) ((est_bits + 7u) / 8u);
}


#endif /* ! MHD_H2_HUFFMAN_EST_H */
