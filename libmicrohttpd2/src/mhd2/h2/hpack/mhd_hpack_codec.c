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
 * @file src/mhd2/h2/hpack/mhd_hpack_codec.c
 * @brief  The implementation of the HPACK header-compression codec functions.
 * @author Karlson2k (Evgeny Grin)
 *
 * The sizes of all strings are intentionally limited to 32 bits (4GiB).
 * The sizes of all strings in the dynamic table are limited to 32 or 16 bits,
 * depending on value of #mhd_HPACK_DTBL_BITS macro.
 */

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"
#include "sys_malloc.h"
#include <string.h>

#include "mhd_constexpr.h"
#include "mhd_align.h"

#include "mhd_assert.h"
#include "mhd_unreachable.h"
#include "mhd_predict.h"

#include "mhd_bithelpers.h"

#include "mhd_str_types.h"
#include "mhd_str_macros.h"
#include "mhd_buffer.h"

#include "mhd_tristate.h"
#include "mhd_hpack_dec_types.h"
#include "mhd_hpack_enc_types.h"

#include "mhd_hpack_codec.h"
#if ! defined(mhd_HPACK_TESTING_TABLES_ONLY) || ! defined(MHD_UNIT_TESTING)
#  include "h2_huffman_codec.h"
#  include "h2_huffman_est.h"
#endif


/**
 * Number of entries in the static table
 */
#define mhd_HPACK_STBL_ENTRIES          (61u)

/**
 * The last HPACK index number in the static table
 */
#define mhd_HPACK_STBL_LAST_IDX         mhd_HPACK_STBL_ENTRIES


/* ****** ----------------- Dynamic table handling ----------------- ****** */

/* ========================================================================
 *
 *  The dynamic tables should be accessed only by mhd_* functions.
 *
 *  All functions prefixed with dtbl_* are internal helpers and should not
 *  be used directly.
 *
 * ========================================================================
 */

#if mhd_HPACK_DTBL_BITS == 32
/**
 * A type used to store sizes of dynamic table elements.
 *
 * This is a compact type; it uses the minimal amount of memory.
 */
typedef uint_least32_t dtbl_size_t;
/**
 * A type used to operate on sizes of dynamic and static table elements
 *
 * This type should be more friendly for faster processing by CPU.
 * It could be the same underlying type as @a dtbl_size_t
 */
typedef uint_fast32_t dtbl_size_ft;
/**
 * A type used to store the number of dynamic and static table elements
 *
 * This is a compact type; it uses the minimal amount of memory.
 */
typedef uint_least32_t dtbl_idx_t;
/**
 * A type used to operate and address dynamic and static table elements
 *
 * This type should be more friendly for faster processing by CPU.
 * It could be the same underlying type as @a dtbl_idx_t
 */
typedef uint_fast32_t dtbl_idx_ft;
/**
 * Check whether value @a val fits 32 bits type.
 *
 * If any non-zero bit is set above the lowest 32 bits, the macro returns
 * boolean false.
 *
 * This macro strictly checks whether the provided value is suitable for use
 * in dynamic table elements. Even if the underlying type uint_least32_t is
 * wider than 32 bits, this macro enforces the limit to 32 bits only.
 *
 * This macro is designed to work only with unsigned types. No signed types
 * are used in dynamic table data.
 *
 * The parameter is evaluated only once.
 */
#  define mhd_DTBL_VALUE_FITS(val)      (0xFFFFFFFFu == ((val) | 0xFFFFFFFFu))
#elif mhd_HPACK_DTBL_BITS == 16
/**
 * A type used to store sizes of dynamic table elements.
 *
 * This is a compact type; it uses the minimal amount of memory.
 */
typedef uint_least16_t dtbl_size_t;
/**
 * A type used to operate sizes of dynamic and static table elements
 *
 * This type should be more friendly for faster processing by CPU.
 * It could be the same underlying type as @a dtbl_size_t
 */
typedef uint_fast16_t dtbl_size_ft;
/**
 * A type used to store the number of dynamic and static table elements
 *
 * This is a compact type; it uses the minimal amount of memory.
 */
typedef uint_least16_t dtbl_idx_t;
/**
 * A type used to operate and address dynamic and static table elements
 *
 * This type should be more friendly for faster processing by CPU.
 * It could be the same underlying type as @a dtbl_idx_t
 */
typedef uint_fast16_t dtbl_idx_ft;
/**
 * Check whether value @a val fits 16 bits type.
 *
 * If any non-zero bit is set above the lowest 16 bits, the macro returns
 * boolean false.
 *
 * This macro strictly checks whether the provided value is suitable for use
 * in dynamic table elements. Even if the underlying type uint_least16_t is
 * wider than 16 bits, this macro enforces the limit to 16 bits only.
 *
 * This macro is designed to work only with unsigned types. No signed types
 * are used in dynamic table data.
 *
 * The parameter is evaluated only once.
 */
#  define mhd_DTBL_VALUE_FITS(val)      (0xFFFFu == ((val) | 0xFFFFu))
#else
#error Unsupported mhd_HPACK_DTBL_BITS value
#endif


/**
 * The data for a dynamic table entry
 */
struct mhd_HpackDTblEntryInfo
{
  /**
   * The offset of the name in the buffer
   */
  dtbl_size_t offset;
  /**
   * The length of the name string.
   * The name string is not zero-terminated.
   */
  dtbl_size_t name_len;
  /**
   * The length of the value string.
   * The value is located at @a offset + @a name_len.
   * The value string is not zero-terminated.
   */
  dtbl_size_t val_len;
};

/**
 * Size (in bytes) of one dynamic-table entry-information record.
 */
#define mhd_DTBL_ENTRY_INFO_SIZE \
        ((dtbl_size_t) (sizeof(struct mhd_HpackDTblEntryInfo)))

/**
 * HPACK dynamic-table per-entry overhead, in bytes (RFC 7541 4.1).
 *
 * The macro is needed to statically initialise mhd_dtbl_entry_slack
 * in C11 mode (as 'static const' variable).
 */
#define mhd_HPACK_ENTRY_OVERHEAD (32u)

/**
 * HPACK dynamic-table per-entry overhead, in bytes (RFC 7541 4.1).
 * The size of a dynamic-table entry is:
 *   32 + length(header field name) + length(header field value),
 * where both lengths are in bytes as defined in RFC 7541 5.2.
 */
mhd_constexpr dtbl_size_t mhd_dtbl_entry_overhead =
  mhd_HPACK_ENTRY_OVERHEAD;


/**
 * The extra slack between entries in the strings buffer.
 * Used when there is extra space while adding a new entry.
 * This extra slack reduces the need to move strings in the buffer when the
 * entry is evicted and the strings are replaced with the new entry's strings.
 *
 * If strings are placed optimally (with this slack), then one entry took
 * exactly the formal HPACK size in the buffer (strings + entry information
 * data).
 */
mhd_constexpr dtbl_size_t mhd_dtbl_entry_slack =
  mhd_HPACK_ENTRY_OVERHEAD - mhd_DTBL_ENTRY_INFO_SIZE;

/**
 * The first HPACK index in the dynamic table
 */
mhd_constexpr dtbl_idx_t mhd_dtbl_hpack_idx_offset =
  mhd_HPACK_STBL_LAST_IDX + 1u;

/**
 * The maximum possible HPACK index when largest possible size of the dynamic
 * table is used
 */
#define mhd_HPACK_MAX_POSSIBLE_IDX \
        ((mhd_DTBL_MAX_SIZE / mhd_HPACK_ENTRY_OVERHEAD) \
         + mhd_HPACK_STBL_LAST_IDX)

/**
 * Get the formal HPACK size of the potential new entry.
 * @param strings_len the total size of the strings (the length of the name of
 *                    the field + the length of the value of the field)
 * @return the formal HPACK size of the potential new entry
 */
MHD_FN_PURE_ mhd_static_inline dtbl_size_t
dtbl_new_entry_strs_size_formal (dtbl_size_ft strings_len)
{
  dtbl_size_ft formal_size = strings_len + mhd_dtbl_entry_overhead;
  mhd_assert (strings_len < formal_size);
  mhd_assert (mhd_DTBL_VALUE_FITS (strings_len));
  mhd_assert (mhd_DTBL_VALUE_FITS (formal_size));
  return (dtbl_size_t) formal_size;
}


/**
 * Get the formal HPACK size of the potential new entry.
 * @param name_len the length of the name of the field
 * @param val_len the length of the value of the field
 * @return the formal HPACK size of the potential new entry
 */
MHD_FN_PURE_ mhd_static_inline dtbl_size_t
dtbl_new_entry_size_formal (dtbl_size_ft name_len,
                            dtbl_size_ft val_len)
{
  const dtbl_size_ft entry_strs_size = name_len + val_len;
  mhd_assert (val_len <= entry_strs_size);
  mhd_assert (mhd_DTBL_VALUE_FITS (entry_strs_size));
  mhd_assert (mhd_DTBL_VALUE_FITS (name_len));
  mhd_assert (mhd_DTBL_VALUE_FITS (val_len));
  return dtbl_new_entry_strs_size_formal (entry_strs_size);
}


/**
 * Get the total size of the strings of the entry.
 * This is the minimal size required for the entry in the strings buffer.
 * @param entr_inf the pointer to the entry info
 * @return the total size of the strings of the entry
 */
MHD_FN_PURE_ mhd_static_inline dtbl_size_t
dtbl_entr_strs_size_min (const struct mhd_HpackDTblEntryInfo *entr_inf)
{
  return entr_inf->name_len + entr_inf->val_len;
}


/**
 * Get the total size of the strings of the entry plus standard slack size.
 * This is the optimal size used for the entry in the strings buffer when the
 * current insertion slot has enough space.
 * @param entr_inf the pointer to the entry info
 * @return the total size of the strings of the entry plus standard slack size
 */
MHD_FN_PURE_ mhd_static_inline dtbl_size_t
dtbl_entr_strs_size_optm (const struct mhd_HpackDTblEntryInfo *entr_inf)
{
  return dtbl_entr_strs_size_min (entr_inf) + mhd_dtbl_entry_slack;
}


/**
 * Get the formal HPACK size of the entry.
 * The formal size of the entry is the size of the strings plus fixed
 * HPACK per-entry overhead.
 * @param entr_inf the pointer to the entry info
 * @return the formal HPACK size of the entry
 */
MHD_FN_PURE_ mhd_static_inline dtbl_size_t
dtbl_entr_size_formal (const struct mhd_HpackDTblEntryInfo *entr_inf)
{
  const dtbl_size_t ret = dtbl_new_entry_size_formal (entr_inf->name_len,
                                                      entr_inf->val_len);
  mhd_assert (dtbl_entr_strs_size_min (entr_inf) + mhd_dtbl_entry_overhead == \
              ret);
  return ret;
}


/**
 * Get the position (offset) of the (inclusive) start of the entry's strings
 * in the strings buffer.
 * This points to the first byte of the entry's strings. If the entry has
 * zero-length strings, the pointer denotes a (possibly zero-sized) area
 * that may coincide with the start of the entry's slack (if any) or with
 * the next entry's strings start (if present).
 * @param entr_inf the pointer to the entry info
 * @return the position (offset) of the (inclusive) start of the entry's strings
 */
MHD_FN_PURE_ mhd_static_inline dtbl_size_t
dtbl_entr_strs_start (const struct mhd_HpackDTblEntryInfo *entr_inf)
{
  return entr_inf->offset;
}


/**
 * Get the position of the (exclusive) end of the entry's strings in the
 * strings buffer.
 * This points to the next char (byte) after the strings of the entry.
 * @param entr_inf the pointer to the entry info
 * @return the position of the end of the entry's strings in the strings buffer
 */
MHD_FN_PURE_ mhd_static_inline dtbl_size_t
dtbl_entr_strs_end_min (const struct mhd_HpackDTblEntryInfo *entr_inf)
{
  return dtbl_entr_strs_start (entr_inf) + dtbl_entr_strs_size_min (entr_inf);
}


/**
 * Get the position (offset) immediately after the standard slack following the
 * end of the entry's strings in the strings buffer.
 * This points to the preferred position of the next entry's strings.
 * @param entr_inf the pointer to the entry info
 * @return the position (offset) immediately after the standard slack
 */
MHD_FN_PURE_ mhd_static_inline dtbl_size_t
dtbl_entr_strs_end_optm (const struct mhd_HpackDTblEntryInfo *entr_inf)
{
  return dtbl_entr_strs_start (entr_inf) + dtbl_entr_strs_size_optm (entr_inf);
}


/*
 * The dynamic HPACK table is organised as follows:
 * + The shared buffer is placed immediately after mhd_HpackDTblContext in
 *   memory.
 * + The buffer stores both the strings (names and values) and the entry info
 *   data (one mhd_HpackDTblEntryInfo per entry).
 * + Strings grow upward from the bottom of the buffer (lower addresses), while
 *   entry-info data grow downward from the top of the buffer (higher
 *   addresses).
 * + Because the buffer is shared, the same area may be used either by strings
 *   (few entries with large strings) or by entry info data (many entries with
 *   small strings).
 * + The topmost entry info data corresponds to the bottommost strings, and
 *   vice versa.
 * + Both regions (strings and entry info data) effectively form two circular
 *   buffers that dynamically share the same memory space region: the bottom
 *   part is strings area (filled from bottom to up) and the upper part is
 *   entries info data area (filled from top to down). See also "zero position"
 *   and the "edge entry" below.
 * + The table data tracks the newest entry; the new entries are added at
 *   higher (than the newest entry) location numbers.
 * + HPACK indices are counted in the opposite direction (the smallest HPACK
 *   index refers to the newest entry; the next entry's location number is the
 *   newest location minus one).
 * + Because the size of an entry info (sizeof(struct mhd_HpackDTblEntryInfo))
 *   is smaller than the mandatory HPACK per-entry overhead (32 bytes),
 *   strings are inserted with an additional slack when there is enough space
 *   before the next entry's strings.
 *
 * Terminology used below:
 * + "entry info" (or "entry information data") -- mhd_HpackDTblEntryInfo data.
 * + "zero position entry" -- the entry whose strings are at the bottom of the
 *   buffer and whose entry info data is at the very top of the buffer. This
 *   is the first entry added to an empty table.
 * + "edge entry" -- the entry whose strings lie above all other strings and
 *   whose entry info data lies below all other entry info data. Any space
 *   between this entry's strings and its entry info data is not used by
 *   other entries.
 * + "newest" (or latest) entry -- the most recently added entry (the
 *   lowest HPACK index).
 * + "oldest" entry -- the entry added before all other current entries; its
 *   strings immediately follow the newest entry's strings (or are at location
 *   zero if the newest entry is the edge entry). Its entry info data
 *   immediately precedes the newest entry's data (or is at the top if the
 *   newest entry is the edge entry).
 */

/**
 * Dynamic HPACK table data
 */
struct mhd_HpackDTblContext
{
  /**
   * The size of the allocated buffer.
   * The buffer is located in memory right after this structure.
   */
  dtbl_size_t buf_alloc_size;

  /**
   * The current number of entries used
   */
  dtbl_idx_t num_entries;

  /**
   * Offset of the current newest (most recently added) entry; it also has
   * the lowest HPACK index.
   * The "next" entry (newest_pos + 1, or 0 when the newest entry is the
   * edge entry (newest_pos == num_entries - 1)) is the oldest entry and
   * is evicted first if needed.
   * When a new entry is added, newest_pos is incremented or wrapped to 0
   * (when the newest entry is at the edge and insertion wraps).
   */
  dtbl_idx_t newest_pos;

  /**
   * The cached value of the official table size (as defined by HPACK).
   * Used to speed up calculations. Can be re-created from entries information.
   */
  dtbl_size_t cur_size;

  /**
   * The dynamic table size limit as defined by HPACK
   */
  dtbl_size_t size_limit;
};


/* **** ---------- Dynamic table internal helpers -------------- **** */

/* ** Basic table information ** */


/**
 * Get the number of entries in the table
 * @param dyn the pointer to the dynamic table structure
 * @return the number of entries in the table
 */
MHD_FN_PURE_ mhd_static_inline dtbl_idx_t
dtbl_get_num_entries (const struct mhd_HpackDTblContext *dyn)
{
  return dyn->num_entries;
}


/**
 * Check whether the table is empty (no entries)
 * @param dyn the pointer to the dynamic table structure
 * @return 'true' if table has no entries,
 *         'false' otherwise
 */
MHD_FN_PURE_ mhd_static_inline bool
dtbl_is_empty (const struct mhd_HpackDTblContext *dyn)
{
  mhd_assert ((0u == dyn->num_entries) == (0u == dyn->cur_size));
  return (0u == dtbl_get_num_entries (dyn));
}


/**
 * Get the pointer to the strings buffer
 * @param dyn the pointer to the dynamic table structure
 * @return the pointer to the strings buffer
 */
MHD_FN_CONST_ mhd_static_inline char *
dtbl_get_strs_buff (struct mhd_HpackDTblContext *dyn)
{
  return (char*)
         (dyn + 1u);
}


/**
 * Get a const pointer to the strings buffer
 * @param dyn the pointer to the dynamic table structure
 * @return const pointer to the strings buffer
 */
MHD_FN_CONST_ mhd_static_inline const char *
dtbl_get_strs_buffc (const struct mhd_HpackDTblContext *dyn)
{
  return (const char*)
         (dyn + 1u);
}


/**
 * Get the pointer to the top (by location in memory) dynamic table entry data.
 * The entries info data grow downward.
 * @param dyn the pointer to the dynamic table structure
 * @return the pointer to the top (by location in memory) entry data
 */
MHD_FN_PURE_ mhd_static_inline struct mhd_HpackDTblEntryInfo *
dtbl_get_infos (struct mhd_HpackDTblContext *dyn)
{
  return ((struct mhd_HpackDTblEntryInfo *)
          (void *)
          (dtbl_get_strs_buff (dyn) + dyn->buf_alloc_size)) - 1u;
}


/**
 * Get a const pointer to the top (by location in memory) dynamic table entry
 * data.
 * The entries info data grow downward.
 * @param dyn const pointer to the dynamic table structure
 * @return const pointer to the top (by location in memory) entry data
 */
MHD_FN_PURE_ mhd_static_inline const struct mhd_HpackDTblEntryInfo *
dtbl_get_infosc (const struct mhd_HpackDTblContext *dyn)
{
  return ((const struct mhd_HpackDTblEntryInfo *)
          (const void *)
          (dtbl_get_strs_buffc (dyn) + dyn->buf_alloc_size)) - 1u;
}


/**
 * Get the position of the entry located at the edge of the buffer.
 *
 * This is the entry with the strings located above all other strings
 * and the entry information data located below all other entries information
 * data.
 *
 * If any space is left between entry's strings data and information data, this
 * space is not used by other entries.
 *
 * The result is undefined if the table has no entries.
 * @param dyn the pointer to the dynamic table structure
 * @return the position of the edge entry,
 *         undefined if the table has no entries
 */
MHD_FN_PURE_ mhd_static_inline dtbl_idx_t
dtbl_get_pos_edge (const struct mhd_HpackDTblContext *dyn)
{
  mhd_assert (! dtbl_is_empty (dyn));
  mhd_assert (dyn->buf_alloc_size >=
              mhd_DTBL_ENTRY_INFO_SIZE * dyn->num_entries);
  return (dtbl_idx_t) (dyn->num_entries - 1u);
}


/**
 * Get the position of the previous entry for the specified entry position.
 *
 * This is a position of the entry previous to the specified entry position.
 * The returned value is one less than the specified position or wraps to the
 * edge position when the specified position is zero.
 *
 * The result is undefined if the table has no entries.
 * @param dyn the pointer to the dynamic table structure
 * @param loc_pos the number of location position
 * @return the position of the previous entry for specified entry position,
 *         undefined if the table has no entries
 */
MHD_FN_PURE_ mhd_static_inline dtbl_idx_t
dtbl_get_pos_prev (const struct mhd_HpackDTblContext *dyn,
                   dtbl_idx_ft loc_pos)
{
  mhd_assert (! dtbl_is_empty (dyn));
  mhd_assert (loc_pos <= dtbl_get_pos_edge (dyn));
#ifdef MHD_USE_CODE_HARDENING
  if (0u == loc_pos)
    return dtbl_get_pos_edge (dyn);
  return (dtbl_idx_t) (loc_pos - 1u);
#else  /* ! MHD_USE_CODE_HARDENING */
  return (dtbl_idx_t) ((dyn->num_entries + loc_pos - 1u) % dyn->num_entries);
#endif /* ! MHD_USE_CODE_HARDENING */
}


/**
 * Get the position of the next entry for the specified entry position.
 *
 * This is a position of the entry next to the specified entry position.
 * The returned value is greater by one than specified position or wraps to
 * zero if the specified position is edge position.
 *
 * The result is undefined if the table has no entries.
 * @param dyn the pointer to the dynamic table structure
 * @param loc_pos the number of location position
 * @return the position of the next entry for the specified entry position,
 *         undefined if the table has no entries
 */
MHD_FN_PURE_ mhd_static_inline dtbl_idx_t
dtbl_get_pos_next (const struct mhd_HpackDTblContext *dyn,
                   dtbl_idx_ft loc_pos)
{
  mhd_assert (! dtbl_is_empty (dyn));
  mhd_assert (loc_pos <= dtbl_get_pos_edge (dyn));
#ifdef MHD_USE_CODE_HARDENING
  if (dtbl_get_pos_edge (dyn) == loc_pos)
    return 0u;
  return (dtbl_idx_t) (loc_pos + 1u);
#else /* ! MHD_USE_CODE_HARDENING */
  return (dtbl_idx_t) ((dyn->num_entries + loc_pos + 1u) % dyn->num_entries);
#endif /* ! MHD_USE_CODE_HARDENING */
}


/**
 * Get the position of the newest entry.
 *
 * This is a position of the last added entry.
 *
 * The result is undefined if the table has no entries.
 * @param dyn the pointer to the dynamic table structure
 * @return the position of the newest entry,
 *         undefined if the table has no entries
 */
MHD_FN_PURE_ mhd_static_inline dtbl_idx_t
dtbl_get_pos_newest (const struct mhd_HpackDTblContext *dyn)
{
  mhd_assert (! dtbl_is_empty (dyn));
  return dyn->newest_pos;
}


/**
 * Get the position of the oldest entry.
 *
 * This is a position of the current oldest entry in the table. This entry
 * is evicted first if eviction is needed.
 *
 * The result is undefined if the table has no entries.
 * @param dyn the pointer to the dynamic table structure
 * @return the position of the oldest entry,
 *         undefined if the table has no entries
 */
MHD_FN_PURE_ mhd_static_inline dtbl_idx_t
dtbl_get_pos_oldest (const struct mhd_HpackDTblContext *dyn)
{
  return dtbl_get_pos_next (dyn,
                            dtbl_get_pos_newest (dyn));
}


/**
 * Convert an HPACK table index to the position number in the dynamic table.
 *
 * The result is undefined if the specified index is less than or equal to the
 * number of entries in the static table.
 * The result is undefined if the specified index is larger than the last valid
 * HPACK index in the table.
 * @param dyn the pointer to the dynamic table structure
 * @param hpack_idx the HPACK index of the entry
 * @return the position of the requested entry in the table,
 *         undefined if the @a hpack_idx is not valid for the table
 */
MHD_FN_PURE_ mhd_static_inline dtbl_idx_t
dtbl_get_pos_from_hpack_idx (const struct mhd_HpackDTblContext *dyn,
                             dtbl_idx_ft hpack_idx)
{
  dtbl_idx_ft pos_back_from_newest =
    (dtbl_idx_ft) (hpack_idx - mhd_dtbl_hpack_idx_offset);
  mhd_assert (mhd_DTBL_VALUE_FITS (hpack_idx));
  mhd_assert (mhd_HPACK_STBL_LAST_IDX < hpack_idx);
  mhd_assert (dtbl_get_num_entries (dyn) + mhd_dtbl_hpack_idx_offset > \
              hpack_idx);

#ifdef MHD_USE_CODE_HARDENING
  if (dtbl_get_pos_newest (dyn) >= pos_back_from_newest)
    return (dtbl_idx_t) (dtbl_get_pos_newest (dyn) - pos_back_from_newest);
  return (dtbl_idx_t) (dtbl_get_num_entries (dyn) + dtbl_get_pos_newest (dyn)
                       - pos_back_from_newest);
#else  /* ! MHD_USE_CODE_HARDENING */
  return
    (dtbl_idx_t)
    ((dtbl_get_num_entries (dyn)
      + dtbl_get_pos_newest (dyn) - pos_back_from_newest)
     % dtbl_get_num_entries (dyn));
#endif /* ! MHD_USE_CODE_HARDENING */
}


/**
 * Convert a dynamic-table location position to the corresponding HPACK index.
 *
 * This is the inverse of #dtbl_get_pos_from_hpack_idx().
 * The returned HPACK index is strictly greater than the last index in the
 * static table (#mhd_HPACK_STBL_LAST_IDX).
 *
 * Behaviour is undefined if @a loc_pos is not a valid position for @a dyn.
 * @param dyn the pointer to the dynamic table structure
 * @param loc_pos the location position number (0 .. #dtbl_get_pos_edge())
 * @return the HPACK index corresponding to @a loc_pos
 *         undefined if the @a loc_pos is not valid for the table
 */
MHD_FN_PURE_ mhd_static_inline dtbl_idx_t
dtbl_get_hpack_idx_from_pos (const struct mhd_HpackDTblContext *dyn,
                             dtbl_idx_ft loc_pos)
{
  mhd_assert (mhd_DTBL_VALUE_FITS (loc_pos));
  mhd_assert (dtbl_get_pos_edge (dyn) >= loc_pos);

#ifdef MHD_USE_CODE_HARDENING
  if (dtbl_get_pos_newest (dyn) >= loc_pos)
    return (dtbl_idx_t) (dtbl_get_pos_newest (dyn) - loc_pos
                         + mhd_dtbl_hpack_idx_offset);
  return (dtbl_idx_t) (dtbl_get_num_entries (dyn) + dtbl_get_pos_newest (dyn)
                       - loc_pos + mhd_dtbl_hpack_idx_offset);
#else  /* ! MHD_USE_CODE_HARDENING */
  return
    (dtbl_idx_t)
    (((dtbl_get_num_entries (dyn) + dtbl_get_pos_newest (dyn) - loc_pos))
     % dtbl_get_num_entries (dyn) + mhd_dtbl_hpack_idx_offset);
#endif /* ! MHD_USE_CODE_HARDENING */
}


/**
 * Get the current exclusive upper bound (in bytes) for valid offsets
 * within the strings region.

 * This equals the distance from the start of the strings region to the first
 * byte occupied by entry-information data. As more entry information is added,
 * this limit decreases. For an empty table, the limit equals buf_alloc_size.
 * @param dyn the pointer to the dynamic table structure
 * @return the current exclusive upper bound for offsets in the strings region
 */
MHD_FN_PURE_ mhd_static_inline dtbl_size_t
dtbl_get_strs_ceiling (const struct mhd_HpackDTblContext *dyn)
{
  dtbl_size_ft ceiling =
    dyn->buf_alloc_size
    - (dtbl_size_ft) mhd_DTBL_ENTRY_INFO_SIZE * dyn->num_entries;

  mhd_assert (dyn->buf_alloc_size >=
              mhd_DTBL_ENTRY_INFO_SIZE * dyn->num_entries);
  mhd_assert (mhd_DTBL_VALUE_FITS (ceiling));

  return (dtbl_size_t) ceiling;
}


/**
 * Get the formal maximum HPACK size in the table.
 * @param dyn the pointer to the dynamic table structure
 * @return the formal HPACK size in the table
 */
MHD_FN_PURE_ mhd_static_inline dtbl_size_t
dtbl_get_size_max_formal (const struct mhd_HpackDTblContext *dyn)
{
  return dyn->size_limit;
}


/**
 * Get the amount of formal HPACK free space in the table.
 * @param dyn the pointer to the dynamic table structure
 * @return the formal HPACK free space in the table
 */
MHD_FN_PURE_ mhd_static_inline dtbl_size_t
dtbl_get_free_formal (const struct mhd_HpackDTblContext *dyn)
{
  mhd_assert (dyn->size_limit >= dyn->cur_size);
  return dyn->size_limit - dyn->cur_size;
}


/**
 * Get the amount of formal HPACK used space in the table.
 * @param dyn the pointer to the dynamic table structure
 * @return the formal HPACK used space in the table
 */
MHD_FN_PURE_ mhd_static_inline dtbl_size_t
dtbl_get_used_formal (const struct mhd_HpackDTblContext *dyn)
{
  mhd_assert (dyn->size_limit >= dyn->cur_size);
  return dyn->cur_size;
}


/* ** Location of entry information data based on entry position in the
      table ** */

/**
 * Get the pointer to the dynamic table entry by location position number.
 * This is not the same as HPACK index.
 * The result is undefined if the table has no entries.
 * @param dyn the pointer to the dynamic table structure
 * @param loc_pos the number of location position
 * @return the pointer to the dynamic table entry,
 *         undefined if the table has no entries
 */
MHD_FN_PURE_ mhd_static_inline struct mhd_HpackDTblEntryInfo *
dtbl_pos_entry_info (struct mhd_HpackDTblContext *dyn,
                     dtbl_idx_ft loc_pos)
{
  mhd_assert (mhd_DTBL_VALUE_FITS (loc_pos));
  mhd_assert (! dtbl_is_empty (dyn));
  mhd_assert (dyn->num_entries > loc_pos);
  mhd_assert (dyn->buf_alloc_size >=
              mhd_DTBL_ENTRY_INFO_SIZE * dyn->num_entries);
  return dtbl_get_infos (dyn) - loc_pos;
}


/**
 * Get a const pointer to the dynamic table entry by location position number.
 * This is not the same as HPACK index.
 * The result is undefined if the table has no entries.
 * @param dyn const pointer to the dynamic table structure
 * @param loc_pos the number of location position
 * @return the pointer to the dynamic table entry,
 *         undefined if the table has no entries
 */
MHD_FN_PURE_ mhd_static_inline const struct mhd_HpackDTblEntryInfo *
dtbl_pos_entry_infoc (const struct mhd_HpackDTblContext *dyn,
                      dtbl_idx_ft loc_pos)
{
  mhd_assert (mhd_DTBL_VALUE_FITS (loc_pos));
  mhd_assert (! dtbl_is_empty (dyn));
  mhd_assert (dyn->num_entries > loc_pos);
  mhd_assert (dyn->buf_alloc_size >=
              mhd_DTBL_ENTRY_INFO_SIZE * dyn->num_entries);
  return dtbl_get_infosc (dyn) - loc_pos;
}


/**
 * Get the pointer to the zero location entry information data.
 * This is the highest address of the entries data location in the table.
 * The result is undefined if the table has no entries.
 * @param dyn the pointer to the dynamic table structure
 * @return the pointer to the zero location entry info data,
 *         undefined if the table has no entries
 */
MHD_FN_PURE_ mhd_static_inline struct mhd_HpackDTblEntryInfo *
dtbl_zero_entry_info (struct mhd_HpackDTblContext *dyn)
{
  return dtbl_pos_entry_info (dyn,
                              0u);
}


/**
 * Get a const pointer to the zero location entry information data.
 * This is the highest address of the entries data location in the table.
 * The result is undefined if the table has no entries.
 * @param dyn the pointer to the dynamic table structure
 * @return const pointer to the zero location entry information data,
 *         undefined if the table has no entries
 */
MHD_FN_PURE_ mhd_static_inline const struct mhd_HpackDTblEntryInfo *
dtbl_zero_entry_infoc (const struct mhd_HpackDTblContext *dyn)
{
  return dtbl_pos_entry_infoc (dyn,
                               0u);
}


/**
 * Get the pointer to the table's edge entry information data.
 * This is the lowest address of the entries data location in the table.
 * The result is undefined if the table has no entries.
 * @param dyn the pointer to the dynamic table structure
 * @return the pointer to the table's edge entry information data,
 *         undefined if the table has no entries
 */
