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
 * @file src/mhd2/h2/hpack/h2_huffman_codec.c
 * @brief  The implementation of HTTP/2 Huffman encoding and decoding functions
 * @author Karlson2k (Evgeny Grin)
 */


#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"
#include <string.h>

#include "mhd_assert.h"

#include "mhd_constexpr.h"
#include "mhd_predict.h"
#include "mhd_align.h"

#include "mhd_bithelpers.h"

#include "h2_huffman_codec.h"

/**
 * @def MHD_USE_CODE_HARDENING
 * Enable additional code-hardening checks.
 *
 * Automatically enabled when unit-testing.
 */
#if defined(MHD_UNIT_TESTING)
#  if ! defined(MHD_USE_CODE_HARDENING)
#    define MHD_USE_CODE_HARDENING      1
#  endif
#endif

/**
 * HTTP/2 static Huffman codes from RFC 7541
 *
 * Index is the character value (0..255).
 * The table values are codes that are left-algined (MSB-aligned) to
 * the 32 bit value.
 * See https://www.rfc-editor.org/rfc/rfc7541.html#appendix-B
 *
 * @note The table does not include EOS code.
 */
mhd_constexpr uint_least32_t mhd_h2huff_code_by_sym[256] = {
  /*     (  0) */ 0xFFC00000u,
  /*     (  1) */ 0xFFFFB000u,
  /*     (  2) */ 0xFFFFFE20u,
  /*     (  3) */ 0xFFFFFE30u,
  /*     (  4) */ 0xFFFFFE40u,
  /*     (  5) */ 0xFFFFFE50u,
  /*     (  6) */ 0xFFFFFE60u,
  /*     (  7) */ 0xFFFFFE70u,
  /*     (  8) */ 0xFFFFFE80u,
  /*     (  9) */ 0xFFFFEA00u,
  /*     ( 10) */ 0xFFFFFFF0u,
  /*     ( 11) */ 0xFFFFFE90u,
  /*     ( 12) */ 0xFFFFFEA0u,
  /*     ( 13) */ 0xFFFFFFF4u,
  /*     ( 14) */ 0xFFFFFEB0u,
  /*     ( 15) */ 0xFFFFFEC0u,
  /*     ( 16) */ 0xFFFFFED0u,
  /*     ( 17) */ 0xFFFFFEE0u,
  /*     ( 18) */ 0xFFFFFEF0u,
  /*     ( 19) */ 0xFFFFFF00u,
  /*     ( 20) */ 0xFFFFFF10u,
  /*     ( 21) */ 0xFFFFFF20u,
  /*     ( 22) */ 0xFFFFFFF8u,
  /*     ( 23) */ 0xFFFFFF30u,
  /*     ( 24) */ 0xFFFFFF40u,
  /*     ( 25) */ 0xFFFFFF50u,
  /*     ( 26) */ 0xFFFFFF60u,
  /*     ( 27) */ 0xFFFFFF70u,
  /*     ( 28) */ 0xFFFFFF80u,
  /*     ( 29) */ 0xFFFFFF90u,
  /*     ( 30) */ 0xFFFFFFA0u,
  /*     ( 31) */ 0xFFFFFFB0u,
  /* ' ' ( 32) */ 0x50000000u,
  /* '!' ( 33) */ 0xFE000000u,
  /* '"' ( 34) */ 0xFE400000u,
  /* '#' ( 35) */ 0xFFA00000u,
  /* '$' ( 36) */ 0xFFC80000u,
  /* '%' ( 37) */ 0x54000000u,
  /* '&' ( 38) */ 0xF8000000u,
  /* ''' ( 39) */ 0xFF400000u,
  /* '(' ( 40) */ 0xFE800000u,
  /* ')' ( 41) */ 0xFEC00000u,
  /* '*' ( 42) */ 0xF9000000u,
  /* '+' ( 43) */ 0xFF600000u,
  /* ',' ( 44) */ 0xFA000000u,
  /* '-' ( 45) */ 0x58000000u,
  /* '.' ( 46) */ 0x5C000000u,
  /* '/' ( 47) */ 0x60000000u,
  /* '0' ( 48) */ 0x00000000u,
  /* '1' ( 49) */ 0x08000000u,
  /* '2' ( 50) */ 0x10000000u,
  /* '3' ( 51) */ 0x64000000u,
  /* '4' ( 52) */ 0x68000000u,
  /* '5' ( 53) */ 0x6C000000u,
  /* '6' ( 54) */ 0x70000000u,
  /* '7' ( 55) */ 0x74000000u,
  /* '8' ( 56) */ 0x78000000u,
  /* '9' ( 57) */ 0x7C000000u,
  /* ':' ( 58) */ 0xB8000000u,
  /* ';' ( 59) */ 0xFB000000u,
  /* '<' ( 60) */ 0xFFF80000u,
  /* '=' ( 61) */ 0x80000000u,
  /* '>' ( 62) */ 0xFFB00000u,
  /* '?' ( 63) */ 0xFF000000u,
  /* '@' ( 64) */ 0xFFD00000u,
  /* 'A' ( 65) */ 0x84000000u,
  /* 'B' ( 66) */ 0xBA000000u,
  /* 'C' ( 67) */ 0xBC000000u,
  /* 'D' ( 68) */ 0xBE000000u,
  /* 'E' ( 69) */ 0xC0000000u,
  /* 'F' ( 70) */ 0xC2000000u,
  /* 'G' ( 71) */ 0xC4000000u,
  /* 'H' ( 72) */ 0xC6000000u,
  /* 'I' ( 73) */ 0xC8000000u,
  /* 'J' ( 74) */ 0xCA000000u,
  /* 'K' ( 75) */ 0xCC000000u,
  /* 'L' ( 76) */ 0xCE000000u,
  /* 'M' ( 77) */ 0xD0000000u,
  /* 'N' ( 78) */ 0xD2000000u,
  /* 'O' ( 79) */ 0xD4000000u,
  /* 'P' ( 80) */ 0xD6000000u,
  /* 'Q' ( 81) */ 0xD8000000u,
  /* 'R' ( 82) */ 0xDA000000u,
  /* 'S' ( 83) */ 0xDC000000u,
  /* 'T' ( 84) */ 0xDE000000u,
  /* 'U' ( 85) */ 0xE0000000u,
  /* 'V' ( 86) */ 0xE2000000u,
  /* 'W' ( 87) */ 0xE4000000u,
  /* 'X' ( 88) */ 0xFC000000u,
  /* 'Y' ( 89) */ 0xE6000000u,
  /* 'Z' ( 90) */ 0xFD000000u,
  /* '[' ( 91) */ 0xFFD80000u,
  /* '\' ( 92) */ 0xFFFE0000u,
  /* ']' ( 93) */ 0xFFE00000u,
  /* '^' ( 94) */ 0xFFF00000u,
  /* '_' ( 95) */ 0x88000000u,
  /* '`' ( 96) */ 0xFFFA0000u,
  /* 'a' ( 97) */ 0x18000000u,
  /* 'b' ( 98) */ 0x8C000000u,
  /* 'c' ( 99) */ 0x20000000u,
  /* 'd' (100) */ 0x90000000u,
  /* 'e' (101) */ 0x28000000u,
  /* 'f' (102) */ 0x94000000u,
  /* 'g' (103) */ 0x98000000u,
  /* 'h' (104) */ 0x9C000000u,
  /* 'i' (105) */ 0x30000000u,
  /* 'j' (106) */ 0xE8000000u,
  /* 'k' (107) */ 0xEA000000u,
  /* 'l' (108) */ 0xA0000000u,
  /* 'm' (109) */ 0xA4000000u,
  /* 'n' (110) */ 0xA8000000u,
  /* 'o' (111) */ 0x38000000u,
  /* 'p' (112) */ 0xAC000000u,
  /* 'q' (113) */ 0xEC000000u,
  /* 'r' (114) */ 0xB0000000u,
  /* 's' (115) */ 0x40000000u,
  /* 't' (116) */ 0x48000000u,
  /* 'u' (117) */ 0xB4000000u,
  /* 'v' (118) */ 0xEE000000u,
  /* 'w' (119) */ 0xF0000000u,
  /* 'x' (120) */ 0xF2000000u,
  /* 'y' (121) */ 0xF4000000u,
  /* 'z' (122) */ 0xF6000000u,
  /* '{' (123) */ 0xFFFC0000u,
  /* '|' (124) */ 0xFF800000u,
  /* '}' (125) */ 0xFFF40000u,
  /* '~' (126) */ 0xFFE80000u,
  /*     (127) */ 0xFFFFFFC0u,
  /*     (128) */ 0xFFFE6000u,
  /*     (129) */ 0xFFFF4800u,
  /*     (130) */ 0xFFFE7000u,
  /*     (131) */ 0xFFFE8000u,
  /*     (132) */ 0xFFFF4C00u,
  /*     (133) */ 0xFFFF5000u,
  /*     (134) */ 0xFFFF5400u,
  /*     (135) */ 0xFFFFB200u,
  /*     (136) */ 0xFFFF5800u,
  /*     (137) */ 0xFFFFB400u,
  /*     (138) */ 0xFFFFB600u,
  /*     (139) */ 0xFFFFB800u,
  /*     (140) */ 0xFFFFBA00u,
  /*     (141) */ 0xFFFFBC00u,
  /*     (142) */ 0xFFFFEB00u,
  /*     (143) */ 0xFFFFBE00u,
  /*     (144) */ 0xFFFFEC00u,
  /*     (145) */ 0xFFFFED00u,
  /*     (146) */ 0xFFFF5C00u,
  /*     (147) */ 0xFFFFC000u,
  /*     (148) */ 0xFFFFEE00u,
  /*     (149) */ 0xFFFFC200u,
  /*     (150) */ 0xFFFFC400u,
  /*     (151) */ 0xFFFFC600u,
  /*     (152) */ 0xFFFFC800u,
  /*     (153) */ 0xFFFEE000u,
  /*     (154) */ 0xFFFF6000u,
  /*     (155) */ 0xFFFFCA00u,
  /*     (156) */ 0xFFFF6400u,
  /*     (157) */ 0xFFFFCC00u,
  /*     (158) */ 0xFFFFCE00u,
  /*     (159) */ 0xFFFFEF00u,
  /*     (160) */ 0xFFFF6800u,
  /*     (161) */ 0xFFFEE800u,
  /*     (162) */ 0xFFFE9000u,
  /*     (163) */ 0xFFFF6C00u,
  /*     (164) */ 0xFFFF7000u,
  /*     (165) */ 0xFFFFD000u,
  /*     (166) */ 0xFFFFD200u,
  /*     (167) */ 0xFFFEF000u,
  /*     (168) */ 0xFFFFD400u,
  /*     (169) */ 0xFFFF7400u,
  /*     (170) */ 0xFFFF7800u,
  /*     (171) */ 0xFFFFF000u,
  /*     (172) */ 0xFFFEF800u,
  /*     (173) */ 0xFFFF7C00u,
  /*     (174) */ 0xFFFFD600u,
  /*     (175) */ 0xFFFFD800u,
  /*     (176) */ 0xFFFF0000u,
  /*     (177) */ 0xFFFF0800u,
  /*     (178) */ 0xFFFF8000u,
  /*     (179) */ 0xFFFF1000u,
  /*     (180) */ 0xFFFFDA00u,
  /*     (181) */ 0xFFFF8400u,
  /*     (182) */ 0xFFFFDC00u,
  /*     (183) */ 0xFFFFDE00u,
  /*     (184) */ 0xFFFEA000u,
  /*     (185) */ 0xFFFF8800u,
  /*     (186) */ 0xFFFF8C00u,
  /*     (187) */ 0xFFFF9000u,
  /*     (188) */ 0xFFFFE000u,
  /*     (189) */ 0xFFFF9400u,
  /*     (190) */ 0xFFFF9800u,
  /*     (191) */ 0xFFFFE200u,
  /*     (192) */ 0xFFFFF800u,
  /*     (193) */ 0xFFFFF840u,
  /*     (194) */ 0xFFFEB000u,
  /*     (195) */ 0xFFFE2000u,
  /*     (196) */ 0xFFFF9C00u,
  /*     (197) */ 0xFFFFE400u,
  /*     (198) */ 0xFFFFA000u,
  /*     (199) */ 0xFFFFF600u,
  /*     (200) */ 0xFFFFF880u,
  /*     (201) */ 0xFFFFF8C0u,
  /*     (202) */ 0xFFFFF900u,
  /*     (203) */ 0xFFFFFBC0u,
  /*     (204) */ 0xFFFFFBE0u,
  /*     (205) */ 0xFFFFF940u,
  /*     (206) */ 0xFFFFF100u,
  /*     (207) */ 0xFFFFF680u,
  /*     (208) */ 0xFFFE4000u,
  /*     (209) */ 0xFFFF1800u,
  /*     (210) */ 0xFFFFF980u,
  /*     (211) */ 0xFFFFFC00u,
  /*     (212) */ 0xFFFFFC20u,
  /*     (213) */ 0xFFFFF9C0u,
  /*     (214) */ 0xFFFFFC40u,
  /*     (215) */ 0xFFFFF200u,
  /*     (216) */ 0xFFFF2000u,
  /*     (217) */ 0xFFFF2800u,
  /*     (218) */ 0xFFFFFA00u,
  /*     (219) */ 0xFFFFFA40u,
  /*     (220) */ 0xFFFFFFD0u,
  /*     (221) */ 0xFFFFFC60u,
  /*     (222) */ 0xFFFFFC80u,
  /*     (223) */ 0xFFFFFCA0u,
  /*     (224) */ 0xFFFEC000u,
  /*     (225) */ 0xFFFFF300u,
  /*     (226) */ 0xFFFED000u,
  /*     (227) */ 0xFFFF3000u,
  /*     (228) */ 0xFFFFA400u,
  /*     (229) */ 0xFFFF3800u,
  /*     (230) */ 0xFFFF4000u,
  /*     (231) */ 0xFFFFE600u,
  /*     (232) */ 0xFFFFA800u,
  /*     (233) */ 0xFFFFAC00u,
  /*     (234) */ 0xFFFFF700u,
  /*     (235) */ 0xFFFFF780u,
  /*     (236) */ 0xFFFFF400u,
  /*     (237) */ 0xFFFFF500u,
  /*     (238) */ 0xFFFFFA80u,
  /*     (239) */ 0xFFFFE800u,
  /*     (240) */ 0xFFFFFAC0u,
  /*     (241) */ 0xFFFFFCC0u,
  /*     (242) */ 0xFFFFFB00u,
  /*     (243) */ 0xFFFFFB40u,
  /*     (244) */ 0xFFFFFCE0u,
  /*     (245) */ 0xFFFFFD00u,
  /*     (246) */ 0xFFFFFD20u,
  /*     (247) */ 0xFFFFFD40u,
  /*     (248) */ 0xFFFFFD60u,
  /*     (249) */ 0xFFFFFFE0u,
  /*     (250) */ 0xFFFFFD80u,
  /*     (251) */ 0xFFFFFDA0u,
  /*     (252) */ 0xFFFFFDC0u,
  /*     (253) */ 0xFFFFFDE0u,
  /*     (254) */ 0xFFFFFE00u,
  /*     (255) */ 0xFFFFFB80u
};


