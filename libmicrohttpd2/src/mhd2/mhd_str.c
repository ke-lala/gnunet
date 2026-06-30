/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2015-2024 Evgeny Grin (Karlson2k)

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
 * @file microhttpd/mhd_str.c
 * @brief  Functions implementations for string manipulating
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "mhd_assert.h"
#include "mhd_limits.h"

#include "mhd_constexpr.h"
#include "mhd_assume.h"

#include <string.h>

#include "mhd_str.h"

#ifdef MHD_FAVOR_SMALL_CODE
#  ifdef mhd_static_inline
#    undef mhd_static_inline
#  endif /* mhd_static_inline */
/* Do not force inlining and do not use macro functions, use normal static
   functions instead.
   This may give more flexibility for size optimizations. */
#  define mhd_static_inline static
#  ifndef HAVE_INLINE_FUNCS
#    define HAVE_INLINE_FUNCS 1
#  endif /* !HAVE_INLINE_FUNCS */
#endif /* MHD_FAVOR_SMALL_CODE */

/*
 * Block of functions/macros that use US-ASCII charset as required by HTTP
 * standards. Not affected by current locale settings.
 */

#ifdef HAVE_INLINE_FUNCS

#ifdef mhd_HAVE_STR_TO_UPPER
/**
 * Check whether character is lower case letter in US-ASCII
 *
 * @param c character to check
 * @return non-zero if character is lower case letter, zero otherwise
 */
mhd_static_inline MHD_FN_CONST_ bool
isasciilower (char c)
{
  const unsigned int uc = (unsigned int) (unsigned char) c;
  const unsigned int t = uc - (unsigned int) (unsigned char) 'a';
  return (((unsigned int) ('z' - 'a')) >= t);
}


#endif /* mhd_HAVE_STR_TO_UPPER */


/**
 * Check whether character is upper case letter in US-ASCII
 *
 * @param c character to check
 * @return non-zero if character is upper case letter, zero otherwise
 */
mhd_static_inline MHD_FN_CONST_ bool
isasciiupper (char c)
{
  const unsigned int uc = (unsigned int) (unsigned char) c;
  const unsigned int t = uc - (unsigned int) (unsigned char) 'A';
  return (((unsigned int) ('Z' - 'A')) >= t);
}


#if 0 /* Disable unused functions. */
/**
 * Check whether character is letter in US-ASCII
 *
 * @param c character to check
 * @return non-zero if character is letter in US-ASCII, zero otherwise
 */
mhd_static_inline MHD_FN_CONST_ bool
isasciialpha (char c)
{
  return isasciilower (c) || isasciiupper (c);
}


#endif /* Disable unused functions. */


/**
 * Check whether character is decimal digit in US-ASCII
 *
 * @param c character to check
 * @return non-zero if character is decimal digit, zero otherwise
 */
mhd_static_inline MHD_FN_CONST_ bool
isasciidigit (char c)
{
  return (c <= '9') && (c >= '0');
}


#if 0 /* Disable unused functions. */
/**
 * Check whether character is hexadecimal digit in US-ASCII
 *
 * @param c character to check
 * @return non-zero if character is decimal digit, zero otherwise
 */
mhd_static_inline MHD_FN_CONST_ bool
isasciixdigit (char c)
{
  return isasciidigit (c) ||
         ( (c <= 'F') && (c >= 'A') ) ||
         ( (c <= 'f') && (c >= 'a') );
}


/**
 * Check whether character is decimal digit or letter in US-ASCII
 *
 * @param c character to check
 * @return non-zero if character is decimal digit or letter, zero otherwise
 */
mhd_static_inline MHD_FN_CONST_ bool
isasciialnum (char c)
{
  return isasciialpha (c) || isasciidigit (c);
}


#endif /* Disable unused functions. */

/**
 * Convert US-ASCII character to lower case.
 * If character is upper case letter in US-ASCII than it's converted to lower
 * case analog. If character is NOT upper case letter than it's returned
 * unmodified.
 *
 * @param c character to convert
 * @return converted to lower case character
 */
mhd_static_inline MHD_FN_CONST_ char
toasciilower (char c)
{
  return (char) (((unsigned char) c) | ((isasciiupper (c) ? 1u : 0u) << 5u));
}


#ifdef mhd_HAVE_STR_TO_UPPER

/**
 * Convert US-ASCII character to upper case.
 * If character is lower case letter in US-ASCII than it's converted to upper
 * case counterpart. If character is NOT lower case letter than it's returned
 * unmodified.
 *
 * @param c character to convert
 * @return converted to upper case character
 */
mhd_static_inline MHD_FN_CONST_ char
toasciiupper (char c)
{
  return (char) (unsigned char)
         (((unsigned char) c) & ~((isasciilower (c) ? 1u : 0u) << 5u));
}


#endif /* mhd_HAVE_STR_TO_UPPER */


#if defined(MHD_FAVOR_SMALL_CODE) /* Used only in mhd_str_to_uvalue_n() */
/**
 * Convert US-ASCII decimal digit to its value.
 *
 * @param c character to convert
 * @return value of decimal digit or -1 if @ c is not decimal digit
 */
mhd_static_inline MHD_FN_CONST_ int
todigitvalue (char c)
{
  if (isasciidigit (c))
    return (unsigned char) (c - '0');

  return -1;
}


#endif /* MHD_FAVOR_SMALL_CODE */


/**
 * Convert US-ASCII hexadecimal digit to its value.
 *
 * @param c character to convert
 * @return value of hexadecimal digit or -1 if @ c is not hexadecimal digit
 */
mhd_static_inline MHD_FN_CONST_ int
xdigittovalue (char c)
{
  const unsigned char uc = (unsigned char) c; /* Force unsigned value */
#if ! defined(MHD_FAVOR_SMALL_CODE)
  static const signed char map_xdigit_to_value[256] = {
    -1 /* 0x00 (NUL) */,
    -1 /* 0x01 (SOH) */,
    -1 /* 0x02 (STX) */,
    -1 /* 0x03 (ETX) */,
    -1 /* 0x04 (EOT) */,
    -1 /* 0x05 (ENQ) */,
    -1 /* 0x06 (ACK) */,
    -1 /* 0x07 (BEL) */,
    -1 /* 0x08 (BS)  */,
    -1 /* 0x09 (HT)  */,
    -1 /* 0x0A (LF)  */,
    -1 /* 0x0B (VT)  */,
    -1 /* 0x0C (FF)  */,
    -1 /* 0x0D (CR)  */,
    -1 /* 0x0E (SO)  */,
    -1 /* 0x0F (SI)  */,
    -1 /* 0x10 (DLE) */,
    -1 /* 0x11 (DC1) */,
    -1 /* 0x12 (DC2) */,
    -1 /* 0x13 (DC3) */,
    -1 /* 0x14 (DC4) */,
    -1 /* 0x15 (NAK) */,
    -1 /* 0x16 (SYN) */,
    -1 /* 0x17 (ETB) */,
    -1 /* 0x18 (CAN) */,
    -1 /* 0x19 (EM)  */,
    -1 /* 0x1A (SUB) */,
    -1 /* 0x1B (ESC) */,
    -1 /* 0x1C (FS)  */,
    -1 /* 0x1D (GS)  */,
    -1 /* 0x1E (RS)  */,
    -1 /* 0x1F (US)  */,
    -1 /* 0x20 (' ') */,
    -1 /* 0x21 ('!') */,
    -1 /* 0x22 ('"') */,
    -1 /* 0x23 ('#') */,
    -1 /* 0x24 ('$') */,
    -1 /* 0x25 ('%') */,
    -1 /* 0x26 ('&') */,
    -1 /* 0x27 ('\'') */,
    -1 /* 0x28 ('(') */,
    -1 /* 0x29 (')') */,
    -1 /* 0x2A ('*') */,
    -1 /* 0x2B ('+') */,
    -1 /* 0x2C (',') */,
    -1 /* 0x2D ('-') */,
    -1 /* 0x2E ('.') */,
    -1 /* 0x2F ('/') */,
    0 /*  0x30 ('0') */,
    1 /*  0x31 ('1') */,
    2 /*  0x32 ('2') */,
    3 /*  0x33 ('3') */,
    4 /*  0x34 ('4') */,
    5 /*  0x35 ('5') */,
    6 /*  0x36 ('6') */,
    7 /*  0x37 ('7') */,
    8 /*  0x38 ('8') */,
    9 /*  0x39 ('9') */,
    -1 /* 0x3A (':') */,
    -1 /* 0x3B (';') */,
    -1 /* 0x3C ('<') */,
    -1 /* 0x3D ('=') */,
    -1 /* 0x3E ('>') */,
    -1 /* 0x3F ('?') */,
    -1 /* 0x40 ('@') */,
    10 /* 0x41 ('A') */,
    11 /* 0x42 ('B') */,
    12 /* 0x43 ('C') */,
    13 /* 0x44 ('D') */,
    14 /* 0x45 ('E') */,
    15 /* 0x46 ('F') */,
    -1 /* 0x47 ('G') */,
    -1 /* 0x48 ('H') */,
    -1 /* 0x49 ('I') */,
    -1 /* 0x4A ('J') */,
    -1 /* 0x4B ('K') */,
    -1 /* 0x4C ('L') */,
    -1 /* 0x4D ('M') */,
    -1 /* 0x4E ('N') */,
    -1 /* 0x4F ('O') */,
    -1 /* 0x50 ('P') */,
    -1 /* 0x51 ('Q') */,
    -1 /* 0x52 ('R') */,
    -1 /* 0x53 ('S') */,
    -1 /* 0x54 ('T') */,
    -1 /* 0x55 ('U') */,
    -1 /* 0x56 ('V') */,
    -1 /* 0x57 ('W') */,
    -1 /* 0x58 ('X') */,
    -1 /* 0x59 ('Y') */,
    -1 /* 0x5A ('Z') */,
    -1 /* 0x5B ('[') */,
    -1 /* 0x5C ('\') */,
    -1 /* 0x5D (']') */,
    -1 /* 0x5E ('^') */,
    -1 /* 0x5F ('_') */,
    -1 /* 0x60 ('`') */,
    10 /* 0x61 ('a') */,
    11 /* 0x62 ('b') */,
    12 /* 0x63 ('c') */,
    13 /* 0x64 ('d') */,
    14 /* 0x65 ('e') */,
    15 /* 0x66 ('f') */,
    -1 /* 0x67 ('g') */,
    -1 /* 0x68 ('h') */,
    -1 /* 0x69 ('i') */,
    -1 /* 0x6A ('j') */,
    -1 /* 0x6B ('k') */,
    -1 /* 0x6C ('l') */,
    -1 /* 0x6D ('m') */,
    -1 /* 0x6E ('n') */,
    -1 /* 0x6F ('o') */,
    -1 /* 0x70 ('p') */,
    -1 /* 0x71 ('q') */,
    -1 /* 0x72 ('r') */,
    -1 /* 0x73 ('s') */,
    -1 /* 0x74 ('t') */,
    -1 /* 0x75 ('u') */,
    -1 /* 0x76 ('v') */,
    -1 /* 0x77 ('w') */,
    -1 /* 0x78 ('x') */,
    -1 /* 0x79 ('y') */,
    -1 /* 0x7A ('z') */,
    -1 /* 0x7B ('{') */,
    -1 /* 0x7C ('|') */,
    -1 /* 0x7D ('}') */,
    -1 /* 0x7E ('~') */,
    -1 /* 0x7F (DEL) */,
    -1 /* 0x80 (EXT) */,
    -1 /* 0x81 (EXT) */,
    -1 /* 0x82 (EXT) */,
    -1 /* 0x83 (EXT) */,
    -1 /* 0x84 (EXT) */,
    -1 /* 0x85 (EXT) */,
    -1 /* 0x86 (EXT) */,
    -1 /* 0x87 (EXT) */,
    -1 /* 0x88 (EXT) */,
    -1 /* 0x89 (EXT) */,
    -1 /* 0x8A (EXT) */,
    -1 /* 0x8B (EXT) */,
    -1 /* 0x8C (EXT) */,
    -1 /* 0x8D (EXT) */,
    -1 /* 0x8E (EXT) */,
    -1 /* 0x8F (EXT) */,
    -1 /* 0x90 (EXT) */,
    -1 /* 0x91 (EXT) */,
    -1 /* 0x92 (EXT) */,
    -1 /* 0x93 (EXT) */,
    -1 /* 0x94 (EXT) */,
    -1 /* 0x95 (EXT) */,
    -1 /* 0x96 (EXT) */,
    -1 /* 0x97 (EXT) */,
    -1 /* 0x98 (EXT) */,
    -1 /* 0x99 (EXT) */,
    -1 /* 0x9A (EXT) */,
    -1 /* 0x9B (EXT) */,
    -1 /* 0x9C (EXT) */,
    -1 /* 0x9D (EXT) */,
    -1 /* 0x9E (EXT) */,
    -1 /* 0x9F (EXT) */,
    -1 /* 0xA0 (EXT) */,
    -1 /* 0xA1 (EXT) */,
    -1 /* 0xA2 (EXT) */,
    -1 /* 0xA3 (EXT) */,
    -1 /* 0xA4 (EXT) */,
    -1 /* 0xA5 (EXT) */,
    -1 /* 0xA6 (EXT) */,
    -1 /* 0xA7 (EXT) */,
    -1 /* 0xA8 (EXT) */,
    -1 /* 0xA9 (EXT) */,
    -1 /* 0xAA (EXT) */,
    -1 /* 0xAB (EXT) */,
    -1 /* 0xAC (EXT) */,
    -1 /* 0xAD (EXT) */,
    -1 /* 0xAE (EXT) */,
    -1 /* 0xAF (EXT) */,
    -1 /* 0xB0 (EXT) */,
    -1 /* 0xB1 (EXT) */,
    -1 /* 0xB2 (EXT) */,
    -1 /* 0xB3 (EXT) */,
    -1 /* 0xB4 (EXT) */,
    -1 /* 0xB5 (EXT) */,
    -1 /* 0xB6 (EXT) */,
    -1 /* 0xB7 (EXT) */,
    -1 /* 0xB8 (EXT) */,
    -1 /* 0xB9 (EXT) */,
    -1 /* 0xBA (EXT) */,
    -1 /* 0xBB (EXT) */,
    -1 /* 0xBC (EXT) */,
    -1 /* 0xBD (EXT) */,
    -1 /* 0xBE (EXT) */,
    -1 /* 0xBF (EXT) */,
    -1 /* 0xC0 (EXT) */,
    -1 /* 0xC1 (EXT) */,
    -1 /* 0xC2 (EXT) */,
    -1 /* 0xC3 (EXT) */,
    -1 /* 0xC4 (EXT) */,
    -1 /* 0xC5 (EXT) */,
    -1 /* 0xC6 (EXT) */,
    -1 /* 0xC7 (EXT) */,
    -1 /* 0xC8 (EXT) */,
    -1 /* 0xC9 (EXT) */,
    -1 /* 0xCA (EXT) */,
    -1 /* 0xCB (EXT) */,
    -1 /* 0xCC (EXT) */,
    -1 /* 0xCD (EXT) */,
    -1 /* 0xCE (EXT) */,
    -1 /* 0xCF (EXT) */,
    -1 /* 0xD0 (EXT) */,
    -1 /* 0xD1 (EXT) */,
    -1 /* 0xD2 (EXT) */,
    -1 /* 0xD3 (EXT) */,
    -1 /* 0xD4 (EXT) */,
    -1 /* 0xD5 (EXT) */,
    -1 /* 0xD6 (EXT) */,
    -1 /* 0xD7 (EXT) */,
    -1 /* 0xD8 (EXT) */,
    -1 /* 0xD9 (EXT) */,
    -1 /* 0xDA (EXT) */,
    -1 /* 0xDB (EXT) */,
    -1 /* 0xDC (EXT) */,
    -1 /* 0xDD (EXT) */,
    -1 /* 0xDE (EXT) */,
    -1 /* 0xDF (EXT) */,
    -1 /* 0xE0 (EXT) */,
    -1 /* 0xE1 (EXT) */,
    -1 /* 0xE2 (EXT) */,
    -1 /* 0xE3 (EXT) */,
    -1 /* 0xE4 (EXT) */,
    -1 /* 0xE5 (EXT) */,
    -1 /* 0xE6 (EXT) */,
    -1 /* 0xE7 (EXT) */,
    -1 /* 0xE8 (EXT) */,
    -1 /* 0xE9 (EXT) */,
    -1 /* 0xEA (EXT) */,
    -1 /* 0xEB (EXT) */,
    -1 /* 0xEC (EXT) */,
    -1 /* 0xED (EXT) */,
    -1 /* 0xEE (EXT) */,
    -1 /* 0xEF (EXT) */,
    -1 /* 0xF0 (EXT) */,
    -1 /* 0xF1 (EXT) */,
    -1 /* 0xF2 (EXT) */,
    -1 /* 0xF3 (EXT) */,
    -1 /* 0xF4 (EXT) */,
    -1 /* 0xF5 (EXT) */,
    -1 /* 0xF6 (EXT) */,
    -1 /* 0xF7 (EXT) */,
    -1 /* 0xF8 (EXT) */,
    -1 /* 0xF9 (EXT) */,
    -1 /* 0xFA (EXT) */,
    -1 /* 0xFB (EXT) */,
    -1 /* 0xFC (EXT) */,
    -1 /* 0xFD (EXT) */,
    -1 /* 0xFE (EXT) */,
    -1   /* 0xFF (EXT) */
  };
  return map_xdigit_to_value[uc];
#else  /* MHD_FAVOR_SMALL_CODE */
  unsigned int try_val;

  try_val = uc - (unsigned char) '0';
  if (9 >= try_val)
    return (int) (unsigned int) try_val;
  try_val = (uc | 0x20u /* fold case */) - (unsigned char) 'a';
  if (5 >= try_val)
    return (int) (unsigned int) (try_val + 10u);

  return -1;
#endif /* MHD_FAVOR_SMALL_CODE */
}