MHD_FN_PURE_ mhd_static_inline struct mhd_HpackDTblEntryInfo *
dtbl_edge_entry_info (struct mhd_HpackDTblContext *dyn)
{
  struct mhd_HpackDTblEntryInfo *const ptr =
    dtbl_pos_entry_info (dyn,
                         dtbl_get_pos_edge (dyn));
  mhd_assert (((const void *) ptr) == \
              ((const void *) (dtbl_get_strs_buffc (dyn)
                               + dtbl_get_strs_ceiling (dyn))));
  return ptr;
}


/**
 * Get a const pointer to the table edge entry information data.
 * This is the lowest address of the entries data location in the table.
 * The result is undefined if the table has no entries.
 * @param dyn the pointer to the dynamic table structure
 * @return const pointer to the table edge entry information data,
 *         undefined if the table has no entries
 */
MHD_FN_PURE_ mhd_static_inline const struct mhd_HpackDTblEntryInfo *
dtbl_edge_entry_infoc (const struct mhd_HpackDTblContext *dyn)
{
  const struct mhd_HpackDTblEntryInfo *const ptr =
    dtbl_pos_entry_infoc (dyn,
                          dtbl_get_pos_edge (dyn));
  mhd_assert (((const void *) ptr) == \
              ((const void *) (dtbl_get_strs_buffc (dyn)
                               + dtbl_get_strs_ceiling (dyn))));
  return ptr;
}


/**
 * Get the pointer to the newest entry information data.
 * The result is undefined if the table has no entries.
 * @param dyn the pointer to the dynamic table structure
 * @return the pointer to the newest entry information data,
 *         undefined if the table has no entries
 */
MHD_FN_PURE_ mhd_static_inline struct mhd_HpackDTblEntryInfo *
dtbl_newest_entry_info (struct mhd_HpackDTblContext *dyn)
{
  return dtbl_pos_entry_info (dyn,
                              dtbl_get_pos_newest (dyn));
}


/**
 * Get a const pointer to the newest entry information data.
 * The result is undefined if the table has no entries.
 * @param dyn const pointer to the dynamic table structure
 * @return const pointer to the newest entry information data,
 *         undefined if the table has no entries
 */
MHD_FN_PURE_ mhd_static_inline const struct mhd_HpackDTblEntryInfo *
dtbl_newest_entry_infoc (const struct mhd_HpackDTblContext *dyn)
{
  return dtbl_pos_entry_infoc (dyn,
                               dtbl_get_pos_newest (dyn));
}


/**
 * Get the pointer to the oldest entry information data.
 * The result is undefined if the table has no entries.
 * @param dyn the pointer to the dynamic table structure
 * @return the pointer to the oldest entry information data,
 *         undefined if the table has no entries
 */
MHD_FN_PURE_ mhd_static_inline struct mhd_HpackDTblEntryInfo *
dtbl_oldest_entry_info (struct mhd_HpackDTblContext *dyn)
{
  return dtbl_pos_entry_info (dyn,
                              dtbl_get_pos_oldest (dyn));
}


/**
 * Get a const pointer to the oldest entry information data.
 * The result is undefined if the table has no entries.
 * @param dyn const pointer to the dynamic table structure
 * @return const pointer to the oldest entry information data,
 *         undefined if the table has no entries
 */
MHD_FN_PURE_ mhd_static_inline const struct mhd_HpackDTblEntryInfo *
dtbl_oldest_entry_infoc (const struct mhd_HpackDTblContext *dyn)
{
  return dtbl_pos_entry_infoc (dyn,
                               dtbl_get_pos_oldest (dyn));
}


/* ** Entries strings information based on the entry position in the table ** */

/**
 * Get the total size of the strings of the entry.
 * This is the minimal size required for the entry in the strings buffer.
 * @param dyn the pointer to the dynamic table structure
 * @param loc_pos the number of location position
 * @return the total size of the strings of the entry
 */
MHD_FN_PURE_ mhd_static_inline dtbl_size_t
dtbl_pos_strs_size_min (const struct mhd_HpackDTblContext *dyn,
                        dtbl_idx_ft loc_pos)
{
  return dtbl_entr_strs_size_min (dtbl_pos_entry_infoc (dyn,
                                                        loc_pos));
}


/**
 * Get the total size of the strings of the entry plus standard slack size.
 * This is the optimal size used for the entry in the strings buffer when the
 * current insertion slot has enough space.
 * @param dyn the pointer to the dynamic table structure
 * @param loc_pos the number of location position
 * @return the total size of the strings of the entry plus standard slack size
 */
MHD_FN_PURE_ mhd_static_inline dtbl_size_t
dtbl_pos_strs_size_optm (const struct mhd_HpackDTblContext *dyn,
                         dtbl_idx_ft loc_pos)
{
  return dtbl_entr_strs_size_optm (dtbl_pos_entry_infoc (dyn,
                                                         loc_pos));
}


/**
 * Get the formal HPACK size of the entry.
 * The formal size of the entry is the size of the strings plus fixed
 * HPACK per-entry overhead.
 * @param dyn the pointer to the dynamic table structure
 * @param loc_pos the number of location position
 * @return the formal HPACK size of the entry
 */
MHD_FN_PURE_ mhd_static_inline dtbl_size_t
dtbl_pos_size_formal (const struct mhd_HpackDTblContext *dyn,
                      dtbl_idx_ft loc_pos)
{
  return dtbl_entr_size_formal (dtbl_pos_entry_infoc (dyn,
                                                      loc_pos));
}


/**
 * Get the position (offset) of the (inclusive) start of the entry's strings
 * in the strings buffer.
 * This points to the first byte of the entry's strings. If the entry has
 * zero-length strings, the pointer denotes a (possibly zero-sized) area
 * that may coincide with the start of the entry's slack (if any) or with
 * the next entry's strings start (if present).
 * @param dyn the pointer to the dynamic table structure
 * @param loc_pos the number of location position
 * @return the position (offset) of the (inclusive) start of the entry's strings
 */
MHD_FN_PURE_ mhd_static_inline dtbl_size_t
dtbl_pos_strs_start (const struct mhd_HpackDTblContext *dyn,
                     dtbl_idx_ft loc_pos)
{
  return dtbl_entr_strs_start (dtbl_pos_entry_infoc (dyn,
                                                     loc_pos));
}


/**
 * Get the position of the (exclusive) end of the entry's strings in the
 * strings buffer.
 * This points to the next char (byte) after the strings of the entry.
 * @param dyn the pointer to the dynamic table structure
 * @param loc_pos the number of location position
 * @return the position of the end of the entry's strings in the strings buffer
 */
MHD_FN_PURE_ mhd_static_inline dtbl_size_t
dtbl_pos_strs_end_min (const struct mhd_HpackDTblContext *dyn,
                       dtbl_idx_ft loc_pos)
{
  return dtbl_entr_strs_end_min (dtbl_pos_entry_infoc (dyn,
                                                       loc_pos));
}


/**
 * Get the position after standard slack after the end of the entry's strings
 * in the strings buffer.
 * This points to the preferred position of the next entry's strings.
 * @param dyn the pointer to the dynamic table structure
 * @param loc_pos the number of location position
 * @return the position of the end of the entry's strings in the strings buffer
 */
MHD_FN_PURE_ mhd_static_inline dtbl_size_t
dtbl_pos_strs_end_optm (const struct mhd_HpackDTblContext *dyn,
                        dtbl_idx_ft loc_pos)
{
  return dtbl_entr_strs_end_optm (dtbl_pos_entry_infoc (dyn,
                                                        loc_pos));
}


/* ** Entries strings location information based on the pointer to the
      entry ** */

/**
 * Get a pointer to the (inclusive) start of the entry's strings in the
 * strings buffer.
 * This points to the first byte of the entry's strings. If the entry has
 * zero-length strings, the pointer denotes a (possibly zero-sized) area
 * that may coincide with the start of the entry's slack (if any) or with
 * the next entry's strings start (if present).
 * The result is undefined if the entry is not in the table.
 * @param dyn the pointer to the dynamic table structure
 * @param entr_inf the pointer to the entry information
 * @return the pointer of the (inclusive) start of the entry's strings,
 *         result is undefined if the entry is not in the table
 */
MHD_FN_PURE_ mhd_static_inline char *
dtbl_entr_strs_ptr_start (struct mhd_HpackDTblContext *dyn,
                          const struct mhd_HpackDTblEntryInfo *entr_inf)
{
  mhd_assert (dtbl_zero_entry_infoc (dyn) >= entr_inf);
  mhd_assert (dtbl_edge_entry_infoc (dyn) <= entr_inf);
  return dtbl_get_strs_buff (dyn) + dtbl_entr_strs_start (entr_inf);
}


/**
 * Get const pointer to the (inclusive) start of the entry's strings in the
 * strings buffer.
 * This points to the first byte of the entry's strings. If the entry has
 * zero-length strings, the pointer denotes a (possibly zero-sized) area
 * that may coincide with the start of the entry's slack (if any) or with
 * the next entry's strings start (if present).
 * The result is undefined if the entry is not in the table.
 * @param dyn the pointer to the dynamic table structure
 * @param entr_inf the pointer to the entry information
 * @return const pointer of the (inclusive) start of the entry's strings,
 *         result is undefined if the entry is not in the table
 */
MHD_FN_PURE_ mhd_static_inline const char *
dtbl_entr_strs_ptr_startc (const struct mhd_HpackDTblContext *dyn,
                           const struct mhd_HpackDTblEntryInfo *entr_inf)
{
  mhd_assert (dtbl_zero_entry_infoc (dyn) >= entr_inf);
  mhd_assert (dtbl_edge_entry_infoc (dyn) <= entr_inf);
  return dtbl_get_strs_buffc (dyn) + dtbl_entr_strs_start (entr_inf);
}


/**
 * Get a pointer to the (exclusive) end of the entry's strings in the
 * strings buffer.
 * This points to the next char (byte) after the strings of the entry.
 * The result is undefined if the entry is not in the table.
 * @param dyn the pointer to the dynamic table structure
 * @param entr_inf the pointer to the entry information
 * @return the pointer to the (exclusive) end of the entry's strings,
 *         result is undefined if the entry is not in the table
 */
MHD_FN_PURE_ mhd_static_inline char *
dtbl_entr_strs_ptr_end (struct mhd_HpackDTblContext *dyn,
                        const struct mhd_HpackDTblEntryInfo *entr_inf)
{
  mhd_assert (dtbl_zero_entry_infoc (dyn) >= entr_inf);
  mhd_assert (dtbl_edge_entry_infoc (dyn) <= entr_inf);
  return dtbl_get_strs_buff (dyn) + dtbl_entr_strs_end_min (entr_inf);
}


/**
 * Get a const pointer to the (exclusive) end of the entry's strings in the
 * strings buffer.
 * This points to the next char (byte) after the strings of the entry.
 * The result is undefined if the entry is not in the table.
 * @param dyn const pointer to the dynamic table structure
 * @param entr_inf const pointer to the entry information
 * @return const pointer to the (exclusive) end of the entry's strings,
 *         result is undefined if the entry is not in the table
 */
MHD_FN_PURE_ mhd_static_inline const char *
dtbl_entr_strs_ptr_endc (const struct mhd_HpackDTblContext *dyn,
                         const struct mhd_HpackDTblEntryInfo *entr_inf)
{
  mhd_assert (dtbl_zero_entry_infoc (dyn) >= entr_inf);
  mhd_assert (dtbl_edge_entry_infoc (dyn) <= entr_inf);
  return dtbl_get_strs_buffc (dyn) + dtbl_entr_strs_end_min (entr_inf);
}


/**
 * Get a const pointer to the (exclusive) end of the entry's standard slack
 * after the entry's strings in the strings buffer.
 * This points to the preferred location of the next entry's strings.
 * The result is undefined if the entry is not in the table.
 * @param dyn const pointer to the dynamic table structure
 * @param entr_inf const pointer to the entry information
 * @return const pointer to the (exclusive) end of the entry's standard slack,
 *         result is undefined if the entry is not in the table
 */
MHD_FN_PURE_ mhd_static_inline const char *
dtbl_entr_strs_ptr_end_slackc (const struct mhd_HpackDTblContext *dyn,
                               const struct mhd_HpackDTblEntryInfo *entr_inf)
{
  mhd_assert (dtbl_zero_entry_infoc (dyn) >= entr_inf);
  mhd_assert (dtbl_edge_entry_infoc (dyn) <= entr_inf);
  return dtbl_get_strs_buffc (dyn) + dtbl_entr_strs_end_optm (entr_inf);
}


/**
 * Get const pointer to the entry's name.
 * This points to the first byte of the entry's name. If the entry has
 * zero-length name, the pointer denotes a zero-sized area.
 * The result is undefined if the entry is not in the table.
 * @param dyn the pointer to the dynamic table structure
 * @param entr_inf the pointer to the entry information
 * @return const pointer to the entry's name,
 *         result is undefined if the entry is not in the table
 */
MHD_FN_PURE_ mhd_static_inline const char *
dtbl_entr_strs_ptr_namec (const struct mhd_HpackDTblContext *dyn,
                          const struct mhd_HpackDTblEntryInfo *entr_inf)
{
  return dtbl_entr_strs_ptr_startc (dyn,
                                    entr_inf);
}


/**
 * Get const pointer to the entry's value.
 * This points to the first byte of the entry's value. If the entry has
 * zero-length value, the pointer denotes a zero-sized area.
 * The result is undefined if the entry is not in the table.
 * @param dyn the pointer to the dynamic table structure
 * @param entr_inf the pointer to the entry information
 * @return const pointer to the entry's value,
 *         result is undefined if the entry is not in the table
 */
MHD_FN_PURE_ mhd_static_inline const char *
dtbl_entr_strs_ptr_valuec (const struct mhd_HpackDTblContext *dyn,
                           const struct mhd_HpackDTblEntryInfo *entr_inf)
{
  return dtbl_entr_strs_ptr_startc (dyn,
                                    entr_inf) + entr_inf->name_len;
}


/* ** Information about the entry in the table based on the pointer to
      the entry ** */

/**
 * Get the size of the space between entry's strings and entry information data
 * as if the provided entry were an edge entry.
 * The gap could be zero in some conditions.
 * The result is undefined if the entry is not in the table.
 * @param dyn const pointer to the dynamic table structure
 * @param entr_inf const pointer to the entry information
 * @return the size of the space between entry's strings and information,
 *         result is undefined if the entry is not in the table
 */
MHD_FN_PURE_ mhd_static_inline dtbl_size_t
dtbl_entr_as_edge_get_gap (const struct mhd_HpackDTblContext *dyn,
                           const struct mhd_HpackDTblEntryInfo *entr_inf)
{
  const char *upper_ptr = (const char *) entr_inf;
  const char *lower_ptr = dtbl_entr_strs_ptr_endc (dyn, entr_inf);
  const dtbl_size_ft gap = (dtbl_size_ft) (upper_ptr - lower_ptr);

  mhd_assert (dtbl_zero_entry_infoc (dyn) >= entr_inf);
  mhd_assert (dtbl_edge_entry_infoc (dyn) <= entr_inf);
  mhd_assert (lower_ptr <= upper_ptr);
  mhd_assert (mhd_DTBL_VALUE_FITS (gap));
  mhd_assert (gap < dyn->buf_alloc_size);

  return (dtbl_size_t) gap;
}


/* ** Entries strings location information based on entry position in
      the table ** */

/**
 * Get a pointer to the (inclusive) start of the entry's strings in the
 * strings buffer.
 * This points to the first char (byte) of the entry's strings. If the entry
 * has zero-length strings then this points to the first byte of entry slack
 * (if any) or the first char of the next entry's strings (if any).
 * The result is undefined if the location number is equal to or greater than
 * the number of entries in the table.
 * @param dyn the pointer to the dynamic table structure
 * @param loc_pos the number of location position
 * @return the pointer to the (inclusive) start of the entry's strings
 */
MHD_FN_PURE_ mhd_static_inline char *
dtbl_pos_strs_ptr_start (struct mhd_HpackDTblContext *dyn,
                         dtbl_idx_ft loc_pos)
{
  return dtbl_entr_strs_ptr_start (dyn,
                                   dtbl_pos_entry_info (dyn,
                                                        loc_pos));
}


/**
 * Get a const pointer to the (inclusive) start of the entry's strings in the
 * strings buffer.
 * This points to the first char (byte) of the entry's strings. If the entry
 * has zero-length strings then this points to the first byte of entry slack
 * (if any) or the first char of the next entry's strings (if any).
 * The result is undefined if the location number is equal to or greater than
 * the number of entries in the table.
 * @param dyn const pointer to the dynamic table structure
 * @param loc_pos the number of location position
 * @return const pointer to the (inclusive) start of the entry's strings,
 *         result is undefined if the entry is not in the table
 */
MHD_FN_PURE_ mhd_static_inline const char *
dtbl_pos_strs_ptr_startc (const struct mhd_HpackDTblContext *dyn,
                          dtbl_idx_ft loc_pos)
{
  return dtbl_entr_strs_ptr_startc (dyn,
                                    dtbl_pos_entry_infoc (dyn,
                                                          loc_pos));
}


/**
 * Get a pointer to the (exclusive) end of the entry's strings in the
 * strings buffer.
 * This points to the next char (byte) after the strings of the entry.
 * The result is undefined if the location number is equal or greater than the
 * number of entries in the table.
 * @param dyn the pointer to the dynamic table structure
 * @param loc_pos the number of location position
 * @return the pointer to the (exclusive) end of the entry's strings
 */
MHD_FN_PURE_ mhd_static_inline char *
dtbl_pos_strs_ptr_end (struct mhd_HpackDTblContext *dyn,
                       dtbl_idx_ft loc_pos)
{
  return dtbl_entr_strs_ptr_end (dyn,
                                 dtbl_pos_entry_info (dyn,
                                                      loc_pos));
}


/**
 * Get a const pointer to the (exclusive) end of the entry's strings in the
 * strings buffer.
 * This points to the next char (byte) after the strings of the entry.
 * The result is undefined if the location number is equal or greater than the
 * number of entries in the table.
 * @param dyn const pointer to the dynamic table structure
 * @param loc_pos the number of location position
 * @return const pointer to the (exclusive) end of the entry's strings
 */
MHD_FN_PURE_ mhd_static_inline const char *
dtbl_pos_strs_ptr_endc (const struct mhd_HpackDTblContext *dyn,
                        dtbl_idx_ft loc_pos)
{
  return dtbl_entr_strs_ptr_endc (dyn,
                                  dtbl_pos_entry_infoc (dyn,
                                                        loc_pos));
}


/**
 * Get a const pointer to the (exclusive) end of the entry's standard slack
 * after the entry's strings in the strings buffer.
 * This points to the preferred location of the next entry's strings.
 * The result is undefined if the location number is equal or greater than the
 * number of entries in the table.
 * @param dyn const pointer to the dynamic table structure
 * @param loc_pos the number of location position
 * @return const pointer to the (exclusive) end of the entry's standard slack
 */
MHD_FN_PURE_ mhd_static_inline const char *
dtbl_pos_strs_ptr_end_slackc (const struct mhd_HpackDTblContext *dyn,
                              dtbl_idx_ft loc_pos)
{
  return dtbl_entr_strs_ptr_end_slackc (dyn,
                                        dtbl_pos_entry_infoc (dyn,
                                                              loc_pos));
}


/* ** Information about the entry in the table based on entry position in
      the table ** */

/**
 * Get the size of the space between entry's strings and entry information data
 * as if the provided entry were an edge entry.
 * The gap could be zero in some conditions.
 * The result is undefined if the location number is equal or greater than the
 * number of entries in the table.
 * @param dyn const pointer to the dynamic table structure
 * @param loc_pos the number of location position
 * @return the size of the space between entry's strings and information
 */
MHD_FN_PURE_ mhd_static_inline dtbl_size_t
dtbl_pos_as_edge_get_gap (const struct mhd_HpackDTblContext *dyn,
                          dtbl_idx_ft loc_pos)
{
  return dtbl_entr_as_edge_get_gap (dyn,
                                    dtbl_pos_entry_infoc (dyn,
                                                          loc_pos));
}


/* ** Additional means of access to the entries information ** */

/**
 * Get table's entries information as a pointer to an array.
 *
 * The returned array has #dtbl_get_num_entries() elements.
 * The returned pointer becomes invalid if any entry is added or evicted
 * from the table.
 *
 * Behaviour is undefined if table is empty.
 * @param dyn the pointer to the dynamic table structure
 * @return table's entries information as a pointer to an array
 */
MHD_FN_PURE_ mhd_static_inline struct mhd_HpackDTblEntryInfo *
dtbl_get_infos_as_array (struct mhd_HpackDTblContext *dyn)
{
  return dtbl_edge_entry_info (dyn);
}


/**
 * Get table's entries information as a pointer to a const array.
 *
 * The returned array has #dtbl_get_num_entries() elements.
 *
 * The first (zero index) item in the array is the edge entry, the last item
 * in the array is zero position entry.
 *
 * The returned pointer becomes invalid if any entry is added or evicted
 * from the table.
 *
 * Behaviour is undefined if table is empty.
 * @param dyn the pointer to the dynamic table structure
 * @return table's entries information as a pointer to a const array
 */
MHD_FN_PURE_ mhd_static_inline const struct mhd_HpackDTblEntryInfo *
dtbl_get_infos_as_arrayc (const struct mhd_HpackDTblContext *dyn)
{
  return dtbl_edge_entry_infoc (dyn);
}


/* ** Additional information about the table ** */

/**
 * Get the size of the free space available for new entries (including
 * entry's strings, entry info data, and per-entry slack) between the
 * string region and the entry-info region in the shared buffer.
 *
 * The gap could be zero in some conditions.

 * Unlike #dtbl_bottom_gap(), this space is used for both strings data and
 * entries info data when adding new entries.
 *
 * This is not the formal HPACK free size.
 * @param dyn const pointer to the dynamic table structure
 * @return the size of the space available at the edge of the table
 */
MHD_FN_PURE_ mhd_static_inline dtbl_size_t
dtbl_edge_gap (const struct mhd_HpackDTblContext *dyn)
{
  if (dtbl_is_empty (dyn))
    return dyn->buf_alloc_size;

  return dtbl_entr_as_edge_get_gap (dyn,
                                    dtbl_edge_entry_infoc (dyn));
}


/**
 * Get the size of the free space available for strings at the bottom of
 * the shared buffer.
 *
 * Unlike #dtbl_edge_gap(), if table is not empty, this space can be used only
 * for the strings data for an entry added at zero position.
 *
 * @param dyn const pointer to the dynamic table structure
 * @return the size of the space available at the bottom of the table
 */
MHD_FN_PURE_ mhd_static_inline dtbl_size_t
dtbl_bottom_gap (const struct mhd_HpackDTblContext *dyn)
{
  if (dtbl_is_empty (dyn))
    return dyn->buf_alloc_size;

  return dtbl_pos_strs_start (dyn, 0u);
}


/* ** Manipulating strings in the dynamic table ** */

/**
 * Choose the offset of the strings in the strings buffer for a new entry
 * following another entry (non-zero position).
 *
 * If enough space is available, up to the standard slack bytes are left
 * between entries' strings.
 *
 * Result is undefined if @a size_of_space is less than @a entry_strs_size.
 * @param space_start the offset of the start (inclusive) of free space in
 *                    the buffer
 * @param size_of_space the amount of free space at the @a space_start offset
 * @param entry_strs_size the size of new entries strings
 * @return the offset to put entries strings in the buffer
 */
MHD_FN_CONST_ mhd_static_inline dtbl_size_t
dtbl_choose_strs_offset_for_size (dtbl_size_ft space_start,
                                  dtbl_size_ft size_of_space,
                                  dtbl_size_ft entry_strs_size)
{
  const dtbl_size_ft extra_space = size_of_space - entry_strs_size;

  mhd_assert (size_of_space >= entry_strs_size);
  mhd_assert (mhd_DTBL_VALUE_FITS (space_start));
  mhd_assert (mhd_DTBL_VALUE_FITS (size_of_space));
  mhd_assert (mhd_DTBL_VALUE_FITS (entry_strs_size));

  if (mhd_dtbl_entry_slack <= extra_space)
    return (dtbl_size_t) (space_start + mhd_dtbl_entry_slack);

  return (dtbl_size_t) (space_start + extra_space);
}


/**
 * Completely reset dynamic table data.
 * This fully removes all entries from the table, leaving the size of the table
 * and the table allocation the same.
 * @param dyn the pointer to the dynamic table structure
 */
mhd_static_inline void
dtbl_reset (struct mhd_HpackDTblContext *dyn)
{
  dyn->num_entries = 0u;
  dyn->newest_pos = 0u;
  dyn->cur_size = 0u;
}


/**
 * Move selected entries' strings in the strings buffer down (to the start of
 * the buffer).
 * The strings are moved for all entries from @a from_pos up to the
 * edge (highest number) entry.
 * @param dyn the pointer to the dynamic table structure
 * @param from_pos the first entry position to move strings
 * @param shift_down_size the amount of bytes to shift
 */
static void
dtbl_move_strs_down (struct mhd_HpackDTblContext *dyn,
                     dtbl_idx_ft from_pos,
                     dtbl_size_ft shift_down_size)
{
  char *move_area_src = dtbl_pos_strs_ptr_start (dyn,
                                                 from_pos);
  size_t move_area_size =
    (size_t)
    (dtbl_pos_strs_ptr_endc (dyn,
                             dtbl_get_pos_edge (dyn)) - move_area_src);
  dtbl_idx_ft i;

  mhd_assert (mhd_DTBL_VALUE_FITS (from_pos));
  mhd_assert (mhd_DTBL_VALUE_FITS (shift_down_size));
  mhd_assert (0u != shift_down_size);
  mhd_assert (dtbl_get_pos_edge (dyn) >= from_pos);
  mhd_assert (shift_down_size <= dtbl_pos_strs_start (dyn, from_pos));
  mhd_assert ((0u == from_pos) || \
              (dtbl_pos_strs_end_min (dyn, from_pos - 1u) <= \
               dtbl_pos_strs_start (dyn, from_pos) - shift_down_size));
  mhd_assert ((0u != from_pos) || \
              (dtbl_bottom_gap (dyn) >= shift_down_size));
  mhd_assert (dtbl_edge_gap (dyn) < dyn->buf_alloc_size);
  mhd_assert (dyn->buf_alloc_size > move_area_size);

  /* Optimisation ideas: instead of shifting all entries uniformly, they
   * can be "compressed" by eliminating the slack between some of the top
   * entries. This will require more processing, more movements on the next
   * rounds, but saves a lot if the dynamic table is large. */

  /* Move all strings in the buffer for selected entries */
  memmove (move_area_src - shift_down_size,
           move_area_src,
           move_area_size);

#ifndef NDEBUG
  /* Zero-out standard string slack of the last entry strings */
  if (mhd_dtbl_entry_slack <= shift_down_size)
    memset (move_area_src - shift_down_size + move_area_size,
            0,
            mhd_dtbl_entry_slack);
  else
    memset (move_area_src - shift_down_size + move_area_size,
            0,
            shift_down_size);
#endif /* ! NDEBUG */

  for (i = from_pos; dtbl_get_pos_edge (dyn) >= i; ++i)
    dtbl_pos_entry_info (dyn,
                         i)->offset -= (dtbl_size_t) shift_down_size;
}


/**
 * Move selected entries' strings in the strings buffer up (to the entries
 * information data).
 * The strings are moved for all entries from @a from_entry up to the
 * edge (highest number) entry.
 * @param dyn the pointer to the dynamic table structure
 * @param from_pos the first entry position to move strings
 * @param shift_up_size the amount of bytes to shift
 */
static void
dtbl_move_strs_up (struct mhd_HpackDTblContext *dyn,
                   dtbl_idx_ft from_pos,
                   dtbl_size_ft shift_up_size)
{
  char *move_area_src = dtbl_pos_strs_ptr_start (dyn,
                                                 from_pos);
  size_t move_area_size =
    (size_t)
    (dtbl_pos_strs_ptr_endc (dyn,
                             dtbl_get_pos_edge (dyn)) - move_area_src);
  dtbl_idx_ft i;

  mhd_assert (mhd_DTBL_VALUE_FITS (shift_up_size));
  mhd_assert (0u != shift_up_size);
  mhd_assert (dtbl_get_pos_edge (dyn) >= from_pos);
  mhd_assert (shift_up_size < (dyn->buf_alloc_size));
  mhd_assert (dtbl_edge_gap (dyn) >= shift_up_size);
  mhd_assert (dyn->buf_alloc_size > move_area_size);

  /* Optimisation ideas: instead of shifting all entries uniformly, they
   * can be "compacted" by eliminating the slack between some of the bottom
   * entries. This will require more processing and probably more movements on
   * the next rounds, but saves a lot if the dynamic table is large. */

#ifndef NDEBUG
  /* Zero-out standard string slack of the last entry strings AFTER the moved
     data if space is available */
  if (1)
  {
    const dtbl_size_ft top_gap_final = dtbl_edge_gap (dyn) - shift_up_size;

    if (mhd_dtbl_entry_slack <= top_gap_final)
      memset (move_area_src + shift_up_size + move_area_size,
              0,
              mhd_dtbl_entry_slack);
    else if (0u != top_gap_final)
      memset (move_area_src + shift_up_size + move_area_size,
              0,
              top_gap_final);
  }
#endif /* ! NDEBUG */

  /* Move all strings in the buffer for selected entries */
  memmove (move_area_src + shift_up_size,
           move_area_src,
           move_area_size);

  for (i = from_pos; dtbl_get_pos_edge (dyn) >= i; ++i)
    dtbl_pos_entry_info (dyn,
                         i)->offset += (dtbl_size_t) shift_up_size;
}


/**
 * Compact strings in the shared buffer so that all currently unused space
 * is located at the edge (between the strings region and entry information
 * data region).
 *
 * If the newest entry is not the edge entry, the function removes any extra
 * gap between the newest and the oldest entries, keeping only the standard
 * slack. Otherwise, the extra gap at the bottom of the buffer is eliminated.
 *
 * The function does not change the number of entries or their formal sizes.
 * Behaviour is undefined if table's internal data is not consistent.
 * @param dyn the pointer to the dynamic table structure
 */
static void
dtbl_compact_strs (struct mhd_HpackDTblContext *dyn)
{
  if (dtbl_get_pos_edge (dyn) != dtbl_get_pos_newest (dyn))
  {
    /* Remove extra space between the newest and the oldest,
       leave the standard slack only. */
    const dtbl_size_t strs_start_optimal =
      dtbl_pos_strs_end_optm (dyn,
                              dtbl_get_pos_newest (dyn));
    const dtbl_size_t strs_start_current =
      dtbl_pos_strs_start (dyn,
                           dtbl_get_pos_oldest (dyn));
    if (strs_start_current > strs_start_optimal)
    {
      /* There is an extra slack */
      const dtbl_size_t shift_size = strs_start_current - strs_start_optimal;
      dtbl_move_strs_down (dyn,
                           dtbl_get_pos_oldest (dyn),
                           shift_size);
    }
  }
  else
  {
    /* Remove extra space at the bottom of the strings */
    const dtbl_size_t shift_size = dtbl_pos_strs_start (dyn,
                                                        0u);

    /* If there is an extra space - remove it */
    if (0u != shift_size)
      dtbl_move_strs_down (dyn,
                           0u,
                           shift_size);
  }
  /* All the free space must be at the edge of the buffer.
     The buffer allocation is larger than the formal table size. */
  mhd_assert (dtbl_edge_gap (dyn) > dtbl_get_free_formal (dyn));
}


/**
 * Choose the offset of the strings in the strings buffer for a new entry
 * following another entry (non-zero position).
 *
 * If enough space is available, leave up to the standard slack between
 * entries' strings.
 *
 * Result is undefined if @a space_end is less than @a space_start.
 * Result is undefined if not enough space for @a entry_strs_size is in
 * between @a space_start and @a space_end.
 * @param space_start the offset of the start (inclusive) of free space in
 *                    the buffer
 * @param space_end the offset of the end (exclusive) of free space in
 *                  the buffer
 * @param entry_strs_size the size of new entries strings
 * @return the offset to put entries strings in the buffer
 */