/**
 * HTTP/2 static Huffman codes length in bits from RFC 7541
 * See https://www.rfc-editor.org/rfc/rfc7541.html#appendix-B
 *
 * Index is the character value (0..255).
 * The table values are lengths of the code in bits.
 * See https://www.rfc-editor.org/rfc/rfc7541.html#appendix-B
 *
 * @note The table does not include EOS code.
 */
mhd_constexpr uint8_t mhd_h2huff_bitlen_by_sym[256] = {
  /*     (  0) */ 13u,
  /*     (  1) */ 23u,
  /*     (  2) */ 28u,
  /*     (  3) */ 28u,
  /*     (  4) */ 28u,
  /*     (  5) */ 28u,
  /*     (  6) */ 28u,
  /*     (  7) */ 28u,
  /*     (  8) */ 28u,
  /*     (  9) */ 24u,
  /*     ( 10) */ 30u,
  /*     ( 11) */ 28u,
  /*     ( 12) */ 28u,
  /*     ( 13) */ 30u,
  /*     ( 14) */ 28u,
  /*     ( 15) */ 28u,
  /*     ( 16) */ 28u,
  /*     ( 17) */ 28u,
  /*     ( 18) */ 28u,
  /*     ( 19) */ 28u,
  /*     ( 20) */ 28u,
  /*     ( 21) */ 28u,
  /*     ( 22) */ 30u,
  /*     ( 23) */ 28u,
  /*     ( 24) */ 28u,
  /*     ( 25) */ 28u,
  /*     ( 26) */ 28u,
  /*     ( 27) */ 28u,
  /*     ( 28) */ 28u,
  /*     ( 29) */ 28u,
  /*     ( 30) */ 28u,
  /*     ( 31) */ 28u,
  /* ' ' ( 32) */  6u,
  /* '!' ( 33) */ 10u,
  /* '"' ( 34) */ 10u,
  /* '#' ( 35) */ 12u,
  /* '$' ( 36) */ 13u,
  /* '%' ( 37) */  6u,
  /* '&' ( 38) */  8u,
  /* ''' ( 39) */ 11u,
  /* '(' ( 40) */ 10u,
  /* ')' ( 41) */ 10u,
  /* '*' ( 42) */  8u,
  /* '+' ( 43) */ 11u,
  /* ',' ( 44) */  8u,
  /* '-' ( 45) */  6u,
  /* '.' ( 46) */  6u,
  /* '/' ( 47) */  6u,
  /* '0' ( 48) */  5u,
  /* '1' ( 49) */  5u,
  /* '2' ( 50) */  5u,
  /* '3' ( 51) */  6u,
  /* '4' ( 52) */  6u,
  /* '5' ( 53) */  6u,
  /* '6' ( 54) */  6u,
  /* '7' ( 55) */  6u,
  /* '8' ( 56) */  6u,
  /* '9' ( 57) */  6u,
  /* ':' ( 58) */  7u,
  /* ';' ( 59) */  8u,
  /* '<' ( 60) */ 15u,
  /* '=' ( 61) */  6u,
  /* '>' ( 62) */ 12u,
  /* '?' ( 63) */ 10u,
  /* '@' ( 64) */ 13u,
  /* 'A' ( 65) */  6u,
  /* 'B' ( 66) */  7u,
  /* 'C' ( 67) */  7u,
  /* 'D' ( 68) */  7u,
  /* 'E' ( 69) */  7u,
  /* 'F' ( 70) */  7u,
  /* 'G' ( 71) */  7u,
  /* 'H' ( 72) */  7u,
  /* 'I' ( 73) */  7u,
  /* 'J' ( 74) */  7u,
  /* 'K' ( 75) */  7u,
  /* 'L' ( 76) */  7u,
  /* 'M' ( 77) */  7u,
  /* 'N' ( 78) */  7u,
  /* 'O' ( 79) */  7u,
  /* 'P' ( 80) */  7u,
  /* 'Q' ( 81) */  7u,
  /* 'R' ( 82) */  7u,
  /* 'S' ( 83) */  7u,
  /* 'T' ( 84) */  7u,
  /* 'U' ( 85) */  7u,
  /* 'V' ( 86) */  7u,
  /* 'W' ( 87) */  7u,
  /* 'X' ( 88) */  8u,
  /* 'Y' ( 89) */  7u,
  /* 'Z' ( 90) */  8u,
  /* '[' ( 91) */ 13u,
  /* '\' ( 92) */ 19u,
  /* ']' ( 93) */ 13u,
  /* '^' ( 94) */ 14u,
  /* '_' ( 95) */  6u,
  /* '`' ( 96) */ 15u,
  /* 'a' ( 97) */  5u,
  /* 'b' ( 98) */  6u,
  /* 'c' ( 99) */  5u,
  /* 'd' (100) */  6u,
  /* 'e' (101) */  5u,
  /* 'f' (102) */  6u,
  /* 'g' (103) */  6u,
  /* 'h' (104) */  6u,
  /* 'i' (105) */  5u,
  /* 'j' (106) */  7u,
  /* 'k' (107) */  7u,
  /* 'l' (108) */  6u,
  /* 'm' (109) */  6u,
  /* 'n' (110) */  6u,
  /* 'o' (111) */  5u,
  /* 'p' (112) */  6u,
  /* 'q' (113) */  7u,
  /* 'r' (114) */  6u,
  /* 's' (115) */  5u,
  /* 't' (116) */  5u,
  /* 'u' (117) */  6u,
  /* 'v' (118) */  7u,
  /* 'w' (119) */  7u,
  /* 'x' (120) */  7u,
  /* 'y' (121) */  7u,
  /* 'z' (122) */  7u,
  /* '{' (123) */ 15u,
  /* '|' (124) */ 11u,
  /* '}' (125) */ 14u,
  /* '~' (126) */ 13u,
  /*     (127) */ 28u,
  /*     (128) */ 20u,
  /*     (129) */ 22u,
  /*     (130) */ 20u,
  /*     (131) */ 20u,
  /*     (132) */ 22u,
  /*     (133) */ 22u,
  /*     (134) */ 22u,
  /*     (135) */ 23u,
  /*     (136) */ 22u,
  /*     (137) */ 23u,
  /*     (138) */ 23u,
  /*     (139) */ 23u,
  /*     (140) */ 23u,
  /*     (141) */ 23u,
  /*     (142) */ 24u,
  /*     (143) */ 23u,
  /*     (144) */ 24u,
  /*     (145) */ 24u,
  /*     (146) */ 22u,
  /*     (147) */ 23u,
  /*     (148) */ 24u,
  /*     (149) */ 23u,
  /*     (150) */ 23u,
  /*     (151) */ 23u,
  /*     (152) */ 23u,
  /*     (153) */ 21u,
  /*     (154) */ 22u,
  /*     (155) */ 23u,
  /*     (156) */ 22u,
  /*     (157) */ 23u,
  /*     (158) */ 23u,
  /*     (159) */ 24u,
  /*     (160) */ 22u,
  /*     (161) */ 21u,
  /*     (162) */ 20u,
  /*     (163) */ 22u,
  /*     (164) */ 22u,
  /*     (165) */ 23u,
  /*     (166) */ 23u,
  /*     (167) */ 21u,
  /*     (168) */ 23u,
  /*     (169) */ 22u,
  /*     (170) */ 22u,
  /*     (171) */ 24u,
  /*     (172) */ 21u,
  /*     (173) */ 22u,
  /*     (174) */ 23u,
  /*     (175) */ 23u,
  /*     (176) */ 21u,
  /*     (177) */ 21u,
  /*     (178) */ 22u,
  /*     (179) */ 21u,
  /*     (180) */ 23u,
  /*     (181) */ 22u,
  /*     (182) */ 23u,
  /*     (183) */ 23u,
  /*     (184) */ 20u,
  /*     (185) */ 22u,
  /*     (186) */ 22u,
  /*     (187) */ 22u,
  /*     (188) */ 23u,
  /*     (189) */ 22u,
  /*     (190) */ 22u,
  /*     (191) */ 23u,
  /*     (192) */ 26u,
  /*     (193) */ 26u,
  /*     (194) */ 20u,
  /*     (195) */ 19u,
  /*     (196) */ 22u,
  /*     (197) */ 23u,
  /*     (198) */ 22u,
  /*     (199) */ 25u,
  /*     (200) */ 26u,
  /*     (201) */ 26u,
  /*     (202) */ 26u,
  /*     (203) */ 27u,
  /*     (204) */ 27u,
  /*     (205) */ 26u,
  /*     (206) */ 24u,
  /*     (207) */ 25u,
  /*     (208) */ 19u,
  /*     (209) */ 21u,
  /*     (210) */ 26u,
  /*     (211) */ 27u,
  /*     (212) */ 27u,
  /*     (213) */ 26u,
  /*     (214) */ 27u,
  /*     (215) */ 24u,
  /*     (216) */ 21u,
  /*     (217) */ 21u,
  /*     (218) */ 26u,
  /*     (219) */ 26u,
  /*     (220) */ 28u,
  /*     (221) */ 27u,
  /*     (222) */ 27u,
  /*     (223) */ 27u,
  /*     (224) */ 20u,
  /*     (225) */ 24u,
  /*     (226) */ 20u,
  /*     (227) */ 21u,
  /*     (228) */ 22u,
  /*     (229) */ 21u,
  /*     (230) */ 21u,
  /*     (231) */ 23u,
  /*     (232) */ 22u,
  /*     (233) */ 22u,
  /*     (234) */ 25u,
  /*     (235) */ 25u,
  /*     (236) */ 24u,
  /*     (237) */ 24u,
  /*     (238) */ 26u,
  /*     (239) */ 23u,
  /*     (240) */ 26u,
  /*     (241) */ 27u,
  /*     (242) */ 26u,
  /*     (243) */ 26u,
  /*     (244) */ 27u,
  /*     (245) */ 27u,
  /*     (246) */ 27u,
  /*     (247) */ 27u,
  /*     (248) */ 27u,
  /*     (249) */ 28u,
  /*     (250) */ 27u,
  /*     (251) */ 27u,
  /*     (252) */ 27u,
  /*     (253) */ 27u,
  /*     (254) */ 27u,
  /*     (255) */ 26u
};

