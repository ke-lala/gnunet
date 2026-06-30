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
 * @file src/mhd2/h2/h2_reply_funcs.c
 * @brief  Definitions of HTTP/2 reply sending functions
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include "mhd_str_macros.h"

#include "mhd_constexpr.h"

#include "mhd_assert.h"
#include "mhd_unreachable.h"

#include "mhd_buffer.h"
#include "mhd_response.h"
#include "mhd_connection.h"
#include "mhd_daemon.h"

#include "mhd_str.h"
#include "mhd_read_file.h"

#include "stream_process_reply.h"

#include "h2_conn_data.h"
#include "h2_stream_data.h"

#include "h2_frame_init.h"
#include "h2_proc_conn.h"
#include "h2_proc_out.h"

#include "h2_frame_codec.h"

#include "hpack/mhd_hpack_codec.h"


#include "h2_reply_funcs.h"

struct mhd_H2Stream;    /* Forward declaration */

/* local wrapper */
mhd_static_inline MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1) MHD_FN_PAR_IN_ (2) MHD_FN_PAR_IN_ (3)
MHD_FN_PAR_OUT_SIZE_ (6,5) MHD_FN_PAR_OUT_ (7) bool
enc_field (struct mhd_HpackEncContext *restrict hk_enc,
           const struct mhd_BufferConst *restrict name,
           const struct mhd_BufferConst *restrict value,
           enum mhd_HpackEncPolicy enc_pol,
           const size_t out_buff_size,
           uint8_t *restrict out_buff,
           size_t *restrict bytes_encoded)
{
  enum mhd_HpackEncResult enc_res;

  enc_res = mhd_hpack_enc_field (hk_enc,
                                 name,
                                 value,
                                 enc_pol,
                                 out_buff_size,
                                 out_buff,
                                 bytes_encoded);

  mhd_assert (mhd_HPACK_ENC_RES_ALLOC_ERR != enc_res);

  return (mhd_HPACK_ENC_RES_OK == enc_res);
}


static MHD_FN_PAR_NONNULL_ALL_ size_t
stream_headers_encode (struct mhd_H2Stream *restrict s,
                       struct mhd_Buffer *restrict pl,
                       bool *restrict fields_complete)
{
  struct mhd_HpackEncContext *const hk_enc = &(s->c->h2.hk_enc);
  struct MHD_Response *const r = s->rpl.response;
  uint8_t *restrict buff = (uint8_t *) pl->data;
  size_t pos;
  size_t pos_incr;
  size_t fld_num;
  enum mhd_HpackEncResult enc_res;
  struct mhd_ResponseHeader *hdr;

  *fields_complete = false; /* Could be updated at the end */
  pos = 0u;
  fld_num = 0u;

  /* Pseudo-header */
  if (fld_num >= s->rpl.fields.num_sent)
  {
    enc_res = mhd_hpack_enc_ph_status (hk_enc,
                                       (uint_fast16_t) s->rpl.response->sc,
                                       mhd_HPACK_ENC_PFS_POL_NORMAL,
                                       pl->size - pos,
                                       buff + pos,
                                       &pos_incr);
    mhd_assert (mhd_HPACK_ENC_RES_ALLOC_ERR != enc_res);
    if (mhd_HPACK_ENC_RES_OK != enc_res)
      return pos;

    pos += pos_incr;
    ++(s->rpl.fields.num_sent);
  }
  ++fld_num;

  /* "date" header */

  if ( (! r->cfg.has_hdr_date) &&
       (! s->c->daemon->req_cfg.suppress_date) )
  {
    if (fld_num >= s->rpl.fields.num_sent)
    {
      char val_buff[30];
      if (mhd_build_date_str (val_buff))
      {
        static const struct mhd_BufferConst hdr_name =
          mhd_MSTR_INIT ("date");
        struct mhd_BufferConst hdr_val;

        hdr_val.data = val_buff;
        hdr_val.size = 29u;

        if (! enc_field (hk_enc,
                         &hdr_name,
                         &hdr_val,
                         1 >= s->c->h2.streams.num_streams ?
                         mhd_HPACK_ENC_POL_LOW_PRIO : mhd_HPACK_ENC_POL_NEUTRAL,
                         pl->size - pos,
                         buff + pos,
                         &pos_incr))
          return pos;

        pos += pos_incr;
        ++(s->rpl.fields.num_sent);
      }
    }
    ++fld_num;
  }

  /* "content-length" header */
  if (s->rpl.fields.auto_cntn_len)
  {
    if (fld_num >= s->rpl.fields.num_sent)
    {
      static const struct mhd_BufferConst hdr_name =
        mhd_MSTR_INIT ("content-length");
      char val_buff[21]; /* Maximum supported value is 18446744073709551615 */
      struct mhd_BufferConst hdr_val;

      mhd_assert (MHD_SIZE_UNKNOWN > r->cntn_size);
      hdr_val.data = val_buff;
      hdr_val.size = mhd_uint64_to_str (r->cntn_size,
                                        val_buff,
                                        sizeof(val_buff));
      mhd_assert (0u != hdr_val.size);

      if (! enc_field (hk_enc,
                       &hdr_name,
                       &hdr_val,
                       r->reuse.reusable ?
                       mhd_HPACK_ENC_POL_NEUTRAL : mhd_HPACK_ENC_POL_LOW_PRIO,
                       pl->size - pos,
                       buff + pos,
                       &pos_incr))
        return pos;

      pos += pos_incr;
      ++(s->rpl.fields.num_sent);
    }
    ++fld_num;
  }

  /* User headers */

  for (hdr = mhd_DLINKEDL_GET_FIRST (r, headers);
       NULL != hdr;
       hdr = mhd_DLINKEDL_GET_NEXT (hdr, headers))
  {
    if (NULL == hdr->h2.name.data)
      continue; /* The header is HTTP/1.x only */

    if (fld_num >= s->rpl.fields.num_sent)
    {
      if (! enc_field (hk_enc,
                       &(hdr->h2.name),
                       &(hdr->h2.value),
                       mhd_HPACK_ENC_POL_NEUTRAL,
                       pl->size - pos,
                       buff + pos,
                       &pos_incr))
        return pos;
      pos += pos_incr;
      ++(s->rpl.fields.num_sent);
    }
    ++fld_num;
  }

  *fields_complete = true;
  return pos;
}


