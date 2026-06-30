# SPDX-License-Identifier: FSFAP
#
# SYNOPSIS
#
#   MHD_IS_FILE_EXEC([pathname])
#
# DESCRIPTION
#
#   This produces zero shell status code if specified file is executable or
#   non-zero shell status code if specified file does not exist, is not
#   executable or is a directory.
#   On platform, where the system may automatically add executable
#   suffix (extension) when command is called, this macro checks also
#   pathname combined with automatic suffix.
#   To avoid possible word splitting, put parameter in shell quotes.
#
#   Example usage:
#
#     AS_IF([MHD_IS_FILE_EXEC(["/usr/bin/sometool"])],
#      [AC_MSG_WARNING([/usr/bin/sometool not available])])
#
# LICENSE
#
#   Copyright (c) 2026 Karlson2k (Evgeny Grin) <k2k@drgrin.dev>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 2

AC_DEFUN([MHD_IS_FILE_EXEC],[dnl
AC_REQUIRE([_MHD_IS_FILE_EXEC_BODY])dnl
mhd_fn_is_file_exec $1])dnl AC_DEFUN MHD_IS_FILE_EXEC


AC_DEFUN([_MHD_IS_FILE_EXEC_BODY], [m4_divert_text([SHELL_FN],[dnl
mhd_fn_is_file_exec () {
  for mhd_test_exec_ext in "" $ac_executable_extensions ; do
    AS_IF([AS_EXECUTABLE_P(["${1}${mhd_test_exec_ext}"])],[return 0])
  done
  return 1
} # mhd_fn_is_file_exec
])])dnl AC_DEFUN _MHD_IS_FILE_EXEC_BODY
