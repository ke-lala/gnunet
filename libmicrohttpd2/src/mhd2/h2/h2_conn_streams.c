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
 * @file src/mhd2/h2/h2_conn_streams.c
 * @brief  Implementation of HTTP/2 connection streams processing functions
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include "mhd_assert.h"
#include "mhd_unreachable.h"

#include "mhd_constexpr.h"

#include "compat_calloc.h"
#include "sys_malloc.h"

#include "mhd_dlinked_list.h"

#include "mhd_response.h"
#include "mhd_connection.h"

#include "mempool_funcs.h"

#include "response_destroy.h"

#include "h2_bit_masks.h"
#include "h2_err_codes.h"

#include "h2_stream_data.h"
#include "h2_proc_out.h"
#include "h2_proc_conn.h"
#include "h2_app_cb.h"

#include "h2_req_items_funcs.h"
#include "h2_req_fields.h"
#include "h2_reply_funcs.h"

#include "h2_conn_streams.h"

#include <string.h>


static MHD_FN_PAR_NONNULL_ALL_ struct mhd_H2Stream *
conn_add_new_stream (struct MHD_Connection *restrict c,
                     uint_least32_t stream_id)
{
  struct mhd_H2Stream *s;

  mhd_assert ((stream_id & mhd_H2_STREAM_ID_MASK) == stream_id);
  mhd_assert (c->h2.rcv_cfg.max_concur_streams > c->h2.streams.num_streams);

  s = (struct mhd_H2Stream *) mhd_calloc (1u,
                                          sizeof(struct mhd_H2Stream));
  if (NULL == s)
    return NULL;

  s->is_h2 = true;
  s->stream_id = stream_id;

#ifndef HAVE_NULL_PTR_ALL_ZEROS
  mhd_DLINKEDL_INIT_LINKS (s, streams);
  mhd_DLINKEDL_INIT_LINKS (s, send_q);
  s->req.app_context = NULL;
#endif /* ! HAVE_NULL_PTR_ALL_ZEROS */

  s->c = c;

  s->req.is_http2 = true;
  s->req.stage = mhd_H2_REQ_STAGE_HEADERS_INCOMPLETE;
  s->req.pos_method = mhd_H2_REQ_ITEM_POS_INVALID;
  s->req.pos_path = mhd_H2_REQ_ITEM_POS_INVALID;
  s->req.pos_authority = mhd_H2_REQ_ITEM_POS_INVALID;

  s->state.recv_window = (int_least32_t) c->h2.rcv_cfg.stream_init_win_sz;
  mhd_assert (0 < s->state.recv_window);
  s->state.send_window = (int_least32_t) c->h2.peer.stream_init_win_sz;
  mhd_assert (0 < s->state.send_window);

  mhd_DLINKEDL_INS_LAST_D (&(c->h2.streams.active), s, streams);
  mhd_assert (0u != ~(c->h2.streams.num_streams));
  ++(c->h2.streams.num_streams);

  return s;
}


static MHD_FN_PAR_NONNULL_ALL_ void
conn_remove_stream (struct MHD_Connection *c,
                    struct mhd_H2Stream *restrict s)
{
  mhd_assert (s->c == c);

  if (NULL != s->rpl.response)
    mhd_response_dec_use_count (s->rpl.response);
  mhd_DLINKEDL_DEL_D (&(c->h2.streams.active), s, streams);

  free (s);
  mhd_assert (0u != c->h2.streams.num_streams);
  --(c->h2.streams.num_streams);
}


static MHD_FN_PAR_NONNULL_ALL_ struct mhd_H2Stream *
conn_find_stream (struct MHD_Connection *restrict c,
                  uint_least32_t stream_id)
{
  struct mhd_H2Stream *s;

  // TODO: improve search. Binary tree or linear array?
  for (s = mhd_DLINKEDL_GET_FIRST_D (&(c->h2.streams.active));
       NULL != s;
       s = mhd_DLINKEDL_GET_NEXT (s, streams))
  {
    if (stream_id == s->stream_id)
      return s;
  }

  return NULL;
}


