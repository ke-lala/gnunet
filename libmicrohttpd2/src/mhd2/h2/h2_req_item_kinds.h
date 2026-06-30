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
 * @file src/mhd2/h2/h2_req_item_kinds.h
 * @brief  Definition of the kinds of the HTTP/2 request items
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_H2_REQ_ITEM_KINDS_H
#define MHD_H2_REQ_ITEM_KINDS_H 1

#include "mhd_sys_options.h"

#ifdef mhd_USE_ENUM_BASE_T
#  include "sys_base_types.h"
#endif


/**
 * Request item kind
 */
enum mhd_H2RequestItemKind
mhd_ENUM_BASE_T (uint_least8_t)
{
  mhd_H2_RIK_HEADER =           (1u << 0u),
  mhd_H2_RIK_PSEUDOHEADER =     (1u << 1u),
  mhd_H2_RIK_COOKIE =           (1u << 2u),
  mhd_H2_RIK_URI_PARAM =        (1u << 3u),
  mhd_H2_RIK_URI_PARAM_NV =     (1u << 3u) + (1u << 7u),
  mhd_H2_RIK_TRAILER =          (1u << 4u),
  mhd_H2_RIK_PLACEHOLDER =      (1u << 5u),
  mhd_H2_RIK_ELIMINATED =       (1u << 6u)
};

#endif /* ! MHD_H2_REQ_ITEM_KINDS_H */
