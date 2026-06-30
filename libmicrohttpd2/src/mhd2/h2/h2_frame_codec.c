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
 * @file src/mhd2/h2/h2_frame_codec.c
 * @brief  Implementation of HTTP/2 frame decoding and encoding functions
 * @author Karlson2k (Evgeny Grin)
 *
 * Details of HTTP/2 frame layout are defined in RFC 9113.
 */

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include "mhd_assert.h"
#include "mhd_unreachable.h"

#include <string.h>

#include "mhd_bithelpers.h"

#include "h2_bit_masks.h"

#include "h2_frame_length.h"

#include "h2_frame_codec.h"


/**
 * HTTP/2 frame flag PRIORITY (0x20) used by HEADERS frames
 */
#define mhd_FFLAG_PRIORITY      (0x20u)
/**
 * HTTP/2 frame flag PADDED (0x08) used by DATA, HEADERS and PUSH_PROMISE frames
 */
#define mhd_FFLAG_PADDED        (0x08u)
/**
 * HTTP/2 frame flag END_HEADERS (0x04) used by HEADERS and CONTINUATION frames
 */
#define mhd_FFLAG_END_HEADERS   (0x04u)
/**
 * HTTP/2 frame flag END_STREAM (0x01) used by DATA and HEADERS frames
 */
#define mhd_FFLAG_END_STREAM    (0x01u)
/**
 * HTTP/2 frame flag ACK (0x01) used by SETTINGS and PING frames
 */
#define mhd_FFLAG_ACK           (0x01u)

/**
 * Exclusive bit (0x80000000) for the HEADERS/PRIORITY dependency field.
 */
#define mhd_FFLAG_HEADERS_EXCLUSIVE     (0x80000000u)


/**
 * Decode a DATA frame.
 *
 * Validates sizes data and flags data for valid combinations.
 *
 * @param payload_buff_size the available input bytes in the @a payload_buff
 * @param payload_buff the pointer to the beginning of the frame payload
 * @param flags the frame flags byte
 * @param[in,out] frame_info the frame information; updated with DATA details
 * @param[out] frame_payload set to the final payload slice that starts after
 *                           any extra header fields and ends before padding
 *                           bytes (if any)
 * @return #mhd_H2_F_DEC_OK on success,
 *         or a relevant decode error code
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (2,1)
MHD_FN_PAR_INOUT_ (4) MHD_FN_PAR_OUT_ (5)
enum mhd_H2FrameDecodeResult
frame_decode_data (size_t payload_buff_size,
                   uint8_t *restrict payload_buff,
                   uint_least8_t flags,
                   union mhd_H2FrameUnion *restrict frame_info,
                   struct mhd_Buffer *restrict frame_payload)
{
  struct mhd_H2FrameDataInfo *const data =
    &(frame_info->data);
  size_t real_payload_pos;

  mhd_assert (mhd_H2_FRAME_DATA_ID == data->type);

  if (0u == data->stream_id)
    return mhd_H2_F_DEC_CONN_ERR_PROT;

  data->padded =        (0 != (flags & mhd_FFLAG_PADDED));
  data->end_stream =    (0 != (flags & mhd_FFLAG_END_STREAM));

  real_payload_pos = 0u;
  if (data->padded)
  {
    if (real_payload_pos >= data->length)
      return mhd_H2_F_DEC_CONN_ERR_PROT;
    if (real_payload_pos >= payload_buff_size)
      return mhd_H2_F_DEC_F_HEADER_INCOMPLETE;

    data->pad_length = (uint_least8_t) payload_buff[real_payload_pos++];
  }
  else
    data->pad_length = 0u;

  if (data->length < (real_payload_pos + data->pad_length))
    return mhd_H2_F_DEC_CONN_ERR_PROT;
  if (payload_buff_size < data->length)
    return mhd_H2_F_DEC_F_PAYLOAD_INCOMPLETE;

  frame_payload->data = (char *) payload_buff + real_payload_pos;
  frame_payload->size =
    (size_t) (data->length - real_payload_pos - data->pad_length);

  return mhd_H2_F_DEC_OK;
}


/**
 * Decode a HEADERS frame.
 *
 * Validates sizes data and flags data for valid combinations.
 *
 * @param payload_buff_size the available input bytes in the @a payload_buff
 * @param payload_buff the pointer to the beginning of the frame payload
 * @param flags the frame flags byte
 * @param[in,out] frame_info the frame information;
 *                           updated with HEADERS details
 * @param[out] frame_payload set to the final payload slice that starts after
 *                           any extra header fields and ends before padding
 *                           bytes (if any)
 * @return #mhd_H2_F_DEC_OK on success,
 *         or a relevant decode error code
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (2,1)
MHD_FN_PAR_INOUT_ (4) MHD_FN_PAR_OUT_ (5)
enum mhd_H2FrameDecodeResult
frame_decode_headers (size_t payload_buff_size,
                      uint8_t *restrict payload_buff,
                      uint_least8_t flags,
                      union mhd_H2FrameUnion *restrict frame_info,
                      struct mhd_Buffer *restrict frame_payload)
{
  struct mhd_H2FrameHeadersInfo *const headers =
    &(frame_info->headers);
  size_t real_payload_pos;

  mhd_assert (mhd_H2_FRAME_HEADERS_ID == headers->type);

  if (0u == headers->stream_id)
    return mhd_H2_F_DEC_CONN_ERR_PROT;

  headers->priority =           (0 != (flags & mhd_FFLAG_PRIORITY));
  headers->padded =             (0 != (flags & mhd_FFLAG_PADDED));
  headers->end_headers =        (0 != (flags & mhd_FFLAG_END_HEADERS));
  headers->end_stream =         (0 != (flags & mhd_FFLAG_END_STREAM));

  real_payload_pos = 0u;
  if (headers->padded)
  {
    if (real_payload_pos >= headers->length)
      return mhd_H2_F_DEC_CONN_ERR_PROT;
    if (real_payload_pos >= payload_buff_size)
      return mhd_H2_F_DEC_F_HEADER_INCOMPLETE;
    headers->pad_length = (uint_least8_t) payload_buff[real_payload_pos++];
  }
  else
    headers->pad_length = 0u;

  if (headers->priority)
  {
    uint_least32_t stream_dep_id;
    if ((real_payload_pos + 5u) > headers->length)
      return mhd_H2_F_DEC_CONN_ERR_PROT;
    if ((real_payload_pos + 5u) > payload_buff_size)
      return mhd_H2_F_DEC_F_HEADER_INCOMPLETE;
    stream_dep_id = mhd_GET_32BIT_BE_UNALIGN (payload_buff + real_payload_pos);
    real_payload_pos += 4u;
    headers->exclusive = (0 != (stream_dep_id & mhd_FFLAG_HEADERS_EXCLUSIVE));
    headers->stream_dependency = (uint_least32_t)
                                 (stream_dep_id & mhd_H2_STREAM_ID_MASK);
    if (headers->stream_id == headers->stream_dependency)
      return mhd_H2_F_DEC_STREAM_ERR_PROT;
    /* Use "on-wire" 'weight' format. Add one to get a real number.
       Do not bother handling calculations here as it is deprecated anyway. */
    headers->weight = payload_buff[real_payload_pos++];
  }
  else
  {
    headers->exclusive = false;
    headers->stream_dependency = 0u;
    headers->weight = 0u;
  }

  if (headers->length < (real_payload_pos + headers->pad_length))
    return mhd_H2_F_DEC_CONN_ERR_PROT;
  if (payload_buff_size < headers->length)
    return mhd_H2_F_DEC_F_PAYLOAD_INCOMPLETE;

  frame_payload->data = (char *) payload_buff + real_payload_pos;
  frame_payload->size =
    (size_t) (headers->length - real_payload_pos - headers->pad_length);

  return mhd_H2_F_DEC_OK;
}


