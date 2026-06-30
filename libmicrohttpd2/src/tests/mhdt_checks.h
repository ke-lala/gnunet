/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2016-2025 Karlson2k (Evgeny Grin)

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
 * @file src/tests/mhdt_checks.h
 * @brief  MHD test framework helpers
 * @author Karlson2k (Evgeny Grin)
 */
#ifndef MHDT_CHECKS_H
#define MHDT_CHECKS_H 1

#include "mhd_sys_options.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#endif

#if defined(_MSC_VER)
#pragma warning(push)
/* Disable C4505 "unreferenced local function has been removed" */
#pragma warning(disable:4505)
#endif /* _MSC_VER */

#ifndef HAVE_SNPRINTF
#  ifdef _WIN32
#    define snprintf            _snprintf
#    define HAVE_SNPRINTF       1
#  endif
#endif

/*
 * ****** Generic test helpers ******
 */

#ifdef MHD_HAVE_MHD_FUNC_
#  define MHDT_ERR_EXIT() \
        do { fprintf (stderr, "\n\n%s:%lu:%s(): Unexpected error exit!\n", \
                      __FILE__, (unsigned long) __LINE__, MHD_FUNC_); \
             fflush (stderr); exit (99);} while (0)
#  define MHDT_ERR_EXIT_D(desc) \
        do { fprintf (stderr, "\n\n%s:%lu:%s(): Unexpected error exit: %s\n", \
                      __FILE__, (unsigned long) __LINE__, MHD_FUNC_, (desc)); \
             fflush (stderr); exit (99);} while (0)
#else
#  define MHDT_ERR_EXIT() \
        do { fprintf (stderr, "\n\n%s:%lu: Unexpected error exit!\n", \
                      __FILE__, (unsigned long) __LINE__); \
             fflush (stderr); exit (99);} while (0)
#  define MHDT_ERR_EXIT_D(desc) \
        do { fprintf (stderr, "\n\n%s:%lu: Unexpected error exit: %s\n", \
                      __FILE__, (unsigned long) __LINE__, (desc)); \
             fflush (stderr); exit (99);} while (0)
#endif

/**
 * Stringify parameter
 */
#define mhdt_STR_MACRO(x) #x
/**
 * Stringify parameter after expansion
 */
#define mhdt_STR_MACRO_EXP(x) mhdt_STR_MACRO (x)


/*
 * ****** Internal data and functions ******
 */

#define MHDT_NULL_PRNT "[NULL]"

/** Counter for failed checks within one test */
static unsigned int MHDT_checks_err_counter = 0u;

/** Counter for total number of failed tests */
static unsigned int MHDT_tests_err_counter = 0u;

/** Non-zero if current test printed something */
static int MHDT_cur_test_printed_something = 0;


enum MHDT_VerbosityLevelEnum
{
  MHDT_VERB_LVL_SILENT = 0,  /**< only failures are printed */
  MHDT_VERB_LVL_BASIC = 1,   /**< failures and all tests results are printed */
  MHDT_VERB_LVL_VERBOSE = 2  /**< all checks and test results are printed */
};
static enum MHDT_VerbosityLevelEnum MHDT_verbosity_level = MHDT_VERB_LVL_BASIC;

/* Return non-zero if even succeed check result should be printed */
static inline int
mhdt_print_check_succeed (void)
{
  return (MHDT_VERB_LVL_VERBOSE <= MHDT_verbosity_level);
}


/* Return non-zero if succeed check test should be printed */
static inline int
mhdt_print_test_succeed (void)
{
  return (MHDT_VERB_LVL_BASIC <= MHDT_verbosity_level);
}


static inline void
mhdt_mark_print_in_test_set (void)
{
  if (MHDT_cur_test_printed_something)
    return;
  fprintf (stdout, "_____\n");
  fflush (stdout);
  MHDT_cur_test_printed_something = ! 0;
}


static inline void
mhdt_mark_print_in_test_reset (void)
{
  MHDT_cur_test_printed_something = 0;
}


static inline int
mhdt_mark_print_in_test_is_set (void)
{
  return MHDT_cur_test_printed_something;
}


/*
 * ****** Configuration functions ******
 */

static inline void
MHDT_set_verbosity (enum MHDT_VerbosityLevelEnum level)
{
  MHDT_verbosity_level = level;
}


/*
 * ****** Functions for getting results ******
 */


/* Return non-zero if succeeded, zero if failed */
static inline int
MHDT_test_result (const char *filename,
                  long line,
                  const char *func_name,
                  const char *description)
{
#ifdef MHD_HAVE_MHD_FUNC_
  const int have_func_name = ((NULL != func_name) && (0 != func_name[0]));
#else  /* ! MHD_HAVE_MHD_FUNC_ */
  const int have_func_name = 0;
#endif
  const int have_descr = ((NULL != description) && (0 != description[0]));
  const int test_succeed = (0u == MHDT_checks_err_counter);

  MHDT_checks_err_counter = 0u; /* Reset the counter for the next test */

  if (test_succeed && ! mhdt_print_test_succeed ())
    return test_succeed;

  fprintf (test_succeed ? stdout : stderr,
           "%s"
           "%s:%s%s\n"
           "%s:%ld: %s%s%s\n\n"
           "%s",
           (mhdt_mark_print_in_test_is_set () ? "-----\n" : ""),
           (test_succeed ? "Succeed" : "FAILED"),
           (have_descr ? " " : ""),
           (have_descr ? description : ""),
           filename,
           line,
           (have_func_name ? func_name : ""),
           (have_func_name ? "(): " : ""),
           (test_succeed ? "All checks passed" : "Some checks failed"),
           (mhdt_print_check_succeed () ? "\n" : ""));
  fflush (test_succeed ? stdout : stderr);
  mhdt_mark_print_in_test_reset ();

  MHDT_tests_err_counter += (test_succeed ? 0 : 1);

  return test_succeed;
}


