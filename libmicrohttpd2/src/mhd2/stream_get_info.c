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
 * @file src/mhd2/stream_get_info.c
 * @brief  The implementation of MHD_stream_get_info_*() functions
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "mhd_unreachable.h"
#include "mhd_cntnr_ptr.h"

#include "mhd_stream.h"
#include "mhd_connection.h"

#include "daemon_funcs.h"

#include "mhd_public_api.h"


MHD_EXTERN_ MHD_FN_MUST_CHECK_RESULT_
MHD_FN_PAR_NONNULL_ (1)
MHD_FN_PAR_NONNULL_ (3) MHD_FN_PAR_OUT_ (3) enum MHD_StatusCode
MHD_stream_get_info_fixed_sz (
  struct MHD_Stream *MHD_RESTRICT stream,
  enum MHD_StreamInfoFixedType info_type,
  union MHD_StreamInfoFixedData *MHD_RESTRICT output_buf,
  size_t output_buf_size)
{
  switch (info_type)
  {
  case MHD_STREAM_INFO_FIXED_DAEMON:
    if (sizeof(output_buf->v_daemon) > output_buf_size)
      return MHD_SC_INFO_GET_BUFF_TOO_SMALL;
    output_buf->v_daemon =
      mhd_daemon_get_master_daemon (mhd_CNTNR_PTR (stream, \
                                                   struct MHD_Connection, \
                                                   h1_stream)->daemon);
    return MHD_SC_OK;
  case MHD_STREAM_INFO_FIXED_CONNECTION:
    if (sizeof(output_buf->v_connection) > output_buf_size)
      return MHD_SC_INFO_GET_BUFF_TOO_SMALL;
    output_buf->v_connection = mhd_CNTNR_PTR (stream, \
                                              struct MHD_Connection, \
                                              h1_stream);
    return MHD_SC_OK;

  case MHD_STREAM_INFO_FIXED_SENTINEL:
  default:
    break;
  }
  return MHD_SC_INFO_GET_TYPE_UNKNOWN;
}


MHD_EXTERN_ MHD_FN_MUST_CHECK_RESULT_
MHD_FN_PAR_NONNULL_ (1)
MHD_FN_PAR_NONNULL_ (3) MHD_FN_PAR_OUT_ (3) enum MHD_StatusCode
MHD_stream_get_info_dynamic_sz (
  struct MHD_Stream *MHD_RESTRICT stream,
  enum MHD_StreamInfoDynamicType info_type,
  union MHD_StreamInfoDynamicData *MHD_RESTRICT output_buf,
  size_t output_buf_size)
{
  switch (info_type)
  {
  case MHD_STREAM_INFO_DYNAMIC_REQUEST:
    if (mhd_HTTP_STAGE_REQ_LINE_RECEIVED >
        mhd_CNTNR_PTR (stream, \
                       struct MHD_Connection, \
                       h1_stream)->stage)
      return MHD_SC_TOO_EARLY;
    if (sizeof(output_buf->v_request) > output_buf_size)
      return MHD_SC_INFO_GET_BUFF_TOO_SMALL;
    output_buf->v_request =
      &(mhd_CNTNR_PTR (stream, \
                       struct MHD_Connection, \
                       h1_stream)->rq);
    return MHD_SC_OK;

  case MHD_STREAM_INFO_DYNAMIC_SENTINEL:
  default:
    break;
  }
  return MHD_SC_INFO_GET_TYPE_UNKNOWN;
}