/**
 * Decode a PRIORITY frame.
 *
 * Validates the fixed size of the frame.
 *
 * @param payload_buff_size the available input bytes in the @a payload_buff
 * @param payload_buff the pointer to the beginning of the frame payload
 * @param[in,out] frame_info the frame information;
 *                           updated with PRIORITY details
 * @return #mhd_H2_F_DEC_OK on success,
 *         or a relevant decode error code
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (2,1)
MHD_FN_PAR_INOUT_ (3)
enum mhd_H2FrameDecodeResult
frame_decode_priority (size_t payload_buff_size,
                       uint8_t *restrict payload_buff,
                       union mhd_H2FrameUnion *restrict frame_info)
{
  struct mhd_H2FramePriorityInfo *const priority =
    &(frame_info->priority);
  uint_least32_t stream_dep_id;

  mhd_assert (mhd_H2_FRAME_PRIORITY_ID == priority->type);

  if (0u == priority->stream_id)
    return mhd_H2_F_DEC_CONN_ERR_PROT;
  if (5u != priority->length)
    return mhd_H2_F_DEC_STREAM_ERR_F_SIZE;
  if (5u > payload_buff_size)
    return mhd_H2_F_DEC_F_HEADER_INCOMPLETE;

  stream_dep_id = mhd_GET_32BIT_BE_UNALIGN (payload_buff + 0u);
  priority->exclusive = (0 != (stream_dep_id & mhd_FFLAG_HEADERS_EXCLUSIVE));
  priority->stream_dependency = (uint_least32_t)
                                (stream_dep_id & mhd_H2_STREAM_ID_MASK);
  /* Use "on-wire" 'weight' format. Add one to get a real number.
     Do not bother handling calculations here as it is deprecated anyway. */
  priority->weight = payload_buff[4];

  if (priority->stream_id == priority->stream_dependency)
    return mhd_H2_F_DEC_STREAM_ERR_PROT;

  return mhd_H2_F_DEC_OK;
}


