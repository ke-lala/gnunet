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
 * @file src/mhd2/h2/h2_app_cb.c
 * @brief  Implementation of HTTP/2 functions for calling application callbacks
 * @author Karlson2k (Evgeny Grin)
 */


#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include "mhd_assert.h"
#include "mhd_unreachable.h"

#include "mhd_str_types.h"

#include "mhd_daemon.h"
#include "mhd_connection.h"
#include "h2_conn_data.h"
#include "h2_stream_data.h"

#include "mhd_panic.h"
#include "daemon_logger.h"

#include "response_destroy.h"

#include "h2_req_items_funcs.h"
#include "h2_conn_streams.h"

#include "h2_app_cb.h"


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_stream_cb_early_uri (struct mhd_H2Stream *restrict s)
{
  mhd_assert (mhd_H2_REQ_STAGE_HEADERS_DECODING == s->req.stage);
  mhd_assert (mhd_HTTP_METHOD_NO_METHOD != s->req.method);
  mhd_assert (mhd_h2_items_debug_get_streamid (s->c->h2.mem.req_ib)
              == s->stream_id);
  mhd_assert (mhd_H2_REQ_ITEM_POS_INVALID != s->req.pos_path);

  if (NULL != s->c->daemon->req_cfg.uri_cb.cb)
  {
    struct MHD_EarlyUriCbData req_data;
    bool res;

    req_data.request = (struct MHD_Request *) (void*) &(s->req);
    res = mhd_h2_items_get_item_value (s->c->h2.mem.req_ib,
                                       s->req.pos_path,
                                       &(req_data.full_uri));
    mhd_assert (res);
    (void) res;

    if (s->c->h2.state.top_proc_stream_id < s->stream_id)
      s->c->h2.state.top_proc_stream_id = s->stream_id;
    s->req.app_seen = true;

    s->c->daemon->req_cfg.uri_cb.cb (s->c->daemon->req_cfg.uri_cb.cls,
                                     &req_data,
                                     &(s->req.app_context));
  }

  return true;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_stream_cb_request (struct mhd_H2Stream *restrict s)
{
  struct MHD_Connection *restrict c = s->c;
  struct MHD_Daemon *restrict d = c->daemon;
  struct MHD_String path;
  const struct MHD_Action *a;

  mhd_assert (mhd_C_IS_HTTP2 (c));
  mhd_assert (mhd_H2_REQ_STAGE_HEADERS_PROCESSING == s->req.stage);
  mhd_assert (mhd_HTTP_METHOD_NO_METHOD != s->req.method);
  mhd_assert (mhd_h2_items_debug_get_streamid (s->c->h2.mem.req_ib)
              == s->stream_id);
  mhd_assert (mhd_H2_REQ_ITEM_POS_INVALID != s->req.pos_path);

  mhd_assert (NULL == s->rpl.response);

  if (mhd_ACTION_NO_ACTION != s->req.app_act.head_act.act)
    MHD_PANIC ("MHD_Action has been set already");

  if (1)
  {
    bool res =
      mhd_h2_items_get_item_value (s->c->h2.mem.req_ib,
                                   s->req.pos_path,
                                   &(path));
    mhd_assert (res);
    (void) res;
  }

  if (s->c->h2.state.top_proc_stream_id < s->stream_id)
    s->c->h2.state.top_proc_stream_id = s->stream_id;
  s->req.app_seen = true;

  a = d->req_cfg.cb (d->req_cfg.cb_cls,
                     (struct MHD_Request *) (void*) &(s->req),
                     &path,
                     (enum MHD_HTTP_Method) s->req.method,
                     s->req.cntn_size);

  if ((NULL != a)
      && (((&(s->req.app_act.head_act) != a))
          || ! mhd_ACTION_IS_VALID (s->req.app_act.head_act.act)))
  {
    mhd_LOG_MSG (d, MHD_SC_ACTION_INVALID, \
                 "Provided action is not a correct action generated " \
                 "for the current request.");
    /* Perform cleanup of the created but now unused action */
    switch (s->req.app_act.head_act.act)
    {
    case mhd_ACTION_RESPONSE:
      mhd_assert (NULL != s->req.app_act.head_act.data.response);
      mhd_response_dec_use_count (s->req.app_act.head_act.data.response);
      break;
    case mhd_ACTION_UPLOAD:
    case mhd_ACTION_SUSPEND:
      /* No cleanup needed */
      break;
#ifdef MHD_SUPPORT_POST_PARSER
    case mhd_ACTION_POST_PARSE:
      /* No cleanup needed */
      break;
#endif /* MHD_SUPPORT_POST_PARSER */
#ifdef MHD_SUPPORT_UPGRADE
    case mhd_ACTION_UPGRADE:
      /* No cleanup needed */
      break;
#endif /* MHD_SUPPORT_UPGRADE */
    case mhd_ACTION_ABORT:
      mhd_UNREACHABLE ();
      break;
    case mhd_ACTION_NO_ACTION:
    default:
      break;
    }
    a = NULL;
  }
  if (NULL == a)
    s->req.app_act.head_act.act = mhd_ACTION_ABORT;

  switch (s->req.app_act.head_act.act)
  {
  case mhd_ACTION_RESPONSE:
    s->rpl.response = s->req.app_act.head_act.data.response;
    return true;
#if 0
  case mhd_ACTION_UPLOAD:
    if (0 != s->req.cntn_size)
    {
      if (! check_and_alloc_buf_for_upload_processing (c))
        return true;
      c->stage = mhd_HTTP_STAGE_BODY_RECEIVING;
      return (0 != c->read_buffer_offset);
    }
    c->stage = mhd_HTTP_STAGE_FULL_REQ_RECEIVED;
    return true;
#ifdef MHD_SUPPORT_POST_PARSER
  case mhd_ACTION_POST_PARSE:
    if (0 == s->req.cntn.cntn_size)
    {
      s->req.u_proc.post.parse_result = MHD_POST_PARSE_RES_REQUEST_EMPTY;
      c->stage = mhd_HTTP_STAGE_FULL_REQ_RECEIVED;
      return true;
    }
    if (! mhd_stream_prepare_for_post_parse (c))
    {
      mhd_assert (mhd_HTTP_STAGE_FOOTERS_RECEIVED < c->stage);
      return true;
    }
    if (need_100_continue (c))
    {
      c->stage = mhd_HTTP_STAGE_CONTINUE_SENDING;
      return true;
    }
    c->stage = mhd_HTTP_STAGE_BODY_RECEIVING;
    return true;
#endif /* MHD_SUPPORT_POST_PARSER */
  case mhd_ACTION_SUSPEND:
    c->suspended = true;
#ifdef MHD_USE_TRACE_SUSPEND_RESUME
    fprintf (stderr,
             "%%%%%% Suspending connection, FD: %2llu\n",
             (unsigned long long) c->sk.fd);
#endif /* MHD_USE_TRACE_SUSPEND_RESUME */
    s->req.app_act.head_act.act = mhd_ACTION_NO_ACTION;
    return false;
#ifdef MHD_SUPPORT_UPGRADE
  case mhd_ACTION_UPGRADE:
    mhd_assert (0 == s->req.cntn.cntn_size);
    c->stage = mhd_HTTP_STAGE_UPGRADE_HEADERS_SENDING;
    return false;
#endif /* MHD_SUPPORT_UPGRADE */
  case mhd_ACTION_ABORT:
    mhd_conn_start_closing_app_abort (c);
    return true;
  case mhd_ACTION_NO_ACTION:
  default:
    mhd_assert (0 && "Impossible value");
    mhd_UNREACHABLE ();
    break;
#endif
  }

  return mhd_h2_stream_abort (s,
                              mhd_H2_ERR_INTERNAL_ERROR);
}
