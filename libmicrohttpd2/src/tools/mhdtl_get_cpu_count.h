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
 * @file src/tools/mhdtl_get_cpu_count.h
 * @brief  Declaration of functions to detect the number of available
 *         CPU cores.
 * @author Karlson2k (Evgeny Grin)
 */


#ifndef MHDTL_GET_CPU_COUNT_H_
#define MHDTL_GET_CPU_COUNT_H_ 1


/**
 * Detect the number of logical CPU cores available for the process.
 * The number of cores available for this process could be different from
 * value of cores available on the system. The OS may have limit on number
 * assigned/allowed cores for single process and process may have limited
 * CPU affinity.
 * @return the number of logical CPU cores available for the process or
 *         -1 if failed to detect
 */
int
mhdtl_get_proc_cpu_count (void);


/**
 * Try to detect the number of logical CPU cores available for the system.
 * The number of available logical CPU cores could be changed any time due to
 * CPU hotplug.
 * @return the number of logical CPU cores available,
 *         -1 if failed to detect.
 */
int
mhdtl_get_system_cpu_count (void);

#endif /* MHDTL_GET_CPU_COUNT_H_ */
