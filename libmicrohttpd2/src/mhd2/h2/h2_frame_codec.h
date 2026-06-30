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
 * @file src/mhd2/h2/h2_frame_codec.h
 * @brief  Declarations of HTTP/2 frame decoding and encoding functions
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_H2_FRAME_CODEC_H
#define MHD_H2_FRAME_CODEC_H 1

#include "mhd_sys_options.h"

#include "sys_base_types.h"

#include "mhd_buffer.h"
#include "h2_frame_types.h"

#if defined(_MSC_FULL_VER)
#pragma warning(push)
/* Disable C4505 "unreferenced local function has been removed" */
#pragma warning(disable:4505)
#endif /* _MSC_FULL_VER */

/**
 * The basic (mandatory) HTTP/2 frame header size
 */
#define mhd_H2_FR_HDR_BASE_SIZE      (9u)

/**
 * The minimum HTTP/2 frame size
 */
#define mhd_H2_FR_SIZE_MIN   mhd_H2_FR_HDR_BASE_SIZE

/**
 * Maximum extra frame header size for known frame types.
 */
#define mhd_H2_FR_HDR_EXTRA_SIZE_MAX    (8u)

/**
 * Result of frame decode
 */
enum MHD_FIXED_ENUM_ mhd_H2FrameDecodeResult
{
  /**
   * Frame is successfully decoded
   */
  mhd_H2_F_DEC_OK = 0
  ,
  /**
   * Frame header is completely decoded, but frame payload is incomplete
   * Frame information is valid.
   */
  mhd_H2_F_DEC_F_PAYLOAD_INCOMPLETE
  ,
  /**
   * Frame header information is not complete.
   * Not enough data to decode frame header.
   */
  mhd_H2_F_DEC_F_HEADER_INCOMPLETE
  ,
  /**
   * Stream error of type "Frame size error"
   */
  mhd_H2_F_DEC_STREAM_ERR_F_SIZE
  ,
  /**
   * Stream error of type "Protocol error"
   */
  mhd_H2_F_DEC_STREAM_ERR_PROT
  ,
  /**
   * Connection error of type "Frame size error"
   */
  mhd_H2_F_DEC_CONN_ERR_F_SIZE
  ,
  /**
   * Connection error of type "Protocol error"
   */
  mhd_H2_F_DEC_CONN_ERR_PROT
};

#define mhd_H2_FRAME_DEC_ERR_IS_HARD(res) \
        (mhd_H2_F_DEC_F_HEADER_INCOMPLETE < (res))

/**
 * Decode an HTTP/2 frame.
 *
 * If result is #mhd_H2_F_DEC_OK or #mhd_H2_F_DEC_F_PAYLOAD_INCOMPLETE then
 * @a frame_info has full information about known frame types.
 * When result is not #mhd_H2_F_DEC_OK, #mhd_H2_F_DEC_F_PAYLOAD_INCOMPLETE or
 * #mhd_H2_F_DEC_F_HEADER_INCOMPLETE, the only valid data in the @a frame_info
 * is "length", "type" and "stream_id".
 * @param buff_size the size of the data available to decode
 * @param buff the data to decode
 * @param max_frame_size the maximum allowed frame size
 * @param[out] frame_info set to frame information
 * @param[out] frame_payload on successful decode set to the frame payload
 *                           size and location
 * @return #mhd_H2_F_DEC_OK on success,
 *         the error code otherwise
 */
MHD_INTERNAL enum mhd_H2FrameDecodeResult
mhd_h2_frame_decode (size_t buff_size,
                     uint8_t *restrict buff,
                     uint_least32_t max_frame_size,
                     union mhd_H2FrameUnion *restrict frame_info,
                     struct mhd_Buffer *restrict frame_payload)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_IN_SIZE_(2,1)
MHD_FN_PAR_OUT_(4) MHD_FN_PAR_OUT_ (5);


