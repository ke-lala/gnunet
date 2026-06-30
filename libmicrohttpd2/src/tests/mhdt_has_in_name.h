/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2016-2025 Evgeny Grin (Karlson2k)

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
 * @file src/tests/mhdt_has_in_name.h
 * @brief  A simple helper function to detect tokens in the filename
 * @author Karlson2k (Evgeny Grin)
 */
#ifndef MHDT_HAS_IN_NAME_H
#define MHDT_HAS_IN_NAME_H 1

#include <string.h>

/**
 * Check whether program name contains specific @a marker string.
 * Only last component in pathname is checked for marker presence,
 * all leading directories names (if any) are ignored. Directories
 * separators are handled correctly on both non-W32 and W32
 * platforms.
 * @param prog_name the program name, may include path
 * @param marker    the marker to look for
 * @return zero if any parameter is NULL or empty string or
 *         @a prog_name ends with slash or @a marker is not found in
 *         program name, non-zero if @a maker is found in program
 *         name.
 */
static inline int
mhdt_has_in_name (const char *prog_name, const char *marker)
{
  const char *dir_symb;
  const char *basename;

  if (! prog_name || ! marker || ! prog_name[0] || ! marker[0])
    return 0;

  basename = prog_name;
  dir_symb = strrchr (basename, '/');
  if (NULL != dir_symb)
    basename = dir_symb + 1;

#if defined(_WIN32) || defined(__CYGWIN__)
  dir_symb = strrchr (basename, '\\');
  if (NULL != dir_symb)
    basename = dir_symb + 1;
#endif /* _WIN32 || __CYGWIN__ */

  return (NULL != strstr (basename, marker));
}


#endif /* ! MHDT_HAS_IN_NAME_H */