#define MHDT_TEST_RESULT() \
        MHDT_test_result (__FILE__, __LINE__, MHD_FUNC_, NULL)
#define MHDT_TEST_RESULT_D(desc) \
        MHDT_test_result (__FILE__, __LINE__, MHD_FUNC_, desc)


/* Return ZERO on success, ONE if any test failed */
static inline int
MHDT_final_result (const char *exec_selfname,
                   const char *filename)
{
  const char *my_name;
  const int no_errors = (0u == MHDT_tests_err_counter);

  if ((NULL != exec_selfname) && (0 != exec_selfname[0]))
    my_name = exec_selfname;
  else if ((NULL != filename) && (0 != filename[0]))
    my_name = filename;
  else
    my_name = "test";

  fprintf ((no_errors ? stdout : stderr),
           "%s: %s\n",
           (no_errors ? "SUCCEED" : "FAILED"),
           my_name);

  fflush (no_errors ? stdout : stderr);

  return no_errors ? 0 : 1;
}


#define MHDT_FINAL_RESULT(exec_selfname) \
        MHDT_final_result (exec_selfname, __FILE__)


/*
 * ****** Individual checkers ******
 */

/* Return non-zero if succeeded, zero if failed */
static inline int
MHDT_check_bool (int result,
                 int expected,
                 const char *check_str,
                 const char *check_str_exp,
                 const char *filename,
                 long line,
                 const char *func_name,
                 const char *description)
{
#ifdef MHD_HAVE_MHD_FUNC_
  const int have_func_name = ((NULL != func_name) && (0 != func_name[0]));
#else  /* ! MHD_HAVE_MHD_FUNC_ */
  const int have_func_name = 0;
#endif
  const int have_descr = ((NULL != description) && (0 != description[0]));
  int exp_different;
  int check_succeed;

  check_succeed = ((! ! result) == (! ! expected));

  if (check_succeed && ! mhdt_print_check_succeed ())
    return check_succeed;

  mhdt_mark_print_in_test_set ();
  exp_different = (0 != strcmp (check_str, check_str_exp));
  fprintf (check_succeed ? stdout : stderr,
           "%s:%s%s\n"
           "%s:%ld: %s%sthe result of the evaluation of the\n"
           "\texpression: '%s'%s%s%s\n"
           "\tActual: '%s'\tExpected: '%s'\n",
           (check_succeed ? "Pass" : "FAIL"),
           (have_descr ? " " : ""),
           (have_descr ? description : ""),
           filename,
           line,
           (have_func_name ? func_name : ""),
           (have_func_name ? "(): " : ""),
           check_str,
           (exp_different ? "\n\texpanded:   '" : ""),
           (exp_different ? check_str_exp : ""),
           (exp_different ? "'" : ""),
           ((! ! result) ? "NON-zero" : "ZERO"),
           ((! ! expected) ? "NON-zero" : "ZERO"));
  fflush (check_succeed ? stdout : stderr);

  MHDT_checks_err_counter += (check_succeed ? 0 : 1);

  return check_succeed;
}


#define MHDT_EXPECT_TRUE(cond) \
        MHDT_check_bool ((cond), ! 0, #cond, \
                         mhdt_STR_MACRO_EXP (cond), __FILE__, __LINE__, \
                         MHD_FUNC_, NULL)
#define MHDT_EXPECT_FALSE(cond) \
        MHDT_check_bool ((cond), 0, #cond, \
                         mhdt_STR_MACRO_EXP (cond), __FILE__, __LINE__, \
                         MHD_FUNC_, NULL)

#define MHDT_EXPECT_TRUE_D(cond, desc) \
        MHDT_check_bool ((cond), ! 0, #cond, \
                         mhdt_STR_MACRO_EXP (cond), __FILE__, __LINE__, \
                         MHD_FUNC_, desc)
#define MHDT_EXPECT_FALSE_D(cond, desc) \
        MHDT_check_bool ((cond), 0, #cond, \
                         mhdt_STR_MACRO_EXP (cond), __FILE__, __LINE__, \
                         MHD_FUNC_, desc)

#define MHDT_ASSERT MHDT_EXPECT_TRUE
#define MHDT_ASSERT_D MHDT_EXPECT_TRUE_D

