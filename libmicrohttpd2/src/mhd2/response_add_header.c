/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2021-2024 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/response_add_header.c
 * @brief  The definitions of MHD_response_add_*header() functions
 * @author Karlson2k (Evgeny Grin)
 * @author Christian Grothoff
 */

#include "mhd_sys_options.h"

#include "mhd_str_macros.h"
#include "mhd_arr_num_elems.h"

#include "response_add_header.h"
#include "mhd_response.h"
#include "mhd_locks.h"

#include <string.h>
#include "sys_malloc.h"

#include "mhd_str.h"

#include "mhd_public_api.h"

#ifdef MHD_SUPPORT_HTTP2
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_CSTR_ (2) MHD_FN_PAR_IN_SIZE_ (2,1) bool
is_name_h2_allowed (size_t name_len,
                    const char name[MHD_FN_PAR_DYN_ARR_SIZE_ (name_len)])
{
  static const struct MHD_String h2_forbidden[] = {
    mhd_MSTR_INIT ("Connection"),
    mhd_MSTR_INIT ("Transfer-Encoding"),
    mhd_MSTR_INIT ("Upgrade"),
    mhd_MSTR_INIT ("Keep-Alive"),
    mhd_MSTR_INIT ("Proxy-Connection")
  };
  size_t i;

  for (i = 0u; i < mhd_ARR_NUM_ELEMS (h2_forbidden); ++i)
  {
    const struct MHD_String *const frbdn = h2_forbidden + i;

    if (frbdn->len != name_len)
      continue;
    if (mhd_str_equal_caseless_bin_n (frbdn->cstr,
                                      name,
                                      name_len))
      return false;
  }
  return true;
}


#endif /* MHD_SUPPORT_HTTP2 */

static
MHD_FN_PAR_NONNULL_ (1)
MHD_FN_PAR_NONNULL_ (3) MHD_FN_PAR_CSTR_ (3)
MHD_FN_PAR_NONNULL_ (5) MHD_FN_PAR_CSTR_ (5) bool
response_add_header_no_check (
  struct MHD_Response *restrict response,
  size_t name_len,
  const char *restrict name,
  size_t value_len,
  const char *restrict value)
{
  char *buf;
  size_t pos;
  struct mhd_ResponseHeader *new_hdr;
  size_t strings_size = 0u;
#ifdef MHD_SUPPORT_HTTP2
  bool h2_allowed;
  bool name_is_lower = false;
  size_t val_empty_prf = 0u;
  size_t val_empty_suf = 0u;

  mhd_assert (0 == name[name_len]);
  mhd_assert (0 == value[value_len]);

  h2_allowed = is_name_h2_allowed (name_len,
                                   name);
  if (h2_allowed)
  {
    name_is_lower = mhd_str_is_lowercase_bin_n (name_len,
                                                name);
    strings_size += (name_is_lower ? 0u : (name_len + 1u));

    while ((' ' == value[val_empty_prf]) || ('\t' == value[val_empty_prf]))
      ++val_empty_prf;
    if (val_empty_prf != value_len)
    {
      while ((' ' == value[value_len - 1u - val_empty_suf])
             || ('\t' == value[value_len - 1u - val_empty_suf]))
        ++val_empty_suf;
    }

    mhd_assert (val_empty_prf <= value_len);
    mhd_assert ((val_empty_suf < value_len) || (0u == value_len));
    mhd_assert (val_empty_prf + val_empty_suf <= value_len);

    if ((0u != val_empty_prf) || (0u != val_empty_suf))
      strings_size += value_len + 1u - val_empty_prf - val_empty_suf;
  }
#endif /* MHD_SUPPORT_HTTP2 */

  mhd_assert (0 == name[name_len]);
  mhd_assert (0 == value[value_len]);

  strings_size += name_len + 1u;
  strings_size += value_len + 1u;

  new_hdr = (struct mhd_ResponseHeader *)
            malloc (sizeof(struct mhd_ResponseHeader) + strings_size);
  if (NULL == new_hdr)
    return false;

  buf = ((char *) new_hdr) + sizeof(struct mhd_ResponseHeader);
  pos = 0u;
  memcpy (buf + pos, name, name_len + 1u);
  new_hdr->name.cstr = buf + pos;
  new_hdr->name.len = name_len;
  pos += name_len + 1u;
  memcpy (buf + pos, value, value_len + 1u);
  new_hdr->value.cstr = buf + pos;
  new_hdr->value.len = value_len;
  pos += value_len + 1u;

#ifdef MHD_SUPPORT_HTTP2
  if (h2_allowed)
  {
    if (! name_is_lower)
    {
      mhd_str_to_lowercase_bin_n (name_len + 1u,
                                  name,
                                  buf + pos);
      new_hdr->h2.name.data = buf + pos;
      new_hdr->h2.name.size = name_len;
      pos += name_len + 1u;
    }
    else
    {
      new_hdr->h2.name.data = new_hdr->name.cstr;
      new_hdr->h2.name.size = new_hdr->name.len;
    }

    if ((0u != val_empty_prf) || (0u != val_empty_suf))
    {
      memcpy (buf + pos,
              value + val_empty_prf,
              value_len - val_empty_prf - val_empty_suf);
      buf[pos + value_len - val_empty_prf - val_empty_suf] = 0;
      new_hdr->h2.value.data = buf + pos;
      new_hdr->h2.value.size = value_len - val_empty_prf - val_empty_suf;
      pos += value_len - val_empty_prf - val_empty_suf + 1u;
    }
    else
    {
      new_hdr->h2.value.data = new_hdr->value.cstr;
      new_hdr->h2.value.size = new_hdr->value.len;
    }
    // TODO: implement checking name for "never-index" patterns and other indexing preferences patterns
  }
  else
  {
    new_hdr->h2.name.data = NULL;
    new_hdr->h2.name.size = 0u;
    new_hdr->h2.value.data = NULL;
    new_hdr->h2.value.size = 0u;
  }
  mhd_assert ((NULL != new_hdr->h2.name.data)
              || (0u == new_hdr->h2.name.size));
  mhd_assert ((NULL != new_hdr->h2.value.data)
              || (0u == new_hdr->h2.value.size));
  mhd_assert ((NULL != new_hdr->h2.name.data) || \
              (NULL == new_hdr->h2.value.data));
  mhd_assert ((NULL != new_hdr->h2.value.data) || \
              (NULL == new_hdr->h2.name.data));
#endif /* MHD_SUPPORT_HTTP2 */

  mhd_assert (strings_size == pos);
  (void) pos; /* Mute compiler warning in non-debug builds */

  mhd_DLINKEDL_INIT_LINKS (new_hdr, headers);
  mhd_DLINKEDL_INS_LAST (response, new_hdr, headers);
  return true;
}


