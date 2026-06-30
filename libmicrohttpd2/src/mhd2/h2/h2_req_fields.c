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
 * @file src/mhd2/h2/h2_dec_fields.c
 * @brief  Implementation of HTTP/2 request fields functions
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include <string.h>

#include "mhd_assert.h"
#include "mhd_unreachable.h"
#include "mhd_assume.h"

#include "mhd_constexpr.h"
#include "mhd_buffer.h"
#include "mhd_str_types.h"
#include "mhd_str_macros.h"

#include "mhd_connection.h"
#include "h2_conn_data.h"

#include "h2_stream_data.h"

#include "mhd_str.h"

#include "stream_process_request.h"

#include "h2_req_item_kinds.h"
#include "h2_req_item_struct.h"
#include "h2_req_items_funcs.h"

#include "hpack/mhd_hpack_codec.h"

#include "h2_proc_conn.h"
#include "h2_conn_streams.h"

#include "h2_req_fields.h"

MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1) MHD_FN_PAR_IN_ (2)
MHD_FN_PAR_INOUT_ (4) MHD_FN_PAR_OUT_ (5) enum mhd_H2DecFieldsResult
mhd_h2_req_fields_decode (struct mhd_HpackDecContext *restrict hk_dec,
                          const struct mhd_Buffer *restrict enc_data,
                          bool are_trailers,
                          struct mhd_H2ReqItemsBlock *restrict ib,
                          size_t *restrict left_unprocessed)
{
  mhd_constexpr unsigned int max_no_fields = 8u;
  const enum mhd_H2RequestItemKind field_kind =
    are_trailers ? mhd_H2_RIK_TRAILER : mhd_H2_RIK_HEADER;
  size_t pos;
  unsigned int no_fields;
  enum mhd_H2DecFieldsResult ret;

  pos = 0u;
  no_fields = 0u;
  ret = mhd_H2_DEC_FIELDS_OK;

  while (pos < enc_data->size)
  {
    size_t name_len;
    size_t val_len;
    size_t pos_incr;
    enum mhd_HpackDecResult res;
    struct mhd_Buffer buff;

    mhd_assert (mhd_H2_DEC_FIELDS_OK == ret);

    if (! mhd_h2_items_get_buff_new_item (ib,
                                          &buff))
      return mhd_H2_DEC_FIELDS_NO_SPACE;

    res = mhd_hpack_dec_data (hk_dec,
                              enc_data->size - pos,
                              (const uint8_t *) enc_data->data + pos,
                              buff.size,
                              buff.data,
                              &name_len,
                              &val_len,
                              &pos_incr);

    if (! mhd_HPACK_DEC_RES_IS_ERR (res))
    {
      mhd_assert (0u != pos_incr);
      pos += pos_incr;
    }

    switch (res)
    {
    case mhd_HPACK_DEC_RES_NO_NEW_FIELD:
      if (max_no_fields < ++no_fields)
      {
        ret = mhd_H2_DEC_FIELDS_PROT_ABUSE;
        break;
      }
      mhd_h2_items_cancel_new_item_buff (ib);
      continue;
    case mhd_HPACK_DEC_RES_NEW_FIELD:
      mhd_assert (buff.size >= (name_len + val_len + 2u));
      mhd_assert (0 == buff.data[name_len]);
      mhd_assert (0 == buff.data[name_len + 1 + val_len]);
      mhd_h2_items_add_new_item_buff (ib,
                                      name_len,
                                      val_len,
                                      (':' == buff.data[0]) ?
                                      mhd_H2_RIK_PSEUDOHEADER : field_kind);
      continue;
    case mhd_HPACK_DEC_RES_INCOMPLETE:
      mhd_assert (mhd_H2_DEC_FIELDS_OK == ret);
      break;
    case mhd_HPACK_DEC_RES_ALLOC_ERR:
      ret = mhd_H2_DEC_FIELDS_INT_ERR;
      break;
    case mhd_HPACK_DEC_RES_BUFFER_TOO_SMALL: // TODO: support "minimal" decoding for decoder state updates
    case mhd_HPACK_DEC_RES_STRING_TOO_LONG:
      ret = mhd_H2_DEC_FIELDS_NO_SPACE;
      break;
    case mhd_HPACK_DEC_RES_NUMBER_TOO_LONG:
    case mhd_HPACK_DEC_RES_DYN_SIZE_UPD_TOO_LARGE:
      ret = mhd_H2_DEC_FIELDS_PROT_ABUSE;
      break;
    case mhd_HPACK_DEC_RES_DYN_SIZE_UPD_MISSING:
    case mhd_HPACK_DEC_RES_HUFFMAN_ERR:
    case mhd_HPACK_DEC_RES_HPACK_BAD_IDX:
    case mhd_HPACK_DEC_RES_HPACK_ERR:
      ret = mhd_H2_DEC_FIELDS_BROKEN_DATA;
      break;
    case mhd_HPACK_DEC_RES_INTERNAL_ERR:
    default:
      mhd_UNREACHABLE_D ("Impossible value");
      ret = mhd_H2_DEC_FIELDS_INT_ERR;
      break;
    }
    mhd_h2_items_cancel_new_item_buff (ib);
    break; /* Break the loop */
  }

  mhd_assert (pos <= enc_data->size);
  *left_unprocessed = enc_data->size - pos;

  return ret;
}