MHD_FN_CONST_ mhd_static_inline dtbl_size_t
dtbl_choose_strs_offset (dtbl_size_ft space_start,
                         dtbl_size_ft space_end,
                         dtbl_size_ft entry_strs_size)
{
  const dtbl_size_ft space_size = space_end - space_start;

  mhd_assert (space_start <= space_end);
  mhd_assert (mhd_DTBL_VALUE_FITS (space_start));
  mhd_assert (mhd_DTBL_VALUE_FITS (space_end));
  mhd_assert (mhd_DTBL_VALUE_FITS (space_size));
  mhd_assert (space_end >= space_size);

  return (dtbl_size_t) dtbl_choose_strs_offset_for_size (space_start,
                                                         space_size,
                                                         entry_strs_size);
}


#ifndef NDEBUG

/**
 * Zero-out end up to mhd_dtbl_entry_slack at the end of the strings of some
 * entry.
 * The input data is a pointer to the end of strings and available space.
 * @param entr_strs_end_ptr the pointer to the end of the strings
 * @param space_available amount of space before next used memory area
 */
mhd_static_inline void
dtbl_zeroout_strs_slack_ptr_space (char *entr_strs_end_ptr,
                                   dtbl_size_ft space_available)
{
  const dtbl_size_ft zero_out_size =
    (mhd_dtbl_entry_slack <= space_available) ?
    mhd_dtbl_entry_slack : space_available;
  mhd_assert (mhd_DTBL_VALUE_FITS (space_available));

  if (0u != space_available)
    memset (entr_strs_end_ptr,
            0,
            zero_out_size);
}


/**
 * Zero-out end up to mhd_dtbl_entry_slack at the end of the strings of some
 * entry.
 * The input data is an offset of the end of strings and available space.
 * @param dyn pointer to the dynamic table structure
 * @param entr_strs_end_offset the offset of the end of the strings
 * @param space_available amount of space before next used memory area
 */
mhd_static_inline void
dtbl_zeroout_strs_slack_offset_space (struct mhd_HpackDTblContext *dyn,
                                      dtbl_size_ft entr_strs_end_offset,
                                      dtbl_size_ft space_available)
{
  mhd_assert (dyn->buf_alloc_size >= entr_strs_end_offset);
  mhd_assert (dyn->buf_alloc_size >= space_available);
  mhd_assert (dyn->buf_alloc_size >= (entr_strs_end_offset + space_available));
  dtbl_zeroout_strs_slack_ptr_space (dtbl_get_strs_buff (dyn)
                                     + entr_strs_end_offset,
                                     space_available);
}


/**
 * Zero-out end up to mhd_dtbl_entry_slack at the end of the strings of some
 * entry.
 * The input data is dynamic table struct, an offset of the end of strings and
 * offset of the next data in the buffer.
 * @param dyn pointer to the dynamic table structure
 * @param entr_strs_end_offset the offset of the end of the strings
 * @param next_data_offset the offset of the next used memory area in the
 *                         buffer
 */
mhd_static_inline void
dtbl_zeroout_strs_slack_offset_next (struct mhd_HpackDTblContext *dyn,
                                     dtbl_size_ft entr_strs_end_offset,
                                     dtbl_size_ft next_data_offset)
{
  mhd_assert (dyn->buf_alloc_size >= entr_strs_end_offset);
  mhd_assert (dyn->buf_alloc_size >= next_data_offset);
  mhd_assert (next_data_offset >= entr_strs_end_offset);
  dtbl_zeroout_strs_slack_offset_space (dyn,
                                        entr_strs_end_offset,
                                        next_data_offset
                                        - entr_strs_end_offset);
}


/**
 * Zero-out end up to mhd_dtbl_entry_slack at the end of the strings of some
 * entry.
 * @param dyn pointer to the dynamic table structure
 * @param entry the pointer to the entry information data
 * @param space_available amount of space before next used memory area
 */
mhd_static_inline void
dtbl_zeroout_strs_slack_entry_space (struct mhd_HpackDTblContext *dyn,
                                     const struct mhd_HpackDTblEntryInfo *entry,
                                     dtbl_size_ft space_available)
{
  mhd_assert (dyn->buf_alloc_size >= space_available);
  dtbl_zeroout_strs_slack_ptr_space (dtbl_entr_strs_ptr_end (dyn, entry),
                                     space_available);
}


/**
 * Zero-out end up to mhd_dtbl_entry_slack at the end of the strings of some
 * entry.
 * @param dyn pointer to the dynamic table structure
 * @param entry the pointer to the entry information data
 * @param next_data_offset the offset of the next used memory area in the
 *                         buffer
 */
mhd_static_inline void
dtbl_zeroout_strs_slack_entry_next (struct mhd_HpackDTblContext *dyn,
                                    const struct mhd_HpackDTblEntryInfo *entry,
                                    dtbl_size_ft next_data_offset)
{
  mhd_assert (dyn->buf_alloc_size >= next_data_offset);
  dtbl_zeroout_strs_slack_offset_next (dyn,
                                       dtbl_entr_strs_end_min (entry),
                                       next_data_offset);
}


/**
 * Zero-out end up to mhd_dtbl_entry_slack at the end of the strings of some
 * entry.
 * @param dyn pointer to the dynamic table structure
 * @param loc_pos the number of location position
 */
mhd_static_inline void
dtbl_zeroout_strs_slack_pos (struct mhd_HpackDTblContext *dyn,
                             dtbl_idx_ft loc_pos)
{
  if (dtbl_get_pos_edge (dyn) == loc_pos)
    dtbl_zeroout_strs_slack_entry_space (dyn,
                                         dtbl_pos_entry_infoc (dyn,
                                                               loc_pos),
                                         dtbl_edge_gap (dyn));
  else
    dtbl_zeroout_strs_slack_offset_next (dyn,
                                         dtbl_pos_strs_end_min (dyn,
                                                                loc_pos),
                                         dtbl_pos_strs_start (dyn,
                                                              loc_pos + 1u));
}


#else  /* NDEBUG */

/**
 * No-op macro in non-debug builds.
 */
#define dtbl_zeroout_strs_slack_ptr_space(ptr,space)       ((void) 0)

/**
 * No-op macro in non-debug builds.
 */
#define dtbl_zeroout_strs_slack_offset_space(dyn,offset,space)     ((void) 0)

/**
 * No-op macro in non-debug builds.
 */
#define dtbl_zeroout_strs_slack_offset_next(dyn,offset,next_offset)  ((void) 0)

/**
 * No-op macro in non-debug builds.
 */
#define dtbl_zeroout_strs_slack_entry_space(dyn,entry,space)       ((void) 0)

/**
 * No-op macro in non-debug builds.
 */
#define dtbl_zeroout_strs_slack_entry_next(dyn,entry,next_offset)  ((void) 0)

/**
 * No-op macro in non-debug builds.
 */
#define dtbl_zeroout_strs_slack_pos(dyn,loc_pos)   ((void) 0)

#endif /* NDEBUG */

/**
 * Copy strings to the strings buffer for a potential new entry.
 *
 * This function ONLY copies strings to the strings buffer.
 * It does not create a new entry, nor update any numbers or limits.
 *
 * The caller may create a new entry pointing to copied strings and update
 * related data in the dynamic table structure following the call of this
 * function.
 *
 * The table data must be in consistent and valid state.
 *
 * In debug builds the function checks whether the copied data does not
 * overwrite any other used data in the buffer.
 *
 * @param dyn pointer to the dynamic table structure
 * @param name the name of the header, does NOT need to be zero-terminated
 * @param val the value of the header, does NOT need to be zero terminated
 * @param new_entry the pointer to the newly created entry; this entry must not
 *                  be in the table; must contain the lengths of the name
 *                  and the value corresponding to the strings pointed to by
 *                  @a name and @a val respectively.
 */
static void
dtbl_new_entry_copy_entr_strs (
  struct mhd_HpackDTblContext *restrict dyn,
  const char *restrict name,
  const char *restrict val,
  const struct mhd_HpackDTblEntryInfo *restrict new_entry)
{
  char *const strs_buff = dtbl_get_strs_buff (dyn);

#ifndef MHD_ASAN_ACTIVE
#  ifdef HAVE_UINTPTR_T
  /* The new entry must not be in the table */
  mhd_assert (dtbl_is_empty (dyn) ||
              (((uintptr_t) (const void*) dtbl_zero_entry_infoc (dyn)) < \
               (uintptr_t) (const void*) new_entry) || \
              (((uintptr_t) (const void*) dtbl_zero_entry_infoc (dyn)) > \
               (uintptr_t) (const void*) new_entry));
#  endif /* HAVE_UINTPTR_T */
#endif /* ! MHD_ASAN_ACTIVE*/

#ifndef NDEBUG
  if (1)
  {
    /* Find position of the entry which string is located after the new copied
       strings. */
    dtbl_idx_ft i;
    dtbl_size_ft next_data_offset = 0u;
    for (i = 0u; dyn->num_entries > i; ++i)
    {
      /* Check whether the buffer area referenced in the new entry is not used
         by other entries */
      mhd_assert ((0u == dtbl_pos_strs_size_min (dyn, i)) || \
                  (dtbl_pos_strs_end_min (dyn, i) <= \
                   dtbl_entr_strs_start (new_entry)) || \
                  (dtbl_entr_strs_end_min (new_entry) <= \
                   dtbl_pos_strs_start (dyn, i)));

      if (dtbl_entr_strs_end_min (new_entry) <= \
          dtbl_pos_strs_start (dyn, i))
      {
        next_data_offset = dtbl_pos_strs_start (dyn, i);
        break;
      }
    }
    if (dyn->num_entries == i)
    {
      /* Adding strings are at the edge of the strings buffer */
      mhd_assert (0u == next_data_offset);
      mhd_assert (dtbl_entr_strs_end_min (new_entry) <= \
                  dtbl_get_strs_ceiling (dyn));
      next_data_offset = dtbl_get_strs_ceiling (dyn);
    }
    mhd_assert (dtbl_entr_strs_end_min (new_entry) <= next_data_offset);
    dtbl_zeroout_strs_slack_entry_next (dyn,
                                        new_entry,
                                        next_data_offset);
  }
#endif

  /* Do not use dtbl_entr_strs_ptr_start() here as it does not work with
     entries outside the table. */
  if (0u != new_entry->name_len)
    memcpy (strs_buff + dtbl_entr_strs_start (new_entry),
            name,
            new_entry->name_len);
  if (0u != new_entry->val_len)
    memcpy (strs_buff + dtbl_entr_strs_start (new_entry) + new_entry->name_len,
            val,
            new_entry->val_len);
}


/**
 * Return a pointer to the slot for the next entry info.
 * The new slot is assumed to be located at the next edge location (below
 * the current edge entry location).
 * This function neither modifies the table nor reserves memory.
 * The returned pointer refers to writable but not yet initialised space
 * inside the table buffer; the caller must fill it and then increment
 * dyn->num_entries.
 * The result is undefined if there is no space in the buffer for the
 * additional entry info.
 * @param dyn pointer to the dynamic table structure
 * @return pointer to writable memory for the next entry info
 */
MHD_FN_PURE_ mhd_static_inline struct mhd_HpackDTblEntryInfo *
dtbl_new_edge_peek_slot (struct mhd_HpackDTblContext *dyn)
{
  mhd_assert (mhd_DTBL_ENTRY_INFO_SIZE <= dtbl_edge_gap (dyn));
  /* Do not call dtbl_pos_entry_info() as it works only with valid position
   * numbers, while the new position number is not valid yet. */
  return dtbl_get_infos (dyn) - dyn->num_entries;
}


/* ** Intrusive dangerous functions ** */

/**
 * Shift entries info data toward higher location positions by one location
 * position, starting at the specified location position and INCLUDING the
 * edge entry (i.e., the block [insert_pos_loc .. edge] is moved to
 * [insert_pos_loc + 1 .. edge + 1]). The entry information at
 * @a insert_pos_loc becomes uninitialised.
 *
 * Only entries information data are moved; strings in the buffer are not
 * modified.
 *
 * Note: this function internally moves data downward as higher location
 * numbers correspond to lower entry info addresses.
 *
 * This function does not update any table's data. The caller is responsible
 * for setting a valid entry data at the @a insert_pos_loc position, updating
 * the number of entries in the table, correcting the total size of the data
 * in the table and probably updating the position of the newest entry.
 *
 * Behaviour is undefined if @a insert_pos_loc is not a valid position in the
 * table or if the location of the next edge position is already used by the
 * strings in the buffer.
 *
 * @warning This function leaves table's data in an inconsistent state, the
 * caller should update the table's data properly. Until the data is fixed,
 * many dynamic table helper functions will work incorrectly.
 *
 * @param dyn pointer to the dynamic table structure
 * @param insert_pos_loc the location position of the first entry data to move
 */
mhd_static_inline void
dtbl_move_infos_up (struct mhd_HpackDTblContext *dyn,
                    const dtbl_idx_ft insert_pos_loc)
{
  mhd_assert (dtbl_get_pos_edge (dyn) >= insert_pos_loc);
  mhd_assert (dtbl_edge_gap (dyn) >= mhd_DTBL_ENTRY_INFO_SIZE);
  memmove (dtbl_new_edge_peek_slot (dyn),
           dtbl_edge_entry_infoc (dyn),
           (size_t)
           ((dtbl_get_pos_edge (dyn) - insert_pos_loc + 1u)
            * mhd_DTBL_ENTRY_INFO_SIZE));
}


/**
 * Shift entries info data for a contiguous range of locations toward lower
 * location positions to the specified location position.
 * The block [first .. last] is moved to [final .. final + last - first].
 * Depending on direction of the move, the entry-info slots in the range
 * (final + last - first .. last] or in the range [first .. final) become
 * uninitialised.
 *
 * Only entries information data are moved; strings in the buffer are not
 * modified.
 *
 * This function does not update any table's data. The caller is responsible
 * for updating the number of entries in the table, correcting the total size
 * of the data in the table and probably updating the position of the newest
 * entry.
 *
 * Behaviour is undefined if the specified positions are not valid for the
 * table.
 *
 * @warning This function leaves table's data in inconsistent state, the caller
 * should update the table's data properly. Until the data is fixed, many
 * dynamic table helper functions will work incorrectly.
 *
 * @param dyn pointer to the dynamic table structure
 * @param range_first_loc the first inclusive (lowest-numbered) entry position
 *                        to move
 * @param range_last_loc the last inclusive (higher number) entry position to
 *                       move, could be equal to @a range_first_loc
 * @param final_first_loc the final position location number of the first entry
 */
mhd_static_inline void
dtbl_move_infos_pos (struct mhd_HpackDTblContext *dyn,
                     const dtbl_idx_ft range_first_loc,
                     const dtbl_idx_ft range_last_loc,
                     const dtbl_idx_ft final_first_loc)
{
  /** Number of elements to move, including both the last and the first */
  const dtbl_idx_ft num_elements = range_last_loc - range_first_loc + 1u;
  /** The final position location number of the last entry */
  const dtbl_idx_ft final_last_loc = final_first_loc + num_elements - 1u;
  /* Do not use dtbl_pos_entry_info() here to avoid triggering asserts as
     the table data can be inconsistent */
  struct mhd_HpackDTblEntryInfo *const zero_info_pos = dtbl_get_infos (dyn);
  const struct mhd_HpackDTblEntryInfo *const src =
    zero_info_pos - range_last_loc;
  struct mhd_HpackDTblEntryInfo *const dst = zero_info_pos - final_last_loc;
  mhd_assert ((dyn->buf_alloc_size / mhd_DTBL_ENTRY_INFO_SIZE) \
              >= range_first_loc);
  mhd_assert ((dyn->buf_alloc_size / mhd_DTBL_ENTRY_INFO_SIZE) \
              >= range_last_loc);
  mhd_assert ((dyn->buf_alloc_size / mhd_DTBL_ENTRY_INFO_SIZE) \
              >= final_first_loc);
  mhd_assert ((dyn->buf_alloc_size / mhd_DTBL_ENTRY_INFO_SIZE) \
              >= final_last_loc);
  mhd_assert (range_first_loc <= range_last_loc);

  if (range_first_loc == final_first_loc)
    return;

  memmove (dst,
           src,
           (size_t) (num_elements * mhd_DTBL_ENTRY_INFO_SIZE));
}


/* ** Manipulating functions ** */

#ifndef NDEBUG
/**
 * Check internal consistency of the dynamic table internal data.
 * @param dyn the pointer to the dynamic table structure to check
 */
static void
dtbl_check_internals (const struct mhd_HpackDTblContext *dyn)
{
  mhd_assert (0u != dyn->buf_alloc_size);
  mhd_assert (dyn->buf_alloc_size > dyn->size_limit);
  mhd_assert (dyn->cur_size <= dyn->size_limit);
  mhd_assert (dyn->buf_alloc_size >= \
              (dyn->num_entries * mhd_DTBL_ENTRY_INFO_SIZE));
  mhd_assert (dyn->newest_pos <= dyn->num_entries);
  if (dtbl_is_empty (dyn))
  {
    mhd_assert (0u == dyn->cur_size);
    mhd_assert (0u == dyn->newest_pos);
  }
  else
  {
    const struct mhd_HpackDTblEntryInfo *const zero_entry =
      dtbl_zero_entry_infoc (dyn);
    dtbl_size_ft counted_size = 0u;
    dtbl_idx_ft i;

    mhd_assert (dyn->newest_pos < dyn->num_entries);
    mhd_assert ((0u != dyn->cur_size) && \
                "Each entry has minimal size, even with zero-length strings");
    mhd_assert (dyn->cur_size >= \
                (dyn->num_entries * mhd_dtbl_entry_overhead));
    mhd_assert (dtbl_edge_gap (dyn) <= dyn->buf_alloc_size);

    /* Check zero entry individually */
    /* If the newest entry is the edge entry, zero entry may have gap
       at the start of the buffer. */
    if (0u != dtbl_get_pos_oldest (dyn))
    {
      mhd_assert ((0u == zero_entry->offset) && \
                  "The extra gap between entries' strings is allowed only " \
                  "between the newest and the oldest entries");
    }
    mhd_assert (zero_entry->offset < dyn->buf_alloc_size);
    mhd_assert (zero_entry->name_len < dyn->buf_alloc_size);
    mhd_assert (zero_entry->val_len < dyn->buf_alloc_size);
    mhd_assert (dtbl_entr_strs_end_min (zero_entry) < dyn->buf_alloc_size);
    mhd_assert (dtbl_entr_strs_ptr_endc (dyn, zero_entry) <= \
                (const char*) dtbl_edge_entry_infoc (dyn));
    counted_size += dtbl_entr_size_formal (zero_entry);
    mhd_assert (counted_size <= dyn->cur_size);

    for (i = 1u; i <= dtbl_get_pos_edge (dyn); ++i)
    {
      const struct mhd_HpackDTblEntryInfo *const check_entry =
        dtbl_pos_entry_infoc (dyn,
                              i);

      mhd_assert ((dtbl_pos_strs_end_min (dyn, i - 1u) <= \
                   dtbl_pos_strs_start (dyn, i)) && \
                  "Strings data cannot overlap between entries");

      if (dtbl_get_pos_oldest (dyn) != i)
        mhd_assert ((dtbl_pos_strs_end_optm (dyn, i - 1u) >= \
                     dtbl_pos_strs_start (dyn, i)) && \
                    "The extra gap between entries' strings is allowed only " \
                    "between the newest and the oldest entries");

      mhd_assert (dtbl_pos_strs_start (dyn, i) < dyn->buf_alloc_size);
      mhd_assert (check_entry->name_len < dyn->buf_alloc_size);
      mhd_assert (check_entry->val_len < dyn->buf_alloc_size);
      mhd_assert (dtbl_entr_strs_end_min (check_entry) < dyn->buf_alloc_size);
      mhd_assert (dtbl_entr_strs_ptr_endc (dyn, check_entry) <= \
                  (const char*) dtbl_edge_entry_infoc (dyn));
      if (dtbl_get_pos_edge (dyn) != i)
        mhd_assert (0u != dtbl_pos_as_edge_get_gap (dyn, i));

      counted_size += dtbl_entr_size_formal (check_entry);
      mhd_assert (counted_size <= dyn->cur_size);
    }

    mhd_assert (dyn->cur_size == counted_size);
  }
}


#else  /* NDEBUG */
/* No-op in non-debug builds */
#define dtbl_check_internals(dyn)       ((void) 0)
#endif /* NDEBUG */

/**
 * Add the first entry to the table
 *
 * The table must be empty otherwise the behaviour is undefined.
 * The table must have enough space for the new entry.
 *
 * @param dyn the pointer to the dynamic table structure
 * @param name_len the length of the @a name
 * @param name the name of the header, does NOT need to be zero-terminated
 * @param val_len the length of the @a val
 * @param val the value of the header, does NOT need to be zero terminated
 */
static MHD_FN_PAR_IN_SIZE_ (3,2) MHD_FN_PAR_IN_SIZE_ (5,4) void
dtbl_add_first_entry (struct mhd_HpackDTblContext *restrict dyn,
                      const dtbl_size_ft name_len,
                      const char *restrict name,
                      const dtbl_size_ft val_len,
                      const char *restrict val)
{
  const dtbl_size_ft entry_strs_size = name_len + val_len;
  struct mhd_HpackDTblEntryInfo new_entry;

  /* Check parameters */
  mhd_assert (mhd_DTBL_VALUE_FITS (name_len));
  mhd_assert (mhd_DTBL_VALUE_FITS (val_len));
  mhd_assert (mhd_DTBL_VALUE_FITS (entry_strs_size));
  mhd_assert (entry_strs_size >= name_len);
  mhd_assert (entry_strs_size >= val_len);

  /* Check conditions */
  mhd_assert (dtbl_is_empty (dyn));

  dtbl_check_internals (dyn);

  new_entry.name_len = (dtbl_size_t) name_len;
  new_entry.val_len = (dtbl_size_t) val_len;
  new_entry.offset = 0u;

  mhd_assert (dtbl_get_free_formal (dyn) >= \
              dtbl_entr_size_formal (&new_entry));
  mhd_assert (dtbl_edge_gap (dyn) == dtbl_get_strs_ceiling (dyn));
  mhd_assert (dtbl_get_strs_ceiling (dyn) >= \
              (dtbl_entr_strs_size_min (&new_entry) \
               + mhd_DTBL_ENTRY_INFO_SIZE));

  dtbl_new_entry_copy_entr_strs (dyn,
                                 name,
                                 val,
                                 &new_entry);

  *(dtbl_new_edge_peek_slot (dyn)) = new_entry;
  dyn->num_entries = 1u;
  dyn->cur_size = dtbl_entr_size_formal (&new_entry);
  mhd_assert (0u == dtbl_get_pos_newest (dyn));
}


/**
 * Add new entry into the table at the new edge position
 *
 * This function adds a new entry after the existing edge-position entry,
 * updates all internal table data.
 * The function does NOT move strings in the strings buffer. The table's
 * buffer must have enough space for the new entry's strings and the new
 * entry data.
 *
 * The newest entry must be the edge entry.
 * The table must have enough space for the new entry.
 * The table must not be empty otherwise behaviour is undefined.
 *
 * @param dyn the pointer to the dynamic table structure
 * @param name_len the length of the @a name
 * @param name the name of the header, does NOT need to be zero-terminated
 * @param val_len the length of the @a val
 * @param val the value of the header, does NOT need to be zero terminated
 */
static MHD_FN_PAR_IN_SIZE_ (3,2) MHD_FN_PAR_IN_SIZE_ (5,4) void
dtbl_add_new_entry_at_new_edge (struct mhd_HpackDTblContext *restrict dyn,
                                const dtbl_size_ft name_len,
                                const char *restrict name,
                                const dtbl_size_ft val_len,
                                const char *restrict val)
{
  /** The total size of the strings of the new entry */
  const dtbl_size_ft entry_strs_size = name_len + val_len;
  struct mhd_HpackDTblEntryInfo new_entry;

  /* Check parameters */
  mhd_assert (mhd_DTBL_VALUE_FITS (name_len));
  mhd_assert (mhd_DTBL_VALUE_FITS (val_len));
  mhd_assert (mhd_DTBL_VALUE_FITS (entry_strs_size));
  mhd_assert (entry_strs_size >= name_len);
  mhd_assert (entry_strs_size >= val_len);
  mhd_assert (dtbl_get_free_formal (dyn) >= \
              dtbl_new_entry_size_formal (name_len, val_len));

  /* Check conditions */
  mhd_assert (! dtbl_is_empty (dyn));
  mhd_assert (dtbl_get_pos_edge (dyn) == dtbl_get_pos_newest (dyn));
  mhd_assert (dtbl_edge_gap (dyn) >= \
              entry_strs_size + mhd_DTBL_ENTRY_INFO_SIZE);

  dtbl_check_internals (dyn);

  /* Inserting at the edge */
  /* The simple case: just add new data at the edge. The previous entry
   * exists.  */
  /* Both strings and the entry info data must be stored in this memory
     area (edge gap). */

  new_entry.name_len = (dtbl_size_t) name_len;
  new_entry.val_len = (dtbl_size_t) val_len;
  new_entry.offset =
    dtbl_choose_strs_offset (dtbl_pos_strs_end_min (dyn,
                                                    dtbl_get_pos_edge (dyn)),
                             dtbl_get_strs_ceiling (dyn)
                             - mhd_DTBL_ENTRY_INFO_SIZE,
                             entry_strs_size);

  mhd_assert (dtbl_edge_gap (dyn) >= \
              dtbl_entr_strs_size_min (&new_entry) + mhd_DTBL_ENTRY_INFO_SIZE);

  dtbl_new_entry_copy_entr_strs (dyn,
                                 name,
                                 val,
                                 &new_entry);

  *(dtbl_new_edge_peek_slot (dyn)) = new_entry;
  dyn->newest_pos = dyn->num_entries;
  ++(dyn->num_entries);
  dyn->cur_size += dtbl_entr_size_formal (&new_entry);

  mhd_assert (dyn->cur_size > dtbl_entr_size_formal (&new_entry));
  mhd_assert (! dtbl_is_empty (dyn));
  mhd_assert (0u != dyn->newest_pos);
  mhd_assert (dyn->size_limit >= dyn->cur_size);
  /* The next assert evaluates dtbl_edge_gap(), which also checks the
     strings/infos do not overlap. */
  mhd_assert (dyn->buf_alloc_size > dtbl_edge_gap (dyn));
}


/**
 * Insert new entry into the table after the current newest (latest added)
 * entry. If the latest entry is at the edge, then the new entry is inserted
 * at zero position.
 *
 * This function inserts a new entry, moving entries information data as
 * necessary, updates all internal table data.
 * The function does NOT move strings in the strings buffer. The strings
 * buffer after the latest entry must have enough space for the new entry
 * strings.
 *
 * This function never inserts an entry at the edge (zero position is used
 * instead).
 * The table must have enough space for the new entry.
 * The table must not be empty otherwise behaviour is undefined.
 *
 * @param dyn the pointer to the dynamic table structure
 * @param name_len the length of the @a name
 * @param name the name of the header, does NOT need to be zero-terminated
 * @param val_len the length of the @a val
 * @param val the value of the header, does NOT need to be zero terminated
 */
static void
dtbl_insert_next_new_entry (struct mhd_HpackDTblContext *restrict dyn,
                            const dtbl_size_ft name_len,
                            const char *restrict name,
                            const dtbl_size_ft val_len,
                            const char *restrict val)
{
  /** The total size of the strings of the new entry */
  const dtbl_size_ft entry_strs_size = name_len + val_len;
  const dtbl_idx_ft loc_pos = dtbl_get_pos_oldest (dyn);
  const bool insert_at_zero = (0u == loc_pos);
  /** The pointer to the insert entry.
      The entry information data will be moved (together with higher numbered
      entries) and new entry will be inserted to this location. */
  struct mhd_HpackDTblEntryInfo *const insert_entry_ptr =
    dtbl_oldest_entry_info (dyn);
  /** The offset of the start of the available space */
  const dtbl_size_ft avail_space_start =
    insert_at_zero ? 0u : dtbl_entr_strs_end_min (dtbl_newest_entry_info (dyn));
  /** The offset of the end of the available space */
  const dtbl_size_ft avail_space_end =
    dtbl_entr_strs_start (dtbl_oldest_entry_infoc (dyn));
  struct mhd_HpackDTblEntryInfo new_entry;

  /* Check parameters */
  mhd_assert (mhd_DTBL_VALUE_FITS (name_len));
  mhd_assert (mhd_DTBL_VALUE_FITS (val_len));
  mhd_assert (mhd_DTBL_VALUE_FITS (entry_strs_size));
  mhd_assert (entry_strs_size >= name_len);
  mhd_assert (entry_strs_size >= val_len);
  mhd_assert (dtbl_get_pos_prev (dyn, loc_pos) == dtbl_get_pos_newest (dyn));
  mhd_assert (dtbl_get_pos_edge (dyn) >= loc_pos);
  /* Insertion as zero position is possible only if the newest entry
     is the edge entry (and the insertion wraps to the other side of
     the buffer). */
  mhd_assert (insert_at_zero ==
              (dtbl_get_pos_newest (dyn) == dtbl_get_pos_edge (dyn)));

  /* Check conditions */
  mhd_assert (! dtbl_is_empty (dyn));

  dtbl_check_internals (dyn);

  /* The new entry must be inserted either between two entries or at zero
     location position. The inserted entry is not at the edge (is followed by
     another entry). */
  mhd_assert (avail_space_end >= avail_space_start);

  new_entry.name_len = (dtbl_size_t) name_len;
  new_entry.val_len = (dtbl_size_t) val_len;
  new_entry.offset =
    insert_at_zero ? 0u : dtbl_choose_strs_offset (avail_space_start,
                                                   avail_space_end,
                                                   entry_strs_size);

  mhd_assert (avail_space_start <= new_entry.offset);
  mhd_assert (avail_space_end >= new_entry.offset);
  mhd_assert (avail_space_end >= new_entry.offset + entry_strs_size);
  mhd_assert (avail_space_end >= dtbl_entr_strs_end_min (&new_entry));
  mhd_assert (dtbl_get_free_formal (dyn) >= \
              dtbl_entr_size_formal (&new_entry));

  dtbl_new_entry_copy_entr_strs (dyn,
                                 name,
                                 val,
                                 &new_entry);

  /* Move entries info data as the new entry data must be inserted */
  dtbl_move_infos_up (dyn,
                      loc_pos);

  *insert_entry_ptr = new_entry;
  ++(dyn->num_entries);
  dyn->newest_pos = (dtbl_idx_t) loc_pos;
  dyn->cur_size += dtbl_entr_size_formal (&new_entry);

  mhd_assert (dyn->cur_size > dtbl_entr_size_formal (&new_entry));
  mhd_assert (dtbl_get_pos_edge (dyn) > dtbl_get_pos_newest (dyn));
  mhd_assert (! dtbl_is_empty (dyn));
  mhd_assert (dyn->size_limit >= dyn->cur_size);
  /* The next assert calls dtbl_edge_gap() which force checking non-overlap
     of entries and strings. */
  mhd_assert (dyn->buf_alloc_size > dtbl_edge_gap (dyn));
}


/**
 * Extend the table by inserting a new entry without prior eviction.
 *
 * The table must have enough formal free space for the new entry.
 * Behaviour is undefined if table's internal data is not consistent.
 * @param dyn the pointer to the dynamic table structure
 * @param name_len the length of the @a name
 * @param name the name of the header, does NOT need to be zero-terminated
 * @param val_len the length of the @a val
 * @param val the value of the header, does NOT need to be zero terminated
 */
