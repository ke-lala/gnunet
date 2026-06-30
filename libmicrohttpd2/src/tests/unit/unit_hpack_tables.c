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
 * @file src/test/unit/unit_hpack_tables.c
 * @brief  The tests for HPACK tables functions
 * @author Karlson2k (Evgeny Grin)
 */


#include "mhd_sys_options.h"

#define mhd_HPACK_TESTING_TABLES_ONLY      1
/* Include .c file (!) to access static functions.
   Only official interface mhd_* is used to handle the tables. */
#include "h2/hpack/mhd_hpack_codec.c"

#include "mhdt_has_in_name.h"
#include "mhdt_has_param.h"
#include "mhdt_checks.h"

#ifndef MHD_ENABLE_SLOW_TESTS
static int mhdtl_enable_deep_tests = 0;
#else
static int mhdtl_enable_deep_tests = ! 0;
#endif

#define MHDTL_DYN_ENTRY_MIN_SIZE        mhd_HPACK_ENTRY_OVERHEAD

/*
 * Local test helpers
 */

#define MHDTL_HPACK_ENTRY_SIZE(name_len,val_len) \
        (name_len + val_len + mhd_HPACK_ENTRY_OVERHEAD)


static inline uint_least16_t
mhdtl_rotl16 (uint_least16_t val, unsigned int rot)
{
  return
    (uint_least16_t)
    (((val & 0xFFFFu) << (rot % 16u))
     | ((val & 0xFFFFu) >> ((16u - (rot % 16u)) % 16u)));
}


static inline uint_least16_t
mhdtl_mix16 (uint_fast16_t a, uint_fast16_t b)
{
  /* Mix with Golden Ratio's fractional part */
  uint_least16_t res =
    (uint_least16_t) (((a & 0xFFFFu) * 0x9E37u)
                      ^ mhdtl_rotl16 ((uint_least16_t) b, 5));
  /* Re-mix bits better */
  res ^= (uint_least16_t) (res >> 7);
  res = (uint_least16_t) ((res * 0x9E37u) & 0xFFFFu);
  res ^= (uint_least16_t) (res >> 9);

  return res;
}


static size_t
mhdtl_dyn_get_size_used (const struct mhd_HpackDTblContext *dyn)
{
  const size_t used = mhd_dtbl_get_table_used (dyn);
  MHDT_EXPECT_UINT_GE_VAL (mhd_dtbl_get_table_max_size (dyn), \
                           used);
  return used;
}


static size_t
mhdtl_dyn_get_free (const struct mhd_HpackDTblContext *dyn)
{
  const size_t used = mhd_dtbl_get_table_used (dyn);
  const size_t max_size = mhd_dtbl_get_table_max_size (dyn);
  MHDT_EXPECT_UINT_GE_VAL (max_size, used);
  return max_size - used;
}


#define MHDTL_DTBL_FIND_ENTRY_STR(dyn,name,val) \
        mhd_dtbl_find_entry (dyn, strlen (name), name, strlen (val), val)

#define MHDTL_DTBL_FIND_NAME_STR(dyn,name) \
        mhd_dtbl_find_name (dyn, strlen (name), name)


/* Return non-zero if succeeded, zero if failed */
static int
mhdtl_dyn_check_get_valid_invalid (const struct mhd_HpackDTblContext *dyn)
{
  const dtbl_idx_ft num_entries = (dtbl_idx_t) mhd_dtbl_get_num_entries (dyn);
  struct mhd_BufferConst check_name;
  struct mhd_BufferConst check_value;

  if (! MHDT_EXPECT_FALSE_D (mhd_dtbl_get_entry (dyn, \
                                                 mhd_HPACK_STBL_LAST_IDX + 1u \
                                                 + num_entries, \
                                                 &check_name, \
                                                 &check_value), \
                             "getting entry with the index outside valid "
                             "range should fail"))
    return 0;

  if (0u == num_entries)
    return ! 0;

  if (! MHDT_EXPECT_TRUE_D (mhd_dtbl_get_entry (dyn, \
                                                mhd_HPACK_STBL_LAST_IDX + 1u, \
                                                &check_name, \
                                                &check_value), \
                            "non-empty table should return successfully " \
                            "the first valid entry"))
    return 0;
  if (! MHDT_EXPECT_TRUE_D (mhd_dtbl_get_entry (dyn, \
                                                mhd_HPACK_STBL_LAST_IDX \
                                                + num_entries, \
                                                &check_name, \
                                                &check_value), \
                            "non-empty table should return successfully " \
                            "the last valid entry"))
    return 0;

  return ! 0;
}


/* Return non-zero if succeeded, zero if failed */
static int
mhdtl_dyn_check_entry_n_at_idx (struct mhd_HpackDTblContext *dyn,
                                size_t idx,
                                size_t expect_name_len,
                                const char *expect_name,
                                size_t expect_value_len,
                                const char *expect_value)
{
  struct mhd_BufferConst check_name;
  struct mhd_BufferConst check_value;

  dtbl_idx_t idx_idx = (dtbl_idx_t) idx;
  if (0 == idx)
    MHDT_ERR_EXIT ();
  if (idx_idx != idx)
    MHDT_ERR_EXIT ();

  if (MHDT_EXPECT_TRUE_D (mhd_dtbl_get_entry (dyn,        \
                                              idx_idx,     \
                                              &check_name,  \
                                              &check_value), \
                          "the entry with specified index should exist"))
  {
    MHDT_EXPECT_STRN_EQ_STR_D (check_name.size, \
                               check_name.data, \
                               expect_name,     \
                               "the name of the entry should match " \
                               "expected value");
    MHDT_EXPECT_STRN_EQ_STR_D (check_value.size, \
                               check_value.data, \
                               expect_value,     \
                               "the name of the entry should match " \
                               "expected value");
  }
  else
    return 0;

  /* It should be possible to find the entry. The index could be different
     if entries are duplicated. */

  MHDT_EXPECT_UINT_GT_VAL_D (mhd_dtbl_find_entry (dyn, \
                                                  expect_name_len, \
                                                  expect_name, \
                                                  expect_value_len, \
                                                  expect_value), \
                             mhd_HPACK_STBL_LAST_IDX, \
                             "search by known entry name and value " \
                             "should succeed");
  MHDT_EXPECT_UINT_GT_VAL_D (mhd_dtbl_find_name (dyn, \
                                                 expect_name_len, \
                                                 expect_name), \
                             mhd_HPACK_STBL_LAST_IDX, \
                             "search by known entry name " \
                             "should succeed");

  return ! 0;
}


/* Return non-zero if succeeded, zero if failed */
static int
mhdtl_dyn_check_entry_at_idx (struct mhd_HpackDTblContext *dyn,
                              size_t idx,
                              const char *expect_name,
                              const char *expect_value)
{
  return mhdtl_dyn_check_entry_n_at_idx (dyn,
                                         idx,
                                         strlen (expect_name),
                                         expect_name,
                                         strlen (expect_value),
                                         expect_value);
}


