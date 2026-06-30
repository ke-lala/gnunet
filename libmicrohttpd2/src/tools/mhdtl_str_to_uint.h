/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2023-2025 Evgeny Grin (Karlson2k)

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
 * @file src/tools/mhdtl_str_to_uint.h
 * @brief  Function to decode the value of decimal string number.
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHDTL_STR_TO_UINT_H_
#define MHDTL_STR_TO_UINT_H_ 1

#include <stddef.h>

/**
 * Convert decimal string to unsigned int.
 * Function stops at the end of the string or on first non-digit character.
 * @param str the string to convert
 * @param[out] value the pointer to put the result
 * @return return the number of digits converted or
 *         zero if no digits found or result would overflow the output
 *         variable (the output set to UINT_MAX in this case).
 */
static inline size_t
mhdtl_str_to_uint (const char *str, unsigned int *value)
{
  size_t i;
  unsigned int v = 0;

  for (i = 0; 0 != str[i]; ++i)
  {
    const char chr = str[i];
    unsigned int digit;
    unsigned int prev_v;
    if (('0' > chr) || ('9' < chr))
      break;
    digit = (unsigned char) (chr - '0');
    prev_v = v;
    v *= 10;
    if (v / 10 == prev_v)
    {
      prev_v = v;
      v += digit;
      if (v >= prev_v)
        continue;
    }
    /* Overflow */
    *value = 0U - 1;
    return 0;
  }
  *value = v;
  return i;
}


#endif /* MHDTL_STR_TO_UINT_H_ */
