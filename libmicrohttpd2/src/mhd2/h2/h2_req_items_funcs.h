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
 * @file src/mhd2/h2/h2_req_items_funcs.h
 * @brief  Declarations of the request items (headers, URI params) functions
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_H2_REQ_ITEMS_FUNCS_H
#define MHD_H2_REQ_ITEMS_FUNCS_H 1

#include "mhd_sys_options.h"

#include "sys_base_types.h"
#include "sys_bool_type.h"

#include "h2_req_item_kinds.h"

struct mhd_H2ReqItem;           /* Forward declaration */
struct mhd_H2ReqItemsBlock;     /* Forward declaration */
struct mhd_Buffer;              /* Forward declaration */
struct MHD_String;              /* Forward declaration */


MHD_INTERNAL void
mhd_h2_items_block_destroy (struct mhd_H2ReqItemsBlock *ib)
MHD_FN_PAR_NONNULL_ALL_;

/**
 * Create request items block
 * @param buffer_size the size of the items block, must be less than UINT32_MAX
 * @return the pointer to the new request items block if succeed,
 *         NULL if failed (out of memory)
 */
MHD_INTERNAL struct mhd_H2ReqItemsBlock *
mhd_h2_items_block_create (size_t buffer_size)
mhd_FN_RET_UNALIASED mhd_FN_OBJ_CONSTRUCTOR (mhd_h2_items_block_destroy);


/**
 * Reset request items block
 * @param ib the pointer to the previously initialised items block to reset
 */
MHD_INTERNAL void
mhd_h2_items_block_reset (struct mhd_H2ReqItemsBlock *restrict ib)
MHD_FN_PAR_INOUT_ (1) MHD_FN_PAR_NONNULL_ALL_;

/**
 * Allocates the buffer space for a new item.
 *
 * This function gives all available buffer space, excluding space for the
 * new item header.
 *
 * It must be finally followed by a single call of one of the
 * #mhd_h2_items_add_new_item_buff() or #mhd_h2_items_cancel_new_item_buff().
 * @param ib the pointer to items block data
 * @param[out] buff set to the available space in the buffer
 * @return 'true' if succeed,
 *         'false' if no space for a new item is available
 */
MHD_INTERNAL bool
mhd_h2_items_get_buff_new_item (struct mhd_H2ReqItemsBlock *restrict ib,
                                struct mhd_Buffer *restrict buff)
MHD_FN_PAR_INOUT_ (1) MHD_FN_PAR_OUT_ (2) MHD_FN_PAR_NONNULL_ALL_;

/**
 * Allocates the buffer space for a new item header, assuming that strings
 * will be placed over other allocated and then reduced item.
 *
 * It must be finally followed by a single call of one of the
 * #mhd_h2_mhd_h2_items_add_new_item_reserved() or
 * #mhd_h2_items_cancel_new_item_buff().
 * @param ib the pointer to items block data
 * @return 'true' if succeed,
 *         'false' if no space for a new item is available
 */
MHD_INTERNAL bool
mhd_h2_items_reserve_new_item (struct mhd_H2ReqItemsBlock *restrict ib)
MHD_FN_PAR_INOUT_ (1) MHD_FN_PAR_NONNULL_ALL_;

/**
 * Add a new item to the items block based on previously allocated space
 *
 * The new item must be located at the start of the buffer which must be
 * previously allocated by calling #mhd_h2_items_get_new_item_buff() function.
 *
 * The name string must be at zero position and must be zero-terminated.
 * The value string must start immediately after zero-termination of the name
 * string and must be zero-terminated too (the form is "name\0value\0").
 *
 * The strings must fit the buffer.
 * @param ib the pointer to items block data
 * @param name_len the length of the name string, not including mandatory
 *                 zero termination
 * @param val_len the length of the value string, not including mandatory
 *                zero termination
 */
MHD_INTERNAL void
mhd_h2_items_add_new_item_buff (struct mhd_H2ReqItemsBlock *restrict ib,
                                size_t name_len,
                                size_t val_len,
                                enum mhd_H2RequestItemKind kind)