/* Return non-zero if succeeded, zero if failed */
static MHD_FN_PAR_IN_SIZE_ (3,2) MHD_FN_PAR_IN_SIZE_ (5,4) int
mhdtl_dyn_add_hdr_n_with_check (struct mhd_HpackDTblContext *dyn,
                                size_t hdr_name_len,
                                const char *hdr_name,
                                size_t hdr_value_len,
                                const char *hdr_value)
{
  char prev_name_buf[512];
  size_t prev_name_len = 0u;
  char prev_val_buf[512];
  size_t prev_val_len = 0u;
  int have_prev_entry = 0;
  char last_name_buf[512];
  size_t last_name_len = 0u;
  char last_val_buf[512];
  size_t last_val_len = 0u;
  int have_last_entry = 0;
  const size_t hdr_size = MHDTL_HPACK_ENTRY_SIZE (hdr_name_len, hdr_value_len);
  const size_t table_max_size = mhd_dtbl_get_table_max_size (dyn);
  const size_t table_free_before = mhdtl_dyn_get_free (dyn);
  const dtbl_idx_ft num_entries_before = (dtbl_idx_t)
                                         mhd_dtbl_get_num_entries (dyn);
  struct mhd_BufferConst check_name = {0u, NULL};
  struct mhd_BufferConst check_value = {0u, NULL};

  if ((mhd_DTBL_MAX_SIZE < hdr_name_len) ||
      (mhd_DTBL_MAX_SIZE < hdr_value_len) ||
      (mhd_DTBL_MAX_SIZE < hdr_size))
    MHDT_ERR_EXIT ();

  MHDT_EXPECT_UINT_GE_VAL (table_max_size, table_free_before);
  MHDT_EXPECT_UINT_GE_VAL_D (mhdtl_dyn_get_size_used (dyn), \
                             num_entries_before * MHDTL_DYN_ENTRY_MIN_SIZE, \
                             "Each entry should have at least a minimal size");

  if (mhd_dtbl_get_entry (dyn,
                          mhd_HPACK_STBL_LAST_IDX + 1u,
                          &check_name, &check_value))
  { /* Check whether the current newest entry and the new entry will fit
       the table together */
    if (hdr_size + MHDTL_HPACK_ENTRY_SIZE (check_name.size, check_value.size)
        <= mhd_dtbl_get_table_max_size (dyn))
    {
      if ((sizeof(prev_name_buf) > check_name.size)
          && (sizeof(prev_val_buf) > check_value.size))
      {
        /* Save the first entry to check later that it is not changed after
           adding the new entry */
        memcpy (prev_name_buf,
                check_name.data,
                check_name.size);
        prev_name_buf[check_name.size] = 0; /* Zero-terminate for convenience */
        prev_name_len = check_name.size;
        memcpy (prev_val_buf,
                check_value.data,
                check_value.size);
        prev_val_buf[check_value.size] = 0; /* Zero-terminate for convenience */
        prev_val_len = check_value.size;
        have_prev_entry = ! 0;
      }
    }
  }
  if ((1u < mhd_dtbl_get_num_entries (dyn))
      && (hdr_size <= table_free_before))
  {
    if (MHDT_EXPECT_TRUE (mhd_dtbl_get_entry (dyn,
                                              mhd_HPACK_STBL_LAST_IDX
                                              + num_entries_before,
                                              &check_name, &check_value)))
    {
      if ((sizeof(last_name_buf) > check_name.size)
          && (sizeof(last_val_buf) > check_value.size))
      {
        /* Save the last entry to check later that it is not changed after
           adding the new entry */
        memcpy (last_name_buf,
                check_name.data,
                check_name.size);
        last_name_buf[check_name.size] = 0; /* Zero-terminate for convenience */
        last_name_len = check_name.size;
        memcpy (last_val_buf,
                check_value.data,
                check_value.size);
        last_val_buf[check_value.size] = 0; /* Zero-terminate for convenience */
        last_val_len = check_value.size;
        have_last_entry = ! 0;
      }
    }
  }

  mhd_dtbl_new_entry (dyn,
                      hdr_name_len,
                      hdr_name,
                      hdr_value_len,
                      hdr_value);

  if (hdr_size <= table_free_before)
  {
    /* No eviction */
    MHDT_EXPECT_UINT_EQ_VAL_D (mhd_dtbl_get_num_entries (dyn), \
                               num_entries_before + 1u, \
                               "added one entry without eviction");
    MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_free (dyn), \
                             table_free_before - hdr_size);
    if (have_last_entry)
      mhdtl_dyn_check_entry_n_at_idx (dyn,
                                      mhd_HPACK_STBL_LAST_IDX
                                      + num_entries_before + 1u,
                                      last_name_len,
                                      last_name_buf,
                                      last_val_len,
                                      last_val_buf);
  }
  else
  {
    /* Eviction */
    MHDT_EXPECT_UINT_LE_VAL_D (mhd_dtbl_get_num_entries (dyn), \
                               num_entries_before,
                               "some entries evicted when adding new entry");
    if (hdr_size > table_max_size)
    {
      /* Full eviction */
      MHDT_EXPECT_UINT_EQ_VAL_D (mhd_dtbl_get_num_entries (dyn), 0u, \
                                 "all entries evicted by adding too large " \
                                 "new entry");
      MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_free (dyn), table_max_size);
      MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_table_max_size (dyn), \
                               table_max_size);
      return ! 0;
    }
    else if (hdr_size + MHDTL_DYN_ENTRY_MIN_SIZE > table_max_size)
      MHDT_EXPECT_UINT_EQ_VAL_D (mhd_dtbl_get_num_entries (dyn), 1u, \
                                 "exactly one new entry fits the table");
    else
      MHDT_EXPECT_UINT_GE_VAL_D (mhd_dtbl_get_num_entries (dyn), 1u,
                                 "some entries evicted and one entry added");

  }

  /* The new entry has been added to the table */

  if (1u == mhd_dtbl_get_num_entries (dyn))
    MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), hdr_size);
  else
    MHDT_EXPECT_UINT_GE_VAL (mhdtl_dyn_get_size_used (dyn), \
                             hdr_size + MHDTL_DYN_ENTRY_MIN_SIZE);

  if (! MHDT_EXPECT_TRUE_D (mhd_dtbl_get_entry (dyn, \
                                                mhd_HPACK_STBL_LAST_IDX + 1u, \
                                                &check_name, \
                                                &check_value), \
                            "new entry should be available as the newest " \
                            "entry in the dynamic table"))
    return 0;

  if (! MHDT_EXPECT_STRN_EQ_D (check_name.size, \
                               check_name.data, \
                               hdr_name_len, \
                               hdr_name, \
                               "The name of the newest entry should " \
                               "match the last added entry's name"))
    return 0;
  if (! MHDT_EXPECT_STRN_EQ_D (check_value.size, \
                               check_value.data, \
                               hdr_value_len, \
                               hdr_value, \
                               "The value of the newest entry should " \
                               "match the last added entry's value"))
    return 0;

  if (! MHDT_EXPECT_UINT_GT_VAL_D (mhd_dtbl_find_entry (dyn, \
                                                        hdr_name_len, \
                                                        hdr_name, \
                                                        hdr_value_len, \
                                                        hdr_value), \
                                   mhd_HPACK_STBL_LAST_IDX, \
                                   "search for the newly added entry should " \
                                   "be successful"))
    return 0;
  if (! MHDT_EXPECT_UINT_GT_VAL_D (mhd_dtbl_find_name (dyn, \
                                                       hdr_name_len, \
                                                       hdr_name), \
                                   mhd_HPACK_STBL_LAST_IDX, \
                                   "search for the newly added entry name "
                                   "should be successful"))
    return 0;

  if (have_prev_entry)
  {
    if (MHDT_EXPECT_TRUE_D (mhd_dtbl_get_entry (dyn, \
                                                mhd_HPACK_STBL_LAST_IDX + 2u, \
                                                &check_name, &check_value), \
                            "previous entry must remain the same"))
    {
      MHDT_EXPECT_STRN_EQ_D (check_name.size, check_name.data, \
                             prev_name_len, prev_name_buf, \
                             "previous entry must be unchanged");
      MHDT_EXPECT_STRN_EQ_D (check_value.size, check_value.data, \
                             prev_val_len, prev_val_buf, \
                             "previous entry must be unchanged");
    }
  }


  mhdtl_dyn_check_get_valid_invalid (dyn);

  return ! 0;
}


/* Return non-zero if succeeded, zero if failed */
static MHD_FN_PAR_CSTR_ (2) MHD_FN_PAR_CSTR_ (3) int
mhdtl_dyn_add_hdr_with_check (struct mhd_HpackDTblContext *dyn,
                              const char *hdr_name,
                              const char *hdr_value)
{
  const size_t hdr_name_len = strlen (hdr_name);
  const size_t hdr_value_len = strlen (hdr_value);

  return mhdtl_dyn_add_hdr_n_with_check (dyn,
                                         hdr_name_len,
                                         hdr_name,
                                         hdr_value_len,
                                         hdr_value);
}


/* Return non-zero if succeeded, zero if failed */
static int
mhdtl_dyn_add_sized_strs_with_check (struct mhd_HpackDTblContext *dyn,
                                     size_t hdr_strings_size)
{
#define MHDTL_NAMES_BUF_SIZE 512
  static char names[MHDTL_NAMES_BUF_SIZE] = "";
  static char values[MHDTL_NAMES_BUF_SIZE] = "";
#undef MHDTL_NAMES_BUF_SIZE
  /* Use '- 1u' to have valid addresses for zero-sized names and values */
  static const size_t bufs_size = sizeof(names) - 1u;
  static size_t names_off = 0u;
  static size_t values_off = 0u;
  size_t name_size;
  size_t value_size;
  char *hdr_name;
  char *hdr_value;

  if ((bufs_size + bufs_size) < hdr_strings_size)
    MHDT_ERR_EXIT_D ("the entry must not be larger than a hardcoded limit");

  if (0 == values[0])
  {
    size_t i;
    for (i = 0; i < sizeof(names); ++i)
    {
      names[i] = (char) ('a' + (char) (i % ('z' - 'a' + 1)));
      values[i] = (char) (' ' + (char) ((bufs_size - i) % ('~' - ' ' + 1)));
    }
  }

  value_size = hdr_strings_size / 2;
  name_size = hdr_strings_size - value_size;

  hdr_name =
    names + (names_off++) % (bufs_size - name_size + 1u);
  hdr_value =
    values + (bufs_size - value_size)
    - ((values_off++) % (bufs_size - value_size + 1u));

  return mhdtl_dyn_add_hdr_n_with_check (dyn,
                                         name_size,
                                         hdr_name,
                                         value_size,
                                         hdr_value);
}


/* Return non-zero if succeeded, zero if failed */
static int
mhdtl_dyn_add_sized_entry_with_check (struct mhd_HpackDTblContext *dyn,
                                      size_t entry_size)
{
  if (MHDTL_DYN_ENTRY_MIN_SIZE > entry_size)
    MHDT_ERR_EXIT_D ("the entry must be at least a minimal entry size");

  return mhdtl_dyn_add_sized_strs_with_check (dyn,
                                              entry_size
                                              - mhd_HPACK_ENTRY_OVERHEAD);
}


static void
mhdtl_stat_check_entry_n_at_idx (size_t idx,
                                 size_t expect_name_len,
                                 const char *expect_name,
                                 size_t expect_value_len,
                                 const char *expect_value)
{
  struct mhd_BufferConst check_name;
  struct mhd_BufferConst check_value;

  dtbl_idx_t idx_idx = (dtbl_idx_t) idx;
  if (0 == idx)
    MHDT_ERR_EXIT ();
  if (idx_idx != idx)
    MHDT_ERR_EXIT ();
  if (0u == idx)
    MHDT_ERR_EXIT ();
  if (mhd_HPACK_STBL_LAST_IDX < idx)
    MHDT_ERR_EXIT ();
  if (0u == expect_name_len)
    MHDT_ERR_EXIT ();
  if ((':' == expect_name[0]) != (idx < (mhd_HPACK_STBL_NORM_START_POS + 1u)))
    MHDT_ERR_EXIT ();

  mhd_stbl_get_entry (idx_idx,
                      &check_name,
                      &check_value);

  MHDT_EXPECT_STRN_EQ_STR_D (check_name.size, \
                             check_name.data, \
                             expect_name, \
                             "the name of the entry should match " \
                             "expected value");
  MHDT_EXPECT_STRN_EQ_STR_D (check_value.size, \
                             check_value.data, \
                             expect_value, \
                             "the name of the entry should match " \
                             "expected value");

  /* It should be possible to find the entry. The index could be different
     if entries are duplicated. */

  if (':' != expect_name[0])
  {
    MHDT_EXPECT_UINT_NE_VAL_D (mhd_stbl_find_entry_real (expect_name_len, \
                                                         expect_name, \
                                                         expect_value_len,
                                                         expect_value), \
                               0u, \
                               "search by known entry name and value " \
                               "should not fail");
    MHDT_EXPECT_UINT_EQ_VAL_D (mhd_stbl_find_entry_real (expect_name_len, \
                                                         expect_name, \
                                                         expect_value_len,
                                                         expect_value), \
                               idx, \
                               "search by known entry name and value " \
                               "should return the " \
                               "known entry index");

    MHDT_EXPECT_UINT_NE_VAL_D (mhd_stbl_find_name_real (expect_name_len, \
                                                        expect_name), \
                               0u, \
                               "search by known entry name " \
                               "should not fail");
    MHDT_EXPECT_UINT_LE_VAL_D (mhd_stbl_find_name_real (expect_name_len, \
                                                        expect_name), \
                               idx, \
                               "search by known entry name " \
                               "should return value not greater than the " \
                               "known entry index");
  }
  else
  {
    MHDT_EXPECT_UINT_EQ_VAL_D (mhd_stbl_find_entry_real (expect_name_len, \
                                                         expect_name, \
                                                         expect_value_len,
                                                         expect_value), \
                               0u, \
                               "search by pseudo-header entry name " \
                               "and value should fail");

    MHDT_EXPECT_UINT_EQ_VAL_D (mhd_stbl_find_name_real (expect_name_len, \
                                                        expect_name), \
                               0u, \
                               "search by pseudo-header entry name " \
                               "should fail");
  }
}