static void
stream_start_replying (struct mhd_H2Stream *restrict s)
MHD_FN_PAR_NONNULL_ALL_;

MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_stream_req_problem (struct mhd_H2Stream *restrict s,
                           enum mhd_H2RequestProblemType problem_type)
{
  // TODO: send error reply
  (void) problem_type;
  return mhd_h2_stream_abort (s,
                              mhd_H2_ERR_PROTOCOL_ERROR); // TODO: use correct error code
}


static MHD_FN_PAR_NONNULL_ALL_ bool
stream_send_rst_stream (struct mhd_H2Stream *restrict s,
                        enum mhd_H2ErrorCode err)
{
  mhd_assert ((! s->state.sent_rst_stream) &&
              "RST_STREAM must not be sent more than once for active stream");

  if (! mhd_h2_q_rst_stream (s->c,
                             s->stream_id,
                             err))
    return false;

  s->state.sent_rst_stream = true;
  s->state.mhd_err = err;

  return true;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_stream_abort (struct mhd_H2Stream *restrict s,
                     enum mhd_H2ErrorCode err)
{
  s->req.stage = mhd_H2_REQ_STAGE_BROKEN;
  // TODO: Handle correctly by RST_STREAM
  mhd_h2_conn_finish (s->c,
                      err,
                      true);
  return false;
}


static MHD_FN_PAR_NONNULL_ALL_ bool
stream_proc_complete_headers (struct mhd_H2Stream *restrict s)
{
  mhd_assert (mhd_H2_REQ_STAGE_HEADERS_INCOMPLETE == s->req.stage);
  mhd_assert (0u == s->c->h2.state.continuation_stream_id);
  mhd_assert (mhd_h2_items_debug_get_streamid (s->c->h2.mem.req_ib)
              == s->stream_id);

  s->req.stage = mhd_H2_REQ_STAGE_HEADERS_DECODING;
  s->req.cntn_size = (s->req.got_end_stream ? 0u : MHD_SIZE_UNKNOWN);

  if (! mhd_h2_req_headers_preprocess (s))
    return false;

  if (! mhd_h2_stream_cb_early_uri (s))
    return false;

  if (! mhd_h2_req_uri_parse (s))
    return false;

  if (! mhd_h2_req_cookie_parse (s))
    return false;

  s->req.stage = mhd_H2_REQ_STAGE_HEADERS_PROCESSING;

  if (! mhd_h2_stream_cb_request (s))
    return false;

  mhd_assert (mhd_H2_REQ_STAGE_BROKEN != s->req.stage);

  if (s->req.got_end_stream)
  {
    s->req.stage = mhd_H2_REQ_STAGE_END_STREAM;
    mhd_assert (NULL != s->rpl.response);
  }
  else
    s->req.stage = mhd_H2_REQ_STAGE_HEADERS_COMPLETE;

  if (NULL != s->rpl.response)
    stream_start_replying (s);

  return true;
}


static MHD_FN_PAR_NONNULL_ALL_ bool
stream_proc_in_headers (struct mhd_H2Stream *restrict s,
                        bool end_headers,
                        struct mhd_Buffer *restrict payload)
{
  struct MHD_Connection *const c = s->c;
  size_t unprocessed;