/**
 * Convert 4 bit value to US-ASCII hexadecimal digit.
 *
 * @param v the value to convert, must be less then 16
 * @return hexadecimal digit
 */
mhd_static_inline MHD_FN_CONST_ char
valuetoxdigit (unsigned int v)
{
#if ! defined(MHD_FAVOR_SMALL_CODE)
  static const char map_value_to_xdigit[16] =
  { '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

  mhd_assert (16 > v);

  return map_value_to_xdigit[v];
#else  /* MHD_FAVOR_SMALL_CODE */

  mhd_assert (16 > v);

  if (v <= 9)
    return '0' + (char) (v);
  return 'a' + (char) (v - 10);
#endif /* MHD_FAVOR_SMALL_CODE */
}


#if ! defined(MHD_FAVOR_SMALL_CODE)
/**
 * Convert 8 bit value to two US-ASCII hexadecimal digits.
 *
 * @param v the value to convert
 * @return pointer to char[2] with two hexadecimal digits
 */
mhd_static_inline MHD_FN_CONST_ MHD_FN_RETURNS_NONNULL_ const char*
uint8totwoxdigits (uint8_t v)
{
  static const char map_uint8_to_two_xdigits[][2] =
  { { '0', '0' },
    { '0', '1' },
    { '0', '2' },
    { '0', '3' },
    { '0', '4' },
    { '0', '5' },
    { '0', '6' },
    { '0', '7' },
    { '0', '8' },
    { '0', '9' },
    { '0', 'a' },
    { '0', 'b' },
    { '0', 'c' },
    { '0', 'd' },
    { '0', 'e' },
    { '0', 'f' },
    { '1', '0' },
    { '1', '1' },
    { '1', '2' },
    { '1', '3' },
    { '1', '4' },
    { '1', '5' },
    { '1', '6' },
    { '1', '7' },
    { '1', '8' },
    { '1', '9' },
    { '1', 'a' },
    { '1', 'b' },
    { '1', 'c' },
    { '1', 'd' },
    { '1', 'e' },
    { '1', 'f' },
    { '2', '0' },
    { '2', '1' },
    { '2', '2' },
    { '2', '3' },
    { '2', '4' },
    { '2', '5' },
    { '2', '6' },
    { '2', '7' },
    { '2', '8' },
    { '2', '9' },
    { '2', 'a' },
    { '2', 'b' },
    { '2', 'c' },
    { '2', 'd' },
    { '2', 'e' },
    { '2', 'f' },
    { '3', '0' },
    { '3', '1' },
    { '3', '2' },
    { '3', '3' },
    { '3', '4' },
    { '3', '5' },
    { '3', '6' },
    { '3', '7' },
    { '3', '8' },
    { '3', '9' },
    { '3', 'a' },
    { '3', 'b' },
    { '3', 'c' },
    { '3', 'd' },
    { '3', 'e' },
    { '3', 'f' },
    { '4', '0' },
    { '4', '1' },
    { '4', '2' },
    { '4', '3' },
    { '4', '4' },
    { '4', '5' },
    { '4', '6' },
    { '4', '7' },
    { '4', '8' },
    { '4', '9' },
    { '4', 'a' },
    { '4', 'b' },
    { '4', 'c' },
    { '4', 'd' },
    { '4', 'e' },
    { '4', 'f' },
    { '5', '0' },
    { '5', '1' },
    { '5', '2' },
    { '5', '3' },
    { '5', '4' },
    { '5', '5' },
    { '5', '6' },
    { '5', '7' },
    { '5', '8' },
    { '5', '9' },
    { '5', 'a' },
    { '5', 'b' },
    { '5', 'c' },
    { '5', 'd' },
    { '5', 'e' },
    { '5', 'f' },
    { '6', '0' },
    { '6', '1' },
    { '6', '2' },
    { '6', '3' },
    { '6', '4' },
    { '6', '5' },
    { '6', '6' },
    { '6', '7' },
    { '6', '8' },
    { '6', '9' },
    { '6', 'a' },
    { '6', 'b' },
    { '6', 'c' },
    { '6', 'd' },
    { '6', 'e' },
    { '6', 'f' },
    { '7', '0' },
    { '7', '1' },
    { '7', '2' },
    { '7', '3' },
    { '7', '4' },
    { '7', '5' },
    { '7', '6' },
    { '7', '7' },
    { '7', '8' },
    { '7', '9' },
    { '7', 'a' },
    { '7', 'b' },
    { '7', 'c' },
    { '7', 'd' },
    { '7', 'e' },
    { '7', 'f' },
    { '8', '0' },
    { '8', '1' },
    { '8', '2' },
    { '8', '3' },
    { '8', '4' },
    { '8', '5' },
    { '8', '6' },
    { '8', '7' },
    { '8', '8' },
    { '8', '9' },
    { '8', 'a' },
    { '8', 'b' },
    { '8', 'c' },
    { '8', 'd' },
    { '8', 'e' },
    { '8', 'f' },
    { '9', '0' },
    { '9', '1' },
    { '9', '2' },
    { '9', '3' },
    { '9', '4' },
    { '9', '5' },
    { '9', '6' },
    { '9', '7' },
    { '9', '8' },
    { '9', '9' },
    { '9', 'a' },
    { '9', 'b' },
    { '9', 'c' },
    { '9', 'd' },
    { '9', 'e' },
    { '9', 'f' },
    { 'a', '0' },
    { 'a', '1' },
    { 'a', '2' },
    { 'a', '3' },
    { 'a', '4' },
    { 'a', '5' },
    { 'a', '6' },
    { 'a', '7' },
    { 'a', '8' },
    { 'a', '9' },
    { 'a', 'a' },
    { 'a', 'b' },
    { 'a', 'c' },
    { 'a', 'd' },
    { 'a', 'e' },
    { 'a', 'f' },
    { 'b', '0' },
    { 'b', '1' },
    { 'b', '2' },
    { 'b', '3' },
    { 'b', '4' },
    { 'b', '5' },
    { 'b', '6' },
    { 'b', '7' },
    { 'b', '8' },
    { 'b', '9' },
    { 'b', 'a' },
    { 'b', 'b' },
    { 'b', 'c' },
    { 'b', 'd' },
    { 'b', 'e' },
    { 'b', 'f' },
    { 'c', '0' },
    { 'c', '1' },
    { 'c', '2' },
    { 'c', '3' },
    { 'c', '4' },
    { 'c', '5' },
    { 'c', '6' },
    { 'c', '7' },
    { 'c', '8' },
    { 'c', '9' },
    { 'c', 'a' },
    { 'c', 'b' },
    { 'c', 'c' },
    { 'c', 'd' },
    { 'c', 'e' },
    { 'c', 'f' },
    { 'd', '0' },
    { 'd', '1' },
    { 'd', '2' },
    { 'd', '3' },
    { 'd', '4' },
    { 'd', '5' },
    { 'd', '6' },
    { 'd', '7' },
    { 'd', '8' },
    { 'd', '9' },
    { 'd', 'a' },
    { 'd', 'b' },
    { 'd', 'c' },
    { 'd', 'd' },
    { 'd', 'e' },
    { 'd', 'f' },
    { 'e', '0' },
    { 'e', '1' },
    { 'e', '2' },
    { 'e', '3' },
    { 'e', '4' },
    { 'e', '5' },
    { 'e', '6' },
    { 'e', '7' },
    { 'e', '8' },
    { 'e', '9' },
    { 'e', 'a' },
    { 'e', 'b' },
    { 'e', 'c' },
    { 'e', 'd' },
    { 'e', 'e' },
    { 'e', 'f' },
    { 'f', '0' },
    { 'f', '1' },
    { 'f', '2' },
    { 'f', '3' },
    { 'f', '4' },
    { 'f', '5' },
    { 'f', '6' },
    { 'f', '7' },
    { 'f', '8' },
    { 'f', '9' },
    { 'f', 'a' },
    { 'f', 'b' },
    { 'f', 'c' },
    { 'f', 'd' },
    { 'f', 'e' },
    { 'f', 'f' }
#ifndef NDEBUG
    ,
    { 0, 0 }
#endif /* ! NDEBUG */
  };

  mhd_assert (257u == \
              (sizeof(map_uint8_to_two_xdigits) \
               / sizeof(map_uint8_to_two_xdigits[0])));

  return map_uint8_to_two_xdigits[v];
  /**
   * Indicates that function uint8totwoxdigits() is available
   */
#define mhd_HAVE_UINT8TOTWOXDIGITS 1
}


#endif /* ! MHD_FAVOR_SMALL_CODE */


/**
 * Caseless compare two characters.
 *
 * @param c1 the first char to compare
 * @param c2 the second char to compare
 * @return boolean 'true' if chars are caseless equal, false otherwise
 */
mhd_static_inline MHD_FN_CONST_ bool
charsequalcaseless (char c1, char c2)
{
  if (c1 == c2)
    return true;
  /* Fold case on both sides */
  c1 = ((char) (~0x20u & (unsigned char) c1));
  c2 = ((char) (~0x20u & (unsigned char) c2));
  return (c1 == c2) && isasciiupper (c1);
}


/**
 * Compare mixed case and lower case characters.
 *
 * @param mc the mixed case char to compare
 * @param lc the lower case char to compare
 * @return boolean 'true' if chars are caseless equal, false otherwise
 */
mhd_static_inline MHD_FN_CONST_ bool
charsequallowercase (char mc, char lc)
{
  char uc;
  if (mc == lc)
    return true;
  uc = ((char) (~0x20u & (unsigned char) lc));
  return (mc == uc) && isasciiupper (mc);
}


#else  /* !HAVE_INLINE_FUNCS */


/**
 * Checks whether character is lower case letter in US-ASCII
 *
 * @param c character to check
 * @return boolean true if character is lower case letter,
 *         boolean false otherwise
 */
#  define isasciilower(c) ((((char) (c)) >= 'a') && (((char) (c)) <= 'z'))


/**
 * Checks whether character is upper case letter in US-ASCII
 *
 * @param c character to check
 * @return boolean true if character is upper case letter,
 *         boolean false otherwise
 */
#  define isasciiupper(c) ((((char) (c)) <= 'Z') && (((char) (c)) >= 'A'))


/**
 * Checks whether character is letter in US-ASCII
 *
 * @param c character to check
 * @return boolean true if character is letter, boolean false
 *         otherwise
 */
#  define isasciialpha(c) (isasciilower (c) || isasciiupper (c))


/**
 * Check whether character is decimal digit in US-ASCII
 *
 * @param c character to check
 * @return boolean true if character is decimal digit, boolean false
 *         otherwise
 */
#  define isasciidigit(c) ((((char) (c)) <= '9') && (((char) (c)) >= '0'))


/**
 * Check whether character is hexadecimal digit in US-ASCII
 *
 * @param c character to check
 * @return boolean true if character is hexadecimal digit,
 *         boolean false otherwise
 */
#  define isasciixdigit(c) (isasciidigit ((c)) || \
                            (((char) (c)) <= 'F' && ((char) (c)) >= 'A') || \
                            (((char) (c)) <= 'f' && ((char) (c)) >= 'a'))


