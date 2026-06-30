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
 * @file src/mhd2/h2/h2_frame_init.h
 * @brief  Declarations of HTTP/2 frame decoding and encoding functions
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_H2_FRAME_INIT_H
#define MHD_H2_FRAME_INIT_H 1

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include <string.h>

#include "h2_frame_types.h"
#include "h2_frame_length.h"

#if defined(_MSC_FULL_VER)
#pragma warning(push)
/* Disable C4505 "unreferenced local function has been removed" */
#pragma warning(disable:4505)
#endif /* _MSC_FULL_VER */

mhd_static_inline struct mhd_H2FrameDataInfo*
mhd_h2_frame_init_data (union mhd_H2FrameUnion *restrict frame_info,
                        uint_least32_t stream_id,
                        bool end_stream)
{
  struct mhd_H2FrameDataInfo *const data =  &(frame_info->data);
  data->type = mhd_H2_FRAME_DATA_ID;
  data->length = 0u; /* To be set later */
  data->stream_id = stream_id;

  data->padded = false;
  data->end_stream = end_stream;

  data->pad_length = 0u;

  return data;
}


mhd_static_inline struct mhd_H2FrameHeadersInfo*
mhd_h2_frame_init_headers (union mhd_H2FrameUnion *restrict frame_info,
                           uint_least32_t stream_id,
                           bool end_headers,
                           bool end_stream)
{
  struct mhd_H2FrameHeadersInfo *const headers = &(frame_info->headers);
  headers->type = mhd_H2_FRAME_HEADERS_ID;
  headers->length = 0u; /* To be set later */
  headers->stream_id = stream_id;

  headers->priority = false;
  headers->padded = false;
  headers->end_headers = end_headers;
  headers->end_stream = end_stream;

  headers->pad_length = 0u;
  headers->exclusive = false;
  headers->stream_dependency = 0u;
  headers->weight = 0u;

  return headers;
}


mhd_static_inline struct mhd_H2FrameRstStreamInfo*
mhd_h2_frame_init_rst_stream (union mhd_H2FrameUnion *restrict frame_info,
                              uint_least32_t stream_id,
                              enum mhd_H2ErrorCode error_code)
{
  struct mhd_H2FrameRstStreamInfo *const rst_stream = &(frame_info->rst_stream);
  rst_stream->type = mhd_H2_FRAME_RST_STREAM_ID;
  rst_stream->length = mhd_H2_FR_FIXED_LEN_RST_STREAM; /* Fixed size */
  rst_stream->stream_id = stream_id;

  rst_stream->error_code = error_code;

  return rst_stream;
}


mhd_static_inline struct mhd_H2FrameSettingsInfo*
mhd_h2_frame_init_settings (union mhd_H2FrameUnion *restrict frame_info,
                            bool ack)
{
  struct mhd_H2FrameSettingsInfo *const settings = &(frame_info->settings);
  settings->type = mhd_H2_FRAME_SETTINGS_ID;
  settings->length = 0u; /* Could be increased later */
  settings->stream_id = 0u;

  settings->ack = ack;

  return settings;
}


mhd_static_inline struct mhd_H2FramePingInfo*
mhd_h2_frame_init_ping (union mhd_H2FrameUnion *restrict frame_info,
                        bool ack,
                        const uint8_t opaque_data[MHD_FN_PAR_FIX_ARR_SIZE_ (8)])
{
  struct mhd_H2FramePingInfo *const ping = &(frame_info->ping);
  ping->type = mhd_H2_FRAME_PING_ID;
  ping->length = mhd_H2_FR_FIXED_LEN_PING; /* Fixed size */
  ping->stream_id = 0u;

  ping->ack = ack;

  memcpy (ping->opaque_data,
          opaque_data,
          8u);

  return ping;
}


mhd_static_inline struct mhd_H2FrameGoawayInfo*
mhd_h2_frame_init_goaway (union mhd_H2FrameUnion *restrict frame_info,
                          uint_least32_t last_stream_id,
                          enum mhd_H2ErrorCode error_code)
{
  struct mhd_H2FrameGoawayInfo *const goaway = &(frame_info->goaway);
  goaway->type = mhd_H2_FRAME_GOAWAY_ID;
  goaway->length = 8u; /* Could be increased later */
  goaway->stream_id = 0u;

  goaway->last_stream_id = last_stream_id;
  goaway->error_code = error_code;

  return goaway;
}


mhd_static_inline struct mhd_H2FrameWindowUpdateInfo*
mhd_h2_frame_init_window_update (union mhd_H2FrameUnion *restrict frame_info,
                                 uint_least32_t stream_id,
                                 uint_least32_t window_size_increment)
{
  struct mhd_H2FrameWindowUpdateInfo *const window_update =
    &(frame_info->window_update);
  window_update->type = mhd_H2_FRAME_WINDOW_UPDATE_ID;
  window_update->length = mhd_H2_FR_FIXED_LEN_WINDOW_UPDATE; /* Fixed size */
  window_update->stream_id = stream_id;

  window_update->window_size_increment = window_size_increment;

  return window_update;
}


mhd_static_inline struct mhd_H2FrameContinuationInfo*
mhd_h2_frame_init_continuation (union mhd_H2FrameUnion *restrict frame_info,
                                uint_least32_t stream_id,
                                bool end_headers)
{
  struct mhd_H2FrameContinuationInfo *const continuation =
    &(frame_info->continuation);
  continuation->type = mhd_H2_FRAME_CONTINUATION_ID;
  continuation->length = 0u; /* To be set later */
  continuation->stream_id = stream_id;

  continuation->end_headers = end_headers;

  return continuation;
}


#if defined(_MSC_FULL_VER)
/* Restore warnings */
#pragma warning(pop)
#endif /* _MSC_FULL_VER */

#endif /* ! MHD_H2_FRAME_INIT_H */