static void
mhdtl_stat_check_entry_at_idx (size_t idx,
                               const char *expect_name,
                               const char *expect_value)
{
  mhdtl_stat_check_entry_n_at_idx (idx,
                                   strlen (expect_name),
                                   expect_name,
                                   strlen (expect_value),
                                   expect_value);
}


#define MHDTL_STBL_FIND_ENTRY_REAL_STR(name,value) \
        mhd_stbl_find_entry_real (strlen (name),(name), \
                                  strlen (value),(value))
#define MHDTL_STBL_FIND_NAME_REAL_STR(name) \
        mhd_stbl_find_name_real (strlen (name),(name))


/* Return non-zero if succeeded, zero if failed */
static int
mhdtl_comb_check_entry_n_at_idx (struct mhd_HpackDTblContext *dyn,
                                 size_t idx,
                                 size_t expect_name_len,
                                 const char *expect_name,
                                 size_t expect_value_len,
                                 const char *expect_value)
{
  struct mhd_BufferConst check_name;
  struct mhd_BufferConst check_value;

  dtbl_idx_t idx_idx = (dtbl_idx_t) idx;
  dtbl_idx_t found_by_name_and_value;
  dtbl_idx_t found_by_name_only;
  if (0 == idx)
    MHDT_ERR_EXIT ();
  if (idx_idx != idx)
    MHDT_ERR_EXIT ();
  if ((':' == expect_name[0]) != (idx < (mhd_HPACK_STBL_NORM_START_POS + 1u)))
    MHDT_ERR_EXIT_D ("pseudo-header entries are allowed only at first " \
                     "static table positions");

  if (MHDT_EXPECT_TRUE_D (mhd_htbl_get_entry (dyn,        \
                                              idx_idx,     \
                                              &check_name,  \
                                              &check_value), \
                          "the entry with specified index should exist"))
  {
    MHDT_EXPECT_STRN_EQ_STR_D (check_name.size, \
                               check_name.data, \
                               expect_name,     \
                               "the name of the entry should match " \
                               "expected value");
    MHDT_EXPECT_STRN_EQ_STR_D (check_value.size, \
                               check_value.data, \
                               expect_value,     \
                               "the name of the entry should match " \
                               "expected value");
  }
  else
    return 0;

  /* It should be possible to find the entry. */
  found_by_name_and_value =
    mhd_htbl_find_entry_real (dyn,
                              expect_name_len,
                              expect_name,
                              expect_value_len,
                              expect_value);
  found_by_name_only =
    mhd_htbl_find_name_real (dyn,
                             expect_name_len,
                             expect_name);

  if ((0u == expect_name_len) ||
      (':' != expect_name[0]))
  {
    MHDT_EXPECT_UINT_NE_VAL_D (found_by_name_and_value, \
                               0u, \
                               "search by known entry name and value " \
                               "should not fail");
    MHDT_EXPECT_UINT_LE_VAL_D (found_by_name_and_value, \
                               mhd_HPACK_STBL_LAST_IDX \
                               + mhd_dtbl_get_num_entries (dyn), \
                               "search by known entry name and value " \
                               "should return a valid index number");
    if (mhd_HPACK_STBL_LAST_IDX >= idx)
      MHDT_EXPECT_UINT_EQ_VAL_D (found_by_name_and_value, \
                                 idx, \
                                 "search by known entry name and value " \
                                 "should return the " \
                                 "known entry index");


    MHDT_EXPECT_UINT_NE_VAL_D (found_by_name_only, \
                               0u, \
                               "search by known entry name " \
                               "should not fail");
    MHDT_EXPECT_UINT_LE_VAL_D (found_by_name_only, \
                               mhd_HPACK_STBL_LAST_IDX \
                               + mhd_dtbl_get_num_entries (dyn), \
                               "search by known entry name " \
                               "should return a valid index number");
    /* The next check is not required by RFC, but guaranteed by implementation
       design. */
    if (mhd_HPACK_STBL_LAST_IDX >= idx)
      MHDT_EXPECT_UINT_LE_VAL_D (found_by_name_and_value, \
                                 mhd_HPACK_STBL_LAST_IDX, \
                                 "search by known static entry name " \
                                 "should return the " \
                                 "static table index");
  }
  else
  {
    MHDT_EXPECT_UINT_EQ_VAL_D (found_by_name_and_value, \
                               0u, \
                               "search by pseudo-header entry name " \
                               "and value should fail");

    MHDT_EXPECT_UINT_EQ_VAL_D (found_by_name_only, \
                               0u, \
                               "search by pseudo-header entry name " \
                               "should fail");
  }

  return ! 0;
}


/* Return non-zero if succeeded, zero if failed */
static int
mhdtl_comb_check_entry_at_idx (struct mhd_HpackDTblContext *dyn,
                               size_t idx,
                               const char *expect_name,
                               const char *expect_value)
{
  return mhdtl_comb_check_entry_n_at_idx (dyn,
                                          idx,
                                          strlen (expect_name),
                                          expect_name,
                                          strlen (expect_value),
                                          expect_value);
}


#define MHDTL_HTBL_FIND_ENTRY_REAL_STR(dyn,name,value) \
        mhd_htbl_find_entry_real (dyn,strlen (name),(name), \
                                  strlen (value),(value))
#define MHDTL_HTBL_FIND_NAME_REAL_STR(dyn,name) \
        mhd_htbl_find_name_real (dyn,strlen (name),(name))


/*
 * The dynamic table tests
 */

static int
test_dyn_create_destroy (void)
{
  struct mhd_HpackDTblContext*dyn;

  MHDT_EXPECT_PTR_NONNULL_D (dyn = \
                               mhd_dtbl_create (mhd_hpack_def_dyn_table_size), \
                             "Create HPACK dynamic table");
  if (NULL == dyn)
    return MHDT_TEST_RESULT ();

  MHDT_EXPECT_UINT_EQ_VAL_D (mhd_dtbl_get_num_entries (dyn), 0u, \
                             "an empty table must have no entries");
  mhdtl_dyn_check_get_valid_invalid (dyn);

  mhd_dtbl_destroy (dyn);

  return MHDT_TEST_RESULT ();
}


static int
test_dyn_create_destroy_larger (void)
{
  struct mhd_HpackDTblContext*dyn;

  MHDT_EXPECT_PTR_NONNULL_D (dyn = mhd_dtbl_create (59u * 1024u), \
                             "Create HPACK dynamic table");
  if (NULL == dyn)
    return MHDT_TEST_RESULT ();

  MHDT_EXPECT_UINT_EQ_VAL_D (mhd_dtbl_get_num_entries (dyn), 0u, \
                             "an empty table must have no entries");
  mhdtl_dyn_check_get_valid_invalid (dyn);

  mhd_dtbl_destroy (dyn);

  MHDT_EXPECT_PTR_NONNULL_D (dyn = mhd_dtbl_create (mhd_DTBL_MAX_SIZE), \
                             "Create HPACK dynamic table with maximum size");
  if (NULL == dyn)
    return MHDT_TEST_RESULT ();

  /* Add some entries with checks */
  mhdtl_dyn_add_sized_entry_with_check (dyn, 123u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 234u);
  mhdtl_dyn_check_get_valid_invalid (dyn);

  mhd_dtbl_destroy (dyn);

  return MHDT_TEST_RESULT ();
}


static int
test_dyn_add_three (void)
{
  struct mhd_HpackDTblContext*dyn;

  MHDT_EXPECT_PTR_NONNULL_D (dyn = mhd_dtbl_create (256u), \
                             "Create HPACK dynamic table");
  if (NULL == dyn)
    return MHDT_TEST_RESULT ();

  mhdtl_dyn_add_hdr_with_check (dyn, "header1", "value1");
  mhdtl_dyn_add_hdr_with_check (dyn, "header2", "longer value of the header2");
  mhdtl_dyn_add_hdr_with_check (dyn, "Header3", "value of the header with the "
                                "uppercase name");

  mhd_dtbl_destroy (dyn);

  return MHDT_TEST_RESULT ();
}


static int
test_dyn_add_empty (void)
{
  struct mhd_HpackDTblContext*dyn;

  MHDT_EXPECT_PTR_NONNULL_D (dyn = mhd_dtbl_create (256u), \
                             "Create HPACK dynamic table");
  if (NULL == dyn)
    return MHDT_TEST_RESULT ();

  mhdtl_dyn_add_hdr_with_check (dyn, "", "");
  mhdtl_dyn_add_hdr_with_check (dyn, "h", "");
  mhdtl_dyn_add_hdr_with_check (dyn, "", "v");

  mhd_dtbl_destroy (dyn);

  return MHDT_TEST_RESULT ();
}


static int
test_dyn_add_evict_simple (void)
{
  struct mhd_HpackDTblContext*dyn;

  MHDT_EXPECT_PTR_NONNULL_D (dyn = mhd_dtbl_create (128u), \
                             "Create HPACK dynamic table");
  if (NULL == dyn)
    return MHDT_TEST_RESULT ();

  mhdtl_dyn_add_hdr_with_check (dyn, "header1", "value1");
  mhdtl_dyn_add_hdr_with_check (dyn, "header2", "longer value of the header2");
  mhdtl_dyn_add_hdr_with_check (dyn, "Header3", "value of the header with the "
                                "uppercase name");
  mhdtl_dyn_add_hdr_with_check (dyn, "header4", "smaller header");
  mhdtl_dyn_add_hdr_with_check (dyn, "header5", "value5");

  mhd_dtbl_destroy (dyn);

  return MHDT_TEST_RESULT ();
}