  switch (mhd_h2_req_fields_decode (&(c->h2.hk_dec),
                                    payload,
                                    false,
                                    c->h2.mem.req_ib,
                                    &unprocessed))
  {
  case mhd_H2_DEC_FIELDS_OK:
    break;
  case mhd_H2_DEC_FIELDS_NO_SPACE:
  // TODO: Send error response before closing, use RST_STREAM
  case mhd_H2_DEC_FIELDS_INT_ERR:
    return mhd_h2_stream_req_problem (s,
                                      mhd_H2_REQ_PRBLM_INT_ERROR);
  case mhd_H2_DEC_FIELDS_BROKEN_DATA:
    s->req.stage = mhd_H2_REQ_STAGE_BROKEN;
    mhd_h2_conn_finish (c,
                        mhd_H2_ERR_COMPRESSION_ERROR,
                        true);
    return false;
  case mhd_H2_DEC_FIELDS_PROT_ABUSE:
    s->req.stage = mhd_H2_REQ_STAGE_BROKEN;
    mhd_h2_conn_finish (c,
                        mhd_H2_ERR_ENHANCE_YOUR_CALM,
                        true);
    return false;
  default:
    mhd_UNREACHABLE ();
    s->req.stage = mhd_H2_REQ_STAGE_BROKEN;
    mhd_h2_conn_finish (c,
                        mhd_H2_ERR_INTERNAL_ERROR,
                        true);
    return false;
  }

  if (end_headers && (0u != unprocessed))
    return ! mhd_h2_conn_finish (c,
                                 mhd_H2_ERR_COMPRESSION_ERROR,
                                 true);

  if (! end_headers)
  {
    if (0u != unprocessed)
    {
      const size_t payload_offset = (size_t) (payload->data - c->read_buffer);

      /* Unprocessed part may contain only a single field line.
         Stop stream if it is larger than 3/4 of the max headers size. */
      if ((c->h2.rcv_cfg.max_header_list - c->h2.rcv_cfg.max_header_list / 4) <=
          unprocessed)
        return mhd_h2_stream_req_problem (s,
                                          mhd_H2_REQ_PRBLM_HEADERS_TOO_LARGE);

      mhd_assert (c->h2.rcv_cfg.max_frame_size < mhd_pool_get_size (c->pool));
      if (((mhd_pool_get_size (c->pool) - c->h2.rcv_cfg.max_frame_size) / 2) <=
          unprocessed)
        return mhd_h2_stream_req_problem (s,
                                          mhd_H2_REQ_PRBLM_HEADERS_TOO_LARGE);

      c->h2.buff.unproc_hdrs_pos = payload_offset + payload->size - unprocessed;
      c->h2.buff.unproc_hdrs_size = unprocessed;
    }

    c->h2.state.continuation_stream_id = s->stream_id;
  }
  else
  {
    stream_proc_complete_headers (s);
  }

  return true;
}


static MHD_FN_PAR_NONNULL_ALL_ bool
conn_proc_new_in_stream (struct MHD_Connection *restrict c,
                         uint_least32_t stream_id,
                         bool end_stream,
                         bool end_headers,
                         struct mhd_Buffer *restrict payload)
{
  struct mhd_H2Stream *s;

  mhd_assert ((stream_id & mhd_H2_STREAM_ID_MASK) == stream_id);
  mhd_assert (c->h2.state.top_seen_stream_id >= c->h2.state.top_proc_stream_id);
  mhd_assert (c->h2.state.top_seen_stream_id < stream_id);
  mhd_assert (0u == c->h2.state.continuation_stream_id);
  mhd_assert (NULL == conn_find_stream (c, stream_id));
  mhd_assert (0u == c->h2.buff.unproc_hdrs_size);
  mhd_assert (c->read_buffer <= payload->data);
  mhd_assert ((c->read_buffer_size + c->read_buffer) >=
              (payload->data + payload->size));

  if (c->h2.streams.num_streams >= c->h2.rcv_cfg.max_concur_streams)
    return mhd_h2_q_rst_stream (c,
                                stream_id,
                                mhd_H2_ERR_REFUSED_STREAM);

  s = conn_add_new_stream (c,
                           stream_id);

  if (NULL == s)
    return mhd_h2_q_rst_stream (c, /* REFUSED_STREAM indicates that stream has not been processed at all */
                                stream_id,
                                mhd_H2_ERR_REFUSED_STREAM);
  mhd_h2_items_block_reset (c->h2.mem.req_ib);


