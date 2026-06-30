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
 * @file src/test/unit/unit_h2_huffman_coding.c
 * @brief  The tests for HTTP/2 Huffman coding function
 * @author Karlson2k (Evgeny Grin)
 */


#include "mhd_sys_options.h"

#include "h2/hpack/h2_huffman_codec.h"

#include <stdio.h>
#include <string.h>

#include "sys_base_types.h"
#include "mhd_str_types.h"
#include "mhd_str_macros.h"

#include "mhdt_has_in_name.h"
#include "mhdt_has_param.h"

#ifndef MHD_ENABLE_SLOW_TESTS
static int enable_deep_tests = 0;
#else
static int enable_deep_tests = ! 0;
#endif

struct mhdt_HEcondedData
{
  const size_t size;
  const uint8_t data[640];
};

struct mhdt_HTestData
{
  const struct MHD_String str;
  const struct mhdt_HEcondedData enc;
};

static const struct mhdt_HTestData h_data[] = {
  /*
   **  These test data entries are extracted from RFC 7541 examples **
   */
  {
    mhd_MSTR_INIT ("www.example.com"),
    {
      12u,
      {
        0xf1, 0xe3, 0xc2, 0xe5, 0xf2, 0x3a, 0x6b, 0xa0, 0xab, 0x90,
        0xf4, 0xff
      }
    }
  }
  ,
  {
    mhd_MSTR_INIT ("no-cache"),
    {
      6u,
      {
        0xa8, 0xeb, 0x10, 0x64, 0x9c, 0xbf
      }
    }
  }
  ,
  {
    mhd_MSTR_INIT ("custom-key"),
    {
      8u,
      {
        0x25, 0xa8, 0x49, 0xe9, 0x5b, 0xa9, 0x7d, 0x7f
      }
    }
  }
  ,
  {
    mhd_MSTR_INIT ("custom-value"),
    {
      9u,
      {
        0x25, 0xa8, 0x49, 0xe9, 0x5b, 0xb8, 0xe8, 0xb4, 0xbf
      }
    }
  }
  ,
  {
    mhd_MSTR_INIT ("302"),
    {
      2u,
      {
        0x64, 0x02
      }
    }
  }
  ,
  {
    mhd_MSTR_INIT ("private"),
    {
      5u,
      {
        0xae, 0xc3, 0x77, 0x1a, 0x4b
      }
    }
  }
  ,
  {
    mhd_MSTR_INIT ("Mon, 21 Oct 2013 20:13:21 GMT"),
    {
      22u,
      {
        0xd0, 0x7a, 0xbe, 0x94, 0x10, 0x54, 0xd4, 0x44, 0xa8, 0x20, 0x05,
        0x95, 0x04, 0x0b, 0x81, 0x66, 0xe0, 0x82, 0xa6, 0x2d, 0x1b, 0xff
      }
    }
  }
  ,
  {
    mhd_MSTR_INIT ("https://www.example.com"),
    {
      17u,
      {
        0x9d, 0x29, 0xad, 0x17, 0x18, 0x63, 0xc7, 0x8f, 0x0b, 0x97, 0xc8, 0xe9,
        0xae, 0x82, 0xae, 0x43, 0xd3
      }
    }
  }
  ,
  {
    mhd_MSTR_INIT ("307"),
    {
      3u,
      {
        0x64, 0x0e, 0xff
      }
    }
  }
  ,
  {
    mhd_MSTR_INIT ("gzip"),
    {
      3u,
      {
        0x9b, 0xd9, 0xab
      }
    }
  }
  ,
  {
    mhd_MSTR_INIT ("foo=ASDJKHQKBZXOQWEOPIUAXQWEOIU; max-age=3600; version=1"),
    {
      45u,
      {
        0x94, 0xe7, 0x82, 0x1d, 0xd7, 0xf2, 0xe6, 0xc7, 0xb3, 0x35, 0xdf, 0xdf,
        0xcd, 0x5b, 0x39, 0x60, 0xd5, 0xaf, 0x27, 0x08, 0x7f, 0x36, 0x72, 0xc1,
        0xab, 0x27, 0x0f, 0xb5, 0x29, 0x1f, 0x95, 0x87, 0x31, 0x60, 0x65, 0xc0,
        0x03, 0xed, 0x4e, 0xe5, 0xb1, 0x06, 0x3d, 0x50, 0x07
      }
    }
  }
  /*
   **  Hand-crafted test entries **
   */
  ,
  {
    mhd_MSTR_INIT ("Hello, Huffman coding!"),
    {
      17u,
      {
        0xC6, 0x5A, 0x28, 0x3F, 0xD2, 0x98, 0xED, 0x96, 0x5A, 0x47, 0x52, 0x84,
        0x3C, 0x86, 0xAA, 0x6F, 0xE3
      }
    }
  }
  ,
  {
    mhd_MSTR_INIT ("nginx"),
    {
      4u,
      {
        0xAA, 0x63, 0x55, 0xE7
      }
    }
  }
  ,
  {
    mhd_MSTR_INIT ("apache"),
    {
      4u,
      {
        0x1D, 0x63, 0x24, 0xE5
      }
    }
  }
  ,
  {
    mhd_MSTR_INIT (" "),
    {
      1u,
      {
        0x53
      }
    }
  }
  ,
  {
    mhd_MSTR_INIT ("0"),
    {
      1u,
      {
        0x07
      }
    }
  }
  ,
  {   /* Empty */
    mhd_MSTR_INIT (""),
    {
      0u,
      {
        0
      }
    }
  }
  ,
  {   /* Long codes */
    mhd_MSTR_INIT ("\x0A" "\x0D" "\x16" "\x0A" "\x0D" "\x16" "\x0A" \
                   "\x0D" "\x16" "\x0A"),
    {
      38u,
      {
        0xFF, 0xFF, 0xFF, 0xF3, 0xFF, 0xFF, 0xFF, 0xDF, 0xFF, 0xFF, 0xFF,
        0xBF, 0xFF, 0xFF, 0xFC, 0xFF, 0xFF, 0xFF, 0xF7, 0xFF, 0xFF, 0xFF,
        0xEF, 0xFF, 0xFF, 0xFF, 0x3F, 0xFF, 0xFF, 0xFD, 0xFF, 0xFF, 0xFF,
        0xFB, 0xFF, 0xFF, 0xFF, 0xCF
      }
    }
  }
  ,
  {   /* Shortest + longest codes */
    mhd_MSTR_INIT ("0" "\x0A" "1"),
    {
      5u,
      {
        0x07, 0xFF, 0xFF, 0xFF, 0x81
      }
    }
  }
  ,
  {   /* Shortest + longest codes */
    mhd_MSTR_INIT ("\x0D" "2a" "\x16" "ce" "\x0A"),
    {
      14u,
      {
        0xFF, 0xFF, 0xFF, 0xF4, 0x43, 0xFF, 0xFF, 0xFF, 0xF8, 0x85, 0xFF, 0xFF,
        0xFF, 0xF3
      }
    }
  }
  ,
  {   /* Shortest + longest codes */
    mhd_MSTR_INIT ("01" "\x0A" "2" "\x0D" "ac" "\x16" "ei" "\x0A" \
                   "os" "\x0D" "t"),
    {
      25u,
      {
        0x00, 0x7F, 0xFF, 0xFF, 0xFC, 0x17, 0xFF, 0xFF, 0xFF, 0xA3, 0x27, 0xFF,
        0xFF, 0xFF, 0xC5, 0x37, 0xFF, 0xFF, 0xFF, 0x87, 0x47, 0xFF, 0xFF, 0xFF,
        0xA9
      }
    }
  }
  ,
  {   /* Short + long codes */
    mhd_MSTR_INIT ("0 :&!'#" "\x00" "^<\\" "\x80" "\x99" "\x81" \
                   "\x01" "\x09" "\xC7" "\xC0" "\xCB" "\x02" "\x0A"),
    {
      46u,
      {
        0x02, 0x97, 0x3E, 0x3F, 0x8F, 0xF5, 0xFF, 0x5F, 0xF8, 0xFF, 0xF3, 0xFF,
        0xE7, 0xFF, 0xF0, 0xFF, 0xFE, 0x6F, 0xFF, 0xEE, 0x7F, 0xFF, 0xA5, 0xFF,
        0xFF, 0x63, 0xFF, 0xFF, 0xAB, 0xFF, 0xFF, 0xD9, 0xFF, 0xFF, 0xF0, 0x7F,
        0xFF, 0xFD, 0xEF, 0xFF, 0xFF, 0xE2, 0xFF, 0xFF, 0xFF, 0xF3
      }
    }
  }
  ,
  {   /* All printable chars */
    mhd_MSTR_INIT (" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJ" \
                   "KLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"),
    {
      93u,
      {
        0x53, 0xF8, 0xFE, 0x7F, 0xEB, 0xFF, 0x2A, 0xFC, 0x7F, 0xAF, 0xEB, 0xFB,
        0xF9, 0xFF, 0x7F, 0x4B, 0x2E, 0xC0, 0x02, 0x26, 0x5A, 0x6D, 0xC7, 0x5E,
        0x7E, 0xE7, 0xDF, 0xFF, 0xC8, 0x3F, 0xEF, 0xFC, 0xFF, 0xD4, 0x37, 0x6F,
        0x5F, 0xC1, 0x87, 0x16, 0x3C, 0x99, 0x73, 0x67, 0xD1, 0xA7, 0x56, 0xBD,
        0x9B, 0x77, 0x6F, 0xE1, 0xC7, 0x97, 0xE7, 0x3F, 0xDF, 0xFD, 0xFF, 0xFF,
        0x0F, 0xFE, 0x7F, 0xF9, 0x17, 0xFF, 0xD1, 0xC6, 0x49, 0x0B, 0x2C, 0xD3,
        0x9B, 0xA7, 0x5A, 0x29, 0xA8, 0xF5, 0xF6, 0xB1, 0x09, 0xB7, 0xBF, 0x8F,
        0x3E, 0xBD, 0xFF, 0xFE, 0xFF, 0x9F, 0xFE, 0xFF, 0xF7
      }
    }
  }
  ,
  {   /* All printable chars in reverse */
    mhd_MSTR_INIT ("~}|{zyxwvutsrqponmlkjihgfedcba`_^]\\[ZYXWVUT" \
                   "SRQPONMLKJIHGFEDCBA@?>=<;:9876543210/.-,+*)('&%$#\"! "),
    {
      93u,
      {
        0xFF, 0xEF, 0xFF, 0xBF, 0xF3, 0xFF, 0xF7, 0xBF, 0x5E, 0x7C, 0x77, 0xB5,
        0x28, 0xB3, 0xB5, 0x67, 0xAA, 0x9A, 0x3A, 0xF4, 0x34, 0xF3, 0x4A, 0x59,
        0x09, 0x18, 0xFF, 0xFE, 0xC5, 0xFF, 0xE7, 0xFF, 0x3F, 0xFF, 0x87, 0xFE,
        0xFF, 0x79, 0xFE, 0x72, 0xE3, 0xC3, 0x7E, 0xED, 0xBB, 0x35, 0xEA, 0xD3,
        0xA3, 0x3E, 0x6C, 0xB9, 0x31, 0xE2, 0xC3, 0x82, 0xFD, 0xEB, 0xB0, 0xFF,
        0xEB, 0xFC, 0xFF, 0xB8, 0x3F, 0xFE, 0x7D, 0xDC, 0x7D, 0xE7, 0x5C, 0x6D,
        0xA6, 0x44, 0x10, 0x30, 0xBA, 0xDF, 0x5F, 0xEF, 0xE7, 0xFB, 0xFE, 0xBF,
        0xD7, 0xC2, 0xBF, 0xF9, 0xFF, 0xAF, 0xE7, 0xF8, 0x53
      }
    }
  }
  ,
  {   /* All chars */
    mhd_MSTR_INIT ("\x00" "\x01" "\x02" "\x03" "\x04" "\x05" "\x06" "\x07" \
                   "\x08" "\x09" "\x0A" "\x0B" "\x0C" "\x0D" "\x0E" "\x0F" \
                   "\x10" "\x11" "\x12" "\x13" "\x14" "\x15" "\x16" "\x17" \
                   "\x18" "\x19" "\x1A" "\x1B" "\x1C" "\x1D" "\x1E" "\x1F" \
                   " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQR" \
                   "STUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" "\x7F" \
                   "\x80" "\x81" "\x82" "\x83" "\x84" "\x85" "\x86" "\x87" \
                   "\x88" "\x89" "\x8A" "\x8B" "\x8C" "\x8D" "\x8E" "\x8F" \
                   "\x90" "\x91" "\x92" "\x93" "\x94" "\x95" "\x96" "\x97" \
                   "\x98" "\x99" "\x9A" "\x9B" "\x9C" "\x9D" "\x9E" "\x9F" \
                   "\xA0" "\xA1" "\xA2" "\xA3" "\xA4" "\xA5" "\xA6" "\xA7" \
                   "\xA8" "\xA9" "\xAA" "\xAB" "\xAC" "\xAD" "\xAE" "\xAF" \
                   "\xB0" "\xB1" "\xB2" "\xB3" "\xB4" "\xB5" "\xB6" "\xB7" \
                   "\xB8" "\xB9" "\xBA" "\xBB" "\xBC" "\xBD" "\xBE" "\xBF" \
                   "\xC0" "\xC1" "\xC2" "\xC3" "\xC4" "\xC5" "\xC6" "\xC7" \
                   "\xC8" "\xC9" "\xCA" "\xCB" "\xCC" "\xCD" "\xCE" "\xCF" \
                   "\xD0" "\xD1" "\xD2" "\xD3" "\xD4" "\xD5" "\xD6" "\xD7" \
                   "\xD8" "\xD9" "\xDA" "\xDB" "\xDC" "\xDD" "\xDE" "\xDF" \
                   "\xE0" "\xE1" "\xE2" "\xE3" "\xE4" "\xE5" "\xE6" "\xE7" \
                   "\xE8" "\xE9" "\xEA" "\xEB" "\xEC" "\xED" "\xEE" "\xEF" \
                   "\xF0" "\xF1" "\xF2" "\xF3" "\xF4" "\xF5" "\xF6" "\xF7" \
                   "\xF8" "\xF9" "\xFA" "\xFB" "\xFC" "\xFD" "\xFE" "\xFF"),
    {
      583u,
      {
        0xFF, 0xC7, 0xFF, 0xFD, 0x8F, 0xFF, 0xFF, 0xE2, 0xFF, 0xFF, 0xFE, 0x3F,
        0xFF, 0xFF, 0xE4, 0xFF, 0xFF, 0xFE, 0x5F, 0xFF, 0xFF, 0xE6, 0xFF, 0xFF,
        0xFE, 0x7F, 0xFF, 0xFF, 0xE8, 0xFF, 0xFF, 0xEA, 0xFF, 0xFF, 0xFF, 0xF3,
        0xFF, 0xFF, 0xFA, 0x7F, 0xFF, 0xFF, 0xAB, 0xFF, 0xFF, 0xFF, 0xDF, 0xFF,
        0xFF, 0xEB, 0xFF, 0xFF, 0xFE, 0xCF, 0xFF, 0xFF, 0xED, 0xFF, 0xFF, 0xFE,
        0xEF, 0xFF, 0xFF, 0xEF, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0xF1, 0xFF,
        0xFF, 0xFF, 0x2F, 0xFF, 0xFF, 0xFF, 0xBF, 0xFF, 0xFF, 0xCF, 0xFF, 0xFF,
        0xFD, 0x3F, 0xFF, 0xFF, 0xD7, 0xFF, 0xFF, 0xFD, 0xBF, 0xFF, 0xFF, 0xDF,
        0xFF, 0xFF, 0xFE, 0x3F, 0xFF, 0xFF, 0xE7, 0xFF, 0xFF, 0xFE, 0xBF, 0xFF,
        0xFF, 0xED, 0x4F, 0xE3, 0xF9, 0xFF, 0xAF, 0xFC, 0xAB, 0xF1, 0xFE, 0xBF,
        0xAF, 0xEF, 0xE7, 0xFD, 0xFD, 0x2C, 0xBB, 0x00, 0x08, 0x99, 0x69, 0xB7,
        0x1D, 0x79, 0xFB, 0x9F, 0x7F, 0xFF, 0x20, 0xFF, 0xBF, 0xF3, 0xFF, 0x50,
        0xDD, 0xBD, 0x7F, 0x06, 0x1C, 0x58, 0xF2, 0x65, 0xCD, 0x9F, 0x46, 0x9D,
        0x5A, 0xF6, 0x6D, 0xDD, 0xBF, 0x87, 0x1E, 0x5F, 0x9C, 0xFF, 0x7F, 0xF7,
        0xFF, 0xFC, 0x3F, 0xF9, 0xFF, 0xE4, 0x5F, 0xFF, 0x47, 0x19, 0x24, 0x2C,
        0xB3, 0x4E, 0x6E, 0x9D, 0x68, 0xA6, 0xA3, 0xD7, 0xDA, 0xC4, 0x26, 0xDE,
        0xFE, 0x3C, 0xFA, 0xF7, 0xFF, 0xFB, 0xFE, 0x7F, 0xFB, 0xFF, 0xDF, 0xFF,
        0xFF, 0xFC, 0xFF, 0xFE, 0x6F, 0xFF, 0xF4, 0xBF, 0xFF, 0x9F, 0xFF, 0xFA,
        0x3F, 0xFF, 0xD3, 0xFF, 0xFF, 0x53, 0xFF, 0xFD, 0x5F, 0xFF, 0xFB, 0x3F,
        0xFF, 0xEB, 0x7F, 0xFF, 0xDA, 0xFF, 0xFF, 0xB7, 0xFF, 0xFF, 0x73, 0xFF,
        0xFE, 0xEF, 0xFF, 0xFD, 0xEF, 0xFF, 0xFE, 0xBF, 0xFF, 0xFB, 0xFF, 0xFF,
        0xFD, 0x9F, 0xFF, 0xFD, 0xBF, 0xFF, 0xEB, 0xFF, 0xFF, 0xE0, 0xFF, 0xFF,
        0xEE, 0xFF, 0xFF, 0xC3, 0xFF, 0xFF, 0x8B, 0xFF, 0xFF, 0x1F, 0xFF, 0xFE,
        0x4F, 0xFF, 0xEE, 0x7F, 0xFF, 0xB1, 0xFF, 0xFF, 0x97, 0xFF, 0xFD, 0x9F,
        0xFF, 0xFC, 0xDF, 0xFF, 0xF9, 0xFF, 0xFF, 0xFB, 0xFF, 0xFF, 0xDA, 0xFF,
        0xFE, 0xEF, 0xFF, 0xF4, 0xFF, 0xFF, 0xB7, 0xFF, 0xFE, 0xE7, 0xFF, 0xFE,
        0x8F, 0xFF, 0xFD, 0x3F, 0xFF, 0xDE, 0xFF, 0xFF, 0xD5, 0xFF, 0xFE, 0xEF,
        0xFF, 0xFB, 0xDF, 0xFF, 0xFE, 0x1F, 0xFF, 0xDF, 0xFF, 0xFF, 0x7F, 0xFF,
        0xFF, 0x5F, 0xFF, 0xFE, 0xCF, 0xFF, 0xF0, 0x7F, 0xFF, 0x87, 0xFF, 0xFE,
        0x0F, 0xFF, 0xF1, 0x7F, 0xFF, 0xED, 0xFF, 0xFF, 0x87, 0xFF, 0xFF, 0x77,
        0xFF, 0xFE, 0xFF, 0xFF, 0xEA, 0xFF, 0xFF, 0x8B, 0xFF, 0xFE, 0x3F, 0xFF,
        0xF9, 0x3F, 0xFF, 0xF8, 0x7F, 0xFF, 0xCB, 0xFF, 0xFF, 0x37, 0xFF, 0xFF,
        0x1F, 0xFF, 0xFF, 0x83, 0xFF, 0xFF, 0xE1, 0xFF, 0xFE, 0xBF, 0xFF, 0xE3,
        0xFF, 0xFF, 0x3F, 0xFF, 0xFF, 0x2F, 0xFF, 0xFA, 0x3F, 0xFF, 0xFD, 0x9F,
        0xFF, 0xFF, 0x17, 0xFF, 0xFF, 0xC7, 0xFF, 0xFF, 0xF2, 0x7F, 0xFF, 0xFD,
        0xEF, 0xFF, 0xFF, 0xBF, 0xFF, 0xFF, 0xF2, 0xFF, 0xFF, 0xF8, 0xFF, 0xFF,
        0xFB, 0x7F, 0xFF, 0x97, 0xFF, 0xF8, 0xFF, 0xFF, 0xFE, 0x6F, 0xFF, 0xFF,
        0xC1, 0xFF, 0xFF, 0xF8, 0x7F, 0xFF, 0xFE, 0x7F, 0xFF, 0xFF, 0xC5, 0xFF,
        0xFF, 0xE5, 0xFF, 0xFE, 0x4F, 0xFF, 0xF2, 0xFF, 0xFF, 0xFD, 0x1F, 0xFF,
        0xFF, 0x4F, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFE, 0x3F, 0xFF, 0xFF, 0xC9,
        0xFF, 0xFF, 0xF9, 0x7F, 0xFF, 0xB3, 0xFF, 0xFF, 0xCF, 0xFF, 0xFB, 0x7F,
        0xFF, 0xCD, 0xFF, 0xFF, 0x4F, 0xFF, 0xF9, 0xFF, 0xFF, 0xD1, 0xFF, 0xFF,
        0xCF, 0xFF, 0xFE, 0xAF, 0xFF, 0xFA, 0xFF, 0xFF, 0xFD, 0xDF, 0xFF, 0xFE,
        0xFF, 0xFF, 0xFF, 0x4F, 0xFF, 0xFF, 0x5F, 0xFF, 0xFF, 0xAB, 0xFF, 0xFF,
        0xA7, 0xFF, 0xFF, 0xD7, 0xFF, 0xFF, 0xF9, 0xBF, 0xFF, 0xFE, 0xCF, 0xFF,
        0xFF, 0xB7, 0xFF, 0xFF, 0xF3, 0xFF, 0xFF, 0xFE, 0x8F, 0xFF, 0xFF, 0xD3,
        0xFF, 0xFF, 0xFA, 0xBF, 0xFF, 0xFF, 0x5F, 0xFF, 0xFF, 0xFF, 0x7F, 0xFF,
        0xFE, 0xCF, 0xFF, 0xFF, 0xDB, 0xFF, 0xFF, 0xFB, 0xBF, 0xFF, 0xFF, 0x7F,
        0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFB, 0xBF
      }
    }
  }
  ,
  {   /* All chars in reverse */
    mhd_MSTR_INIT ("\xFF" "\xFE" "\xFD" "\xFC" "\xFB" "\xFA" "\xF9" "\xF8" \
                   "\xF7" "\xF6" "\xF5" "\xF4" "\xF3" "\xF2" "\xF1" "\xF0" \
                   "\xEF" "\xEE" "\xED" "\xEC" "\xEB" "\xEA" "\xE9" "\xE8" \
                   "\xE7" "\xE6" "\xE5" "\xE4" "\xE3" "\xE2" "\xE1" "\xE0" \
                   "\xDF" "\xDE" "\xDD" "\xDC" "\xDB" "\xDA" "\xD9" "\xD8" \
                   "\xD7" "\xD6" "\xD5" "\xD4" "\xD3" "\xD2" "\xD1" "\xD0" \
                   "\xCF" "\xCE" "\xCD" "\xCC" "\xCB" "\xCA" "\xC9" "\xC8" \
                   "\xC7" "\xC6" "\xC5" "\xC4" "\xC3" "\xC2" "\xC1" "\xC0" \
                   "\xBF" "\xBE" "\xBD" "\xBC" "\xBB" "\xBA" "\xB9" "\xB8" \
                   "\xB7" "\xB6" "\xB5" "\xB4" "\xB3" "\xB2" "\xB1" "\xB0" \
                   "\xAF" "\xAE" "\xAD" "\xAC" "\xAB" "\xAA" "\xA9" "\xA8" \
                   "\xA7" "\xA6" "\xA5" "\xA4" "\xA3" "\xA2" "\xA1" "\xA0" \
                   "\x9F" "\x9E" "\x9D" "\x9C" "\x9B" "\x9A" "\x99" "\x98" \
                   "\x97" "\x96" "\x95" "\x94" "\x93" "\x92" "\x91" "\x90" \
                   "\x8F" "\x8E" "\x8D" "\x8C" "\x8B" "\x8A" "\x89" "\x88" \
                   "\x87" "\x86" "\x85" "\x84" "\x83" "\x82" "\x81" "\x80" \
                   "\x7F" "~}|{zyxwvutsrqponmlkjihgfedcba`_^]\\[ZYXWVUTS" \
                   "RQPONMLKJIHGFEDCBA@?>=<;:9876543210/.-,+*)('&%$#\"! " \
                   "\x1F" "\x1E" "\x1D" "\x1C" "\x1B" "\x1A" "\x19" "\x18" \
                   "\x17" "\x16" "\x15" "\x14" "\x13" "\x12" "\x11" "\x10" \
                   "\x0F" "\x0E" "\x0D" "\x0C" "\x0B" "\x0A" "\x09" "\x08" \
                   "\x07" "\x06" "\x05" "\x04" "\x03" "\x02" "\x01" "\x00"),
    {
      583u,
      {
        0xFF, 0xFF, 0xFB, 0xBF, 0xFF, 0xFF, 0x87, 0xFF, 0xFF, 0xEF,
        0xFF, 0xFF, 0xFD, 0xDF, 0xFF, 0xFF, 0xB7, 0xFF, 0xFF, 0xF6,
        0x7F, 0xFF, 0xFF, 0xF7, 0xFF, 0xFF, 0xEB, 0xFF, 0xFF, 0xFD,
        0x5F, 0xFF, 0xFF, 0xA7, 0xFF, 0xFF, 0xF4, 0x7F, 0xFF, 0xFE,
        0x7F, 0xFF, 0xFF, 0xB7, 0xFF, 0xFF, 0xEC, 0xFF, 0xFF, 0xFC,
        0xDF, 0xFF, 0xFF, 0x5F, 0xFF, 0xFF, 0x4F, 0xFF, 0xFF, 0xAB,
        0xFF, 0xFF, 0xD7, 0xFF, 0xFF, 0xD3, 0xFF, 0xFF, 0xDF, 0xFF,
        0xFF, 0xEE, 0xFF, 0xFF, 0xAF, 0xFF, 0xFE, 0xAF, 0xFF, 0xFE,
        0x7F, 0xFF, 0xE8, 0xFF, 0xFF, 0x3F, 0xFF, 0xFD, 0x3F, 0xFF,
        0xE6, 0xFF, 0xFE, 0xDF, 0xFF, 0xFF, 0x3F, 0xFF, 0xEC, 0xFF,
        0xFF, 0xFC, 0xBF, 0xFF, 0xFF, 0x93, 0xFF, 0xFF, 0xF1, 0xFF,
        0xFF, 0xFF, 0xEF, 0xFF, 0xFF, 0xD3, 0xFF, 0xFF, 0xF4, 0x7F,
        0xFF, 0x97, 0xFF, 0xFC, 0x9F, 0xFF, 0xFE, 0x5F, 0xFF, 0xFF,
        0x8B, 0xFF, 0xFF, 0xE7, 0xFF, 0xFF, 0xFC, 0x3F, 0xFF, 0xFF,
        0x83, 0xFF, 0xFF, 0xE6, 0xFF, 0xFF, 0x1F, 0xFF, 0xF2, 0xFF,
        0xFF, 0xF6, 0xFF, 0xFF, 0xF8, 0xFF, 0xFF, 0xFC, 0xBF, 0xFF,
        0xFF, 0x7F, 0xFF, 0xFF, 0xEF, 0x7F, 0xFF, 0xFC, 0x9F, 0xFF,
        0xFF, 0x1F, 0xFF, 0xFF, 0xC5, 0xFF, 0xFF, 0xEC, 0xFF, 0xFF,
        0xA3, 0xFF, 0xFF, 0x97, 0xFF, 0xFC, 0xFF, 0xFF, 0xC7, 0xFF,
        0xFA, 0xFF, 0xFF, 0xFE, 0x1F, 0xFF, 0xFF, 0x83, 0xFF, 0xFF,
        0x8F, 0xFF, 0xFC, 0xDF, 0xFF, 0xF2, 0xFF, 0xFF, 0xF0, 0xFF,
        0xFF, 0x93, 0xFF, 0xFE, 0x3F, 0xFF, 0xF8, 0xBF, 0xFF, 0xAB,
        0xFF, 0xFF, 0x7F, 0xFF, 0xFE, 0xEF, 0xFF, 0xF8, 0x7F, 0xFF,
        0xF6, 0xFF, 0xFF, 0x8B, 0xFF, 0xFE, 0x0F, 0xFF, 0xF0, 0xFF,
        0xFF, 0x83, 0xFF, 0xFF, 0x67, 0xFF, 0xFE, 0xBF, 0xFF, 0xF7,
        0xFF, 0xFF, 0xBF, 0xFF, 0xFF, 0xE1, 0xFF, 0xFE, 0xF7, 0xFF,
        0xFB, 0xBF, 0xFF, 0xFA, 0xBF, 0xFF, 0xBD, 0xFF, 0xFF, 0xA7,
        0xFF, 0xFF, 0x47, 0xFF, 0xFB, 0x9F, 0xFF, 0xED, 0xFF, 0xFF,
        0x4F, 0xFF, 0xF7, 0x7F, 0xFF, 0xDA, 0xFF, 0xFF, 0xEF, 0xFF,
        0xFF, 0xCF, 0xFF, 0xFF, 0x9B, 0xFF, 0xFD, 0x9F, 0xFF, 0xFC,
        0xBF, 0xFF, 0xEC, 0x7F, 0xFF, 0x73, 0xFF, 0xFF, 0x27, 0xFF,
        0xFE, 0x3F, 0xFF, 0xFC, 0x5F, 0xFF, 0xF8, 0x7F, 0xFF, 0xFB,
        0xBF, 0xFF, 0xF0, 0x7F, 0xFF, 0xAF, 0xFF, 0xFF, 0xDB, 0xFF,
        0xFF, 0xD9, 0xFF, 0xFF, 0x7F, 0xFF, 0xFF, 0xAF, 0xFF, 0xFE,
        0xF7, 0xFF, 0xFD, 0xDF, 0xFF, 0xFB, 0x9F, 0xFF, 0xF6, 0xFF,
        0xFF, 0xED, 0x7F, 0xFF, 0xAD, 0xFF, 0xFF, 0x67, 0xFF, 0xFD,
        0x5F, 0xFF, 0xF5, 0x3F, 0xFF, 0xD3, 0xFF, 0xFE, 0x8F, 0xFF,
        0xE7, 0xFF, 0xFF, 0x4B, 0xFF, 0xF9, 0xBF, 0xFF, 0xFF, 0xF3,
        0xFF, 0xBF, 0xFE, 0xFF, 0xCF, 0xFF, 0xDE, 0xFD, 0x79, 0xF1,
        0xDE, 0xD4, 0xA2, 0xCE, 0xD5, 0x9E, 0xAA, 0x68, 0xEB, 0xD0,
        0xD3, 0xCD, 0x29, 0x64, 0x24, 0x63, 0xFF, 0xFB, 0x17, 0xFF,
        0x9F, 0xFC, 0xFF, 0xFE, 0x1F, 0xFB, 0xFD, 0xE7, 0xF9, 0xCB,
        0x8F, 0x0D, 0xFB, 0xB6, 0xEC, 0xD7, 0xAB, 0x4E, 0x8C, 0xF9,
        0xB2, 0xE4, 0xC7, 0x8B, 0x0E, 0x0B, 0xF7, 0xAE, 0xC3, 0xFF,
        0xAF, 0xF3, 0xFE, 0xE0, 0xFF, 0xF9, 0xF7, 0x71, 0xF7, 0x9D,
        0x71, 0xB6, 0x99, 0x10, 0x40, 0xC2, 0xEB, 0x7D, 0x7F, 0xBF,
        0x9F, 0xEF, 0xFA, 0xFF, 0x5F, 0x0A, 0xFF, 0xE7, 0xFE, 0xBF,
        0x9F, 0xE1, 0x4F, 0xFF, 0xFF, 0xFB, 0xFF, 0xFF, 0xFF, 0xAF,
        0xFF, 0xFF, 0xF9, 0xFF, 0xFF, 0xFF, 0x8F, 0xFF, 0xFF, 0xF7,
        0xFF, 0xFF, 0xFF, 0x6F, 0xFF, 0xFF, 0xF5, 0xFF, 0xFF, 0xFF,
        0x4F, 0xFF, 0xFF, 0xF3, 0xFF, 0xFF, 0xFF, 0xFB, 0xFF, 0xFF,
        0xFC, 0xBF, 0xFF, 0xFF, 0xC7, 0xFF, 0xFF, 0xFC, 0x3F, 0xFF,
        0xFF, 0xBF, 0xFF, 0xFF, 0xFB, 0xBF, 0xFF, 0xFF, 0xB7, 0xFF,
        0xFF, 0xFB, 0x3F, 0xFF, 0xFF, 0xAF, 0xFF, 0xFF, 0xFF, 0xDF,
        0xFF, 0xFF, 0xEA, 0xFF, 0xFF, 0xFE, 0x9F, 0xFF, 0xFF, 0xFF,
        0x3F, 0xFF, 0xFA, 0xBF, 0xFF, 0xFF, 0xA3, 0xFF, 0xFF, 0xF9,
        0xFF, 0xFF, 0xFF, 0x9B, 0xFF, 0xFF, 0xF9, 0x7F, 0xFF, 0xFF,
        0x93, 0xFF, 0xFF, 0xF8, 0xFF, 0xFF, 0xFF, 0x8B, 0xFF, 0xFE,
        0xC7, 0xFE, 0x3F
      }
    }
  }
  /* Various paddings */
  ,
  {   /* Zero bits padding */
    mhd_MSTR_INIT ("aaaaaaaa"),
    {
      5u,
      {
        0x18, 0xC6, 0x31, 0x8C, 0x63
      }
    }
  }
  ,
  {   /* 1 bit padding */
    mhd_MSTR_INIT ("aaa"),
    {
      2u,
      {
        0x18, 0xC7
      }
    }
  }
  ,
  {   /* 2 bits padding */
    mhd_MSTR_INIT ("aaaaaa"),
    {
      4u,
      {
        0x18, 0xC6, 0x31, 0x8F
      }
    }
  }
  ,
  {   /* 3 bits padding */
    mhd_MSTR_INIT ("a"),
    {
      1u,
      {
        0x1F
      }
    }
  }
  ,
  {   /* 4 bits padding */
    mhd_MSTR_INIT ("aaaa"),
    {
      3u,
      {
        0x18, 0xC6, 0x3F
      }
    }
  }
  ,
  {   /* 5 bits padding */
    mhd_MSTR_INIT ("aaaaaaa"),
    {
      5u,
      {
        0x18, 0xC6, 0x31, 0x8C, 0x7F
      }
    }
  }
  ,
  {   /* 6 bits padding */
    mhd_MSTR_INIT ("aa"),
    {
      2u,
      {
        0x18, 0xFF
      }
    }
  }
  ,
  {   /* 7 bits padding */
    mhd_MSTR_INIT ("aaaaa"),
    {
      4u,
      {
        0x18, 0xC6, 0x31, 0xFF
      }
    }
  }
};