/**
 * Decode an RST_STREAM frame.
 *
 * Validates the fixed size of the frame.
 *
 * @param payload_buff_size the available input bytes in the @a payload_buff
 * @param payload_buff the pointer to the beginning of the frame payload
 * @param[in,out] frame_info the frame information;
 *                           updated with RST_STREAM details
 * @return #mhd_H2_F_DEC_OK on success,
 *         or a relevant decode error code
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (2,1)
MHD_FN_PAR_INOUT_ (3)
enum mhd_H2FrameDecodeResult
frame_decode_rst_stream (size_t payload_buff_size,
                         uint8_t *restrict payload_buff,
                         union mhd_H2FrameUnion *restrict frame_info)
{
  struct mhd_H2FrameRstStreamInfo *const rst_stream =
    &(frame_info->rst_stream);
  uint_fast32_t load_buff32b;

  mhd_assert (mhd_H2_FRAME_RST_STREAM_ID == rst_stream->type);

  if (0u == rst_stream->stream_id)
    return mhd_H2_F_DEC_CONN_ERR_PROT;
  if (4u != rst_stream->length)
    return mhd_H2_F_DEC_STREAM_ERR_F_SIZE;
  if (4u > payload_buff_size)
    return mhd_H2_F_DEC_F_HEADER_INCOMPLETE;

  load_buff32b = mhd_GET_32BIT_BE_UNALIGN (payload_buff + 0u);
  rst_stream->error_code = (enum mhd_H2ErrorCode) load_buff32b;

  return mhd_H2_F_DEC_OK;
}


/**
 * Decode a SETTINGS frame.
 *
 * Validates stream identifier (must be zero), ACK semantics and that the
 * length is a multiple of 6; exposes raw SETTINGS pairs via @a frame_payload
 *
 * @param payload_buff_size the available input bytes in the @a payload_buff
 * @param payload_buff the pointer to the beginning of the frame payload
 * @param flags the frame flags byte
 * @param[in,out] frame_info the frame information;
 *                           updated with SETTINGS details
 * @param[out] frame_payload set to the contiguous sequence of 6-byte pairs
 * @return #mhd_H2_F_DEC_OK on success,
 *         or a relevant decode error code
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (2,1)
MHD_FN_PAR_INOUT_ (4) MHD_FN_PAR_OUT_ (5)
enum mhd_H2FrameDecodeResult
frame_decode_settings (size_t payload_buff_size,
                       uint8_t *restrict payload_buff,
                       uint_least8_t flags,
                       union mhd_H2FrameUnion *restrict frame_info,
                       struct mhd_Buffer *restrict frame_payload)
{
  struct mhd_H2FrameSettingsInfo *const settings =
    &(frame_info->settings);

  mhd_assert (mhd_H2_FRAME_SETTINGS_ID == settings->type);

  if (0u != settings->stream_id)
    return mhd_H2_F_DEC_CONN_ERR_PROT;

  settings->ack = (0 != (flags & mhd_FFLAG_ACK));

  if (settings->ack && (0u != settings->length))
    return mhd_H2_F_DEC_CONN_ERR_F_SIZE;

  if (0u != (settings->length % 6))
    return mhd_H2_F_DEC_CONN_ERR_F_SIZE;

  if (payload_buff_size < settings->length)
    return mhd_H2_F_DEC_F_PAYLOAD_INCOMPLETE;

  frame_payload->data = (char *) payload_buff;
  frame_payload->size = settings->length;

  return mhd_H2_F_DEC_OK;
}


/**
 * Decode a PUSH_PROMISE frame.
 *
 * Validates sizes and exposes the header block fragment via @a frame_payload.
 *
 * @param payload_buff_size the available input bytes in the @a payload_buff
 * @param payload_buff the pointer to the beginning of the frame payload
 * @param flags the frame flags byte
 * @param[in,out] frame_info the frame information;
 *                           updated with PUSH_PROMISE details
 * @param[out] frame_payload set to the header block fragment slice within
 *                           payload
 * @return #mhd_H2_F_DEC_OK on success,
 *         or a relevant decode error code
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (2,1)
MHD_FN_PAR_INOUT_ (4) MHD_FN_PAR_OUT_ (5)
enum mhd_H2FrameDecodeResult
frame_decode_push_promise (size_t payload_buff_size,
                           uint8_t *restrict payload_buff,
                           uint_least8_t flags,
                           union mhd_H2FrameUnion *restrict frame_info,
                           struct mhd_Buffer *restrict frame_payload)
{
  struct mhd_H2FramePushPromiseInfo *const push_promise =
    &(frame_info->push_promise);
  size_t real_payload_pos;

  mhd_assert (mhd_H2_FRAME_PUSH_PROMISE_ID == push_promise->type);

  push_promise->padded =        (0 != (flags & mhd_FFLAG_PADDED));
  push_promise->end_headers =   (0 != (flags & mhd_FFLAG_END_HEADERS));

  real_payload_pos = 0u;
  if (push_promise->padded)
  {
    if (real_payload_pos >= push_promise->length)
      return mhd_H2_F_DEC_CONN_ERR_PROT;
    if (real_payload_pos >= payload_buff_size)
      return mhd_H2_F_DEC_F_HEADER_INCOMPLETE;
    push_promise->pad_length =
      (uint_least8_t) payload_buff[real_payload_pos++];
  }
  else
    push_promise->pad_length = 0u;

  if (real_payload_pos + 4u > push_promise->length)
    return mhd_H2_F_DEC_CONN_ERR_PROT;
  if (real_payload_pos + 4u > payload_buff_size)
    return mhd_H2_F_DEC_F_HEADER_INCOMPLETE;

  push_promise->promised_stream_id =
    (uint_least32_t)
    (mhd_GET_32BIT_BE_UNALIGN (payload_buff + real_payload_pos)
     & mhd_H2_STREAM_ID_MASK);
  real_payload_pos += 4u;

  if (push_promise->length < (real_payload_pos + push_promise->pad_length))
    return mhd_H2_F_DEC_CONN_ERR_PROT;
  if (0u == push_promise->stream_id)
    return mhd_H2_F_DEC_CONN_ERR_PROT;
  if (0u == push_promise->promised_stream_id)
    return mhd_H2_F_DEC_CONN_ERR_PROT;
  if (payload_buff_size < push_promise->length)
    return mhd_H2_F_DEC_F_PAYLOAD_INCOMPLETE;

  frame_payload->data = (char *) payload_buff + real_payload_pos;
  frame_payload->size =
    (size_t)
    (push_promise->length - real_payload_pos - push_promise->pad_length);

  return mhd_H2_F_DEC_OK;
}


/**
 * Decode a PING frame.
 *
 * Validates stream identifier (must be zero) and fixed size (8).
 *
 * @param payload_buff_size the available input bytes in the @a payload_buff
 * @param payload_buff the pointer to the beginning of the frame payload
 * @param flags the frame flags byte
 * @param[in,out] frame_info the frame information;
 *                           updated with PING details
 * @return #mhd_H2_F_DEC_OK on success,
 *         or a relevant decode error code
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (2,1)
MHD_FN_PAR_INOUT_ (4)
enum mhd_H2FrameDecodeResult
frame_decode_ping (size_t payload_buff_size,
                   uint8_t *restrict payload_buff,
                   uint_least8_t flags,
                   union mhd_H2FrameUnion *restrict frame_info)
{
  struct mhd_H2FramePingInfo *const ping =
    &(frame_info->ping);

  mhd_assert (mhd_H2_FRAME_PING_ID == ping->type);

  ping->ack = (0 != (flags & mhd_FFLAG_ACK));

  if (0u != ping->stream_id)
    return mhd_H2_F_DEC_CONN_ERR_PROT;
  if (8u != ping->length)
    return mhd_H2_F_DEC_CONN_ERR_F_SIZE;
  if (8u > payload_buff_size)
    return mhd_H2_F_DEC_F_HEADER_INCOMPLETE;

  memcpy (ping->opaque_data,
          payload_buff,
          8u);

  return mhd_H2_F_DEC_OK;
}


/**
 * Decode a GOAWAY frame.
 *
 * Validates stream identifier (must be zero) and that length is at least 8.
 *
 * @param payload_buff_size the available input bytes in the @a payload_buff
 * @param payload_buff the pointer to the beginning of the frame payload
 * @param[in,out] frame_info the frame information;
 *                           updated with GOAWAY details
 * @param[out] frame_payload set to the optional debug data slice
 * @return #mhd_H2_F_DEC_OK on success,
 *         or a relevant decode error code
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (2,1)
MHD_FN_PAR_INOUT_ (3) MHD_FN_PAR_OUT_ (4)
enum mhd_H2FrameDecodeResult
frame_decode_goaway (size_t payload_buff_size,
                     uint8_t *restrict payload_buff,
                     union mhd_H2FrameUnion *restrict frame_info,
                     struct mhd_Buffer *restrict frame_payload)
{
  struct mhd_H2FrameGoawayInfo *const goaway =
    &(frame_info->goaway);
  size_t real_payload_pos;
  uint_fast32_t load_buff32b;

  mhd_assert (mhd_H2_FRAME_GOAWAY_ID == goaway->type);

  if (0u != goaway->stream_id)
    return mhd_H2_F_DEC_CONN_ERR_PROT;
  if (8u > goaway->length)
    return mhd_H2_F_DEC_CONN_ERR_F_SIZE;
  if (8u > payload_buff_size)
    return mhd_H2_F_DEC_F_HEADER_INCOMPLETE;

  real_payload_pos = 0u;
  goaway->last_stream_id =
    (uint_least32_t)
    (mhd_GET_32BIT_BE_UNALIGN (payload_buff + real_payload_pos)
     & mhd_MASK_31BITS);
  real_payload_pos += 4u;
  load_buff32b = mhd_GET_32BIT_BE_UNALIGN (payload_buff + real_payload_pos);
  goaway->error_code = (enum mhd_H2ErrorCode) load_buff32b;
  real_payload_pos += 4u;

  frame_payload->data = (char *) payload_buff + real_payload_pos;
  frame_payload->size = (size_t) (goaway->length - real_payload_pos);

  return mhd_H2_F_DEC_OK;
}


/**
 * Decode a WINDOW_UPDATE frame.
 *
 * Validates fixed size (4) and that the increment is non-zero.
 *
 * @param payload_buff_size the available input bytes in the @a payload_buff
 * @param payload_buff the pointer to the beginning of the frame payload
 * @param[in,out] frame_info the frame information;
 *                           updated with WINDOW_UPDATE details
 * @return #mhd_H2_F_DEC_OK on success,
 *         or a relevant decode error code
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (2,1)
MHD_FN_PAR_INOUT_ (3)
enum mhd_H2FrameDecodeResult
frame_decode_window_update (size_t payload_buff_size,
                            uint8_t *restrict payload_buff,
                            union mhd_H2FrameUnion *restrict frame_info)
{
  struct mhd_H2FrameWindowUpdateInfo *const window_update =
    &(frame_info->window_update);

  mhd_assert (mhd_H2_FRAME_WINDOW_UPDATE_ID == window_update->type);

  if (4u != window_update->length)
    return mhd_H2_F_DEC_CONN_ERR_F_SIZE;
  if (4u > payload_buff_size)
    return mhd_H2_F_DEC_F_HEADER_INCOMPLETE;

  window_update->window_size_increment =
    (uint_least32_t)
    (mhd_GET_32BIT_BE_UNALIGN (payload_buff + 0u) & mhd_MASK_31BITS);

  if (0u == window_update->window_size_increment)
    return (0u == window_update->stream_id) ?
           mhd_H2_F_DEC_CONN_ERR_PROT : mhd_H2_F_DEC_STREAM_ERR_PROT;

  return mhd_H2_F_DEC_OK;
}


/**
 * Decode a CONTINUATION frame.
 *
 * @param payload_buff_size the available input bytes in the @a payload_buff
 * @param payload_buff the pointer to the beginning of the frame payload
 * @param flags the frame flags byte
 * @param[in,out] frame_info the frame information;
 *                           updated with CONTINUATION details
 * @param[out] frame_payload set to the continuation fragment slice
 * @return #mhd_H2_F_DEC_OK on success,
 *         or a relevant decode error code
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (2,1)
MHD_FN_PAR_INOUT_ (4) MHD_FN_PAR_OUT_ (5)
enum mhd_H2FrameDecodeResult
frame_decode_continuation (size_t payload_buff_size,
                           uint8_t *restrict payload_buff,
                           uint_least8_t flags,
                           union mhd_H2FrameUnion *restrict frame_info,
                           struct mhd_Buffer *restrict frame_payload)
{
  struct mhd_H2FrameContinuationInfo *const continuation =
    &(frame_info->continuation);

  mhd_assert (mhd_H2_FRAME_CONTINUATION_ID == continuation->type);

  if (0u == continuation->stream_id)
    return mhd_H2_F_DEC_CONN_ERR_PROT;

  continuation->end_headers = (0 != (flags & mhd_FFLAG_END_HEADERS));

  if (payload_buff_size < continuation->length)
    return mhd_H2_F_DEC_F_PAYLOAD_INCOMPLETE;

  frame_payload->data = (char *) payload_buff;
  frame_payload->size = continuation->length;

  return mhd_H2_F_DEC_OK;
}


/**
 * Decode an unknown frame type as opaque bytes.
 *
 * Exposes the entire payload via @a frame_payload if fully available
 *
 * @param payload_buff_size the available input bytes in the @a payload_buff
 * @param payload_buff the pointer to the beginning of the frame payload
 * @param[in,out] frame_info the frame information;
 *                           only selector fields are used
 * @param[out] frame_payload set to the raw payload slice
 * @return #mhd_H2_F_DEC_OK on success,
 *         or a relevant decode error code
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (2,1)
MHD_FN_PAR_INOUT_ (3) MHD_FN_PAR_OUT_ (4)
enum mhd_H2FrameDecodeResult
frame_decode_unknown_type (size_t payload_buff_size,
                           uint8_t *restrict payload_buff,
                           union mhd_H2FrameUnion *restrict frame_info,
                           struct mhd_Buffer *restrict frame_payload)
{
  if (payload_buff_size < frame_info->selector.length)
    return mhd_H2_F_DEC_F_PAYLOAD_INCOMPLETE;

  frame_payload->data = (char *) payload_buff;
  frame_payload->size = frame_info->selector.length;

  return mhd_H2_F_DEC_OK;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (2,1)
MHD_FN_PAR_OUT_ (4) MHD_FN_PAR_OUT_ (5) enum mhd_H2FrameDecodeResult
mhd_h2_frame_decode (size_t buff_size,
                     uint8_t *restrict buff,
                     uint_least32_t max_frame_size,
                     union mhd_H2FrameUnion *restrict frame_info,
                     struct mhd_Buffer *restrict frame_payload)
{
  uint_fast32_t len_and_type;
  uint_least32_t length;
  uint_least8_t type;
  uint_least8_t flags;
  uint_least32_t stream_id;

  if (mhd_H2_FR_SIZE_MIN > buff_size)
    return mhd_H2_F_DEC_F_HEADER_INCOMPLETE;

  len_and_type = mhd_GET_32BIT_BE_UNALIGN (buff + 0u);

  length = (uint_least32_t) (len_and_type >> 8u);
  type = (uint_least8_t) (len_and_type & 0xFFu);

  flags = buff[4];

  stream_id = (mhd_GET_32BIT_BE_UNALIGN (buff + 5u) & mhd_H2_STREAM_ID_MASK);

  frame_info->selector.length = length;
  frame_info->selector.type = (enum mhd_H2FrameIDs) type;
  frame_info->selector.stream_id = stream_id;

  if (max_frame_size < length)
    return mhd_H2_F_DEC_CONN_ERR_F_SIZE;

  switch ((enum mhd_H2FrameIDs) type)
  {
  case mhd_H2_FRAME_IDS_DATA_ID:
    return frame_decode_data (buff_size - mhd_H2_FR_SIZE_MIN,
                              buff + mhd_H2_FR_SIZE_MIN,
                              flags,
                              frame_info,
                              frame_payload);
  case mhd_H2_FRAME_IDS_HEADERS_ID:
    return frame_decode_headers (buff_size - mhd_H2_FR_SIZE_MIN,
                                 buff + mhd_H2_FR_SIZE_MIN,
                                 flags,
                                 frame_info,
                                 frame_payload);
  case mhd_H2_FRAME_IDS_PRIORITY_ID:
    frame_payload->size = 0u;
    frame_payload->data = NULL;
    return frame_decode_priority (buff_size - mhd_H2_FR_SIZE_MIN,
                                  buff + mhd_H2_FR_SIZE_MIN,
                                  frame_info);
  case mhd_H2_FRAME_IDS_RST_STREAM_ID:
    frame_payload->size = 0u;
    frame_payload->data = NULL;
    return frame_decode_rst_stream (buff_size - mhd_H2_FR_SIZE_MIN,
                                    buff + mhd_H2_FR_SIZE_MIN,
                                    frame_info);
  case mhd_H2_FRAME_IDS_SETTINGS_ID:
    return frame_decode_settings (buff_size - mhd_H2_FR_SIZE_MIN,
                                  buff + mhd_H2_FR_SIZE_MIN,
                                  flags,
                                  frame_info,
                                  frame_payload);
  case mhd_H2_FRAME_IDS_PUSH_PROMISE_ID:
    return frame_decode_push_promise (buff_size - mhd_H2_FR_SIZE_MIN,
                                      buff + mhd_H2_FR_SIZE_MIN,
                                      flags,
                                      frame_info,
                                      frame_payload);
  case mhd_H2_FRAME_IDS_PING_ID:
    frame_payload->size = 0u;
    frame_payload->data = NULL;
    return frame_decode_ping (buff_size - mhd_H2_FR_SIZE_MIN,
                              buff + mhd_H2_FR_SIZE_MIN,
                              flags,
                              frame_info);
  case mhd_H2_FRAME_IDS_GOAWAY_ID:
    return frame_decode_goaway (buff_size - mhd_H2_FR_SIZE_MIN,
                                buff + mhd_H2_FR_SIZE_MIN,
                                frame_info,
                                frame_payload);
  case mhd_H2_FRAME_IDS_WINDOW_UPDATE_ID:
    frame_payload->size = 0u;
    frame_payload->data = NULL;
    return frame_decode_window_update (buff_size - mhd_H2_FR_SIZE_MIN,
                                       buff + mhd_H2_FR_SIZE_MIN,
                                       frame_info);
  case mhd_H2_FRAME_IDS_CONTINUATION_ID:
    return frame_decode_continuation (buff_size - mhd_H2_FR_SIZE_MIN,
                                      buff + mhd_H2_FR_SIZE_MIN,
                                      flags,
                                      frame_info,
                                      frame_payload);
  default:
    break;
  }
  return frame_decode_unknown_type (buff_size - mhd_H2_FR_SIZE_MIN,
                                    buff + mhd_H2_FR_SIZE_MIN,
                                    frame_info,
                                    frame_payload);
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_ (1)
size_t
mhd_h2_frame_get_extra_hdr_size (
  const union mhd_H2FrameUnion *restrict frame_info)
{
  size_t extra;
  extra = 0u;
  switch (frame_info->selector.type)
  {
  case mhd_H2_FRAME_IDS_DATA_ID:
    if (frame_info->data.padded)
      extra += 1u;
    break;
  case mhd_H2_FRAME_IDS_HEADERS_ID:
    if (frame_info->headers.padded)
      extra += 1u;
    if (frame_info->headers.priority)
      extra += 5u;
    break;
  case mhd_H2_FRAME_IDS_PRIORITY_ID:
    extra += 5u;
    break;
  case mhd_H2_FRAME_IDS_RST_STREAM_ID:
    extra += 4u;
    break;
  case mhd_H2_FRAME_IDS_SETTINGS_ID:
    break;
  case mhd_H2_FRAME_IDS_PUSH_PROMISE_ID:
    if (frame_info->push_promise.padded)
      extra += 1u;
    extra += 4u;
    break;
  case mhd_H2_FRAME_IDS_PING_ID:
    extra += mhd_H2_FR_FIXED_LEN_PING;
    break;
  case mhd_H2_FRAME_IDS_GOAWAY_ID:
    extra += 8u;
    break;
  case mhd_H2_FRAME_IDS_WINDOW_UPDATE_ID:
    extra += mhd_H2_FR_FIXED_LEN_WINDOW_UPDATE;
    break;
  case mhd_H2_FRAME_IDS_CONTINUATION_ID:
    break;
  default:
    break;
  }
  return extra;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_ (1)
size_t
mhd_h2_frame_get_padding_size (
  const union mhd_H2FrameUnion *restrict frame_info)
{
  switch (frame_info->selector.type)
  {
  case mhd_H2_FRAME_IDS_DATA_ID:
    mhd_assert ((0u == frame_info->data.pad_length) || \
                frame_info->data.padded);
    return frame_info->data.pad_length;
  case mhd_H2_FRAME_IDS_HEADERS_ID:
    mhd_assert ((0u == frame_info->headers.pad_length) || \
                frame_info->headers.padded);
    return frame_info->headers.pad_length;
  case mhd_H2_FRAME_IDS_PUSH_PROMISE_ID:
    mhd_assert ((0u == frame_info->push_promise.pad_length) || \
                frame_info->push_promise.padded);
    return frame_info->push_promise.pad_length;
  case mhd_H2_FRAME_IDS_PRIORITY_ID:
  case mhd_H2_FRAME_IDS_RST_STREAM_ID:
  case mhd_H2_FRAME_IDS_SETTINGS_ID:
  case mhd_H2_FRAME_IDS_PING_ID:
  case mhd_H2_FRAME_IDS_GOAWAY_ID:
  case mhd_H2_FRAME_IDS_WINDOW_UPDATE_ID:
  case mhd_H2_FRAME_IDS_CONTINUATION_ID:
  default:
    break;
  }
  return 0u;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1) size_t
mhd_h2_frame_set_payload_size (union mhd_H2FrameUnion *restrict frame_info,
                               size_t payload_size)
{
  uint_least32_t fr_length;

  mhd_assert (frame_info->selector.length == \
              (frame_info->selector.length & mhd_H2_FR_LENGTH_MASK));
  mhd_assert (frame_info->selector.type == \
              (((uint_least64_t) frame_info->selector.type) & 0xFFu));
#ifndef NDEBUG
  switch (frame_info->selector.type)
  {
  case mhd_H2_FRAME_IDS_PRIORITY_ID:
  case mhd_H2_FRAME_IDS_RST_STREAM_ID:
  case mhd_H2_FRAME_IDS_PING_ID:
  case mhd_H2_FRAME_IDS_WINDOW_UPDATE_ID:
    mhd_assert (0u == payload_size);
    break;
  case mhd_H2_FRAME_IDS_DATA_ID:
  case mhd_H2_FRAME_IDS_HEADERS_ID:
  case mhd_H2_FRAME_IDS_SETTINGS_ID:
  case mhd_H2_FRAME_IDS_PUSH_PROMISE_ID:
  case mhd_H2_FRAME_IDS_GOAWAY_ID:
  case mhd_H2_FRAME_IDS_CONTINUATION_ID:
  default:
    break;
  }
#endif /* ! NDEBUG */
  mhd_assert (frame_info->selector.length == \
              (frame_info->selector.length & mhd_H2_FR_LENGTH_MASK));

  fr_length = (uint_least32_t)
              (mhd_h2_frame_get_extra_hdr_size (frame_info)
               + mhd_h2_frame_get_padding_size (frame_info)
               + payload_size);

  mhd_assert (fr_length == (fr_length & mhd_H2_FR_LENGTH_MASK));

  frame_info->selector.length = fr_length;

  return (size_t) (mhd_H2_FR_HDR_BASE_SIZE + (size_t) fr_length);
}