static inline MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1) bool
req_validate_fields_chars (struct mhd_H2Stream *restrict s)
{
  // TODO: implement checking all field chars for validity, RFC 9113 Section 8.2.1
  (void) s;
  return true;
}


static inline MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1)
MHD_FN_PAR_OUT_ (2) bool
req_pseudoheaders_preprocess (struct mhd_H2Stream *restrict s,
                              size_t *pos)
{
  static const struct MHD_String ph_method = mhd_MSTR_INIT (":method");
  static const struct MHD_String ph_scheme = mhd_MSTR_INIT (":scheme");
  static const struct MHD_String ph_authority = mhd_MSTR_INIT (":authority");
  static const struct MHD_String ph_path = mhd_MSTR_INIT (":path");
  struct mhd_H2ReqItemsBlock *const ib = s->c->h2.mem.req_ib;
  const char *buff = mhd_h2_items_get_strings_buffc (ib);
  bool have_method = false;
  bool have_scheme = false;
  bool have_authority = false;
  bool have_path = false;

  *pos = 0u;
  while (1)
  {
    const struct mhd_H2ReqItem *item;
    item = mhd_h2_items_get_item_nc (ib,
                                     *pos);

    if (NULL == item)
      break;
    else if (mhd_H2_RIK_PSEUDOHEADER != item->kind)
      break;
    else if ((item->name_len == ph_method.len) &&
             (0 == memcmp (buff + item->offset,
                           ph_method.cstr,
                           ph_method.len)))
    {
      if (have_method)
        return mhd_h2_stream_req_problem (s,
                                          mhd_H2_REQ_PRBLM_PSEUDOHDR_DUP);
      have_method = true;
      s->req.pos_method = *pos;
      s->req.method =
        mhd_parse_http_method (item->val_len,
                               buff + item->offset + ph_method.len + 1u);
    }
    else if ((item->name_len == ph_path.len) &&
             (0 == memcmp (buff + item->offset,
                           ph_path.cstr,
                           ph_path.len)))
    {
      if (have_path)
        return mhd_h2_stream_req_problem (s,
                                          mhd_H2_REQ_PRBLM_PSEUDOHDR_DUP);
      have_path = true;
      s->req.pos_path = *pos;
    }
    else if ((item->name_len == ph_authority.len) &&
             (0 == memcmp (buff + item->offset,
                           ph_authority.cstr,
                           ph_authority.len)))
    {
      if (have_authority)
        return mhd_h2_stream_req_problem (s,
                                          mhd_H2_REQ_PRBLM_PSEUDOHDR_DUP);
      have_authority = true;
      s->req.pos_authority = *pos;
    }
    else if ((item->name_len == ph_scheme.len) &&
             (0 == memcmp (buff + item->offset,
                           ph_scheme.cstr,
                           ph_scheme.len)))
    {
      if (have_scheme)
        return mhd_h2_stream_req_problem (s,
                                          mhd_H2_REQ_PRBLM_PSEUDOHDR_DUP);
      have_scheme = true;
    }
    mhd_assert (':' == buff[item->offset]);
    ++(*pos);
  }

  if (! have_method)
    return mhd_h2_stream_req_problem (s,
                                      mhd_H2_REQ_PRBLM_PSEUDOHDR_MISSING);

  if (mhd_HTTP_METHOD_CONNECT != s->req.method)
  {
    if (! have_path || ! have_scheme)
      return mhd_h2_stream_req_problem (s,
                                        mhd_H2_REQ_PRBLM_PSEUDOHDR_EXTRA);
  }
  else
  {
    if (have_path || have_scheme)
      return mhd_h2_stream_req_problem (s,
                                        mhd_H2_REQ_PRBLM_PSEUDOHDR_MISSING);
  }

  return true;
}


