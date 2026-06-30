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
 * @file src/mhd2/h2/h2_req_items_funcs.c
 * @brief  Function for the request items (headers, URI params)
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "sys_base_types.h"

#include "mhd_align.h"
#include "mhd_predict.h"
#include "mhd_constexpr.h"

#include "mhd_assert.h"

#include "sys_malloc.h"

#include "mhd_buffer.h"
#include "mhd_str_types.h"

#include "h2_req_item_struct.h"

#include "h2_req_items_funcs.h"


struct mhd_H2ReqItemsBlock
{
  /**
   * Number of items in the items block
   */
  size_t num_items;

  /**
   * The size of the items buffer, in bytes
   */
  size_t buf_size;

  /**
   * The starting offset of the free buffer space
   */
  uint_least32_t start_free;

#ifndef NDEBUG
  uint_least32_t stream_id;
  bool buff_locked;
#endif /* ! NDEBUG */
};

mhd_constexpr size_t mhd_rii_size = sizeof (struct mhd_H2ReqItem);

mhd_static_inline char *
h2_ib_get_buff (struct mhd_H2ReqItemsBlock *ib)
{
  mhd_assert (ib->start_free <= ib->buf_size);
  return (char *) (ib + 1u);
}


mhd_static_inline const char *
h2_ib_get_buffc (const struct mhd_H2ReqItemsBlock *ib)
{
  mhd_assert (ib->start_free <= ib->buf_size);
  return (const char *) (ib + 1u);
}


mhd_static_inline struct mhd_H2ReqItem *
h2_ib_get_zero_item (struct mhd_H2ReqItemsBlock *ib)
{
  return ((struct mhd_H2ReqItem *)
          (void *) (h2_ib_get_buff (ib) + ib->buf_size))
         - 1u;
}


mhd_static_inline const struct mhd_H2ReqItem *
h2_ib_get_zero_itemc (const struct mhd_H2ReqItemsBlock *ib)
{
  return ((const struct mhd_H2ReqItem *)
          (const void *) (h2_ib_get_buffc (ib) + ib->buf_size))
         - 1u;
}


/* 'pos' is zero-based */
mhd_static_inline struct mhd_H2ReqItem *
h2_ib_get_n_item (struct mhd_H2ReqItemsBlock *ib,
                  size_t pos)
{
  struct mhd_H2ReqItem *const ret = h2_ib_get_zero_item (ib) - pos;
  mhd_assert (ib->buf_size >= (ib->start_free
                               + ib->num_items * mhd_rii_size));
  return ret;
}


/* 'pos' is zero-based */
mhd_static_inline const struct mhd_H2ReqItem *
h2_ib_get_n_itemc (const struct mhd_H2ReqItemsBlock *ib,
                   size_t pos)
{
  const struct mhd_H2ReqItem *const ret = h2_ib_get_zero_itemc (ib) - pos;
  mhd_assert (ib->buf_size >= (ib->start_free
                               + ib->num_items * mhd_rii_size));
  return ret;
}


mhd_static_inline size_t
h2_ib_get_buff_free_size (const struct mhd_H2ReqItemsBlock *ib)
{
  mhd_assert (ib->buf_size >= (ib->start_free
                               + ib->num_items * sizeof(struct mhd_H2ReqItem)));
  return ib->buf_size - ib->start_free - (ib->num_items * mhd_rii_size);
}


mhd_static_inline char *
h2_ib_get_buff_free_ptr (struct mhd_H2ReqItemsBlock *ib)
{
  return h2_ib_get_buff (ib) + ib->start_free;
}