static MHD_FN_PAR_NONNULL_ALL_ bool
stream_headers_send (struct mhd_H2Stream *s)
{
  union mhd_H2FrameUnion h2frame;
  struct mhd_H2FrameHeadersInfo *hdrs;
  struct mhd_H2FrameContinuationInfo *cont;
  struct mhd_Buffer buff;
  struct mhd_Buffer payload;
  size_t payload_offset;
  bool *complete_header;
  size_t payload_used;

  mhd_assert (mhd_H2_RPL_STAGE_HEADERS_INCOMPLETE == s->rpl.stage);

  if (0u == s->rpl.fields.num_sent)
  {
    hdrs = mhd_h2_frame_init_headers (&h2frame,
                                      s->stream_id,
                                      false, /* could be updated below */
                                      ! s->rpl.send_content);
    cont = NULL;
    complete_header = &(hdrs->end_headers);
  }
  else
  {
    hdrs = NULL;
    cont = mhd_h2_frame_init_continuation (&h2frame,
                                           s->stream_id,
                                           false); /* could be updated below */
    complete_header = &(cont->end_headers);
  }

  if (! mhd_h2_out_buff_acquire_fr_w_payload (s->c,
                                              &h2frame,
                                              &buff,
                                              &payload_offset))
    return false;

  payload.data = buff.data + payload_offset;
  payload.size = buff.size - payload_offset;

  payload_used = stream_headers_encode (s,
                                        &payload,
                                        complete_header);

  if (0u != payload_used)
  {
    const size_t full_fr_size =  mhd_h2_frame_set_payload_size (&h2frame,
                                                                payload_used);
    const size_t final_fr_hdr_size =
      mhd_h2_frame_hdr_encode (&h2frame,
                               payload_offset,
                               (uint8_t*) buff.data);
    mhd_assert (payload_offset == final_fr_hdr_size);
    (void) final_fr_hdr_size;

    mhd_h2_out_buff_unlock (s->c,
                            full_fr_size);
    if (*complete_header)
    {
      s->rpl.stage = s->rpl.send_content ?
                     mhd_H2_RPL_STAGE_HEADERS_COMPLETE :
                     mhd_H2_RPL_STAGE_END_STREAM;
    }
    return true; /* Success exit point */
  }

  mhd_h2_out_buff_unlock (s->c,
                          0u);

  if (((s->c->write_buffer_size - s->c->write_buffer_append_offset) >=
       mhd_H2_FR_HDR_BASE_SIZE + s->c->h2.peer.max_frame_size) ||
      (0u == s->c->write_buffer_append_offset))
  {
    /* The output buffer may contain the maximum size frame, but no single
       header has been added. It makes no sense to wait more as the
       response header is too large to be used in this connection. */
    s->state.mhd_err = mhd_H2_ERR_INTERNAL_ERROR;
    s->rpl.stage = mhd_H2_RPL_STAGE_BROKEN;
    return false;
  }

  return false;
}