/**
 * The symbol with the longest H2 Huffman code
 */
#define mhd_H2HUFF_LONGEST_CODE_SYM 22u

/**
 * Get MSB-aligned 32-bit code and widen to @c uint_fast32_t, keeping it
 * MSB-aligned on the wider type.
 * @param idx Symbol (0..255)
 */
#define mhd_GET_H2HUFF_CODE_AS_UINTFAST32_ALG(idx) \
        (((uint_fast32_t) mhd_h2huff_code_by_sym[(idx)]) << \
          ((sizeof(uint_fast32_t) - 4u) * 8u))


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (2,1) MHD_FN_PAR_OUT_SIZE_ (4,3) size_t
mhd_h2_huffman_encode (size_t str_len,
                       const char *restrict str,
                       size_t out_buf_size,
                       void *restrict out_buf)
{
  mhd_constexpr uint_fast32_t uif32_all_ones =
    (uint_fast32_t) (~((uint_fast32_t) 0u));
  /** The width of the @a acc_out. */
  mhd_constexpr uint_fast8_t acc_out_bits_width =
    (uint_fast8_t) (sizeof(uint_fast32_t) * 8u);
  uint8_t *const out_b = (uint8_t *) out_buf;
  /** 32-bit (or wider) output accumulator.
      Filled completely with encoded data and then the data is pushed to the
      output buffer. */
  uint_fast32_t acc_out;
  /** The number of MSB bits currently used in the @a acc_out */
  uint_fast8_t acc_out_used_bits;
  size_t str_idx;
  size_t out_idx;

#ifndef MHD_UNIT_TESTING
  /* DO check the input for validity if NOT unit-testing */
  mhd_assert (0 != str_len);
  mhd_assert (0 != out_buf_size);
#endif /* MHD_UNIT_TESTING */
#ifdef BUILDING_MHD_LIB
  mhd_assert (out_buf_size <= str_len);
#endif /* ! BUILDING_MHD_LIB */
  mhd_assert (out_buf_size <= ((~((size_t) (0))) - sizeof(acc_out)));

  for (str_idx = 0u, out_idx = 0u,
       acc_out = 0u, acc_out_used_bits = 0u; str_idx < str_len; ++str_idx)
  {
    const uint_fast8_t sym = (uint_fast8_t) (unsigned char) str[str_idx];
    const uint_fast32_t sym_code =
      mhd_GET_H2HUFF_CODE_AS_UINTFAST32_ALG (sym);
    const uint_fast8_t sym_code_bits =
      (uint_fast8_t) mhd_h2huff_bitlen_by_sym[sym];

    mhd_assert (acc_out_bits_width > acc_out_used_bits);
    mhd_assert (0 == (acc_out & (uif32_all_ones >> acc_out_used_bits)));
    mhd_assert (5u <= sym_code_bits);
    mhd_assert (30u >= sym_code_bits);
    mhd_assert (0 == (sym_code & (uif32_all_ones >> sym_code_bits)));

    acc_out |= (sym_code >> acc_out_used_bits);
    acc_out_used_bits += sym_code_bits;

    if (mhd_COND_PRED_FALSE_P (acc_out_used_bits >= acc_out_bits_width, 0.81))
    { /* The accumulator 'acc_out' must be written to the output buffer. */
      uint_fast32_t data_out;
      size_t next_out_idx;
      /* Number of bits in the 'sym_code' that does not fit into 'acc_out' */
      const uint_fast8_t sym_code_bits_left =
        acc_out_used_bits - acc_out_bits_width;
      /* The number of bits from 'sym_code' to be flushed to the output buffer */
      const uint_fast8_t sym_code_bits_to_flush =
        sym_code_bits - sym_code_bits_left;

      mhd_assert (29 >= sym_code_bits_left);

      next_out_idx = out_idx + sizeof(acc_out);
      if (mhd_COND_HARDLY_EVER (sizeof(acc_out) > next_out_idx))
        return 0; /* 'out_idx' overflow */
      if (mhd_COND_ALMOST_NEVER (out_buf_size < next_out_idx))
        return 0; /* Not enough space in the "out_buf" */

      mhd_PUT_UINTFAST32_BE (&data_out, acc_out);
      memcpy (out_b + out_idx, &data_out, sizeof(data_out));
      out_idx = next_out_idx;

      mhd_assert (0 != sym_code_bits_to_flush);
      /* The shift performed in two steps to avoid branches and
         undefined behaviour */
      mhd_assert (sym_code_bits >= sym_code_bits_to_flush);
      acc_out = (sym_code << 1u) << (sym_code_bits_to_flush - 1u);
      mhd_assert (0 == (acc_out & (uif32_all_ones >> sym_code_bits_left)));
      acc_out_used_bits = sym_code_bits_left;
    }
  }
  mhd_assert (acc_out_bits_width > acc_out_used_bits);
  mhd_assert (0 == (acc_out & (uif32_all_ones >> acc_out_used_bits)));

  if (mhd_COND_PRED_TRUE_P (0 != acc_out_used_bits, \
                            (sizeof(uint_fast32_t) - 1.0) \
                            / ((double) sizeof(uint_fast32_t))))
  { /* The accumulator 'acc_out' must be flushed to the output buffer. */
    const uint_fast8_t tail_bytes =
      (uint_fast8_t) (acc_out_used_bits + 7u) >> 3u;
    size_t next_out_idx;
    uint_fast32_t data_out;

    mhd_assert (sizeof(acc_out) >= tail_bytes);
    mhd_assert (0 != tail_bytes);

    next_out_idx = out_idx + tail_bytes;
    if (mhd_COND_HARDLY_EVER (tail_bytes > next_out_idx))
      return 0; /* 'out_idx' overflow */
    if (mhd_COND_ALMOST_NEVER (out_buf_size < next_out_idx))
      return 0; /* Not enough space in the "out_buf" */

    acc_out |= uif32_all_ones >> acc_out_used_bits;
    mhd_PUT_UINTFAST32_BE (&data_out, acc_out);
    memcpy (out_b + out_idx, &data_out, tail_bytes);
    out_idx = next_out_idx;
  }

  mhd_assert (out_buf_size >= out_idx);

  return out_idx;
}