static inline MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1)
MHD_FN_PAR_OUT_ (2) bool
req_headers_preprocess (struct mhd_H2Stream *restrict s,
                        size_t *pos)
{
  static const struct MHD_String h_host = mhd_MSTR_INIT ("host");
  static const struct MHD_String h_cntn_len = mhd_MSTR_INIT ("content-length");
  struct mhd_H2ReqItemsBlock *const ib = s->c->h2.mem.req_ib;
  const char *buff = mhd_h2_items_get_strings_buffc (ib);

  while (1)
  {
    const struct mhd_H2ReqItem *item;
    item = mhd_h2_items_get_item_nc (ib,
                                     *pos);

    if (NULL == item)
      break;
    else if (mhd_H2_RIK_PSEUDOHEADER == item->kind)
      return mhd_h2_stream_req_problem (s,
                                        mhd_H2_REQ_PRBLM_PSEUDOHDR_AFTER_HDR);
    else if (mhd_H2_RIK_HEADER != item->kind)
      (void) 0; /* skip */
    else if ((item->name_len == h_cntn_len.len) &&
             (0 == memcmp (buff + item->offset,
                           h_cntn_len.cstr,
                           h_cntn_len.len)))
    {
      uint_fast64_t cntnt_len;

      if ((0u == item->val_len) ||
          (item->val_len !=
           mhd_str_to_uint64 (buff + item->offset + h_cntn_len.len + 1u,
                              &cntnt_len)))
        return mhd_h2_stream_req_problem (s,
                                          mhd_H2_REQ_PRBLM_CNTNT_LEN_WRONG);

      if ((MHD_SIZE_UNKNOWN != s->req.cntn_size) &&
          (s->req.cntn_size != cntnt_len))
        return mhd_h2_stream_req_problem (s,
                                          mhd_H2_REQ_PRBLM_CNTNT_LEN_WRONG);
      else
        s->req.cntn_size = cntnt_len;
    }
    else if ((item->name_len == h_host.len) &&
             (0 == memcmp (buff + item->offset,
                           h_host.cstr,
                           h_host.len)))
    {
      if (mhd_H2_REQ_ITEM_POS_INVALID == s->req.pos_authority)
        s->req.pos_authority = *pos;
      else
      {
        const struct mhd_H2ReqItem *item_auth =
          mhd_h2_items_get_item_nc (ib,
                                    s->req.pos_authority);
        mhd_assert (NULL != item_auth);
        if ((item_auth->val_len != item->val_len) ||
            (! mhd_str_equal_caseless_bin_n (buff + item->offset
                                             + item->name_len + 1u,
                                             buff + item_auth->offset
                                             + item_auth->name_len + 1u,
                                             item->val_len)))
          return
            mhd_h2_stream_req_problem (s,
                                       mhd_H2_REQ_PRBLM_HOST_HDR_WRONG_EXTRA);
      }
    }

    ++(*pos);
    mhd_assert ((mhd_H2_RIK_HEADER != item->kind) ||
                (':' != buff[item->offset]));
  }

  return true;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1) bool