static void
print_hex (FILE *stream, size_t hex_size, const uint8_t *hex)
{
  size_t i;
  if (0 == hex_size)
    return;
  fprintf (stream, "%02X", (unsigned int) hex[0]);
  for (i = 1; i < hex_size; ++i)
    fprintf (stream, " %02X", (unsigned int) hex[i]);
}


/* Not thread-safe! Assuming that the function is called only in one thread */
static const char *
print_mixed (size_t str_len, const char *str)
{
  static char tmp_bufs[8][2048]; /* Should be enough for testing */
  static size_t cur_buf_idx = 0;
  static const size_t buf_size = sizeof(tmp_bufs[0]);
  char *buf;
  size_t i;
  size_t out_idx;
  int quote_open = 0;

  buf = tmp_bufs[(cur_buf_idx++) % (sizeof(tmp_bufs) / sizeof(tmp_bufs[0]))];

  for (i = 0, out_idx = 0; i < str_len; ++i)
  {
    const char c = str[i];
    if (0x20 <= c && 0x7E >= c)
    {
      if (! quote_open)
      {
        if (0 != i)
        {
          if (buf_size <= out_idx + 3)
            break;
          buf[out_idx++] = ' ';
        }
        else if (buf_size <= out_idx + 2)
          break;

        buf[out_idx++] = '"';
        quote_open = ! 0;
      }
      else if (buf_size <= out_idx + 1)
        break;

      buf[out_idx++] = c;
    }
    else
    {
      const uint8_t digit1 = ((uint8_t) c) & 0xFu;
      const uint8_t digit2 = ((uint8_t) c) >> 4;
      if (quote_open)
      {
        if (buf_size <= out_idx + 8)
          break;
        buf[out_idx++] = '"';
        quote_open = 0;
        buf[out_idx++] = ' ';
      }
      else if (buf_size <= out_idx + 6)
        break;
      buf[out_idx++] = '"';
      buf[out_idx++] = '\\';
      buf[out_idx++] = 'x';
      buf[out_idx++] =
        (char) ((digit1 < 10) ? ('0' + digit1) : ('A' + digit1 - 10));
      buf[out_idx++] =
        (char) ((digit2 < 10) ? ('0' + digit2) : ('A' + digit2 - 10));
      buf[out_idx++] = '"';
    }
  }
  if (i < str_len)
    return "[TOO LARGE STRING]";

  if (quote_open)
  {
    if (buf_size <= out_idx + 2)
      return "[TOO LARGE STRING]";
    buf[out_idx++] = '"';
  }

  if (buf_size <= out_idx + 1)
    return "[TOO LARGE STRING]";

  buf[out_idx++] = 0;

  return buf;
}