MHD_FN_PAR_INOUT_ (1) MHD_FN_PAR_NONNULL_ALL_;

MHD_INTERNAL void
mhd_h2_items_add_new_item_reserved (struct mhd_H2ReqItemsBlock *restrict ib,
                                    size_t name_start,
                                    size_t name_len,
                                    size_t val_len,
                                    enum mhd_H2RequestItemKind kind)
MHD_FN_PAR_INOUT_ (1) MHD_FN_PAR_NONNULL_ALL_;


#ifndef NDEBUG
MHD_INTERNAL void
mhd_h2_items_cancel_new_item_buff (struct mhd_H2ReqItemsBlock *restrict ib)
MHD_FN_PAR_INOUT_ (1) MHD_FN_PAR_NONNULL_ALL_;

#else  /* NDEBUG */
#  define mhd_h2_items_cancel_new_item_buff(ib)         ((void) 0) /* do nothing */
#endif /* NDEBUG */

MHD_INTERNAL char *
mhd_h2_items_get_strings_buff (struct mhd_H2ReqItemsBlock *restrict ib)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_RETURNS_NONNULL_ MHD_FN_PURE_;

MHD_INTERNAL const char *
mhd_h2_items_get_strings_buffc (const struct mhd_H2ReqItemsBlock *restrict ib)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_RETURNS_NONNULL_ MHD_FN_PURE_;

/* return NULL if 'pos' does not exist */
MHD_INTERNAL struct mhd_H2ReqItem *
mhd_h2_items_get_item_n (struct mhd_H2ReqItemsBlock *restrict ib,
                         size_t pos)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PURE_;

/* return NULL if 'pos' does not exist */
MHD_INTERNAL const struct mhd_H2ReqItem *
mhd_h2_items_get_item_nc (const struct mhd_H2ReqItemsBlock *restrict ib,
                          size_t pos)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PURE_;

MHD_INTERNAL bool
mhd_h2_items_get_item_name (struct mhd_H2ReqItemsBlock *restrict ib,
                            size_t pos,
                            struct MHD_String *restrict name)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_OUT_ (3) MHD_FN_PURE_;

MHD_INTERNAL bool
mhd_h2_items_get_item_value (struct mhd_H2ReqItemsBlock *restrict ib,
                             size_t pos,
                             struct MHD_String *restrict value)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_OUT_ (3) MHD_FN_PURE_;

MHD_INTERNAL bool
mhd_h2_items_get_item_kind (struct mhd_H2ReqItemsBlock *restrict ib,
                            size_t pos,
                            enum mhd_H2RequestItemKind *restrict kind)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_OUT_ (3) MHD_FN_PURE_;

MHD_INTERNAL bool
mhd_h2_items_get_item_full (struct mhd_H2ReqItemsBlock *restrict ib,
                            size_t pos,
                            struct MHD_String *restrict name,
                            struct MHD_String *restrict value,
                            enum mhd_H2RequestItemKind *restrict kind)
MHD_FN_PURE_ MHD_FN_PAR_NONNULL_ALL_
  MHD_FN_PAR_OUT_ (3) MHD_FN_PAR_OUT_ (4) MHD_FN_PAR_OUT_ (5);

#ifndef NDEBUG
MHD_INTERNAL void
mhd_h2_items_debug_set_streamid (struct mhd_H2ReqItemsBlock *restrict ib,
                                 uint_least32_t stream_id)
MHD_FN_PAR_NONNULL_ALL_;

MHD_INTERNAL uint_least32_t
mhd_h2_items_debug_get_streamid (struct mhd_H2ReqItemsBlock *restrict ib)
MHD_FN_PAR_NONNULL_ALL_;

#else /* NDEBUG */
#  define mhd_h2_items_debug_set_streamid(ib,stream_id) ((void) 0)
#  define mhd_h2_items_debug_get_streamid(ib) ((void) 0)
#endif

#endif /* ! MHD_H2_REQ_ITEMS_FUNCS_H */