/**
 * Encode DATA extra header and flags.
 *
 * Does not write payload or trailing pad bytes.
 *
 * @param frame_info the DATA frame information
 * @param[out] flags the pointer to the header flags byte to update
 * @param out_extra_hdr_size available space in @a out_extra_hdr
 * @param[out] out_extra_hdr the output buffer for extra header bytes
 * @return the size of the frame basic header plus the size of the frame extra
 *         header written,
 *         or 0 if the output buffer is too small
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_ (1)
MHD_FN_PAR_OUT_ (2) MHD_FN_PAR_OUT_SIZE_ (4,3) size_t
frame_hdr_encode_data (const union mhd_H2FrameUnion *restrict frame_info,
                       uint8_t *restrict flags,
                       const size_t out_extra_hdr_size,
                       uint8_t *restrict out_extra_hdr)
{
  const struct mhd_H2FrameDataInfo *const data =
    &(frame_info->data);
  size_t extra_hdr_pos;

  mhd_assert (mhd_H2_FRAME_DATA_ID == data->type);

  mhd_assert (0u != data->stream_id);
  mhd_assert ((0u == data->pad_length) ||
              (data->padded));

  *flags =  (uint8_t) (data->padded     ? mhd_FFLAG_PADDED     : 0u);
  *flags |= (uint8_t) (data->end_stream ? mhd_FFLAG_END_STREAM : 0u);

  extra_hdr_pos = 0u;
  if (data->padded)
  {
    if (out_extra_hdr_size <= extra_hdr_pos)
      return 0u;
    out_extra_hdr[extra_hdr_pos++] = (uint8_t) data->pad_length;
  }

  mhd_assert (data->length >= (extra_hdr_pos + data->pad_length));

  return mhd_H2_FR_HDR_BASE_SIZE + extra_hdr_pos;
}


/**
 * Encode HEADERS extra header and flags.
 *
 * @param frame_info the HEADERS frame information
 * @param[out] flags the pointer to the header flags byte to update
 * @param out_extra_hdr_size available space in @a out_extra_hdr
 * @param[out] out_extra_hdr the output buffer for extra header bytes
 * @return the size of the frame basic header plus the size of the frame extra
 *         header written,
 *         or 0 if the output buffer is too small
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_ (1)
MHD_FN_PAR_OUT_ (2) MHD_FN_PAR_OUT_SIZE_ (4,3) size_t
frame_hdr_encode_headers (const union mhd_H2FrameUnion *restrict frame_info,
                          uint8_t *restrict flags,
                          const size_t out_extra_hdr_size,
                          uint8_t *restrict out_extra_hdr)
{
  const struct mhd_H2FrameHeadersInfo *const headers =
    &(frame_info->headers);
  size_t extra_hdr_pos;

  mhd_assert (mhd_H2_FRAME_HEADERS_ID == headers->type);

  mhd_assert (0u != headers->stream_id);
  mhd_assert ((0u == headers->pad_length) ||
              (headers->padded));

  *flags =  (uint8_t) (headers->priority ?    mhd_FFLAG_PRIORITY    : 0u);
  *flags |= (uint8_t) (headers->padded ?      mhd_FFLAG_PADDED      : 0u);
  *flags |= (uint8_t) (headers->end_headers ? mhd_FFLAG_END_HEADERS : 0u);
  *flags |= (uint8_t) (headers->end_stream ?  mhd_FFLAG_END_STREAM  : 0u);

  extra_hdr_pos = 0u;
  if (headers->padded)
  {
    if (out_extra_hdr_size <= extra_hdr_pos)
      return 0u;
    out_extra_hdr[extra_hdr_pos++] = (uint8_t) headers->pad_length;
  }

  if (headers->priority)
  {
    uint32_t excl_n_strm_dep;
    if (out_extra_hdr_size < (extra_hdr_pos + 5u))
      return 0u;

    excl_n_strm_dep = (uint32_t)
                      (headers->stream_dependency & mhd_H2_STREAM_ID_MASK);
    excl_n_strm_dep |= (uint32_t)
                       (headers->exclusive ? mhd_FFLAG_HEADERS_EXCLUSIVE : 0u);
    mhd_PUT_32BIT_BE_UNALIGN (out_extra_hdr + extra_hdr_pos,
                              excl_n_strm_dep);
    extra_hdr_pos += 4u;

    /* Use "on-wire" 'weight' format. */
    out_extra_hdr[extra_hdr_pos++] = (uint8_t) headers->weight;
  }

  mhd_assert (headers->length >= (extra_hdr_pos + headers->pad_length));

  return mhd_H2_FR_HDR_BASE_SIZE + extra_hdr_pos;
}