/* non-zero error code on error, zero on success */
static int
basic_data_check (void)
{
  size_t i;
  for (i = 0; i < sizeof(h_data) / sizeof(h_data[0]); ++i)
  {
    /* Check the input data */
#if 0 /* Disabled check, such data is used for checks */
    if (0 == h_data[i].str.len)
    {
      fflush (stdout);
      fprintf (stderr, "ERROR: wrong zero length of the string data for "
               "the entry number %lu\n",
               (unsigned long) i);
      return 99;
    }
#endif /* Disabled check */
    if (NULL == h_data[i].str.cstr)
    {
      fflush (stdout);
      fprintf (stderr, "ERROR: wrong zero NULL pointer for the string data for "
               "the entry number %lu\n",
               (unsigned long) i);
      return 99;
    }
    else if (h_data[i].str.len < strlen (h_data[i].str.cstr))
    {
      fflush (stdout);
      fprintf (stderr, "ERROR: unterminated string data for "
               "the entry number %lu\n",
               (unsigned long) i);
      return 99;
    }
    if (0 != h_data[i].str.cstr[h_data[i].str.len])
    {
      fflush (stdout);
      fprintf (stderr, "ERROR: unterminated string for the string data for "
               "the entry number %lu\n",
               (unsigned long) i);
      return 99;
    }
    if ((0 == h_data[i].enc.size)
        && (0 != h_data[i].str.len))
    {
      fflush (stdout);
      fprintf (stderr, "ERROR: wrong zero length of encoded data for "
               "the entry with with the string data \"%s\"\n",
               h_data[i].str.cstr);
      return 99;
    }
    if (((sizeof(h_data[i].enc.data) != h_data[i].enc.size)) &&
        (0 != h_data[i].enc.data[h_data[i].enc.size]))
    {
      fflush (stdout);
      fprintf (stderr, "ERROR: wrong length of encoded data for "
               "the entry with with the string data \"%s\"\n",
               h_data[i].str.cstr);
      return 99;
    }
  }
  return 0;
}