static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_SIZE_ (4,3)
MHD_FN_PAR_OUT_ (5) bool
content_read_iovec (struct MHD_Response *restrict r,
                    uint_fast64_t offset,
                    size_t buff_size,
                    uint8_t *restrict buff,
                    size_t *restrict written)
{
  size_t i;
  uint_fast64_t skipped;
  const mhd_iovec *const restrict iov = r->cntn.iovec.iov;

  mhd_assert (mhd_RESPONSE_CONTENT_DATA_IOVEC == r->cntn_dtype);

  skipped = 0u;

  for (i = 0u; r->cntn.iovec.cnt > i; ++i)
  {
    if (skipped + iov[i].iov_len > offset)
      break;
    skipped += iov[i].iov_len;
    mhd_assert (skipped >= iov[i].iov_len);
  }

  if (r->cntn.iovec.cnt == i)
    return false;

  if (1)
  {
    size_t elmnt_copy;
    const size_t elmnt_off = (size_t) (offset - skipped);

    if (elmnt_off != (offset - skipped))
      return false;

    mhd_assert (0u != iov[i].iov_len);

    elmnt_copy = (size_t) (iov[i].iov_len - elmnt_off);
    if (buff_size < elmnt_copy)
      elmnt_copy = buff_size;

    memcpy (buff,
            ((const uint8_t *) iov[i].iov_base) + elmnt_off,
            elmnt_copy);
    *written = elmnt_copy;

    if (elmnt_copy == buff_size)
      return true;

    ++i;
  }

  for ((void) i; r->cntn.iovec.cnt > i; ++i)
  {
    mhd_assert (0u != iov[i].iov_len);
    if ((buff_size - *written) <= iov[i].iov_len)
    {
      memcpy (buff + *written,
              iov[i].iov_base,
              buff_size - *written);
      *written = buff_size;
      return true;
    }
    memcpy (buff + *written,
            iov[i].iov_base,
            (size_t) iov[i].iov_len);
    *written += (size_t) iov[i].iov_len;
    mhd_assert (*written > iov[i].iov_len);
  }
  return true;
}


static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_SIZE_ (4,3)
MHD_FN_PAR_OUT_ (5) bool
content_read_file (struct MHD_Response *restrict r,
                   uint_fast64_t offset,
                   size_t buff_size,
                   uint8_t *restrict buff,
                   size_t *restrict written)
{
  uint_fast64_t file_off;

  mhd_assert (mhd_RESPONSE_CONTENT_DATA_FILE == r->cntn_dtype);
  // TODO: support pipe reading without position
  mhd_assert (! r->cntn.file.is_pipe);

  file_off = offset + r->cntn.file.offset;
  if (file_off < offset)
    return false; /* Offset too large */

  return (mhd_FILE_READ_OK ==
          mhd_read_file (r->cntn.file.fd,
                         file_off,
                         buff_size,
                         (char*) buff,
                         written));
}


mhd_constexpr uint_least32_t min_size_for_data = 128u;

