/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2025 Christian Grothoff
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
 * @file src/mhd2/mhd_enum_to_str.c
 * @brief convenience functions returning constant string values
 * @author Christian Grothoff, Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "mhd_str_macros.h"

#include "sys_null_macro.h"
#include "mhd_public_api.h"

MHD_EXTERN_ MHD_FN_CONST_ const struct MHD_String *
MHD_protocol_version_to_string (enum MHD_HTTP_ProtocolVersion pv)
{
  switch (pv)
  {
  case MHD_HTTP_VERSION_INVALID:
    if (1)
    {
      static const struct MHD_String ret = mhd_MSTR_INIT ("[invalid]");
      return &ret;
    }
    break;
  case MHD_HTTP_VERSION_1_0:
    if (1)
    {
      static const struct MHD_String ret = mhd_MSTR_INIT ("HTTP/1.0");
      return &ret;
    }
    break;
  case MHD_HTTP_VERSION_1_1:
    if (1)
    {
      static const struct MHD_String ret = mhd_MSTR_INIT ("HTTP/1.1");
      return &ret;
    }
    break;
  case MHD_HTTP_VERSION_2:
    if (1)
    {
      static const struct MHD_String ret = mhd_MSTR_INIT ("HTTP/2");
      return &ret;
    }
    break;
  case MHD_HTTP_VERSION_3:
    if (1)
    {
      static const struct MHD_String ret = mhd_MSTR_INIT ("HTTP/3");
      return &ret;
    }
    break;
  case MHD_HTTP_VERSION_FUTURE:
    if (1)
    {
      static const struct MHD_String ret = mhd_MSTR_INIT ("[future version]");
      return &ret;
    }
    break;
  default:
    break;
  }

  return NULL;
}


MHD_EXTERN_ MHD_FN_CONST_ const struct MHD_String *
MHD_http_method_to_string (enum MHD_HTTP_Method method)
{
  switch (method)
  {
  case MHD_HTTP_METHOD_GET:
    if (1)
    {
      static const struct MHD_String ret = mhd_MSTR_INIT ("GET");
      return &ret;
    }
    break;
  case MHD_HTTP_METHOD_HEAD:
    if (1)
    {
      static const struct MHD_String ret = mhd_MSTR_INIT ("HEAD");
      return &ret;
    }
    break;
  case MHD_HTTP_METHOD_POST:
    if (1)
    {
      static const struct MHD_String ret = mhd_MSTR_INIT ("POST");
      return &ret;
    }
    break;
  case MHD_HTTP_METHOD_PUT:
    if (1)
    {
      static const struct MHD_String ret = mhd_MSTR_INIT ("PUT");
      return &ret;
    }
    break;
  case MHD_HTTP_METHOD_DELETE:
    if (1)
    {
      static const struct MHD_String ret = mhd_MSTR_INIT ("DELETE");
      return &ret;
    }
    break;
  case MHD_HTTP_METHOD_CONNECT:
    if (1)
    {
      static const struct MHD_String ret = mhd_MSTR_INIT ("CONNECT");
      return &ret;
    }
    break;
  case MHD_HTTP_METHOD_OPTIONS:
    if (1)
    {
      static const struct MHD_String ret = mhd_MSTR_INIT ("OPTIONS");
      return &ret;
    }
    break;
  case MHD_HTTP_METHOD_TRACE:
    if (1)
    {
      static const struct MHD_String ret = mhd_MSTR_INIT ("TRACE");
      return &ret;
    }
    break;
  case MHD_HTTP_METHOD_ASTERISK:
    if (1)
    {
      static const struct MHD_String ret = mhd_MSTR_INIT ("*");
      return &ret;
    }
    break;
  case MHD_HTTP_METHOD_OTHER: /* Handled as unknown value */
  default:
    break;
  }

  return NULL;
}