static int
test_dyn_add_evict_wrap (void)
{
  struct mhd_HpackDTblContext*dyn;

  MHDT_EXPECT_PTR_NONNULL_D (dyn = mhd_dtbl_create (280u), \
                             "Create HPACK dynamic table");
  if (NULL == dyn)
    return MHDT_TEST_RESULT ();

#define MHDTL_H01N "notably-long-name1"         /* length: 18 */
#define MHDTL_H01V "some notably long data1"    /* length: 23 */
#define MHDTL_H02N "longer-name2"       /* length: 12 */
#define MHDTL_H02V "some longer data2"  /* length: 17 */
#define MHDTL_H03N "abcdefgh3"          /* length: 9 */
#define MHDTL_H03V "random data3"       /* length: 12 */
#define MHDTL_H04N "short-hdr4"         /* length: 10 */
#define MHDTL_H04V "value4"             /* length: 6 */
#define MHDTL_H05N "some-header5"                       /* length: 12 */
#define MHDTL_H05V "even longer value of the header5"   /* length: 32 */
#define MHDTL_H06N "longer-name6"       /* length: 12 */
#define MHDTL_H06V "some longer data6"  /* length: 17 */
#define MHDTL_H07N "some-extremely-long-name7"                  /* length: 25 */
#define MHDTL_H07V "some really long value of the header7"      /* length: 37 */
#define MHDTL_H08N "someheader8"        /* length: 11 */
#define MHDTL_H08V "value8"             /* length: 6 */
#define MHDTL_H09N "the-long-long-long-long-long-long-long-name9"       /* length: 44 */
#define MHDTL_H09V "the header (field) value, with commas and other " \
        "extra characters to make it really huge, larger " \
        "enough to replace all other headers but one, the " \
        "last one."                    /* length: 154 */
#define MHDTL_H10N "the-long-long-long-long-long-long-long-name10"      /* length: 45 */
#define MHDTL_H10V "the header (field) value, with commas and other " \
        "extra characters to make it really huge, larger " \
        "enough to replace all other headers, leaving space " \
        "only for a tiny header"                      /* length: 169 */
#define MHDTL_H11N "a"          /* length: 1 */
#define MHDTL_H11V "B"          /* length: 1 */
#define MHDTL_H12N "c"          /* length: 1 */
#define MHDTL_H12V "D"          /* length: 1 */
#define MHDTL_H13N "e"          /* length: 1 */
#define MHDTL_H13V "F"          /* length: 1 */
#define MHDTL_H14N "g"          /* length: 1 */
#define MHDTL_H14V "H"          /* length: 1 */
#define MHDTL_H15N "i"          /* length: 1 */
#define MHDTL_H15V "J"          /* length: 1 */
#define MHDTL_H16N "k"          /* length: 1 */
#define MHDTL_H16V "L"          /* length: 1 */
#define MHDTL_H17N "m"          /* length: 1 */
#define MHDTL_H17V "N"          /* length: 1 */
#define MHDTL_H18N "name18"     /* length: 6 */
#define MHDTL_H18V "vl18"       /* length: 4 */
#define MHDTL_H19N "o"          /* length: 1 */
#define MHDTL_H19V "P"          /* length: 1 */
#define MHDTL_H20N "m"          /* length: 1 */
#define MHDTL_H20V "N"          /* length: 1 */

  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H01N, MHDTL_H01V); /* 18 + 23 + 32 = 73 */
  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H02N, MHDTL_H02V); /* 12 + 17 + 32 = 61 */
  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H03N, MHDTL_H03V); /*  9 + 12 + 32 = 53 */
  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H04N, MHDTL_H04V); /* 10 +  6 + 32 = 48 */
  /* 73 + 61 + 48 + 76 = 258 */

  /* '1', '2', '3', '4' [insert pos] */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 235u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 4u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                MHDTL_H04N, MHDTL_H04V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                MHDTL_H03N, MHDTL_H03V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 3u,
                                MHDTL_H02N, MHDTL_H02V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 4u,
                                MHDTL_H01N, MHDTL_H01V);

  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H05N, MHDTL_H05V); /* 12 + 32 + 32 = 76 */
  /* '5' [insert pos], '2', '3', '4' */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 238u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 4u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                MHDTL_H05N, MHDTL_H05V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                MHDTL_H04N, MHDTL_H04V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 3u,
                                MHDTL_H03N, MHDTL_H03V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 4u,
                                MHDTL_H02N, MHDTL_H02V);

  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H06N, MHDTL_H06V); /* 12 + 17 + 32 = 61 */
  /* '5', '6' [insert pos], '3', '4' */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 238u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 4u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                MHDTL_H06N, MHDTL_H06V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                MHDTL_H05N, MHDTL_H05V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 3u,
                                MHDTL_H04N, MHDTL_H04V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 4u,
                                MHDTL_H03N, MHDTL_H03V);

  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H07N, MHDTL_H07V); /* 25 + 37 + 32 = 94 */
  /* '5', '6', '7' [insert pos], '4' */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 279u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 4u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                MHDTL_H07N, MHDTL_H07V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                MHDTL_H06N, MHDTL_H06V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 3u,
                                MHDTL_H05N, MHDTL_H05V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 4u,
                                MHDTL_H04N, MHDTL_H04V);

  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H08N, MHDTL_H08V); /* 11 +  6 + 32 = 49 */
  /* '5', '6', '7', '8' [insert pos] */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 280u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 4u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                MHDTL_H08N, MHDTL_H08V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                MHDTL_H07N, MHDTL_H07V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 3u,
                                MHDTL_H06N, MHDTL_H06V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 4u,
                                MHDTL_H05N, MHDTL_H05V);

  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H09N, MHDTL_H09V); /* 44 + 154 + 32 = 230 */
  /* '9' [insert pos], '8' */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 279u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 2u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                MHDTL_H09N, MHDTL_H09V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                MHDTL_H08N, MHDTL_H08V);

  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H10N, MHDTL_H10V); /* 45 + 169 + 32 = 246 */
  /* '10' [insert pos] */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 246u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 1u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                MHDTL_H10N, MHDTL_H10V);

  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H11N, MHDTL_H11V); /*  1 +  1 + 32 = 34 */
  /* '10', '11' [insert pos] */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 280u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 2u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                MHDTL_H11N, MHDTL_H11V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                MHDTL_H10N, MHDTL_H10V);

  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H12N, MHDTL_H12V); /*  1 +  1 + 32 = 34 */
  /* '12' [insert pos], '11' */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 68u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 2u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                MHDTL_H12N, MHDTL_H12V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                MHDTL_H11N, MHDTL_H11V);

  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H13N, MHDTL_H13V); /*  1 +  1 + 32 = 34 */
  /* '12', '13' [insert pos], '11' */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 102u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 3u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                MHDTL_H13N, MHDTL_H13V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                MHDTL_H12N, MHDTL_H12V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 3u,
                                MHDTL_H11N, MHDTL_H11V);

  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H14N, MHDTL_H14V); /*  1 +  1 + 32 = 34 */
  /* '12', '13', '14' [insert pos], '11' */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 136u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 4u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                MHDTL_H14N, MHDTL_H14V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                MHDTL_H13N, MHDTL_H13V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 3u,
                                MHDTL_H12N, MHDTL_H12V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 4u,
                                MHDTL_H11N, MHDTL_H11V);

  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H15N, MHDTL_H15V); /*  1 +  1 + 32 = 34 */
  /* '12', '13', '14', '15' [insert pos], '11' */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 170u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 5u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                MHDTL_H15N, MHDTL_H15V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                MHDTL_H14N, MHDTL_H14V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 3u,
                                MHDTL_H13N, MHDTL_H13V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 4u,
                                MHDTL_H12N, MHDTL_H12V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 5u,
                                MHDTL_H11N, MHDTL_H11V);

  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H16N, MHDTL_H16V); /*  1 +  1 + 32 = 34 */
  /* '12', '13', '14', '15', '16' [insert pos], '11' */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 204u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 6u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                MHDTL_H16N, MHDTL_H16V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                MHDTL_H15N, MHDTL_H15V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 3u,
                                MHDTL_H14N, MHDTL_H14V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 4u,
                                MHDTL_H13N, MHDTL_H13V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 5u,
                                MHDTL_H12N, MHDTL_H12V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 6u,
                                MHDTL_H11N, MHDTL_H11V);

  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H17N, MHDTL_H17V); /*  1 +  1 + 32 = 34 */
  /* '12', '13', '14', '15', '16', '17' [insert pos], '11' */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 238u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 7u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                MHDTL_H17N, MHDTL_H17V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                MHDTL_H16N, MHDTL_H16V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 3u,
                                MHDTL_H15N, MHDTL_H15V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 4u,
                                MHDTL_H14N, MHDTL_H14V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 5u,
                                MHDTL_H13N, MHDTL_H13V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 6u,
                                MHDTL_H12N, MHDTL_H12V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 7u,
                                MHDTL_H11N, MHDTL_H11V);

  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H18N, MHDTL_H18V); /*  6 +  4 + 32 = 42 */
  /* '12', '13', '14', '15', '16', '17', '18' [insert pos], '11' */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 280u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 8u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                MHDTL_H18N, MHDTL_H18V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                MHDTL_H17N, MHDTL_H17V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 3u,
                                MHDTL_H16N, MHDTL_H16V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 4u,
                                MHDTL_H15N, MHDTL_H15V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 5u,
                                MHDTL_H14N, MHDTL_H14V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 6u,
                                MHDTL_H13N, MHDTL_H13V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 7u,
                                MHDTL_H12N, MHDTL_H12V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 8u,
                                MHDTL_H11N, MHDTL_H11V);

  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H19N, MHDTL_H19V); /*  1 +  1 + 32 = 34 */
  /* '12', '13', '14', '15', '16', '17', '18', '19' [insert pos] */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 280u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 8u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                MHDTL_H19N, MHDTL_H19V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                MHDTL_H18N, MHDTL_H18V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 3u,
                                MHDTL_H17N, MHDTL_H17V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 4u,
                                MHDTL_H16N, MHDTL_H16V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 5u,
                                MHDTL_H15N, MHDTL_H15V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 6u,
                                MHDTL_H14N, MHDTL_H14V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 7u,
                                MHDTL_H13N, MHDTL_H13V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 8u,
                                MHDTL_H12N, MHDTL_H12V);

  mhdtl_dyn_add_sized_entry_with_check (dyn, 246u);
  /* 'generated' [insert pos], '19' */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 280u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 2u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                MHDTL_H19N, MHDTL_H19V);

  mhdtl_dyn_add_sized_entry_with_check (dyn, 247u); /* Should be reset on the next add */
  /* Re-use headers */
  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H12N, MHDTL_H12V); /*  1 +  1 + 32 = 34 */
  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H13N, MHDTL_H13V); /*  1 +  1 + 32 = 34 */
  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H14N, MHDTL_H14V); /*  1 +  1 + 32 = 34 */
  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H15N, MHDTL_H15V); /*  1 +  1 + 32 = 34 */
  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H16N, MHDTL_H16V); /*  1 +  1 + 32 = 34 */
  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H17N, MHDTL_H17V); /*  1 +  1 + 32 = 34 */
  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H18N, MHDTL_H18V); /*  6 +  4 + 32 = 42 */
  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H19N, MHDTL_H19V); /*  1 +  1 + 32 = 34 */
  /* '12', '13', '14', '15', '16', '17', '18', '19' [insert pos] */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 280u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 8u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                MHDTL_H19N, MHDTL_H19V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                MHDTL_H18N, MHDTL_H18V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 3u,
                                MHDTL_H17N, MHDTL_H17V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 4u,
                                MHDTL_H16N, MHDTL_H16V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 5u,
                                MHDTL_H15N, MHDTL_H15V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 6u,
                                MHDTL_H14N, MHDTL_H14V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 7u,
                                MHDTL_H13N, MHDTL_H13V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 8u,
                                MHDTL_H12N, MHDTL_H12V);

  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H20N, MHDTL_H20V); /*  1 +  1 + 32 = 34 */
  /* '20' [insert pos], '13', '14', '15', '16', '17', '18', '19' */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 280u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 8u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                MHDTL_H20N, MHDTL_H20V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                MHDTL_H19N, MHDTL_H19V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 3u,
                                MHDTL_H18N, MHDTL_H18V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 4u,
                                MHDTL_H17N, MHDTL_H17V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 5u,
                                MHDTL_H16N, MHDTL_H16V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 6u,
                                MHDTL_H15N, MHDTL_H15V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 7u,
                                MHDTL_H14N, MHDTL_H14V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 8u,
                                MHDTL_H13N, MHDTL_H13V);

  mhdtl_dyn_add_sized_entry_with_check (dyn, 212u);
  /* '20', 'generated' [insert pos], '19' */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 280u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 3u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                MHDTL_H20N, MHDTL_H20V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 3u,
                                MHDTL_H19N, MHDTL_H19V);

  mhd_dtbl_destroy (dyn);

