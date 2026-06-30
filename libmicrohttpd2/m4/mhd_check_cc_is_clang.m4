# SPDX-License-Identifier: FSFAP
#
# SYNOPSIS
#
#   MHD_CHECK_CC_IS_CLANG([ACTION-IF-CLANG], [ACTION-IF-NOT-CLANG])
#
# DESCRIPTION
#
#   This macro checks whether the compiler set by $CC is actually clang or
#   llvm-based compiler.
#   The result is cached in variable mhd_cv_cc_clang_based.
#
#   Example usage:
#
#     MHD_CHECK_CC_IS_CLANG
#
#
# LICENSE
#
#   Copyright (c) 2022-2026 Karlson2k (Evgeny Grin) <k2k@drgrin.dev>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 1

AC_DEFUN([MHD_CHECK_CC_IS_CLANG],[dnl
AC_PREREQ([2.64])dnl
AC_REQUIRE([AC_PROG_CC])dnl
AC_CACHE_CHECK([whether $CC is clang or llvm-based],
[mhd_cv_cc_clang_based],
[AS_VAR_IF([GCC],["yes"],
[AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#if ! defined(__clang__) && ! defined(__llvm__)
#error This compiler is not clang nor llvm-based compiler
fail test here %%%@<:@-1@:>@
#endif
void test_func1(void);
void test_func1(void) {return;}
]])],dnl AC_LANG_SOURCE
[mhd_cv_cc_clang_based="yes"],[mhd_cv_cc_clang_based="no"])dnl AC_COMPILE_IFELSE
],[mhd_cv_cc_clang_based="no"])dnl AS_VAR_IF GCC
])
dnl AC_CACHE_CHECK
m4_n([m4_ifnblank([$1$2],[AS_VAR_IF([mhd_cv_cc_clang_based],["yes"],[$1],[$2])])])dnl
# Re-use result in AX_PTHREAD macro
AS_VAR_SET_IF([ax_cv_PTHREAD_CLANG],[:],[ax_cv_PTHREAD_CLANG="$mhd_cv_cc_clang_based"])
])dnl AC_DEFUN MHD_CHECK_ADD_CC_CFLAG