/**
 * Encode PRIORITY payload into the extra header area.
 *
 * @param frame_info the PRIORITY frame information
 * @param out_extra_hdr_size available space in @a out_extra_hdr
 * @param[out] out_extra_hdr the output buffer for the 5-byte payload
 * @return the size of the frame basic header plus the size of the frame extra
 *         header written,
 *         or 0 if the output buffer is too small
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_ (1)
MHD_FN_PAR_OUT_SIZE_ (3,2) size_t
frame_hdr_encode_priority (const union mhd_H2FrameUnion *restrict frame_info,
                           const size_t out_extra_hdr_size,
                           uint8_t *restrict out_extra_hdr)
{
  const struct mhd_H2FramePriorityInfo *const priority =
    &(frame_info->priority);
  uint32_t excl_n_strm_dep;
  size_t extra_hdr_pos;

  mhd_assert (mhd_H2_FRAME_PRIORITY_ID == priority->type);

  mhd_assert (0u != priority->stream_id);
  mhd_assert (priority->stream_id != priority->stream_dependency);

  extra_hdr_pos = 0u;

  if (out_extra_hdr_size < (extra_hdr_pos + 5u))
    return 0u;

  excl_n_strm_dep = (uint32_t)
                    (priority->stream_dependency & mhd_H2_STREAM_ID_MASK);
  excl_n_strm_dep |= (uint32_t)
                     (priority->exclusive ? mhd_FFLAG_HEADERS_EXCLUSIVE : 0u);
  mhd_PUT_32BIT_BE_UNALIGN (out_extra_hdr + extra_hdr_pos,
                            excl_n_strm_dep);
  extra_hdr_pos += 4u;

  /* Use "on-wire" 'weight' format. */
  out_extra_hdr[extra_hdr_pos++] = (uint8_t) priority->weight;

  mhd_assert (priority->length == extra_hdr_pos);
  mhd_assert (mhd_H2_FR_FIXED_LEN_PRIORITY == extra_hdr_pos);

  return mhd_H2_FR_HDR_BASE_SIZE + extra_hdr_pos;
}