static void
dtbl_extend_with_entry (struct mhd_HpackDTblContext *restrict dyn,
                        const dtbl_size_ft name_len,
                        const char *restrict name,
                        const dtbl_size_ft val_len,
                        const char *restrict val)
{
  const dtbl_size_ft entry_strs_size = name_len + val_len;

  mhd_assert (mhd_DTBL_VALUE_FITS (name_len));
  mhd_assert (mhd_DTBL_VALUE_FITS (val_len));
  mhd_assert (mhd_DTBL_VALUE_FITS (entry_strs_size));
  mhd_assert (entry_strs_size >= name_len);
  mhd_assert (entry_strs_size >= val_len);
  mhd_assert (dtbl_get_free_formal (dyn) >= \
              dtbl_new_entry_size_formal (name_len, val_len));

  dtbl_check_internals (dyn);

  if (dtbl_is_empty (dyn))
  {
    /* Empty table */
    dtbl_add_first_entry (dyn,
                          name_len,
                          name,
                          val_len,
                          val);

    return;  /* Inserted at zero position */
  }
  else if (dtbl_get_pos_newest (dyn) == dtbl_get_pos_edge (dyn))
  {
    /* Current insert position is at the edge */

    /* This section selects where to add a new entry. There are two options:
       + insert at the edge;
       + insert at the bottom (position wrap). */

    /** The space left on the top for strings and the new entry info */
    const dtbl_size_ft top_gap = dtbl_edge_gap (dyn);
    /** The space left on the bottom for strings */
    const dtbl_size_ft bottom_gap = dtbl_bottom_gap (dyn);
    /* 'true' to insert at the edge, 'false' to insert at the bottom */
    bool insert_at_the_edge;
    mhd_assert (! dtbl_is_empty (dyn));
    mhd_assert (0u != dyn->cur_size);

    if (mhd_DTBL_ENTRY_INFO_SIZE > top_gap)
    {
      /* Not enough space to add new entry info data */
      mhd_assert (0u != bottom_gap);
      mhd_assert (top_gap + bottom_gap >= \
                  mhd_DTBL_ENTRY_INFO_SIZE + entry_strs_size);
      dtbl_move_strs_down (dyn,
                           0u,
                           bottom_gap);
      mhd_assert (0u == dtbl_bottom_gap (dyn));
      insert_at_the_edge = true;
    }
    else if (entry_strs_size + mhd_dtbl_entry_slack <= bottom_gap)
    {
      /* The new strings and the standard slack fully fit the bottom space
       * in the buffer, the top space is enough for the new entry info. */
      insert_at_the_edge = false;
    }
    else if (entry_strs_size + mhd_dtbl_entry_slack
             + mhd_DTBL_ENTRY_INFO_SIZE <= top_gap)
    {
      /* The new strings, the new entry info and the standard slack fully fit
       * the top space in the buffer. */
      insert_at_the_edge = true;
    }
    else if (entry_strs_size <= bottom_gap)
    {
      /* The new strings without the standard slack fully fit the bottom space
       * in the buffer, the top space is enough for the new entry info. */
      insert_at_the_edge = false;
    }
    else if (entry_strs_size + mhd_DTBL_ENTRY_INFO_SIZE <= top_gap)
    {
      /* The new strings without the standard slack and the new entry info
       * fully fit the top space in the buffer. */
      insert_at_the_edge = true;
    }
    else
    {
      /* Neither top nor bottom of the buffer is enough for the new entry.
       * The buffer needs to be moved. */
      /* Strings could be moved either down or up */
      /* As the strings must be moved in any case, move strings to the bottom
         to insert the new entry at the edge and thus avoid moving entries
         info data in memory. */
      mhd_assert (top_gap < entry_strs_size \
                  + mhd_dtbl_entry_slack + mhd_DTBL_ENTRY_INFO_SIZE);
      mhd_assert ((top_gap + bottom_gap >= \
                   mhd_dtbl_entry_slack \
                   + entry_strs_size \
                   + mhd_dtbl_entry_slack + mhd_DTBL_ENTRY_INFO_SIZE) && \
                  "The total allocation size of the buffer is larger than " \
                  "required for strict HPACK. All extra size should be now " \
                  "on the top and on the bottom, as all other strings " \
                  "should now be place optimally or denser. The total free " \
                  "space must be enough for the previous entry slack and " \
                  "for complete new entry, including slack and info data.");

      dtbl_move_strs_down (dyn,
                           0u,
                           bottom_gap);

      mhd_assert (dtbl_edge_gap (dyn) >= \
                  entry_strs_size + mhd_DTBL_ENTRY_INFO_SIZE);
      mhd_assert ((dtbl_edge_gap (dyn) >= \
                   mhd_dtbl_entry_slack \
                   + entry_strs_size \
                   + mhd_dtbl_entry_slack + mhd_DTBL_ENTRY_INFO_SIZE) && \
                  "Strings have been compacted up to optimal space or "
                  "denser. The free space should be enough for optimal "
                  "placement.");
      insert_at_the_edge = true;
    }

    if (insert_at_the_edge)
    {
      dtbl_add_new_entry_at_new_edge (dyn,
                                      name_len,
                                      name,
                                      val_len,
                                      val);

      return;  /* Inserted at new edge position */
    }
  }
  else
  {
    /* Current insert position is in between two entries */

    /** The end of the strings of the newest entry */
    const dtbl_size_ft newest_entry_end =
      dtbl_pos_strs_end_min (dyn,
                             dtbl_get_pos_newest (dyn));
    /** The gap between the newest entry and the oldest entry */
    /** The start of the strings of the newest entry */
    const dtbl_size_ft oldest_entry_start =
      dtbl_pos_strs_start (dyn,
                           dtbl_get_pos_oldest (dyn));
    const dtbl_size_ft inbetween_gap =
      oldest_entry_start - newest_entry_end;
    /** The space left on the top for the new entry info data */
    const dtbl_size_ft top_gap = dtbl_edge_gap (dyn);
    /** The optimal space to place a new entry.
        The size consist of standard slack for previous entry string,
        the new entry strings and the standard slack for this entry. */
    const dtbl_size_ft optimal_inbetween_size =
      mhd_dtbl_entry_slack + entry_strs_size + mhd_dtbl_entry_slack;

    mhd_assert (dtbl_get_pos_edge (dyn) != dtbl_get_pos_newest (dyn));
    mhd_assert (dtbl_get_pos_oldest (dyn) > dtbl_get_pos_newest (dyn));
    mhd_assert (oldest_entry_start >= newest_entry_end);
    mhd_assert (0u != dtbl_get_pos_oldest (dyn));
    mhd_assert (0u != dyn->cur_size);

    mhd_assert (top_gap + inbetween_gap >= \
                entry_strs_size + mhd_DTBL_ENTRY_INFO_SIZE);
    mhd_assert ((top_gap + inbetween_gap >= \
                 optimal_inbetween_size + mhd_DTBL_ENTRY_INFO_SIZE) && \
                "This is not required for the insertion of the entry " \
                "but this is guaranteed by the checking the overall size " \
                "of the buffer before the insertion, so this is a check " \
                "for the overall handling logic.");

    if (mhd_DTBL_ENTRY_INFO_SIZE > top_gap)
    {
      /* Not enough space to add new entry info data */
      /* Shrink in-between space to the optimal entry strings size */
      const dtbl_size_ft shift_size = inbetween_gap - optimal_inbetween_size;

      mhd_assert (inbetween_gap > optimal_inbetween_size);
      mhd_assert (top_gap + shift_size >= mhd_DTBL_ENTRY_INFO_SIZE);

      dtbl_move_strs_down (dyn,
                           dtbl_get_pos_oldest (dyn),
                           shift_size);
    }
    else if (inbetween_gap < entry_strs_size)
    {
      /* Not enough space to add new entry strings */
      /* Grow in-between space to the standard step */
      const dtbl_size_ft shift_size = optimal_inbetween_size - inbetween_gap;

      mhd_assert (inbetween_gap < optimal_inbetween_size);
      mhd_assert (top_gap - shift_size >= mhd_DTBL_ENTRY_INFO_SIZE);

      dtbl_move_strs_up (dyn,
                         dtbl_get_pos_oldest (dyn),
                         shift_size);
    }
  }

  /* The new entry must be inserted either between two entries or at zero
     location position. The inserted entry is not at the edge (is followed by
     another entry). */
  /* Insertion to the empty table and insertion at the edge are handled
     earlier. */
  dtbl_insert_next_new_entry (dyn,
                              name_len,
                              name,
                              val_len,
                              val);
}


/**
 * Evict the oldest entries as needed and add a new entry.
 *
 * The formal size of the new entry must be less than or equal to the table
 * maximum formal size.
 * The table must NOT have enough free space to add a new entry without
 * eviction.
 *
 * Behaviour is undefined if table's internal data is not consistent.
 * @param dyn the pointer to the dynamic table structure
 * @param name_len the length of the @a name
 * @param name the name of the header, does NOT need to be zero-terminated
 * @param val_len the length of the @a val
 * @param val the value of the header, does NOT need to be zero terminated
 */
static MHD_FN_PAR_IN_SIZE_ (3,2) MHD_FN_PAR_IN_SIZE_ (5,4) void
dtbl_evict_add_entry (struct mhd_HpackDTblContext *restrict dyn,
                      const dtbl_size_ft name_len,
                      const char *restrict name,
                      const dtbl_size_ft val_len,
                      const char *restrict val)
{
  /** The total size of the strings of the new entry */
  const dtbl_size_ft entry_strs_size = name_len + val_len;
  /** The starting eviction position */
  const dtbl_idx_ft eviction_start =
    dtbl_get_pos_oldest (dyn);
  /** The final (inclusive) eviction entry */
  dtbl_idx_ft eviction_end;
  const dtbl_size_ft needed_evict_min =
    dtbl_new_entry_size_formal (name_len, val_len) - dtbl_get_free_formal (dyn);
  dtbl_size_ft evicted_size;
  /** The total number of entries to evict */
  dtbl_idx_ft num_to_evict;

  mhd_assert (mhd_DTBL_VALUE_FITS (name_len));
  mhd_assert (mhd_DTBL_VALUE_FITS (val_len));
  mhd_assert (0u != dyn->cur_size);
  mhd_assert (! dtbl_is_empty (dyn));
  mhd_assert (mhd_DTBL_VALUE_FITS (entry_strs_size));
  mhd_assert (entry_strs_size >= name_len);
  mhd_assert (entry_strs_size >= val_len);
  mhd_assert (dtbl_get_free_formal (dyn) < \
              dtbl_new_entry_size_formal (name_len, val_len));
  mhd_assert (dtbl_get_size_max_formal (dyn) >= \
              dtbl_new_entry_size_formal (name_len, val_len));
  mhd_assert (0u != needed_evict_min);
  mhd_assert (needed_evict_min <= dyn->cur_size);
  dtbl_check_internals (dyn);

  eviction_end = eviction_start;
  evicted_size = dtbl_pos_size_formal (dyn,
                                       eviction_end);

  while (needed_evict_min > evicted_size)
  {
    eviction_end = dtbl_get_pos_next (dyn,
                                      eviction_end);

    mhd_assert (eviction_start != eviction_end);

    evicted_size += dtbl_pos_size_formal (dyn,
                                          eviction_end);
  }
  mhd_assert (needed_evict_min <= evicted_size);
#ifdef MHD_USE_CODE_HARDENING
  if (eviction_start > eviction_end)
    num_to_evict =
      eviction_end + dtbl_get_num_entries (dyn) - eviction_start + 1u;
  else
    num_to_evict = eviction_end - eviction_start + 1u;
#else  /* ! MHD_USE_CODE_HARDENING */
  num_to_evict =
    ((dtbl_get_num_entries (dyn) + eviction_end
      - eviction_start) % dtbl_get_num_entries (dyn)) + 1u;
#endif /* ! MHD_USE_CODE_HARDENING */
  mhd_assert (0u != num_to_evict);
  mhd_assert (dtbl_get_num_entries (dyn) >= num_to_evict);

  if (mhd_COND_ALMOST_NEVER (dtbl_get_num_entries (dyn) == num_to_evict))
  {
    /* Simplest situation: evicted all existing entries completely */
    /* Processing:
       + reset the table,
       + add the new first entry */
    dtbl_reset (dyn);
    dtbl_add_first_entry (dyn,
                          name_len,
                          name,
                          val_len,
                          val);
    return;
  }
  else if (dtbl_get_pos_edge (dyn) == eviction_end)
  {
    /* Eviction area ends at the edge, at least one entry is not evicted. */
    /* Processing:
       + reduce the number of entries in the table (evicted entries become
         ignored),
       + reduce the official size of the table,
       + add the new entry at the edge.
       No need to move the data in the table's buffer. */
    mhd_assert (eviction_end >= eviction_start);
    mhd_assert ((0u == dtbl_pos_strs_start (dyn, 0u)) && \
                "An extra gap is allowed only between the newest and the " \
                "oldest entries. The newest entry was not the edge entry " \
                "before the eviction.");
    mhd_assert (dtbl_get_pos_newest (dyn) == (eviction_start - 1u));

    dyn->cur_size -= (dtbl_size_t) evicted_size;
    dyn->num_entries = (dtbl_idx_t) eviction_start;

    dtbl_add_new_entry_at_new_edge (dyn,
                                    name_len,
                                    name,
                                    val_len,
                                    val);
    return;
  }
  else if ((0u != eviction_start) &&
           (eviction_end >= eviction_start))
  {
    /* Entries are evicted in between other entries, at least two entries
       are not evicted (at the start and at the edge). */
    /* Processing:
       + set strings size of the first evicted entry to zero (will be replaced
         with new entry strings),
       + remove other evicted entries information data (if any) by moving
         higher numbered entries,
       + reduce the official size of the table,
       + move strings data in the buffer (if needed),
       + replace the first evicted entry with the new entry. */
    struct mhd_HpackDTblEntryInfo *replace_entry_ptr =
      dtbl_pos_entry_info (dyn,
                           eviction_start);
    /** The last entry to keep before the evicted entries */
    dtbl_idx_ft last_entry_keep = dtbl_get_pos_prev (dyn,
                                                     eviction_start);
    /** The first entry to keep after the evicted entries */
    dtbl_idx_ft first_entry_keep = dtbl_get_pos_next (dyn,
                                                      eviction_end);
    /** Number of entries to keep at the edge (after evicted entries) */
    dtbl_idx_ft num_keep_at_edge =
      dtbl_get_pos_edge (dyn) - first_entry_keep + 1u;
    /** The position of the start of the space for the new entry strings */
    const dtbl_size_ft space_start = dtbl_pos_strs_end_min (dyn,
                                                            last_entry_keep);
    /** The position of the end of the space for the new entry strings */
    dtbl_size_ft space_end = dtbl_pos_strs_start (dyn,
                                                  first_entry_keep);
    dtbl_size_ft space_size = space_end - space_start;
    struct mhd_HpackDTblEntryInfo new_entry;

    mhd_assert (first_entry_keep > last_entry_keep);
    mhd_assert (dtbl_get_num_entries (dyn) - num_to_evict >= 2u);
    mhd_assert (dtbl_get_pos_edge (dyn) >= first_entry_keep);
    mhd_assert (0u != num_keep_at_edge);
    mhd_assert (dtbl_get_num_entries (dyn) > num_keep_at_edge);
    mhd_assert (space_end >= space_start);
    mhd_assert (dyn->buf_alloc_size > space_size);

    replace_entry_ptr->name_len = 0u;
    replace_entry_ptr->val_len = 0u;
    /* Keep the entry to be replaced and move not evicted entries at the edge */
    dtbl_move_infos_pos (dyn,
                         first_entry_keep,
                         dtbl_get_pos_edge (dyn),
                         eviction_start + 1u);
    /* Keep the standard overhead of the entry being replaced */
    dyn->cur_size -= (dtbl_size_t) (evicted_size - mhd_dtbl_entry_overhead);
    dyn->num_entries -= (dtbl_idx_t) (num_to_evict - 1u);

    if (space_size < entry_strs_size)
    {
      /* No space to put the new entry strings.
       * Need to move strings in the buffer. */
      const dtbl_size_ft shift_size =
        mhd_dtbl_entry_slack + entry_strs_size + mhd_dtbl_entry_slack
        - space_size;
      mhd_assert (dtbl_edge_gap (dyn) > shift_size);
      dtbl_move_strs_up (dyn,
                         eviction_start + 1u,
                         shift_size);
      space_size =
        mhd_dtbl_entry_slack + entry_strs_size + mhd_dtbl_entry_slack;
    }

    mhd_assert (space_size >= entry_strs_size);

    new_entry.name_len = (dtbl_size_t) name_len;
    new_entry.val_len = (dtbl_size_t) val_len;
    new_entry.offset = dtbl_choose_strs_offset_for_size (space_start,
                                                         space_size,
                                                         entry_strs_size);

    mhd_assert (dtbl_get_num_entries (dyn) > (eviction_start + 1u));
    mhd_assert (dtbl_entr_strs_end_min (&new_entry) <= \
                dtbl_pos_strs_start (dyn, eviction_start + 1u));

    dtbl_new_entry_copy_entr_strs (dyn,
                                   name,
                                   val,
                                   &new_entry);
    *replace_entry_ptr = new_entry;
    /* Keep the standard overhead of the entry being replaced  */
    dyn->cur_size += (dtbl_size_t) entry_strs_size;
    mhd_assert ((dyn->newest_pos + 1u) == eviction_start);
    dyn->newest_pos = (dtbl_idx_t) eviction_start;

    return;
  }
  else
  {
    /* Eviction area includes zero position entry, at least one entry is not
       evicted.
       The most complex case: some free space is at the bottom of the
       buffer and some free space can be at the edge of the buffer.
       The code should choose where to insert a new entry: at the bottom or
       at the edge. */
    /* Processing:
       + if bottom area is large enough insert at the bottom (no need to move
         strings (typically large), only entries info data may need to be
         moved (fast as it is typically smaller and is always aligned),
       + otherwise remove evicted info data with low numbers, move strings
         in the buffer (if needed) and add the new entry at the edge. */
    /** The first entry to keep */
    dtbl_idx_ft first_entry_keep = dtbl_get_pos_next (dyn,
                                                      eviction_end);
    /** The last entry to keep */
    dtbl_idx_ft last_entry_keep = dtbl_get_pos_prev (dyn,
                                                     eviction_start);
    dtbl_idx_ft num_to_keep =
      ((dtbl_idx_ft) (last_entry_keep - first_entry_keep) + 1u);
    /** The available space at the bottom of the strings buffer after
        eviction of the entries */
    dtbl_size_ft new_bottom_gap = dtbl_pos_strs_start (dyn,
                                                       first_entry_keep);

    mhd_assert (dtbl_get_pos_edge (dyn) != eviction_end);
    mhd_assert (last_entry_keep >= first_entry_keep);
    mhd_assert (0u != num_to_keep);
    mhd_assert (dtbl_get_num_entries (dyn) > num_to_keep);
    mhd_assert (num_to_keep + num_to_evict == dtbl_get_num_entries (dyn));

    if (new_bottom_gap >= entry_strs_size)
    {
      /* Enough space at the bottom to put the new entry strings */
      /* No need to check the space for the entries information data as
         new entry replaces evicted zero position entry. */
      struct mhd_HpackDTblEntryInfo *replace_entry_ptr =
        dtbl_zero_entry_info (dyn);
      struct mhd_HpackDTblEntryInfo new_entry;

      /* Keep data correct and asserts quite */
      replace_entry_ptr->name_len = 0u;
      replace_entry_ptr->val_len = 0u;
      /* Move entries information data if needed,
         the zero position entry information will be overwritten with
         a new data. */
      dtbl_move_infos_pos (dyn,
                           first_entry_keep,
                           last_entry_keep,
                           1u);
      /* Keep the standard overhead of the entry being replaced */
      dyn->cur_size -= (dtbl_size_t) (evicted_size - mhd_dtbl_entry_overhead);
      dyn->num_entries = (dtbl_idx_t) num_to_keep + 1u; /* Plus replaced zero position */

      new_entry.name_len = (dtbl_size_t) name_len;
      new_entry.val_len = (dtbl_size_t) val_len;
      new_entry.offset = 0u;

      mhd_assert (dtbl_entr_strs_end_min (&new_entry) <= \
                  dtbl_pos_strs_start (dyn, 1u));

      dtbl_new_entry_copy_entr_strs (dyn,
                                     name,
                                     val,
                                     &new_entry);
      *replace_entry_ptr = new_entry;
      /* Keep the standard overhead of the entry being replaced  */
      dyn->cur_size += (dtbl_size_t) entry_strs_size;
      dyn->newest_pos = 0u;

      dtbl_zeroout_strs_slack_pos (dyn,
                                   dtbl_get_pos_newest (dyn));

      return;
    }
    else
    {
      /* Not enough space at zero position in the buffer */
      /* The new entry will be added at the edge of the buffer after
         eviction */
      /** The available space at the top of the strings buffer after moving
          entries information data */
      const dtbl_size_ft new_top_gap =
        dtbl_pos_as_edge_get_gap (dyn,
                                  last_entry_keep) /* The gap after the last kept entry */
        + (first_entry_keep * mhd_DTBL_ENTRY_INFO_SIZE); /* 'first_entry_keep' will be evicted at zero position */

      mhd_assert (1u <= first_entry_keep);
      mhd_assert (new_top_gap + new_bottom_gap >= \
                  entry_strs_size + mhd_DTBL_ENTRY_INFO_SIZE);
      mhd_assert ((new_top_gap + new_bottom_gap >= \
                   mhd_dtbl_entry_slack + entry_strs_size
                   + mhd_dtbl_entry_slack + mhd_DTBL_ENTRY_INFO_SIZE) && \
                  "This is not required for the insertion of the entry " \
                  "but this is guaranteed by the checking the overall size " \
                  "of the buffer before the insertion, so this is a check " \
                  "for the overall handling logic.");

      /* Move entries information data first to free some space */
      /* No slot kept in evicted entries as the new entry will be added
         at the edge */
      dtbl_move_infos_pos (dyn,
                           first_entry_keep,
                           last_entry_keep,
                           0u);
      /* Keep the table internal data correct */
      dyn->num_entries = (dtbl_idx_t) num_to_keep;
      dyn->newest_pos = (dtbl_idx_t) (num_to_keep - 1u);
      dyn->cur_size -= (dtbl_size_t) evicted_size;

      mhd_assert (new_top_gap == dtbl_edge_gap (dyn));

      if (new_top_gap < (entry_strs_size + mhd_DTBL_ENTRY_INFO_SIZE))
      {
        /* Not enough space on the top of the buffer (checked earlier),
           not enough space at the bottom of the buffer.
           The strings in the buffer need to be moved.
           Eliminate all space at the bottom. */
        const dtbl_size_ft shift_size = new_bottom_gap;
        mhd_assert (0u != new_bottom_gap);
        mhd_assert (new_bottom_gap == dtbl_bottom_gap (dyn));

        dtbl_move_strs_down (dyn,
                             0u,
                             shift_size);
        mhd_assert (0u == dtbl_bottom_gap (dyn));
        mhd_assert (new_top_gap + shift_size == dtbl_edge_gap (dyn));
        mhd_assert (dtbl_edge_gap (dyn) >= \
                    mhd_dtbl_entry_slack \
                    + dtbl_new_entry_strs_size_formal (entry_strs_size) && \
                    "All strings have been compacted, the free space must " \
                    "be enough for the previous entry slack and for " \
                    "a complete new entry, including slack and info data.");
      }

      /* The entries have been evicted.
         The edge of the buffer (top of the strings buffer) has enough space
         for the new strings and the new entry info */
      dtbl_add_new_entry_at_new_edge (dyn,
                                      name_len,
                                      name,
                                      val_len,
                                      val);

      return;
    }
  }
}


/**
 * Evict entries to reach the specified final formal table size.
 *
 * The function evicts the oldest entries until the formal used size is less
 * than or equal to @a final_formal_size.
 *
 * The table must not be empty.
 * Behaviour is undefined if @a final_formal_size is not less than the current
 * formal used size.
 * @param dyn the pointer to the dynamic table structure
 * @param max_used_final the target formal size of data in the table
 */
static void
dtbl_evict_to_size (struct mhd_HpackDTblContext *restrict dyn,
                    dtbl_size_ft max_used_final)
{
  const dtbl_size_ft needed_evict_min =
    dtbl_get_used_formal (dyn) - max_used_final;
  /** The starting eviction position */
  const dtbl_idx_ft eviction_start =
    dtbl_get_pos_oldest (dyn);
  /** The final (inclusive) eviction entry */
  dtbl_idx_ft eviction_end;

  dtbl_size_ft evicted_size;
  /** The total number of entries to evict */
  dtbl_idx_ft num_to_evict;

  mhd_assert (dtbl_get_used_formal (dyn) > max_used_final);
  mhd_assert (0u != dyn->cur_size);
  mhd_assert (! dtbl_is_empty (dyn));
  mhd_assert (0u != needed_evict_min);
  mhd_assert (needed_evict_min <= dyn->cur_size);

  eviction_end = eviction_start;
  evicted_size = dtbl_pos_size_formal (dyn,
                                       eviction_end);

  while (needed_evict_min > evicted_size)
  {
    eviction_end = dtbl_get_pos_next (dyn,
                                      eviction_end);

    mhd_assert (eviction_start != eviction_end);

    evicted_size += dtbl_pos_size_formal (dyn,
                                          eviction_end);
  }

  mhd_assert (needed_evict_min <= evicted_size);
  num_to_evict =
    (dtbl_get_num_entries (dyn) + eviction_end
     - eviction_start) % dtbl_get_num_entries (dyn) + 1u;
  mhd_assert (0u != num_to_evict);
  mhd_assert (dtbl_get_num_entries (dyn) >= num_to_evict);

  if (mhd_COND_ALMOST_NEVER (dtbl_get_num_entries (dyn) == num_to_evict))
  {
    /* Simplest situation: evicted all existing entries completely */
    dtbl_reset (dyn);
    return;
  }
  else if (dtbl_get_pos_edge (dyn) == eviction_end)
  {
    /* Eviction area ends at the edge, at least one entry is not evicted. */
    mhd_assert (eviction_end >= eviction_start);
    mhd_assert (dtbl_get_pos_newest (dyn) == (eviction_start - 1u));

    dyn->cur_size -= (dtbl_size_t) evicted_size;
    dyn->num_entries = (dtbl_idx_t) eviction_start;

    return;
  }
  else if ((0u != eviction_start) &&
           (eviction_end >= eviction_start))
  {
    /* Entries are evicted in-between of other entries, at least two entries
       are not evicted (at the start and at the edge). */
    /** The last entry to keep before the evicted entries */
    dtbl_idx_ft last_entry_keep = dtbl_get_pos_prev (dyn,
                                                     eviction_start);
    /** The first entry to keep after the evicted entries */
    dtbl_idx_ft first_entry_keep = dtbl_get_pos_next (dyn,
                                                      eviction_end);

    mhd_assert (first_entry_keep > last_entry_keep);
    mhd_assert (dtbl_get_num_entries (dyn) - num_to_evict >= 2u);
    mhd_assert (dtbl_get_pos_edge (dyn) >= first_entry_keep);

    /* Move not evicted entries at the edge */
    dtbl_move_infos_pos (dyn,
                         first_entry_keep,
                         dtbl_get_pos_edge (dyn),
                         eviction_start);
    dyn->cur_size -= (dtbl_size_t) evicted_size;
    dyn->num_entries -= (dtbl_idx_t) num_to_evict;
    mhd_assert (dtbl_get_pos_edge (dyn) >= dtbl_get_pos_newest (dyn));

    return;
  }
  else
  {
    /* Eviction area includes zero position entry, at least one entry is not
       evicted. */
    /** The first entry to keep */
    dtbl_idx_ft first_entry_keep = dtbl_get_pos_next (dyn,
                                                      eviction_end);
    /** The last entry to keep */
    dtbl_idx_ft last_entry_keep = dtbl_get_pos_prev (dyn,
                                                     eviction_start);
    dtbl_idx_ft num_to_keep =
      ((dtbl_idx_ft) (last_entry_keep - first_entry_keep) + 1u);

    mhd_assert (dtbl_get_pos_edge (dyn) != eviction_end);
    mhd_assert (0u != num_to_keep);
    mhd_assert (dtbl_get_num_entries (dyn) > num_to_keep);
    mhd_assert (num_to_keep + num_to_evict == dtbl_get_num_entries (dyn));

    dtbl_move_infos_pos (dyn,
                         first_entry_keep,
                         last_entry_keep,
                         0u);
    dyn->cur_size -= (dtbl_size_t) evicted_size;
    dyn->num_entries = (dtbl_idx_t) num_to_keep;
    dyn->newest_pos = dtbl_get_pos_edge (dyn);

    return;
  }
}


/**
 * Adapt the in-memory layout to a new allocation and/or formal size.
 *
 * The function updates @a dyn to match @a new_alloc_size and
 * @a new_formal_size, moving entries information data as needed.
 *
 * The @a new_formal_size must be larger than or equal to the current formal
 * size of the entries in the table.
 * The table must not be empty.
 * @param dyn the pointer to the dynamic table structure
 * @param new_alloc_size the new size of the shared buffer allocation
 * @param new_formal_size the new formal HPACK table size limit
 */
static void
dtbl_perform_resize (struct mhd_HpackDTblContext *restrict dyn,
                     const dtbl_size_ft new_alloc_size,
                     const dtbl_size_ft new_formal_size)
{
  /* Obtain the data from the old table state */
  const struct mhd_HpackDTblEntryInfo *const infos_old_ptr =
    dtbl_edge_entry_infoc (dyn);
  struct mhd_HpackDTblEntryInfo *infos_new_ptr;
  const dtbl_size_ft entries_total_size =
    dtbl_get_num_entries (dyn) * mhd_DTBL_ENTRY_INFO_SIZE;

  mhd_assert (! dtbl_is_empty (dyn));
  mhd_assert (mhd_DTBL_VALUE_FITS (new_alloc_size));
  mhd_assert (mhd_DTBL_VALUE_FITS (new_formal_size));
  mhd_assert (new_formal_size <= mhd_DTBL_MAX_SIZE);
  mhd_assert (new_formal_size < new_alloc_size);
  mhd_assert (dtbl_get_used_formal (dyn) <= new_formal_size);

  if (dyn->buf_alloc_size > new_alloc_size)
  {
    /* Shrinking the buffer */
    mhd_assert (dtbl_get_size_max_formal (dyn) > new_formal_size);
    mhd_assert (((dyn->buf_alloc_size - new_alloc_size) \
                 % mhd_ALIGNOF (struct mhd_HpackDTblEntryInfo)) == 0);

    if (dtbl_edge_gap (dyn) < (dyn->buf_alloc_size - new_alloc_size))
      dtbl_compact_strs (dyn);

    mhd_assert (dtbl_edge_gap (dyn) >= (dyn->buf_alloc_size - new_alloc_size));

  }
  else if (mhd_COND_ALMOST_NEVER (new_alloc_size == dyn->buf_alloc_size))
  {
    dyn->size_limit = (dtbl_size_t) new_formal_size;
    return; /* Just update the formal size */
  }
  else
  {
    /* Growing the buffer */
    mhd_assert (dtbl_get_size_max_formal (dyn) < new_formal_size);
    mhd_assert (((new_alloc_size - dyn->buf_alloc_size) \
                 % mhd_ALIGNOF (struct mhd_HpackDTblEntryInfo)) == 0);
  }

  /* Set the new table size */
  dyn->size_limit = (dtbl_size_t) new_formal_size;
  dyn->buf_alloc_size = (dtbl_size_t) new_alloc_size;

  /* Get the data location based on the new table size */
  infos_new_ptr = dtbl_edge_entry_info (dyn);
  memmove (infos_new_ptr,
           infos_old_ptr,
           (size_t) entries_total_size);
}


/**
 * Adapt the in-memory layout to a new allocation and/or formal size.
 *
 * The function updates @a dyn to match @a new_alloc_size and
 * @a new_formal_size, moving entries information data as needed.
 *
 * The @a new_formal_size must be larger than or equal to the current formal
 * size of the entries in the table.
 * @param dyn the pointer to the dynamic table structure
 * @param new_alloc_size the new size of the shared buffer allocation
 * @param new_formal_size the new formal HPACK table size limit
 */
static void
dtbl_adapt_to_new_size (struct mhd_HpackDTblContext *restrict dyn,
                        const dtbl_size_ft new_alloc_size,
                        const dtbl_size_ft new_formal_size)
{
  mhd_assert (mhd_DTBL_VALUE_FITS (new_alloc_size));
  mhd_assert (mhd_DTBL_VALUE_FITS (new_formal_size));
  mhd_assert (new_formal_size <= mhd_DTBL_MAX_SIZE);
  mhd_assert (new_formal_size < new_alloc_size);

  if (! dtbl_is_empty (dyn))
  {
    dtbl_perform_resize (dyn,
                         new_alloc_size,
                         new_formal_size);
    return; /* Internal structure has been fully updated */
  }

  /* Just set the new table size */
  dyn->size_limit = (dtbl_size_t) new_formal_size;
  dyn->buf_alloc_size = (dtbl_size_t) new_alloc_size;

}


