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
 * @file src/mhd2/mhd_cntnr_ptr.h
 * @brief  The definition of mhd_CNTNR_PTR() and mhd_CNTNR_CPTR() macros.
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_CNTNR_PTR_H
#define MHD_CNTNR_PTR_H 1

#include "mhd_sys_options.h"

#include "sys_base_types.h"

#include "sys_offsetof.h"

/**
 * Get the pointer to the outer @a cntnr_type structure containing @a membr_name
 * member by the @a membr_ptr pointer to the member.
 *
 * This macro checks at compile time whether pointer to the @a membr_name in
 * the @a cntnr_type is compatible with the provided @a membr_ptr pointer.
 *
 * @param membr_ptr the pointer to the member
 * @param cntnr_type the type of the container with the @a membr_name member
 * @param membr_name the name of the member pointed by @a membr_ptr
 * @return the pointer to the outer structure
 */
#define mhd_CNTNR_PTR(membr_ptr,cntnr_type,membr_name)   \
        ((cntnr_type*) (void*)                            \
         (((char*) (0 ?                                    \
                    (&(((cntnr_type*) NULL)->membr_name)) : \
                    (membr_ptr))) - offsetof (cntnr_type,membr_name)))

/**
 * Get the const pointer to the outer @a cntnr_type structure
 * containing @a membr_name member by the @a membr_ptr pointer to the member.
 *
 * This macro checks at compile time whether pointer to the @a membr_name in
 * the @a cntnr_type is compatible with the provided @a membr_ptr pointer.
 *
 * @param membr_ptr the pointer to the member
 * @param cntnr_type the type of the container with the @a membr_name member
 * @param membr_name the name of the member pointed by @a membr_ptr
 * @return the pointer to the outer structure
 */
#define mhd_CNTNR_CPTR(membr_ptr,cntnr_type,membr_name)        \
        ((const cntnr_type*) (const void*)                      \
         (((const char*) (0 ?                                    \
                          (&(((cntnr_type*) NULL)->membr_name)) : \
                          (membr_ptr))) - offsetof (cntnr_type,membr_name)))

#endif /* ! MHD_CNTNR_PTR_H */