/**
 * Encode RST_STREAM payload into the extra header area.
 *
 * @param frame_info the RST_STREAM frame information
 * @param out_extra_hdr_size available space in @a out_extra_hdr
 * @param[out] out_extra_hdr the output buffer for the 4-byte payload
 * @return the size of the frame basic header plus the size of the frame extra
 *         header written,
 *         or 0 if the output buffer is too small
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_ (1)
MHD_FN_PAR_OUT_SIZE_ (3,2) size_t
frame_hdr_encode_rst_stream (const union mhd_H2FrameUnion *restrict frame_info,
                             const size_t out_extra_hdr_size,
                             uint8_t *restrict out_extra_hdr)
{
  const struct mhd_H2FrameRstStreamInfo *const rst_stream =
    &(frame_info->rst_stream);
  size_t extra_hdr_pos;

  mhd_assert (mhd_H2_FRAME_RST_STREAM_ID == rst_stream->type);

  mhd_assert (0u != rst_stream->stream_id);

  extra_hdr_pos = 0u;

  if (out_extra_hdr_size < (extra_hdr_pos + 4u))
    return 0u;

  mhd_PUT_32BIT_BE_UNALIGN (out_extra_hdr + extra_hdr_pos,
                            (uint32_t) rst_stream->error_code);
  extra_hdr_pos += 4u;

  mhd_assert (rst_stream->length == extra_hdr_pos);
  mhd_assert (mhd_H2_FR_FIXED_LEN_RST_STREAM == extra_hdr_pos);

  return mhd_H2_FR_HDR_BASE_SIZE + extra_hdr_pos;
}


/**
 * Encode SETTINGS header flags.
 *
 * SETTINGS has no extra header bytes. Any SETTINGS parameters belong to the
 * payload and are not written by this function.
 *
 * @param frame_info the SETTINGS frame information
 * @param[out] flags the pointer to the header flags byte to update
 * @return the size of the frame basic header plus the size of the frame extra
 *         header written
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_ (1)
MHD_FN_PAR_OUT_ (2) size_t
frame_hdr_encode_settings (const union mhd_H2FrameUnion *restrict frame_info,
                           uint8_t *restrict flags)
{
  const struct mhd_H2FrameSettingsInfo *const settings =
    &(frame_info->settings);

  mhd_assert (mhd_H2_FRAME_SETTINGS_ID == settings->type);

  mhd_assert (0u == settings->stream_id);
  mhd_assert ((! settings->ack) || (0u == settings->length));
  mhd_assert (0u == (settings->length % 6));

  *flags = (uint8_t) (settings->ack ? mhd_FFLAG_ACK : 0u);

  return mhd_H2_FR_HDR_BASE_SIZE;
}


/**
 * Encode PUSH_PROMISE extra header and flags.
 *
 * This function does not write the header block fragment or trailing
 * pad bytes.
 *
 * @param frame_info the PUSH_PROMISE frame information
 * @param[out] flags the pointer to the header flags byte to update
 * @param out_extra_hdr_size available space in @a out_extra_hdr
 * @param[out] out_extra_hdr the output buffer for extra header bytes
 * @return the size of the frame basic header plus the size of the frame extra
 *         header written,
 *         or 0 if the output buffer is too small
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_ (1)
MHD_FN_PAR_OUT_ (2) MHD_FN_PAR_OUT_SIZE_ (4,3) size_t
frame_hdr_encode_push_promise (
  const union mhd_H2FrameUnion *restrict frame_info,
  uint8_t *restrict flags,
  const size_t out_extra_hdr_size,
  uint8_t *restrict out_extra_hdr)
{
  const struct mhd_H2FramePushPromiseInfo *const push_promise =
    &(frame_info->push_promise);
  size_t extra_hdr_pos;

  mhd_assert (mhd_H2_FRAME_PUSH_PROMISE_ID == push_promise->type);

  mhd_assert (0u != push_promise->stream_id);
  mhd_assert ((0u == push_promise->pad_length) ||
              (push_promise->padded));
  mhd_assert (0u != push_promise->promised_stream_id);
  mhd_assert (push_promise->promised_stream_id ==
              (push_promise->promised_stream_id & mhd_H2_STREAM_ID_MASK));

  *flags =  (uint8_t) (push_promise->padded ?      mhd_FFLAG_PADDED      : 0u);
  *flags |= (uint8_t) (push_promise->end_headers ? mhd_FFLAG_END_HEADERS : 0u);

  extra_hdr_pos = 0u;
  if (push_promise->padded)
  {
    if (out_extra_hdr_size <= extra_hdr_pos)
      return 0u;
    out_extra_hdr[extra_hdr_pos++] = (uint8_t) push_promise->pad_length;
  }

  if (out_extra_hdr_size < (extra_hdr_pos + 4u))
    return 0u;

  mhd_PUT_32BIT_BE_UNALIGN (out_extra_hdr + extra_hdr_pos,
                            (uint32_t) (push_promise->promised_stream_id
                                        & mhd_H2_STREAM_ID_MASK));
  extra_hdr_pos += 4u;

  mhd_assert (push_promise->length >=
              (extra_hdr_pos + push_promise->pad_length));

  return mhd_H2_FR_HDR_BASE_SIZE + extra_hdr_pos;
}


/**
 * Encode PING payload and flags.
 *
 * @param frame_info the PING frame information
 * @param[out] flags the pointer to the header flags byte to update
 * @param out_extra_hdr_size available space in @a out_extra_hdr
 * @param[out] out_extra_hdr the output buffer for the 8-byte payload
 * @return the size of the frame basic header plus the size of the frame extra
 *         header written,
 *         or 0 if the output buffer is too small
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_ (1)
MHD_FN_PAR_OUT_ (2) MHD_FN_PAR_OUT_SIZE_ (4,3) size_t
frame_hdr_encode_ping (const union mhd_H2FrameUnion *restrict frame_info,
                       uint8_t *restrict flags,
                       const size_t out_extra_hdr_size,
                       uint8_t *restrict out_extra_hdr)
{
  const struct mhd_H2FramePingInfo *const ping =
    &(frame_info->ping);
  size_t extra_hdr_pos;

  mhd_assert (mhd_H2_FRAME_PING_ID == ping->type);

  mhd_assert (0u == ping->stream_id);

  *flags =  (uint8_t) (ping->ack ? mhd_FFLAG_ACK : 0u);

  extra_hdr_pos = 0u;

  if (out_extra_hdr_size < (extra_hdr_pos + 8u))
    return 0u;

  memcpy (out_extra_hdr,
          ping->opaque_data,
          8u);
  extra_hdr_pos += 8u;

  mhd_assert (ping->length == extra_hdr_pos);
  mhd_assert (mhd_H2_FR_FIXED_LEN_PING == extra_hdr_pos);

  return mhd_H2_FR_HDR_BASE_SIZE + extra_hdr_pos;
}


/**
 * Encode GOAWAY fixed fields into the extra header area.
 *
 * Optional debug data belongs to the payload and is not written by this
 * function.
 *
 * @param frame_info the GOAWAY frame information
 * @param out_extra_hdr_size available space in @a out_extra_hdr
 * @param[out] out_extra_hdr the output buffer for the 8-byte fixed fields
 * @return the size of the frame basic header plus the size of the frame extra
 *         header written,
 *         or 0 if the output buffer is too small
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_ (1)
MHD_FN_PAR_OUT_SIZE_ (3,2) size_t
frame_hdr_encode_goaway (const union mhd_H2FrameUnion *restrict frame_info,
                         const size_t out_extra_hdr_size,
                         uint8_t *restrict out_extra_hdr)
{
  const struct mhd_H2FrameGoawayInfo *const goaway =
    &(frame_info->goaway);
  size_t extra_hdr_pos;

  mhd_assert (mhd_H2_FRAME_GOAWAY_ID == goaway->type);

  mhd_assert (0u == goaway->stream_id);

  extra_hdr_pos = 0u;

  if (out_extra_hdr_size < (extra_hdr_pos + 8u))
    return 0u;

  mhd_PUT_32BIT_BE_UNALIGN (out_extra_hdr + extra_hdr_pos,
                            (uint32_t) (goaway->last_stream_id
                                        & mhd_H2_STREAM_ID_MASK));
  extra_hdr_pos += 4u;

  mhd_PUT_32BIT_BE_UNALIGN (out_extra_hdr + extra_hdr_pos,
                            (uint32_t) goaway->error_code);
  extra_hdr_pos += 4u;

  mhd_assert (goaway->length >= extra_hdr_pos);

  return mhd_H2_FR_HDR_BASE_SIZE + extra_hdr_pos;
}


/**
 * Encode WINDOW_UPDATE payload into the extra header area.
 *
 * @param frame_info the WINDOW_UPDATE frame information
 * @param out_extra_hdr_size available space in @a out_extra_hdr
 * @param[out] out_extra_hdr the output buffer for the 4-byte payload
 * @return the size of the frame basic header plus the size of the frame extra
 *         header written,
 *         or 0 if the output buffer is too small
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_ (1)
MHD_FN_PAR_OUT_SIZE_ (3,2) size_t
frame_hdr_encode_window_update (
  const union mhd_H2FrameUnion *restrict frame_info,
  const size_t out_extra_hdr_size,
  uint8_t *restrict out_extra_hdr)
{
  const struct mhd_H2FrameWindowUpdateInfo *const window_update =
    &(frame_info->window_update);
  size_t extra_hdr_pos;

  mhd_assert (mhd_H2_FRAME_WINDOW_UPDATE_ID == window_update->type);
  mhd_assert (0u != window_update->window_size_increment);
  mhd_assert (window_update->window_size_increment ==
              (window_update->window_size_increment & mhd_MASK_31BITS));

  extra_hdr_pos = 0u;

  if (out_extra_hdr_size < (extra_hdr_pos + 4u))
    return 0u;

  mhd_PUT_32BIT_BE_UNALIGN (out_extra_hdr + extra_hdr_pos,
                            (uint32_t) (window_update->window_size_increment
                                        & mhd_MASK_31BITS));
  extra_hdr_pos += 4u;

  mhd_assert (window_update->length >= extra_hdr_pos);
  mhd_assert (mhd_H2_FR_FIXED_LEN_WINDOW_UPDATE == extra_hdr_pos);

  return mhd_H2_FR_HDR_BASE_SIZE + extra_hdr_pos;
}


/**
 * Encode CONTINUATION header flags.
 *
 * CONTINUATION has no extra header bytes.
 *
 * @param frame_info the CONTINUATION frame information
 * @param[out] flags the pointer to the header flags byte to update
 * @return the size of the frame basic header plus the size of the frame extra
 *         header written
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_ (1)
MHD_FN_PAR_OUT_ (2) size_t
frame_hdr_encode_continuation (
  const union mhd_H2FrameUnion *restrict frame_info,
  uint8_t *restrict flags)
{
  const struct mhd_H2FrameContinuationInfo *const continuation =
    &(frame_info->continuation);

  mhd_assert (mhd_H2_FRAME_CONTINUATION_ID == continuation->type);

  mhd_assert (0u != continuation->stream_id);

  *flags = (uint8_t) (continuation->end_headers ? mhd_FFLAG_END_HEADERS : 0u);

  return mhd_H2_FR_HDR_BASE_SIZE;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_ (1)
MHD_FN_PAR_OUT_SIZE_ (3,2) size_t
mhd_h2_frame_hdr_encode (const union mhd_H2FrameUnion *restrict frame_info,
                         size_t out_buff_size,
                         uint8_t *restrict out_buff)
{
  uint32_t len_and_type;

  mhd_assert (frame_info->selector.length == \
              (frame_info->selector.length & mhd_H2_FR_LENGTH_MASK));
  mhd_assert (frame_info->selector.type == \
              (((uint_least64_t) frame_info->selector.type) & 0xFFu));
  mhd_assert (frame_info->selector.stream_id == \
              (frame_info->selector.stream_id & mhd_H2_STREAM_ID_MASK));

  if (mhd_H2_FR_SIZE_MIN > out_buff_size)
    return 0u;

  len_and_type = (uint_least8_t) (frame_info->selector.type & 0xFFu);
  len_and_type |=
    (uint32_t)
    ((((uint_least32_t) frame_info->selector.length) << 8u)
     & mhd_H2_FR_LENGTH_MASK);

  mhd_PUT_32BIT_BE_UNALIGN (out_buff + 0u,
                            len_and_type);
  out_buff[4] = 0u; /* flags */
  mhd_PUT_32BIT_BE_UNALIGN (out_buff + 5u,
                            frame_info->selector.stream_id
                            & mhd_H2_STREAM_ID_MASK);

  switch (frame_info->selector.type)
  {
  case mhd_H2_FRAME_IDS_DATA_ID:
    return frame_hdr_encode_data (frame_info,
                                  out_buff + 4u,
                                  out_buff_size - mhd_H2_FR_HDR_BASE_SIZE,
                                  out_buff + mhd_H2_FR_HDR_BASE_SIZE);
  case mhd_H2_FRAME_IDS_HEADERS_ID:
    return frame_hdr_encode_headers (frame_info,
                                     out_buff + 4u,
                                     out_buff_size - mhd_H2_FR_HDR_BASE_SIZE,
                                     out_buff + mhd_H2_FR_HDR_BASE_SIZE);
  case mhd_H2_FRAME_IDS_PRIORITY_ID:
    return frame_hdr_encode_priority (frame_info,
                                      out_buff_size - mhd_H2_FR_HDR_BASE_SIZE,
                                      out_buff + mhd_H2_FR_HDR_BASE_SIZE);
  case mhd_H2_FRAME_IDS_RST_STREAM_ID:
    return frame_hdr_encode_rst_stream (frame_info,
                                        out_buff_size - mhd_H2_FR_HDR_BASE_SIZE,
                                        out_buff + mhd_H2_FR_HDR_BASE_SIZE);
  case mhd_H2_FRAME_IDS_SETTINGS_ID:
    return frame_hdr_encode_settings (frame_info,
                                      out_buff + 4u);
  case mhd_H2_FRAME_IDS_PUSH_PROMISE_ID:
    return
      frame_hdr_encode_push_promise (frame_info,
                                     out_buff + 4u,
                                     out_buff_size - mhd_H2_FR_HDR_BASE_SIZE,
                                     out_buff + mhd_H2_FR_HDR_BASE_SIZE);
  case mhd_H2_FRAME_IDS_PING_ID:
    return frame_hdr_encode_ping (frame_info,
                                  out_buff + 4u,
                                  out_buff_size - mhd_H2_FR_HDR_BASE_SIZE,
                                  out_buff + mhd_H2_FR_HDR_BASE_SIZE);
  case mhd_H2_FRAME_IDS_GOAWAY_ID:
    return frame_hdr_encode_goaway (frame_info,
                                    out_buff_size - mhd_H2_FR_HDR_BASE_SIZE,
                                    out_buff + mhd_H2_FR_HDR_BASE_SIZE);
  case mhd_H2_FRAME_IDS_WINDOW_UPDATE_ID:
    return
      frame_hdr_encode_window_update (frame_info,
                                      out_buff_size - mhd_H2_FR_HDR_BASE_SIZE,
                                      out_buff + mhd_H2_FR_HDR_BASE_SIZE);
  case mhd_H2_FRAME_IDS_CONTINUATION_ID:
    return
      frame_hdr_encode_continuation (frame_info,
                                     out_buff + 4u);
  default:
    break;
  }
  mhd_UNREACHABLE_D ("Unknown frame types should not be sent");
  return mhd_H2_FR_HDR_BASE_SIZE;
}
