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
 * @file src/mhd2/h2/h2_proc_out.h
 * @brief  Declarations of HTTP/2 connection outgoing data processing functions
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_H2_PROC_OUT_H
#define MHD_H2_PROC_OUT_H 1

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include "h2_err_codes.h"

struct MHD_Connection; /* forward declaration */
struct mhd_Buffer; /* forward declaration */
union mhd_H2FrameUnion; /* forward declaration */

/**
 * Check whether the output buffer has at least specified free space.
 * @param c the connection to check
 * @param space_needed the amount of free space needed
 * @return 'true' if output buffer has enough space,
 *         'false' otherwise
 */
MHD_INTERNAL bool
mhd_h2_out_buff_has_space_sz (struct MHD_Connection *restrict c,
                              size_t space_needed)
MHD_FN_PAR_NONNULL_ALL_;

/**
 * Check whether the output buffer has enough space to add the frame (together
 * with optional payload).
 * @param c the connection to check
 * @param h2frame the frame to use, the 'type', all flags and the 'length'
 *                must be set
 * @return 'true' if output buffer has enough space,
 *         'false' otherwise
 */
MHD_INTERNAL bool
mhd_h2_out_buff_has_space_fr (struct MHD_Connection *restrict c,
                              union mhd_H2FrameUnion *restrict h2frame)
MHD_FN_PAR_NONNULL_ALL_;


/**
 * Acquire the output buffer for specified frame with undefined payload size.
 *
 * The @a h2frame must have all information set except the length of the frame.
 * The provided @a buff is not larger than maximum frame size allowed for
 * connection. If @a h2frame has padding, then the size for the padding is
 * reserved at the end of the @a buff (but not included to the @a buff size).
 *
 * In the debug builds this function "locks" the output buffer if succeed.
 * The caller must always call #mhd_h2_out_buff_unlock() after finishing
 * working with the buffer even if nothing is written to the buffer.
 *
 * @param c the connection to use
 * @param h2frame the frame to use, the 'type' and all flags must be set,
 *                the 'length' is ignored
 * @param[out] buff set to the acquired space in the buffer, the size is always
 *                  larger than the basic header and the extra header;
 *                  the space reserved for the padding is not included
 * @param[out] payload_offset set to the offset where to put the frame payload
 * @return 'true' if output buffer has enough space for the frame header and at
 *                least one byte for the payload,
 *         'false' otherwise
 */
MHD_INTERNAL bool
mhd_h2_out_buff_acquire_fr_w_payload (
  struct MHD_Connection *restrict c,
  const union mhd_H2FrameUnion *restrict h2frame,
  struct mhd_Buffer *restrict buff,
  size_t *restrict payload_offset)
MHD_FN_PAR_NONNULL_ALL_;

/**
 * Acquire the output buffer for specified frame with undefined payload size.
 *
 * Version of #mhd_h2_out_buff_acquire_fr_w_payload() with predefined limit
 * for the frame full payload size.
 *
 * @param c the connection to use
 * @param h2frame the frame to use, the 'type' and all flags must be set,
 *                the 'length' is ignored
 * @param full_payload_limit the maximum size of the frame (excluding base
 *                           frame header)
 * @param[out] buff set to the acquired space in the buffer, the size is always
 *                  larger than the basic header and the extra header;
 *                  the space reserved for the padding is not included
 * @param[out] payload_offset set to the offset where to put the frame payload
 * @return 'true' if output buffer has enough space for the frame header and at
 *                least one byte for the payload,
 *         'false' otherwise
 */
MHD_INTERNAL bool
mhd_h2_out_buff_acquire_fr_w_payload_l (
  struct MHD_Connection *restrict c,
  const union mhd_H2FrameUnion *restrict h2frame,
  uint_least32_t full_payload_limit,
  struct mhd_Buffer *restrict buff,
  size_t *restrict payload_offset)
MHD_FN_PAR_NONNULL_ALL_;


/**
 * Finish writing the data to the output buffer.
 * The output buffer must be previously acquired by calling
 * #mhd_h2_out_buff_acquire_fr_w_payload().
 * @param c the connection to use
 * @param size_used the size of the frame written; if frame had padding then
 *                  it could be larger (by the padding size) than the size of
 *                  the buffer provided by #mhd_h2_out_buff_acquire_fr_w_payload()
 */
MHD_INTERNAL void
mhd_h2_out_buff_unlock (struct MHD_Connection *restrict c,
                        size_t size_used)
MHD_FN_PAR_NONNULL_ALL_;

/**
 * Queue to the sending buffer a frame without payload
 * @param c the connection to use
 * @param h2frame the frame to queue
 * @return 'true' if the frame is successfully added,
 *         'false' if sending buffer has not enough space
 */
MHD_INTERNAL bool
mhd_h2_q_frame_no_payload (struct MHD_Connection *restrict c,
                           union mhd_H2FrameUnion *restrict h2frame)
MHD_FN_PAR_NONNULL_ALL_;

/**
 * Queue to the sending buffer a RST_STREAM frame
 * @param c the connection to use
 * @param stream_id the stream ID for the frame
 * @param err the HTTP/2 error code
 * @return 'true' if the frame is successfully added,
 *         'false' if sending buffer has not enough space
 */
MHD_INTERNAL bool
mhd_h2_q_rst_stream (struct MHD_Connection *restrict c,
                     uint_least32_t stream_id,
                     enum mhd_H2ErrorCode err)
MHD_FN_PAR_NONNULL_ALL_;

/**
 * Queue to the sending buffer a PING frame
 * @param c the connection to use
 * @param ack the value of the ACK flag in the frame
 * @param opaque the PING opaque data
 * @return 'true' if the frame is successfully added,
 *         'false' if sending buffer has not enough space
 */
MHD_INTERNAL bool
mhd_h2_q_ping (struct MHD_Connection *restrict c,
               bool ack,
               const uint8_t opaque[MHD_FN_PAR_FIX_ARR_SIZE_ (8)])
MHD_FN_PAR_NONNULL_ALL_;


/**
 * Queue to the sending buffer a GOAWAY frame without additional debug info
 * @param c the connection to use
 * @param err the HTTP/2 error code
 * @return 'true' if the frame is successfully added,
 *         'false' if sending buffer has not enough space
 */
MHD_INTERNAL bool
mhd_h2_q_goaway (struct MHD_Connection *restrict c,
                 enum mhd_H2ErrorCode err)
MHD_FN_PAR_NONNULL_ALL_;

/**
 * Queue to the sending buffer a WINDOW_UPDATE frame
 * @param c the connection to use
 * @param stream_id the stream ID for the frame
 * @param win_size_incr the Window Size Increment value;
 *                      must not be zero, must fit 31 bits
 * @return 'true' if the frame is successfully added,
 *         'false' if sending buffer has not enough space
 */
MHD_INTERNAL bool
mhd_h2_q_window_update (struct MHD_Connection *restrict c,
                        uint_least32_t stream_id,
                        uint_least32_t win_size_incr)
MHD_FN_PAR_NONNULL_ALL_;

#endif /* ! MHD_H2_PROC_OUT_H */