/* ------------------ Encode testing ------------------- */


/* non-zero error code on error, zero on success */
static int
perform_single_enc_check (size_t str_len,
                          const char *restrict str,
                          size_t valid_enc_size,
                          const uint8_t *restrict valid_enc,
                          size_t use_buff_size)
{
  uint8_t buff[sizeof(h_data[0].enc.data) * 4];
  size_t res;

  if (sizeof(buff) < use_buff_size)
  {
    fflush (stdout);
    fprintf (stderr, "ERROR: wrong size of the buffer specified for "
             "the check.\n"
             "The specified size:        %lu\n"
             "The maximum possible size: %lu\n",
             (unsigned long) use_buff_size,
             (unsigned long) sizeof(buff));
    return 99;
  }
  memset (buff, 0, sizeof(buff));

  res = mhd_h2_huffman_encode (str_len,
                               str,
                               use_buff_size,
                               buff);
  if (valid_enc_size > use_buff_size)
  {
    if (0 == res)
      return 0; /* Failed as required */
    fflush (stdout);
    fprintf (stderr, "FAILED: mhd_h2_huffman_encode(%lu, %s, %lu, buffer) "
             "returned value %lu, while expected ZERO\n"
             "If the buffer would have enough space then "
             "the encoded data must be:",
             (unsigned long) str_len,
             print_mixed (str_len, str),
             (unsigned long) use_buff_size,
             (unsigned long) res);
    print_hex (stderr, valid_enc_size, valid_enc);
    fprintf (stderr, "\nResulting encoded data: ");
    print_hex (stderr, res, buff);
    fprintf (stderr, "\n");
    return 3;
  }

  if (valid_enc_size != res)
  {
    fflush (stdout);
    fprintf (stderr, "FAILED: mhd_h2_huffman_encode(%lu, %s, %lu, buffer) "
             "returned value %lu, while expected %lu\n",
             (unsigned long) str_len,
             print_mixed (str_len, str),
             (unsigned long) valid_enc_size,
             (unsigned long) res,
             (unsigned long) valid_enc_size);
    return 1;
  }
  if (0 != memcmp (valid_enc, buff, res))
  {
    fflush (stdout);
    fprintf (stderr, "FAILED: mhd_h2_huffman_encode(%lu, %s, %lu, buffer) "
             "returned expected value %lu, "
             "but the data is encoded incorrectly in the output buffer.\n",
             (unsigned long) str_len,
             print_mixed (str_len, str),
             (unsigned long) valid_enc_size,
             (unsigned long) res);
    fprintf (stderr,   "Expected encoded data:  ");
    print_hex (stderr, valid_enc_size, valid_enc);
    fprintf (stderr, "\nResulting encoded data: ");
    print_hex (stderr, res, buff);
    fprintf (stderr, "\n");
    return 2;
  }

  return 0;
}


