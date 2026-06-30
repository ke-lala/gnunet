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
 * @file src/mhd2/h2/h2_req_get_items.c
 * @brief  Implementation of HTTP/2 request items public getters
 * @author Karlson2k (Evgeny Grin)
 */


#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include "mhd_assert.h"

#include "mhd_cntnr_ptr.h"

#include "mhd_connection.h"
#include "h2_conn_data.h"
#include "h2_stream_data.h"

#include "h2_req_item_struct.h"
#include "h2_req_items_funcs.h"

#include "mhd_str.h"

#include "h2_req_get_items.h"


mhd_static_inline unsigned int
req_items_kind_mask (enum MHD_ValueKind kind)
{
  unsigned int m;
  m = 0u;
  if (0u != (MHD_VK_HEADER & (unsigned int) kind))
    m |= (unsigned int) mhd_H2_RIK_HEADER;
  if (0u != (MHD_VK_COOKIE & (unsigned int) kind))
    m |= (unsigned int) mhd_H2_RIK_COOKIE;
  if (0u != (MHD_VK_URI_QUERY_PARAM & (unsigned int) kind))
  {
    m |= (unsigned int) mhd_H2_RIK_URI_PARAM;
    m |= (unsigned int) mhd_H2_RIK_URI_PARAM_NV;
  }
  if (0u != (MHD_VK_TRAILER & (unsigned int) kind))
    m |= (unsigned int) mhd_H2_RIK_TRAILER;

  return m;
}


MHD_INTERNAL
MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (4,3)
MHD_FN_PAR_CSTR_ (4) MHD_FN_PAR_OUT_ (5) bool
mhd_h2_request_get_value_n (struct MHD_Request *restrict r,
                            enum MHD_ValueKind kind,
                            size_t key_len,
                            const char *restrict key,
                            struct MHD_StringNullable *restrict value_out)
{
  struct mhd_H2Stream *const s =
    mhd_CNTNR_PTR ((struct mhd_H2RequestData *) (void*) r,
                   struct mhd_H2Stream, req);
  struct MHD_Connection *const c = s->c;
  size_t pos;
  unsigned int type_mask;
  const char *buff;

  mhd_assert (r->is_http2);
  mhd_assert (s->is_h2);

  if ((mhd_H2_REQ_STAGE_HEADERS_DECODING != s->req.stage)
      && (mhd_H2_REQ_STAGE_HEADERS_PROCESSING != s->req.stage)
      && (mhd_H2_REQ_STAGE_TRAILERS_DECODING != s->req.stage)
      && (mhd_H2_REQ_STAGE_TRAILERS_PROCESSING != s->req.stage))
    return false;

  mhd_assert (mhd_h2_items_debug_get_streamid (c->h2.mem.req_ib)
              == s->stream_id);

  type_mask = req_items_kind_mask (kind);
  buff = mhd_h2_items_get_strings_buffc (c->h2.mem.req_ib);

  pos = 0u;
  while (true)
  {
    const struct mhd_H2ReqItem *itm;

    itm = mhd_h2_items_get_item_nc (c->h2.mem.req_ib,
                                    pos++);

    if (NULL == itm)
      break;

    if (key_len != itm->name_len)
      continue;

    if (0u == (type_mask & (unsigned int) itm->kind))
      continue;

    if (! mhd_str_equal_lowercase_bin_n (key,
                                         buff + itm->offset,
                                         key_len))
      continue;

    if (mhd_H2_RIK_URI_PARAM_NV != itm->kind)
    {
      value_out->cstr = buff + itm->offset + itm->name_len + 1u;
      value_out->len = (size_t) itm->val_len;
    }
    else
    {
      value_out->cstr = NULL;
      value_out->len = 0u;
    }

    return true;
  }

  return false;
}


mhd_static_inline enum MHD_ValueKind
req_item_kind_h2_to_h1 (enum mhd_H2RequestItemKind h2kind)
{
  switch (h2kind)
  {
  case mhd_H2_RIK_HEADER:
  case mhd_H2_RIK_PSEUDOHEADER:
    return MHD_VK_HEADER;
  case mhd_H2_RIK_COOKIE:
    return MHD_VK_COOKIE;
  case mhd_H2_RIK_URI_PARAM:
  case mhd_H2_RIK_URI_PARAM_NV:
    return MHD_VK_URI_QUERY_PARAM;
  case mhd_H2_RIK_TRAILER:
    return MHD_VK_TRAILER;
  case mhd_H2_RIK_PLACEHOLDER:
  case mhd_H2_RIK_ELIMINATED:
  default:
    break;
  }
  return MHD_VK_HEADER;
}


MHD_INTERNAL
MHD_FN_PAR_NONNULL_ (1) size_t
mhd_h2_request_get_values_cb (struct MHD_Request *r,
                              enum MHD_ValueKind kind,
                              MHD_NameValueIterator iterator,
                              void *iterator_cls)
{
  struct mhd_H2Stream *const s =
    mhd_CNTNR_PTR ((struct mhd_H2RequestData *) (void*) r,
                   struct mhd_H2Stream, req);
  struct MHD_Connection *const c = s->c;
  size_t pos;
  unsigned int type_mask;
  const char *buff;
  size_t count;


  mhd_assert (r->is_http2);
  mhd_assert (s->is_h2);

  if ((mhd_H2_REQ_STAGE_HEADERS_DECODING != s->req.stage)
      && (mhd_H2_REQ_STAGE_HEADERS_PROCESSING != s->req.stage)
      && (mhd_H2_REQ_STAGE_TRAILERS_DECODING != s->req.stage)
      && (mhd_H2_REQ_STAGE_TRAILERS_PROCESSING != s->req.stage))
    return false;

  mhd_assert (mhd_h2_items_debug_get_streamid (c->h2.mem.req_ib)
              == s->stream_id);

  type_mask = req_items_kind_mask (kind);
  buff = mhd_h2_items_get_strings_buffc (c->h2.mem.req_ib);

  pos = 0u;
  count = 0u;
  while (true)
  {
    const struct mhd_H2ReqItem *itm;

    itm = mhd_h2_items_get_item_nc (c->h2.mem.req_ib,
                                    pos++);

    if (NULL == itm)
      break;

    if (((unsigned int) itm->kind) != (type_mask & (unsigned int) itm->kind))
      continue;

    ++count;
    if (NULL != iterator)
    {
      struct MHD_NameAndValue nv;

      nv.name.cstr = buff + itm->offset;
      nv.name.len = (size_t) itm->val_len;
      if (mhd_H2_RIK_URI_PARAM_NV != itm->kind)
      {
        nv.value.cstr = buff + itm->offset + itm->name_len + 1u;
        nv.value.len = (size_t) itm->val_len;
      }
      else
      {
        nv.value.cstr = NULL;
        nv.value.len = 0u;
      }

      if (MHD_NO ==
          iterator (iterator_cls,
                    req_item_kind_h2_to_h1 (itm->kind),
                    &nv))
        return count;
    }
  }

  return count;
}