  mhd_h2_items_debug_set_streamid (c->h2.mem.req_ib,
                                   stream_id);

  s->req.got_end_stream = end_stream;

  /* The next call process frame data. Current function must not return
     'false' (unless the connection is broken) beyond this point as the
     connection data (HPACK) has been modified . */
  return stream_proc_in_headers (s,
                                 end_headers,
                                 payload);
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_conn_streamid_in_headers (struct MHD_Connection *restrict c,
                                 uint_least32_t stream_id,
                                 bool end_stream,
                                 bool end_headers,
                                 struct mhd_Buffer *restrict payload)
{
  mhd_assert (0u != stream_id);
  mhd_assert ((stream_id & mhd_H2_STREAM_ID_MASK) == stream_id);
  mhd_assert (c->h2.state.top_seen_stream_id >= c->h2.state.top_proc_stream_id);

  if (0u == (stream_id & 1u))
    return mhd_h2_conn_finish (c,
                               mhd_H2_ERR_PROTOCOL_ERROR,
                               false);

  if (c->h2.state.top_seen_stream_id < stream_id)
    return conn_proc_new_in_stream (c,
                                    stream_id,
                                    end_stream,
                                    end_headers,
                                    payload);
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_conn_streamid_in_continuation (struct MHD_Connection *restrict c,
                                      uint_least32_t stream_id,
                                      bool end_headers,
                                      struct mhd_Buffer *payload)
{
  struct mhd_Buffer combined_payload;
  struct mhd_H2Stream *s;

  mhd_assert (0u != stream_id);
  mhd_assert ((stream_id & mhd_H2_STREAM_ID_MASK) == stream_id);
  mhd_assert (c->h2.state.top_seen_stream_id >= c->h2.state.top_proc_stream_id);

  if (c->h2.state.continuation_stream_id != stream_id)
    return mhd_h2_conn_finish (c,
                               mhd_H2_ERR_PROTOCOL_ERROR,
                               false);
  s = conn_find_stream (c,
                        stream_id);
  mhd_assert (NULL != s);

  if (0u == c->h2.buff.unproc_hdrs_size)
    combined_payload = *payload;
  else
  {
    /* Concatenate previously unprocessed part and the new part.
       This will break CONTINUATION frame header, but the frame header is not
       needed as all data from the header has been decoded.
       However, unless connection is broken, 'false' must not be
       returned by this function beyond this point as the same frame
       cannot be decoded again. */
    memmove (payload->data - c->h2.buff.unproc_hdrs_size,
             c->read_buffer + c->h2.buff.unproc_hdrs_pos,
             c->h2.buff.unproc_hdrs_size);
    combined_payload.data = payload->data - c->h2.buff.unproc_hdrs_size;
    combined_payload.size = c->h2.buff.unproc_hdrs_size + payload->size;
  }