/* non-zero error code on error, zero on success */
static int
check_h2_h_encode_perfect_size (void)
{
  size_t i;
  for (i = 0; i < sizeof(h_data) / sizeof(h_data[0]); ++i)
  {
    int res;

    res = perform_single_enc_check (h_data[i].str.len,
                                    h_data[i].str.cstr,
                                    h_data[i].enc.size,
                                    h_data[i].enc.data,
                                    h_data[i].enc.size);

    if (0 != res)
      return res;
  }
  return 0;
}


/* non-zero error code on error, zero on success */
static int
check_h2_h_encode_larger_size (void)
{
  size_t i;
  for (i = 0; i < sizeof(h_data) / sizeof(h_data[0]); ++i)
  {
    int res;

    res = perform_single_enc_check (h_data[i].str.len,
                                    h_data[i].str.cstr,
                                    h_data[i].enc.size,
                                    h_data[i].enc.data,
                                    h_data[i].enc.size + 1);

    if (0 != res)
      return res;
  }
  return 0;
}


/* non-zero error code on error, zero on success */
static int
check_h2_h_encode_smaller_size (void)
{
  size_t i;
  for (i = 0; i < sizeof(h_data) / sizeof(h_data[0]); ++i)
  {
    int res;
    if (0 == h_data[i].enc.size)
      continue;

    res = perform_single_enc_check (h_data[i].str.len,
                                    h_data[i].str.cstr,
                                    h_data[i].enc.size,
                                    h_data[i].enc.data,
                                    h_data[i].enc.size - 1);

    if (0 != res)
      return res;
  }
  return 0;
}


