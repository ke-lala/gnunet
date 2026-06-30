# SPDX-License-Identifier: FSFAP
#
# SYNOPSIS
#
#   MHD_CHECK_PROG_ABS([VAR],[ABS_FILENAME])
#
# DESCRIPTION
#
#   If VAR is NOT already set, check whether ABS_FILENAME is executable
#   tool. On success set VAR to ABS_FILENAME.
#
#   Example usage:
#
#     MHD_CHECK_TOOL_ABS([NM],["/usr/bin/special-nm"])
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

AC_DEFUN([MHD_CHECK_PROG_ABS],[dnl
m4_newline([[# Expansion of $0 macro starts here]])
AS_VAR_SET_IF([$1],[],
  [
    mhd_check_prog_abs_tool_check=$2
    AC_MSG_CHECKING([fo][r tool ${mhd_check_prog_abs_tool_check}])
    AS_IF([MHD_IS_FILE_EXEC([$2])],[$1=$2])
    AS_VAR_SET_IF([$1],[AC_MSG_RESULT([yes])],[AC_MSG_RESULT([no])])
  ]
)
m4_newline([[# Expansion of $0 macro ends here]])
])dnl AC_DEFUN MHD_CHECK_TOOL_ABS