#ifndef NDEBUG
/**
 * Track decoding table initialisation.
 */
static bool h2huff_decode_inited = false;
#endif

/**
 * @def MHD_USE_HUFFMAN_DECODE_TABLES_TAIL_GAP
 * Omit the unreachable tail part of some decode tables to save memory.
 *
 * When enabled, a decode table may be allocated with a shortened size that
 * excludes entries that are never addressed by valid codes (given the
 * prefix/next-prefix boundaries).
 *
 * If not defined then enabled for code hardening and for debug builds.
 */
#ifndef MHD_USE_HUFFMAN_DECODE_TABLES_TAIL_GAP
#  if defined(MHD_USE_CODE_HARDENING) || ! defined(NDEBUG)
#    define MHD_USE_HUFFMAN_DECODE_TABLES_TAIL_GAP 1
#  else
#    define MHD_USE_HUFFMAN_DECODE_TABLES_TAIL_GAP 0
#  endif
#endif


/**
 * @def HUFF_DTBL_SIZE_ROUNDED
 * Full table size for a given index width @a twidth (no tail gap).
 */
#define HUFF_DTBL_SIZE_ROUNDED(twidth) (1u << (twidth))

/**
 * @def HUFF_DTBL_ENTRIES
 * Number of actually used entries for a table with prefix width
 * @a pwidth, table index width @a twidth, and the next table prefix width
 * @a next_pwidth.
 *
 * This corresponds to the addressable range of codes for that table,
 * excluding the part that belongs to the next table deeper in the chain.
 */
