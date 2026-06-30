# SPDX-License-Identifier: FSFAP
#
# SYNOPSIS
#
#   MHD_PRINT_FIRST_WORD([some string or $variable])
#
# DESCRIPTION
#
#   This macro prints the first word from the first parameter after
#   performing shell variable expansion and word-splitting.
#
#   Example usage:
#
#     cc_cmd=`MHD_PRINT_FIRST_WORD([$CC])`
#
#   If CC is set to "gcc -std=c11' then just cc_cmd will be set to "gcc".
#
# LICENSE
#
#   Copyright (c) 2026 Karlson2k (Evgeny Grin) <k2k@drgrin.dev>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 1

AC_DEFUN([MHD_PRINT_FIRST_WORD],[dnl
AC_REQUIRE([_MHD_PRINT_FIRST_WORD_BODY])dnl
m4_bmatch([$1], ["], [m4_fatal([$0: First macro argument must not contain double quote char '"'])])dnl
mhd_fn_print_first_word "$1"])dnl AC_DEFUN MHD_PRINT_FIRST_WORD


AC_DEFUN([_MHD_PRINT_FIRST_WORD_BODY], [m4_divert_text([SHELL_FN],[dnl
mhd_fn_print_first_word () {
  set dummy $[]1
  AS_ECHO_N(["$[]2"])
} # _mhd_fn_print_first_word
])])dnl AC_DEFUN _MHD_PRINT_FIRST_WORD_BODY