  return stream_proc_in_headers (s,
                                 end_headers,
                                 &combined_payload);
}


static MHD_FN_PAR_NONNULL_ALL_ void
stream_set_reply_props (struct mhd_H2Stream *restrict s)
{
  mhd_assert (mhd_H2_RPL_STAGE_HEADERS_INCOMPLETE == s->rpl.stage);
  mhd_assert ((mhd_H2_REQ_STAGE_END_STREAM == s->req.stage) ||
              (mhd_H2_REQ_STAGE_HEADERS_COMPLETE == s->req.stage));
  mhd_assert (NULL != s->rpl.response);
#if 0 // TODO: implement chained replies
  if (199 >= s->rpl.response->sc)
  {
    s->rpl.fields.auto_cntn_len = false;
    s->rpl.send_content = false;
    s->rpl.chained = true;
  }
  else
#endif
  if (MHD_HTTP_STATUS_NO_CONTENT == s->rpl.response->sc)
  {
    s->rpl.fields.auto_cntn_len = false;
    s->rpl.send_content = false;
  }
  else if ((mhd_HTTP_METHOD_HEAD == s->req.method) ||
           (MHD_HTTP_STATUS_NOT_MODIFIED == s->rpl.response->sc))
  {
    s->rpl.fields.auto_cntn_len =
      (MHD_NO == s->rpl.response->cfg.head_only)
      && (MHD_SIZE_UNKNOWN != s->rpl.response->cntn_size);
    s->rpl.send_content = false;
  }
  else
  {
    s->rpl.fields.auto_cntn_len = (MHD_SIZE_UNKNOWN != s->rpl.response->
                                   cntn_size);
    s->rpl.send_content = (0u != s->rpl.response->cntn_size);
  }
  s->rpl.cntn_read_pos = 0u;
}


static MHD_FN_PAR_NONNULL_ALL_ void
stream_start_replying (struct mhd_H2Stream *restrict s)
{
  struct MHD_Connection *c = s->c;

  /* The stream must not be in the sending queue */
  mhd_assert (NULL == mhd_DLINKEDL_GET_NEXT (s, send_q));
  mhd_assert (s != mhd_DLINKEDL_GET_LAST_D (&(c->h2.streams.send_q)));
  mhd_assert (mhd_H2_RPL_STAGE_HEADERS_INCOMPLETE == s->rpl.stage);
  mhd_assert (NULL != s->rpl.response);

  stream_set_reply_props (s);

  mhd_DLINKEDL_INS_LAST (&(c->h2.streams), s, send_q);
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_conn_streamid_in_rst_stream (struct MHD_Connection *restrict c,
                                    uint_least32_t stream_id,
                                    enum mhd_H2ErrorCode err)
{

}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_conn_streamid_window_incr (struct MHD_Connection *restrict c,
                                  uint_least32_t stream_id,
                                  uint_least32_t incr)
{
  struct mhd_H2Stream *s = conn_find_stream (c,
                                             stream_id);
  if (NULL == s)
  {
    if ((0u == (stream_id & 1u)) ||
        (c->h2.state.top_rst_stream_id < stream_id))
      return mhd_h2_conn_finish (c,
                                 mhd_H2_ERR_PROTOCOL_ERROR,
                                 false);

    return true; /* Just ignore the frame */
  }
  if ((0 < s->state.send_window)
      && (0 > s->state.send_window + (int_least32_t) stream_id))
    return mhd_h2_stream_req_problem (s,
                                      mhd_H2_REQ_PRBLM_FLOW_CONTROL);
  s->state.send_window += (int_least32_t) stream_id;
  return true;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_conn_streamid_abort (struct MHD_Connection *restrict c,
                            uint_least32_t stream_id,
                            enum mhd_H2ErrorCode err)
{

}


static MHD_FN_PAR_NONNULL_ALL_ bool
stream_maintain_rcv_window (struct mhd_H2Stream *restrict s)
{
  struct MHD_Connection *restrict c = s->c;

  mhd_assert (0 <= s->state.recv_window);
  /* Dumb algorithm: if receive windows is less than three quarters of the full
   * window size, then bump to the full size. */

  if ((c->h2.rcv_cfg.stream_init_win_sz - c->h2.rcv_cfg.stream_init_win_sz / 4)
      <= (uint_least32_t) s->state.recv_window)
  {
    uint_least32_t incr =
      (uint_least32_t)
      (c->h2.rcv_cfg.stream_init_win_sz
       - (uint_least32_t) s->state.recv_window);
    mhd_assert (0x7FFFFFFFu >= incr);
    if (! mhd_h2_q_window_update (c,
                                  s->stream_id,
                                  incr))
      return false;

    s->state.recv_window = (int_least32_t) c->h2.rcv_cfg.stream_init_win_sz;
  }

  return true;
}


mhd_static_inline MHD_FN_PAR_NONNULL_ALL_ bool
stream_is_closed (const struct mhd_H2Stream *restrict s)
{
  /* If END_STREAM flag has been both send and received then stream is closed */
  if ((mhd_H2_REQ_STAGE_END_STREAM == s->req.stage)
      && (mhd_H2_RPL_STAGE_END_STREAM == s->rpl.stage))
    return true;

  if ((s->state.rcvd_rst_stream) || (s->state.sent_rst_stream))
    return true;

  return false;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_conn_maintain_streams_all (struct MHD_Connection *c)
{
  struct mhd_H2Stream *next;
  struct mhd_H2Stream *s;
  mhd_assert ((! c->h2.state.sent_goaway.occurred) ||
              (mhd_H2_ERR_NO_ERROR == c->h2.state.sent_goaway.code));

  next = mhd_DLINKEDL_GET_FIRST_D (&(c->h2.streams.active));
  while (NULL != (s = next))
  {
    next = mhd_DLINKEDL_GET_NEXT (s, streams);

    /* Send RST_STREAM is needed */
    if ((! s->state.rcvd_rst_stream)
        && (! s->state.sent_rst_stream))
    {
      if ((mhd_H2_REQ_STAGE_BROKEN == s->req.stage) ||
          (mhd_H2_RPL_STAGE_BROKEN == s->rpl.stage))
      {
        enum mhd_H2ErrorCode err;
        err = s->state.mhd_err;
        if (mhd_H2_ERR_NO_ERROR == err)
          err = mhd_H2_ERR_INTERNAL_ERROR;
        if (! stream_send_rst_stream (s,
                                      err))
          return false;
      }
    }

    /* Close and remove stream if it is finished */
    if (stream_is_closed (s))
    {
      conn_remove_stream (c,
                          s);
      continue;
    }

    if (mhd_H2_REQ_STAGE_END_STREAM > s->req.stage)
    {
      if (! stream_maintain_rcv_window (s))
        return false;
    }
  }

  return true;
}


mhd_constexpr size_t min_usable_buff = 32u;

MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_conn_process_streams_sending_queue (struct MHD_Connection *c)
{
  struct mhd_H2Stream *already_processed;
  mhd_assert ((! c->h2.state.sent_goaway.occurred) ||
              (mhd_H2_ERR_NO_ERROR == c->h2.state.sent_goaway.code));

  already_processed = NULL;
  while (! 0)
  {
    struct mhd_H2Stream *const s =
      mhd_DLINKEDL_GET_FIRST_D (&(c->h2.streams.send_q));

    if (NULL == s)
      break;

    if (already_processed == s)
      break;

    mhd_assert (! stream_is_closed (s));

    mhd_h2_stream_reply_send (s);
    if (mhd_H2_ERR_NO_ERROR != c->h2.state.sent_goaway.code)
      return false;

    mhd_DLINKEDL_DEL_D (&(c->h2.streams.send_q), s, send_q);
    if (stream_is_closed (s))
      conn_remove_stream (c,
                          s);
    else if (mhd_H2_RPL_STAGE_END_STREAM > s->rpl.stage)
    {
      /* Still sending, move the stream to the end of the queue */
      mhd_DLINKEDL_INS_LAST_D (&(c->h2.streams.send_q), s, send_q);
      if (NULL == already_processed)
        already_processed = s;
    }
    if ((c->write_buffer_size - c->write_buffer_append_offset) <
        min_usable_buff)
      return false;
  }
  return true;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ void
mhd_h2_conn_close_streams_all  (struct MHD_Connection *restrict c)
{
  while (! 0)
  {
    struct mhd_H2Stream *const s =
      mhd_DLINKEDL_GET_FIRST_D (&(c->h2.streams.send_q));

    if (NULL == s)
      break;

    mhd_assert (! stream_is_closed (s));

    mhd_DLINKEDL_DEL_D (&(c->h2.streams.send_q), s, send_q);
    conn_remove_stream (c,
                        s);
  }
  mhd_assert (0u == c->h2.streams.num_streams);
}