#undef MHDTL_H01N
#undef MHDTL_H01V
#undef MHDTL_H02N
#undef MHDTL_H02V
#undef MHDTL_H03N
#undef MHDTL_H03V
#undef MHDTL_H04N
#undef MHDTL_H04V
#undef MHDTL_H05N
#undef MHDTL_H05V
#undef MHDTL_H06N
#undef MHDTL_H06V
#undef MHDTL_H07N
#undef MHDTL_H07V
#undef MHDTL_H08N
#undef MHDTL_H08V
#undef MHDTL_H09N
#undef MHDTL_H09V
#undef MHDTL_H10N
#undef MHDTL_H10V
#undef MHDTL_H11N
#undef MHDTL_H11V
#undef MHDTL_H12N
#undef MHDTL_H12V
#undef MHDTL_H13N
#undef MHDTL_H13V
#undef MHDTL_H14N
#undef MHDTL_H14V
#undef MHDTL_H15N
#undef MHDTL_H15V
#undef MHDTL_H16N
#undef MHDTL_H16V
#undef MHDTL_H17N
#undef MHDTL_H17V
#undef MHDTL_H18N
#undef MHDTL_H18V
#undef MHDTL_H19N
#undef MHDTL_H19V
#undef MHDTL_H20N
#undef MHDTL_H20V

  return MHDT_TEST_RESULT ();
}


static int
test_dyn_add_evict_inter (void)
{
  struct mhd_HpackDTblContext*dyn;
  struct mhd_BufferConst check_name;
  struct mhd_BufferConst check_value;

  MHDT_EXPECT_PTR_NONNULL_D (dyn = mhd_dtbl_create (705u), \
                             "Create HPACK dynamic table");
  if (NULL == dyn)
    return MHDT_TEST_RESULT ();

  mhdtl_dyn_add_sized_entry_with_check (dyn, 50u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 1u);
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 50u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 75u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 2u);
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 125u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 100u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 3u);
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 225u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 125u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 4u);
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 350u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 150u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 5u);
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 500u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 175u);

  /* { 50, 75, 100, 125, 150, 175 [insert pos] } */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 675u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 6u);

  if (MHDT_EXPECT_TRUE (mhd_dtbl_get_entry (dyn, \
                                            mhd_HPACK_STBL_LAST_IDX + 6u, \
                                            &check_name, \
                                            &check_value)))
    MHDT_EXPECT_UINT_EQ_VAL (MHDTL_HPACK_ENTRY_SIZE (check_name.size, \
                                                     check_value.size), \
                             50u);

  /* First two entries should be replaced with a single entry, which
     is 2 bytes smaller than two entries.
     (50, 75) -> (123) */
  mhdtl_dyn_add_sized_entry_with_check (dyn, 123u);
  /* { 123 [insert pos], 100, 125, 150, 175 } */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 673u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 5u);

  if (MHDT_EXPECT_TRUE (mhd_dtbl_get_entry (dyn, \
                                            mhd_HPACK_STBL_LAST_IDX + 5u, \
                                            &check_name, \
                                            &check_value)))
    MHDT_EXPECT_UINT_EQ_VAL (MHDTL_HPACK_ENTRY_SIZE (check_name.size, \
                                                     check_value.size), \
                             100u);

  /* Next entry should be replaced with a smaller entry.
     (100) -> (35) */
  mhdtl_dyn_add_sized_entry_with_check (dyn, 35u);
  /* { 123, 35 [insert pos], 125, 150, 175 } */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 608u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 5u);

  if (MHDT_EXPECT_TRUE (mhd_dtbl_get_entry (dyn, \
                                            mhd_HPACK_STBL_LAST_IDX + 5u, \
                                            &check_name, \
                                            &check_value)))
    MHDT_EXPECT_UINT_EQ_VAL (MHDTL_HPACK_ENTRY_SIZE (check_name.size, \
                                                     check_value.size), \
                             125u);

  /* Should be added without eviction.
     -> (97) */
  mhdtl_dyn_add_sized_entry_with_check (dyn, 97u);
  /* { 123, 35, 97 [insert pos], 125, 150, 175 } */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 705u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 6u);

  if (MHDT_EXPECT_TRUE (mhd_dtbl_get_entry (dyn, \
                                            mhd_HPACK_STBL_LAST_IDX + 6u, \
                                            &check_name, \
                                            &check_value)))
    MHDT_EXPECT_UINT_EQ_VAL (MHDTL_HPACK_ENTRY_SIZE (check_name.size, \
                                                     check_value.size), \
                             125u);

  /* Two entries should be replaced with single one.
     (125, 150) -> (275) */
  mhdtl_dyn_add_sized_entry_with_check (dyn, 275u);
  /* { 123, 35, 97, 275 [insert pos], 175 } */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 705u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 5u);

  if (MHDT_EXPECT_TRUE (mhd_dtbl_get_entry (dyn, \
                                            mhd_HPACK_STBL_LAST_IDX + 5u, \
                                            &check_name, \
                                            &check_value)))
    MHDT_EXPECT_UINT_EQ_VAL (MHDTL_HPACK_ENTRY_SIZE (check_name.size, \
                                                     check_value.size), \
                             175u);

  /* Two entries should be replaced with single one.
     (175, 123) -> (251) */
  mhdtl_dyn_add_sized_entry_with_check (dyn, 251u);
  /* { 35, 97, 275, 251 [insert pos] } */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 658u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 4u);

  if (MHDT_EXPECT_TRUE (mhd_dtbl_get_entry (dyn, \
                                            mhd_HPACK_STBL_LAST_IDX + 4u, \
                                            &check_name, \
                                            &check_value)))
    MHDT_EXPECT_UINT_EQ_VAL (MHDTL_HPACK_ENTRY_SIZE (check_name.size, \
                                                     check_value.size), \
                             35u);

  /* Should be added without eviction.
     -> (42) */
  mhdtl_dyn_add_sized_entry_with_check (dyn, 42u);
  /* { 42 [insert pos], 35, 97, 275, 251 } */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 700u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 5u);

  if (MHDT_EXPECT_TRUE (mhd_dtbl_get_entry (dyn, \
                                            mhd_HPACK_STBL_LAST_IDX + 5u, \
                                            &check_name, \
                                            &check_value)))
    MHDT_EXPECT_UINT_EQ_VAL (MHDTL_HPACK_ENTRY_SIZE (check_name.size, \
                                                     check_value.size), \
                             35u);

  /* Two entries should be replaced with single one.
     (35, 97) -> (51) */
  mhdtl_dyn_add_sized_entry_with_check (dyn, 51u);
  /* { 42, 51 [insert pos], 275, 251 } */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 619u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 4u);

  if (MHDT_EXPECT_TRUE (mhd_dtbl_get_entry (dyn, \
                                            mhd_HPACK_STBL_LAST_IDX + 4u, \
                                            &check_name, \
                                            &check_value)))
    MHDT_EXPECT_UINT_EQ_VAL (MHDTL_HPACK_ENTRY_SIZE (check_name.size, \
                                                     check_value.size), \
                             275u);

  /* Three entries should be replaced with single one.
     (275, 251, 42) -> (613) */
  mhdtl_dyn_add_sized_entry_with_check (dyn, 613u);
  /* { 51, 613 [insert pos] } */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 664u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 2u);

  /* (51, 613) -> (672) */
  mhdtl_dyn_add_sized_entry_with_check (dyn, 672u);
  /* { 672 [insert pos] } */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 672u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 1u);

  /* -> (33) */
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  /* { 672, 33 [insert pos] } */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 705u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 2u);

  /* (672) -> (33) */
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  /* { 33 [insert pos], 33 } */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 66u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 2u);

  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  /* { 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
       33, 33, 33 [insert pos], 33 } */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 693u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 21u);

  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  /* { 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
       33, 33, 33, 33 [insert pos] } */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 693u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 21u);

  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  /* { 33 [insert pos], 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
       33, 33, 33, 33, 33, 33, 33 } */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 693u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 21u);

  mhdtl_dyn_add_sized_entry_with_check (dyn, 639u);
  /* { 33, 639 [insert pos], 33 } */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 705u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 3u);

  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  /* { 33 [insert pos], 639, 33 } */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 705u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 3u);

  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  /* { 33, 33 [insert pos], 33 } */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 99u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 3u);

  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 33u);
  /* { 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
       33, 33, 33 [insert pos], 33 } */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 693u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 21u);

  mhd_dtbl_destroy (dyn);

  return MHDT_TEST_RESULT ();
}