MHD_INTERNAL mhd_FN_RET_UNALIASED
mhd_FN_OBJ_CONSTRUCTOR (mhd_h2_items_block_destroy)
struct mhd_H2ReqItemsBlock *
mhd_h2_items_block_create (size_t buffer_size)
{
  struct mhd_H2ReqItemsBlock *ret;
  uint_fast32_t buf_alloc_size;

  buf_alloc_size = (buffer_size & 0xFFFFFFFFu);
  if (mhd_COND_HARDLY_EVER ((0xFFFFFFFFu
                             - 2u * mhd_ALIGNOF (struct mhd_H2ReqItem))
                            > buffer_size))
    buf_alloc_size =
      (uint_fast32_t) (0xFFFFFFFFu - 2 * mhd_ALIGNOF (struct mhd_H2ReqItem));

  /* Round up to alignment */
  buf_alloc_size +=
    (uint_fast32_t)
    ((mhd_ALIGNOF (struct mhd_H2ReqItem)
      - (buf_alloc_size % mhd_ALIGNOF (struct mhd_H2ReqItem)))
     % mhd_ALIGNOF (struct mhd_H2ReqItem));

  /* Adjust the allocation size in case if alignment of mhd_H2ReqItem is
     stricter than alignment of mhd_H2ReqItemsBlock */
  buf_alloc_size +=
    (uint_fast32_t)
    ((mhd_ALIGNOF (struct mhd_H2ReqItem)
      - (sizeof(*ret) % mhd_ALIGNOF (struct mhd_H2ReqItem)))
     % mhd_ALIGNOF (struct mhd_H2ReqItem));

  mhd_assert ((buffer_size <= buf_alloc_size) || \
              (0xFFFFFFFFu - 2 * mhd_ALIGNOF (struct mhd_H2ReqItem) \
               <= buf_alloc_size));

  ret = (struct mhd_H2ReqItemsBlock *) malloc (sizeof (*ret) + buf_alloc_size);

  if (NULL == ret)
    return NULL; /* Failure exit point */

  ret->buf_size = (size_t) buf_alloc_size;
#ifndef NDEBUG
  ret->buff_locked = false;
#endif /* ! NDEBUG */
  mhd_h2_items_block_reset (ret);

  return ret;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ void
mhd_h2_items_block_destroy (struct mhd_H2ReqItemsBlock *ib)
{
  free (ib);
}


MHD_INTERNAL
MHD_FN_PAR_INOUT_ (1) void
mhd_h2_items_block_reset (struct mhd_H2ReqItemsBlock *restrict ib)
{
  mhd_assert (ib->start_free <= ib->buf_size);
  mhd_assert (! ib->buff_locked);

  ib->num_items = 0u;
  ib->start_free = 0u;

#ifndef NDEBUG
  ib->stream_id = 0u;
#endif /* ! NDEBUG */
}


MHD_INTERNAL
MHD_FN_PAR_INOUT_ (1) MHD_FN_PAR_OUT_ (2)
MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_items_get_buff_new_item (struct mhd_H2ReqItemsBlock *restrict ib,
                                struct mhd_Buffer *restrict buff)
{
  const size_t free_space = h2_ib_get_buff_free_size (ib);

  mhd_assert (! ib->buff_locked);

  if (mhd_rii_size + 2u > free_space) /* 2 for two zero-terminations */
    return false;

#ifndef NDEBUG
  ib->buff_locked = true;
#endif /* ! NDEBUG */

  buff->data = h2_ib_get_buff_free_ptr (ib);
  buff->size = free_space - mhd_rii_size;

  return true;
}


MHD_INTERNAL
MHD_FN_PAR_INOUT_ (1) MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_items_reserve_new_item (struct mhd_H2ReqItemsBlock *restrict ib)
{
  const size_t free_space = h2_ib_get_buff_free_size (ib);

  mhd_assert (! ib->buff_locked);

  if (mhd_rii_size + 2u > free_space) /* 2 for two zero-terminations */
    return false;

#ifndef NDEBUG
  ib->buff_locked = true;
#endif /* ! NDEBUG */

  return true;
}


MHD_INTERNAL
MHD_FN_PAR_INOUT_ (1) MHD_FN_PAR_NONNULL_ALL_ void
mhd_h2_items_add_new_item_buff (struct mhd_H2ReqItemsBlock *restrict ib,
                                size_t name_len,
                                size_t val_len,
                                enum mhd_H2RequestItemKind kind)
{
  struct mhd_H2ReqItem *const itm = h2_ib_get_n_item (ib, ib->num_items);

  mhd_assert (ib->buff_locked);
  mhd_assert (h2_ib_get_buff_free_size (ib) >= \
              name_len + val_len + 2u + mhd_rii_size);
  mhd_assert (0 == h2_ib_get_buff_free_ptr (ib)[name_len]);
  mhd_assert (0 == h2_ib_get_buff_free_ptr (ib)[name_len + 1 + val_len]);

  itm->kind = kind;
  itm->offset = ib->start_free;
  itm->name_len = (uint_least32_t) name_len;
  itm->val_len = (uint_least32_t) val_len;

  ib->start_free += (uint_least32_t) (name_len + val_len + 2u);
  ++ib->num_items;

#ifndef NDEBUG
  ib->buff_locked = false;
#endif /* ! NDEBUG */

  mhd_assert (ib->buf_size >= (ib->start_free
                               + ib->num_items * sizeof(struct mhd_H2ReqItem)));
}


MHD_INTERNAL
MHD_FN_PAR_INOUT_ (1) MHD_FN_PAR_NONNULL_ALL_ void
mhd_h2_items_add_new_item_reserved (struct mhd_H2ReqItemsBlock *restrict ib,
                                    size_t name_start,
                                    size_t name_len,
                                    size_t val_len,
                                    enum mhd_H2RequestItemKind kind)
{
  struct mhd_H2ReqItem *const itm = h2_ib_get_n_item (ib, ib->num_items);

  mhd_assert (ib->buff_locked);
  mhd_assert (h2_ib_get_buff_free_size (ib) >= mhd_rii_size);
  mhd_assert (0 == h2_ib_get_buffc (ib)[name_start + name_len]);
  mhd_assert ((mhd_H2_RIK_URI_PARAM_NV == kind) ||
              (0 == h2_ib_get_buffc (ib)[name_start + name_len + 1 + val_len]));
  mhd_assert (name_start < ib->start_free);
  mhd_assert (name_len + val_len + 2u <= ib->start_free);

  itm->kind = kind;
  itm->offset = (uint_least32_t) name_start;
  itm->name_len = (uint_least32_t) name_len;
  itm->val_len = (uint_least32_t) val_len;

  ++ib->num_items;

#ifndef NDEBUG
  ib->buff_locked = false;
#endif /* ! NDEBUG */

  mhd_assert (ib->buf_size >= (ib->start_free
                               + ib->num_items * sizeof(struct mhd_H2ReqItem)));
}


#ifndef NDEBUG
MHD_INTERNAL
MHD_FN_PAR_INOUT_ (1) MHD_FN_PAR_NONNULL_ALL_ void
mhd_h2_items_cancel_new_item_buff (struct mhd_H2ReqItemsBlock *restrict ib)
{
  mhd_assert (ib->buff_locked);
  ib->buff_locked = false;
}


#endif /* ! NDEBUG */


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ MHD_FN_RETURNS_NONNULL_
MHD_FN_PURE_ char *
mhd_h2_items_get_strings_buff (struct mhd_H2ReqItemsBlock *restrict ib)
{
  return h2_ib_get_buff (ib);
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ MHD_FN_RETURNS_NONNULL_
MHD_FN_PURE_ const char *
mhd_h2_items_get_strings_buffc (const struct mhd_H2ReqItemsBlock *restrict ib)
{
  return h2_ib_get_buffc (ib);
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PURE_ struct mhd_H2ReqItem *
mhd_h2_items_get_item_n (struct mhd_H2ReqItemsBlock *restrict ib,
                         size_t pos)
{
  if (ib->num_items <= pos)
    return NULL;
  return h2_ib_get_n_item (ib,
                           pos);
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PURE_ const struct mhd_H2ReqItem *
mhd_h2_items_get_item_nc (const struct mhd_H2ReqItemsBlock *restrict ib,
                          size_t pos)
{
  if (ib->num_items <= pos)
    return NULL;
  return h2_ib_get_n_itemc (ib,
                            pos);
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_ (3) MHD_FN_PURE_ bool
mhd_h2_items_get_item_name (struct mhd_H2ReqItemsBlock *restrict ib,
                            size_t pos,
                            struct MHD_String *restrict name)
{
  const struct mhd_H2ReqItem *const itm = h2_ib_get_n_itemc (ib,
                                                             pos);
  if (NULL == itm)
    return false;

  name->cstr = h2_ib_get_buffc (ib) + itm->offset;
  name->len = itm->name_len;
  mhd_assert (0 == name->cstr[name->len]);

  return true;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_ (3) MHD_FN_PURE_ bool
mhd_h2_items_get_item_value (struct mhd_H2ReqItemsBlock *restrict ib,
                             size_t pos,
                             struct MHD_String *restrict value)
{
  const struct mhd_H2ReqItem *const itm = h2_ib_get_n_itemc (ib,
                                                             pos);
  if (NULL == itm)
    return false;

  value->cstr = h2_ib_get_buffc (ib) + itm->offset + itm->name_len + 1u;
  value->len = itm->val_len;
  mhd_assert (0 == value->cstr[value->len]);

  return true;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_ (3) MHD_FN_PURE_ bool
mhd_h2_items_get_item_kind (struct mhd_H2ReqItemsBlock *restrict ib,
                            size_t pos,
                            enum mhd_H2RequestItemKind *restrict kind)
{
  const struct mhd_H2ReqItem *const itm = h2_ib_get_n_itemc (ib,
                                                             pos);
  if (NULL == itm)
    return false;

  *kind = itm->kind;
  mhd_assert (0u != (unsigned int) *kind);

  return true;

}


MHD_INTERNAL MHD_FN_PURE_ MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_ (3) MHD_FN_PAR_OUT_ (4) MHD_FN_PAR_OUT_ (5) bool
mhd_h2_items_get_item_full (struct mhd_H2ReqItemsBlock *restrict ib,
                            size_t pos,
                            struct MHD_String *restrict name,
                            struct MHD_String *restrict value,
                            enum mhd_H2RequestItemKind *restrict kind)
{
  const struct mhd_H2ReqItem *const itm = h2_ib_get_n_itemc (ib,
                                                             pos);
  const char *const buff = h2_ib_get_buffc (ib);

  if (NULL == itm)
    return false;

  name->cstr = buff + itm->offset;
  name->len = itm->name_len;
  value->cstr = buff + itm->offset + itm->name_len + 1u;
  value->len = itm->val_len;
  *kind = itm->kind;

  mhd_assert (0 == name->cstr[name->len]);
  mhd_assert (0 == value->cstr[value->len]);
  mhd_assert (0u != (unsigned int) *kind);

  return true;
}


#ifndef NDEBUG
MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ void
mhd_h2_items_debug_set_streamid (struct mhd_H2ReqItemsBlock *restrict ib,
                                 uint_least32_t stream_id)
{
  ib->stream_id = stream_id;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ uint_least32_t
mhd_h2_items_debug_get_streamid (struct mhd_H2ReqItemsBlock *restrict ib)
{
  return ib->stream_id;
}


#endif /* ! NDEBUG */