/* non-zero error code on error, zero on success */
static int
check_h2_h_encode_fixed_size (size_t use_size)
{
  size_t i;
  for (i = 0; i < sizeof(h_data) / sizeof(h_data[0]); ++i)
  {
    int res;

    res = perform_single_enc_check (h_data[i].str.len,
                                    h_data[i].str.cstr,
                                    h_data[i].enc.size,
                                    h_data[i].enc.data,
                                    use_size);

    if (0 != res)
      return res;
  }
  return 0;
}


/* non-zero error code on error, zero on success */
static int
check_h2_h_encode_fixed_sizes (void)
{
  size_t i;
  for (i = 0; i <= 16; ++i)
  {
    int res;

    res = check_h2_h_encode_fixed_size (i);

    if (0 != res)
      return res;
  }
  return 0;
}


/* non-zero error code on error, zero on success */
static int
test_encode (void)
{
  int res;

  printf ("Checking H2 Huffman encoding with"
          " precise output buffer size...\n");
  res = check_h2_h_encode_perfect_size ();
  if (0 != res)
    return res;
  printf ("Succeed\n");

  printf ("Checking H2 Huffman encoding with"
          " larger output buffer size...\n");
  res = check_h2_h_encode_larger_size ();
  if (0 != res)
    return res;
  printf ("Succeed\n");

  printf ("Checking H2 Huffman encoding with"
          " smaller output buffer size...\n");
  res = check_h2_h_encode_smaller_size ();
  if (0 != res)
    return res;
  printf ("Succeed\n");

  printf ("Checking H2 Huffman encoding with"
          " various fixed output buffer sizes...\n");
  res = check_h2_h_encode_fixed_sizes ();
  if (0 != res)
    return res;
  printf ("Succeed\n");

  return 0;
}


/* ------------------ Decode testing ------------------- */

static const char *
decode_result_to_string (enum mhd_H2HuffDecodeRes res)
{
  switch (res)
  {
  case MHD_H2_HUFF_DEC_RES_OK:
    return "'OK'";
  case MHD_H2_HUFF_DEC_RES_NO_SPACE:
    return "'NO_SPACE'";
  case MHD_H2_HUFF_DEC_RES_BROKEN_DATA:
    return "'BROKEN_DATA'";
  default:
    break;
  }
  return "[UNKNOWN RESULT]";
}


/* non-zero error code on error, zero on success */
static int
perform_single_dec_valid_check (size_t enc_size,
                                const uint8_t *restrict enc,
                                size_t valid_str_len,
                                const char *restrict valid_str,
                                size_t use_buff_size)
{
  char buff[257 * 2];
  enum mhd_H2HuffDecodeRes res;
  size_t ret_size;

  if (sizeof(buff) < use_buff_size)
  {
    fflush (stdout);
    fprintf (stderr, "ERROR: wrong size of the buffer specified for "
             "the check.\n"
             "The specified size:        %lu\n"
             "The maximum possible size: %lu\n",
             (unsigned long) use_buff_size,
             (unsigned long) sizeof(buff));
    return 99;
  }
  memset (buff, 0, sizeof(buff));

  ret_size = mhd_h2_huffman_decode (enc_size,
                                    enc,
                                    use_buff_size,
                                    buff,
                                    &res);
  if (valid_str_len > use_buff_size)
  {
    if ((0 == ret_size) && (MHD_H2_HUFF_DEC_RES_NO_SPACE == res))
      return 0; /* Failed as required */
    fflush (stdout);
    fprintf (stderr, "FAILED: mhd_h2_huffman_decode(%lu, encoded_data, "
             "%lu, %s%s, ->%s) returned %lu\n",
             (unsigned long) enc_size,
             (unsigned long) use_buff_size,
             (0 == ret_size) ? "buffer" : "->",
             (0 == ret_size) ? "" : print_mixed (ret_size, buff),
             decode_result_to_string (res),
             (unsigned long) ret_size);
    if (0 != ret_size)
      fprintf (stderr, "The function returned value %lu, while expected ZERO\n",
               (unsigned long) ret_size);
    else
      fprintf (stderr, "The function returned ZERO as expected, but the status "
               "must be 'NO_SPACE'\n");

    fprintf (stderr, "The encoded data is: ");
    print_hex (stderr, enc_size, enc);
    fprintf (stderr, "\nIf the buffer would have enough space then "
             "the decoded string must be: %s\n",
             print_mixed (valid_str_len, valid_str));
    return 3;
  }

  if ((valid_str_len != ret_size) ||
      ((MHD_H2_HUFF_DEC_RES_OK != res)))
  {
    fflush (stdout);
    fprintf (stderr, "FAILED: mhd_h2_huffman_decode(%lu, encoded_data, "
             "%lu, %s%s, ->%s) returned %lu\n",
             (unsigned long) enc_size,
             (unsigned long) use_buff_size,
             (0 == ret_size) ? "buffer" : "->",
             (0 == ret_size) ? "" : print_mixed (ret_size, buff),
             decode_result_to_string (res),
             (unsigned long) ret_size);

    if (valid_str_len != ret_size)
      fprintf (stderr, "The function returned value %lu, while expected %lu\n",
               (unsigned long) ret_size, (unsigned long) valid_str_len);
    else
      fprintf (stderr, "The function returned expected value %lu, "
               "but the status must be 'OK'\n",
               (unsigned long) ret_size);

    fprintf (stderr, "The encoded data is: ");
    print_hex (stderr, enc_size, enc);
    fprintf (stderr, "\nThe expected decoded string: %s\n",
             print_mixed (valid_str_len, valid_str));

    return 1;
  }
  if (0 != memcmp (valid_str, buff, ret_size))
  {
    fflush (stdout);
    fprintf (stderr, "FAILED: mhd_h2_huffman_decode(%lu, encoded_data, "
             "%lu, buffer, ->%s) returned %lu\n",
             (unsigned long) enc_size,
             (unsigned long) use_buff_size,
             decode_result_to_string (res),
             (unsigned long) ret_size);
    fprintf (stderr,"The function returned expected value %lu and set "
             "correct 'OK' status, but the data is decoded incorrectly "
             "in the output buffer.\n",
             (unsigned long) ret_size);
    fprintf (stderr, "The decoded string:  %s\n",
             print_mixed (ret_size, buff));
    fprintf (stderr, "The expected string: %s\n",
             print_mixed (valid_str_len, valid_str));
    fprintf (stderr, "The encoded data is: ");
    print_hex (stderr, enc_size, enc);
    fprintf (stderr, "\n");
    return 2;
  }

  return 0;
}