/**
 * Check whether character is decimal digit or letter in US-ASCII
 *
 * @param c character to check
 * @return boolean true if character is decimal digit or letter,
 *         boolean false otherwise
 */
#  define isasciialnum(c) (isasciialpha (c) || isasciidigit (c))


/**
 * Convert US-ASCII character to lower case.
 * If character is upper case letter in US-ASCII than it's converted to lower
 * case analog. If character is NOT upper case letter than it's returned
 * unmodified.
 *
 * @param c character to convert
 * @return converted to lower case character
 */
#  define toasciilower(c) \
        ((isasciiupper (c)) ? (((char) (c)) - 'A' + 'a') : ((char) (c)))


/**
 * Convert US-ASCII character to upper case.
 * If character is lower case letter in US-ASCII than it's converted to upper
 * case analog. If character is NOT lower case letter than it's returned
 * unmodified.
 *
 * @param c character to convert
 * @return converted to upper case character
 */
#  define toasciiupper(c) ((isasciilower (c)) ? (((char) (c)) - 'a' + 'A') : \
                           ((char) (c)))


/**
 * Convert US-ASCII decimal digit to its value.
 *
 * @param c character to convert
 * @return value of hexadecimal digit or -1 if @ c is not hexadecimal digit
 */
#  define todigitvalue(c) (isasciidigit (c) ? (int) (((char) (c)) - '0') : \
                           (int) (-1))


/**
 * Convert US-ASCII hexadecimal digit to its value.
 * @param c character to convert
 * @return value of hexadecimal digit or -1 if @ c is not hexadecimal digit
 */
#  define xdigittovalue(c) (isasciidigit (c) ? (int) (((char) (c)) - '0') : \
                            ( (((char) (c)) >= 'A' && ((char) (c)) <= 'F') ? \
                              (int) (((unsigned char) (c)) - 'A' + 10) : \
                              ( (((char) (c)) >= 'a' && ((char) (c)) <= 'f') ? \
                                (int) (((unsigned char) (c)) - 'a' + 10) : \
                                (int) (-1) )))