#define HUFF_DTBL_ENTRIES(pwidth,twidth,next_pwidth) \
        (HUFF_DTBL_SIZE_ROUNDED (twidth) \
         - (1u << ((pwidth) + (twidth) - (next_pwidth))) )

/**
 * @def HUFF_DTBL_SIZE
 * Actual allocation size for the decode table (may include extra space).
 */
#if MHD_USE_HUFFMAN_DECODE_TABLES_TAIL_GAP
#  define HUFF_DTBL_SIZE(pwidth,twidth,next_pwidth) \
        HUFF_DTBL_ENTRIES (pwidth,twidth,next_pwidth)
#else
#  define HUFF_DTBL_SIZE(pwidth,twidth,next_pwidth) \
        HUFF_DTBL_SIZE_ROUNDED (twidth)
#endif


/* Decode table for prefix 0 bits, table width 8 bits.
   Codes:
   '0' ( 48)  00000xxx                                  [ 5]
   '1' ( 49)  00001xxx                                  [ 5]
   '2' ( 50)  00010xxx                                  [ 5]
    ...
    ...
   ';' ( 59)  11111011                                  [ 8]
   'X' ( 88)  11111100                                  [ 8]
   'Z' ( 90)  11111101                                  [ 8]
 */
mhd_ALIGNED_64 static
uint_least16_t mhd_h2huff_SL_by_code_0p8c[HUFF_DTBL_SIZE (0, 8, 7)];

/* Decode table for prefix 7 bits, table width 6 bits.
   Codes:
   '!' ( 33)  1111111 000xxx                            [10]
   '"' ( 34)  1111111 001xxx                            [10]
   '(' ( 40)  1111111 010xxx                            [10]
    ...
    ...
   '[' ( 91)  1111111 111011                            [13]
   ']' ( 93)  1111111 111100                            [13]
   '~' (126)  1111111 111101                            [13]
 */
mhd_ALIGNED_64 static
uint_least16_t mhd_h2huff_SL_by_code_7p6c[HUFF_DTBL_SIZE (7, 6, 12)];

/* Decode table for prefix 12 bits, table width 3 bits.
   Codes:
   '^' ( 94)  111111111111 00x                          [14]
   '}' (125)  111111111111 01x                          [14]
   '<' ( 60)  111111111111 100                          [15]
   '`' ( 96)  111111111111 101                          [15]
   '{' (123)  111111111111 110                          [15]
 */
mhd_ALIGNED_8 static
uint_least16_t mhd_h2huff_SL_by_code_12p3c[HUFF_DTBL_SIZE (12,3,15)];

/* Decode table for prefix 15 bits, table width 7 bits.
   Codes:
   '\' ( 92)  111111111111111 0000xxx                   [19]
       (195)  111111111111111 0001xxx                   [19]
       (208)  111111111111111 0010xxx                   [19]
    ...
    ...
       (169)  111111111111111 1011101                   [22]
       (170)  111111111111111 1011110                   [22]
       (173)  111111111111111 1011111                   [22]
 */
mhd_ALIGNED_16 static
uint_least16_t mhd_h2huff_SL_by_code_15p7c[HUFF_DTBL_SIZE (15,7,17)];

/* Decode table for prefix 17 bits, table width 6 bits.
   Codes:
       (188)  1111111111111111111 0000xx                [23]
       (191)  1111111111111111111 0001xx                [23]
       (197)  1111111111111111111 0010xx                [23]
    ...
    ...
       (207)  1111111111111111111 101101                [25]
       (234)  1111111111111111111 101110                [25]
       (235)  1111111111111111111 101111                [25]
 */
mhd_ALIGNED_16 static
uint_least16_t mhd_h2huff_SL_by_code_17p6c[HUFF_DTBL_SIZE (17,6,19)];

/* Decode table for prefix 19 bits, table width 6 bits.
   Codes:
       (192)  111111111111111111111 00000x              [26]
       (193)  111111111111111111111 00001x              [26]
       (200)  111111111111111111111 00010x              [26]
    ...
    ...
       (251)  111111111111111111111 101101              [27]
       (252)  111111111111111111111 101110              [27]
       (253)  111111111111111111111 101111              [27]
 */
mhd_ALIGNED_16 static
uint_least16_t mhd_h2huff_SL_by_code_19p6c[HUFF_DTBL_SIZE (19,6,21)];

/* Decode table for prefix 21 bits, table width 6 bits.
   Codes:
       (192)  111111111111111111111 00000x              [26]
       (193)  111111111111111111111 00001x              [26]
       (200)  111111111111111111111 00010x              [26]
    ...
    ...
       (251)  111111111111111111111 101101              [27]
       (252)  111111111111111111111 101110              [27]
       (253)  111111111111111111111 101111              [27]
 */
mhd_ALIGNED_16 static
uint_least16_t mhd_h2huff_SL_by_code_21p6c[HUFF_DTBL_SIZE (21,6,23)];

/* Decode table for prefix 23 bits, table width 5 bits.
   Codes:
       (254)  11111111111111111111111 0000x             [27]
       (  2)  11111111111111111111111 00010             [28]
       (  3)  11111111111111111111111 00011             [28]
    ...
    ...
       (127)  11111111111111111111111 11100             [28]
       (220)  11111111111111111111111 11101             [28]
       (249)  11111111111111111111111 11110             [28]
 */
mhd_ALIGNED_16 static
uint_least16_t mhd_h2huff_SL_by_code_23p5c[HUFF_DTBL_SIZE (23,5,28)];

/* Decode table for prefix 28 bits, table width 2 bits.
   Codes:
       ( 10)  1111111111111111111111111111 00           [30]
       ( 13)  1111111111111111111111111111 01           [30]
       ( 22)  1111111111111111111111111111 10           [30]
 */
mhd_ALIGNED_8 static
uint_least16_t mhd_h2huff_SL_by_code_28p2c[HUFF_DTBL_SIZE (28,2,30)];


/**
 * Populate entries for a single symbol within a particular decode table.
 *
 * A decode table covers a window of codes that share a fixed @a pbits prefix
 * (all-ones in MSB-aligned representation) and differ in up to @a tbits
 * following bits. This writes the appropriate range of entries for
 * a particular @a code / @a codebits pair into @a table_to_fill.
 *
 * @param sym      the symbol (0..255)
 * @param code     the MSB-aligned 32-bit code
 * @param codebits the @a code length in bits (5..30)
 * @param pbits    the prefix width in bits (all-one MSB prefix)
 * @param tbits    the table-index width in bits
 * @param next_pbits the next table prefix width
 * @param[out] table_to_fill the pointer to the target decode table.
 */