static MHD_FN_PAR_NONNULL_ALL_ bool
stream_content_send (struct mhd_H2Stream *s)
{
  struct MHD_Response *const r = s->rpl.response;
  union mhd_H2FrameUnion h2frame;
  struct mhd_H2FrameDataInfo *dat;
  struct mhd_Buffer buff;
  uint8_t *pld_buff;
  size_t pld_buff_size;
  size_t cntnt_left;
  size_t payload_offset;
  size_t payload_used;
  int_least32_t wndw_limit;
  uint_least32_t full_payload_limit;

  mhd_assert (mhd_H2_RPL_STAGE_HEADERS_COMPLETE == s->rpl.stage);
  mhd_assert (s->rpl.send_content);
  mhd_assert (0u != r->cntn_size);
  mhd_assert (! r->cfg.head_only);

  if (s->c->h2.state.send_window < s->state.send_window)
    wndw_limit = s->c->h2.state.send_window;
  else
    wndw_limit = s->state.send_window;

  if (0 >= wndw_limit)
    return false; /* The peer should increment window(s) first */

  full_payload_limit = (uint_least32_t) wndw_limit;
  if (MHD_SIZE_UNKNOWN != r->cntn_size)
  {
    cntnt_left = r->cntn_size - s->rpl.cntn_read_pos;
    if (cntnt_left < full_payload_limit)
      full_payload_limit = (uint_least32_t) cntnt_left;
  }
  else
    cntnt_left = MHD_SIZE_UNKNOWN;

  if ((min_size_for_data > full_payload_limit)
      && (cntnt_left != full_payload_limit))
    return false;

  dat = mhd_h2_frame_init_data (&h2frame,
                                s->stream_id,
                                false);  /* could be updated below */

  if (! mhd_h2_out_buff_acquire_fr_w_payload_l (s->c,
                                                &h2frame,
                                                full_payload_limit,
                                                &buff,
                                                &payload_offset))
    return false;

  pld_buff = (uint8_t*) buff.data + payload_offset;
  pld_buff_size = buff.size - payload_offset;
  mhd_assert (mhd_H2_FR_HDR_BASE_SIZE < pld_buff_size);

  mhd_assert (r->cntn_size > s->rpl.cntn_read_pos);

  payload_used = 0u;
  switch (r->cntn_dtype)
  {
  case mhd_RESPONSE_CONTENT_DATA_BUFFER:
    payload_used = (size_t) full_payload_limit;
    memcpy (pld_buff,
            r->cntn.buf + s->rpl.cntn_read_pos,
            payload_used);
    break;
  case mhd_RESPONSE_CONTENT_DATA_IOVEC:
    if (! content_read_iovec (r,
                              s->rpl.cntn_read_pos,
                              pld_buff_size,
                              pld_buff,
                              &payload_used))
      payload_used = 0u;
    break;
  case mhd_RESPONSE_CONTENT_DATA_FILE:
    if (! content_read_file (r,
                             s->rpl.cntn_read_pos,
                             pld_buff_size,
                             pld_buff,
                             &payload_used))
      payload_used = 0u;
    break;
  case mhd_RESPONSE_CONTENT_DATA_CALLBACK:
    s->rpl.stage = mhd_H2_RPL_STAGE_BROKEN;
    break;
  case mhd_RESPONSE_CONTENT_DATA_INVALID:
  default:
    mhd_UNREACHABLE ();
    s->rpl.stage = mhd_H2_RPL_STAGE_BROKEN;
    break;
  }

  dat->end_stream = (payload_used + s->rpl.cntn_read_pos == r->cntn_size);

  if (0u != payload_used)
  {
    const size_t full_fr_size =  mhd_h2_frame_set_payload_size (&h2frame,
                                                                payload_used);
    const size_t final_fr_hdr_size =
      mhd_h2_frame_hdr_encode (&h2frame,
                               payload_offset,
                               (uint8_t*) buff.data);
    mhd_assert (payload_offset == final_fr_hdr_size);
    (void) final_fr_hdr_size;

    mhd_h2_out_buff_unlock (s->c,
                            full_fr_size);
    s->c->h2.state.send_window -=
      (int_least32_t) (full_fr_size - mhd_H2_FR_HDR_BASE_SIZE);
    mhd_assert (0 <= s->c->h2.state.send_window);
    s->state.send_window -=
      (int_least32_t) (full_fr_size - mhd_H2_FR_HDR_BASE_SIZE);
    mhd_assert (0 <= s->state.send_window);

    return true; /* Success exit point */
  }

  mhd_h2_out_buff_unlock (s->c,
                          0u);

  s->state.mhd_err = mhd_H2_ERR_INTERNAL_ERROR;
  s->rpl.stage = mhd_H2_RPL_STAGE_BROKEN;

  return false;

}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_stream_reply_send (struct mhd_H2Stream *s)
{
  mhd_assert (s->is_h2);
  mhd_assert (mhd_H2_RPL_STAGE_END_STREAM != s->rpl.stage);
  mhd_assert (mhd_H2_RPL_STAGE_BROKEN != s->rpl.stage);

  if (mhd_H2_RPL_STAGE_HEADERS_INCOMPLETE == s->rpl.stage)
  {
    if (! mhd_hpack_enc_dyn_resize (&(s->c->h2.hk_enc)))
    {
      /* Ignore failure of the next function as the connection and stream
         will be retried next round if connection is not aborted. */
      mhd_h2_conn_finish (s->c,
                          mhd_H2_ERR_INTERNAL_ERROR,
                          false);
      return false;
    }

    if (! stream_headers_send (s))
      return false;

    if ((mhd_H2_RPL_STAGE_HEADERS_COMPLETE == s->rpl.stage) &&
        (mhd_RESPONSE_CONTENT_DATA_FILE <= s->rpl.response->cntn_dtype))
      return true; /* Do not combine with content sending as the data is not ready yet */
  }

  if (mhd_H2_RPL_STAGE_HEADERS_COMPLETE == s->rpl.stage)
  {
    if (! stream_content_send (s))
      return false;
  }

  return true;

}