/* ** Allocation helpers ** */

/**
 * Calculate the buffer allocation size from the requested formal table size.
 *
 * The returned size includes additional slack to reduce the need for frequent
 * compaction and is rounded up to alignment suitable for entry information
 * data. The size accounts for the alignment difference between the context
 * structure and the entry information data.
 *
 * @param formal_size the requested formal HPACK table size
 * @return the allocation size for the strings/infos shared buffer
 */
mhd_static_inline dtbl_size_t
dtbl_calc_alloc_size (dtbl_size_ft formal_size)
{
  dtbl_size_ft dyn_table_alloc_size;

  mhd_assert (mhd_DTBL_VALUE_FITS (formal_size));

  dyn_table_alloc_size = formal_size;
  /* Add some slack to lower the need for the buffer compaction */
  dyn_table_alloc_size += formal_size / 64;
  dyn_table_alloc_size += 2 * mhd_DTBL_ENTRY_INFO_SIZE;
  /* Round up to alignment of the entry info data, which is placed at the
     end of the buffer. */
  dyn_table_alloc_size =
    ((dyn_table_alloc_size + mhd_ALIGNOF (struct mhd_HpackDTblEntryInfo) - 1u)
     / mhd_ALIGNOF (struct mhd_HpackDTblEntryInfo))
    * mhd_ALIGNOF (struct mhd_HpackDTblEntryInfo);
  /* Adjust the size of the allocation in case the alignment of
     mhd_HpackDTblEntryInfo is stricter than that of mhd_HpackDTblContext */
  dyn_table_alloc_size +=
    (mhd_ALIGNOF (struct mhd_HpackDTblEntryInfo)
     - (sizeof(struct mhd_HpackDTblContext)
        % mhd_ALIGNOF (struct mhd_HpackDTblEntryInfo)))
    % mhd_ALIGNOF (struct mhd_HpackDTblEntryInfo);

  mhd_assert (mhd_DTBL_VALUE_FITS (dyn_table_alloc_size));

  return (dtbl_size_t) dyn_table_alloc_size;
}


/* ** Entries finders ** */

/**
 * Find an entry in the dynamic table that exactly matches the given
 * name and value.
 *
 * The @a name and @a val do not need to be zero-terminated.
 * The table must not be empty.
 *
 * @param dyn const pointer to the dynamic table structure
 * @param name_len length of @a name in bytes
 * @param name pointer to the header field name
 * @param val_len length of @a val in bytes
 * @param val pointer to the header field value
 * @return the HPACK index (> #mhd_HPACK_STBL_LAST_IDX) of the matching entry,
 *         or 0 if not found
 */
static MHD_FN_PAR_IN_SIZE_ (3,2) MHD_FN_PAR_IN_SIZE_ (5,4) dtbl_idx_t
dtbl_find_entry (const struct mhd_HpackDTblContext *restrict dyn,
                 dtbl_size_ft name_len,
                 const char *restrict name,
                 dtbl_size_ft val_len,
                 const char *restrict val)
{
  /* The table must not be empty */
  const struct mhd_HpackDTblEntryInfo *entries =
    dtbl_get_infos_as_arrayc (dyn);
  dtbl_idx_ft i;
  for (i = 0u; i < dtbl_get_num_entries (dyn); ++i)
  {
    const struct mhd_HpackDTblEntryInfo *const entry = entries + i;

    if (name_len != entry->name_len)
      continue;
    if (val_len != entry->val_len)
      continue;
    if (((0u == name_len) ||
         (0 == memcmp (name,
                       dtbl_entr_strs_ptr_namec (dyn,
                                                 entry),
                       name_len)))
        &&
        ((0u == val_len) ||
         (0 == memcmp (val,
                       dtbl_entr_strs_ptr_valuec (dyn,
                                                  entry),
                       val_len))))
    { /* Found the entry */
      return dtbl_get_hpack_idx_from_pos (dyn,
                                          dtbl_get_pos_edge (dyn) - i);
    }
  }
  return 0u; /* Not found */
}


/**
 * Find an entry in the dynamic table whose name exactly matches @a name.
 *
 * The @a name does not need to be zero-terminated.
 * The table must not be empty.
 *
 * @param dyn const pointer to the dynamic table structure
 * @param name_len length of @a name in bytes
 * @param name pointer to the header field name
 * @return the HPACK index (> #mhd_HPACK_STBL_LAST_IDX) of the matching entry,
 *         or 0 if not found
 */
static MHD_FN_PAR_IN_SIZE_ (3,2) dtbl_idx_t
dtbl_find_name (const struct mhd_HpackDTblContext *restrict dyn,
                dtbl_size_ft name_len,
                const char *restrict name)
{
  /* The table must not be empty */
  const struct mhd_HpackDTblEntryInfo *entries =
    dtbl_get_infos_as_arrayc (dyn);
  dtbl_idx_ft i;
  for (i = 0u; i < dtbl_get_num_entries (dyn); ++i)
  {
    const struct mhd_HpackDTblEntryInfo *const entry = entries + i;

    if (name_len != entry->name_len)
      continue;
    if ((0u == name_len) ||
        (0 == memcmp (name,
                      dtbl_entr_strs_ptr_namec (dyn,
                                                entry),
                      name_len)))
    { /* Found the entry */
      return dtbl_get_hpack_idx_from_pos (dyn,
                                          dtbl_get_pos_edge (dyn) - i);
    }
  }
  return 0u; /* Not found */
}


/* **** ________________ End of dynamic table helpers _________________ **** */

/* ****** ------------------- Dynamic table API --------------------- ****** */

/*
 * The API is designed to be used by one thread only.
 * If any thread is modifying the data in the dynamic table, then any access
 * in any other thread at the same time is not safe!
 */


/**
 * Create a dynamic HPACK table context with the specified formal size limit.
 *
 * The allocation includes the context and a shared buffer. The table is
 * initialised to an empty state. The function allocates slightly more than
 * @a dyn_table_size due to the internal overhead.
 *
 * @param dyn_table_size the requested formal HPACK table size limit
 * @return pointer to the newly created context on success,
 *         NULL on allocation failure
 */
static mhd_FN_RET_UNALIASED
struct mhd_HpackDTblContext *
mhd_dtbl_create (size_t dyn_table_size)
{
  struct mhd_HpackDTblContext*dyn;
  dtbl_size_ft alloc_size;
  mhd_assert (mhd_DTBL_MAX_SIZE >= dyn_table_size);
  mhd_assert (mhd_DTBL_VALUE_FITS (dyn_table_size));

  alloc_size = dtbl_calc_alloc_size ((dtbl_size_ft) dyn_table_size);

  dyn = (struct mhd_HpackDTblContext*) malloc (sizeof(*dyn)
                                               + (size_t) alloc_size);
  if (NULL == dyn)
    return NULL; /* Failure exit point */

  dyn->buf_alloc_size = (dtbl_size_t) alloc_size;
  dyn->size_limit = (dtbl_size_t) dyn_table_size;
  dtbl_reset (dyn);

  dtbl_check_internals (dyn);

  return dyn;
}


/**
 * Destroy a dynamic HPACK table context and free all associated memory.
 *
 * @param dyn the pointer to the dynamic table structure to destroy
 */
mhd_static_inline MHD_FN_PAR_NONNULL_ALL_ void
mhd_dtbl_destroy (struct mhd_HpackDTblContext *dyn)
{
  dtbl_check_internals (dyn);
  /* Everything is in a single memory allocation, just free it */
  free (dyn);
}


/**
 * Get the current formal maximum table size (the HPACK size limit).
 * @param dyn the pointer to the dynamic table structure
 * @return the formal maximum size of the table
 */
static MHD_FN_PURE_ size_t
mhd_dtbl_get_table_max_size (const struct mhd_HpackDTblContext *dyn)
{
  return (size_t) dtbl_get_size_max_formal (dyn);
}


/**
 * Get the current amount of formal used space in the table.
 * @param dyn the pointer to the dynamic table structure
 * @return the formal used space in the table
 */
static MHD_FN_PURE_ size_t
mhd_dtbl_get_table_used (const struct mhd_HpackDTblContext *dyn)
{
  return (size_t) dtbl_get_used_formal (dyn);
}


/**
 * Get the current number of entries in the table.
 * @param dyn the pointer to the dynamic table structure
 * @return the number of entries in the table
 */
static MHD_FN_PURE_ size_t
mhd_dtbl_get_num_entries (const struct mhd_HpackDTblContext *dyn)
{
  return (size_t) dtbl_get_num_entries (dyn);
}


/**
 * Evict the oldest dynamic-table entries until the formal (HPACK) used size
 * becomes less than or equal to the requested value.
 *
 * If the table is already within the limit, nothing is changed.
 *
 * The function does not change the formal maximum table size and does not
 * allocate memory.
 *
 * @param dyn the pointer to the dynamic table structure
 * @param max_used_formal the target upper bound (in bytes) for the formal
 *                        used size after eviction
 */
static void
mhd_dtbl_evict_to_size (struct mhd_HpackDTblContext *dyn,
                        size_t max_used_formal)
{
  if (dtbl_is_empty (dyn))
    return;
  else if (0u == max_used_formal)
    dtbl_reset (dyn);
  else if (dtbl_get_used_formal (dyn) <= max_used_formal)
    return;
  else
    dtbl_evict_to_size (dyn,
                        (dtbl_size_t) max_used_formal);

  dtbl_check_internals (dyn);
}


/**
 * Resize the dynamic HPACK table.
 *
 * On allocation failure when growing, the original table is unchanged.
 * The shrinking of the table never fails.
 *
 * @param dyn_pp the pointer to the variable holding the pointer dynamic
 *               table structure, the value of the variable could be updated
 * @param dyn_table_size the new formal HPACK table size limit
 * @return 'true' on success (the variable pointer by @a dyn_pp could be
 *         updated),
 *         'false' if growing failed (the dynamic table remains valid, but
 *         not resized)
 */
static bool
mhd_dtbl_resize (struct mhd_HpackDTblContext** const dyn_pp,
                 size_t dyn_table_size)
{
  const dtbl_size_ft old_official_size = dtbl_get_size_max_formal (*dyn_pp);
  dtbl_size_ft new_alloc_size;
  struct mhd_HpackDTblContext *new_dyn;
  mhd_assert (mhd_DTBL_MAX_SIZE >= dyn_table_size);
  mhd_assert (mhd_DTBL_VALUE_FITS (dyn_table_size));

  if (old_official_size == dyn_table_size)
    return true; /* Do nothing */

  new_alloc_size = dtbl_calc_alloc_size ((dtbl_size_ft) dyn_table_size);

  if (old_official_size < dyn_table_size)
  {
    /* Growing table size */
    /* No need to evict */
    new_dyn = (struct mhd_HpackDTblContext*)
              realloc (*dyn_pp,
                       sizeof(**dyn_pp) + (size_t) new_alloc_size);
    if (NULL == new_dyn)
      return false; /* No table resize */
    *dyn_pp = new_dyn;

    /* Adapt the table data to the larger size */
    dtbl_adapt_to_new_size (new_dyn,
                            new_alloc_size,
                            (dtbl_size_ft) dyn_table_size);
  }
  else
  {
    /* Shrinking table size */
    mhd_dtbl_evict_to_size (*dyn_pp,
                            (dtbl_size_ft) dyn_table_size);

    /* Adapt table data before resizing */
    dtbl_adapt_to_new_size (*dyn_pp,
                            new_alloc_size,
                            (dtbl_size_ft) dyn_table_size);

    /* Try to reduce the allocated memory */
    new_dyn = (struct mhd_HpackDTblContext*)
              realloc (*dyn_pp,
                       sizeof(**dyn_pp) + (size_t) new_alloc_size);

    /* If realloc() failed, just use the previous allocation.
       The table will use the new (reduced) size anyway, while the allocation
       will be kept larger than needed. */
    if (mhd_COND_VIRTUALLY_ALWAYS (NULL != new_dyn))
      *dyn_pp = new_dyn;
  }

  dtbl_check_internals (new_dyn);

  return true;
}


/**
 * Check whether the new entry may fit the dynamic table
 * @param dyn the pointer to the dynamic table structure
 * @param name_len the length of the name of the new entry
 * @param val_len the length of the value of the new entry
 * @return 'true' if the new entry may be stored in the @a dyn dynamic table,
 *         'false' if the new entry formal size is larger than @a dyn may hold.
 */
static bool
mhd_dtbl_check_entry_fit (struct mhd_HpackDTblContext *restrict dyn,
                          size_t name_len,
                          size_t val_len)
{
  size_t entry_size;
  /* Carefully check the values, taking into account possible type overflow
     when performing calculations */
  entry_size = name_len + val_len;
  if (mhd_COND_HARDLY_EVER (entry_size < val_len))
    return false;
  entry_size += mhd_dtbl_entry_overhead;
  if (mhd_COND_HARDLY_EVER (entry_size < mhd_dtbl_entry_overhead))
    return false;

  return (dtbl_get_size_max_formal (dyn) >= entry_size);
}


/**
 * Add a new entry to the dynamic table.
 *
 * If the entry cannot fit the table size limit, the table is reset to the
 * empty state and the entry is discarded.
 * If there is enough formal free space, the entry is inserted. Otherwise, the
 * oldest entries are evicted and the new entry is inserted.
 *
 * The function copies the provided strings into the table's buffer.
 * @param dyn the pointer to the dynamic table structure
 * @param name_len the length of the @a name, must fit #mhd_HPACK_DTBL_BITS bits
 * @param name the name of the header, does NOT need to be zero-terminated
 * @param val_len the length of the @a val, must fit #mhd_HPACK_DTBL_BITS bits
 * @param val the value of the header, does NOT need to be zero terminated
 */
static MHD_FN_PAR_IN_SIZE_ (3,2) MHD_FN_PAR_IN_SIZE_ (5,4) void
mhd_dtbl_new_entry (struct mhd_HpackDTblContext *restrict dyn,
                    size_t name_len,
                    const char *restrict name,
                    size_t val_len,
                    const char *restrict val)
{
  if (mhd_COND_ALMOST_NEVER (! mhd_dtbl_check_entry_fit (dyn,
                                                         name_len,
                                                         val_len)))
  {
    /* The entry cannot fit the table.
     * Reset table to empty state (need to evict all entries). */
    dtbl_reset (dyn);

  }
  else if (dtbl_get_free_formal (dyn)
           >= dtbl_new_entry_size_formal ((dtbl_size_ft) name_len,
                                          (dtbl_size_ft) val_len))
  {
    /* Enough space. Insert new entry. */
    mhd_assert (mhd_DTBL_VALUE_FITS (name_len));
    mhd_assert (mhd_DTBL_VALUE_FITS (val_len));
    dtbl_extend_with_entry (dyn,
                            (dtbl_size_ft) name_len,
                            name,
                            (dtbl_size_ft) val_len,
                            val);
  }
  else
  {
    /* Not enough free space, but the new entry fit the table after eviction.
     * Evict some entries and add a new one. */
    mhd_assert (mhd_DTBL_VALUE_FITS (name_len));
    mhd_assert (mhd_DTBL_VALUE_FITS (val_len));
    dtbl_evict_add_entry (dyn,
                          (dtbl_size_ft) name_len,
                          name,
                          (dtbl_size_ft) val_len,
                          val);
  }

  dtbl_check_internals (dyn);
}


/**
 * Get a dynamic-table entry by HPACK index.
 *
 * The HPACK index must refer to the dynamic table (greater than the number
 * of entries in the static table). On success, the function returns pointers
 * to the non-zero-terminated name and value buffers inside the table and
 * their lengths.
 *
 * The strings returned (on success) in @a name_out and @a value_out must be
 * used/processed before any other actions with the dynamic table. Any change
 * in the dynamic table may invalidate pointers in @a name_out and
 * @a value_out.
 *
 * Behaviour is undefined if @a idx is less or equal to #mhd_HPACK_STBL_LAST_IDX
 *
 * @param dyn const pointer to the dynamic table structure
 * @param idx the HPACK index of the requested entry, must be strictly larger
 *            than #mhd_HPACK_STBL_LAST_IDX
 * @param[out] name_out the output buffer for the header name,
 *                      the result is NOT zero-terminated
 * @param[out] value_out the output buffer for the header value,
 *                       the result is NOT zero-terminated
 * @return 'true' if the entry exists and output buffers are set,
 *         'false' otherwise
 */
static MHD_FN_PAR_OUT_ (3) MHD_FN_PAR_OUT_ (4) bool
mhd_dtbl_get_entry (const struct mhd_HpackDTblContext *restrict dyn,
                    dtbl_idx_ft idx,
                    struct mhd_BufferConst *restrict name_out,
                    struct mhd_BufferConst *restrict value_out)
{
  const struct mhd_HpackDTblEntryInfo *entry;
  mhd_assert (mhd_HPACK_STBL_LAST_IDX < idx);
  if (dtbl_is_empty (dyn))
    return false;
  if (dtbl_get_pos_edge (dyn) < (idx - mhd_dtbl_hpack_idx_offset))
    return false;

  entry = dtbl_pos_entry_infoc (dyn,
                                dtbl_get_pos_from_hpack_idx (dyn,
                                                             idx));
  name_out->size = (size_t) entry->name_len;
  name_out->data = dtbl_entr_strs_ptr_startc (dyn,
                                              entry);
  value_out->size = (size_t) entry->val_len;
  value_out->data = name_out->data + name_out->size;

  return true;
}


/**
 * Look up a dynamic-table entry equal to the provided name and value.
 *
 * If the table is empty or no exact match is found, 0 is returned.
 * The input strings do not need to be zero-terminated.
 *
 * @param dyn const pointer to the dynamic table structure
 * @param name_len length of @a name in bytes
 * @param name pointer to the header field name,
 *             does NOT need to be zero-terminated
 * @param val_len length of @a val in bytes
 * @param val pointer to the header field value,
 *            does NOT need to be zero-terminated
 * @return the HPACK index (> #mhd_HPACK_STBL_LAST_IDX) of the matching entry,
 *         or 0 if not found
 */
static MHD_FN_PAR_IN_SIZE_ (3,2) MHD_FN_PAR_IN_SIZE_ (5,4) dtbl_idx_t
mhd_dtbl_find_entry (const struct mhd_HpackDTblContext *restrict dyn,
                     size_t name_len,
                     const char *restrict name,
                     size_t val_len,
                     const char *restrict val)
{
  if (dtbl_is_empty (dyn))
    return 0u;

  if (mhd_COND_HARDLY_EVER (! mhd_DTBL_VALUE_FITS (name_len)))
    return 0u;
  if (mhd_COND_HARDLY_EVER (! mhd_DTBL_VALUE_FITS (val_len)))
    return 0u;

  return dtbl_find_entry (dyn,
                          (dtbl_size_ft) name_len,
                          name,
                          (dtbl_size_ft) val_len,
                          val);
}


/**
 * Look up a dynamic-table entry whose name equals @a name.
 *
 * If the table is empty or no match is found, 0 is returned.
 * The input string does not need to be zero-terminated.
 *
 * @param dyn const pointer to the dynamic table structure
 * @param name_len length of @a name in bytes
 * @param name pointer to the header field name,
 *             does NOT need to be zero-terminated
 * @return the HPACK index (> #mhd_HPACK_STBL_LAST_IDX) of the matching entry,
 *         or 0 if not found
 */
static MHD_FN_PAR_IN_SIZE_ (3,2) dtbl_idx_t
mhd_dtbl_find_name (const struct mhd_HpackDTblContext *restrict dyn,
                    size_t name_len,
                    const char *restrict name)
{
  if (dtbl_is_empty (dyn))
    return 0u;

  if (mhd_COND_HARDLY_EVER (! mhd_DTBL_VALUE_FITS (name_len)))
    return 0u;

  return dtbl_find_name (dyn,
                         (dtbl_size_ft) name_len,
                         name);
}


/* ****** ----------------- Static table handling ----------------- ****** */
/* ========================================================================
 *
 *  The static table data should be accessed only by mhd_* functions.
 *
 *  All functions prefixed with stbl_* are internal helpers and should not
 *  be used directly.
 *
 * ========================================================================
 */

/**
 * HPACK static table element
 */
struct mhd_HpackStaticEntry
{
  /**
   * The name of the header field
   */
  const struct MHD_String name;
  /**
   * The value of the header field.
   */
  const struct MHD_String value;
};

/* The next variable cannot be declared as 'mhd_constexpr' as it contains
   pointers to the strings */
/**
 * HPACK static table.
 * Add 1 to the array index to obtain the HPACK index.
 *
 * This table is extracted (and transformed) from RFC 7541.
 * See https://datatracker.ietf.org/doc/html/rfc7541#appendix-A
 */
static const struct mhd_HpackStaticEntry
  mhd_hpack_static[mhd_HPACK_STBL_ENTRIES] = {
  /* 1  */ { mhd_MSTR_INIT (":authority"), mhd_MSTR_INIT ("") },
  /* 2  */ { mhd_MSTR_INIT (":method"), mhd_MSTR_INIT ("GET") },
  /* 3  */ { mhd_MSTR_INIT (":method"), mhd_MSTR_INIT ("POST") },
  /* 4  */ { mhd_MSTR_INIT (":path"), mhd_MSTR_INIT ("/") },
  /* 5  */ { mhd_MSTR_INIT (":path"), mhd_MSTR_INIT ("/index.html") },
  /* 6  */ { mhd_MSTR_INIT (":scheme"), mhd_MSTR_INIT ("http") },
  /* 7  */ { mhd_MSTR_INIT (":scheme"), mhd_MSTR_INIT ("https") },
  /* 8  */ { mhd_MSTR_INIT (":status"), mhd_MSTR_INIT ("200") },
  /* 9  */ { mhd_MSTR_INIT (":status"), mhd_MSTR_INIT ("204") },
  /* 10 */ { mhd_MSTR_INIT (":status"), mhd_MSTR_INIT ("206") },
  /* 11 */ { mhd_MSTR_INIT (":status"), mhd_MSTR_INIT ("304") },
  /* 12 */ { mhd_MSTR_INIT (":status"), mhd_MSTR_INIT ("400") },
  /* 13 */ { mhd_MSTR_INIT (":status"), mhd_MSTR_INIT ("404") },
  /* 14 */ { mhd_MSTR_INIT (":status"), mhd_MSTR_INIT ("500") },
  /* 15 */ { mhd_MSTR_INIT ("accept-charset"), mhd_MSTR_INIT ("") },
  /* 16 */ { mhd_MSTR_INIT ("accept-encoding"),
             mhd_MSTR_INIT ("gzip, deflate") },
  /* 17 */ { mhd_MSTR_INIT ("accept-language"), mhd_MSTR_INIT ("") },
  /* 18 */ { mhd_MSTR_INIT ("accept-ranges"), mhd_MSTR_INIT ("") },
  /* 19 */ { mhd_MSTR_INIT ("accept"), mhd_MSTR_INIT ("") },
  /* 20 */ { mhd_MSTR_INIT ("access-control-allow-origin"),
             mhd_MSTR_INIT ("") },
  /* 21 */ { mhd_MSTR_INIT ("age"), mhd_MSTR_INIT ("") },
  /* 22 */ { mhd_MSTR_INIT ("allow"), mhd_MSTR_INIT ("") },
  /* 23 */ { mhd_MSTR_INIT ("authorization"), mhd_MSTR_INIT ("") },
  /* 24 */ { mhd_MSTR_INIT ("cache-control"), mhd_MSTR_INIT ("") },
  /* 25 */ { mhd_MSTR_INIT ("content-disposition"), mhd_MSTR_INIT ("") },
  /* 26 */ { mhd_MSTR_INIT ("content-encoding"), mhd_MSTR_INIT ("") },
  /* 27 */ { mhd_MSTR_INIT ("content-language"), mhd_MSTR_INIT ("") },
  /* 28 */ { mhd_MSTR_INIT ("content-length"), mhd_MSTR_INIT ("") },
  /* 29 */ { mhd_MSTR_INIT ("content-location"), mhd_MSTR_INIT ("") },
  /* 30 */ { mhd_MSTR_INIT ("content-range"), mhd_MSTR_INIT ("") },
  /* 31 */ { mhd_MSTR_INIT ("content-type"), mhd_MSTR_INIT ("") },
  /* 32 */ { mhd_MSTR_INIT ("cookie"), mhd_MSTR_INIT ("") },
  /* 33 */ { mhd_MSTR_INIT ("date"), mhd_MSTR_INIT ("") },
  /* 34 */ { mhd_MSTR_INIT ("etag"), mhd_MSTR_INIT ("") },
  /* 35 */ { mhd_MSTR_INIT ("expect"), mhd_MSTR_INIT ("") },
  /* 36 */ { mhd_MSTR_INIT ("expires"), mhd_MSTR_INIT ("") },
  /* 37 */ { mhd_MSTR_INIT ("from"), mhd_MSTR_INIT ("") },
  /* 38 */ { mhd_MSTR_INIT ("host"), mhd_MSTR_INIT ("") },
  /* 39 */ { mhd_MSTR_INIT ("if-match"), mhd_MSTR_INIT ("") },
  /* 40 */ { mhd_MSTR_INIT ("if-modified-since"), mhd_MSTR_INIT ("") },
  /* 41 */ { mhd_MSTR_INIT ("if-none-match"), mhd_MSTR_INIT ("") },
  /* 42 */ { mhd_MSTR_INIT ("if-range"), mhd_MSTR_INIT ("") },
  /* 43 */ { mhd_MSTR_INIT ("if-unmodified-since"), mhd_MSTR_INIT ("") },
  /* 44 */ { mhd_MSTR_INIT ("last-modified"), mhd_MSTR_INIT ("") },
  /* 45 */ { mhd_MSTR_INIT ("link"), mhd_MSTR_INIT ("") },
  /* 46 */ { mhd_MSTR_INIT ("location"), mhd_MSTR_INIT ("") },
  /* 47 */ { mhd_MSTR_INIT ("max-forwards"), mhd_MSTR_INIT ("") },
  /* 48 */ { mhd_MSTR_INIT ("proxy-authenticate"), mhd_MSTR_INIT ("") },
  /* 49 */ { mhd_MSTR_INIT ("proxy-authorization"), mhd_MSTR_INIT ("") },
  /* 50 */ { mhd_MSTR_INIT ("range"), mhd_MSTR_INIT ("") },
  /* 51 */ { mhd_MSTR_INIT ("referer"), mhd_MSTR_INIT ("") },
  /* 52 */ { mhd_MSTR_INIT ("refresh"), mhd_MSTR_INIT ("") },
  /* 53 */ { mhd_MSTR_INIT ("retry-after"), mhd_MSTR_INIT ("") },
  /* 54 */ { mhd_MSTR_INIT ("server"), mhd_MSTR_INIT ("") },
  /* 55 */ { mhd_MSTR_INIT ("set-cookie"), mhd_MSTR_INIT ("") },
  /* 56 */ { mhd_MSTR_INIT ("strict-transport-security"), mhd_MSTR_INIT ("") },
  /* 57 */ { mhd_MSTR_INIT ("transfer-encoding"), mhd_MSTR_INIT ("") },
  /* 58 */ { mhd_MSTR_INIT ("user-agent"), mhd_MSTR_INIT ("") },
  /* 59 */ { mhd_MSTR_INIT ("vary"), mhd_MSTR_INIT ("") },
  /* 60 */ { mhd_MSTR_INIT ("via"), mhd_MSTR_INIT ("") },
  /* 61 */ { mhd_MSTR_INIT ("www-authenticate"), mhd_MSTR_INIT ("") }
};

/**
 * The position of the first ":status" pseud-header field in the
 * @a mhd_hpack_static table
 */
#define mhd_HPACK_STBL_PF_STATUS_START_POS         (8u)

/**
 * Convert an HPACK index (matching the static table) to a 0-based position in
 * the static table data.
 *
 * Behaviour is undefined if @a hpack_idx is 0 or greater than
 * #mhd_HPACK_STBL_LAST_IDX.
 * @param hpack_idx the HPACK index of the static-table entry
 *                  (1 .. #mhd_HPACK_STBL_LAST_IDX)
 * @return the 0-based position corresponding to @a hpack_idx
 */
MHD_FN_CONST_ mhd_static_inline dtbl_idx_t
stbl_get_pos_from_hpack_idx (dtbl_idx_ft hpack_idx)
{
  mhd_assert (0u != hpack_idx);
  mhd_assert (mhd_HPACK_STBL_LAST_IDX >= hpack_idx);
  return (dtbl_idx_t) (hpack_idx - 1u);
}


/**
 * Convert a 0-based static table position to the HPACK index.
 *
 * The returned index is in the range 1 .. #mhd_HPACK_STBL_LAST_IDX.
 *
 * Behaviour is undefined if @a loc_pos is not a valid static-table position,
 * i.e. if it is greater than or equal to #mhd_HPACK_STBL_ENTRIES.
 * @param loc_pos the 0-based position in the static table
 * @return the HPACK index corresponding to @a loc_pos
 */
MHD_FN_CONST_ mhd_static_inline dtbl_idx_t
stbl_get_hpack_idx_from_pos (dtbl_idx_ft loc_pos)
{
  mhd_assert (mhd_HPACK_STBL_LAST_IDX > loc_pos);
  return (dtbl_idx_t) (loc_pos + 1u);
}


/**
 * Get a pointer to the static table entry by its 0-based position.
 *
 * Behaviour is undefined if @a loc_pos is not a valid static-table position,
 * i.e. if it is greater than or equal to #mhd_HPACK_STBL_ENTRIES.
 * @param loc_pos the 0-based position in the static table
 * @return const pointer to the static entry descriptor
 */
MHD_FN_CONST_ mhd_static_inline const struct mhd_HpackStaticEntry *
stbl_pos_entry_info (dtbl_idx_ft loc_pos)
{
  mhd_assert (sizeof(mhd_hpack_static) / sizeof(mhd_hpack_static[0]) \
              == mhd_HPACK_STBL_ENTRIES);
  mhd_assert (mhd_HPACK_STBL_ENTRIES > loc_pos);
  return mhd_hpack_static + loc_pos;
}


/**
 * Get a pointer to the static table entry by its HPACK index.
 *
 * Behaviour is undefined if @a hpack_idx is 0 or greater than
 * #mhd_HPACK_STBL_LAST_IDX.
 * @param hpack_idx the HPACK index of the entry
 * @return const pointer to the static entry descriptor
 */
MHD_FN_CONST_ mhd_static_inline const struct mhd_HpackStaticEntry *
stbl_idx_entry_info (dtbl_idx_ft hpack_idx)
{
  return stbl_pos_entry_info (stbl_get_pos_from_hpack_idx (hpack_idx));
}


/* **** _____________ End of static table data helpers ______________ ****** */

/* ****------------------- Static table data API ---------------------****** */
/**
 * The position of the first real (non-pseudo) header in the
 * @a mhd_hpack_static table
 */
#define mhd_HPACK_STBL_NORM_START_POS         (14u)

/**
 * The position of the only real (non-pseudo) header with a non-empty value in
 * the @a mhd_hpack_static table
 */
#define mhd_HPACK_STBL_NORM_WITH_VALUE_POS         (15u)

/**
 * Get a static-table entry by HPACK index.
 *
 * The index @a idx must refer to the static table
 * (i.e. 1 .. #mhd_HPACK_STBL_LAST_IDX).
 * On return, @a name_out and @a value_out are set to point to the entry
 * data and their lengths.
 *
 * Behaviour is undefined if @a idx is 0 or greater
 * than #mhd_HPACK_STBL_LAST_IDX.
 * @param idx the HPACK index within the static table
 * @param[out] name_out output buffer for the header name
 * @param[out] value_out output buffer for the header value
 */
static MHD_FN_PAR_OUT_ (2) MHD_FN_PAR_OUT_ (3) void
mhd_stbl_get_entry (dtbl_idx_ft idx,
                    struct mhd_BufferConst *restrict name_out,
                    struct mhd_BufferConst *restrict value_out)
{
  const struct mhd_HpackStaticEntry *const entry = stbl_idx_entry_info (idx);