mhd_h2_req_headers_preprocess (struct mhd_H2Stream *restrict s)
{
  size_t pos;

  mhd_assert (mhd_h2_items_debug_get_streamid (s->c->h2.mem.req_ib)
              == s->stream_id);

  if (! req_validate_fields_chars (s))
    return false;

  if (! req_pseudoheaders_preprocess (s,
                                      &pos))
    return false;

  mhd_assert (0u != pos);
  mhd_assert (mhd_HTTP_METHOD_NO_METHOD != s->req.method);

  if (! req_headers_preprocess (s,
                                &pos))
    return false;

  return true;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1) bool
mhd_h2_req_uri_parse (struct mhd_H2Stream *restrict s)
{
  struct mhd_H2ReqItemsBlock *const restrict ib = s->c->h2.mem.req_ib;
  char *const restrict buff = mhd_h2_items_get_strings_buff (ib);
  struct mhd_H2ReqItem *restrict item =
    mhd_h2_items_get_item_n (ib,
                             s->req.pos_path);
  const size_t path_start = (size_t) (item->offset + item->name_len + 1u);
  char *questn_mark;

  questn_mark = (char*) memchr (buff + path_start,
                                '?',
                                (size_t) item->val_len);
  if (NULL == questn_mark)
  {
    item->val_len =
      (uint_least32_t)
      mhd_str_dec_norm_uri_path ((size_t) item->val_len,
                                 buff + path_start);
  }
  else
  {
    const size_t path_len = (size_t) (questn_mark - (buff + path_start));
    const size_t uri_end = (size_t) (path_start + item->val_len);
    size_t i = path_start + path_len + 1u;

    mhd_assert (path_len < item->val_len);

    buff[path_start + path_len] = '\0'; /* Zero-terminate the path */
    item->val_len =
      (uint_least32_t)
      mhd_str_dec_norm_uri_path (path_len,
                                 buff + path_start);

    do
    {
      size_t name_start;
      size_t name_len;
      size_t value_start;
      size_t value_len;

      value_start = 0u;
      for (name_start = i; i < uri_end; ++i) /* Processing parameter */
      {
        if ('+' == buff[i])
          buff[i] = ' ';
        else if ('=' == buff[i])
        {
          /* Found start of the value */
          for (value_start = ++i; i < uri_end; ++i) /* Processing parameter value */
          {
            if ('+' == buff[i])
              buff[i] = ' ';
            else if ('&' == buff[i]) /* delimiter for the next parameter */
              break; /* Next parameter */
          }
          break; /* End of the current parameter */
        }
        else if ('&' == buff[i])
          break; /* End of the name of the parameter without a value */
      }

      /* PCT-decode, zero-terminate and store the found parameter */

      if (0u != value_start) /* Value cannot start at zero position */
      { /* Name with value */
        mhd_assert (name_start + 1u <= value_start);
        name_len = value_start - name_start - 1u;
        value_len = i - value_start;
      }
      else
      { /* Name without value */
        name_len = i - name_start;
        value_len = 0u;
      }
      name_len = mhd_str_pct_decode_lenient_n (buff + name_start,
                                               name_len,
                                               buff + name_start,
                                               name_len,
                                               NULL); // TODO: add support for broken encoding detection
      buff[name_start + name_len] = 0;

      if (0u != value_start)
      {
        value_len =
          mhd_str_pct_decode_lenient_n (buff + name_start + name_len + 1u,
                                        value_len,
                                        buff + value_start,
                                        value_len,
                                        NULL); // TODO: add support for broken encoding detection
        buff[value_start + value_len] = 0;
      }

      if (! mhd_h2_items_reserve_new_item (ib))
        break; // TODO: support reporting no space errors

      mhd_h2_items_add_new_item_reserved (ib,
                                          name_start,
                                          name_len,
                                          value_len,
                                          (0u != value_start)
                                          ? mhd_H2_RIK_URI_PARAM :
                                          mhd_H2_RIK_URI_PARAM_NV);

    } while (uri_end > ++i);

  }

  return true;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1) bool
mhd_h2_req_cookie_parse (struct mhd_H2Stream *restrict s)
{
  // TODO: handle cookie combining
  // TODO: Implement cookie parsing
  return true;
}