static int
test_dyn_add_series (void)
{
  const uint_fast16_t num_inter =
    (uint_fast16_t)
    (mhdtl_enable_deep_tests ? (16u * 1024u) : (1024u));
  struct mhd_HpackDTblContext*dyn;
  uint_fast16_t i;

  MHDT_EXPECT_PTR_NONNULL_D (dyn = mhd_dtbl_create (925u), \
                             "Create HPACK dynamic table");
  if (NULL == dyn)
    return MHDT_TEST_RESULT ();

  /* Add large strings */
  for (i = 0; i < num_inter; ++i)
  {
    size_t prev_used = 0u;
    size_t cur_used;
    size_t strs_size;

    cur_used = mhdtl_dyn_get_size_used (dyn);
    do
    {
      strs_size = 50u + mhdtl_mix16 (i,
                                     (uint_least16_t) prev_used) % 256u;
      prev_used = cur_used;
      mhdtl_dyn_add_sized_strs_with_check (dyn,
                                           strs_size);
      cur_used = mhdtl_dyn_get_size_used (dyn);
    } while (prev_used + strs_size + mhd_HPACK_ENTRY_OVERHEAD
             == cur_used);
    mhd_dtbl_evict_to_size (dyn,
                            0u);
  }

  /* Add small strings */
  for (i = 0; i < num_inter; ++i)
  {
    size_t prev_used = 0u;
    size_t cur_used;
    size_t strs_size;

    cur_used = mhdtl_dyn_get_size_used (dyn);
    do
    {
      strs_size = mhdtl_mix16 (i,
                               (uint_least16_t) prev_used) % 16u;
      prev_used = cur_used;
      mhdtl_dyn_add_sized_strs_with_check (dyn,
                                           strs_size);
      cur_used = mhdtl_dyn_get_size_used (dyn);
    } while (prev_used + strs_size + mhd_HPACK_ENTRY_OVERHEAD
             == cur_used);
    mhd_dtbl_evict_to_size (dyn,
                            0u);
  }

  /* Add mixed-size strings */
  for (i = 0; i < num_inter; ++i)
  {
    size_t prev_used = 0u;
    size_t cur_used;
    size_t strs_size;

    cur_used = mhdtl_dyn_get_size_used (dyn);
    do
    {
      strs_size = mhdtl_mix16 (i,
                               (uint_least16_t) prev_used) % 312u;
      prev_used = cur_used;
      mhdtl_dyn_add_sized_strs_with_check (dyn,
                                           strs_size);
      cur_used = mhdtl_dyn_get_size_used (dyn);
    } while (prev_used + strs_size + mhd_HPACK_ENTRY_OVERHEAD
             == cur_used);
    mhd_dtbl_evict_to_size (dyn,
                            0u);
  }

  mhd_dtbl_destroy (dyn);

  return MHDT_TEST_RESULT ();
}


static int
test_dyn_add_evict_series (void)
{
  const uint_fast16_t num_inter =
    (uint_fast16_t)
    (mhdtl_enable_deep_tests ? (1024u) : (64u));
  struct mhd_HpackDTblContext*dyn;
  uint_fast16_t i;

  MHDT_EXPECT_PTR_NONNULL_D (dyn = mhd_dtbl_create (925u), \
                             "Create HPACK dynamic table");
  if (NULL == dyn)
    return MHDT_TEST_RESULT ();

  for (i = 0; i < num_inter; ++i)
  {
    uint_fast16_t j;
    /* Add large strings */
    for (j = 0; j < 128u; ++j)
    {
      size_t strs_size = 50u + mhdtl_mix16 (i, j) % 256u;
      mhdtl_dyn_add_sized_strs_with_check (dyn,
                                           strs_size);
      /* Sometimes repeat the same size */
      if (16u == (j % 32u))
      {
        unsigned int num_rep = mhdtl_mix16 (j, i) % 16u;
        while (0 != num_rep--)
          mhdtl_dyn_add_sized_strs_with_check (dyn,
                                               strs_size);
      }
    }
    /* Add small strings */
    for (j = 0; j < 512u; ++j)
    {
      size_t strs_size = mhdtl_mix16 (i, j) % 15u;
      mhdtl_dyn_add_sized_strs_with_check (dyn,
                                           strs_size);
      /* Sometimes repeat the same size */
      if (16u == (j % 32u))
      {
        unsigned int num_rep = mhdtl_mix16 (j, i) % 32u;
        while (0 != num_rep--)
          mhdtl_dyn_add_sized_strs_with_check (dyn,
                                               strs_size);
      }
    }
    /* Add mixed-size strings */
    for (j = 0; j < 256u; ++j)
    {
      size_t strs_size = mhdtl_mix16 (i, j) % 312u;
      mhdtl_dyn_add_sized_strs_with_check (dyn,
                                           strs_size);
      /* Sometimes repeat the same size */
      if (16u == (j % 32u))
      {
        unsigned int num_rep = mhdtl_mix16 (j, i) % 24u;
        while (0 != num_rep--)
          mhdtl_dyn_add_sized_strs_with_check (dyn,
                                               strs_size);
      }
    }
    /* Again add small strings before adding large strings */
    for (j = 0; j < 32u; ++j)
    {
      size_t strs_size = mhdtl_mix16 (i, j) % 15u;
      mhdtl_dyn_add_sized_strs_with_check (dyn,
                                           strs_size);
    }
  }

  mhd_dtbl_destroy (dyn);

  return MHDT_TEST_RESULT ();
}


static int
test_dyn_evict_to_size (void)
{
  struct mhd_HpackDTblContext*dyn;

  MHDT_EXPECT_PTR_NONNULL_D (dyn = mhd_dtbl_create (256u), \
                             "Create HPACK dynamic table");
  if (NULL == dyn)
    return MHDT_TEST_RESULT ();

#define MHDTL_H01N "a"   /* length: 1 */
#define MHDTL_H01V "b"   /* length: 1 */
#define MHDTL_H02N "cd"  /* length: 2 */
#define MHDTL_H02V "ef"  /* length: 2 */
#define MHDTL_H03N "header3"             /* length: 7 */
#define MHDTL_H03V "value of header3"    /* length: 16 */
#define MHDTL_H04N "some-header4"                        /* length: 12 */
#define MHDTL_H04V "even longer value of the header4"    /* length: 32 */

  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H01N, MHDTL_H01V); /* 1 + 1 + 32 = 34 */
  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H02N, MHDTL_H02V); /* 2 + 2 + 32 = 36 */
  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H03N, MHDTL_H03V); /* 7 + 16 + 32 = 55 */
  mhdtl_dyn_add_hdr_with_check (dyn, MHDTL_H04N, MHDTL_H04V); /* 12 + 32 + 32 = 76 */
  /* Total size: 34 + 36 + 55 + 76 = 201 */

  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 201u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 4u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                MHDTL_H04N, MHDTL_H04V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                MHDTL_H03N, MHDTL_H03V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 3u,
                                MHDTL_H02N, MHDTL_H02V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 4u,
                                MHDTL_H01N, MHDTL_H01V);

  mhdtl_dyn_check_get_valid_invalid (dyn);

  mhd_dtbl_evict_to_size (dyn, 202u); /* Nothing should be evicted */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 201u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 4u);
  mhdtl_dyn_check_get_valid_invalid (dyn);

  mhd_dtbl_evict_to_size (dyn, 201u); /* Still nothing should be evicted */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 201u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 4u);
  mhdtl_dyn_check_get_valid_invalid (dyn);

  mhd_dtbl_evict_to_size (dyn, 200u); /* Oldest entry should be evicted */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 201u - 34u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 3u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                MHDTL_H04N, MHDTL_H04V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                MHDTL_H03N, MHDTL_H03V);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 3u,
                                MHDTL_H02N, MHDTL_H02V);

  mhdtl_dyn_check_get_valid_invalid (dyn);

  mhd_dtbl_evict_to_size (dyn, 76u + 1u); /* Only last entry should be left */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 76u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 1u);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                MHDTL_H04N, MHDTL_H04V);

  mhdtl_dyn_check_get_valid_invalid (dyn);

  mhd_dtbl_evict_to_size (dyn, 76u - 1u); /* Everything should be evicted */
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 0u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 0u);

  mhdtl_dyn_check_get_valid_invalid (dyn);

#undef MHDTL_H01N
#undef MHDTL_H01V
#undef MHDTL_H02N
#undef MHDTL_H02V
#undef MHDTL_H03N
#undef MHDTL_H03V
#undef MHDTL_H04N
#undef MHDTL_H04V

  mhd_dtbl_destroy (dyn);

  return MHDT_TEST_RESULT ();
}


static int
test_dyn_resize (void)
{
  struct mhd_HpackDTblContext*dyn;

  /* zero-sized table */
  MHDT_EXPECT_PTR_NONNULL_D (dyn = mhd_dtbl_create (321u), \
                             "Create HPACK dynamic table");
  if (NULL == dyn)
    return MHDT_TEST_RESULT ();

  mhdtl_dyn_add_hdr_with_check (dyn, "a", "b");
  mhdtl_dyn_add_sized_entry_with_check (dyn, 150u);
  mhdtl_dyn_add_hdr_with_check (dyn, "c", "d");
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 3u);
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 218u);

  /* Resize table (should be no eviction) */
  MHDT_EXPECT_TRUE_D (mhd_dtbl_resize (&dyn, 230u), "resize of the table");
  MHDT_EXPECT_PTR_NONNULL (dyn);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_table_max_size (dyn), 230u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 3u);
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 218u);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 3u,
                                "a",
                                "b");
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                "c",
                                "d");

  /* Resize table (should be still no eviction) */
  MHDT_EXPECT_TRUE_D (mhd_dtbl_resize (&dyn, 218u), "resize of the table");
  MHDT_EXPECT_PTR_NONNULL (dyn);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_table_max_size (dyn), 218u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 3u);
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 218u);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 3u,
                                "a",
                                "b");
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                "c",
                                "d");

  /* Resize table (oldest entry should be evicted) */
  MHDT_EXPECT_TRUE_D (mhd_dtbl_resize (&dyn, 217u), "resize of the table");
  MHDT_EXPECT_PTR_NONNULL (dyn);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_table_max_size (dyn), 217u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 2u);
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 184u);
  mhdtl_dyn_add_hdr_with_check (dyn, "x", "");
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 3u);
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 217u);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                "x",
                                "");

  /* Resize table (all entries except the newest should be evicted) */
  MHDT_EXPECT_TRUE_D (mhd_dtbl_resize (&dyn, 33u), "resize of the table");
  MHDT_EXPECT_PTR_NONNULL (dyn);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_table_max_size (dyn), 33u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 1u);
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 33u);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                "x",
                                "");

  /* Resize table (grow, no eviction) */
  MHDT_EXPECT_TRUE_D (mhd_dtbl_resize (&dyn, 501u), "resize of the table");
  MHDT_EXPECT_PTR_NONNULL (dyn);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_table_max_size (dyn), 501u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 1u);
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 33u);
  mhdtl_dyn_add_sized_entry_with_check (dyn, 468u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 2u);
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 501u);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                "x",
                                "");

  mhd_dtbl_destroy (dyn);

  return MHDT_TEST_RESULT ();
}