  name_out->size = entry->name.len;
  name_out->data = entry->name.cstr;
  value_out->size = entry->value.len;
  value_out->data = entry->value.cstr;
}


/**
 * Find a static-table entry among "real" (non-pseudo) headers that exactly
 * matches the given name and value.
 *
 * The header name must not start with ':'.
 * The input strings do not need to be zero-terminated.
 *
 * @param name_len length of @a name in bytes,
 *                 must not be zero
 * @param name pointer to the header field name,
 *             does NOT need to be zero-terminated
 * @param val_len length of @a val in bytes
 * @param val pointer to the header field value,
 *            does NOT need to be zero-terminated
 * @return the HPACK index (<= #mhd_HPACK_STBL_LAST_IDX) of the matching
 *         static entry, or 0 if not found
 */
static MHD_FN_PAR_IN_SIZE_ (2,1) MHD_FN_PAR_IN_SIZE_ (4,3) dtbl_idx_t
mhd_stbl_find_entry_real (size_t name_len,
                          const char *restrict name,
                          size_t val_len,
                          const char *restrict val)
{
#ifndef MHD_UNIT_TESTING /* Do not abort on a wrong name when unit-testing */
  mhd_assert (0u != name_len);
  mhd_assert (':' != name[0]);
#endif /* ! MHD_UNIT_TESTING */
#ifndef MHD_FAVOR_SMALL_CODE
  if (mhd_COND_ALMOST_ALWAYS (0u != val_len))
  { /* non-empty 'value' */
    /* Process the only normal (real header) entry that has non-empty value */
    mhd_constexpr dtbl_idx_ft i = mhd_HPACK_STBL_NORM_WITH_VALUE_POS;
    do
    {
      const struct mhd_HpackStaticEntry *const entry = stbl_pos_entry_info (i);

      if (name_len != entry->name.len)
        continue;
      mhd_assert (0u != entry->name.len);
      mhd_assert (0u != entry->value.len);

      if (0 == memcmp (name,
                       entry->name.cstr,
                       name_len))
      { /* 'name' matches */
        if (0 == memcmp (val,
                         entry->value.cstr,
                         val_len))
        { /* 'value' matches */
          /* Full match found, return the HPACK index */
          return stbl_get_hpack_idx_from_pos (i);
        }
      }


    } while (0);
  }
  else
  { /* (0u == val_len) */
    /* empty 'value' */
    dtbl_idx_ft i;
    mhd_assert (0u == val_len);
    for (i = mhd_HPACK_STBL_NORM_START_POS; i < mhd_HPACK_STBL_ENTRIES; ++i)
    {
      const struct mhd_HpackStaticEntry *const entry = stbl_pos_entry_info (i);

      if (mhd_HPACK_STBL_NORM_WITH_VALUE_POS == i)
        continue;

      if (name_len != entry->name.len)
        continue;
      mhd_assert (0u != entry->name.len);
      mhd_assert (0u == entry->value.len);
      if (0 == memcmp (name,
                       entry->name.cstr,
                       name_len))
      { /* 'name' matches (and 'value' is empty) */
        /* Full match found, return the HPACK index */
        return stbl_get_hpack_idx_from_pos (i);
      }
    }
  }
#else  /* ! MHD_FAVOR_SMALL_CODE */
  if (1)
  {
    dtbl_idx_ft i;
    for (i = mhd_HPACK_STBL_NORM_START_POS; i < mhd_HPACK_STBL_ENTRIES; ++i)
    {
      const struct mhd_HpackStaticEntry *const entry = stbl_pos_entry_info (i);
      if (name_len != entry->name.len)
        continue;
      if (val_len != entry->value.len)
        continue;
      mhd_assert (0u != entry->name.len);
      if (0 == memcmp (name,
                       entry->name.cstr,
                       name_len))
      { /* 'name' matches */
        if ((0u == val_len) ||
            (0 == memcmp (val,
                          entry->value.cstr,
                          val_len)))
        { /* 'value' matches (empty or identical) */
          /* Full match found, return the HPACK index */
          return stbl_get_hpack_idx_from_pos (i);
        }
      }
    }
  }
#endif /* !MHD_FAVOR_SMALL_CODE */

  return 0u; /* Not found */
}


/**
 * Find a static-table entry among "real" (non-pseudo) headers whose name
 * exactly matches @a name.
 *
 * The header name must not start with ':'.
 * The input string does not need to be zero-terminated.
 *
 * @param name_len length of @a name in bytes,
 *                 must NOT be zero
 * @param name pointer to the header field name,
 *             does NOT need to be zero-terminated
 * @return the HPACK index (<= #mhd_HPACK_STBL_LAST_IDX) of the matching
 *         static entry, or 0 if not found
 */
static MHD_FN_PAR_IN_SIZE_ (2,1) dtbl_idx_t
mhd_stbl_find_name_real (size_t name_len,
                         const char *restrict name)
{
  dtbl_idx_ft i;
#ifndef MHD_UNIT_TESTING /* Do not abort on a wrong name when unit-testing */
  mhd_assert (0u != name_len);
  mhd_assert (':' != name[0]);
#endif /* ! MHD_UNIT_TESTING */
  for (i = mhd_HPACK_STBL_NORM_START_POS; i < mhd_HPACK_STBL_ENTRIES; ++i)
  {
    const struct mhd_HpackStaticEntry *const entry = stbl_pos_entry_info (i);

    if (name_len != entry->name.len)
      continue;
    mhd_assert (0u != entry->name.len);
    if (0 == memcmp (name,
                     entry->name.cstr,
                     name_len))
    { /* Found the entry, return the HPACK index */
      return stbl_get_hpack_idx_from_pos (i);
    }
  }

  return 0u; /* Not found */
}


/* ****** -------------- HPACK header tables handling -------------- ****** */
/*
 * mhd_htbl_ functions are handling combination of HPACK static and dynamic
 * tables.
 * Functions need a pointer to a dynamic table instance.
 *
 * These functions are just convenient wrappers for some operations; they are
 * not designed to cover all operations with static and dynamic tables.
 * Some operations must be performed directly on static or dynamic tables.
 */
/**
 * Get a header-table entry (static or dynamic) by HPACK index.
 *
 * On success, @a name_out and @a value_out are set to point to the entry
 * data and their lengths. The returned buffers are not guaranteed to be
 * zero-terminated and must not be relied upon as C strings.
 *
 * @param dyn const pointer to the dynamic table context
 * @param idx the HPACK index (static or dynamic)
 * @param[out] name_out output buffer for the header name
 * @param[out] value_out output buffer for the header value
 * @return 'true' if the entry exists and outputs are set,
 *         'false' otherwise
 */
static MHD_FN_PAR_OUT_ (3) MHD_FN_PAR_OUT_ (4) bool
mhd_htbl_get_entry (const struct mhd_HpackDTblContext *restrict dyn,
                    dtbl_idx_ft idx,
                    struct mhd_BufferConst *restrict name_out,
                    struct mhd_BufferConst *restrict value_out)
{
  if (mhd_COND_HARDLY_EVER (0u == idx))
    return false;
  if (mhd_HPACK_STBL_LAST_IDX >= idx)
  {
    mhd_stbl_get_entry (idx,
                        name_out,
                        value_out);
    return true;
  }

  return mhd_dtbl_get_entry (dyn,
                             idx,
                             name_out,
                             value_out);
}


/**
 * Look up a header-table entry (static "real" headers first, then dynamic)
 * that exactly matches the given name and value.
 *
 * Pseudo-headers (names starting with ':') are not searched. The input
 * strings do not need to be zero-terminated.
 *
 * @param dyn const pointer to the dynamic table context
 * @param name_len length of @a name in bytes
 * @param name pointer to the header field name, must not start with ':',
 *             does NOT need to be zero-terminated
 * @param val_len length of @a val in bytes
 * @param val pointer to the header field value,
 *            does NOT need to be zero-terminated
 * @return the HPACK index of the matching entry (either static or dynamic),
 *         or 0 if not found
 */
static MHD_FN_PAR_IN_SIZE_ (3,2) MHD_FN_PAR_IN_SIZE_ (5,4) dtbl_idx_t
mhd_htbl_find_entry_real (const struct mhd_HpackDTblContext *restrict dyn,
                          size_t name_len,
                          const char *restrict name,
                          size_t val_len,
                          const char *restrict val)
{
  dtbl_idx_ft idx;
#ifndef MHD_UNIT_TESTING /* Do not abort on a wrong name when unit-testing */
  mhd_assert ((0u == name_len) || (':' != name[0]));
#endif /* ! MHD_UNIT_TESTING */

  if (0u != name_len)
    idx = mhd_stbl_find_entry_real (name_len,
                                    name,
                                    val_len,
                                    val);
  else
    idx = 0u;

  if (0u == idx)
    idx = mhd_dtbl_find_entry (dyn,
                               name_len,
                               name,
                               val_len,
                               val);

  return (dtbl_idx_t) idx;
}


/**
 * Look up a header-table entry (static "real" headers first, then dynamic)
 * whose name exactly matches @a name.
 *
 * Pseudo-headers (names starting with ':') are not searched. The input
 * string does not need to be zero-terminated.
 *
 * @param dyn const pointer to the dynamic table context
 * @param name_len length of @a name in bytes
 * @param name pointer to the header field name, must not start with ':',
 *             does NOT need to be zero-terminated
 * @return the HPACK index of the matching entry (either static or dynamic),
 *         or 0 if not found
 */
static MHD_FN_PAR_IN_SIZE_ (3,2) dtbl_idx_t
mhd_htbl_find_name_real (const struct mhd_HpackDTblContext *restrict dyn,
                         size_t name_len,
                         const char *restrict name)
{
  dtbl_idx_ft idx;
#ifndef MHD_UNIT_TESTING /* Do not abort on a wrong name when unit-testing */
  mhd_assert ((0u == name_len) || (':' != name[0]));
#endif /* ! MHD_UNIT_TESTING */

  if (0u != name_len)
    idx = mhd_stbl_find_name_real (name_len,
                                   name);
  else
    idx = 0u;

  if (0u == idx)
    idx = mhd_dtbl_find_name (dyn,
                              name_len,
                              name);

  return (dtbl_idx_t) idx;
}


/* **** ___________ End of HPACK header tables handling ____________ ****** */

/**
 * H2 HPACK default maximum size of the dynamic table
 */
mhd_constexpr size_t mhd_hpack_def_dyn_table_size = 4096u;

#if ! defined(mhd_HPACK_TESTING_TABLES_ONLY) || ! defined(MHD_UNIT_TESTING)

/**
 * Exactly eight bits all set (to one).
 */
mhd_constexpr uint8_t b8ones = 0xFFu;

/**
 * The maximum number of bytes allowed to encode numbers in HPACK.
 *
 * Current implementation supports only 32-bit numbers for strings and indices,
 * but extra zeros at the end of the encoded numbers can be safely processed.
 * This value limits the number of extra zero bytes at the end to a reasonable
 * value. It is enough to process the output of some weak encoder which may
 * encode numbers always as 64-bit-long values with some extra zero bytes at
 * the end of the encoded form.
 */
mhd_constexpr uint_fast8_t mhd_hpack_num_max_bytes = 12u;

/* ****** ----------------- HPACK headers decoding ----------------- ****** */

/**
 * Result of hpack_dec_number()
 */
enum MHD_FIXED_ENUM_ mhd_HpackGetNumResult
{
  mhd_HPACK_GET_NUM_RES_NO_ERROR,  /**< Success */
  mhd_HPACK_GET_NUM_RES_INCOMPLETE,/**< Not enough data in the input buffer */
  mhd_HPACK_GET_NUM_RES_TOO_LARGE, /**< The decoded integer is too large for 32-bit */
  mhd_HPACK_GET_NUM_RES_TOO_LONG   /**< The tail of the encoded number has too many extra zero bytes */
};

/**
 * Decode an HPACK integer number from the input buffer using a prefix in
 * the first byte.
 * @param first_byte_prefix_bits number of prefix bits in the first byte (1..7)
 * @param buf_size the size of @a buf
 * @param buf the input buffer
 * @param[out] num_out where to store the decoded value (fits into 32-bit range)
 * @param[out] bytes_decoded where to store the number of decoded bytes
 * @return #mhd_HPACK_GET_NUM_RES_NO_ERROR on success,
 *         error code otherwise
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (3,2)
MHD_FN_PAR_OUT_ (4) MHD_FN_PAR_OUT_ (5) enum mhd_HpackGetNumResult
hpack_dec_number (uint_fast8_t first_byte_prefix_bits,
                  const size_t buf_size,
                  const uint8_t buf[MHD_FN_PAR_DYN_ARR_SIZE_ (buf_size)],
                  uint_fast32_t *restrict num_out,
                  size_t *restrict bytes_decoded)
{
  /** The maximum value of the first byte. Also the mask for the first byte. */
  const uint_fast8_t first_byte_val_max =
    (uint_fast8_t) (b8ones >> first_byte_prefix_bits);
  uint_fast8_t first_byte;
  uint_fast32_t dec_num;
  uint_fast8_t i;

  mhd_assert (0 != first_byte_prefix_bits);
  mhd_assert (8 > first_byte_prefix_bits);

  first_byte = (buf[0] & first_byte_val_max);
  if (first_byte_val_max != first_byte)
  {
    *num_out = (uint_fast32_t) first_byte;
    *bytes_decoded = 1u;
    return mhd_HPACK_GET_NUM_RES_NO_ERROR; /* Success exit point */
  }
  dec_num = first_byte;

#ifndef MHD_FAVOR_SMALL_CODE
  /* Unrolled loop */
  i = 1u;
  if (buf_size == i)
    return mhd_HPACK_GET_NUM_RES_INCOMPLETE; /* Failure exit point */
  else
  {
    const uint_fast8_t cur_byte = buf[i];
    const bool is_final = (0u == (cur_byte & 0x80u));
    const uint_fast8_t byte_val = (uint_fast8_t) (cur_byte & 0x7Fu);
    dec_num += (uint_fast32_t) (((uint_fast32_t) byte_val) << (7u * (i - 1u)));
    if (is_final)
    {
      *num_out = dec_num;
      *bytes_decoded = (size_t) (i + 1u);
      return mhd_HPACK_GET_NUM_RES_NO_ERROR; /* Success exit point */
    }
  }

  i = 2u;
  if (buf_size == i)
    return mhd_HPACK_GET_NUM_RES_INCOMPLETE; /* Failure exit point */
  else
  {
    const uint_fast8_t cur_byte = buf[i];
    const bool is_final = (0u == (cur_byte & 0x80u));
    const uint_fast8_t byte_val = (uint_fast8_t) (cur_byte & 0x7Fu);
    dec_num += (uint_fast32_t) (((uint_fast32_t) byte_val) << (7u * (i - 1u)));
    if (is_final)
    {
      *num_out = dec_num;
      *bytes_decoded = (size_t) (i + 1u);
      return mhd_HPACK_GET_NUM_RES_NO_ERROR; /* Success exit point */
    }
  }

  i = 3u;
  if (buf_size == i)
    return mhd_HPACK_GET_NUM_RES_INCOMPLETE; /* Failure exit point */
  else
  {
    const uint_fast8_t cur_byte = buf[i];
    const bool is_final = (0u == (cur_byte & 0x80u));
    const uint_fast8_t byte_val = (uint_fast8_t) (cur_byte & 0x7Fu);
    dec_num += (uint_fast32_t) (((uint_fast32_t) byte_val) << (7u * (i - 1u)));
    if (is_final)
    {
      *num_out = dec_num;
      *bytes_decoded = (size_t) (i + 1u);
      return mhd_HPACK_GET_NUM_RES_NO_ERROR; /* Success exit point */
    }
  }

  i = 4u;
  if (buf_size == i)
    return mhd_HPACK_GET_NUM_RES_INCOMPLETE; /* Failure exit point */
  else
  {
    const uint_fast8_t cur_byte = buf[i];
    const bool is_final = (0u == (cur_byte & 0x80u));
    const uint_fast8_t byte_val = (uint_fast8_t) (cur_byte & 0x7Fu);
    dec_num += (uint_fast32_t) (((uint_fast32_t) byte_val) << (7u * (i - 1u)));
    if (is_final)
    {
      *num_out = dec_num;
      *bytes_decoded = (size_t) (i + 1u);
      return mhd_HPACK_GET_NUM_RES_NO_ERROR; /* Success exit point */
    }
  }

  i = 5u;
#else  /* MHD_FAVOR_SMALL_CODE */
  /* First four bytes cannot overflow the output */
  for (i = 1u; 4u >= i; ++i)
  {
    if (buf_size == i)
      return mhd_HPACK_GET_NUM_RES_INCOMPLETE; /* Failure exit point */
    else
    {
      const uint_fast8_t cur_byte = buf[i];
      const bool is_final = (0u == (cur_byte & 0x80u));
      const uint_fast8_t byte_val = (uint_fast8_t) (cur_byte & 0x7Fu);
      dec_num += (uint_fast32_t) (((uint_fast32_t) byte_val) << (7u * (i - 1u)))
      ;
      if (is_final)
      {
        *num_out = dec_num;
        *bytes_decoded = (size_t) (i + 1u);
        return mhd_HPACK_GET_NUM_RES_NO_ERROR; /* Success exit point */
      }
    }
  }
#endif /* MHD_FAVOR_SMALL_CODE */

  mhd_assert (0u == (dec_num >> 29u));
  mhd_assert (5u == i);
  if (buf_size == i)
    return mhd_HPACK_GET_NUM_RES_INCOMPLETE; /* Failure exit point */
  else
  { /* Handle the fifth byte with overflow checks */
    const uint_fast8_t cur_byte = buf[i];
    const bool is_final = (0u == (cur_byte & 0x80u));
    const uint_fast8_t byte_val = (uint_fast8_t) (cur_byte & 0x7Fu);
    const uint_fast32_t add_val =
      (uint_fast32_t) (((uint_fast32_t) byte_val) << (7u * (i - 1u)));
    if (byte_val != ((add_val & 0xFFFFFFFFu) >> (7u * (i - 1u))))
      return mhd_HPACK_GET_NUM_RES_TOO_LARGE; /* Failure exit point */
    dec_num += add_val;
    if ((dec_num & 0xFFFFFFFFu) < add_val)
      return mhd_HPACK_GET_NUM_RES_TOO_LARGE; /* Failure exit point */
    else if (is_final)
    {
      *num_out = dec_num;
      *bytes_decoded = (size_t) (i + 1u);
      return mhd_HPACK_GET_NUM_RES_NO_ERROR; /* Success exit point */
    }
  }

  /* Process possible extra zero-valued tail bytes */
  while (++i <= mhd_hpack_num_max_bytes)
  {
    if (buf_size == i)
      return mhd_HPACK_GET_NUM_RES_INCOMPLETE; /* Failure exit point */
    else
    {
      const uint_fast8_t cur_byte = buf[i];
      const bool is_final = (0u == (cur_byte & 0x80u));
      const uint_fast8_t byte_val = (uint_fast8_t) (cur_byte & 0x7Fu);
      if (0u != byte_val)
        return mhd_HPACK_GET_NUM_RES_TOO_LARGE; /* Failure exit point */
      else if (is_final)
      {
        *num_out = dec_num;
        *bytes_decoded = (size_t) (i + 1u);
        return mhd_HPACK_GET_NUM_RES_NO_ERROR; /* Success exit point */
      }
    }
  }

  return mhd_HPACK_GET_NUM_RES_TOO_LONG; /* Failure exit point */
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_ (1) bool
mhd_hpack_dec_init (struct mhd_HpackDecContext *hk_dec)
{
  hk_dec->dyn = mhd_dtbl_create (mhd_hpack_def_dyn_table_size);

  if (NULL == hk_dec->dyn)
    return false; /* Failure exit point */

  mhd_assert (mhd_hpack_def_dyn_table_size == \
              mhd_dtbl_get_table_max_size (hk_dec->dyn));

  hk_dec->max_allowed_dyn_size = mhd_hpack_def_dyn_table_size;
  hk_dec->last_remote_dyn_size = hk_dec->max_allowed_dyn_size;

  return true; /* Success exit point */
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1) void
mhd_hpack_dec_deinit (struct mhd_HpackDecContext *hk_dec)
{
  if (NULL == hk_dec->dyn)
    return; /* Nothing to de-initialise */

  mhd_dtbl_destroy (hk_dec->dyn);
  hk_dec->dyn = NULL;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1) void
mhd_hpack_dec_set_allowed_dyn_size (struct mhd_HpackDecContext *hk_dec,
                                    size_t new_allowed_dyn_size)
{
  mhd_assert (mhd_DTBL_MAX_SIZE >= new_allowed_dyn_size);
  hk_dec->max_allowed_dyn_size = new_allowed_dyn_size;
}


/**
 * Ensure that any pending dynamic table resize is applied before decoding
 * fields.
 * Also check for possible missing Dynamic Table Size Update messages (after
 * reception of ACK for settings reducing the maximum table size).
 * @param hk_dec pointer to the decoder context
 * @return non-error decoder result on success;
 *         an error code if resize is disallowed or memory allocation fails
 */
static enum mhd_HpackDecResult
dec_check_resize_pending (struct mhd_HpackDecContext *restrict hk_dec)
{
  mhd_assert (mhd_DTBL_MAX_SIZE >= hk_dec->last_remote_dyn_size);
  if (hk_dec->max_allowed_dyn_size < hk_dec->last_remote_dyn_size)
    return mhd_HPACK_DEC_RES_DYN_SIZE_UPD_MISSING; /* Failure exit point */

  if (mhd_dtbl_get_table_max_size (hk_dec->dyn) != hk_dec->last_remote_dyn_size)
  {
    /* Resize must be performed before processing any headers data */
    if (! mhd_dtbl_resize (&(hk_dec->dyn),
                           hk_dec->last_remote_dyn_size))
      return mhd_HPACK_DEC_RES_ALLOC_ERR; /* Failure exit point */
  }

  mhd_assert (mhd_dtbl_get_table_max_size (hk_dec->dyn) \
              == hk_dec->last_remote_dyn_size);
  return mhd_HPACK_DEC_RES_NEW_FIELD; /* Success, return any non-error code */
}


/**
 * Decode an indexed header field and write "name\0value\0" to @a out_buff.
 * @param hk_dec the decoder context
 * @param enc_data_size the size of @a enc_data
 * @param enc_data the encoded data
 * @param out_buff_size the size of @a out_buff
 * @param[out] out_buff the output buffer for "name\0value\0"
 * @param[out] name_len set to the length of the name, not counting
 *                      terminating zero
 * @param[out] val_len set to the length of the value, not counting
 *                     terminating zero
 * @param[out] bytes_decoded set to the number of decoded bytes
 * @return #mhd_HPACK_DEC_RES_NEW_FIELD on success or an error code
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (3,2) MHD_FN_PAR_OUT_SIZE_ (5,4)
MHD_FN_PAR_OUT_ (6) MHD_FN_PAR_OUT_ (7)
MHD_FN_PAR_OUT_ (8) enum mhd_HpackDecResult
hpack_dec_field_indexed (struct mhd_HpackDecContext *restrict hk_dec,
                         size_t enc_data_size,
                         const uint8_t *restrict enc_data,
                         size_t out_buff_size,
                         char *restrict out_buff,
                         size_t *restrict name_len,
                         size_t *restrict val_len,
                         size_t *restrict bytes_decoded)
{
  enum mhd_HpackDecResult res;
  enum mhd_HpackGetNumResult dec_res;
  size_t idx_enc_len;
  uint_fast32_t field_idx;
  struct mhd_BufferConst idx_name;
  struct mhd_BufferConst idx_value;

  mhd_assert (1u == (enc_data[0] >> 7u));
  mhd_assert (0u != out_buff_size);

  /* If any dynamic table resize is pending, it must be performed before
     header strings processing. */
  res = dec_check_resize_pending (hk_dec);
  if (mhd_HPACK_DEC_RES_IS_ERR (res))
    return res;

  dec_res = hpack_dec_number (1u,
                              enc_data_size,
                              enc_data,
                              &field_idx,
                              &idx_enc_len);
  switch (dec_res)
  {
  case mhd_HPACK_GET_NUM_RES_INCOMPLETE:
    return mhd_HPACK_DEC_RES_INCOMPLETE; /* Failure exit point */
  case mhd_HPACK_GET_NUM_RES_TOO_LARGE:
    return mhd_HPACK_DEC_RES_HPACK_BAD_IDX; /* Failure exit point */
  case mhd_HPACK_GET_NUM_RES_TOO_LONG:
    return mhd_HPACK_DEC_RES_NUMBER_TOO_LONG; /* Failure exit point */
  case mhd_HPACK_GET_NUM_RES_NO_ERROR:
    break;
  default:
    mhd_UNREACHABLE ();
    return mhd_HPACK_DEC_RES_INTERNAL_ERR; /* Failure exit point */
  }

  mhd_assert (0u != idx_enc_len);

  if (mhd_COND_HARDLY_EVER (mhd_HPACK_MAX_POSSIBLE_IDX < field_idx))
    return mhd_HPACK_DEC_RES_HPACK_BAD_IDX; /* Failure exit point */

  if (! mhd_htbl_get_entry (hk_dec->dyn,
                            (dtbl_idx_ft) field_idx,
                            &idx_name,
                            &idx_value))
    return mhd_HPACK_DEC_RES_HPACK_BAD_IDX; /* Failure exit point */

  /* No math overflow check is needed here as both strings are already stored
     in memory together with pointers. */
  if (out_buff_size < (idx_name.size + idx_value.size + 2u))
    return mhd_HPACK_DEC_RES_BUFFER_TOO_SMALL; /* Failure exit point */

  memcpy (out_buff,
          idx_name.data,
          idx_name.size);
  out_buff[idx_name.size] = '\0'; /* Zero-terminate field name */

  memcpy (out_buff + idx_name.size + 1u,
          idx_value.data,
          idx_value.size);
  out_buff[idx_name.size + 1u + idx_value.size] = '\0'; /* Zero-terminate field value */

  *name_len = idx_name.size;
  *val_len = idx_value.size;
  *bytes_decoded = idx_enc_len;

  return mhd_HPACK_DEC_RES_NEW_FIELD;
}


/**
 * Decode an HPACK string literal (with or without Huffman coding).
 * The output string in @a out_buff is zero-terminated.
 * @param enc_data_size the size of @a enc_data
 * @param enc_data the pointer to the encoded data
 * @param out_buff_size the size of @a out_buff
 * @param[out] out_buff the output buffer for the decoded string,
 *                      the output is zero-terminated
 * @param[out] out_len set to the decoded string length,
 *                     not counting zero-termination
 * @param[out] bytes_decoded set to the number of decoded bytes
 * @return #mhd_HPACK_DEC_RES_NEW_FIELD on success,
 *        error code otherwise
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (2,1) MHD_FN_PAR_OUT_SIZE_ (4,3)
MHD_FN_PAR_OUT_ (5) MHD_FN_PAR_OUT_ (6) enum mhd_HpackDecResult
hpack_dec_string_literal (size_t enc_data_size,
                          const uint8_t *restrict enc_data,
                          size_t out_buff_size,
                          char *restrict out_buff,
                          size_t *restrict out_len,
                          size_t *restrict bytes_decoded)
{
  const bool is_huff_enc = (0u != (enc_data[0] & 0x80u));
  uint_fast32_t enc_str_len;
  enum mhd_HpackGetNumResult dec_res;
  size_t enc_num_len;
  size_t dec_str_len;

  mhd_assert (0u != enc_data_size);
  mhd_assert (0u != out_buff_size);

  dec_res = hpack_dec_number (1u,
                              enc_data_size,
                              enc_data,
                              &enc_str_len,
                              &enc_num_len);
  switch (dec_res)
  {
  case mhd_HPACK_GET_NUM_RES_INCOMPLETE:
    return mhd_HPACK_DEC_RES_INCOMPLETE;/* Failure exit point */
  case mhd_HPACK_GET_NUM_RES_TOO_LARGE:
    return mhd_HPACK_DEC_RES_STRING_TOO_LONG; /* Failure exit point */
  case mhd_HPACK_GET_NUM_RES_TOO_LONG:
    return mhd_HPACK_DEC_RES_NUMBER_TOO_LONG; /* Failure exit point */
  case mhd_HPACK_GET_NUM_RES_NO_ERROR:
    break;
  default:
    mhd_UNREACHABLE ();
    return mhd_HPACK_DEC_RES_INTERNAL_ERR; /* Failure exit point */
  }

  mhd_assert (0u != enc_num_len);
  mhd_assert (enc_num_len <= enc_data_size);

  if ((enc_data_size - enc_num_len) < enc_str_len)
    return mhd_HPACK_DEC_RES_INCOMPLETE; /* Failure exit point */

  if (mhd_COND_HARDLY_EVER (0u == enc_str_len))
    dec_str_len = 0; /* Zero length string, can be Huffman-encoded or not */
  else if (is_huff_enc)
  { /* String with Huffman encoding */
    enum mhd_H2HuffDecodeRes huff_dec_res;

    /* mhd_h2_huffman_decode() will check whether the output buffer is large
       enough. */
    dec_str_len = mhd_h2_huffman_decode ((size_t) enc_str_len,
                                         enc_data + enc_num_len,
                                         out_buff_size - 1u, /* leave one byte for zero-termination */
                                         out_buff,
                                         &huff_dec_res);
    switch (huff_dec_res)
    {
    case MHD_H2_HUFF_DEC_RES_NO_SPACE:
      return mhd_HPACK_DEC_RES_BUFFER_TOO_SMALL; /* Failure exit point */
    case MHD_H2_HUFF_DEC_RES_BROKEN_DATA:
      return mhd_HPACK_DEC_RES_HUFFMAN_ERR; /* Failure exit point */
      break;
    case MHD_H2_HUFF_DEC_RES_OK:
      break;
    default:
      mhd_UNREACHABLE ();
      return mhd_HPACK_DEC_RES_INTERNAL_ERR; /* Failure exit point */
    }
    mhd_assert (0u != dec_str_len);
    mhd_assert (MHD_H2_HUFF_DEC_RES_OK == huff_dec_res);
    mhd_assert (dec_str_len < out_buff_size);
  }
  else
  { /* String without Huffman encoding */
    if (out_buff_size <= enc_str_len) /* leave one byte for zero-termination */
      return mhd_HPACK_DEC_RES_BUFFER_TOO_SMALL; /* Failure exit point */

    dec_str_len = (size_t) enc_str_len;
    memcpy (out_buff,
            enc_data + enc_num_len,
            dec_str_len);
  }

  mhd_assert (out_buff_size > dec_str_len);

  out_buff[dec_str_len] = '\0'; /* Zero-terminate the result */
  *out_len = dec_str_len;
  *bytes_decoded = enc_num_len + (size_t) enc_str_len;
  return mhd_HPACK_DEC_RES_NEW_FIELD; /* Return any non-error code */
}


/**
 * Decode a literal header field (with or without indexing) and write
 * "name\0value\0" to the output buffer @a out_buff.
 * If @a with_indexing is 'true', the decoded field is inserted into the
 * dynamic table.
 * @param hk_dec the decoder context
 * @param enc_data_size the size of @a enc_data
 * @param enc_data the encoded data
 * @param out_buff_size the size of @a out_buff
 * @param with_indexing non-zero to insert the field into the dynamic table
 * @param[out] out_buff output the buffer for the decoded strings
 * @param[out] name_len set to the length of the name, not counting
 *                      zero-terminating
 * @param[out] val_len set to the length of the value, not counting
 *                     zero-terminating
 * @param[out] bytes_decoded set to the number of decoded bytes
 * @return #mhd_HPACK_DEC_RES_NEW_FIELD on success,
 *        error code otherwise
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (3,2) MHD_FN_PAR_OUT_SIZE_ (6,5)
MHD_FN_PAR_OUT_ (7) MHD_FN_PAR_OUT_ (8)
MHD_FN_PAR_OUT_ (9) enum mhd_HpackDecResult
hpack_dec_field_literal (struct mhd_HpackDecContext *restrict hk_dec,
                         size_t enc_data_size,
                         const uint8_t *restrict enc_data,
                         bool with_indexing,
                         size_t out_buff_size,
                         char *restrict out_buff,
                         size_t *restrict name_len,
                         size_t *restrict val_len,
                         size_t *restrict bytes_decoded)
{
  const uint_fast8_t prfx_bits = (with_indexing ? 2u : 4u);
  enum mhd_HpackDecResult res;
  size_t pos;
  size_t pos_incr;
  uint_fast32_t name_idx;

  mhd_assert (with_indexing || \
              (1u == (enc_data[0] >> 4u)) || (0u == (enc_data[0] >> 4u)));
  mhd_assert (! with_indexing || \
              (1u == (enc_data[0] >> 6u)));
  mhd_assert (0u != enc_data_size);
  mhd_assert (2u <= out_buff_size);

  /* If any dynamic table resize is pending, it must be performed before
     headers strings processing. */
  res = dec_check_resize_pending (hk_dec);
  if (mhd_HPACK_DEC_RES_IS_ERR (res))
    return res;

  pos = 0u;