MHD_INTERNAL
MHD_FN_PAR_NONNULL_ (1) void
mhd_response_remove_all_headers (struct MHD_Response *restrict r)
{
  struct mhd_ResponseHeader *hdr;

  for (hdr = mhd_DLINKEDL_GET_LAST (r, headers); NULL != hdr;
       hdr = mhd_DLINKEDL_GET_LAST (r, headers))
  {
    mhd_DLINKEDL_DEL (r, hdr, headers);
    free (hdr);
  }
}


static enum MHD_StatusCode
response_add_header_int (struct MHD_Response *restrict response,
                         const char *restrict name,
                         const char *restrict value)
{
  const size_t name_len = strlen (name);
  const size_t value_len = strlen (value);

  if (response->frozen) /* Re-check with the lock held */
    return MHD_SC_TOO_LATE;

  if ((NULL != memchr (name, ' ', name_len)) ||
      (NULL != memchr (name, '\t', name_len)) ||
      (NULL != memchr (name, ':', name_len)) ||
      (NULL != memchr (name, '\n', name_len)) ||
      (NULL != memchr (name, '\r', name_len)))
    return MHD_SC_RESP_HEADER_NAME_INVALID;
  if ((NULL != memchr (value, '\n', value_len)) ||
      (NULL != memchr (value, '\r', value_len)))
    return MHD_SC_RESP_HEADER_VALUE_INVALID;

  if (! response_add_header_no_check (response, name_len, name,
                                      value_len, value))
    return MHD_SC_RESPONSE_HEADER_MEM_ALLOC_FAILED;

  return MHD_SC_OK;
}


MHD_EXTERN_
MHD_FN_PAR_NONNULL_ (2) MHD_FN_PAR_CSTR_ (2)
MHD_FN_PAR_NONNULL_ (3) MHD_FN_PAR_CSTR_ (3) enum MHD_StatusCode
MHD_response_add_header (struct MHD_Response *MHD_RESTRICT response,
                         const char *MHD_RESTRICT name,
                         const char *MHD_RESTRICT value)
{
  bool need_unlock;
  enum MHD_StatusCode res;

  if (NULL == response)
    return MHD_SC_RESP_POINTER_NULL;
  if (response->frozen)
    return MHD_SC_TOO_LATE;

  if (response->reuse.reusable)
  {
    need_unlock = true;
    if (! mhd_mutex_lock (&(response->reuse.settings_lock)))
      return MHD_SC_RESPONSE_MUTEX_LOCK_FAILED;
    mhd_assert (1 == mhd_atomic_counter_get (&(response->reuse.counter)));
  }
  else
    need_unlock = false;

  if (mhd_str_equal_caseless (MHD_HTTP_HEADER_DATE,
                              name))
  {
    if (response->cfg.has_hdr_date)
    {
      if (need_unlock)
        mhd_mutex_unlock_chk (&(response->reuse.settings_lock));
      return MHD_SC_DATE_HEADER_SEVERAL;
    }
    response->cfg.has_hdr_date = true;
  }
  if (mhd_str_equal_caseless (MHD_HTTP_HEADER_CONNECTION,
                              name))
  {
    if (response->cfg.has_hdr_conn)
    {
      if (need_unlock)
        mhd_mutex_unlock_chk (&(response->reuse.settings_lock));
      return MHD_SC_CONNECTION_HEADER_SEVERAL;
    }
    response->cfg.has_hdr_conn = true;
  }

  // TODO: add special processing for "Content-Length", "Transfer-Encoding"

  res = response_add_header_int (response, name, value);

  if (need_unlock)
    mhd_mutex_unlock_chk (&(response->reuse.settings_lock));

  return res;
}


MHD_EXTERN_
MHD_FN_PAR_NONNULL_ (1)
MHD_FN_PAR_NONNULL_ (3) MHD_FN_PAR_CSTR_ (3) enum MHD_StatusCode
MHD_response_add_predef_header (struct MHD_Response *MHD_RESTRICT response,
                                enum MHD_PredefinedHeader stk,
                                const char *MHD_RESTRICT content)
{
  (void) response; (void) stk; (void) content;
  return MHD_SC_FEATURE_DISABLED;
}