/**
 * Find an amount of extra bytes required to encode the HTTP/2 frame.
 *
 * Each frame is encoded to the basic frame header size
 * #mhd_H2_FR_HDR_BASE_SIZE plus optional extra bytes (depending on frame
 * flags), plus the frame payload (only for relevant frame types), plus
 * optional padding at the end.
 *
 * This function calculates the total extra bytes for known frame types based
 * on the frame header type and flags. This extra size is counted as a part
 * of the frame "length".
 * @param frame_info the information about the frame; only type and flags are
 *                   checked
 * @return the number of extra bytes needed to write the frame header, which is
 *         a value in range 0 .. #mhd_H2_FR_HDR_EXTRA_SIZE_MAX;
 *         does not include the basic frame header
 *         size #mhd_H2_FR_HDR_BASE_SIZE
 */
MHD_INTERNAL size_t
mhd_h2_frame_get_extra_hdr_size (
  const union mhd_H2FrameUnion *restrict frame_info)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_IN_ (1);


/**
 * Get the size of the padding (if any) set in the @a frame_info.
 *
 * The padding is counted as a part of frame "length".
 * @param frame_info the information about frame, only type, flags and
 *                   padding size (if any) are checked
 * @return the padding size used at the end of the frame
 */
MHD_INTERNAL size_t
mhd_h2_frame_get_padding_size (
  const union mhd_H2FrameUnion *restrict frame_info)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_IN_ (1);


/**
 * Calculate full frame size
 * @param frame_info the information about frame
 * @return the total size of the frame, including frame header and payload
 */
mhd_static_inline MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_ (1) size_t
mhd_h2_frame_get_total_size (const union mhd_H2FrameUnion *restrict frame_info)
{
  mhd_assert ((mhd_h2_frame_get_extra_hdr_size (frame_info)
               + mhd_h2_frame_get_padding_size (frame_info))
              <= frame_info->selector.length);
  return (size_t) (mhd_H2_FR_HDR_BASE_SIZE + frame_info->selector.length);
}


/**
 * Set 'length' member in the HTTP/2 frame.
 * This function takes into account the frame type, the size of the extra
 * frame header (if any), the padding (if allowed by frame type and set in
 * @a frame_info) and the payload size.
 *
 * @param[in,out] frame_info the information about frame: the type, all flags
 *                           and padding size (if any) must be set; the 'length'
 *                           is modified
 * @param real_payload_size the size of the real payload of the frame (the
 *                          part of the frame after extra header bytes and
 *                          before the padding (if any) bytes;
 *                          must be zero for frames without payload support;
 *                          must fit 24 bits length together with padding and
 *                          extra header
 * @return the total size of the updated frame: the basis header plus the value
 *         of the frame 'length' field
 */
MHD_INTERNAL size_t
mhd_h2_frame_set_payload_size (union mhd_H2FrameUnion *restrict frame_info,
                               size_t real_payload_size)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_INOUT_ (1);

/**
 * Encode the HTTP/2 frame header into the output buffer.
 *
 * This function writes the HTTP/2 basic frame header and extra frame header
 * (if any, for know frame types only) using data from @a frame_info.
 * @param frame_info the frame information to encode
 * @param out_buff_size the size of @a out_buff in bytes
 * @param[out] out_buff the output buffer to receive the encoded header
 * @return the number of bytes written on success
 *         (always #mhd_H2_FR_HDR_BASE_SIZE or more),
 *         or 0 if @a out_buff_size is too small (the output buffer may be
 *         alerted in case of this error)
 */
MHD_INTERNAL size_t
mhd_h2_frame_hdr_encode (const union mhd_H2FrameUnion *restrict frame_info,
                         size_t out_buff_size,
                         uint8_t *restrict out_buff)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_IN_ (1)
MHD_FN_PAR_OUT_SIZE_ (3,2);

#if defined(_MSC_FULL_VER)
/* Restore warnings */
#pragma warning(pop)
#endif /* _MSC_FULL_VER */

#endif /* ! MHD_H2_FRAME_CODEC_H */