static int
test_dyn_find (void)
{
  static const char *hdr1_name = "first-hdr";
  static const char *hdr1_val = "first value";
  static const char *hdr2_name = "second-hdr";
  static const char *hdr2_val = "second value";
  static const char *hdr3_name = "third-hdr";
  static const char *hdr3_val = "third value";
  static const char *hdr4_name = "fourth-hdr";
  static const char *hdr4_val = "fourth value";
  struct mhd_HpackDTblContext*dyn;

  MHDT_EXPECT_PTR_NONNULL_D (dyn = mhd_dtbl_create (256u), \
                             "Create HPACK dynamic table");
  if (NULL == dyn)
    return MHDT_TEST_RESULT ();

  mhdtl_dyn_add_hdr_with_check (dyn, hdr1_name, hdr1_val);
  mhdtl_dyn_add_hdr_with_check (dyn, hdr2_name, hdr2_val);
  mhdtl_dyn_add_hdr_with_check (dyn, hdr3_name, hdr3_val);
  mhdtl_dyn_add_hdr_with_check (dyn, hdr4_name, hdr4_val);

  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 4u,
                                hdr1_name, hdr1_val);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 3u,
                                hdr2_name, hdr2_val);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                hdr3_name, hdr3_val);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                hdr4_name, hdr4_val);

  mhd_dtbl_destroy (dyn);

  return MHDT_TEST_RESULT ();
}


static int
test_dyn_reset_on_too_large (void)
{
  static const size_t tbl_size = 133u; /* not a power of two */
  struct mhd_HpackDTblContext*dyn;

  MHDT_EXPECT_PTR_NONNULL_D (dyn = mhd_dtbl_create (tbl_size), \
                             "Create HPACK dynamic table");
  if (NULL == dyn)
    return MHDT_TEST_RESULT ();

  /* Add small entries */
  mhdtl_dyn_add_hdr_with_check (dyn, "a", "b");
  mhdtl_dyn_add_hdr_with_check (dyn, "c", "d");
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 2u);

  mhdtl_dyn_add_sized_entry_with_check (dyn, tbl_size + 1);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 0u);

  /* Re-add small entries */
  mhdtl_dyn_add_hdr_with_check (dyn, "a", "b");
  mhdtl_dyn_add_hdr_with_check (dyn, "c", "d");
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 2u);

  mhdtl_dyn_add_sized_entry_with_check (dyn, tbl_size);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 1u);

  if (1)
  {
    struct mhd_BufferConst check_name;
    struct mhd_BufferConst check_value;

    if (MHDT_EXPECT_TRUE (mhd_dtbl_get_entry (dyn, \
                                              mhd_HPACK_STBL_LAST_IDX + 1u, \
                                              &check_name,
                                              &check_value)))
    {
      MHDT_EXPECT_UINT_GT_VAL (check_name.size, 1u);
      MHDT_EXPECT_UINT_GT_VAL (check_value.size, 1u);
    }
  }

  mhd_dtbl_destroy (dyn);

  return MHDT_TEST_RESULT ();
}


static int
test_dyn_zero_sizes (void)
{
  struct mhd_HpackDTblContext*dyn;

  /* zero-sized table */
  MHDT_EXPECT_PTR_NONNULL_D (dyn = mhd_dtbl_create (0u), \
                             "Create HPACK dynamic table");
  if (NULL == dyn)
    return MHDT_TEST_RESULT ();

  mhdtl_dyn_add_hdr_with_check (dyn, "a", "b");
  mhdtl_dyn_add_hdr_with_check (dyn, "c", "d");
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 0u);
  /* smallest entry (zero-sized strings) */
  mhdtl_dyn_add_sized_strs_with_check (dyn, 0u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 0u);

  /* Resize table to the same zero size */
  MHDT_EXPECT_TRUE_D (mhd_dtbl_resize (&dyn, 0u), \
                      "resize of the table");
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_table_max_size (dyn), 0u);
  /* smallest entry (zero-sized strings) */
  mhdtl_dyn_add_sized_strs_with_check (dyn, 0u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 0u);

  /* Resize table to non-zero size */
  MHDT_EXPECT_TRUE_D (mhd_dtbl_resize (&dyn, 127u), \
                      "resize of the table");
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_table_max_size (dyn), 127u);

  mhdtl_dyn_add_hdr_with_check (dyn, "a", "b");
  mhdtl_dyn_add_hdr_with_check (dyn, "c", "d");
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 2u);

  mhd_dtbl_evict_to_size (dyn, 0u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 0u);

  mhdtl_dyn_add_hdr_with_check (dyn, "a", "b");
  mhdtl_dyn_add_hdr_with_check (dyn, "c", "d");
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 2u);

  mhdtl_dyn_add_sized_entry_with_check (dyn, 45u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 3u);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 3u,
                                "a",
                                "b");
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                "c",
                                "d");

  /* smallest entries (zero-sized strings) */
  mhdtl_dyn_add_hdr_with_check (dyn, "", "");
  mhdtl_dyn_add_hdr_with_check (dyn, "", "");
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 3u);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 1u,
                                "",
                                "");
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 2u,
                                "",
                                "");

  /* Resize table to zero size again */
  MHDT_EXPECT_TRUE_D (mhd_dtbl_resize (&dyn, 0u), \
                      "resize of the table");
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_table_max_size (dyn), 0u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 0u);
  mhd_dtbl_evict_to_size (dyn, 0u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_table_max_size (dyn), 0u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 0u);

  mhdtl_dyn_add_hdr_with_check (dyn, "a", "b");
  mhdtl_dyn_add_hdr_with_check (dyn, "c", "d");
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 0u);
  mhdtl_dyn_add_sized_strs_with_check (dyn, 0u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 0u);

  /* Resize table to the size smaller than minimal entry size */
  MHDT_EXPECT_TRUE_D (mhd_dtbl_resize (&dyn, 31u), \
                      "resize of the table");
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 0u);
  mhdtl_dyn_add_hdr_with_check (dyn, "a", "b");
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 0u);
  mhdtl_dyn_add_sized_strs_with_check (dyn, 0u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 0u);

  /* Resize table to the minimal size */
  MHDT_EXPECT_TRUE_D (mhd_dtbl_resize (&dyn, 32u), \
                      "resize of the table");
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_table_max_size (dyn), 32u);
  mhdtl_dyn_add_sized_strs_with_check (dyn, 0u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 1u);
  /* Resize table back to the size smaller than minimal entry size */
  MHDT_EXPECT_TRUE_D (mhd_dtbl_resize (&dyn, 31u), \
                      "resize of the table");
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_table_max_size (dyn), 31u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 0u);
  mhdtl_dyn_add_sized_strs_with_check (dyn, 0u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 0u);
  MHDT_EXPECT_TRUE_D (mhd_dtbl_resize (&dyn, 0u), \
                      "resize of the table");
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_table_max_size (dyn), 0u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 0u);

  /* Resize table to the usable size */
  MHDT_EXPECT_TRUE_D (mhd_dtbl_resize (&dyn, 128u), \
                      "resize of the table");
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_table_max_size (dyn), 128u);
  mhdtl_dyn_add_sized_strs_with_check (dyn, 0u);
  mhdtl_dyn_add_sized_strs_with_check (dyn, 0u);
  mhdtl_dyn_add_sized_strs_with_check (dyn, 0u);
  mhdtl_dyn_add_sized_strs_with_check (dyn, 0u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 4u);
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 128u);
  mhdtl_dyn_add_hdr_with_check (dyn, "a", "1");
  mhdtl_dyn_add_hdr_with_check (dyn, "b", "2");
  mhdtl_dyn_add_hdr_with_check (dyn, "c", "3");
  mhdtl_dyn_add_hdr_with_check (dyn, "d", "4");
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 3u);
  MHDT_EXPECT_UINT_EQ_VAL (mhdtl_dyn_get_size_used (dyn), 102u);
  mhdtl_dyn_check_entry_at_idx (dyn,
                                mhd_HPACK_STBL_LAST_IDX + 3u,
                                "b",
                                "2");
  /* Resize table to zero size again */
  MHDT_EXPECT_TRUE_D (mhd_dtbl_resize (&dyn, 0u), \
                      "resize of the table");
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_table_max_size (dyn), 0u);
  MHDT_EXPECT_UINT_EQ_VAL (mhd_dtbl_get_num_entries (dyn), 0u);

  mhd_dtbl_destroy (dyn);

  return MHDT_TEST_RESULT ();
}


static int
test_stat_get_find (void)
{
  mhdtl_stat_check_entry_at_idx (1u,
                                 ":authority",
                                 "");
  mhdtl_stat_check_entry_at_idx (5u,
                                 ":path",
                                 "/index.html");
  mhdtl_stat_check_entry_at_idx (14u,
                                 ":status",
                                 "500");
  mhdtl_stat_check_entry_at_idx (15u,
                                 "accept-charset",
                                 "");
  mhdtl_stat_check_entry_at_idx (16u,
                                 "accept-encoding",
                                 "gzip, deflate");
  mhdtl_stat_check_entry_at_idx (17u,
                                 "accept-language",
                                 "");
  mhdtl_stat_check_entry_at_idx (31u,
                                 "content-type",
                                 "");
  mhdtl_stat_check_entry_at_idx (42u,
                                 "if-range",
                                 "");
  mhdtl_stat_check_entry_at_idx (47u,
                                 "max-forwards",
                                 "");
  mhdtl_stat_check_entry_at_idx (51u,
                                 "referer",
                                 "");
  mhdtl_stat_check_entry_at_idx (58u,
                                 "user-agent",
                                 "");
  mhdtl_stat_check_entry_at_idx (61u,
                                 "www-authenticate",
                                 "");

  return MHDT_TEST_RESULT ();
}


