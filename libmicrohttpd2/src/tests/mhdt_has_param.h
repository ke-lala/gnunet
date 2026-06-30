/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2016-2025 Karlson2k (Evgeny Grin)

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
 * @file src/tests/mhdt_has_param.h
 * @brief  A simple helper function to detect parameter presence
 * @author Karlson2k (Evgeny Grin)
 */
#ifndef MHDT_HAS_PARAM_H
#define MHDT_HAS_PARAM_H 1

#include <string.h>

/**
 * Check whether one of strings in array is equal to @a param.
 * String @a argv[0] is ignored.
 * @param argc number of strings in @a argv, as passed to main function
 * @param argv array of strings, as passed to main function
 * @param param parameter to look for.
 * @return zero if @a argv is NULL, @a param is NULL or empty string,
 *         @a argc is less then 2 or @a param is not found in @a argv,
 *         non-zero if one of strings in @a argv is equal to @a param.
 */
static inline int
mhdt_has_param (int argc, char *const argv[], const char *param)
{
  int i;
  if (! argv || ! param || ! param[0])
    return 0;

  for (i = 1; i < argc; i++)
  {
    if (argv[i] && (strcmp (argv[i], param) == 0) )
      return ! 0;
  }

  return 0;
}


#endif /* ! MHDT_HAS_PARAM_H */