MHD_FN_PAR_INOUT_ (7) MHD_FN_PAR_NONNULL_ALL_ static inline void
init_decode_table_entries_per_sym (
  const uint_fast8_t sym,
  const uint_least32_t code,
  const uint_fast8_t codebits,
  const uint_fast8_t pbits,
  const uint_fast8_t tbits,
  const uint_fast8_t next_pbits,
  uint_least16_t table_to_fill[
    MHD_FN_PAR_DYN_ARR_SIZE_ (HUFF_DTBL_SIZE (pbits,tbits,next_pbits))])
{
  mhd_constexpr uint_least32_t b32ones = 0xFFFFFFFFu; /**< 32 all-ones bits */
  const uint_least16_t sym_and_blen =
    (uint_least16_t) (((uint_least16_t) codebits) << 8u | sym);
  uint_least32_t i_tbl;
  uint_least32_t i_tbl_end;

#ifdef NDEBUG
  (void) next_pbits; /* Mute compiler warning */
#endif /* NDEBUG */

  mhd_assert ((codebits <= 30u) && "The longest code is 30 bits");
  mhd_assert ((pbits < codebits) && \
              "The prefix must not be wider than the code");
  mhd_assert ((pbits + tbits) >= codebits && "The width of the code cannot " \
              "be larger than the total width of the prefix plus the " \
              "width of the table");
  mhd_assert (code == (code & b32ones) && \
              "The code cannot have bits higher than 32 bits");
  mhd_assert (0u == (code & (b32ones >> codebits)) && \
              "The unused part of the code must be zero");
  mhd_assert ((0u == pbits) || \
              ((b32ones >> (32u - pbits)) == (code >> (32u - pbits)) && \
               "The prefix bits in the code must be all set"));
  mhd_assert ((b32ones >> (32u - codebits)) != (code >> (32u - codebits)) && \
              "The bits in the code after the prefix cannot be all set");

  i_tbl = ((code & (b32ones >> pbits)) >> (32u - (pbits + tbits)));
  mhd_assert (HUFF_DTBL_ENTRIES (pbits, tbits, next_pbits) > i_tbl);
  i_tbl_end = i_tbl + (1u << ((pbits + tbits) - codebits)) - 1u;
  mhd_assert (i_tbl <= i_tbl_end);
  mhd_assert (HUFF_DTBL_ENTRIES (pbits, tbits, next_pbits) > i_tbl_end);

  do
  {
    mhd_assert (HUFF_DTBL_ENTRIES (pbits, tbits, next_pbits) > i_tbl);
    mhd_assert (0 == table_to_fill[i_tbl] && \
                "No entry must be written twice");
    table_to_fill[i_tbl] = sym_and_blen;
  } while (i_tbl++ < i_tbl_end);
}


/**
 * Build one decode table for a particular (prefix, width, next-prefix) triple.
 *
 * @param pbits the number of bits in prefix, i.e. the static part common
 *              to all codes in the table (all-one bits)
 * @param tbits the width of the table index in bits, i.e. the variable part
 *              of the code
 * @param next_pbits the next table prefix width
 * @param[out] table_to_fill the pointer to the table to fill
 */
MHD_FN_PAR_INOUT_ (4) MHD_FN_PAR_NONNULL_ALL_ static void
init_decode_table (
  const uint_fast8_t pbits,
  const uint_fast8_t tbits,
  const uint_fast8_t next_pbits,
  uint_least16_t table_to_fill[
    MHD_FN_PAR_DYN_ARR_SIZE_ (HUFF_DTBL_SIZE (pbits,tbits,next_pbits))])
{
  mhd_constexpr uint_least32_t b32ones = 0xFFFFFFFFu; /**< 32 all-ones bits */
  uint_fast8_t sym;

  mhd_assert (30 >= pbits + tbits);
  mhd_assert (5 <= pbits + tbits);
  mhd_assert (next_pbits > pbits);
  mhd_assert (next_pbits <= (pbits + tbits));

  memset (table_to_fill,
          0,
          HUFF_DTBL_SIZE (pbits,tbits,next_pbits) * sizeof(*table_to_fill)); /* Zero the table completely, including unused entries */

  sym = 0;
  do
  {
    uint_least32_t code = mhd_h2huff_code_by_sym[sym];
    const uint_fast8_t codebits = mhd_h2huff_bitlen_by_sym[sym];

    mhd_assert ((code <= mhd_h2huff_code_by_sym[mhd_H2HUFF_LONGEST_CODE_SYM]) \
                && "Huffman code must be less than or equal to the largest " \
                "Huffman code in the table.");
    mhd_assert (codebits <= 30);
    mhd_assert (0 == (code & (b32ones >> codebits)) && \
                "The unused part of the code must be zero");

    if ((0 != pbits) &&
        (((b32ones << (32u - pbits)) & b32ones) > code))
      continue;

    if (((b32ones << (32u - next_pbits)) & b32ones) <= code)
      continue;

    mhd_assert ((pbits + tbits) >= codebits);

    init_decode_table_entries_per_sym (sym,
                                       code,
                                       codebits,
                                       pbits,
                                       tbits,
                                       next_pbits,
                                       table_to_fill);
  } while (255 > sym++);

#ifndef NDEBUG
  if (1)
  { /* Check the table */
    uint_least32_t i;
    for (i = 0;
         i < HUFF_DTBL_ENTRIES (pbits,tbits,next_pbits);
         ++i)
      mhd_assert ((0u != table_to_fill[i]) && \
                  "All valid entries must be used");
#  if MHD_USE_HUFFMAN_DECODE_TABLES_TAIL_GAP
    for (i = HUFF_DTBL_ENTRIES (pbits,tbits,next_pbits);
         i < HUFF_DTBL_SIZE (pbits,tbits,next_pbits);
         ++i)
      mhd_assert ((0u == table_to_fill[i]) && \
                  "All unused entries must be zero");
#  endif /* MHD_USE_HUFFMAN_DECODE_TABLES_TAIL_GAP */
  }
#endif /* ! NDEBUG */
}


MHD_INTERNAL void
mhd_h2_huffman_init (void)
{
  init_decode_table (0,  8, 7,  mhd_h2huff_SL_by_code_0p8c);
  init_decode_table (7,  6, 12, mhd_h2huff_SL_by_code_7p6c);
  init_decode_table (12, 3, 15, mhd_h2huff_SL_by_code_12p3c);
  init_decode_table (15, 7, 17, mhd_h2huff_SL_by_code_15p7c);
  init_decode_table (17, 6, 19, mhd_h2huff_SL_by_code_17p6c);
  init_decode_table (19, 6, 21, mhd_h2huff_SL_by_code_19p6c);
  init_decode_table (21, 6, 23, mhd_h2huff_SL_by_code_21p6c);
  init_decode_table (23, 5, 28, mhd_h2huff_SL_by_code_23p5c);
  init_decode_table (28, 2, 30, mhd_h2huff_SL_by_code_28p2c);

#ifndef NDEBUG
  h2huff_decode_inited = true;
#endif
}


/*
 * Additional decoding optimisation ideas.
 * Decode by two symbols at once (limited subset of the most used symbols).
 * To keep the decoding table small, decode only 5-7 bits codes, put symbols
 * value to the 'uint16_t' value,  where 14 bits encode symbols value (all
 * symbols have high bits zero), zero value means missing second symbol;
 * top two bits encode length of the code pair: 10-13 bits length if first
 * code bit is zero or 11-14 bits if first code bit is zero; for missing
 * second symbol: 5-7 bits length of the symbol.
 */

/**
 * @def mhd_HF_DEC64
 * Use 64-bit accumulator for decoding on 64-bit platforms.
 */