/* Return non-zero if succeeded, zero if failed */
static inline int
MHDT_check_ptr (const void *ptr,
                int expected_non_null,
                const char *check_str,
                const char *check_str_exp,
                const char *filename,
                long line,
                const char *func_name,
                const char *description)
{
#ifdef MHD_HAVE_MHD_FUNC_
  const int have_func_name = ((NULL != func_name) && (0 != func_name[0]));
#else  /* ! MHD_HAVE_MHD_FUNC_ */
  const int have_func_name = 0;
#endif
  const int have_descr = ((NULL != description) && (0 != description[0]));
  int exp_different;
  int check_succeed;

  check_succeed = ((NULL != ptr) == (! ! expected_non_null));

  if (check_succeed && ! mhdt_print_check_succeed ())
    return check_succeed;

  mhdt_mark_print_in_test_set ();
  exp_different = (0 != strcmp (check_str, check_str_exp));
  fprintf (check_succeed ? stdout : stderr,
           "%s:%s%s\n"
           "%s:%ld: %s%sthe value of the\n"
           "\tpointer:  '%s'%s%s%s\n"
           "\tActual: '%s'\tExpected: '%s'\n",
           (check_succeed ? "Pass" : "FAIL"),
           (have_descr ? " " : ""),
           (have_descr ? description : ""),
           filename,
           line,
           (have_func_name ? func_name : ""),
           (have_func_name ? "(): " : ""),
           check_str,
           (exp_different ? "\n\texpanded: '" : ""),
           (exp_different ? check_str_exp : ""),
           (exp_different ? "'" : ""),
           ((NULL != ptr) ? "non-NULL" : "NULL"),
           ((! ! expected_non_null) ? "non-NULL" : "NULL"));
  fflush (check_succeed ? stdout : stderr);

  MHDT_checks_err_counter += (check_succeed ? 0 : 1);

  return check_succeed;
}


#define MHDT_EXPECT_PTR_NULL(ptr) \
        MHDT_check_ptr ((ptr), 0, #ptr, \
                        mhdt_STR_MACRO_EXP (ptr), __FILE__, __LINE__, MHD_FUNC_, \
                        NULL)
#define MHDT_EXPECT_PTR_NONNULL(ptr) \
        MHDT_check_ptr ((ptr), ! 0, #ptr, \
                        mhdt_STR_MACRO_EXP (ptr), __FILE__, __LINE__, MHD_FUNC_, \
                        NULL)

#define MHDT_EXPECT_PTR_NULL_D(ptr, desc) \
        MHDT_check_ptr ((ptr), 0, #ptr, \
                        mhdt_STR_MACRO_EXP (ptr), __FILE__, __LINE__, MHD_FUNC_, \
                        desc)
#define MHDT_EXPECT_PTR_NONNULL_D(ptr, desc) \
        MHDT_check_ptr ((ptr), ! 0, #ptr, \
                        mhdt_STR_MACRO_EXP (ptr), __FILE__, __LINE__, MHD_FUNC_, \
                        desc)

#ifdef PRIuMAX
typedef uintmax_t mhdt_uint_t;
#  define MHDT_UINT_PRI PRIuMAX
#  define MHDT_UINT_PRI_CAST(val) val
#elif defined(PRIuFAST64)
typedef uint_fast64_t mhdt_uint_t;
#  define MHDT_UINT_PRI PRIuFAST64
#  define MHDT_UINT_PRI_CAST(val) val
#else
typedef uint_fast64_t mhdt_uint_t;
#  define MHDT_UINT_PRI "lu"
#  define MHDT_UINT_PRI_CAST(val) ((unsigned long) val)
#endif

/**
 * Comparison type
 */
enum mhdt_CmpType
{
  MHDT_CMP_EQ,/**< a == b */
  MHDT_CMP_LT,/**< a < b */
  MHDT_CMP_LE,/**< a <= b */
  MHDT_CMP_NE,/**< a != b */
  MHDT_CMP_GE,/**< a >= b */
  MHDT_CMP_GT,/**< a > b */
};


