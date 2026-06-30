/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2024 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/upgrade_prep.h
 * @brief  The declaration of functions for preparing for MHD Action for
 *         HTTP-Upgrade
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_UPGRADE_PREP_H
#define MHD_UPGRADE_PREP_H 1

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

struct MHD_Request;             /* Forward declaration */
struct MHD_NameValueCStr;       /* Forward declaration */

/**
 * Prepare connection for the HTTP "Upgrade" action.
 *
 * Unlike other actions, this kind of action manipulates connection's output
 * buffers.
 *
 * @param req the request object
 * @param upgrade_hdr_value the value of the "Upgrade:" header, mandatory
                            string
 * @param num_headers number of elements in the @a headers array,
 *                    must be zero if @a headers is NULL
 * @param headers the optional pointer to the array of the headers (the strings
 *                are copied and does not need to be valid after return from
 *                this function),
 *                can be NULL if @a num_headers is zero
 * @param is_upload_act must be set to 'true' if the action being created is
 *                      "upload" kind of action
 * @return 'true' if succeed,
 *         'false' otherwise
 */
MHD_INTERNAL bool
mhd_upgrade_prep_for_action (struct MHD_Request *restrict req,
                             const char *restrict upgrade_hdr_value,
                             size_t num_headers,
                             const struct MHD_NameValueCStr *restrict headers,
                             bool is_upload_act)
MHD_FN_PAR_NONNULL_ (1) MHD_FN_PAR_NONNULL_ (2) MHD_FN_PAR_CSTR_ (2)
MHD_FN_PAR_IN_SIZE_ (4,3);

#endif /* ! MHD_UPGRADE_PREP_H */