#if SIZEOF_UINT_FAST32_T >= 8 || SIZEOF_VOIDP >= 8
#  define mhd_HF_DEC64   1
#endif

MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (2,1) MHD_FN_PAR_OUT_SIZE_ (4,3)  size_t
mhd_h2_huffman_decode (size_t encoded_size,
                       const void *restrict encoded,
                       size_t out_buf_size,
                       char *restrict out_buf,
                       enum mhd_H2HuffDecodeRes *restrict decode_result)
{
  mhd_constexpr uint_least32_t b32ones = 0xFFFFFFFFu; /**< 32 all-ones bits */
#ifdef mhd_HF_DEC64
  mhd_constexpr uint_least64_t b64ones = 0xFFFFFFFFFFFFFFFFu; /**< 64 bits all set to one */
  /** The width of the @a enc_chunk */
  mhd_constexpr uint_fast8_t enc_chunk_width = 64u;
  /** The chunk of the encoded data to be decoded. */
  uint_least64_t enc_chunk;
  /** The next bytes of the encoded data.
      Moved as needed to the @a enc_chunk by 32-bits portions. */
  uint_least64_t enc_shadow;
  /**< The number of valid bits in the @a enc_shadow */
  uint_fast8_t enc_shadow_bits;
#else  /* ! mhd_HF_DEC64 */
  /** The width of the @a enc_chunk */
  mhd_constexpr uint_fast8_t enc_chunk_width = 32u;
  /** The chunk of the encoded data to be decoded. */
  uint_least32_t enc_chunk;
  /** The next bytes of the encoded data.
      Moved as needed to the @a enc_chunk by bits. */
  uint_least32_t enc_chunk_next;
#endif /* ! mhd_HF_DEC64 */
  /** The total number of valid bits in @a enc_chunk and @a enc_chunk_next */
  uint_fast8_t enc_chunk_bits;
  const uint8_t *const enc_b = (const uint8_t *) encoded;
  size_t enc_idx;
  size_t out_idx;

  mhd_assert (h2huff_decode_inited);
#ifndef MHD_UNIT_TESTING
  /* DO check the input for validity if NOT unit-testing */
  mhd_assert (0 != encoded_size);
  mhd_assert (0 != out_buf_size);
#endif /* ! MHD_UNIT_TESTING */
  mhd_assert (out_buf_size <= ((~((size_t) (0))) - sizeof(uint_least32_t)));

  /* Pre-load data */
  if (1)
  {
    uint_least64_t data_in;
    size_t load_bytes;
    uint_least64_t enc_chunk64;
    data_in = 0;
    load_bytes = ((8u > encoded_size) ? encoded_size : 8u);
    memcpy (&data_in, enc_b, load_bytes);
    enc_chunk64 = mhd_GET_64BIT_BE (&data_in);
#ifdef mhd_HF_DEC64
    enc_chunk = enc_chunk64;
#else  /* ! mhd_HF_DEC64 */
    enc_chunk = ((uint_least32_t) (enc_chunk64 >> 32u)) & b32ones;
    enc_chunk_next = ((uint_least32_t) enc_chunk64) & b32ones;
#endif /* ! mhd_HF_DEC64 */
    enc_chunk_bits = (uint_fast8_t) (load_bytes << 3u);
    enc_idx = load_bytes;
#ifdef mhd_HF_DEC64
    if (enc_idx < encoded_size)
    {
      data_in = 0;
      load_bytes = encoded_size - enc_idx;
      if (8 < load_bytes)
        load_bytes = 8u;
      memcpy (&data_in, enc_b + enc_idx, load_bytes);
      enc_shadow = mhd_GET_64BIT_BE (&data_in);
      enc_shadow_bits = (uint_fast8_t) (load_bytes << 3u);
      enc_idx += load_bytes;
    }
    else
    {
      enc_shadow = 0u;
      enc_shadow_bits = 0u;
    }
#endif /* mhd_HF_DEC64 */
  }
  out_idx = 0;