static int
test_stat_find_non_pseudo_entry_absent (void)
{
  MHDT_EXPECT_UINT_EQ_VAL_D (MHDTL_STBL_FIND_ENTRY_REAL_STR ("foo","bar"), \
                             0u, \
                             "search for non-existing header should fail");
  MHDT_EXPECT_UINT_EQ_VAL_D (MHDTL_STBL_FIND_ENTRY_REAL_STR ("foo",""), \
                             0u, \
                             "search for non-existing header should fail");
  MHDT_EXPECT_UINT_EQ_VAL_D (MHDTL_STBL_FIND_ENTRY_REAL_STR ("","bar"), \
                             0u, \
                             "search for non-existing header should fail");
  MHDT_EXPECT_UINT_EQ_VAL_D (MHDTL_STBL_FIND_ENTRY_REAL_STR ("",""), \
                             0u, \
                             "search for non-existing header should fail");
  MHDT_EXPECT_UINT_EQ_VAL_D (MHDTL_STBL_FIND_ENTRY_REAL_STR (":foo",""), \
                             0u, \
                             "search for non-existing header should fail");
  MHDT_EXPECT_UINT_EQ_VAL_D (MHDTL_STBL_FIND_ENTRY_REAL_STR (":authority",""), \
                             0u, \
                             "search for pseudo-header should fail");
  MHDT_EXPECT_UINT_EQ_VAL_D (MHDTL_STBL_FIND_ENTRY_REAL_STR (":status", \
                                                             "200"), \
                             0u, \
                             "search for pseudo-header should fail");

  return MHDT_TEST_RESULT_D ("search for non-existing non-pseudo headers");
}


static int
test_stat_find_non_pseudo_name_absent (void)
{
  MHDT_EXPECT_UINT_EQ_VAL_D (MHDTL_STBL_FIND_NAME_REAL_STR ("foo"), \
                             0u, \
                             "search for non-existing header should fail");
  MHDT_EXPECT_UINT_EQ_VAL_D (MHDTL_STBL_FIND_NAME_REAL_STR (""), \
                             0u, \
                             "search for non-existing header should fail");
  MHDT_EXPECT_UINT_EQ_VAL_D (MHDTL_STBL_FIND_NAME_REAL_STR (":foo"), \
                             0u, \
                             "search for non-existing header should fail");
  MHDT_EXPECT_UINT_EQ_VAL_D (MHDTL_STBL_FIND_NAME_REAL_STR (":authority"), \
                             0u, \
                             "search for pseudo-header should fail");
  MHDT_EXPECT_UINT_EQ_VAL_D (MHDTL_STBL_FIND_NAME_REAL_STR (":status"), \
                             0u, \
                             "search for pseudo-header should fail");

  return MHDT_TEST_RESULT_D ("search for non-existing non-pseudo names");
}


static int
test_comb_get_find (void)
{
  struct mhd_HpackDTblContext *dyn;

  /* zero-sized table */
  MHDT_EXPECT_PTR_NONNULL_D (dyn = mhd_dtbl_create (456u), \
                             "Create HPACK dynamic table");
  if (NULL == dyn)
    return MHDT_TEST_RESULT ();

  mhdtl_dyn_add_hdr_with_check (dyn, "abc", "xyz");
  mhdtl_dyn_add_hdr_with_check (dyn, "", "empty-name");
  mhdtl_dyn_add_hdr_with_check (dyn, "empty-value", "");
  mhdtl_dyn_add_hdr_with_check (dyn, "", "");
  mhdtl_dyn_add_hdr_with_check (dyn, "2", "two");

  mhdtl_comb_check_entry_at_idx (dyn,
                                 1u,
                                 ":authority",
                                 "");
  mhdtl_comb_check_entry_at_idx (dyn,
                                 5u,
                                 ":path",
                                 "/index.html");
  mhdtl_comb_check_entry_at_idx (dyn,
                                 16u,
                                 "accept-encoding",
                                 "gzip, deflate");
  mhdtl_comb_check_entry_at_idx (dyn,
                                 17u,
                                 "accept-language",
                                 "");
  mhdtl_comb_check_entry_at_idx (dyn,
                                 42u,
                                 "if-range",
                                 "");
  mhdtl_comb_check_entry_at_idx (dyn,
                                 61u,
                                 "www-authenticate",
                                 "");
  mhdtl_comb_check_entry_at_idx (dyn,
                                 62u,
                                 "2",
                                 "two");
  mhdtl_comb_check_entry_at_idx (dyn,
                                 63u,
                                 "",
                                 "");
  mhdtl_comb_check_entry_at_idx (dyn,
                                 64u,
                                 "empty-value",
                                 "");
  mhdtl_comb_check_entry_at_idx (dyn,
                                 65u,
                                 "",
                                 "empty-name");
  mhdtl_comb_check_entry_at_idx (dyn,
                                 66u,
                                 "abc",
                                 "xyz");

  mhd_dtbl_destroy (dyn);

  return MHDT_TEST_RESULT ();
}


static int
test_comb_find_entry_absent (void)
{
  struct mhd_HpackDTblContext *dyn;

  /* zero-sized table */
  MHDT_EXPECT_PTR_NONNULL_D (dyn = mhd_dtbl_create (456u), \
                             "Create HPACK dynamic table");
  if (NULL == dyn)
    return MHDT_TEST_RESULT ();

  mhdtl_dyn_add_hdr_with_check (dyn, "", "");
  mhdtl_dyn_add_hdr_with_check (dyn, "hdr1", "value1");
  mhdtl_dyn_add_hdr_with_check (dyn, "", "empty-name");
  mhdtl_dyn_add_hdr_with_check (dyn, "hdr2", "value2");
  mhdtl_dyn_add_hdr_with_check (dyn, "empty-value", "");

  MHDT_EXPECT_UINT_EQ_VAL_D (MHDTL_HTBL_FIND_ENTRY_REAL_STR (dyn, "foo","bar"), \
                             0u, \
                             "search for non-existing header should fail");
  MHDT_EXPECT_UINT_EQ_VAL_D (MHDTL_HTBL_FIND_ENTRY_REAL_STR (dyn, "foo",""), \
                             0u, \
                             "search for non-existing header should fail");
  MHDT_EXPECT_UINT_EQ_VAL_D (MHDTL_HTBL_FIND_ENTRY_REAL_STR (dyn, "","bar"), \
                             0u, \
                             "search for non-existing header should fail");
  MHDT_EXPECT_UINT_EQ_VAL_D (MHDTL_HTBL_FIND_ENTRY_REAL_STR (dyn, ":foo",""), \
                             0u, \
                             "search for non-existing header should fail");
  MHDT_EXPECT_UINT_EQ_VAL_D (MHDTL_HTBL_FIND_ENTRY_REAL_STR (dyn, \
                                                             ":authority",""), \
                             0u, \
                             "search for pseudo-header should fail");
  MHDT_EXPECT_UINT_EQ_VAL_D (MHDTL_HTBL_FIND_ENTRY_REAL_STR (dyn, ":status", \
                                                             "200"), \
                             0u, \
                             "search for pseudo-header should fail");

  mhdtl_comb_check_entry_at_idx (dyn, mhd_HPACK_STBL_LAST_IDX + 5u,
                                 "",
                                 "");
  mhdtl_comb_check_entry_at_idx (dyn, mhd_HPACK_STBL_LAST_IDX + 1u,
                                 "empty-value",
                                 "");

  mhd_dtbl_destroy (dyn);

  return MHDT_TEST_RESULT ();
}


static int
test_comb_find_name_absent (void)
{
  struct mhd_HpackDTblContext *dyn;

  /* zero-sized table */
  MHDT_EXPECT_PTR_NONNULL_D (dyn = mhd_dtbl_create (456u), \
                             "Create HPACK dynamic table");
  if (NULL == dyn)
    return MHDT_TEST_RESULT ();

  mhdtl_dyn_add_hdr_with_check (dyn, "hdr1", "value1");
  mhdtl_dyn_add_hdr_with_check (dyn, "hdr2", "value2");
  mhdtl_dyn_add_hdr_with_check (dyn, "empty-value", "");
  mhdtl_dyn_add_hdr_with_check (dyn, "", "empty-name");
  mhdtl_dyn_add_hdr_with_check (dyn, "", "");

  MHDT_EXPECT_UINT_EQ_VAL_D (MHDTL_HTBL_FIND_NAME_REAL_STR (dyn, "foo"), \
                             0u, \
                             "search for non-existing header should fail");
  MHDT_EXPECT_UINT_EQ_VAL_D (MHDTL_HTBL_FIND_NAME_REAL_STR (dyn, ":foo"), \
                             0u, \
                             "search for non-existing header should fail");
  MHDT_EXPECT_UINT_EQ_VAL_D (MHDTL_HTBL_FIND_NAME_REAL_STR (dyn, ":authority"), \
                             0u, \
                             "search for pseudo-header should fail");
  MHDT_EXPECT_UINT_EQ_VAL_D (MHDTL_HTBL_FIND_NAME_REAL_STR (dyn, ":status"), \
                             0u, \
                             "search for pseudo-header should fail");

  mhdtl_comb_check_entry_at_idx (dyn,
                                 mhd_HPACK_STBL_LAST_IDX + 1u,
                                 "",
                                 "");
  mhdtl_comb_check_entry_at_idx (dyn,
                                 mhd_HPACK_STBL_LAST_IDX + 5u,
                                 "hdr1",
                                 "value1");

  mhd_dtbl_destroy (dyn);

  return MHDT_TEST_RESULT ();
}


int
main (int argc,
      char *const *argv)
{
  if (argc < 1)
    return 99;

  if (mhdt_has_param (argc, argv, "-v") ||
      mhdt_has_param (argc, argv, "--verbose"))
    MHDT_set_verbosity (MHDT_VERB_LVL_VERBOSE);
  else if (mhdt_has_param (argc, argv, "-s") ||
           mhdt_has_param (argc, argv, "--silent"))
    MHDT_set_verbosity (MHDT_VERB_LVL_SILENT);

  if (! mhdtl_enable_deep_tests)
    mhdtl_enable_deep_tests = mhdt_has_param (argc, argv, "--deep");

  if (mhdt_has_in_name (argv[0], "_dynamic"))
  {
    test_dyn_create_destroy ();
    test_dyn_create_destroy_larger ();
    test_dyn_add_three ();
    test_dyn_add_empty ();
    test_dyn_add_evict_simple ();
    test_dyn_add_evict_wrap ();
    test_dyn_add_evict_inter ();
    test_dyn_evict_to_size ();
    test_dyn_resize ();
    test_dyn_find ();
    test_dyn_reset_on_too_large ();
    test_dyn_zero_sizes ();
    test_dyn_add_series ();
    test_dyn_add_evict_series ();
  }
  else if (mhdt_has_in_name (argv[0], "_static"))
  {
    test_stat_get_find ();
    test_stat_find_non_pseudo_entry_absent ();
    test_stat_find_non_pseudo_name_absent ();
  }
  else if (mhdt_has_in_name (argv[0], "_combined"))
  {
    test_comb_get_find ();
    test_comb_find_entry_absent ();
    test_comb_find_name_absent ();
  }
  else
    MHDT_ERR_EXIT_D ("The test subtype marker string was not found in " \
                     "the name of this binary");

  return MHDT_FINAL_RESULT (argv[0]);
}
