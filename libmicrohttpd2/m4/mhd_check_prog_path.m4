# SPDX-License-Identifier: FSFAP
#
# SYNOPSIS
#
#   MHD_CHECK_PROG_PATH([VAR],[FILENAME])
#
# DESCRIPTION
#
#   If VAR is NOT already set, check whether FILENAME is executable tool
#   in the $PATH. On success set VAR to FILENAME.
#
#   Example usage:
#
#     MHD_CHECK_TOOL_PATH}([NM],["special-nm"])
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

AC_DEFUN([MHD_CHECK_PROG_PATH],[dnl
m4_newline([[# Expansion of $0 macro starts here]])
AS_VAR_SET_IF([$1],[],
  [AC_CHECK_PROG([$1],[$2],[$2])]
)
m4_newline([[# Expansion of $0 macro ends here]])
])dnl AC_DEFUN MHD_CHECK_TOOL_ABS