/* Return non-zero if succeeded, zero if failed */
static inline int
MHDT_check_ui_cmp (mhdt_uint_t val_a,
                   enum mhdt_CmpType cmp_t,
                   mhdt_uint_t val_b,
                   const char *val_a_str,
                   const char *val_b_str,
                   const char *val_a_str_exp,
                   const char *val_b_str_exp,
                   const char *filename,
                   long line,
                   const char *func_name,
                   const char *description)
{
#ifdef MHD_HAVE_MHD_FUNC_
  const int have_func_name = ((NULL != func_name) && (0 != func_name[0]));
#else  /* ! MHD_HAVE_MHD_FUNC_ */
  const int have_func_name = 0;
#endif
  const int have_descr = ((NULL != description) && (0 != description[0]));
  char val_a_buf[512] = "";
  char val_b_buf[512] = "";
  int use_buf_print = 0;
  int str_has_suff;
  int both_str_has_suff;
  const size_t buf_size = (sizeof(val_a_buf) / sizeof(char));
  const char *cmp_str;
  int check_succeed;

  switch (cmp_t)
  {
  case MHDT_CMP_EQ:
    check_succeed = (val_a == val_b);
    cmp_str = "==";
    break;
  case MHDT_CMP_LT:
    check_succeed = (val_a < val_b);
    cmp_str = "<";
    break;
  case MHDT_CMP_LE:
    check_succeed = (val_a <= val_b);
    cmp_str = "<=";
    break;
  case MHDT_CMP_NE:
    check_succeed = (val_a != val_b);
    cmp_str = "!=";
    break;
  case MHDT_CMP_GE:
    check_succeed = (val_a >= val_b);
    cmp_str = ">=";
    break;
  case MHDT_CMP_GT:
    check_succeed = (val_a > val_b);
    cmp_str = ">";
    break;
  default:
    MHDT_ERR_EXIT ();
    break;
  }

  if (check_succeed && ! mhdt_print_check_succeed ())
    return check_succeed;

  both_str_has_suff = ! 0;
  str_has_suff = 0;
  if (1) /* For local scope */
  {
    const mhdt_uint_t val = val_a;
    char *const print_buf = val_a_buf;
    const char *const str = val_a_str_exp;
    int print_res;

    print_res =
#ifdef HAVE_SNPRINTF
      snprintf (print_buf, buf_size,
#else
      sprintf (print_buf,
#endif
                "%" MHDT_UINT_PRI,
                MHDT_UINT_PRI_CAST (val));
    if ((0 > print_res) ||
        (buf_size <= (unsigned int) print_res))
      MHDT_ERR_EXIT ();

    if (0 != strcmp (str, print_buf))
    {
      const size_t str_len = strlen (str);
      if (str_len - 1 != (unsigned int) print_res)
        use_buf_print = ! 0;
      else if ((('u' == str[str_len - 1u]) || ('U' == str[str_len - 1u]))
               && (0 == memcmp (str, print_buf, str_len - 1u)))
        str_has_suff = ! 0;
      else
        use_buf_print = ! 0;
    }
    else
      both_str_has_suff = 0;
  }
  if (1) /* For local scope */
  {
    const mhdt_uint_t val = val_b;
    char *const print_buf = val_b_buf;
    const char *const str = val_b_str_exp;
    int print_res;

    print_res =
#ifdef HAVE_SNPRINTF
      snprintf (print_buf, buf_size,
#else
      sprintf (print_buf,
#endif
                "%" MHDT_UINT_PRI,
                MHDT_UINT_PRI_CAST (val));
    if ((0 > print_res) ||
        (buf_size <= (unsigned int) print_res))
      MHDT_ERR_EXIT ();

    if (0 != strcmp (str, print_buf))
    {
      const size_t str_len = strlen (str);
      if (str_len - 1 != (unsigned int) print_res)
        use_buf_print = ! 0;
      else if ((('u' == str[str_len - 1u]) || ('U' == str[str_len - 1u]))
               && (0 == memcmp (str, print_buf, str_len - 1u)))
        str_has_suff = ! 0;
      else
        use_buf_print = ! 0;
    }
    else
      both_str_has_suff = 0;
  }
  if (! use_buf_print && str_has_suff && ! both_str_has_suff)
    use_buf_print = ! 0;

  mhdt_mark_print_in_test_set ();
  fprintf (check_succeed ? stdout : stderr,
           "%s:%s%s\n"
           "%s:%ld: %s%sThe %s check:\n"
           "\toriginal:  '%s %s %s'\n",
           (check_succeed ? "Pass" : "FAIL"),
            (have_descr ? " " : ""),
             (have_descr ? description : ""),
           filename,
           line,
           (have_func_name ? func_name : ""),
           (have_func_name ? "(): " : ""),
           (check_succeed ? "passed" : "failed"),
           val_a_str, cmp_str, val_b_str);
  if ((0 != strcmp (val_a_str, val_a_str_exp)) ||
      (0 != strcmp (val_b_str, val_b_str_exp)))
    fprintf (check_succeed ? stdout : stderr,
             "\texpanded:  '%s %s %s'\n",
             val_a_str_exp, cmp_str, val_b_str_exp);
  if (use_buf_print)
    fprintf (check_succeed ? stdout : stderr,
             "\tvalues:    '%s %s %s'\n",
             val_a_buf, cmp_str, val_b_buf);

  fflush (check_succeed ? stdout : stderr);

  MHDT_checks_err_counter += (check_succeed ? 0 : 1);

  return check_succeed;
}


#define MHDT_EXPECT_UINT_EQ_VAL(uiv_l,uiv_r) \
        MHDT_check_ui_cmp ((uiv_l), MHDT_CMP_EQ, (uiv_r), \
                           #uiv_l, #uiv_r, \
                           mhdt_STR_MACRO_EXP (uiv_l), \
                           mhdt_STR_MACRO_EXP (uiv_r), \
                           __FILE__, __LINE__, MHD_FUNC_, NULL)
#define MHDT_EXPECT_UINT_LT_VAL(uiv_l,uiv_r) \
        MHDT_check_ui_cmp ((uiv_l), MHDT_CMP_LT, (uiv_r), \
                           #uiv_l, #uiv_r, \
                           mhdt_STR_MACRO_EXP (uiv_l), \
                           mhdt_STR_MACRO_EXP (uiv_r), \
                           __FILE__, __LINE__, MHD_FUNC_, NULL)
#define MHDT_EXPECT_UINT_LE_VAL(uiv_l,uiv_r) \
        MHDT_check_ui_cmp ((uiv_l), MHDT_CMP_LE, (uiv_r), \
                           #uiv_l, #uiv_r, \
                           mhdt_STR_MACRO_EXP (uiv_l), \
                           mhdt_STR_MACRO_EXP (uiv_r), \
                           __FILE__, __LINE__, MHD_FUNC_, NULL)
#define MHDT_EXPECT_UINT_NE_VAL(uiv_l,uiv_r) \
        MHDT_check_ui_cmp ((uiv_l), MHDT_CMP_NE, (uiv_r), \
                           #uiv_l, #uiv_r, \
                           mhdt_STR_MACRO_EXP (uiv_l), \
                           mhdt_STR_MACRO_EXP (uiv_r), \
                           __FILE__, __LINE__, MHD_FUNC_, NULL)
#define MHDT_EXPECT_UINT_GE_VAL(uiv_l,uiv_r) \
        MHDT_check_ui_cmp ((uiv_l), MHDT_CMP_GE, (uiv_r), \
                           #uiv_l, #uiv_r, \
                           mhdt_STR_MACRO_EXP (uiv_l), \
                           mhdt_STR_MACRO_EXP (uiv_r), \
                           __FILE__, __LINE__, MHD_FUNC_, NULL)
#define MHDT_EXPECT_UINT_GT_VAL(uiv_l,uiv_r) \
        MHDT_check_ui_cmp ((uiv_l), MHDT_CMP_GT, (uiv_r), \
                           #uiv_l, #uiv_r, \
                           mhdt_STR_MACRO_EXP (uiv_l), \
                           mhdt_STR_MACRO_EXP (uiv_r), \
                           __FILE__, __LINE__, MHD_FUNC_, NULL)

#define MHDT_EXPECT_UINT_EQ_VAL_D(uiv_l,uiv_r,desc) \
        MHDT_check_ui_cmp ((uiv_l), MHDT_CMP_EQ, (uiv_r), \
                           #uiv_l, #uiv_r, \
                           mhdt_STR_MACRO_EXP (uiv_l), \
                           mhdt_STR_MACRO_EXP (uiv_r), \
                           __FILE__, __LINE__, MHD_FUNC_, desc)
#define MHDT_EXPECT_UINT_LT_VAL_D(uiv_l,uiv_r,desc) \
        MHDT_check_ui_cmp ((uiv_l), MHDT_CMP_LT, (uiv_r), \
                           #uiv_l, #uiv_r, \
                           mhdt_STR_MACRO_EXP (uiv_l), \
                           mhdt_STR_MACRO_EXP (uiv_r), \
                           __FILE__, __LINE__, MHD_FUNC_, desc)
#define MHDT_EXPECT_UINT_LE_VAL_D(uiv_l,uiv_r,desc) \
        MHDT_check_ui_cmp ((uiv_l), MHDT_CMP_LE, (uiv_r), \
                           #uiv_l, #uiv_r, \
                           mhdt_STR_MACRO_EXP (uiv_l), \
                           mhdt_STR_MACRO_EXP (uiv_r), \
                           __FILE__, __LINE__, MHD_FUNC_, desc)
#define MHDT_EXPECT_UINT_NE_VAL_D(uiv_l,uiv_r,desc) \
        MHDT_check_ui_cmp ((uiv_l), MHDT_CMP_NE, (uiv_r), \
                           #uiv_l, #uiv_r, \
                           mhdt_STR_MACRO_EXP (uiv_l), \
                           mhdt_STR_MACRO_EXP (uiv_r), \
                           __FILE__, __LINE__, MHD_FUNC_, desc)
#define MHDT_EXPECT_UINT_GE_VAL_D(uiv_l,uiv_r,desc) \
        MHDT_check_ui_cmp ((uiv_l), MHDT_CMP_GE, (uiv_r), \
                           #uiv_l, #uiv_r, \
                           mhdt_STR_MACRO_EXP (uiv_l), \
                           mhdt_STR_MACRO_EXP (uiv_r), \
                           __FILE__, __LINE__, MHD_FUNC_, desc)
#define MHDT_EXPECT_UINT_GT_VAL_D(uiv_l,uiv_r,desc) \
        MHDT_check_ui_cmp ((uiv_l), MHDT_CMP_GT, (uiv_r), \
                           #uiv_l, #uiv_r, \
                           mhdt_STR_MACRO_EXP (uiv_l), \
                           mhdt_STR_MACRO_EXP (uiv_r), \
                           __FILE__, __LINE__, MHD_FUNC_, desc)


#ifdef PRIdMAX
typedef intmax_t mhdt_int_t;
#  define MHDT_INT_PRI PRIdMAX
#  define MHDT_INT_PRI_CAST(val) val
#elif defined(PRIdFAST64)
typedef int_fast64_t mhdt_int_t;
#  define MHDT_INT_PRI PRIdFAST64
#  define MHDT_INT_PRI_CAST(val) val
#else
typedef int_fast64_t mhdt_int_t;
#  define MHDT_INT_PRI "ld"
#  define MHDT_INT_PRI_CAST(val) ((long) val)
#endif


/* Return non-zero if succeeded, zero if failed */
static inline int
MHDT_check_i_cmp (mhdt_int_t val_a,
                  enum mhdt_CmpType cmp_t,
                  mhdt_int_t val_b,
                  const char *val_a_str,
                  const char *val_b_str,
                  const char *val_a_str_exp,
                  const char *val_b_str_exp,
                  const char *filename,
                  long line,
                  const char *func_name,
                  const char *description)
{
#ifdef MHD_HAVE_MHD_FUNC_
  const int have_func_name = ((NULL != func_name) && (0 != func_name[0]));
#else  /* ! MHD_HAVE_MHD_FUNC_ */
  const int have_func_name = 0;
#endif
  const int have_descr = ((NULL != description) && (0 != description[0]));
  char val_a_buf[512] = "";
  char val_b_buf[512] = "";
  int use_buf_print = 0;
  const size_t buf_size = (sizeof(val_a_buf) / sizeof(char));
  const char *cmp_str;
  int check_succeed;

  switch (cmp_t)
  {
  case MHDT_CMP_EQ:
    check_succeed = (val_a == val_b);
    cmp_str = "==";
    break;
  case MHDT_CMP_LT:
    check_succeed = (val_a < val_b);
    cmp_str = "<";
    break;
  case MHDT_CMP_LE:
    check_succeed = (val_a <= val_b);
    cmp_str = "<=";
    break;
  case MHDT_CMP_NE:
    check_succeed = (val_a != val_b);
    cmp_str = "!=";
    break;
  case MHDT_CMP_GE:
    check_succeed = (val_a >= val_b);
    cmp_str = ">=";
    break;
  case MHDT_CMP_GT:
    check_succeed = (val_a > val_b);
    cmp_str = ">";
    break;
  default:
    MHDT_ERR_EXIT ();
    break;
  }

  if (check_succeed && ! mhdt_print_check_succeed ())
    return check_succeed;

  if (1) /* For local scope */
  {
    const mhdt_int_t val = val_a;
    char *const print_buf = val_a_buf;
    const char *const str = val_a_str_exp;
    int print_res;

    print_res =
#ifdef HAVE_SNPRINTF
      snprintf (print_buf, buf_size,
#else
      sprintf (print_buf,
#endif
                "%" MHDT_INT_PRI,
                MHDT_INT_PRI_CAST (val));
    if ((0 > print_res) ||
        (buf_size <= (unsigned int) print_res))
      MHDT_ERR_EXIT ();

    if (0 != strcmp (str, print_buf))
      use_buf_print = ! 0;
  }
  if (1) /* For local scope */
  {
    const mhdt_int_t val = val_b;
    char *const print_buf = val_b_buf;
    const char *const str = val_b_str_exp;
    int print_res;

    print_res =
#ifdef HAVE_SNPRINTF
      snprintf (print_buf, buf_size,
#else
      sprintf (print_buf,
#endif
                "%" MHDT_INT_PRI,
                MHDT_INT_PRI_CAST (val));
    if ((0 > print_res) ||
        (buf_size <= (unsigned int) print_res))
      MHDT_ERR_EXIT ();

    if (0 != strcmp (str, print_buf))
      use_buf_print = ! 0;
  }

  mhdt_mark_print_in_test_set ();
  fprintf (check_succeed ? stdout : stderr,
           "%s:%s%s\n"
           "%s:%ld: %s%sThe %s check:\n"
           "\toriginal:  '%s %s %s'\n",
           (check_succeed ? "Pass" : "FAIL"),
            (have_descr ? " " : ""),
             (have_descr ? description : ""),
           filename,
           line,
           (have_func_name ? func_name : ""),
           (have_func_name ? "(): " : ""),
           (check_succeed ? "passed" : "failed"),
           val_a_str, cmp_str, val_b_str);
  if ((0 != strcmp (val_a_str, val_a_str_exp)) ||
      (0 != strcmp (val_b_str, val_b_str_exp)))
    fprintf (check_succeed ? stdout : stderr,
             "\texpanded:  '%s %s %s'\n",
             val_a_str_exp, cmp_str, val_b_str_exp);
  if (use_buf_print)
    fprintf (check_succeed ? stdout : stderr,
             "\tvalues:    '%s %s %s'\n",
             val_a_buf, cmp_str, val_b_buf);

  fflush (check_succeed ? stdout : stderr);

  MHDT_checks_err_counter += (check_succeed ? 0 : 1);

  return check_succeed;
}


#define MHDT_EXPECT_INT_EQ_VAL(iv_l,iv_r) \
        MHDT_check_i_cmp ((iv_l), MHDT_CMP_EQ, (iv_r), \
                          #iv_l, #iv_r, \
                          mhdt_STR_MACRO_EXP (iv_l), mhdt_STR_MACRO_EXP (iv_r), \
                          __FILE__, __LINE__, MHD_FUNC_, NULL)
#define MHDT_EXPECT_INT_LT_VAL(iv_l,iv_r) \
        MHDT_check_i_cmp ((iv_l), MHDT_CMP_LT, (iv_r), \
                          #iv_l, #iv_r, \
                          mhdt_STR_MACRO_EXP (iv_l), mhdt_STR_MACRO_EXP (iv_r), \
                          __FILE__, __LINE__, MHD_FUNC_, NULL)
#define MHDT_EXPECT_INT_LE_VAL(iv_l,iv_r) \
        MHDT_check_i_cmp ((iv_l), MHDT_CMP_LE, (iv_r), \
                          #iv_l, #iv_r, \
                          mhdt_STR_MACRO_EXP (iv_l), mhdt_STR_MACRO_EXP (iv_r), \
                          __FILE__, __LINE__, MHD_FUNC_, NULL)
#define MHDT_EXPECT_INT_NE_VAL(iv_l,iv_r) \
        MHDT_check_i_cmp ((iv_l), MHDT_CMP_NE, (iv_r), \
                          #iv_l, #iv_r, \
                          mhdt_STR_MACRO_EXP (iv_l), mhdt_STR_MACRO_EXP (iv_r), \
                          __FILE__, __LINE__, MHD_FUNC_, NULL)
#define MHDT_EXPECT_INT_GE_VAL(iv_l,iv_r) \
        MHDT_check_i_cmp ((iv_l), MHDT_CMP_GE, (iv_r), \
                          #iv_l, #iv_r, \
                          mhdt_STR_MACRO_EXP (iv_l), mhdt_STR_MACRO_EXP (iv_r), \
                          __FILE__, __LINE__, MHD_FUNC_, NULL)
#define MHDT_EXPECT_INT_GT_VAL(iv_l,iv_r) \
        MHDT_check_i_cmp ((iv_l), MHDT_CMP_GT, (iv_r), \
                          #iv_l, #iv_r, \
                          mhdt_STR_MACRO_EXP (iv_l), mhdt_STR_MACRO_EXP (iv_r), \
                          __FILE__, __LINE__, MHD_FUNC_, NULL)

#define MHDT_EXPECT_INT_EQ_VAL_D(iv_l,iv_r,desc) \
        MHDT_check_i_cmp ((iv_l), MHDT_CMP_EQ, (iv_r), \
                          #iv_l, #iv_r, \
                          mhdt_STR_MACRO_EXP (iv_l), mhdt_STR_MACRO_EXP (iv_r), \
                          __FILE__, __LINE__, MHD_FUNC_, desc)
#define MHDT_EXPECT_INT_LT_VAL_D(iv_l,iv_r,desc) \
        MHDT_check_i_cmp ((iv_l), MHDT_CMP_LT, (iv_r), \
                          #iv_l, #iv_r, \
                          mhdt_STR_MACRO_EXP (iv_l), mhdt_STR_MACRO_EXP (iv_r), \
                          __FILE__, __LINE__, MHD_FUNC_, desc)
#define MHDT_EXPECT_INT_LE_VAL_D(iv_l,iv_r,desc) \
        MHDT_check_i_cmp ((iv_l), MHDT_CMP_LE, (iv_r), \
                          #iv_l, #iv_r, \
                          mhdt_STR_MACRO_EXP (iv_l), mhdt_STR_MACRO_EXP (iv_r), \
                          __FILE__, __LINE__, MHD_FUNC_, desc)
#define MHDT_EXPECT_INT_NE_VAL_D(iv_l,iv_r,desc) \
        MHDT_check_i_cmp ((iv_l), MHDT_CMP_NE, (iv_r), \
                          #iv_l, #iv_r, \
                          mhdt_STR_MACRO_EXP (iv_l), mhdt_STR_MACRO_EXP (iv_r), \
                          __FILE__, __LINE__, MHD_FUNC_, desc)
#define MHDT_EXPECT_INT_GE_VAL_D(iv_l,iv_r,desc) \
        MHDT_check_i_cmp ((iv_l), MHDT_CMP_GE, (iv_r), \
                          #iv_l, #iv_r, \
                          mhdt_STR_MACRO_EXP (iv_l), mhdt_STR_MACRO_EXP (iv_r), \
                          __FILE__, __LINE__, MHD_FUNC_, desc)
#define MHDT_EXPECT_INT_GT_VAL_D(iv_l,iv_r,desc) \
        MHDT_check_i_cmp ((iv_l), MHDT_CMP_GT, (iv_r), \
                          #iv_l, #iv_r, \
                          mhdt_STR_MACRO_EXP (iv_l), mhdt_STR_MACRO_EXP (iv_r), \
                          __FILE__, __LINE__, MHD_FUNC_, desc)


/* Return non-zero if succeeded, zero if failed */
static MHD_FN_PAR_CSTR_ (1) MHD_FN_PAR_CSTR_ (2) inline int
MHDT_check_str (const char *result,
                const char *expected,
                const char *check_str,
                const char *check_str_exp,
                const char *filename,
                long line,
                const char *func_name,
                const char *description)
{
#ifdef MHD_HAVE_MHD_FUNC_
  const int have_func_name = ((NULL != func_name) && (0 != func_name[0]));
#else  /* ! MHD_HAVE_MHD_FUNC_ */
  const int have_func_name = 0;
#endif
  const int have_descr = ((NULL != description) && (0 != description[0]));
  int exp_different;
  int check_succeed;

  if (NULL == expected)
    MHDT_ERR_EXIT_D ("'expected' must not be NULL");

  check_succeed = ((NULL != result) && (0 == strcmp (result, expected)));

  if (check_succeed && ! mhdt_print_check_succeed ())
    return check_succeed;

  mhdt_mark_print_in_test_set ();
  exp_different = (0 != strcmp (check_str, check_str_exp));
  fprintf (check_succeed ? stdout : stderr,
           "%s:%s%s\n"
           "%s:%ld: %s%sThe value of '%s'%s%s%s:\n"
           "\tActual: \"%s\"\tExpected: \"%s\"\n",
           (check_succeed ? "Pass" : "FAIL"),
           (have_descr ? " " : ""),
           (have_descr ? description : ""),
           filename,
           line,
           (have_func_name ? func_name : ""),
           (have_func_name ? "(): " : ""),
           check_str,
           (exp_different ? " (expanded: '" : ""),
           (exp_different ? check_str_exp : ""),
           (exp_different ? "')" : ""),
           ((NULL == result) ? MHDT_NULL_PRNT : result),
           expected);
  fflush (check_succeed ? stdout : stderr);

  MHDT_checks_err_counter += (check_succeed ? 0 : 1);

  return check_succeed;
}


#define MHDT_EXPECT_STR_EQ(str, expected) \
        MHDT_check_str ((str), (expected), \
                        #str, mhdt_STR_MACRO_EXP (str), \
                        __FILE__, __LINE__, MHD_FUNC_, \
                        NULL)
#define MHDT_EXPECT_STR_EQ_D(str, expected, desc) \
        MHDT_check_str ((str), (expected), \
                        #str, mhdt_STR_MACRO_EXP (str), \
                        __FILE__, __LINE__, MHD_FUNC_, \
                        desc)

/* 'result' must not be NULL */
/* Return non-zero if succeeded, zero if failed */
static MHD_FN_PAR_CSTR_ (3) inline int
MHDT_check_strn_str (size_t result_len,
                     const char *result,
                     const char *expected,
                     const char *filename,
                     long line,
                     const char *func_name,
                     const char *description)
{
#ifdef MHD_HAVE_MHD_FUNC_
  const int have_func_name = ((NULL != func_name) && (0 != func_name[0]));
#else  /* ! MHD_HAVE_MHD_FUNC_ */
  const int have_func_name = 0;
#endif
  const int have_descr = ((NULL != description) && (0 != description[0]));
  int check_succeed;

  if (NULL == expected)
    MHDT_ERR_EXIT_D ("'expected' must not be NULL");

  check_succeed = ((NULL != result)
                   && (result_len == strlen (expected))
                   && (0 == memcmp (result, expected, result_len)));

  if (check_succeed && ! mhdt_print_check_succeed ())
    return check_succeed;

  mhdt_mark_print_in_test_set ();
  fprintf (check_succeed ? stdout : stderr,
           "%s:%s%s\n"
           "%s:%ld: %s%s\n"
           "\tActual: \"%.*s\" (len: %lu)" /* mis-print binary zero */
           "\tExpected: \"%s\" (len: %lu)\n",
           (check_succeed ? "Pass" : "FAIL"),
           (have_descr ? " " : ""),
           (have_descr ? description : ""),
           filename,
           line,
           (have_func_name ? func_name : ""),
           (have_func_name ? "(): " : ""),
           (int) (result ? result_len : strlen (MHDT_NULL_PRNT)),
           (result ? result : MHDT_NULL_PRNT), (unsigned long) result_len,
           expected, (unsigned long) strlen (expected));
  fflush (check_succeed ? stdout : stderr);

  MHDT_checks_err_counter += (check_succeed ? 0 : 1);

  return check_succeed;
}


#define MHDT_EXPECT_STRN_EQ_STR_D(str_len, str, expected, desc) \
        MHDT_check_strn_str ((str_len), (str), (expected), \
                             __FILE__, __LINE__, MHD_FUNC_, \
                             desc)


/* if 'result_len' is zero then 'result' is ignored */
/* Return non-zero if succeeded, zero if failed */
static inline int
MHDT_check_strn (size_t result_len,
                 const char *result,
                 size_t expected_len,
                 const char *expected,
                 const char *filename,
                 long line,
                 const char *func_name,
                 const char *description)
{
#ifdef MHD_HAVE_MHD_FUNC_
  const int have_func_name = ((NULL != func_name) && (0 != func_name[0]));
#else  /* ! MHD_HAVE_MHD_FUNC_ */
  const int have_func_name = 0;
#endif
  const int have_descr = ((NULL != description) && (0 != description[0]));
  int check_succeed;

  if ((NULL == expected) && (0u != expected_len))
    MHDT_ERR_EXIT_D ("'expected' must not be NULL or " \
                     "'expected_len' must be zero ");

  check_succeed = (result_len == expected_len);
  if (check_succeed && (0u != expected_len))
    check_succeed = (0 == memcmp (result, expected, result_len));

  if (check_succeed && ! mhdt_print_check_succeed ())
    return check_succeed;

  mhdt_mark_print_in_test_set ();
  fprintf (check_succeed ? stdout : stderr,
           "%s:%s%s\n"
           "%s:%ld: %s%s\n"
           "\tActual: \"%.*s\" (len: %lu)"      /* mis-print binary zero */
           "\tExpected: \"%.*s\" (len: %lu)\n", /* mis-print binary zero */
           (check_succeed ? "Pass" : "FAIL"),
           (have_descr ? " " : ""),
           (have_descr ? description : ""),
           filename,
           line,
           (have_func_name ? func_name : ""),
           (have_func_name ? "(): " : ""),
           (int) (result ? result_len : strlen (MHDT_NULL_PRNT)),
           (result ? result : MHDT_NULL_PRNT), (unsigned long) result_len,
           (int) (expected ? expected_len : strlen (MHDT_NULL_PRNT)),
           (expected ? expected : MHDT_NULL_PRNT),
           (unsigned long) expected_len);
  fflush (check_succeed ? stdout : stderr);

  MHDT_checks_err_counter += (check_succeed ? 0 : 1);

  return check_succeed;
}


#define MHDT_EXPECT_STRN_EQ_D(str_len, str, expected_len, expected, desc) \
        MHDT_check_strn ((str_len), (str), (expected_len), (expected), \
                         __FILE__, __LINE__, MHD_FUNC_, \
                         desc)




#if defined(_MSC_VER)
/* Restore warnings */
#pragma warning(pop)
#endif /* _MSC_VER */

#endif /* ! MHDT_CHECKS_H */
