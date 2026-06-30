/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2015-2024 Karlson2k (Evgeny Grin)

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
 * @file src/mhd2/mhd_mono_clock.h
 * @brief  internal monotonic clock functions declarations
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_MONO_CLOCK_H
#define MHD_MONO_CLOCK_H 1
#include "mhd_sys_options.h"

#include "sys_base_types.h"
#include "sys_bool_type.h"

/**
 * Initialise milliseconds counters completely.
 * Must be called only one time per application run.
 */
MHD_INTERNAL void
mhd_mclock_init_once (void);


#ifdef HAVE_CLOCK_GET_TIME
/* Resources may be allocated only for Darwin clock_get_time() */

/**
 * Deinitialise milliseconds counters by freeing any allocated resources
 */
MHD_INTERNAL void
mhd_mclock_deinit (void);

/**
 * Re-initialise monotonic clocks are de-initialisaion has been performed
 */
MHD_INTERNAL void
mhd_mclock_re_init (void);

#else  /* ! HAVE_CLOCK_GET_TIME */
/* No-op implementation */
#  define mhd_mclock_deinit()   ((void) 0)
#  define mhd_mclock_re_init()  ((void) 0)
#endif /* ! HAVE_CLOCK_GET_TIME */


/**
 * Monotonic milliseconds counter, useful for timeout calculation.
 * Tries to be not affected by manually setting the system real time
 * clock or adjustments by NTP synchronization.
 *
 * @return number of microseconds from some fixed moment
 */
MHD_INTERNAL uint_fast64_t
mhd_monotonic_msec_counter (void);

#endif /* MHD_MONO_CLOCK_H */
