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
 * @file src/mhd2/h2/h2_frame_types.h
 * @brief  Definitions of HTTP/2 frame types
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_H2_FRAME_TYPES_H
#define MHD_H2_FRAME_TYPES_H 1

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include "h2_err_codes.h"


enum MHD_FIXED_ENUM_ mhd_H2FrameDataID
mhd_ENUM_BASE_T (uint_least8_t)
{
  mhd_H2_FRAME_DATA_ID = 0x00u
};

struct mhd_H2FrameDataInfo
{
  uint_least32_t length;
  enum mhd_H2FrameDataID type;
  uint_least32_t stream_id;

  bool padded; /* Changes extra header size */
  bool end_stream;

  uint_least8_t pad_length;
};


enum MHD_FIXED_ENUM_ mhd_H2FrameHeadersID
mhd_ENUM_BASE_T (uint_least8_t)
{
  mhd_H2_FRAME_HEADERS_ID = 0x01u
};

struct mhd_H2FrameHeadersInfo
{
  uint_least32_t length;
  enum mhd_H2FrameHeadersID type;
  uint_least32_t stream_id;

  bool priority; /* Changes extra header size */
  bool padded;   /* Changes extra header size */
  bool end_headers;
  bool end_stream;

  uint_least8_t pad_length;
  bool exclusive;
  uint_least32_t stream_dependency;
  uint_least8_t weight; /* "on-wire" format, 0..255 */
};


enum MHD_FIXED_ENUM_ mhd_H2FramePriorityID
mhd_ENUM_BASE_T (uint_least8_t)
{
  mhd_H2_FRAME_PRIORITY_ID = 0x02u
};

struct mhd_H2FramePriorityInfo
{
  uint_least32_t length;
  enum mhd_H2FramePriorityID type;
  uint_least32_t stream_id;

  bool exclusive;
  uint_least32_t stream_dependency;
  uint_least8_t weight; /* "on-wire" format, 0..255 */
};


enum MHD_FIXED_ENUM_ mhd_H2FrameRstStreamID
mhd_ENUM_BASE_T (uint_least8_t)
{
  mhd_H2_FRAME_RST_STREAM_ID = 0x03u
};

struct mhd_H2FrameRstStreamInfo
{
  uint_least32_t length;
  enum mhd_H2FrameRstStreamID type;
  uint_least32_t stream_id;

  enum mhd_H2ErrorCode error_code; // TODO: support 4 > sizeof(int)
};


enum MHD_FIXED_ENUM_ mhd_H2FrameSettingsID
mhd_ENUM_BASE_T (uint_least8_t)
{
  mhd_H2_FRAME_SETTINGS_ID = 0x04u
};

struct mhd_H2FrameSettingsInfo
{
  uint_least32_t length;
  enum mhd_H2FrameSettingsID type;
  uint_least32_t stream_id;

  bool ack;
};


enum MHD_FIXED_ENUM_ mhd_H2FramePushPromiseID
mhd_ENUM_BASE_T (uint_least8_t)
{
  mhd_H2_FRAME_PUSH_PROMISE_ID = 0x05u
};

struct mhd_H2FramePushPromiseInfo
{
  uint_least32_t length;
  enum mhd_H2FramePushPromiseID type;
  uint_least32_t stream_id;

  bool padded; /* Changes extra header size */
  bool end_headers;

  uint_least8_t pad_length;
  uint_least32_t promised_stream_id;
};


enum MHD_FIXED_ENUM_ mhd_H2FramePingID
mhd_ENUM_BASE_T (uint_least8_t)
{
  mhd_H2_FRAME_PING_ID = 0x06u
};

struct mhd_H2FramePingInfo
{
  uint_least32_t length;
  enum mhd_H2FramePingID type;
  uint_least32_t stream_id;

  bool ack;

  uint8_t opaque_data[8];
};


enum MHD_FIXED_ENUM_ mhd_H2FrameGoawayID
mhd_ENUM_BASE_T (uint_least8_t)
{
  mhd_H2_FRAME_GOAWAY_ID = 0x07u
};

struct mhd_H2FrameGoawayInfo
{
  uint_least32_t length;
  enum mhd_H2FrameGoawayID type;
  uint_least32_t stream_id;

  uint_least32_t last_stream_id;
  enum mhd_H2ErrorCode error_code; // TODO: support 4 > sizeof(int)
};


enum MHD_FIXED_ENUM_ mhd_H2FrameWindowUpdateID
mhd_ENUM_BASE_T (uint_least8_t)
{
  mhd_H2_FRAME_WINDOW_UPDATE_ID = 0x08u
};

struct mhd_H2FrameWindowUpdateInfo
{
  uint_least32_t length;
  enum mhd_H2FrameWindowUpdateID type;
  uint_least32_t stream_id;

  uint_least32_t window_size_increment;
};


enum MHD_FIXED_ENUM_ mhd_H2FrameContinuationID
mhd_ENUM_BASE_T (uint_least8_t)
{
  mhd_H2_FRAME_CONTINUATION_ID = 0x09u
};

struct mhd_H2FrameContinuationInfo
{
  uint_least32_t length;
  enum mhd_H2FrameContinuationID type;
  uint_least32_t stream_id;

  bool end_headers;
};


enum mhd_H2FrameIDs
mhd_ENUM_BASE_T (uint_least8_t)
{
  mhd_H2_FRAME_IDS_DATA_ID = mhd_H2_FRAME_DATA_ID,
  mhd_H2_FRAME_IDS_HEADERS_ID = mhd_H2_FRAME_HEADERS_ID,
  mhd_H2_FRAME_IDS_PRIORITY_ID = mhd_H2_FRAME_PRIORITY_ID,
  mhd_H2_FRAME_IDS_RST_STREAM_ID = mhd_H2_FRAME_RST_STREAM_ID,
  mhd_H2_FRAME_IDS_SETTINGS_ID = mhd_H2_FRAME_SETTINGS_ID,
  mhd_H2_FRAME_IDS_PUSH_PROMISE_ID = mhd_H2_FRAME_PUSH_PROMISE_ID,
  mhd_H2_FRAME_IDS_PING_ID = mhd_H2_FRAME_PING_ID,
  mhd_H2_FRAME_IDS_GOAWAY_ID = mhd_H2_FRAME_GOAWAY_ID,
  mhd_H2_FRAME_IDS_WINDOW_UPDATE_ID = mhd_H2_FRAME_WINDOW_UPDATE_ID,
  mhd_H2_FRAME_IDS_CONTINUATION_ID = mhd_H2_FRAME_CONTINUATION_ID
};

struct mhd_H2FrameSelector
{
  uint_least32_t length;
  enum mhd_H2FrameIDs type;
  uint_least32_t stream_id;
};


union mhd_H2FrameUnion
{
  struct mhd_H2FrameDataInfo data;
  struct mhd_H2FrameHeadersInfo headers;
  struct mhd_H2FramePriorityInfo priority;
  struct mhd_H2FrameRstStreamInfo rst_stream;
  struct mhd_H2FrameSettingsInfo settings;
  struct mhd_H2FramePushPromiseInfo push_promise;
  struct mhd_H2FramePingInfo ping;
  struct mhd_H2FrameGoawayInfo goaway;
  struct mhd_H2FrameWindowUpdateInfo window_update;
  struct mhd_H2FrameContinuationInfo continuation;

  struct mhd_H2FrameSelector selector;
};

#endif /* ! MHD_H2_FRAME_TYPES_H */
