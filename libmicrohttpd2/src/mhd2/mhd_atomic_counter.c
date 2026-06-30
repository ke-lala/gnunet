/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2024-2025 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/mhd_atomic_counter.c
 * @brief  The definition of the atomic counter functions
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "mhd_atomic_counter.h"

#if defined(mhd_ATOMIC_BY_LOCKS)

#include "mhd_assert.h"

MHD_INTERNAL mhd_ATOMIC_COUNTER_TYPE
mhd_atomic_counter_get_inc_wrap (struct mhd_AtomicCounter *pcnt)
{
  mhd_ATOMIC_COUNTER_TYPE ret;

  mhd_mutex_lock_chk (&(pcnt->lock));
  ret = pcnt->count++;
  mhd_mutex_unlock_chk (&(pcnt->lock));

  return ret;
}


#ifndef NDEBUG

MHD_INTERNAL mhd_ATOMIC_COUNTER_TYPE
mhd_atomic_counter_get_inc (struct mhd_AtomicCounter *pcnt)
{
  const mhd_ATOMIC_COUNTER_TYPE ret = mhd_atomic_counter_get_inc_wrap (pcnt);

  mhd_assert (mhd_ATOMIC_COUNTER_MAX != ret); /* check for overflow */

  return ret;
}


#endif /* ! NDEBUG */


MHD_INTERNAL mhd_ATOMIC_COUNTER_TYPE
mhd_atomic_counter_get_dec (struct mhd_AtomicCounter *pcnt)
{
  mhd_ATOMIC_COUNTER_TYPE ret;

  mhd_mutex_lock_chk (&(pcnt->lock));
  ret = pcnt->count--;
  mhd_mutex_unlock_chk (&(pcnt->lock));

  mhd_assert (mhd_ATOMIC_COUNTER_MAX != ret); /* check for underflow */

  return ret;
}


MHD_INTERNAL mhd_ATOMIC_COUNTER_TYPE
mhd_atomic_counter_get (struct mhd_AtomicCounter *pcnt)
{
  mhd_ATOMIC_COUNTER_TYPE ret;

  mhd_mutex_lock_chk (&(pcnt->lock));
  ret = pcnt->count;
  mhd_mutex_unlock_chk (&(pcnt->lock));

  return ret;
}


#endif /* mhd_ATOMIC_BY_LOCKS */
