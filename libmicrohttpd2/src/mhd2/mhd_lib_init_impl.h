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
 * @file src/mhd2/mhd_lib_init_impl.h
 * @brief  Library global initialisers and de-initialisers
 * @author Karlson2k (Evgeny Grin)
 *
 * This file should be a .c file, but used as .h file to workaround
 * a GCC/binutils bug.
 */

#ifdef MHD_LIB_INIT_IMPL_H
#error This file must not be included more the one time only
#endif
#define MHD_LIB_INIT_IMPL_H 1

/* Due to peculiarities of linking, on some platforms (at least on W32)
 * the automatic initialisation functions are not called when library is used
 * as a static library and no function is used/referred from the same
 * object/module/c-file.
 */
#ifndef MHD_LIB_INIT_IMPL_H_IN_DAEMON_CREATE_C
#error This file must in included only in 'daemon_create.c' file
#else  /* MHD_LIB_INIT_IMPL_H_IN_DAEMON_CREATE_C */

#include "mhd_sys_options.h"
#include "mhd_lib_init_auto.h"


#ifdef mhd_AUTOINIT_FUNCS_USE

#  ifndef mhd_AUTOINIT_FUNCS_PRAGMA
/* Call automatically initialiser and deinitialiser functions */
AIF_SET_INIT_AND_DEINIT_FUNCS (mhd_lib_global_init_auto, \
                               mhd_lib_global_deinit_auto);
#  else
/* Call automatically initialiser function */
#pragma init(mhd_lib_global_init_auto)
/* Call automatically deinitialiser function */
#pragma fini(mhd_lib_global_deinit_auto)
#  endif

#endif /* AIF_AUTOINIT_FUNCS_ARE_SUPPORTED */

#endif /* MHD_LIB_INIT_IMPL_H_IN_DAEMON_CREATE_C */