/* non-zero error code on error, zero on success */
static int
check_h2_h_decode_perfect_size (void)
{
  size_t i;
  for (i = 0; i < sizeof(h_data) / sizeof(h_data[0]); ++i)
  {
    int res;

    res = perform_single_dec_valid_check (h_data[i].enc.size,
                                          h_data[i].enc.data,
                                          h_data[i].str.len,
                                          h_data[i].str.cstr,
                                          h_data[i].str.len);

    if (0 != res)
      return res;
  }
  return 0;
}


/* non-zero error code on error, zero on success */
static int
check_h2_h_decode_larger_size (void)
{
  size_t i;
  for (i = 0; i < sizeof(h_data) / sizeof(h_data[0]); ++i)
  {
    int res;

    res = perform_single_dec_valid_check (h_data[i].enc.size,
                                          h_data[i].enc.data,
                                          h_data[i].str.len,
                                          h_data[i].str.cstr,
                                          h_data[i].str.len + 1);

    if (0 != res)
      return res;
  }
  return 0;
}


/* non-zero error code on error, zero on success */
static int
check_h2_h_decode_smaller_size (void)
{
  size_t i;
  for (i = 0; i < sizeof(h_data) / sizeof(h_data[0]); ++i)
  {
    int res;
    if (0 == h_data[i].enc.size)
      continue;

    res = perform_single_dec_valid_check (h_data[i].enc.size,
                                          h_data[i].enc.data,
                                          h_data[i].str.len,
                                          h_data[i].str.cstr,
                                          h_data[i].str.len - 1);

    if (0 != res)
      return res;
  }
  return 0;
}


/* non-zero error code on error, zero on success */
static int
check_h2_h_decode_fixed_size (size_t use_size)
{
  size_t i;
  for (i = 0; i < sizeof(h_data) / sizeof(h_data[0]); ++i)
  {
    int res;

    res = perform_single_dec_valid_check (h_data[i].enc.size,
                                          h_data[i].enc.data,
                                          h_data[i].str.len,
                                          h_data[i].str.cstr,
                                          use_size);

    if (0 != res)
      return res;
  }
  return 0;
}


/* non-zero error code on error, zero on success */
static int
check_h2_h_decode_fixed_sizes (void)
{
  size_t i;
  for (i = 0; i <= 16; ++i)
  {
    int res;

    res = check_h2_h_decode_fixed_size (i);

    if (0 != res)
      return res;
  }
  return 0;
}


/* This function tries to decode the data and it is succeed, checks
 * whether the decoded string can be encoded back to the original encoded
 * data.
 * The check does not detect situations when a valid encoded string
 * cannot be decoded, however as correct codes are re-checked by encoder
 * and the total number of valid codes is counted, this check is strong
 * enough (unless decoder and encoder have symmetric bugs).
 */
/* non-zero error code on error, zero on success */
static int
perform_single_dec_unknow_seq_check (size_t enc_size,
                                     const uint8_t *restrict enc,
                                     uint_fast32_t *valid_codes_counter)
{
  char buff[8 * 4];
  static const size_t buff_size = sizeof(buff);
  enum mhd_H2HuffDecodeRes res;
  size_t ret_size;

  if (buff_size < enc_size * 4)
  {
    fflush (stdout);
    fprintf (stderr, "ERROR: wrong size of the value specified for "
             "the check.\n"
             "The specified size:        %lu\n"
             "The maximum possible size: %lu\n",
             (unsigned long) enc_size,
             (unsigned long) buff_size / 4);
    return 99;
  }
  if (0 == enc_size)
  {
    fflush (stdout);
    fprintf (stderr, "ERROR: zero size of the value specified for "
             "the check.\n");
    return 99;
  }

  ret_size = mhd_h2_huffman_decode (enc_size,
                                    enc,
                                    buff_size,
                                    buff,
                                    &res);

  if (0 == ret_size && MHD_H2_HUFF_DEC_RES_OK == res)
  {
    fflush (stdout);
    fprintf (stderr, "FAILED: mhd_h2_huffman_decode(%lu, encoded_data, "
             "%lu, %s%s, ->%s) returned %lu\n",
             (unsigned long) enc_size,
             (unsigned long) buff_size,
             (0 == ret_size) ? "buffer" : "->",
             (0 == ret_size) ? "" : print_mixed (ret_size, buff),
             decode_result_to_string (res),
             (unsigned long) ret_size);
    fprintf (stderr, "The function returned ZERO, but the status was not "
             "set to 'OK' (actual: %s), while the size of the input data"
             " is not ZERO.\n",
             decode_result_to_string (res));
    return 5;
  }
  if (0 != ret_size && MHD_H2_HUFF_DEC_RES_OK != res)
  {
    fflush (stdout);
    fprintf (stderr, "FAILED: mhd_h2_huffman_decode(%lu, encoded_data, "
             "%lu, %s%s, ->%s) returned %lu\n",
             (unsigned long) enc_size,
             (unsigned long) buff_size,
             (0 == ret_size) ? "buffer" : "->",
             (0 == ret_size) ? "" : print_mixed (ret_size, buff),
             decode_result_to_string (res),
             (unsigned long) ret_size);
    fprintf (stderr, "The function returned non-ZERO, but the status was "
             "set to 'OK'.\n");
    return 5;
  }
  if (MHD_H2_HUFF_DEC_RES_NO_SPACE == res)
  {
    fflush (stdout);
    fprintf (stderr, "FAILED: mhd_h2_huffman_decode(%lu, encoded_data, "
             "%lu, %s%s, ->%s) returned %lu\n",
             (unsigned long) enc_size,
             (unsigned long) buff_size,
             (0 == ret_size) ? "buffer" : "->",
             (0 == ret_size) ? "" : print_mixed (ret_size, buff),
             decode_result_to_string (res),
             (unsigned long) ret_size);
    fprintf (stderr, "The function set the status set to 'NO_SPACE', but "
             "the buffer should be enough to hold the result.\n");
    return 6;
  }
  if (MHD_H2_HUFF_DEC_RES_OK == res)
  {
    uint8_t check_enc_buf[128];
    size_t check_enc_size;

    check_enc_size = mhd_h2_huffman_encode (ret_size,
                                            buff,
                                            sizeof(check_enc_buf),
                                            check_enc_buf);
    if (0 == check_enc_size)
    {
      fflush (stdout);
      fprintf (stderr, "ERROR: mhd_h2_huffman_decode(%lu, encoded_data, "
               "%lu, %s%s, ->%s) returned %lu\n",
               (unsigned long) enc_size,
               (unsigned long) buff_size,
               (0 == ret_size) ? "buffer" : "->",
               (0 == ret_size) ? "" : print_mixed (ret_size, buff),
               decode_result_to_string (res),
               (unsigned long) check_enc_size);
      fprintf (stderr, "However, when checking the decoded result by "
               "encoding it back to H2 Huffman encoding the function "
               "mhd_h2_huffman_encode(%lu, %s, %lu, buffer) returned "
               "ZERO, which should not happen as the output "
               "buffer is large enough.\n",
               (unsigned long) ret_size,
               print_mixed (ret_size, buff),
               (unsigned long) sizeof(check_enc_buf));
      return 99;
    }
    if ((enc_size != check_enc_size) ||
        (0 != memcmp (enc, check_enc_buf, check_enc_size)))
    {
      fflush (stdout);
      fprintf (stderr, "FAILED: mhd_h2_huffman_decode(%lu, encoded_data, "
               "%lu, %s%s, ->%s) returned %lu\n",
               (unsigned long) enc_size,
               (unsigned long) buff_size,
               (0 == ret_size) ? "buffer" : "->",
               (0 == ret_size) ? "" : print_mixed (ret_size, buff),
               decode_result_to_string (res),
               (unsigned long) ret_size);
      if (enc_size != check_enc_size)
        fprintf (stderr, "However, when checking the decoded result by "
                 "encoding it back to H2 Huffman encoding the function "
                 "mhd_h2_huffman_encode(%lu, %s, %lu, buffer) returned "
                 "%lu, which does not match the original data size.\n",
                 (unsigned long) ret_size,
                 print_mixed (ret_size, buff),
                 (unsigned long) sizeof(check_enc_buf),
                 (unsigned long) check_enc_size);
      else
        fprintf (stderr, "However, when checking the decoded result by "
                 "encoding it back to H2 Huffman encoding the function "
                 "mhd_h2_huffman_encode(%lu, %s, %lu, buffer) returned "
                 "%lu (as expected), but the encoded data does not match "
                 "original encoded data.\n",
                 (unsigned long) ret_size,
                 print_mixed (ret_size, buff),
                 (unsigned long) sizeof(check_enc_buf),
                 (unsigned long) ret_size);

      fprintf (stderr, "The decoded string:  %s\n",
               print_mixed (ret_size, buff));
      fprintf (stderr, "The original encoded data is: ");
      print_hex (stderr, enc_size, enc);
      fprintf (stderr, "\n"
               "The string encoded back is:   ");
      print_hex (stderr, check_enc_size, check_enc_buf);
      fprintf (stderr, "\n");

      return 7;
    }
    *valid_codes_counter += 1;
  }

  return 0;
}