#ifndef MHD_FAVOR_SMALL_CODE
  if (0u == (enc_data[0] & (b8ones >> prfx_bits)))
  {
    name_idx = 0u; /* Shortcut for frequent case */
    pos_incr = 1u;
  }
  else
#endif /* ! MHD_FAVOR_SMALL_CODE */
  if (1)
  {
    enum mhd_HpackGetNumResult dec_res;
    dec_res = hpack_dec_number (prfx_bits,
                                enc_data_size,
                                enc_data,
                                &name_idx,
                                &pos_incr);
    switch (dec_res)
    {
    case mhd_HPACK_GET_NUM_RES_INCOMPLETE:
      return mhd_HPACK_DEC_RES_INCOMPLETE; /* Failure exit point */
    case mhd_HPACK_GET_NUM_RES_TOO_LARGE:
      return mhd_HPACK_DEC_RES_HPACK_BAD_IDX; /* Failure exit point */
    case mhd_HPACK_GET_NUM_RES_TOO_LONG:
      return mhd_HPACK_DEC_RES_NUMBER_TOO_LONG; /* Failure exit point */
    case mhd_HPACK_GET_NUM_RES_NO_ERROR:
      break;
    default:
      mhd_UNREACHABLE ();
      return mhd_HPACK_DEC_RES_INTERNAL_ERR; /* Failure exit point */
    }

    mhd_assert (0u != pos_incr);
#ifndef MHD_FAVOR_SMALL_CODE
    mhd_assert (0u != name_idx);
#endif /* ! MHD_FAVOR_SMALL_CODE */
  }

  pos += pos_incr;
  mhd_assert (0u != pos);

  if (enc_data_size == pos)
    return mhd_HPACK_DEC_RES_INCOMPLETE; /* Failure exit point */

  if (0u == name_idx)
  { /* Literal name */
    mhd_assert (1u == pos);
    pos = 1u; /* Help compiler to optimise */

    res = hpack_dec_string_literal (enc_data_size - pos,
                                    enc_data + pos,
                                    out_buff_size - 1u,      /* At least one char for the value string */
                                    out_buff,
                                    name_len,
                                    &pos_incr);
    if (mhd_HPACK_DEC_RES_IS_ERR (res))
      return res; /* Failure exit point */
  }
  else
  { /* Indexed name */
    struct mhd_BufferConst idx_name;
    struct mhd_BufferConst idx_value; /* extracted value is unused */

    if (mhd_COND_HARDLY_EVER (mhd_HPACK_MAX_POSSIBLE_IDX < name_idx))
      return mhd_HPACK_DEC_RES_HPACK_BAD_IDX; /* Failure exit point */

    if (! mhd_htbl_get_entry (hk_dec->dyn,
                              (dtbl_idx_ft) name_idx,
                              &idx_name,
                              &idx_value))
      return mhd_HPACK_DEC_RES_HPACK_BAD_IDX; /* Failure exit point */

    if (idx_name.size >= (out_buff_size - 1u))
      return mhd_HPACK_DEC_RES_BUFFER_TOO_SMALL; /* Failure exit point */

    memcpy (out_buff,
            idx_name.data,
            idx_name.size);
    out_buff[idx_name.size] = '\0'; /* Zero-terminate resulting string */
    *name_len = idx_name.size;

    pos_incr = 0u;
  }
  pos += pos_incr;

  if (enc_data_size == pos)
    return mhd_HPACK_DEC_RES_INCOMPLETE; /* Failure exit point */

  mhd_assert (out_buff_size >= (*name_len + 2u));
  res = hpack_dec_string_literal (enc_data_size - pos,
                                  enc_data + pos,
                                  out_buff_size - (*name_len + 1u),
                                  out_buff + (*name_len + 1u),
                                  val_len,
                                  &pos_incr);
  if (mhd_HPACK_DEC_RES_IS_ERR (res))
    return res; /* Failure exit point */

  pos += pos_incr;
  *bytes_decoded = pos;

  if (with_indexing)
    mhd_dtbl_new_entry (hk_dec->dyn,
                        *name_len,
                        out_buff,
                        *val_len,
                        out_buff + (*name_len) + 1u);

  return mhd_HPACK_DEC_RES_NEW_FIELD;
}


/**
 * Decode and apply a Dynamic Table Size Update.
 * Performs eviction only; actual resize is deferred until before first header
 * decoding.
 * @param hk_dec the decoder context
 * @param enc_data_size the size of @a enc_data
 * @param enc_data the encoded data
 * @param[out] bytes_decoded set to the number of decoded bytes
 * @return #mhd_HPACK_DEC_RES_NO_NEW_FIELD on success,
 *         error code otherwise
 */
static MHD_FN_PAR_IN_SIZE_ (3,2)
MHD_FN_PAR_OUT_ (4) enum mhd_HpackDecResult
dec_update_dyn_size (struct mhd_HpackDecContext *restrict hk_dec,
                     const size_t enc_data_size,
                     const uint8_t *restrict enc_data,
                     size_t *restrict bytes_decoded)
{
  uint_fast32_t new_dyn_size;
  size_t used_bytes;
  enum mhd_HpackGetNumResult dec_res;

  mhd_assert ((1u == (enc_data[0] >> 5u)) && \
              "the first byte must be the dynamic table update signal");
  dec_res = hpack_dec_number (3u,
                              enc_data_size,
                              enc_data,
                              &new_dyn_size,
                              &used_bytes);
  switch (dec_res)
  {
  case mhd_HPACK_GET_NUM_RES_INCOMPLETE:
    return mhd_HPACK_DEC_RES_INCOMPLETE;                /* Failure exit point */
  case mhd_HPACK_GET_NUM_RES_TOO_LARGE:
    return mhd_HPACK_DEC_RES_DYN_SIZE_UPD_TOO_LARGE;    /* Failure exit point */
  case mhd_HPACK_GET_NUM_RES_TOO_LONG:
    return mhd_HPACK_DEC_RES_NUMBER_TOO_LONG;           /* Failure exit point */
  case mhd_HPACK_GET_NUM_RES_NO_ERROR:
    break;
  default:
    mhd_UNREACHABLE ();
    return mhd_HPACK_DEC_RES_INTERNAL_ERR;              /* Failure exit point */
  }
  mhd_assert (0u != used_bytes);

  if (hk_dec->max_allowed_dyn_size < new_dyn_size)
    return mhd_HPACK_DEC_RES_DYN_SIZE_UPD_TOO_LARGE;    /* Failure exit point */

  mhd_assert (mhd_DTBL_MAX_SIZE >= new_dyn_size);

  /* Only evict here, no resize yet to avoid repetitive realloc() calls if
     remote sends multiple table size updates in a row. */
  mhd_dtbl_evict_to_size (hk_dec->dyn,
                          (size_t) new_dyn_size);

  hk_dec->last_remote_dyn_size = (size_t) new_dyn_size;

  *bytes_decoded = used_bytes;
  return mhd_HPACK_DEC_RES_NO_NEW_FIELD; /* Success exit point */
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1)
MHD_FN_PAR_IN_SIZE_ (3, 2)
MHD_FN_PAR_OUT_SIZE_ (5, 4)
MHD_FN_PAR_OUT_ (6) MHD_FN_PAR_OUT_ (7)
MHD_FN_PAR_OUT_ (8) enum mhd_HpackDecResult
mhd_hpack_dec_data (struct mhd_HpackDecContext *restrict hk_dec,
                    size_t enc_data_size,
                    const uint8_t *restrict enc_data,
                    size_t out_buff_size,
                    char *restrict out_buff,
                    size_t *restrict name_len,
                    size_t *restrict val_len,
                    size_t *restrict bytes_decoded)
{
  uint_fast8_t action_id;

  mhd_assert (0u != enc_data_size);
  mhd_assert (2u <= out_buff_size);

  action_id = enc_data[0] >> 4u;

  switch (action_id)
  {
  case (1u << 3u) + 0u:
  case (1u << 3u) + 1u:
  case (1u << 3u) + 2u:
  case (1u << 3u) + 3u:
  case (1u << 3u) + 4u:
  case (1u << 3u) + 5u:
  case (1u << 3u) + 6u:
  case (1u << 3u) + 7u:
    /* Indexed field */
    return hpack_dec_field_indexed (hk_dec,
                                    enc_data_size,
                                    enc_data,
                                    out_buff_size,
                                    out_buff,
                                    name_len,
                                    val_len,
                                    bytes_decoded);
  case (1u << 2u) + 0u:
  case (1u << 2u) + 1u:
  case (1u << 2u) + 2u:
  case (1u << 2u) + 3u:
    /* Literal field with indexing */
    return hpack_dec_field_literal (hk_dec,
                                    enc_data_size,
                                    enc_data,
                                    true,
                                    out_buff_size,
                                    out_buff,
                                    name_len,
                                    val_len,
                                    bytes_decoded);
  case 0u << 0u:
    /* Literal field without indexing */
    return hpack_dec_field_literal (hk_dec,
                                    enc_data_size,
                                    enc_data,
                                    false,
                                    out_buff_size,
                                    out_buff,
                                    name_len,
                                    val_len,
                                    bytes_decoded);
  case 1u << 0u:
    /* Literal field never indexed */
    return hpack_dec_field_literal (hk_dec,
                                    enc_data_size,
                                    enc_data,
                                    false,
                                    out_buff_size,
                                    out_buff,
                                    name_len,
                                    val_len,
                                    bytes_decoded);
  case (1u << 1u) + 0u:
  case (1u << 1u) + 1u:
    /* Dynamic table size update */
    return dec_update_dyn_size (hk_dec,
                                enc_data_size,
                                enc_data,
                                bytes_decoded);
  default:
    break;
  }
  mhd_UNREACHABLE ();
  return mhd_HPACK_DEC_RES_INTERNAL_ERR;
}


/* ****** _____________ End of HPACK headers decoding ______________ ****** */

/* ****** ----------------- HPACK headers encoding ----------------- ****** */

/**
 * Compute the number of bytes required to encode an HPACK integer.
 *
 * Implements the integer encoding algorithm from RFC 7541, Section 5.1.
 * The @a prefix_bits parameter specifies the count of fixed most-significant
 * bits in the first byte (e.g., 1 for "1xxxxxxx", 2 for "01xxxxxx",
 * 3 for "001xxxxx", 4 for "0000xxxx"/"0001xxxx").
 * The number of value bits available in the first byte is (8 - @a prefix_bits).
 *
 * @param[in] prefix_bits the count of fixed high-order bits in the first byte;
 *                        must be greater than zero and less than 8
 * @param[in]  number the value to encode, must fit 32 bits
 * @return the total number of bytes needed (always non-zero)
 */
static size_t
hpack_number_len (uint_fast8_t prefix_bits,
                  uint_fast32_t number)
{
  const uint_fast8_t first_byte_val_max =
    (uint_fast8_t) (b8ones >> prefix_bits);
  uint_least32_t val_for_next_bytes;

  mhd_assert (0u != prefix_bits);
  mhd_assert (8u > prefix_bits);
  mhd_assert ((number & 0xFFFFFFFFu) == number);

  if (first_byte_val_max > number) /* the number must be strictly less than */
    return 1u;
  val_for_next_bytes = (uint_least32_t) (number - first_byte_val_max);
  if (0 == val_for_next_bytes)
    return 2u;
  return (uint_fast8_t) \
         ((mhd_BIT_WIDTH32NZ (val_for_next_bytes) + 6u) / 7u) + 1u;
}


/**
 * Encode an HPACK integer into the provided output buffer.
 *
 * Encodes @a number according to RFC 7541, Section 5.1 using the given
 * first-byte prefix. The @a first_byte_prefix must have its lowest
 * (8 - @a first_byte_prefix_bits) bits cleared; these bits will be filled
 * with the encoded value.
 *
 * @param[in]  first_byte_prefix the first byte with fixed MSB pattern set;
 *                                   lower value bits must be zero
 * @param[in]  first_byte_prefix_bits the count of fixed MSBs in the first byte
 *                                    (1 for 1xxxxxxx, 2 for 01xxxxxx, etc.)
 * @param[in]  number the value to encode, must fit 32 bits
 * @param[in]  buf_size the size of @a buf in bytes,
 *                      must not be zero
 * @param[out] buf the output buffer to write the encoded bytes
 * @return the number of bytes written on success;
 *         zero if output buffer is too small to fit the number encoded
 */
static MHD_FN_PAR_OUT_SIZE_ (5,4) size_t
hpack_put_number_to_buf (uint_fast8_t first_byte_prefix,
                         uint_fast8_t first_byte_prefix_bits,
                         uint_fast32_t number,
                         size_t buf_size,
                         uint8_t buf[MHD_FN_PAR_DYN_ARR_SIZE_ (buf_size)])
{
  const uint_fast8_t first_byte_val_max =
    (uint_fast8_t) (b8ones >> first_byte_prefix_bits);
  uint_fast32_t number_left;
  uint_fast8_t i;

  mhd_assert (0u == (first_byte_prefix & first_byte_val_max));
  mhd_assert (0u == ((first_byte_prefix >> 4u) >> 4u));
  mhd_assert (0u != first_byte_prefix_bits);
  mhd_assert (8u > first_byte_prefix_bits);
  mhd_assert ((number & 0xFFFFFFFFu) == number);
  mhd_assert (0u != buf_size);

  if (first_byte_val_max > number) /* the number must be strictly less than */
  {
    buf[0] = (uint8_t) (first_byte_prefix | (uint8_t) number);
    return 1u;
  }
  buf[0] = (uint8_t) (first_byte_prefix | first_byte_val_max);
  number_left = number - first_byte_val_max;
  for (i = 1u; mhd_COND_PREDOMINANTLY (i < buf_size); ++i)
  {
    const uint8_t cur_byte = (uint8_t) (number_left & 0x7Fu);
    number_left >>= 7u;
    if (0 == number_left)
    {
      mhd_assert (0u == (cur_byte & 0x80u));
      buf[i] = cur_byte;
      return i + 1u; /* Success exit point */
    }
    buf[i] = (uint8_t) (cur_byte | 0x80u);
    mhd_assert (6u > i);
  }
  return 0u; /* Not enough space */
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_ (1) bool
mhd_hpack_enc_init (struct mhd_HpackEncContext *hk_enc)
{
  hk_enc->dyn = mhd_dtbl_create (mhd_hpack_def_dyn_table_size);

  if (NULL == hk_enc->dyn)
    return false; /* Failure exit point */

  mhd_assert (mhd_hpack_def_dyn_table_size == \
              mhd_dtbl_get_table_max_size (hk_enc->dyn));

  /* Set all sizes to the same initial value */
  hk_enc->dyn_size_peer = mhd_hpack_def_dyn_table_size;
  hk_enc->dyn_size_new = hk_enc->dyn_size_peer;
  hk_enc->dyn_size_smallest = hk_enc->dyn_size_peer;

  return true; /* Success exit point */
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1) void
mhd_hpack_enc_deinit (struct mhd_HpackEncContext *hk_enc)
{
  if (NULL == hk_enc->dyn)
    return; /* Nothing to deinit */

  mhd_dtbl_destroy (hk_enc->dyn);
  hk_enc->dyn = NULL;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1) void
mhd_hpack_enc_set_dyn_size (struct mhd_HpackEncContext *hk_enc,
                            size_t new_dyn_size)
{
  mhd_assert (mhd_DTBL_MAX_SIZE >= new_dyn_size);
  if (hk_enc->dyn_size_smallest > new_dyn_size)
    hk_enc->dyn_size_smallest = new_dyn_size;

  /* Postpone actual table resize to avoid several realloc() calls if
     multiple table resizes are performed. */
  hk_enc->dyn_size_new = new_dyn_size;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1) bool
mhd_hpack_enc_dyn_resize (struct mhd_HpackEncContext *hk_enc)
{
  mhd_assert (hk_enc->dyn_size_new >= hk_enc->dyn_size_smallest);

  if (mhd_dtbl_get_table_max_size (hk_enc->dyn) != hk_enc->dyn_size_new)
  {
#ifndef MHD_FAVOR_SMALL_CODE
    /* This is just an optimisation to simplify eviction later */
    mhd_dtbl_evict_to_size (hk_enc->dyn,
                            hk_enc->dyn_size_smallest);
#endif /* ! MHD_FAVOR_SMALL_CODE */

    if (mhd_COND_HARDLY_EVER (! mhd_dtbl_resize (&(hk_enc->dyn), \
                                                 hk_enc->dyn_size_new)))
      return false;

    mhd_assert (mhd_dtbl_get_table_max_size (hk_enc->dyn) == \
                hk_enc->dyn_size_new);
  }

  return true;
}


/**
 * Encode an indexed field representation (RFC 7541, Section 6.1).
 *
 * @param[in]  idx the 1-based field index, must be non-zero
 * @param[in]  out_buff_size the size of @a out_buff in bytes,
 *                           must not be zero
 * @param[out] out_buff the output buffer to write the encoded field
 * @param[out] bytes_encoded to be set to the number of bytes written to the
 *                           @a out_buff
 * @return 'true' on success;
 *         'false' if the output buffer is too small
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_SIZE_ (3,2) MHD_FN_PAR_OUT_ (4) bool
hpack_enc_field_indexed (dtbl_idx_ft idx,
                         const size_t out_buff_size,
                         uint8_t *restrict out_buff,
                         size_t *restrict bytes_encoded)
{
  mhd_constexpr uint_fast8_t field_indexed_prfx = (uint_fast8_t) (1u << 7u);
  mhd_constexpr uint_fast8_t field_indexed_prfx_bits = 1u;
  size_t pos;

  mhd_assert (0u != idx);
  mhd_assert (mhd_HPACK_MAX_POSSIBLE_IDX >= idx);
  mhd_assert (0u != out_buff_size);

  pos = hpack_put_number_to_buf (field_indexed_prfx,
                                 field_indexed_prfx_bits,
                                 idx,
                                 out_buff_size,
                                 out_buff);

  if (0u == pos)
    return false; /* Not enough space in the output buffer */

  *bytes_encoded = pos;
  return true;
}


/**
 * Literal header indexing type for HPACK literal representations.
 *
 * Selects which literal form to use (RFC 7541, Sections 6.2.1-6.2.3).
 */
enum MHD_FIXED_ENUM_ mhd_HpackEncLitIndexingType
{
  /**
   * "Literal Header Field with Incremental Indexing"
   * RFC 7541, Section 6.2.1.
   */
  mhd_HPACK_ENC_LIT_IDX_TYPE_INDEXING,
  /**
   * "Literal Header Field without Indexing"
   * RFC 7541, Section 6.2.2.
   */
  mhd_HPACK_ENC_LIT_IDX_TYPE_NOT_INDEXING,
  /**
   * "Literal Header Field Never Indexed"
   * RFC 7541, Section 6.2.3.
   */
  mhd_HPACK_ENC_LIT_IDX_TYPE_NEVER_INDEXING
};


/**
 * Encode a string literal with optional Huffman coding (RFC 7541, Section 5.2).
 *
 * @param[in,out] hk_enc the encoder context
 * @param[in]     str_data the field string to encode
 * @param[in]     huffman_allowed set to 'true' to allow Huffman encoding
 * @param[in]     out_buff_size the size of @a out_buff in bytes, could be zero
 * @param[out]    out_buff the output buffer
 * @param[out]    bytes_encoded to be set to the size of the encoded data
 *                              written to the @a out_buff
 * @return 'true' on success;
 *         'false' if the output buffer is too small
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_ (1)
MHD_FN_PAR_OUT_SIZE_ (4,3) MHD_FN_PAR_OUT_ (5) bool
hpack_enc_string_literal (const struct mhd_BufferConst *restrict str_data,
                          bool huffman_allowed,
                          const size_t out_buff_size,
                          uint8_t *restrict out_buff,
                          size_t *restrict bytes_encoded)
{
  /** The prefix for Huffman-encoded string */
  mhd_constexpr uint8_t huff_on_prfx = (uint8_t) (1u << 7u);
  /** The prefix for literal string without Huffman encoding */
  mhd_constexpr uint8_t huff_off_prfx = (uint8_t) (0u << 7u);
  mhd_constexpr uint8_t huff_prfx_bits = 1u;
  size_t enc_size;
  size_t enc_size_enc_len;

  mhd_assert ((str_data->size & 0xFFFFFFFFu) == str_data->size);

  if (mhd_COND_ALMOST_NEVER (0u == str_data->size))
  {
    if (0u == out_buff_size)
      return false;
    /* If Huffman is allowed, encode zero size as "Huffman encoded" for
       consistency. */
    out_buff[0] = (huffman_allowed ? huff_on_prfx : huff_off_prfx);
    *bytes_encoded = 1u;
    return true;
  }

  if (huffman_allowed)
  {
    uint_fast32_t est_enc_size;
    size_t est_enc_size_enc_len;
    bool is_limited_by_buff_size;

    est_enc_size =
      mhd_h2_huffman_est_avg_size ((uint_fast32_t) str_data->size);
    est_enc_size_enc_len = hpack_number_len (huff_prfx_bits,
                                             est_enc_size);
    if ((out_buff_size <= est_enc_size_enc_len) ||
        ((out_buff_size - est_enc_size_enc_len) < est_enc_size))
    {
      /* Probably the buffer is not large enough to encode the string */
      /* Try as if the string were compressible to a minimal size */
      est_enc_size =
        mhd_h2_huffman_est_min_size ((uint_fast32_t) str_data->size);
      est_enc_size_enc_len = hpack_number_len (huff_prfx_bits,
                                               est_enc_size);
      if (out_buff_size < (est_enc_size_enc_len + est_enc_size))
        return false; /* The output buffer is not large enough */
      is_limited_by_buff_size = true;
    }
    else
      is_limited_by_buff_size =
        ((out_buff_size - est_enc_size_enc_len) < str_data->size);

    mhd_assert (out_buff_size > est_enc_size_enc_len);
    mhd_assert ((out_buff_size - est_enc_size_enc_len) \
                >= est_enc_size);
    mhd_assert (is_limited_by_buff_size || \
                ((out_buff_size - est_enc_size_enc_len) >= str_data->size));

    /* Limit the size of the buffer for the encoded string to the size of
       the original (not encoded) string or the size of the buffer (whatever
       is smaller). By limiting the size of the buffer to the size of the
       original string, Huffman encoding that grows larger than the original
       is aborted early. */
    enc_size =
      mhd_h2_huffman_encode (str_data->size,
                             str_data->data,
                             (size_t)
                             (is_limited_by_buff_size ?
                              (out_buff_size - est_enc_size_enc_len) :
                              str_data->size),
                             out_buff + est_enc_size_enc_len);

    mhd_assert (out_buff_size - est_enc_size_enc_len >= enc_size);

    if (0u != enc_size)
    {
      /* Successfully Huffman-encoded the string */
      enc_size_enc_len =
        hpack_put_number_to_buf (huff_on_prfx,
                                 huff_prfx_bits,
                                 (uint_fast32_t) enc_size,
                                 est_enc_size_enc_len,
                                 out_buff);
      if (mhd_COND_ALMOST_NEVER (0u == enc_size_enc_len))
      {
        /* The actual encoded size is larger than estimated */
        size_t calc_enc_size_enc_len;

        mhd_assert (est_enc_size < enc_size);

        calc_enc_size_enc_len = hpack_number_len (huff_prfx_bits,
                                                  (uint_fast32_t) enc_size);
        if ((out_buff_size - enc_size) < calc_enc_size_enc_len)
          return false; /* The output buffer is not large enough */

        memmove (out_buff + calc_enc_size_enc_len,
                 out_buff + est_enc_size_enc_len,
                 enc_size);

        enc_size_enc_len =
          hpack_put_number_to_buf (huff_on_prfx,
                                   huff_prfx_bits,
                                   (uint_fast32_t) enc_size,
                                   calc_enc_size_enc_len,
                                   out_buff);
        mhd_assert (calc_enc_size_enc_len == enc_size_enc_len);
      }
      else if (est_enc_size_enc_len != enc_size_enc_len)
      {
        mhd_assert (est_enc_size_enc_len > enc_size_enc_len);
        memmove (out_buff + enc_size_enc_len,
                 out_buff + est_enc_size_enc_len,
                 enc_size);
      }

      *bytes_encoded = (enc_size_enc_len + enc_size);
      return true; /* Success exit point */
    }
    else /* 0u == enc_size */
    {
      /* Huffman-encoded version needs more space than provided */
      /* If available space was less than needed to put the string without
         Huffman encoding, then return failure here. */
      if (is_limited_by_buff_size)
        return false;
    }
    /* Retry without Huffman encoding */
  }

  /* Put string without Huffman encoding */
  enc_size = str_data->size;

  if (enc_size >= out_buff_size)
    return false; /* The output buffer is not large enough */

  enc_size_enc_len =
    hpack_put_number_to_buf (huff_off_prfx,
                             huff_prfx_bits,
                             (uint_fast32_t) enc_size,
                             out_buff_size - enc_size,
                             out_buff);

  if (0u == enc_size_enc_len)
    return false; /* The output buffer is not large enough */

  mhd_assert ((out_buff_size - enc_size_enc_len) >= enc_size);

  memcpy (out_buff + enc_size_enc_len,
          str_data->data,
          enc_size);

  *bytes_encoded = (enc_size_enc_len + enc_size);
  return true; /* Success exit point */
}


/**
 * Encode a literal field (name by index reference or literal; value
 * always literal).
 *
 * Produces one of the literal field representations (RFC 7541,
 * Sections 6.2.1-6.2.3).
 * The name may be encoded by index reference (if allowed) or literally; the
 * value is always encoded literally.
 * String representations may use Huffman coding if permitted.
 *
 * @param[in,out] hk_enc the encoder context
 * @param[in]     name the field name bytes and size
 * @param[in]     name_idx the field name index if known and indexed name is
 *                         allowed,
 *                         zero if index is not known or indexed name is not
 *                         allowed
 * @param[in]     value the field value bytes and size
 * @param[in]     msg_type the literal representation kind to use
 * @param[in]     name_idx_stat_allowed set to 'true' if name encoding as a
 *                                      reference to the static table is allowed
 * @param[in]     name_idx_dyn_allowed set to 'true' if name encoding as a
 *                                     reference to the dynamic table is allowed
 * @param[in]     huffman_allowed set to 'true' if Huffman coding is allowed
 * @param[in]     out_buff_size the size of @a out_buff in bytes,
 *                              could be zero (the function will always fail
 *                              if it is less than two)
 * @param[out]    out_buff the output buffer for the encoded field
 * @param[out]    bytes_encoded to be set to the number of bytes written to
 *                              the @a out_buff
 * @return 'true' on success;
 *         'false' if the output buffer is too small
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1)
MHD_FN_PAR_IN_ (2) MHD_FN_PAR_IN_ (4)
MHD_FN_PAR_OUT_SIZE_ (10,9) MHD_FN_PAR_OUT_ (11) bool
hpack_enc_field_literal (struct mhd_HpackEncContext *restrict hk_enc,
                         const struct mhd_BufferConst *restrict name,
                         dtbl_idx_ft name_idx,
                         const struct mhd_BufferConst *restrict value,
                         enum mhd_HpackEncLitIndexingType msg_type,
                         bool name_idx_stat_allowed,
                         bool name_idx_dyn_allowed,
                         bool huffman_allowed,
                         const size_t out_buff_size,
                         uint8_t *restrict out_buff,
                         size_t *restrict bytes_encoded)
{
  mhd_constexpr uint_fast8_t field_indexing_prfx = (uint_fast8_t) (1u << 6u);
  mhd_constexpr uint_fast8_t field_indexing_prfx_bits = 2u;
  mhd_constexpr uint_fast8_t field_not_idxng_prfx = (uint_fast8_t) (0u << 4u);
  mhd_constexpr uint_fast8_t field_not_idxng_prfx_bits = 4u;
  mhd_constexpr uint_fast8_t field_never_idxng_prfx = (uint_fast8_t) (1u << 4u);
  mhd_constexpr uint_fast8_t field_never_idxng_prfx_bits = 4u;
  struct mhd_HpackDTblContext const *restrict dyn = hk_enc->dyn;
  uint_fast8_t first_byte_prefix;
  uint_fast8_t first_byte_prefix_bits;
  size_t pos;
  size_t pos_incr;

  mhd_assert ((0u == name->size) || (':' != name->data[0]));

  if (2u > out_buff_size)
    return false; /* No space even for the minimal field */

  switch (msg_type)
  {
  case mhd_HPACK_ENC_LIT_IDX_TYPE_INDEXING:
    first_byte_prefix = field_indexing_prfx;
    first_byte_prefix_bits = field_indexing_prfx_bits;
    break;
  case mhd_HPACK_ENC_LIT_IDX_TYPE_NOT_INDEXING:
    first_byte_prefix = field_not_idxng_prfx;
    first_byte_prefix_bits = field_not_idxng_prfx_bits;
    break;
  case mhd_HPACK_ENC_LIT_IDX_TYPE_NEVER_INDEXING:
    first_byte_prefix = field_never_idxng_prfx;
    first_byte_prefix_bits = field_never_idxng_prfx_bits;
    break;
  default:
    mhd_UNREACHABLE ();
    return false;
  }

  if (0u == name_idx)
  {
    if (name_idx_stat_allowed && name_idx_dyn_allowed)
      name_idx = mhd_htbl_find_name_real (dyn,
                                          name->size,
                                          name->data);
    else if (name_idx_stat_allowed && (0u != name->size))
      name_idx = mhd_stbl_find_name_real (name->size,
                                          name->data);
    else if (mhd_COND_ALMOST_NEVER (name_idx_dyn_allowed))
      name_idx = mhd_dtbl_find_name (dyn,
                                     name->size,
                                     name->data);
  }
  else
  {
    mhd_assert (name_idx_stat_allowed || \
                (mhd_HPACK_STBL_LAST_IDX < name_idx));
    mhd_assert (name_idx_dyn_allowed || \
                (mhd_HPACK_STBL_LAST_IDX >= name_idx));
  }

  pos = 0u;

  if (0u != name_idx)
  {
    /* Add name as a reference */
    mhd_assert (name_idx_dyn_allowed || name_idx_stat_allowed);
    pos_incr = hpack_put_number_to_buf (first_byte_prefix,
                                        first_byte_prefix_bits,
                                        name_idx,
                                        out_buff_size - pos - 1u, /* Reserve one byte for the field value */
                                        out_buff + pos);
    if (0u == pos_incr)
      return false; /* Not enough space */
    pos += pos_incr;
  }
  else
  {
    /* Add name literally */

    /* Use 'zero' index to indicate literal name */
    out_buff[pos++] = (uint8_t) first_byte_prefix;

    /* The buffer has at least one byte (or more) available;
       the next call will fail if only one byte is available. */
    if (! hpack_enc_string_literal (name,
                                    huffman_allowed,
                                    out_buff_size - pos - 1u,      /* Reserve one byte for the field value */
                                    out_buff + pos,
                                    &pos_incr))
      return false; /* Not enough space */

    pos += pos_incr;
  }

  /* The output buffer should have at least one byte of space available */
  mhd_assert (out_buff_size > pos);

  /* Add value literally */

  if (! hpack_enc_string_literal (value,
                                  huffman_allowed,
                                  out_buff_size - pos,
                                  out_buff + pos,
                                  &pos_incr))
    return false; /* Not enough space */

  pos += pos_incr;
  mhd_assert (out_buff_size >= pos);

  *bytes_encoded = pos;
  return true;
}