#if ! defined(MHD_FAVOR_SMALL_CODE)
static const char map_value_to_xdigit[16] =
{ '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

/**
 * Convert 4 bit value to US-ASCII hexadecimal digit.
 *
 * @param v the value to convert, must be less then 16
 * @return hexadecimal digit
 */
#    define valuetoxdigit(v) map_value_to_xdigit[v]
#else  /* MHD_FAVOR_SMALL_CODE */
/**
 * Convert 4 bit value to US-ASCII hexadecimal digit.
 *
 * @param v the value to convert, must be less then 16
 * @return hexadecimal digit
 */
 #    define valuetoxdigit(v) \
         (char) ((v <= 9) ? ('0' + (char) v) : ('a' + (char) v - 10))
#endif /* MHD_FAVOR_SMALL_CODE */

/**
 * Caseless compare two characters.
 *
 * @param c1 the first char to compare
 * @param c2 the second char to compare
 * @return boolean 'true' if chars are caseless equal, false otherwise
 */
#define charsequalcaseless(c1, c2) \
        (((c1) == (c2)) || \
         (((0x20u | (unsigned char) (c1)) == (0x20u | (unsigned char) (c2))) \
          && isasciilower (((char) (0x20u | (unsigned char) (c2))))) )

/**
  * Compare mixed case and lower case characters.
  *
  * @param mc the mixed case char to compare
  * @param lc the lower case char to compare
  * @return boolean 'true' if chars are caseless equal, false otherwise
  */
#define charsequallowercase(mc,lc) \
        ( ((mc) == (lc)) || \
          (((0x20u | (unsigned char) (mc)) == ((unsigned char) (lc))) && \
           isasciilower (lc)) )
#endif /* !HAVE_INLINE_FUNCS */


#ifndef MHD_FAVOR_SMALL_CODE
MHD_INTERNAL MHD_FN_PURE_ MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_CSTR_ (1) MHD_FN_PAR_CSTR_ (2) bool
mhd_str_equal_caseless (const char *str1,
                        const char *str2)
{
  while (0 != (*str1))
  {
    const char c1 = *str1;
    const char c2 = *str2;
    if (charsequalcaseless (c1, c2))
    {
      str1++;
      str2++;
    }
    else
      return false;
  }
  return 0 == (*str2);
}


#endif /* ! MHD_FAVOR_SMALL_CODE */


MHD_INTERNAL MHD_FN_PURE_ MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_ (1) MHD_FN_PAR_IN_ (2) bool
mhd_str_equal_caseless_n (const char *const str1,
                          const char *const str2,
                          size_t maxlen)
{
  size_t i;

  for (i = 0; i < maxlen; ++i)
  {
    const char c1 = str1[i];
    const char c2 = str2[i];
    if (0 == c2)
      return 0 == c1;
    if (charsequalcaseless (c1, c2))
      continue;
    else
      return false;
  }
  return true;
}


MHD_INTERNAL MHD_FN_PURE_ MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_ (1) MHD_FN_PAR_IN_ (2) bool
mhd_str_equal_caseless_bin_n (const char *const str1,
                              const char *const str2,
                              size_t len)
{
  size_t i;

  for (i = 0; i < len; ++i)
  {
    const char c1 = str1[i];
    const char c2 = str2[i];
    if (charsequalcaseless (c1, c2))
      continue;
    else
      return 0;
  }
  return ! 0;
}


MHD_INTERNAL MHD_FN_PURE_ MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_ (1)
MHD_FN_PAR_IN_ (2) bool
mhd_str_equal_lowercase_bin_n (const char *const mixstr,
                               const char *const lowstr,
                               size_t len)
{
  size_t i;

  for (i = 0; i < len; ++i)
  {
    const char mc = mixstr[i];
    const char lc = lowstr[i];
    mhd_assert (! isasciiupper (lc));
    if (! charsequallowercase (mc, lc))
      return false;
  }
  return true;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (2,1)
MHD_FN_PAR_OUT_SIZE_ (3,1) void
mhd_str_to_lowercase_bin_n (size_t size,
                            const char *restrict inbuff,
                            char *restrict outbuff)
{
  size_t i;

  for (i = 0; i < size; ++i)
    outbuff[i] = toasciilower (inbuff[i]);
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (2,1) bool
mhd_str_is_lowercase_bin_n (size_t len,
                            const char *restrict str)
{
  size_t i;

  for (i = 0; i < len; ++i)
    if (isasciiupper (str[i]))
      return false;

  return true;
}


#ifdef mhd_HAVE_STR_TO_UPPER
MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (2,1)
MHD_FN_PAR_OUT_SIZE_ (3,1) void
mhd_str_to_uppercase_bin_n (size_t size,
                            const char *restrict inbuff,
                            char *restrict outbuff)
{
  size_t i;

  for (i = 0; i < size; ++i)
    outbuff[i] = toasciiupper (inbuff[i]);
}


#endif /* mhd_HAVE_STR_TO_UPPER */


MHD_INTERNAL MHD_FN_PURE_ MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_CSTR_ (1)
MHD_FN_PAR_IN_ (1) MHD_FN_PAR_IN_ (2) bool
mhd_str_has_token_caseless (const char *restrict str,
                            const char *const restrict token,
                            size_t token_len)
{
  if (0 == token_len)
    return false;

  while (0 != *str)
  {
    size_t i;
    /* Skip all whitespaces and empty tokens. */
    while (' ' == *str || '\t' == *str || ',' == *str)
      str++;

    /* Check for token match. */
    i = 0;
    while (1)
    {
      const char sc = *(str++);
      const char tc = token[i++];

      if (0 == sc)
        return false;
      if (! charsequalcaseless (sc, tc))
        break;
      if (i >= token_len)
      {
        /* Check whether substring match token fully or
         * has additional unmatched chars at tail. */
        while (' ' == *str || '\t' == *str)
          str++;
        /* End of (sub)string? */
        if ((0 == *str) || (',' == *str) )
          return true;
        /* Unmatched chars at end of substring. */
        break;
      }
    }
    /* Find next substring. */
    while (0 != *str && ',' != *str)
      str++;
  }
  return false;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (1,2) MHD_FN_PAR_IN_SIZE_ (3,4)
MHD_FN_PAR_OUT_ (5) MHD_FN_PAR_INOUT_ (6) bool
mhd_str_remove_token_caseless (const char *restrict str,
                               size_t str_len,
                               const char *const restrict token,
                               const size_t token_len,
                               char *restrict buf,
                               ssize_t *restrict buf_size)
{
  const char *s1; /**< the "input" string / character */
  char *s2;       /**< the "output" string / character */
  size_t t_pos;   /**< position of matched character in the token */
  bool token_removed;

  mhd_assert (NULL == memchr (token, 0, token_len));
  mhd_assert (NULL == memchr (token, ' ', token_len));
  mhd_assert (NULL == memchr (token, '\t', token_len));
  mhd_assert (NULL == memchr (token, ',', token_len));
  mhd_assert (0 <= *buf_size);

  if (SSIZE_MAX <= ((str_len / 2) * 3 + 3))
  {
    /* The return value may overflow, refuse */
    *buf_size = (ssize_t) -1;
    return false;
  }
  s1 = str;
  s2 = buf;
  token_removed = false;

  while ((size_t) (s1 - str) < str_len)
  {
    const char *cur_token; /**< the first char of current token */
    size_t copy_size;

    /* Skip any initial whitespaces and empty tokens */
    while ( ((size_t) (s1 - str) < str_len) &&
            ((' ' == *s1) || ('\t' == *s1) || (',' == *s1)) )
      s1++;

    /* 's1' points to the first char of token in the input string or
     * points just beyond the end of the input string */

    if ((size_t) (s1 - str) >= str_len)
      break; /* Nothing to copy, end of the input string */

    /* 's1' points to the first char of token in the input string */

    cur_token = s1; /* the first char of input token */

    /* Check the token with case-insensetive match */
    t_pos = 0;
    while ( ((size_t) (s1 - str) < str_len) && (token_len > t_pos) &&
            (charsequalcaseless (*s1, token[t_pos])) )
    {
      s1++;
      t_pos++;
    }
    /* s1 may point just beyond the end of the input string */
    if ( (token_len == t_pos) && (0 != token_len) )
    {
      /* 'token' matched, check that current input token does not have
       * any suffixes */
      while ( ((size_t) (s1 - str) < str_len) &&
              ((' ' == *s1) || ('\t' == *s1)) )
        s1++;
      /* 's1' points to the first non-whitespace char after the token matched
       * requested token or points just beyond the end of the input string after
       * the requested token */
      if (((size_t) (s1 - str) == str_len) || (',' == *s1))
      {/* full token match, do not copy current token to the output */
        token_removed = true;
        continue;
      }
    }

    /* 's1' points to first non-whitespace char, to some char after
     * first non-whitespace char in the token in the input string, to
     * the ',', or just beyond the end of the input string */
    /* The current token in the input string does not match the token
     * to exclude, it must be copied to the output string */
    /* the current token size excluding leading whitespaces and current char */
    copy_size = (size_t) (s1 - cur_token);
    if (buf == s2)
    { /* The first token to copy to the output */
      if ((size_t) *buf_size < copy_size)
      { /* Not enough space in the output buffer */
        *buf_size = (ssize_t) -1;
        return false;
      }
    }
    else
    { /* Some token was already copied to the output buffer */
      mhd_assert (s2 > buf);
      if ((size_t) *buf_size < ((size_t) (s2 - buf)) + copy_size + 2)
      { /* Not enough space in the output buffer */
        *buf_size = (ssize_t) -1;
        return false;
      }
      *(s2++) = ',';
      *(s2++) = ' ';
    }
    /* Copy non-matched token to the output */
    if (0 != copy_size)
    {
      memcpy (s2, cur_token, copy_size);
      s2 += copy_size;
    }

    while ( ((size_t) (s1 - str) < str_len) && (',' != *s1))
    {
      /* 's1' points to first non-whitespace char, to some char after
       * first non-whitespace char in the token in the input string */
      /* Copy all non-whitespace chars from the current token in
       * the input string */
      while ( ((size_t) (s1 - str) < str_len) &&
              (',' != *s1) && (' ' != *s1) && ('\t' != *s1) )
      {
        mhd_assert (s2 >= buf);
        if ((size_t) *buf_size <= (size_t) (s2 - buf)) /* '<= s2' equals '< s2 + 1' */
        { /* Not enough space in the output buffer */
          *buf_size = (ssize_t) -1;
          return false;
        }
        *(s2++) = *(s1++);
      }
      /* 's1' points to some whitespace char in the token in the input
       * string, to the ',', or just beyond the end of the input string */
      /* Skip all whitespaces */
      while ( ((size_t) (s1 - str) < str_len) &&
              ((' ' == *s1) || ('\t' == *s1)) )
        s1++;

      /* 's1' points to the first non-whitespace char in the input string
       * after whitespace chars, to the ',', or just beyond the end of
       * the input string */
      if (((size_t) (s1 - str) < str_len) && (',' != *s1))
      { /* Not the end of the current token */
        mhd_assert (s2 >= buf);
        if ((size_t) *buf_size <= (size_t) (s2 - buf)) /* '<= s2' equals '< s2 + 1' */
        { /* Not enough space in the output buffer */
          *buf_size = (ssize_t) -1;
          return false;
        }
        *(s2++) = ' ';
      }
    }
  }
  mhd_assert (((ssize_t) (s2 - buf)) <= *buf_size);
  *buf_size = (ssize_t) (s2 - buf);
  return token_removed;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1) MHD_FN_PAR_INOUT_ (2)
MHD_FN_PAR_IN_SIZE_ (3,4) bool
mhd_str_remove_tokens_caseless (char *restrict str,
                                size_t *restrict str_len,
                                const char *const restrict tkns,
                                const size_t tkns_len)
{
  size_t pt;                      /**< position in @a tokens */
  bool token_removed;

  mhd_assert (NULL == memchr (tkns, 0, tkns_len));

  token_removed = false;
  pt = 0;

  while (pt < tkns_len && *str_len != 0)
  {
    const char *tkn; /**< the current token */
    size_t tkn_len;

    /* Skip any initial whitespaces and empty tokens in 'tokens' */
    while ( (pt < tkns_len) &&
            ((' ' == tkns[pt]) || ('\t' == tkns[pt]) || (',' == tkns[pt])) )
      pt++;

    if (pt >= tkns_len)
      break; /* No more tokens, nothing to remove */

    /* Found non-whitespace char which is not a comma */
    tkn = tkns + pt;
    do
    {
      do
      {
        pt++;
      } while (pt < tkns_len &&
               (' ' != tkns[pt] && '\t' != tkns[pt] && ',' != tkns[pt]));
      /* Found end of the token string, space, tab, or comma */
      tkn_len = pt - (size_t) (tkn - tkns);

      /* Skip all spaces and tabs */
      while (pt < tkns_len && (' ' == tkns[pt] || '\t' == tkns[pt]))
        pt++;
      /* Found end of the token string or non-whitespace char */
    } while (pt < tkns_len && ',' != tkns[pt]);

    /* 'tkn' is the input token with 'tkn_len' chars */
    mhd_assert (0 != tkn_len);

    if (*str_len == tkn_len)
    {
      if (mhd_str_equal_caseless_bin_n (str, tkn, tkn_len))
      {
        *str_len = 0;
        token_removed = true;
      }
      continue;
    }
    /* 'tkn' cannot match part of 'str' if length of 'tkn' is larger
     * than length of 'str'.
     * It's know that 'tkn' is not equal to the 'str' (was checked previously).
     * As 'str' is normalized when 'tkn' is not equal to the 'str'
     * it is required that 'str' to be at least 3 chars larger then 'tkn'
     * (the comma, the space and at least one additional character for the next
     * token) to remove 'tkn' from the 'str'. */
    if (*str_len > tkn_len + 2)
    { /* Remove 'tkn' from the input string */
      size_t pr;    /**< the 'read' position in the @a str */
      size_t pw;    /**< the 'write' position in the @a str */

      pr = 0;
      pw = 0;

      do
      {
        mhd_assert (pr >= pw);
        mhd_assert ((*str_len) >= (pr + tkn_len));
        if ( ( ((*str_len) == (pr + tkn_len)) || (',' == str[pr + tkn_len]) ) &&
             mhd_str_equal_caseless_bin_n (str + pr, tkn, tkn_len) )
        {
          /* current token in the input string matches the 'tkn', skip it */
          mhd_assert ((*str_len == pr + tkn_len) || \
                      (' ' == str[pr + tkn_len + 1])); /* 'str' must be normalized */
          token_removed = true;
          /* Advance to the next token in the input string or beyond
           * the end of the input string. */
          pr += tkn_len + 2;
        }
        else
        {
          /* current token in the input string does not match the 'tkn',
           * copy to the output */
          if (0 != pw)
          { /* not the first output token, add ", " to separate */
            if (pr != pw + 2)
            {
              str[pw++] = ',';
              str[pw++] = ' ';
            }
            else
              pw += 2; /* 'str' is not yet modified in this round */
          }
          do
          {
            if (pr != pw)
              str[pw] = str[pr];
            pr++;
            pw++;
          } while (pr < *str_len && ',' != str[pr]);
          /* Advance to the next token in the input string or beyond
           * the end of the input string. */
          pr += 2;
        }
        /* 'pr' should point to the next token in the input string or beyond
         * the end of the input string */
        if ((*str_len) < (pr + tkn_len))
        { /* The rest of the 'str + pr' is too small to match 'tkn' */
          if ((*str_len) > pr)
          { /* Copy the rest of the string */
            size_t copy_size;
            copy_size = *str_len - pr;
            if (0 != pw)
            { /* not the first output token, add ", " to separate */
              if (pr != pw + 2)
              {
                str[pw++] = ',';
                str[pw++] = ' ';
              }
              else
                pw += 2; /* 'str' is not yet modified in this round */
            }
            if (pr != pw)
              memmove (str + pw, str + pr, copy_size);
            pw += copy_size;
          }
          *str_len = pw;
          break;
        }
        mhd_assert ((' ' != str[0]) && ('\t' != str[0]));
        mhd_assert ((0 == pr) || (3 <= pr));
        mhd_assert ((0 == pr) || (' ' == str[pr - 1]));
        mhd_assert ((0 == pr) || (',' == str[pr - 2]));
      } while (1);
    }
  }

  return token_removed;
}


#ifndef MHD_FAVOR_SMALL_CODE
/* Use individual function for each case */

MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_CSTR_ (1) MHD_FN_PAR_IN_ (1)
MHD_FN_PAR_OUT_ (2) size_t
mhd_str_to_uint64 (const char *restrict str,
                   uint_fast64_t *restrict out_val)
{
  const char *const start = str;
  uint_fast64_t res;

  if (! isasciidigit (str[0]))
    return 0;

  res = 0;
  do
  {
    const int digit = (unsigned char) (*str) - '0';
    uint_fast64_t prev_res = res;

    res *= 10;
    if (res / 10 != prev_res)
      return 0;
    res += (unsigned int) digit;
    if (res < (unsigned int) digit)
      return 0;

    str++;
  } while (isasciidigit (*str));

  *out_val = res;
  return (size_t) (str - start);
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (1,2)
MHD_FN_PAR_OUT_ (3) size_t
mhd_str_to_uint64_n (const char *restrict str,
                     size_t maxlen,
                     uint_fast64_t *restrict out_val)
{
  uint_fast64_t res;
  size_t i;

  if (! maxlen || ! isasciidigit (str[0]))
    return 0;

  res = 0;
  i = 0;
  do
  {
    const int digit = (unsigned char) str[i] - '0';
    uint_fast64_t prev_res = res;

    res *= 10;
    if (res / 10 != prev_res)
      return 0;
    res += (unsigned int) digit;
    if (res < (unsigned int) digit)
      return 0;
    i++;
  } while ( (i < maxlen) &&
            isasciidigit (str[i]) );

  *out_val = res;
  return i;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_CSTR_ (1)
MHD_FN_PAR_IN_ (1) MHD_FN_PAR_OUT_ (2) size_t
mhd_strx_to_uint32 (const char *restrict str,
                    uint_fast32_t *restrict out_val)
{
  const char *const start = str;
  uint_fast32_t res;
  int digit;

  res = 0;
  digit = xdigittovalue (*str);
  while (digit >= 0)
  {
    uint_fast32_t prev_res = res;

    res *= 16;
    if (res / 16 != prev_res)
      return 0;
    res += (unsigned int) digit;
    if (res < (unsigned int) digit)
      return 0;

    str++;
    digit = xdigittovalue (*str);
  }

  if (str - start > 0)
    *out_val = res;
  return (size_t) (str - start);
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (1,2)
MHD_FN_PAR_OUT_ (3) size_t
mhd_strx_to_uint32_n (const char *restrict str,
                      size_t maxlen,
                      uint_fast32_t *restrict out_val)
{
  size_t i;
  uint_fast32_t res;

  res = 0;
  i = 0;
  while (i < maxlen)
  {
    const int digit = xdigittovalue (str[i]);
    uint_fast32_t prev_res = res;

    if (0 > digit)
      break;

    res *= 16;
    if (res / 16 != prev_res)
      return 0;
    res += (uint_fast32_t) digit;
    if (res < (uint_fast32_t) digit)
      return 0;

    i++;
  }

  if (0 != i)
    *out_val = res;
  return i;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_CSTR_ (1)
MHD_FN_PAR_IN_ (1) MHD_FN_PAR_OUT_ (2) size_t
mhd_strx_to_uint64 (const char *restrict str,
                    uint_fast64_t *restrict out_val)
{
  const char *const start = str;
  uint_fast64_t res;
  int digit;

  res = 0;
  digit = xdigittovalue (*str);
  while (digit >= 0)
  {
    uint_fast64_t prev_res = res;

    res *= 16;
    if (res / 16 != prev_res)
      return 0;
    res += (unsigned int) digit;
    if (res < (unsigned int) digit)
      return 0;

    str++;
    digit = xdigittovalue (*str);
  }

  if (str - start > 0)
    *out_val = res;
  return (size_t) (str - start);
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (1,2)
MHD_FN_PAR_OUT_ (3) size_t
mhd_strx_to_uint64_n (const char *restrict str,
                      size_t maxlen,
                      uint_fast64_t *restrict out_val)
{
  size_t i;
  uint_fast64_t res;

  res = 0;
  i = 0;
  while (i < maxlen)
  {
    const int digit = xdigittovalue (str[i]);
    uint_fast64_t prev_res = res;

    if (0 > digit)
      break;

    res *= 16;
    if (res / 16 != prev_res)
      return 0;
    res += (unsigned int) digit;
    if (res < (unsigned int) digit)
      return 0;
    i++;
  }

  if (0 != i)
    *out_val = res;
  return i;
}


#else  /* MHD_FAVOR_SMALL_CODE */

MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (1,2) MHD_FN_PAR_OUT_SIZE_ (3,4) size_t
mhd_str_to_uvalue_n (const char *restrict str,
                     size_t maxlen,
                     void *restrict out_val,
                     size_t val_size,
                     uint_fast64_t max_val,
                     unsigned int base)
{
  size_t i;
  uint_fast64_t res;
  const uint_fast64_t max_v_div_b = max_val / base;
  const uint_fast64_t max_v_mod_b = max_val % base;

  if ((base != 16) && (base != 10))
    return 0;

  res = 0;
  i = 0;
  while (maxlen > i)
  {
    const int digit = (base == 16) ?
                      xdigittovalue (str[i]) : todigitvalue (str[i]);

    if (0 > digit)
      break;
    if ( ((max_v_div_b) < res) ||
         (( (max_v_div_b) == res) &&
          ( (max_v_mod_b) < (uint_fast64_t) digit) ) )
      return 0;

    res *= base;
    res += (unsigned int) digit;
    i++;
  }

  if (i)
  {
    if (8 == val_size)
      *(uint_fast64_t *) out_val = res;
    else if (4 == val_size)
      *(uint_fast32_t *) out_val = (uint_fast32_t) res;
    else
      return 0;
  }
  return i;
}


#endif /* MHD_FAVOR_SMALL_CODE */


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_SIZE_ (2,3) size_t
mhd_uint32_to_strx (uint_fast32_t val,
                    char *buf,
                    size_t buf_size)
{
  size_t o_pos = 0; /**< position of the output character */
  int digit_pos = 8; /** zero-based, digit position in @a 'val' */
  uint_least32_t val32 = ((uint_least32_t) val) & 0xFFFFFFFFu;
  unsigned int xdigit;

  /* Skip leading zeros */
  do
  {
    digit_pos--;
    xdigit = (unsigned int) (val32 >> 28);
    val32 <<= 4;
    val32 &= 0xFFFFFFFFu;
  } while ((0 == xdigit) && (0 != digit_pos));

  while (o_pos < buf_size)
  {
    buf[o_pos++] = valuetoxdigit (xdigit);
    if (0 == digit_pos)
      return o_pos;
    digit_pos--;
    xdigit = (unsigned int) (val32 >> 28);
    val32 <<= 4;
    val32 &= 0xFFFFFFFFu;
  }
  return 0; /* The buffer is too small */
}


#ifndef MHD_FAVOR_SMALL_CODE
MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_SIZE_ (2,3) size_t
mhd_uint16_to_str (uint_least16_t val,
                   char *buf,
                   size_t buf_size)
{
  char *chr;  /**< pointer to the current printed digit */
  /* The biggest printable number is 65535 */
  uint_least16_t divisor = UINT16_C (10000);
  int digit;

  val &= 0xFFFFu;
  chr = buf;
  digit = (int) (val / divisor);
  mhd_assert (digit < 10);

  /* Do not print leading zeros */
  while ((0 == digit) && (1 < divisor))
  {
    divisor /= 10;
    digit = (int) (val / divisor);
    mhd_assert (digit < 10);
  }

  while (0 != buf_size)
  {
    *chr = (char) ((char) digit + '0');
    chr++;
    buf_size--;
    if (1 == divisor)
      return (size_t) (chr - buf);
    val = (uint_least16_t) (val % divisor);
    divisor /= 10;
    digit = (int) (val / divisor);
    mhd_assert (digit < 10);
  }
  return 0; /* The buffer is too small */
}


#endif /* !MHD_FAVOR_SMALL_CODE */


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_SIZE_ (2,3) size_t
mhd_uint64_to_str (uint_fast64_t val,
                   char *buf,
                   size_t buf_size)
{
  char *chr;  /**< pointer to the current printed digit */
  /* The biggest printable number is 18446744073709551615 */
  uint_fast64_t divisor = (uint_fast64_t) 10000000000000000000U;
  int digit;

  val &= 0xFFFFFFFFFFFFFFFFu;
  chr = buf;
  digit = (int) (val / divisor);
  mhd_assert (digit < 10);

  /* Do not print leading zeros */
  while ((0 == digit) && (1 < divisor))
  {
    divisor /= 10;
    digit = (int) (val / divisor);
    mhd_assert (digit < 10);
  }

  while (0 != buf_size)
  {
    *chr = (char) ((char) digit + '0');
    chr++;
    buf_size--;
    if (1 == divisor)
      return (size_t) (chr - buf);
    val %= divisor;
    divisor /= 10;
    digit = (int) (val / divisor);
    mhd_assert (digit < 10);
  }
  return 0; /* The buffer is too small */
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_SIZE_ (3,4) size_t
mhd_uint8_to_str_pad (uint8_t val,
                      uint8_t min_digits,
                      char *buf,
                      size_t buf_size)
{
  size_t pos; /**< the position of the current printed digit */
  int digit;
  mhd_assert (3 >= min_digits);

  if (0u == buf_size)
    return 0u;

  pos = 0;
  digit = val / 100;
  if (0 == digit)
  {
    if (3 <= min_digits)
      buf[pos++] = '0';
  }
  else
  {
    buf[pos++] = (char) ('0' + (char) digit);
    val %= 100;
    min_digits = 2;
  }

  if (buf_size <= pos)
    return 0;
  digit = val / 10;
  if (0 == digit)
  {
    if (2 <= min_digits)
      buf[pos++] = '0';
  }
  else
  {
    buf[pos++] = (char) ('0' + (char) digit);
    val %= 10;
  }

  if (buf_size <= pos)
    return 0;
  buf[pos++] = (char) ('0' + (char) val);
  return pos;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (1, 2) MHD_FN_PAR_OUT_ (3) size_t
mhd_bin_to_hex (const void *restrict bin,
                size_t size,
                char *restrict hex)
{
  size_t i;

  for (i = 0; i < size; ++i)
  {
    const uint8_t b = ((const uint8_t *) bin)[i];
#ifdef mhd_HAVE_UINT8TOTWOXDIGITS
    const char *two_xdigits = uint8totwoxdigits (b);
    hex[i * 2] = two_xdigits[0];
    hex[i * 2 + 1] = two_xdigits[1];
#else  /* ! mhd_HAVE_UINT8TOTWOXDIGITS */
    hex[i * 2] = valuetoxdigit (b >> 4);
    hex[i * 2 + 1] = valuetoxdigit (b & 0x0Fu);
#endif /* ! mhd_HAVE_UINT8TOTWOXDIGITS */
  }
  return i * 2;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (1, 2) MHD_FN_PAR_OUT_ (3) size_t
mhd_bin_to_hex_z (const void *restrict bin,
                  size_t size,
                  char *restrict hex)
{
  size_t res;

  res = mhd_bin_to_hex (bin, size, hex);
  hex[res] = 0;

  return res;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (1,2) MHD_FN_PAR_OUT_ (3) size_t
mhd_hex_to_bin (const char *restrict hex,
                size_t len,
                void *restrict bin)
{
  size_t r;
  size_t w;

  r = 0;
  w = 0;
  if (0 != len % 2)
  {
    /* Assume the first byte is encoded with single digit */
    const char c2 = hex[r++];
    const int l = xdigittovalue (c2);
    if (0 > l)
      return 0;
    ((uint8_t *) bin)[w++] = (uint8_t) ((unsigned int) l);
  }
  while (r < len)
  {
    const char c1 = hex[r++];
    const char c2 = hex[r++];
    const int h = xdigittovalue (c1);
    const int l = xdigittovalue (c2);
    if ((0 > h) || (0 > l))
      return 0;
    ((uint8_t *) bin)[w++] =
      (uint8_t) ( ((uint8_t) (((uint8_t) ((unsigned int) h)) << 4))
                  | ((uint8_t) ((unsigned int) l)) );
  }
  mhd_assert (len == r);
  mhd_assert ((len + 1) / 2 == w);
  return w;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (1,2) MHD_FN_PAR_OUT_SIZE_ (3,4) size_t
mhd_str_pct_decode_strict_n (const char *pct_encoded,
                             size_t pct_encoded_len,
                             char *decoded,
                             size_t buf_size)
{
#ifdef MHD_FAVOR_SMALL_CODE
  bool broken;
  size_t res;

  res = mhd_str_pct_decode_lenient_n (pct_encoded, pct_encoded_len, decoded,
                                      buf_size, &broken);
  if (broken)
    return 0;
  return res;
#else  /* ! MHD_FAVOR_SMALL_CODE */
  size_t r;
  size_t w;
  r = 0;
  w = 0;

  if (buf_size >= pct_encoded_len)
  {
    while (r < pct_encoded_len)
    {
      const char chr = pct_encoded[r];
      if ('%' == chr)
      {
        if (2 >= pct_encoded_len - r)
          return 0;
        else
        {
          const char c1 = pct_encoded[++r];
          const char c2 = pct_encoded[++r];
          const int h = xdigittovalue (c1);
          const int l = xdigittovalue (c2);
          unsigned char out;
          if ((0 > h) || (0 > l))
            return 0;
          out =
            (unsigned char) (((uint8_t) (((uint8_t) ((unsigned int) h)) << 4))
                             | ((uint8_t) ((unsigned int) l)));
          decoded[w] = (char) out;
        }
      }
      else
        decoded[w] = chr;
      ++r;
      ++w;
    }
    return w;
  }

  while (r < pct_encoded_len)
  {
    const char chr = pct_encoded[r];
    if (w >= buf_size)
      return 0;
    if ('%' == chr)
    {
      if (2 >= pct_encoded_len - r)
        return 0;
      else
      {
        const char c1 = pct_encoded[++r];
        const char c2 = pct_encoded[++r];
        const int h = xdigittovalue (c1);
        const int l = xdigittovalue (c2);
        unsigned char out;
        if ((0 > h) || (0 > l))
          return 0;
        out =
          (unsigned char) (((uint8_t) (((uint8_t) ((unsigned int) h)) << 4))
                           | ((uint8_t) ((unsigned int) l)));
        decoded[w] = (char) out;
      }
    }
    else
      decoded[w] = chr;
    ++r;
    ++w;
  }
  return w;
#endif /* ! MHD_FAVOR_SMALL_CODE */
}


MHD_INTERNAL
MHD_FN_PAR_NONNULL_ (1) MHD_FN_PAR_NONNULL_ (3)
MHD_FN_PAR_IN_SIZE_ (1,2) MHD_FN_PAR_OUT_SIZE_ (3,4) size_t
mhd_str_pct_decode_lenient_n (const char *pct_encoded,
                              size_t pct_encoded_len,
                              char *decoded,
                              size_t buf_size,
                              bool *restrict broken_encoding)
{
  size_t r;
  size_t w;
  r = 0;
  w = 0;
  if (NULL != broken_encoding)
    *broken_encoding = false;
#ifndef MHD_FAVOR_SMALL_CODE
  if (buf_size >= pct_encoded_len)
  {
    while (r < pct_encoded_len)
    {
      const char chr = pct_encoded[r];
      if ('%' == chr)
      {
        if (2 >= pct_encoded_len - r)
        {
          if (NULL != broken_encoding)
            *broken_encoding = true;
          decoded[w] = chr; /* Copy "as is" */
        }
        else
        {
          const char c1 = pct_encoded[++r];
          const char c2 = pct_encoded[++r];
          const int h = xdigittovalue (c1);
          const int l = xdigittovalue (c2);
          unsigned char out;
          if ((0 > h) || (0 > l))
          {
            r -= 2;
            if (NULL != broken_encoding)
              *broken_encoding = true;
            decoded[w] = chr; /* Copy "as is" */
          }
          else
          {
            out =
              (unsigned char)
              (((uint8_t) (((uint8_t) ((unsigned int) h)) << 4))
               | ((uint8_t) ((unsigned int) l)));
            decoded[w] = (char) out;
          }
        }
      }
      else
        decoded[w] = chr;
      ++r;
      ++w;
    }
    return w;
  }
#endif /* ! MHD_FAVOR_SMALL_CODE */
  while (r < pct_encoded_len)
  {
    const char chr = pct_encoded[r];
    if (w >= buf_size)
      return 0;
    if ('%' == chr)
    {
      if (2 >= pct_encoded_len - r)
      {
        if (NULL != broken_encoding)
          *broken_encoding = true;
        decoded[w] = chr; /* Copy "as is" */
      }
      else
      {
        const char c1 = pct_encoded[++r];
        const char c2 = pct_encoded[++r];
        const int h = xdigittovalue (c1);
        const int l = xdigittovalue (c2);
        if ((0 > h) || (0 > l))
        {
          r -= 2;
          if (NULL != broken_encoding)
            *broken_encoding = true;
          decoded[w] = chr; /* Copy "as is" */
        }
        else
        {
          unsigned char out;
          out =
            (unsigned char)
            (((uint8_t) (((uint8_t) ((unsigned int) h)) << 4))
             | ((uint8_t) ((unsigned int) l)));
          decoded[w] = (char) out;
        }
      }
    }
    else
      decoded[w] = chr;
    ++r;
    ++w;
  }
  return w;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_CSTR_ (1) size_t
mhd_str_pct_decode_in_place_strict (char *str)
{
#ifdef MHD_FAVOR_SMALL_CODE
  size_t res;
  bool broken;

  res = mhd_str_pct_decode_in_place_lenient (str, &broken);
  if (broken)
  {
    res = 0;
    str[0] = 0;
  }
  return res;
#else  /* ! MHD_FAVOR_SMALL_CODE */
  size_t r;
  size_t w;
  r = 0;
  w = 0;

  while (0 != str[r])
  {
    const char chr = str[r++];
    if ('%' == chr)
    {
      const char d1 = str[r++];
      if (0 == d1)
        return 0;
      else
      {
        const char d2 = str[r++];
        if (0 == d2)
          return 0;
        else
        {
          const int h = xdigittovalue (d1);
          const int l = xdigittovalue (d2);
          unsigned char out;
          if ((0 > h) || (0 > l))
            return 0;
          out =
            (unsigned char)
            (((uint8_t) (((uint8_t) ((unsigned int) h)) << 4))
             | ((uint8_t) ((unsigned int) l)));
          str[w++] = (char) out;
        }
      }
    }
    else
      str[w++] = chr;
  }
  str[w] = 0;
  return w;
#endif /* ! MHD_FAVOR_SMALL_CODE */
}


MHD_INTERNAL
MHD_FN_PAR_NONNULL_ (1) MHD_FN_PAR_CSTR_ (1) size_t
mhd_str_pct_decode_in_place_lenient (char *restrict str,
                                     bool *restrict broken_encoding)
{
#ifdef MHD_FAVOR_SMALL_CODE
  size_t len;
  size_t res;

  len = strlen (str);
  res = mhd_str_pct_decode_lenient_n (str, len, str, len, broken_encoding);
  str[res] = 0;

  return res;
#else  /* ! MHD_FAVOR_SMALL_CODE */
  size_t r;
  size_t w;
  if (NULL != broken_encoding)
    *broken_encoding = false;
  r = 0;
  w = 0;
  while (0 != str[r])
  {
    const char chr = str[r++];
    if ('%' == chr)
    {
      const char d1 = str[r++];
      if (0 == d1)
      {
        if (NULL != broken_encoding)
          *broken_encoding = true;
        str[w++] = chr; /* Copy "as is" */
        str[w] = 0;
        return w;
      }
      else
      {
        const char d2 = str[r++];
        if (0 == d2)
        {
          if (NULL != broken_encoding)
            *broken_encoding = true;
          str[w++] = chr; /* Copy "as is" */
          str[w++] = d1; /* Copy "as is" */
          str[w] = 0;
          return w;
        }
        else
        {
          const int h = xdigittovalue (d1);
          const int l = xdigittovalue (d2);
          unsigned char out;
          if ((0 > h) || (0 > l))
          {
            if (NULL != broken_encoding)
              *broken_encoding = true;
            str[w++] = chr; /* Copy "as is" */
            str[w++] = d1;
            str[w++] = d2;
            continue;
          }
          out =
            (unsigned char)
            (((uint8_t) (((uint8_t) ((unsigned int) h)) << 4))
             | ((uint8_t) ((unsigned int) l)));
          str[w++] = (char) out;
          continue;
        }
      }
    }
    str[w++] = chr;
  }
  str[w] = 0;
  return w;
#endif /* ! MHD_FAVOR_SMALL_CODE */
}


mhd_static_inline MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (2, 1) bool
pct_decode_no_slash (const size_t str_len,
                     const char *restrict str,
                     const size_t chr_pos,
                     char *restrict chr)
{
  mhd_assert ('%' == *chr);
  mhd_assert (*chr == str[chr_pos]);
  mhd_ASSUME (str_len > chr_pos);

  if ((str_len - chr_pos) <= 2u) /* Overflow-safe check */
    return false; /* The string tail has less than two chars */
  else
  {
    const char d1 = str[chr_pos + 1u];
    const char d2 = str[chr_pos + 2u];
    const int h = xdigittovalue (d1);
    const int l = xdigittovalue (d2);

    if ((0 <= h) && (0 <= l))
    {
      char dec;
      mhd_ASSUME (15 >= h);
      mhd_ASSUME (15 >= l);
      dec = (char) ((((unsigned char) h) << 4u) | ((unsigned char) l));
      if ('/' != dec)
      {
        *chr = dec;
        return true;
      }
    }
  }
  /* No valid hex-number or a slash character (must not be encoded!) */
  return false;
}


MHD_INTERNAL
MHD_FN_PAR_NONNULL_ (2) MHD_FN_PAR_INOUT_SIZE_ (2,1) size_t
mhd_str_dec_norm_uri_path (size_t str_len,
                           char *restrict str)
{
  size_t r; /**< "read" position */
  size_t w; /**< "write" position */

  w = 0u;
  r = 0u;
  while (str_len > r)
  {
    /* Process all segments not started with "/" (if any) */
    char c;
    mhd_ASSUME (w <= r);
    c = str[r];
    if ('/' == c)
      break; /* Processed after this loop */
    if (('%' == c) &&
        pct_decode_no_slash (str_len,
                             str,
                             r,
                             &c))
      r += 2u;
    if ('.' == c)
    {
      char c2;
      if (str_len == r + 1u) /* overflow-safe as 'str_len > r' */
      {
        /* The complete string is "." */
        ++r;    /* Skip "." */
        break;  /* At the edge, stop */
      }
      mhd_ASSUME (w <= r);
      c2 = str[r + 1u];
      if ('/' == c2)
      {
        /* Found "./" at the start of the string */
        r += 2u;  /* Skip "./" */
        continue;
      }
      if (('%' == c2) &&
          pct_decode_no_slash (str_len,
                               str,
                               r + 1u,
                               &c2))
        r += 2u;
      if ('.' == c2)
      {
        char c3;
        if (str_len == r + 2u) /* overflow-safe as 'str_len > r + 1 ' */
        {
          /* The complete string is ".." */
          r += 2u;  /* Skip ".." */
          break;    /* At the edge, stop */
        }
        mhd_ASSUME (w <= r);
        c3 = str[r + 2u];
        if ('/' == c3)
        {
          /* Found "../" at the start of the string */
          r += 3u;  /* Skip "../" */
          continue;
        }
        /* Do not write 'c3' as it has not been percent-decoded */
      }
      str[w++] = c;
      str[w++] = c2;
      r += 2u;
    }
    else
    {
      str[w++] = c;
      r += 1u;
    }
    break;
  }

  mhd_ASSUME (w <= r);
  /* Found first segment which is not "../" and is not "./" OR the end of the string */
  for ((void) r; str_len > r && '/' != str[r]; ++r)
  {
    char c;
    mhd_ASSUME (w <= r);
    c = str[r];
    if (('%' == c) &&
        pct_decode_no_slash (str_len,
                             str,
                             r,
                             &c))
      r += 2u;
    mhd_ASSUME (w <= r);
    str[w++] = c;
  }

  /* Found first '/' which is not skipped OR the end of the string */
  while (str_len > r)
  {
    /* Start of a "/segment" */
    char slash_chr = str[r];
    const size_t seg_start = w;
    mhd_ASSUME ('/' == slash_chr);
    str[w++] = slash_chr;
    ++r;
    if (str_len > r)
    {
      char c;
      mhd_ASSUME (w <= r);
      c = str[r];
      if ('/' == c)
        continue;
      if (('%' == c) &&
          pct_decode_no_slash (str_len,
                               str,
                               r,
                               &c))
        r += 2u;
      if ('.' == c)
      {
        char c2;
        if (str_len == r + 1u) /* overflow-safe as 'str_len > r' */
        {
          /* Found "/." at the end of the string */
          ++r;   /* Skip ".", leave bare '/' */
          break; /* At the edge, stop */
        }
        mhd_ASSUME (w <= r);
        c2 = str[r + 1u];
        if ('/' == c2)
        {
          /* Found "/./" */
          w = seg_start; /* Rewind output to the '/' at the start of the segment */
          ++r;      /* Skip "." */
          continue; /* Go to the next "/", which will be written again */
        }
        if (('%' == c2) &&
            pct_decode_no_slash (str_len,
                                 str,
                                 r + 1u,
                                 &c2))
          r += 2u;
        if ('.' == c2)
        {
          char c3;
          if (str_len == r + 2u) /* overflow-safe as 'str_len > r + 1 ' */
          {
            /* Found "/.." at the end of the string */
            w = seg_start;
            if (0 < w)
              do
              { /* Rewind output to the start of prev segment */
                --w;
              } while (0 < w && '/' != str[w]);
            mhd_ASSUME (w < r);
            str[w++] = '/'; /* Replace prev segment with '/' */
            r += 2u; /* Skip ".." */
            break;   /* At the edge, stop */
          }
          mhd_ASSUME (w <= r);
          c3 = str[r + 2u];
          if ('/' == c3)
          {
            /* Found "/../" */
            w = seg_start;
            if (0 < w)
              do
              { /* Rewind output to the start of prev segment */
                --w;
              } while (0 < w && '/' != str[w]);
            r += 2u;  /* Skip ".."; put next '/' to the start of prev segment */
            continue;
          }
          /* Do not write 'c3' as it has not been percent-decoded */
        }
        str[w++] = c;
        str[w++] = c2;
        r += 2u;
      }
      else
      {
        str[w++] = c;
        r += 1u;
      }
      mhd_assert (seg_start < w);
    }
    for ((void) r; str_len > r && '/' != str[r]; ++r)
    {
      /* Process the end of the segment */
      char c;
      mhd_ASSUME (w <= r);
      c = str[r];
      if (('%' == c) &&
          pct_decode_no_slash (str_len,
                               str,
                               r,
                               &c))
        r += 2u;
      mhd_ASSUME (w <= r);
      str[w++] = c;
    }
    mhd_assert (0u != w);
  }
  mhd_assert (r == str_len);

  if (str_len > w)
    str[w] = '\0';

  return w;
}


#ifdef MHD_SUPPORT_AUTH_DIGEST

MHD_INTERNAL MHD_FN_PURE_ MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (1,2) MHD_FN_PAR_IN_SIZE_ (3,4) bool
mhd_str_equal_quoted_bin_n (const char *quoted,
                            size_t quoted_len,
                            const char *unquoted,
                            size_t unquoted_len)
{
  size_t i;
  size_t j;
  if (unquoted_len < quoted_len / 2)
    return false;

  j = 0;
  for (i = 0; quoted_len > i && unquoted_len > j; ++i, ++j)
  {
    if ('\\' == quoted[i])
    {
      i++; /* Advance to the next character */
      if (quoted_len == i)
        return false; /* No character after escaping backslash */
    }
    if (quoted[i] != unquoted[j])
      return false; /* Different characters */
  }
  if ((quoted_len != i) || (unquoted_len != j))
    return false; /* The strings have different length */

  return true;
}


MHD_INTERNAL MHD_FN_PURE_ MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (1,2) MHD_FN_PAR_IN_SIZE_ (3,4) bool
mhd_str_equal_caseless_quoted_bin_n (const char *quoted,
                                     size_t quoted_len,
                                     const char *unquoted,
                                     size_t unquoted_len)
{
  size_t i;
  size_t j;
  if (unquoted_len < quoted_len / 2)
    return false;

  j = 0;
  for (i = 0; quoted_len > i && unquoted_len > j; ++i, ++j)
  {
    if ('\\' == quoted[i])
    {
      i++; /* Advance to the next character */
      if (quoted_len == i)
        return false; /* No character after escaping backslash */
    }
    if (! charsequalcaseless (quoted[i], unquoted[j]))
      return false; /* Different characters */
  }
  if ((quoted_len != i) || (unquoted_len != j))
    return false; /* The strings have different length */

  return true;
}


#endif /* MHD_SUPPORT_AUTH_DIGEST */

#if defined(MHD_SUPPORT_AUTH_DIGEST) || defined(MHD_SUPPORT_POST_PARSER)

MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (1,2) MHD_FN_PAR_OUT_SIZE_ (3,2) size_t
mhd_str_unquote (const char *quoted,
                 size_t quoted_len,
                 char *result)
{
  size_t r;
  size_t w;

  r = 0;
  w = 0;

  while (quoted_len > r)
  {
    if ('\\' == quoted[r])
    {
      ++r;
      if (quoted_len == r)
        return 0; /* Last backslash is not followed by char to unescape */
    }
    result[w++] = quoted[r++];
  }
  return w;
}


#endif /* MHD_SUPPORT_AUTH_DIGEST || MHD_SUPPORT_POST_PARSER */

#if defined(MHD_SUPPORT_AUTH_DIGEST) || defined(MHD_SUPPORT_AUTH_BASIC)

MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (1,2)
MHD_FN_PAR_OUT_SIZE_ (3,4) size_t
mhd_str_quote (const char *unquoted,
               size_t unquoted_len,
               char *result,
               size_t buf_size)
{
  size_t r;
  size_t w;

  r = 0;
  w = 0;

#ifndef MHD_FAVOR_SMALL_CODE
  if (unquoted_len * 2 <= buf_size)
  {
    /* Fast loop: the output will fit the buffer with any input string content */
    while (unquoted_len > r)
    {
      const char chr = unquoted[r++];
      if (('\\' == chr) || ('\"' == chr))
        result[w++] = '\\'; /* Escape current char */
      result[w++] = chr;
    }
  }
  else
  {
    if (unquoted_len > buf_size)
      return 0; /* Quick fail: the output buffer is too small */
#else  /* MHD_FAVOR_SMALL_CODE */
  if (1)
  {
#endif /* MHD_FAVOR_SMALL_CODE */

    while (unquoted_len > r)
    {
      if (buf_size <= w)
        return 0; /* The output buffer is too small */
      else
      {
        const char chr = unquoted[r++];
        if (('\\' == chr) || ('\"' == chr))
        {
          result[w++] = '\\'; /* Escape current char */
          if (buf_size <= w)
            return 0; /* The output buffer is too small */
        }
        result[w++] = chr;
      }
    }
  }

  mhd_assert (w >= r);
  mhd_assert (w <= r * 2);
  return w;
}


#endif /* MHD_SUPPORT_AUTH_DIGEST || MHD_SUPPORT_AUTH_BASIC */

#ifdef MHD_SUPPORT_AUTH_BASIC

/*
 * MHD_BASE64_FUNC_VERSION
 * 1 = smallest,
 * 2 = medium,
 * 3 = fastest
 */
#ifndef MHD_BASE64_FUNC_VERSION
#ifdef MHD_FAVOR_SMALL_CODE
#define MHD_BASE64_FUNC_VERSION 1
#else  /* ! MHD_FAVOR_SMALL_CODE */
#define MHD_BASE64_FUNC_VERSION 3
#endif /* ! MHD_FAVOR_SMALL_CODE */
#endif /* ! MHD_BASE64_FUNC_VERSION */

#if MHD_BASE64_FUNC_VERSION < 1 || MHD_BASE64_FUNC_VERSION > 3
#error Wrong MHD_BASE64_FUNC_VERSION value
#endif /* MHD_BASE64_FUNC_VERSION < 1 || MHD_BASE64_FUNC_VERSION > 3 */

#if MHD_BASE64_FUNC_VERSION == 3
#define mhd_base64_map_type int
#else  /* MHD_BASE64_FUNC_VERSION < 3 */
#define mhd_base64_map_type int8_t
#endif /* MHD_BASE64_FUNC_VERSION < 3 */

#if MHD_BASE64_FUNC_VERSION == 1
static mhd_base64_map_type
base64_char_to_value_ (uint8_t c)
{
  if ('Z' >= c)
  {
    if ('A' <= c)
      return (mhd_base64_map_type) ((c - 'A') + 0);
    else if ('0' <= c)
    {
      if ('9' >= c)
        return (mhd_base64_map_type) ((c - '0') + 52);
      else if ('=' == c)
        return -2;
      else
        return -1;
    }
    else if ('+' == c)
      return 62;
    else if ('/' == c)
      return 63;
    else
      return -1;
  }
  else if (('z' >= c) && ('a' <= c))
    return (mhd_base64_map_type) ((c - 'a') + 26);
  return -1;
}


#endif /* MHD_BASE64_FUNC_VERSION == 1 */


mhd_DATA_TRUNCATION_RUNTIME_CHECK_DISABLE


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (1,2) MHD_FN_PAR_OUT_SIZE_ (3,4) size_t
mhd_base64_to_bin_n (const char *base64,
                     size_t base64_len,
                     void *bin,
                     size_t bin_size)
{
#if MHD_BASE64_FUNC_VERSION >= 2
  static const mhd_base64_map_type map[] = {
    /* -1 = invalid char, -2 = padding
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    NUL,  SOH,  STX,  ETX,  EOT,  ENQ,  ACK,  BEL,  */
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    /*
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    BS,   HT,   LF,   VT,   FF,   CR,   SO,   SI,   */
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    /*
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    DLE,  DC1,  DC2,  DC3,  DC4,  NAK,  SYN,  ETB,  */
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    /*
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    CAN,  EM,   SUB,  ESC,  FS,   GS,   RS,   US,   */
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    /*
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    ' ',  '!',  '"',  '#',  '$',  '%',  '&',  '\'', */
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    /*
    0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    '(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',  */
    -1,   -1,   -1,   62,   -1,   -1,   -1,   63,
    /*
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  */
    52,   53,   54,   55,   56,   57,   58,   59,
    /*
    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    '8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',  */
    60,   61,   -1,   -1,   -1,   -2,   -1,   -1,
    /*
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    '@',  'A',  'B',  'C',  'D',  'E',  'F',  'G',  */
    -1,    0,    1,    2,    3,    4,    5,    6,
    /*
    0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',  */
    7,     8,    9,   10,   11,   12,   13,   14,
    /*
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
    'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',  */
    15,   16,   17,   18,   19,   20,   21,   22,
    /*
     0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    'X',  'Y',  'Z',  '[',  '\',  ']',  '^',  '_',  */
    23,   24,   25,   -1,   -1,   -1,   -1,   -1,
    /*
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    '`',  'a',  'b',  'c',  'd',  'e',  'f',  'g',  */
    -1,   26,   27,   28,   29,   30,   31,   32,
    /*
    0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',  */
    33,   34,   35,   36,   37,   38,   39,   40,
    /*
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  */
    41,   42,   43,   44,   45,   46,   47,   48,
    /*
    0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    'x',  'y',  'z',  '{',  '|',  '}',  '~',  DEL,  */
    49,   50,   51,   -1,   -1,   -1,   -1,   -1

#if MHD_BASE64_FUNC_VERSION == 3
    ,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  /* 80..8F */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  /* 90..9F */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  /* A0..AF */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  /* B0..BF */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  /* C0..CF */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  /* D0..DF */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  /* E0..EF */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  /* F0..FF */
#endif /* ! MHD_BASE64_FUNC_VERSION == 3 */
  };
#define base64_char_to_value_(c) map[(c)]
#endif /* MHD_BASE64_FUNC_VERSION >= 2 */
  const uint8_t *const in = (const uint8_t *) base64;
  uint8_t *const out = (uint8_t *) bin;
  size_t i;
  size_t j;
  if (0 == base64_len)
    return 0;  /* Nothing to decode */
  if (0 != base64_len % 4)
    return 0;  /* Wrong input length */
  if (base64_len / 4 * 3 - 2 > bin_size)
    return 0;

  j = 0;
  for (i = 0; i < (base64_len - 4); i += 4)
  {
#if MHD_BASE64_FUNC_VERSION == 2
    if (0 != (0x80 & (in[i] | in[i + 1] | in[i + 2] | in[i + 3])))
      return 0;
#endif /* MHD_BASE64_FUNC_VERSION == 2 */
    if (1)
    {
      const mhd_base64_map_type v1 = base64_char_to_value_ (in[i + 0]);
      const mhd_base64_map_type v2 = base64_char_to_value_ (in[i + 1]);
      const mhd_base64_map_type v3 = base64_char_to_value_ (in[i + 2]);
      const mhd_base64_map_type v4 = base64_char_to_value_ (in[i + 3]);
      if ((0 > v1) || (0 > v2) || (0 > v3) || (0 > v4))
        return 0;
      out[j + 0] = (uint8_t) (((uint8_t) (((uint8_t) v1) << 2))
                              | ((uint8_t) (((uint8_t) v2) >> 4)));
      out[j + 1] = (uint8_t) (((uint8_t) (((uint8_t) v2) << 4))
                              | ((uint8_t) (((uint8_t) v3) >> 2)));
      out[j + 2] = (uint8_t) (((uint8_t) (((uint8_t) v3) << 6))
                              | ((uint8_t) v4));
    }
    j += 3;
  }
#if MHD_BASE64_FUNC_VERSION == 2
  if (0 != (0x80 & (in[i] | in[i + 1] | in[i + 2] | in[i + 3])))
    return 0;
#endif /* MHD_BASE64_FUNC_VERSION == 2 */
  if (1)
  { /* The last four chars block */
    const mhd_base64_map_type v1 = base64_char_to_value_ (in[i + 0]);
    const mhd_base64_map_type v2 = base64_char_to_value_ (in[i + 1]);
    const mhd_base64_map_type v3 = base64_char_to_value_ (in[i + 2]);
    const mhd_base64_map_type v4 = base64_char_to_value_ (in[i + 3]);
    if ((0 > v1) || (0 > v2))
      return 0; /* Invalid char or padding at first two positions */
    mhd_assert (j < bin_size);
    out[j++] = (uint8_t) (((uint8_t) (((uint8_t) v1) << 2))
                          | ((uint8_t) (((uint8_t) v2) >> 4)));
    if (0 > v3)
    { /* Third char is either padding or invalid */
      if ((-2 != v3) || (-2 != v4))
        return 0;  /* Both two last chars must be padding */
      if (0 != (uint8_t) (((uint8_t) v2) << 4))
        return 0;  /* Wrong last char */
      return j;
    }
    if (j >= bin_size)
      return 0; /* Not enough space */
    out[j++] = (uint8_t) (((uint8_t) (((uint8_t) v2) << 4))
                          | ((uint8_t) (((uint8_t) v3) >> 2)));
    if (0 > v4)
    { /* Fourth char is either padding or invalid */
      if (-2 != v4)
        return 0;  /* The char must be padding */
      if (0 != (uint8_t) (((uint8_t) v3) << 6))
        return 0;  /* Wrong last char */
      return j;
    }
    if (j >= bin_size)
      return 0; /* Not enough space */
    out[j++] = (uint8_t) (((uint8_t) (((uint8_t) v3) << 6))
                          | ((uint8_t) v4));
  }
  return j;
#if MHD_BASE64_FUNC_VERSION >= 2
#undef base64_char_to_value_
#endif /* MHD_BASE64_FUNC_VERSION >= 2 */
}


mhd_DATA_TRUNCATION_RUNTIME_CHECK_RESTORE


#undef mhd_base64_map_type

#endif /* MHD_SUPPORT_AUTH_BASIC */


MHD_INTERNAL MHD_FN_PURE_ MHD_FN_PAR_NONNULL_ALL_ bool
mhd_str_starts_with_token_opt_param (const struct MHD_String *restrict str,
                                     const struct MHD_String *restrict token)
{
  size_t i;

  mhd_assert (0 != token->len);
  mhd_assert (NULL == memchr (token->cstr, '=', token->len));
  mhd_assert (NULL == memchr (token->cstr, ' ', token->len));
  mhd_assert (NULL == memchr (token->cstr, '\t', token->len));

  if (str->len < token->len)
    return false; /* The string is too short to match */

  if (! mhd_str_equal_caseless_bin_n (str->cstr,
                                      token->cstr,
                                      token->len))
    return false; /* The string does not start with the token */

  for (i = token->len; i < str->len; ++i)
  {
    const char c = str->cstr[i];
    if ((' ' == c) || ('\t' == c))
      continue;
    if (';' == c)
      return true; /* Found the start of the token parameters */
    return false; /* The initial part of the string does not fully match the token */
  }
  mhd_assert (0 && "The string should not have whitespace at the end");
  return true;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_ (1) MHD_FN_PAR_IN_ (2) MHD_FN_PAR_IN_ (3)
MHD_FN_PAR_OUT_ (4) MHD_FN_PAR_OUT_ (5) enum mhd_StingStartsWithTokenResult
mhd_str_starts_with_token_req_param (
  const struct MHD_String *restrict str,
  const struct MHD_String *restrict token,
  const struct MHD_String *restrict par,
  struct mhd_BufferConst *restrict par_value,
  bool *restrict par_value_needs_unquote)
{
  size_t i;
  const char *const restrict cstr = str->cstr;
  bool token_found;
  bool param_found;

  mhd_assert (0 != token->len);
  mhd_assert (NULL == memchr (token->cstr, '=', token->len));
  mhd_assert (NULL == memchr (token->cstr, ' ', token->len));
  mhd_assert (NULL == memchr (token->cstr, '\t', token->len));
  mhd_assert (NULL == memchr (par->cstr, '=', par->len));
  mhd_assert (NULL == memchr (par->cstr, ' ', par->len));
  mhd_assert (NULL == memchr (par->cstr, '\t', par->len));

  par_value->data = NULL;
  par_value->size = 0;

  if (str->len < token->len)
    return mhd_STR_STARTS_W_TOKEN_NO_TOKEN; /* The string is too short to match */

  if (! mhd_str_equal_caseless_bin_n (cstr,
                                      token->cstr,
                                      token->len))
    return mhd_STR_STARTS_W_TOKEN_NO_TOKEN; /* The string does not start with the token */
  token_found = false;
  param_found = false;

  i = token->len;
  do
  {
    /* Find start of the next parameter */
    for ((void) 0; i < str->len; ++i)
    {
      const char c = cstr[i];
      if ((' ' == c) || ('\t' == c))
        continue;
      if (';' == c)
      {
        /* Found the start of the next token parameter */
        if (param_found)
          return mhd_STR_STARTS_W_TOKEN_HAS_TOKEN;
        ++i; /* Move to the next char */
        break;
      }
      if (',' == c)
        return mhd_STR_STARTS_W_TOKEN_HAS_TOKEN; /* Found the start of the next token */

      if (! token_found)
      {
        if (i == token->len)
        {
          /* The initial part of the string does not fully match the token or
             formatting is not correct */
          return mhd_STR_STARTS_W_TOKEN_NO_TOKEN;
        }
        /* The string has garbage after the token and whitespace */
        return mhd_STR_STARTS_W_TOKEN_HAS_TOKEN_BAD_FORMAT;
      }
      /* The garbage after the parameter */
      return mhd_STR_STARTS_W_TOKEN_HAS_TOKEN_BAD_FORMAT;
    }
    token_found = true;

    if (i == str->len)
      return mhd_STR_STARTS_W_TOKEN_HAS_TOKEN;

    /* 'i' is at the start of the parameter */

    while ((' ' == cstr[i]) || ('\t' == cstr[i]))
    {
      if (++i == str->len)
        return mhd_STR_STARTS_W_TOKEN_HAS_TOKEN;
    }

    /* 'i' is at the start of the parameter name */

    if (par->len > str->len - i - 1)
      return mhd_STR_STARTS_W_TOKEN_HAS_TOKEN; /* the token is found, but the parameter is not */
    else
    { /* Check the parameter */
      bool val_needs_unquote;
      size_t j;
      const char *const prm_str = cstr + i;

      for (j = 0; j < par->len; ++j)
        if (! charsequalcaseless (prm_str[j],
                                  par->cstr[j]))
          break;
      i += j;
      mhd_assert (str->len > i);
      if ((j == par->len) &&
          ('=' == cstr[i]))
      {
        /* The parameter name matches required parameter */
        param_found = true;
        par_value->data = cstr + i + 1;
      }
      else
      {
        /* i points to the char in the parameter name */
        while ('=' != cstr[i])
        {
          if ((';' == cstr[i])      /* end of the parameter */
              || (',' == cstr[i])   /* end of the token */
              || (str->len == ++i)) /* end of the field string */
          {
            /* parameter without the value */
            return mhd_STR_STARTS_W_TOKEN_HAS_TOKEN_BAD_FORMAT;
          }
        }
      }
      mhd_assert (str->len > i);
      mhd_assert ('=' == cstr[i]);

      /* 'i' points to '=' between parameter name and parameter value */

      ++i; /* Advance to the first char in the parameter value */
      if (str->len == i)
        return mhd_STR_STARTS_W_TOKEN_HAS_TOKEN; /* Zero-length parameter value */

      val_needs_unquote = false;

      /* 'i' points to the char after '=' */

      if ('"' == cstr[i])
      {
        /* The value is quoted */
        if (param_found)
          ++(par_value->data); /* Point to the first quoted char */
        do
        {
          ++i; /* Advance to the next char */
          if (str->len == i)
            return mhd_STR_STARTS_W_TOKEN_HAS_TOKEN_BAD_FORMAT; /* No closing quote */
          if ('\\' == cstr[i])
          {
            val_needs_unquote = true;
            ++i; /* Skip quoted char */
            if (str->len == i)
              return mhd_STR_STARTS_W_TOKEN_HAS_TOKEN_BAD_FORMAT; /* No closing quote */
          }
        } while ('"' != cstr[i]);
        if (param_found)
        {
          par_value->size = (size_t) ((cstr + i) - par_value->data);
          *par_value_needs_unquote = val_needs_unquote;
        }
        /* Complete value found */
        /* Check for the garbage data at the end */
        ++i; /* Advance to the next char */
      }
      else
      {
        /* The value is not quoted */
        while ((' ' != cstr[i]) &&
               ('\t' != cstr[i]))
        {
          if ((';' == cstr[i])      /* end of the parameter */
              || (',' == cstr[i])   /* end of the token */
              || (str->len == ++i)) /* end of the field string */
            break;
        }
        /* The end parameter value */
        if (param_found)
        {
          par_value->size = (size_t) ((cstr + i) - par_value->data);
          *par_value_needs_unquote = false;
        }
        /* Check for the garbage data at the end */
      }

      /* 'i' points to the next char after end of the parameter value */
    }
  } while (i < str->len);

  mhd_assert (token_found);
  return mhd_STR_STARTS_W_TOKEN_HAS_TOKEN;
}