/* non-zero error code on error, zero on success */
static int
check_h2_h_decode_all_one_byte_values (void)
{
  static const uint_fast32_t known_valid_codes = 74u;
  uint_fast32_t valid_codes_counter;
  uint_fast16_t i;

  valid_codes_counter = 0;
  i = 0;
  do
  {
    uint8_t test_data[1];
    int res;
    test_data[0] = (uint8_t) i;

    res = perform_single_dec_unknow_seq_check (sizeof(test_data),
                                               test_data,
                                               &valid_codes_counter);

    if (0 != res)
      return res;
  } while (0xFFu > i++);

  if (known_valid_codes != valid_codes_counter)
  {
    fflush (stdout);
    fprintf (stderr, "The check found %lu valid code values, while"
             "the expected number of valid code values is %lu.\n",
             (unsigned long) valid_codes_counter,
             (unsigned long) known_valid_codes);
    return 9;
  }

  return 0;
}


/* non-zero error code on error, zero on success */
static int
check_h2_h_decode_all_two_bytes_values (void)
{
  static const uint_fast32_t known_valid_codes = 14717u;
  uint_fast32_t valid_codes_counter;
  uint_fast16_t i;

  valid_codes_counter = 0;
  i = 0;
  do
  {
    uint8_t test_data[2];
    int res;
    test_data[0] = (uint8_t) i & 0xFFu;
    test_data[1] = (uint8_t) ((i & 0xFF00u) >> 8u);

    res = perform_single_dec_unknow_seq_check (sizeof(test_data),
                                               test_data,
                                               &valid_codes_counter);

    if (0 != res)
      return res;
  } while (0xFFFFu > i++);

  if (known_valid_codes != valid_codes_counter)
  {
    fflush (stdout);
    fprintf (stderr, "The check found %lu valid code values, while "
             "the expected number of valid code values is %lu.\n",
             (unsigned long) valid_codes_counter,
             (unsigned long) known_valid_codes);
    return 9;
  }

  return 0;
}


/* non-zero error code on error, zero on success */
static int
check_h2_h_decode_all_three_bytes_values (void)
{
  static const uint_fast32_t known_valid_codes = 6759631u;
  static const uint_fast32_t one_pct_step = 0xFFFFFFu / 100u;
  uint_fast32_t valid_codes_counter;
  uint_fast32_t i;

  valid_codes_counter = 0;
  i = 0;
  printf ("Completed: 00%% ");
  fflush (stdout);
  do
  {
    uint8_t test_data[3];
    int res;
    test_data[0] = (uint8_t) i & 0xFFu;
    test_data[1] = (uint8_t) ((i & 0xFF00u) >> 8u);
    test_data[2] = (uint8_t) ((i & 0xFF0000u) >> 16u);

    res = perform_single_dec_unknow_seq_check (sizeof(test_data),
                                               test_data,
                                               &valid_codes_counter);

    if (0 != res)
      return res;

    /* Imperfect calculations, but enough for tracking the progress */
    if (0u == (i % one_pct_step))
    {
      printf ("\b\b\b\b%02u%% ", (unsigned int) (i / one_pct_step));
      fflush (stdout);
    }
  } while (0xFFFFFFu > i++);
  printf ("\n");

  if (known_valid_codes != valid_codes_counter)
  {
    fflush (stdout);
    fprintf (stderr, "The check found %lu valid code values, while "
             "the expected number of valid code values is %lu.\n",
             (unsigned long) valid_codes_counter,
             (unsigned long) known_valid_codes);
    return 9;
  }

  return 0;
}


/* non-zero error code on error, zero on success */
static int
check_h2_h_decode_all_four_bytes_values (void)
{
  static const uint_fast32_t known_valid_codes = 1395134146u;
  static const uint_fast32_t one_pct_step = 0xFFFFFFFFu / 100u;
  uint_fast32_t valid_codes_counter;
  uint_fast32_t i;

  valid_codes_counter = 0;
  i = 0;
  printf ("Completed: 00%% ");
  fflush (stdout);
  do
  {
    uint8_t test_data[4];
    int res;
    test_data[0] = (uint8_t) i & 0xFFu;
    test_data[1] = (uint8_t) ((i & 0xFF00u) >> 8u);
    test_data[2] = (uint8_t) ((i & 0xFF0000u) >> 16u);
    test_data[3] = (uint8_t) ((i & 0xFF000000u) >> 24u);

    res = perform_single_dec_unknow_seq_check (sizeof(test_data),
                                               test_data,
                                               &valid_codes_counter);

    if (0 != res)
      return res;

    /* Imperfect calculations, but enough for tracking the progress */
    if (0u == (i % one_pct_step))
    {
      printf ("\b\b\b\b%02u%% ", (unsigned int) (i / one_pct_step));
      fflush (stdout);
    }
  } while (0xFFFFFFFFu > i++);
  printf ("\n");

  if (known_valid_codes != valid_codes_counter)
  {
    fflush (stdout);
    fprintf (stderr, "The check found %lu valid code values, while "
             "the expected number of valid code values is %lu.\n",
             (unsigned long) valid_codes_counter,
             (unsigned long) known_valid_codes);
    return 9;
  }

  return 0;
}


/* non-zero error code on error, zero on success */
static int
test_decode (void)
{
  int res;

  mhd_h2_huffman_init ();

  printf ("Checking H2 Huffman decoding with"
          " precise output buffer size...\n");
  res = check_h2_h_decode_perfect_size ();
  if (0 != res)
    return res;
  printf ("Succeed\n");

  printf ("Checking H2 Huffman decoding with"
          " larger output buffer size...\n");
  res = check_h2_h_decode_larger_size ();
  if (0 != res)
    return res;
  printf ("Succeed\n");

  printf ("Checking H2 Huffman decoding with"
          " smaller output buffer size...\n");
  res = check_h2_h_decode_smaller_size ();
  if (0 != res)
    return res;
  printf ("Succeed\n");

  printf ("Checking H2 Huffman decoding with"
          " various fixed output buffer sizes...\n");
  res = check_h2_h_decode_fixed_sizes ();
  if (0 != res)
    return res;
  printf ("Succeed\n");

  printf ("Checking H2 Huffman decoding with"
          " all one byte values...\n");
  res = check_h2_h_decode_all_one_byte_values ();
  if (0 != res)
    return res;
  printf ("Succeed\n");

  printf ("Checking H2 Huffman decoding with"
          " all combinations of two bytes values...\n");
  res = check_h2_h_decode_all_two_bytes_values ();
  if (0 != res)
    return res;
  printf ("Succeed\n");

  printf ("Checking H2 Huffman decoding with"
          " all combinations of three bytes values...\n");
  printf ("This check may take up to a minute.\n");
  res = check_h2_h_decode_all_three_bytes_values ();
  if (0 != res)
    return res;
  printf ("Succeed\n");

  if (enable_deep_tests)
  {
    printf ("Checking H2 Huffman decoding with"
            " all combinations of four bytes values...\n");
    printf ("This check may take between 1 minute and 2 hours.\n");
    res = check_h2_h_decode_all_four_bytes_values ();
    if (0 != res)
      return res;
    printf ("Succeed\n");
  }
  else
  {
    printf ("Skipping checking H2 Huffman decoding with"
            " all combinations of four bytes values as deep tests "
            "are not enabled.\n");
  }

  return 0;
}


int
main (int argc,
      char *const *argv)
{
  if (argc < 1)
    return 99;

  if (! enable_deep_tests)
    enable_deep_tests = mhdt_has_param (argc, argv, "--deep");

  if (0 != basic_data_check ())
    return 99;

  if (mhdt_has_in_name (argv[0], "_decode"))
    return test_decode ();

  return test_encode ();

}