  while (7 < enc_chunk_bits)
  {
    /* 32 bit MSB-aligned current decoding code */
    const uint_least32_t cur_code =
#ifdef mhd_HF_DEC64
      (uint_least32_t) (((uint_least32_t) (enc_chunk >> 32u)) & b32ones)
#else  /* ! mhd_HF_DEC64 */
      enc_chunk
#endif /* ! mhd_HF_DEC64 */
    ;
    uint_fast8_t pbits;
    uint_fast8_t tbits;
    uint_fast8_t next_pbits;
    const uint_least16_t *dec_table;
    uint_least16_t sym_and_blen;
    uint8_t sym;
    uint_fast8_t sym_bits;
    uint_least32_t tbl_pos;

    mhd_assert (encoded_size >= enc_idx);
    mhd_assert (64u >= enc_chunk_bits);

#ifdef mhd_HF_DEC64
    mhd_assert ((64u == enc_chunk_bits) || \
                (0 == ((enc_chunk << enc_chunk_bits) & b64ones)));
    mhd_assert (enc_chunk == (enc_chunk & b64ones));
#else  /* ! mhd_HF_DEC64 */
    mhd_assert ((32u <= enc_chunk_bits) || \
                (0 == ((enc_chunk << enc_chunk_bits) & b32ones)));
    mhd_assert ((32u > enc_chunk_bits) || (64u == enc_chunk_bits) || \
                (0 == ((enc_chunk_next << (enc_chunk_bits - 32u)) & b32ones)));
    mhd_assert (enc_chunk == (enc_chunk & b32ones));
#endif /* ! mhd_HF_DEC64 */

/**
 * Build an the starting code for the table as 32-bit MSB-aligned value.
 * @param pbits the table prefix width in bits.
 */
#define HUFF_DTBL_START(pbits)  ((b32ones << (32u - (pbits)))&b32ones)

    pbits = 0u;
    tbits = 8u;
    dec_table = mhd_h2huff_SL_by_code_0p8c;
    next_pbits = 7u;
    if (mhd_COND_PRED_FALSE_P (HUFF_DTBL_START (next_pbits) <= cur_code, 0.8))
    {
      pbits = next_pbits;
      tbits = 6u;
      dec_table = mhd_h2huff_SL_by_code_7p6c;
      next_pbits = 12u;
      if (mhd_COND_RARELY (HUFF_DTBL_START (next_pbits) <= cur_code))
      {
        pbits = next_pbits;
        tbits = 3u;
        dec_table = mhd_h2huff_SL_by_code_12p3c;
        next_pbits = 15u;
        if (HUFF_DTBL_START (next_pbits) <= cur_code)
        {
          pbits = next_pbits;
          tbits = 7u;
          dec_table = mhd_h2huff_SL_by_code_15p7c;
          next_pbits = 17u;
          if (HUFF_DTBL_START (next_pbits) <= cur_code)
          {
            pbits = next_pbits;
            tbits = 6u;
            dec_table = mhd_h2huff_SL_by_code_17p6c;
            next_pbits = 19u;
            if (HUFF_DTBL_START (next_pbits) <= cur_code)
            {
              pbits = next_pbits;
              tbits = 6u;
              dec_table = mhd_h2huff_SL_by_code_19p6c;
              next_pbits = 21u;
              if (HUFF_DTBL_START (next_pbits) <= cur_code)
              {
                pbits = next_pbits;
                tbits = 6u;
                dec_table = mhd_h2huff_SL_by_code_21p6c;
                next_pbits = 23u;
                if (HUFF_DTBL_START (next_pbits) <= cur_code)
                {
                  pbits = next_pbits;
                  tbits = 5u;
                  dec_table = mhd_h2huff_SL_by_code_23p5c;
                  next_pbits = 28u;
                  if (HUFF_DTBL_START (next_pbits) <= cur_code)
                  {
                    pbits = next_pbits;
                    tbits = 2u;
                    dec_table = mhd_h2huff_SL_by_code_28p2c;
                    next_pbits = 30u;
                    if (HUFF_DTBL_START (next_pbits) <= cur_code)
                    {
                      *decode_result = MHD_H2_HUFF_DEC_RES_BROKEN_DATA;
                      return 0;
                    }
                  }
                }
              }
            }
          }
        }
      }
#ifndef MHD_FAVOR_SMALL_CODE
      tbl_pos = ((cur_code << pbits) & b32ones) >> (32u - tbits);
#endif /* ! MHD_FAVOR_SMALL_CODE */
    }
#ifndef MHD_FAVOR_SMALL_CODE
    else
    {
      tbl_pos = cur_code >> (32u - tbits);
      (void) pbits;
    }
#endif /* ! MHD_FAVOR_SMALL_CODE */

#undef HUFF_DTBL_START

    mhd_assert (5u <= (pbits + tbits));
    mhd_assert (30u >= (pbits + tbits));

#ifdef MHD_FAVOR_SMALL_CODE
    tbl_pos = ((cur_code << pbits) & b32ones) >> (32u - tbits);
#endif /* MHD_FAVOR_SMALL_CODE */

    mhd_assert (HUFF_DTBL_ENTRIES (pbits, tbits, next_pbits) > tbl_pos);
    sym_and_blen = dec_table[tbl_pos];

    mhd_assert (0u != sym_and_blen);

    sym = (uint8_t) (sym_and_blen & 0xFFu);
    sym_bits = (uint_fast8_t) (sym_and_blen >> 8u);

    mhd_assert (0u != sym_bits);
    mhd_assert (5u <= sym_bits);
    mhd_assert (30u >= sym_bits);
    mhd_assert (pbits <= sym_bits);
    mhd_assert ((pbits + tbits) >= sym_bits);
    mhd_assert (mhd_h2huff_bitlen_by_sym[sym] == sym_bits);

    if (mhd_COND_VERY_RARELY (enc_chunk_bits < sym_bits))
    {
      *decode_result = MHD_H2_HUFF_DEC_RES_BROKEN_DATA;
      return 0;
    }
    enc_chunk_bits -= sym_bits;
#ifdef mhd_HF_DEC64
    enc_chunk = (enc_chunk << sym_bits) & b64ones;
#else  /* ! mhd_HF_DEC64 */
    enc_chunk = ((enc_chunk << sym_bits) & b32ones);
    enc_chunk |= enc_chunk_next >> (32u - sym_bits);
    enc_chunk_next = ((enc_chunk_next << sym_bits) & b32ones);
#endif /* ! mhd_HF_DEC64 */

    if (mhd_COND_VERY_RARELY (out_buf_size == out_idx))
    {
      *decode_result = MHD_H2_HUFF_DEC_RES_NO_SPACE;
      return 0;
    }
    out_buf[out_idx++] = (char) sym;

    if (mhd_COND_PRED_FALSE_P (32u > enc_chunk_bits, 0.84))
    {
      /* Load new chunk of encoded data */
#ifdef mhd_HF_DEC64
      if (enc_shadow_bits != 0)
      {
        const uint_fast8_t free_bits = (uint_fast8_t) (64u - enc_chunk_bits);
        const uint_fast8_t move_bits =
          (enc_shadow_bits > free_bits) ? free_bits : enc_shadow_bits;
        mhd_assert (0u != enc_chunk_bits && \
                    "A single symbol cannot be more than 30 bits. If on " \
                    "the previous step 'enc_chunk' has not been filled to " \
                    "more than 32 bits it means that 'enc_shadow' is " \
                    "already empty.");
        mhd_assert (0 != move_bits);

        enc_chunk |= enc_shadow >> enc_chunk_bits;
        enc_chunk_bits += move_bits;
        enc_shadow <<= move_bits;
        enc_shadow_bits -= move_bits;
        mhd_assert ((64u == enc_chunk_bits) || \
                    (0 == ((enc_chunk << enc_chunk_bits) & b64ones)));
      }
      if ((0u == enc_shadow_bits) \
          && (encoded_size > enc_idx))
      {
        uint_least64_t data_in;
        size_t load_bytes;

        data_in = 0;
        load_bytes = encoded_size - enc_idx;
        if (8u < load_bytes)
          load_bytes = 8u;
        memcpy (&data_in, enc_b + enc_idx, load_bytes);
        enc_idx += load_bytes;
        enc_shadow = mhd_GET_64BIT_BE (&data_in);
        enc_shadow_bits = (uint_fast8_t) (load_bytes << 3u);

        if (mhd_COND_RARELY (32u > enc_chunk_bits))
        {
          const uint_fast8_t free_bits = (uint_fast8_t) (64u - enc_chunk_bits);
          const uint_fast8_t use_bits =
            (enc_shadow_bits > free_bits) ? free_bits : enc_shadow_bits;

          mhd_assert (0u != enc_chunk_bits);
          enc_chunk |= enc_shadow >> enc_chunk_bits;
          enc_chunk_bits += use_bits;
          enc_shadow <<= use_bits;
          enc_shadow_bits -= use_bits;
        }
      }
#else  /* ! mhd_HF_DEC64 */
      if (encoded_size > enc_idx)
      {
        uint_least32_t data_in;
        size_t load_bytes;
        uint_least32_t enc_data_load;

        mhd_assert (0u != enc_chunk_bits && \
                    "A single symbol cannot be more than 30 bits. If more " \
                    "data to decode is present, then buffer would be filled " \
                    "on the previous step.");
        mhd_assert (0 == (enc_chunk & (b32ones >> enc_chunk_bits)));
        mhd_assert (0 == enc_chunk_next);

        data_in = 0;
        load_bytes = (encoded_size - enc_idx);
        if (4 < load_bytes)
          load_bytes = 4;
        memcpy (&data_in, enc_b + enc_idx, load_bytes);
        enc_idx += load_bytes;
        enc_data_load = mhd_GET_32BIT_BE (&data_in);

        enc_chunk |= ((enc_data_load >> enc_chunk_bits) & b32ones);
        enc_chunk_next = ((enc_data_load << (32u - enc_chunk_bits)) & b32ones);
        enc_chunk_bits += (uint_fast8_t) (load_bytes << 3u);
      }
#endif /* mhd_HF_DEC64 */
    }
  }

  if (0 != enc_chunk_bits)
  { /* Decode the last portion of the encoded data */

    /** The last portion of the data one-padded on the right */
    const uint_least8_t last_byte =
      (uint_least8_t) ((enc_chunk >> (enc_chunk_width - 8u))
                       | (0xFFu >> enc_chunk_bits));
    mhd_assert (7 >= enc_chunk_bits);

    if (0xFFu != last_byte)
    { /* The encoded data is not pure EOS */
      uint_least16_t sym_and_blen;
      uint8_t sym;
      uint_fast8_t sym_bits;

      sym_and_blen = mhd_h2huff_SL_by_code_0p8c[last_byte];

      mhd_assert (0u != sym_and_blen);

      sym = (uint8_t) (sym_and_blen & 0xFFu);
      sym_bits = (uint_fast8_t) (sym_and_blen >> 8u);
      mhd_assert (sym_bits <= 8u);

      if (mhd_COND_VERY_RARELY (enc_chunk_bits < sym_bits))
      {
        *decode_result = MHD_H2_HUFF_DEC_RES_BROKEN_DATA;
        return 0;
      }

      if (sym_bits != enc_chunk_bits)
      { /* Check whether the tail is EOS */
        if (mhd_COND_VERY_RARELY (((uint_least8_t) (0xFFu >> sym_bits)) !=
                                  (last_byte
                                   & (uint_least8_t) (0xFFu >> sym_bits))))
        {
          *decode_result = MHD_H2_HUFF_DEC_RES_BROKEN_DATA;
          return 0;
        }
      }

      if (mhd_COND_VERY_RARELY (out_buf_size == out_idx))
      {
        *decode_result = MHD_H2_HUFF_DEC_RES_NO_SPACE;
        return 0;
      }
      out_buf[out_idx++] = (char) sym;
    }
  }
  *decode_result = MHD_H2_HUFF_DEC_RES_OK;
  return out_idx;
}