/**
 * Internal per-field encoding result.
 */
enum MHD_FIXED_ENUM_ mhd_HpackEncResultInternal
{
  /**
   * The output buffer is too small
   */
  mhd_ENC_RESULT_INT_NO_SPACE = 0,
  /**
   * The field is encoded successfully, do not add the field to the dynamic
   * table
   */
  mhd_ENC_RESULT_INT_OK_NO_ADD_TO_DYN,
  /**
   * The field is encoded successfully, add the field to the dynamic table
   */
  mhd_ENC_RESULT_INT_OK_ADD_TO_DYN
};

/**
 * Encode one field according to the requested indexing policy.
 *
 * Chooses between indexed and literal representations based on table contents
 * and the @a enc_pol policy, and decides whether to add the field to the
 * dynamic table (using simple size-based heuristics when not explicitly
 * forced).
 *
 * @param[in,out] hk_enc the encoder context
 * @param[in]     name the header name
 * @param[in]     value the header value
 * @param[in]     enc_pol the encoding policy to apply
 * @param[in]     out_buff_size the size of @a out_buff in bytes,
 *                              must not be zero
 * @param[out]    out_buff the output buffer
 * @param[out]    bytes_encoded to be set to the number of bytes written to
 *                              the @a out_buff
 * @return #mhd_ENC_RESULT_INT_NO_SPACE on insufficient buffer;
 *         #mhd_ENC_RESULT_INT_OK_NO_ADD_TO_DYN or
 *         #mhd_ENC_RESULT_INT_OK_ADD_TO_DYN on success
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1)
MHD_FN_PAR_IN_ (2) MHD_FN_PAR_IN_ (3)
MHD_FN_PAR_OUT_SIZE_ (6,5) MHD_FN_PAR_OUT_ (7) enum mhd_HpackEncResultInternal
hpack_enc_field (struct mhd_HpackEncContext *restrict hk_enc,
                 const struct mhd_BufferConst *restrict name,
                 const struct mhd_BufferConst *restrict value,
                 enum mhd_HpackEncPolicy enc_pol,
                 const size_t out_buff_size,
                 uint8_t *restrict out_buff,
                 size_t *restrict bytes_encoded)
{
  mhd_assert (0u != out_buff_size);
  mhd_assert ((name->size & 0xFFFFFFFFu) == name->size);
  mhd_assert ((value->size & 0xFFFFFFFFu) == value->size);

  /* Check the enum values order */
  // TODO: replace with static asserts
  mhd_assert (mhd_HPACK_ENC_POL_FORCED_NEW_IDX < mhd_HPACK_ENC_POL_FORCED);
  mhd_assert (mhd_HPACK_ENC_POL_ALWAYS_IF_FIT < mhd_HPACK_ENC_POL_NOT_INDEXED);
  mhd_assert (mhd_HPACK_ENC_POL_ALWAYS_IF_FIT < mhd_HPACK_ENC_POL_DESIRABLE);
  mhd_assert (mhd_HPACK_ENC_POL_DESIRABLE < mhd_HPACK_ENC_POL_LOWEST_PRIO);
  mhd_assert (mhd_HPACK_ENC_POL_LOWEST_PRIO < mhd_HPACK_ENC_POL_AVOID_NEW_IDX);
  mhd_assert (mhd_HPACK_ENC_POL_AVOID_NEW_IDX < mhd_HPACK_ENC_POL_NOT_INDEXED);
  mhd_assert (mhd_HPACK_ENC_POL_NOT_INDEXED < \
              mhd_HPACK_ENC_POL_NEVER_W_NAME_IDX);
  mhd_assert (mhd_HPACK_ENC_POL_NEVER_W_NAME_IDX < \
              mhd_HPACK_ENC_POL_NEVER_W_NAME_LIT_NO_HUFFMAN);

  if ((mhd_HPACK_ENC_POL_FORCED <= enc_pol)
      && (mhd_HPACK_ENC_POL_AVOID_NEW_IDX >= enc_pol))
  {
    const dtbl_idx_ft field_idx =
      mhd_htbl_find_entry_real (hk_enc->dyn,
                                name->size,
                                name->data,
                                value->size,
                                value->data);

    if (0u != field_idx)
    {
      if (! hpack_enc_field_indexed (field_idx,
                                     out_buff_size,
                                     out_buff,
                                     bytes_encoded))
        return mhd_ENC_RESULT_INT_NO_SPACE;

      return mhd_ENC_RESULT_INT_OK_NO_ADD_TO_DYN;
    }
  }

  /* The field is not in the tables or should not be added as an indexed
     field */

  /* Add the field literally */

  if (mhd_HPACK_ENC_POL_NEVER_W_NAME_IDX <= enc_pol)
  {
    /* Add field literally as "never indexed" */
    const bool name_idx_stat_allowed =
      (mhd_HPACK_ENC_POL_NEVER_W_NAME_IDX_STATIC >= enc_pol);
    const bool name_idx_dyn_allowed =
      (mhd_HPACK_ENC_POL_NEVER_W_NAME_IDX_STATIC > enc_pol);
    const bool huffman_allowed =
      (mhd_HPACK_ENC_POL_NEVER_W_NAME_LIT_NO_HUFFMAN > enc_pol);
    if (! hpack_enc_field_literal (hk_enc,
                                   name,
                                   0u,
                                   value,
                                   mhd_HPACK_ENC_LIT_IDX_TYPE_NEVER_INDEXING,
                                   name_idx_stat_allowed,
                                   name_idx_dyn_allowed,
                                   huffman_allowed,
                                   out_buff_size,
                                   out_buff,
                                   bytes_encoded))
      return mhd_ENC_RESULT_INT_NO_SPACE;

    return mhd_ENC_RESULT_INT_OK_NO_ADD_TO_DYN;
  }

  if (mhd_HPACK_ENC_POL_AVOID_NEW_IDX <= enc_pol)
  {
    /* Adding to the tables is not allowed */
    mhd_assert (mhd_HPACK_ENC_POL_NOT_INDEXED >= enc_pol);

    if (! hpack_enc_field_literal (hk_enc,
                                   name,
                                   0u,
                                   value,
                                   mhd_HPACK_ENC_LIT_IDX_TYPE_NOT_INDEXING,
                                   true,
                                   true,
                                   true,
                                   out_buff_size,
                                   out_buff,
                                   bytes_encoded))
      return mhd_ENC_RESULT_INT_NO_SPACE;

    return mhd_ENC_RESULT_INT_OK_NO_ADD_TO_DYN;
  }

  if (mhd_HPACK_ENC_POL_ALWAYS_IF_FIT >= enc_pol)
  {
    bool add_to_idx;
    if ((mhd_HPACK_ENC_POL_FORCED == enc_pol) ||
        (mhd_HPACK_ENC_POL_FORCED_NEW_IDX == enc_pol))
      add_to_idx = true;
    else
      add_to_idx = mhd_dtbl_check_entry_fit (hk_enc->dyn,
                                             name->size,
                                             value->size);

    if (! hpack_enc_field_literal (hk_enc,
                                   name,
                                   0u,
                                   value,
                                   add_to_idx ?
                                   mhd_HPACK_ENC_LIT_IDX_TYPE_INDEXING :
                                   mhd_HPACK_ENC_LIT_IDX_TYPE_NOT_INDEXING,
                                   true,
                                   true,
                                   true,
                                   out_buff_size,
                                   out_buff,
                                   bytes_encoded))
      return mhd_ENC_RESULT_INT_NO_SPACE;

    return add_to_idx ?
           mhd_ENC_RESULT_INT_OK_ADD_TO_DYN :
           mhd_ENC_RESULT_INT_OK_NO_ADD_TO_DYN;
  }

  /* Indexing or not indexing is not forced.
     Need to decide whether to add the field to the index based on some
     heuristics.
     Use only field size and buffer data when deciding. Do not analyse the
     field name or value (it should be performed by caller). */

  mhd_assert (mhd_HPACK_ENC_POL_DESIRABLE <= enc_pol);
  mhd_assert (mhd_HPACK_ENC_POL_LOWEST_PRIO >= enc_pol);

  if (1) /* For local scope */
  {
    enum mhd_Tristate add_to_idx;

    add_to_idx =
      mhd_dtbl_check_entry_fit (hk_enc->dyn,
                                name->size,
                                value->size) ? mhd_T_MAYBE : mhd_T_NO;

    /* The following algorithm is simplified and can be improved */

    if (mhd_T_IS_MAYBE (add_to_idx))
    {
      const size_t field_size =
        name->size + value->size + mhd_dtbl_entry_overhead;
      const size_t dyn_size = hk_enc->dyn_size_new;
      const size_t dyn_used = mhd_dtbl_get_table_used (hk_enc->dyn);
      const size_t dyn_free = dyn_size - dyn_used;
      const size_t num_entries = mhd_dtbl_get_num_entries (hk_enc->dyn);

      mhd_assert (dyn_size >= dyn_used);

      if (512u > dyn_size)
      {
        /* Very small table, use very basic logic */
        add_to_idx =
          (mhd_HPACK_ENC_POL_NEUTRAL >= enc_pol) ? mhd_T_YES : mhd_T_NO;
      }
      else if (mhd_HPACK_ENC_POL_DESIRABLE >= enc_pol)
      {
        mhd_assert (mhd_HPACK_ENC_POL_DESIRABLE == enc_pol);
        if (field_size <= dyn_free)
          add_to_idx = mhd_T_YES;
        else if (field_size <= (dyn_size - dyn_size / 4))
          add_to_idx = mhd_T_YES;
        else if (2u >= num_entries)
          add_to_idx = mhd_T_YES;
        else
          add_to_idx = mhd_T_NO;
      }
      else if (mhd_HPACK_ENC_POL_NEUTRAL == enc_pol)
      {
        if (field_size <= dyn_free / 4)
          add_to_idx = mhd_T_YES;
        else if (field_size <= dyn_size / 32)
          add_to_idx = mhd_T_YES;
        else if ((field_size <= dyn_size / 4)
                 && ((field_size / 2) >= (dyn_used / num_entries)))
          add_to_idx = mhd_T_YES;
        else
          add_to_idx = mhd_T_NO;
      }
      else if (mhd_HPACK_ENC_POL_LOW_PRIO == enc_pol)
      {
        if (field_size <= dyn_free / 16)
          add_to_idx = mhd_T_YES;
        else if (field_size <= dyn_size / 128)
          add_to_idx = mhd_T_YES;
        else
          add_to_idx = mhd_T_NO;
      }
      else if (mhd_HPACK_ENC_POL_LOWEST_PRIO == enc_pol)
      {
        if (field_size <= dyn_free / 64)
          add_to_idx = mhd_T_YES;
        else if (field_size <= dyn_size / 512)
          add_to_idx = mhd_T_YES;
        else
          add_to_idx = mhd_T_NO;
      }
      else
      {
        mhd_UNREACHABLE ();
        add_to_idx = mhd_T_NO;
      }
    }
    mhd_assert (mhd_T_IS_NOT_MAYBE (add_to_idx));

    if (mhd_T_IS_YES (add_to_idx))
    {
      if (! hpack_enc_field_literal (hk_enc,
                                     name,
                                     0u,
                                     value,
                                     mhd_HPACK_ENC_LIT_IDX_TYPE_INDEXING,
                                     true,
                                     true,
                                     true,
                                     out_buff_size,
                                     out_buff,
                                     bytes_encoded))
        return mhd_ENC_RESULT_INT_NO_SPACE;

      return mhd_ENC_RESULT_INT_OK_ADD_TO_DYN;
    }
  }

  if (! hpack_enc_field_literal (hk_enc,
                                 name,
                                 0u,
                                 value,
                                 mhd_HPACK_ENC_LIT_IDX_TYPE_NOT_INDEXING,
                                 true,
                                 true,
                                 true,
                                 out_buff_size,
                                 out_buff,
                                 bytes_encoded))
    return mhd_ENC_RESULT_INT_NO_SPACE;

  return mhd_ENC_RESULT_INT_OK_NO_ADD_TO_DYN;
}


/**
 * Emit Dynamic Table Size Update representation(s) if needed.
 *
 * If the current dynamic table size differs from the pending minimal/final
 * sizes accumulated in @a hk_enc, this function encodes one or two size
 * updates, and performs local eviction down to the minimal size for
 * consistency.
 *
 * @param[in,out] hk_enc the encoder context
 * @param[in]     out_buff_size the size of @a out_buff in bytes,
 *                              could be zero
 * @param[out]    out_buff the output buffer to write encoded messages
 * @param[out] bytes_encoded the output variable to be set to the number of
 *                           bytes written
 * @return 'true' on success;
 *         'false' if the output buffer is too small
 */
static MHD_FN_PAR_OUT_SIZE_ (3,2) MHD_FN_PAR_OUT_ (4) bool
hpack_enc_check_dyn_size_update (
  struct mhd_HpackEncContext *restrict hk_enc,
  size_t out_buff_size,
  uint8_t *restrict out_buff,
  size_t *restrict bytes_encoded)
{
  /** The prefix for Dynamic Table Size Update message */
  mhd_constexpr uint_fast8_t dyn_size_upd_msg_prfx = (uint_fast8_t) (1u << 5u);
  mhd_constexpr uint_fast8_t dyn_size_upd_msg_prfx_bits = 3u;
  size_t pos;
  size_t pos_incr;
  struct mhd_HpackDTblContext *restrict const dyn = hk_enc->dyn;

  mhd_assert (mhd_DTBL_MAX_SIZE >= hk_enc->dyn_size_smallest);
  mhd_assert (mhd_DTBL_MAX_SIZE >= hk_enc->dyn_size_new);
  mhd_assert (hk_enc->dyn_size_peer >= hk_enc->dyn_size_smallest);
  mhd_assert (hk_enc->dyn_size_new >= hk_enc->dyn_size_smallest);
  mhd_assert (mhd_dtbl_get_table_max_size (dyn) \
              >= hk_enc->dyn_size_smallest);

  if (mhd_dtbl_get_table_max_size (dyn) != hk_enc->dyn_size_smallest)
    mhd_dtbl_evict_to_size (dyn,
                            hk_enc->dyn_size_smallest);

  if ((hk_enc->dyn_size_smallest == hk_enc->dyn_size_peer) &&
      (hk_enc->dyn_size_new == hk_enc->dyn_size_peer))
  {
    *bytes_encoded = 0u;
    return true; /* No resize signal needed */
  }

  /* Need to create a "Dynamic Table Size Update" signal */
  if (0u == out_buff_size)
    return false; /* Not enough space */

  pos = 0u;

  if (hk_enc->dyn_size_peer != hk_enc->dyn_size_smallest)
  {
    /* Signal the minimal size so the peer evicts entries */
    pos_incr =
      hpack_put_number_to_buf (dyn_size_upd_msg_prfx,
                               dyn_size_upd_msg_prfx_bits,
                               (uint_fast32_t) hk_enc->dyn_size_smallest,
                               out_buff_size,
                               out_buff);

    if (0u == pos_incr)
      return false; /* Not enough space */

    pos += pos_incr;
  }

  if (hk_enc->dyn_size_new != hk_enc->dyn_size_smallest)
  {
    if (pos == out_buff_size)
      return false; /* Not enough space for the second resize message */

    /* Signal the final dynamic table size */
    pos_incr =
      hpack_put_number_to_buf (dyn_size_upd_msg_prfx,
                               dyn_size_upd_msg_prfx_bits,
                               (uint_fast32_t) hk_enc->dyn_size_new,
                               out_buff_size - pos,
                               out_buff + pos);

    if (0u == pos_incr)
      return false; /* Not enough space */

    pos += pos_incr;
  }

  mhd_assert (0u != pos);
  *bytes_encoded = pos;
  return true;
}


/**
 * Apply a pending Dynamic Table Size Update for the encoder.
 *
 * Resizes the dynamic table to @a hk_enc->new_dyn_size if needed and updates
 * hk_enc data accordingly.
 *
 * @param[in,out] hk_enc the encoder context
 * @return 'true' on success;
 *         'false' on allocation error
 */
static bool
hpack_enc_perform_dyn_size_update (struct mhd_HpackEncContext *restrict hk_enc)
{
  mhd_assert (mhd_dtbl_get_table_used (hk_enc->dyn)
              <= hk_enc->dyn_size_smallest);
  if (mhd_dtbl_get_table_max_size (hk_enc->dyn) != hk_enc->dyn_size_new)
  {
    if (mhd_COND_HARDLY_EVER (! mhd_dtbl_resize (&(hk_enc->dyn), \
                                                 hk_enc->dyn_size_new)))
      return false;

    mhd_assert (mhd_dtbl_get_table_max_size (hk_enc->dyn) == \
                hk_enc->dyn_size_new);
  }

  hk_enc->dyn_size_smallest = hk_enc->dyn_size_new;
  hk_enc->dyn_size_peer = hk_enc->dyn_size_new;

  return true;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1)
MHD_FN_PAR_IN_ (2) MHD_FN_PAR_IN_ (3)
MHD_FN_PAR_OUT_SIZE_ (6,5) MHD_FN_PAR_OUT_ (7) enum mhd_HpackEncResult
mhd_hpack_enc_field (struct mhd_HpackEncContext *restrict hk_enc,
                     const struct mhd_BufferConst *restrict name,
                     const struct mhd_BufferConst *restrict value,
                     enum mhd_HpackEncPolicy enc_pol,
                     const size_t out_buff_size,
                     uint8_t *restrict out_buff,
                     size_t *restrict bytes_encoded)
{
  size_t pos;
  size_t pos_incr;
  enum mhd_HpackEncResultInternal enc_field_res;

  mhd_assert ((name->size & 0xFFFFFFFFu) == name->size);
  mhd_assert ((value->size & 0xFFFFFFFFu) == value->size);
  mhd_assert ((0u == name->size) || (':' != name->data[0]));

  if (0u == out_buff_size)
    return mhd_HPACK_ENC_BUFFER_TOO_SMALL;

  pos = 0u;

  /* Add Dynamic Table Size Update message if needed */
  if (! hpack_enc_check_dyn_size_update (hk_enc,
                                         out_buff_size - 1u, /* Reserve one byte for minimal field size */
                                         out_buff,
                                         &pos_incr))
    return mhd_HPACK_ENC_BUFFER_TOO_SMALL;

  pos += pos_incr;
  mhd_assert (pos < out_buff_size);

  enc_field_res =
    hpack_enc_field (hk_enc,
                     name,
                     value,
                     enc_pol,
                     out_buff_size - pos,
                     out_buff + pos,
                     &pos_incr);

  if (mhd_ENC_RESULT_INT_NO_SPACE == enc_field_res)
    return mhd_HPACK_ENC_BUFFER_TOO_SMALL;

  pos += pos_incr;

  /* Finally resize the dynamic table (if resize is pending) */
  if (! hpack_enc_perform_dyn_size_update (hk_enc))
    return mhd_HPACK_ENC_RES_ALLOC_ERR;

  /* Add the field (if needed) only after dynamic table resizing (if any) */
  if (mhd_ENC_RESULT_INT_OK_ADD_TO_DYN == enc_field_res)
    mhd_dtbl_new_entry (hk_enc->dyn,
                        name->size,
                        name->data,
                        value->size,
                        value->data);
  else
    mhd_assert (mhd_ENC_RESULT_INT_OK_NO_ADD_TO_DYN == enc_field_res);

  *bytes_encoded = pos;
  return mhd_HPACK_ENC_RES_OK;
}


/**
 * Convert an HTTP status @a code to a three-character decimal string.
 *
 * @param code the status code; must be >= 100 and <= 699
 * @param[out] code_str destination buffer of exactly 3 bytes;
 *                      receives the decimal digits of @a code
 */
mhd_static_inline
MHD_FN_PAR_OUT_ (2) void
status_to_str (uint_fast16_t code,
               char code_str[3])
{
  mhd_assert (100u <= code);
  mhd_assert (699u >= code);

  code_str[0] = (char) ('0' + (char) (uint8_t) ((code / 100u) % 10));
  code_str[1] = (char) ('0' + (char) (uint8_t) ((code /  10u) % 10));
  code_str[2] = (char) ('0' + (char) (uint8_t) ((code /   1u) % 10));
}


/**
 * Pseudo-header ":status" name in the string form
 */
static const struct mhd_BufferConst pf_status_str = mhd_MSTR_INIT (":status");

/**
 * Encode one pseudo-header ":status" according to the requested indexing
 * policy.
 *
 * Chooses between indexed and literal representations based on table contents
 * and the @a enc_pol policy, and decides whether to add the field to the
 * dynamic table (using simple size-based heuristics when not explicitly
 * forced).
 *
 * @param[in,out] hk_enc the encoder context
 * @param[in]     code the status code, must be >= 100 and <= 699
 * @param[in]     enc_pol the encoding policy to apply
 * @param[out]    code_str where the string representation of the @a code
 *                         to be written if literal encoding is used
 * @param[in]     out_buff_size the size of @a out_buff in bytes,
 *                              must not be zero
 * @param[out]    out_buff the output buffer
 * @param[out]    bytes_encoded to be set to the number of bytes written to
 *                              the @a out_buff
 * @return #mhd_ENC_RESULT_INT_NO_SPACE on insufficient buffer;
 *         #mhd_ENC_RESULT_INT_OK_NO_ADD_TO_DYN or
 *         #mhd_ENC_RESULT_INT_OK_ADD_TO_DYN on success
 */
static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1)
MHD_FN_PAR_OUT_ (4)
MHD_FN_PAR_OUT_SIZE_ (6,5) MHD_FN_PAR_OUT_ (7) enum mhd_HpackEncResultInternal
hpack_enc_pf_status (struct mhd_HpackEncContext *restrict hk_enc,
                     uint_fast16_t code,
                     enum mhd_HpackEncPFieldStatusPolicy enc_pol,
                     char code_str[3],
                     const size_t out_buff_size,
                     uint8_t *restrict out_buff,
                     size_t *restrict bytes_encoded)
{
  mhd_constexpr dtbl_idx_ft pf_status_first_idx =
    mhd_HPACK_STBL_PF_STATUS_START_POS;
  mhd_constexpr dtbl_idx_ft pf_status_200_idx = pf_status_first_idx + 0u;
  mhd_constexpr dtbl_idx_ft pf_status_204_idx = pf_status_first_idx + 1u;
  mhd_constexpr dtbl_idx_ft pf_status_206_idx = pf_status_first_idx + 2u;
  mhd_constexpr dtbl_idx_ft pf_status_304_idx = pf_status_first_idx + 3u;
  mhd_constexpr dtbl_idx_ft pf_status_400_idx = pf_status_first_idx + 4u;
  mhd_constexpr dtbl_idx_ft pf_status_404_idx = pf_status_first_idx + 5u;
  mhd_constexpr dtbl_idx_ft pf_status_500_idx = pf_status_first_idx + 6u;
  struct mhd_BufferConst code_val;

  mhd_assert (14u == pf_status_500_idx);

  mhd_assert (0u != out_buff_size);

  /* Check the enum values order */
  // TODO: replace with static asserts
  mhd_assert (mhd_HPACK_ENC_PFS_POL_ALWAYS_NEW_IDX_IF_FIT < \
              mhd_HPACK_ENC_PFS_POL_NORMAL);
  mhd_assert (mhd_HPACK_ENC_PFS_POL_NORMAL < \
              mhd_HPACK_ENC_PFS_POL_AVOID_NEW_IDX);
  mhd_assert (mhd_HPACK_ENC_PFS_POL_AVOID_NEW_IDX < \
              mhd_HPACK_ENC_PFS_POL_STATIC_IDX);
  mhd_assert (mhd_HPACK_ENC_PFS_POL_STATIC_IDX < \
              mhd_HPACK_ENC_PFS_POL_NOT_INDEXED);
  mhd_assert (mhd_HPACK_ENC_PFS_POL_NOT_INDEXED < \
              mhd_HPACK_ENC_PFS_POL_NEVER_W_NAME_IDX);
  mhd_assert (mhd_HPACK_ENC_PFS_POL_NEVER_W_NAME_IDX < \
              mhd_HPACK_ENC_PFS_POL_NEVER_W_NAME_LIT_FORCED);
  mhd_assert (mhd_HPACK_ENC_PFS_POL_NEVER_W_NAME_LIT_FORCED < \
              mhd_HPACK_ENC_PFS_POL_NEVER_W_NAME_LIT_NO_HUFFMAN);


  if ((mhd_HPACK_ENC_PFS_POL_NORMAL <= enc_pol)
      && (mhd_HPACK_ENC_PFS_POL_STATIC_IDX >= enc_pol))
  {
    dtbl_idx_ft field_idx;
    switch (code)
    {
    case 200u:
      field_idx = pf_status_200_idx;
      break;
    case 204u:
      field_idx = pf_status_204_idx;
      break;
    case 206u:
      field_idx = pf_status_206_idx;
      break;
    case 304u:
      field_idx = pf_status_304_idx;
      break;
    case 400u:
      field_idx = pf_status_400_idx;
      break;
    case 404u:
      field_idx = pf_status_404_idx;
      break;
    case 500u:
      field_idx = pf_status_500_idx;
      break;
    default:
      field_idx = 0u;
      break;
    }

    if (0u != field_idx)
    {
      if (! hpack_enc_field_indexed (field_idx,
                                     out_buff_size,
                                     out_buff,
                                     bytes_encoded))
        return mhd_ENC_RESULT_INT_NO_SPACE;

      return mhd_ENC_RESULT_INT_OK_NO_ADD_TO_DYN;
    }
  }

  /* The pseudo-header is not in the static table or should not be added as an
     indexed field */

  /* Create a string representation of the code */
  status_to_str (code, code_str);
  code_val.data = code_str;
  code_val.size = 3u;

  if ((mhd_HPACK_ENC_PFS_POL_NORMAL <= enc_pol)
      && (mhd_HPACK_ENC_PFS_POL_AVOID_NEW_IDX >= enc_pol))
  {
    const dtbl_idx_ft field_idx =
      mhd_dtbl_find_entry (hk_enc->dyn,
                           pf_status_str.size,
                           pf_status_str.data,
                           3u,
                           code_str);

    if (0u != field_idx)
    {
      if (! hpack_enc_field_indexed (field_idx,
                                     out_buff_size,
                                     out_buff,
                                     bytes_encoded))
        return mhd_ENC_RESULT_INT_NO_SPACE;

      return mhd_ENC_RESULT_INT_OK_NO_ADD_TO_DYN;
    }
  }

  /* The field is not in the tables or should not be added as an indexed
     field */

  /* Add the field literally */

  if (mhd_HPACK_ENC_PFS_POL_NEVER_W_NAME_IDX <= enc_pol)
  {
    /* Add field literally as "never indexed" */
    const bool name_idx_stat_allowed =
      (mhd_HPACK_ENC_PFS_POL_NEVER_W_NAME_IDX == enc_pol);
    const bool huffman_allowed =
      (mhd_HPACK_ENC_PFS_POL_NEVER_W_NAME_LIT_NO_HUFFMAN > enc_pol);
    if (! hpack_enc_field_literal (hk_enc,
                                   &pf_status_str,
                                   name_idx_stat_allowed ?
                                   pf_status_first_idx : 0u,
                                   &code_val,
                                   mhd_HPACK_ENC_LIT_IDX_TYPE_NEVER_INDEXING,
                                   name_idx_stat_allowed,
                                   false,
                                   huffman_allowed,
                                   out_buff_size,
                                   out_buff,
                                   bytes_encoded))
      return mhd_ENC_RESULT_INT_NO_SPACE;

    return mhd_ENC_RESULT_INT_OK_NO_ADD_TO_DYN;
  }

  if (mhd_HPACK_ENC_PFS_POL_AVOID_NEW_IDX <= enc_pol)
  {
    /* Adding to the tables is not allowed */
    mhd_assert (mhd_HPACK_ENC_PFS_POL_NOT_INDEXED >= enc_pol);

    if (! hpack_enc_field_literal (hk_enc,
                                   &pf_status_str,
                                   pf_status_first_idx,
                                   &code_val,
                                   mhd_HPACK_ENC_LIT_IDX_TYPE_NOT_INDEXING,
                                   true,
                                   false,
                                   true,
                                   out_buff_size,
                                   out_buff,
                                   bytes_encoded))
      return mhd_ENC_RESULT_INT_NO_SPACE;

    return mhd_ENC_RESULT_INT_OK_NO_ADD_TO_DYN;
  }

  mhd_assert (mhd_HPACK_ENC_PFS_POL_ALWAYS_NEW_IDX_IF_FIT <= enc_pol);
  mhd_assert (mhd_HPACK_ENC_PFS_POL_NORMAL >= enc_pol);

  if (1) /* For local scope */
  {
    const bool add_to_idx =
      mhd_dtbl_check_entry_fit (hk_enc->dyn,
                                pf_status_str.size,
                                3u);

    if (hpack_enc_field_literal (hk_enc,
                                 &pf_status_str,
                                 pf_status_first_idx,
                                 &code_val,
                                 add_to_idx ?
                                 mhd_HPACK_ENC_LIT_IDX_TYPE_INDEXING :
                                 mhd_HPACK_ENC_LIT_IDX_TYPE_NOT_INDEXING,
                                 true,
                                 false,
                                 true,
                                 out_buff_size,
                                 out_buff,
                                 bytes_encoded))
      return add_to_idx ?
             mhd_ENC_RESULT_INT_OK_ADD_TO_DYN :
             mhd_ENC_RESULT_INT_OK_NO_ADD_TO_DYN;

  }

  return mhd_ENC_RESULT_INT_NO_SPACE;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1)
MHD_FN_PAR_OUT_SIZE_ (5,4) MHD_FN_PAR_OUT_ (6) enum mhd_HpackEncResult
mhd_hpack_enc_ph_status (struct mhd_HpackEncContext *restrict hk_enc,
                         uint_fast16_t code,
                         enum mhd_HpackEncPFieldStatusPolicy enc_pol,
                         const size_t out_buff_size,
                         uint8_t *restrict out_buff,
                         size_t *restrict bytes_encoded)
{
  char code_str[3] = "";
  size_t pos;
  size_t pos_incr;
  enum mhd_HpackEncResultInternal enc_field_res;

  mhd_assert (100u <= code);
  mhd_assert (699u >= code);

  if (0u == out_buff_size)
    return mhd_HPACK_ENC_BUFFER_TOO_SMALL;

  pos = 0u;

  /* Add Dynamic Table Size Update message if needed */
  if (! hpack_enc_check_dyn_size_update (hk_enc,
                                         out_buff_size - 1u, /* Reserve one byte for minimal field size */
                                         out_buff,
                                         &pos_incr))
    return mhd_HPACK_ENC_BUFFER_TOO_SMALL;

  pos += pos_incr;
  mhd_assert (pos < out_buff_size);

  enc_field_res =
    hpack_enc_pf_status (hk_enc,
                         code,
                         enc_pol,
                         code_str,
                         out_buff_size,
                         out_buff,
                         &pos_incr);

  if (mhd_ENC_RESULT_INT_NO_SPACE == enc_field_res)
    return mhd_HPACK_ENC_BUFFER_TOO_SMALL;

  pos += pos_incr;

  /* Finally resize the dynamic table (if resize is pending) */
  if (! hpack_enc_perform_dyn_size_update (hk_enc))
    return mhd_HPACK_ENC_RES_ALLOC_ERR;

  /* Add the field (if needed) only after dynamic table resizing (if any) */
  if (mhd_ENC_RESULT_INT_OK_ADD_TO_DYN == enc_field_res)
  {
    mhd_assert ('1' <= code_str[0]);
    mhd_assert ('6' >= code_str[0]);
    mhd_assert ('0' == code_str[1]);
    mhd_dtbl_new_entry (hk_enc->dyn,
                        pf_status_str.size,
                        pf_status_str.data,
                        sizeof(code_str) / sizeof(char),
                        code_str);
  }
  else
    mhd_assert (mhd_ENC_RESULT_INT_OK_NO_ADD_TO_DYN == enc_field_res);

  *bytes_encoded = pos;
  return mhd_HPACK_ENC_RES_OK;
}


/* ****** _____________ End of HPACK headers encoding ______________ ****** */

#endif /* ! mhd_HPACK_TESTING_TABLES_ONLY || ! MHD_UNIT_TESTING */
