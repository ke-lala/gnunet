# SPDX-License-Identifier: FSFAP
#
# SYNOPSIS
#
#   MHD_PKG_CONF_MODULE_VERSION(VARIABLE-PREFIX, [MODULE],
#                               [ACTION-IF-DETECTED],
#                               [ACTION-IF-NOT-DETECTED])
#
# DESCRIPTION
#
#   Set the variable [VARIABLE-PREFIX]_MOD_VERSION to the version of
#   MODULE reported by pkg-config.
#   MODULE may contain a version specification, e.g. modname >= 1.2.
#   MODULE defaults to VARIABLE-PREFIX converted to lower case letters.
#   If pkg-config is not available, MODULE is not found or the MODULE
#   version cannot be detected for any other reason, then the macro does
#   not assign the variable [VARIABLE-PREFIX]_MOD_VERSION and runs
#   ACTION-IF-NOT-DETECTED; otherwise ACTION-IF-DETECTED is run.
#   The detection result is cached.
#
#   Example usage:
#
#     MHD_PKG_CONF_MODULE_VERSION([GNUTLS],[],[],[GNUTLS_MOD_VERSION='0'])
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

AC_DEFUN([MHD_PKG_CONF_MODULE_VERSION],[dnl
m4_ifblank([$1], [m4_fatal([$0: First macro argument must not be empty])])dnl
m4_bmatch(m4_normalize([$1]), [\s],dnl
[m4_fatal([$0: First macro argument must not contain whitespaces])])dnl
m4_bmatch([$1],[,],[m4_fatal([$0: First macro argument must not contain comma])])dnl
AC_REQUIRE([PKG_PROG_PKG_CONFIG])dnl
m4_newline([@%:@[ Expansion of $0 macro starts here]])
m4_pushdef([mhd_MODULEID],[m4_default_nblank([$2],[m4_tolower(m4_normalize([$1]))])])
AS_VAR_PUSHDEF([mhd_out_VAR],m4_normalize([$1])[_MOD_VERSION])dnl
AS_VAR_PUSHDEF([mhd_cv_VAR],[mhd_cv_]m4_normalize([$1])[_pkg_mod_ver])dnl
AC_CACHE_CHECK([for "mhd_MODULEID" pkg-config module version],[mhd_cv_VAR],
[
AS_IF([test -n "${PKG_CONFIG}"],
[mhd_cv_VAR=`$PKG_CONFIG --modversion "mhd_MODULEID" 2>&AS_MESSAGE_LOG_FD` || mhd_cv_VAR="unknown"])
dnl AS_IF test -n "${PKG_CONFIG}
test -n "${mhd_cv_VAR}" || mhd_cv_VAR="unknown"
])
dnl AC_CACHE_CHECK
AS_VAR_IF([mhd_cv_VAR],["unknown"],
[$4],[mhd_out_VAR="$mhd_cv_VAR"
m4_n([$3])dnl
])
dnl AS_VAR_IF mhd_cv_VAR "unknown"
AS_VAR_POPDEF([mhd_cv_VAR])dnl
AS_VAR_POPDEF([mhd_out_VAR])dnl
m4_popdef([mhd_MODULEID])dnl
m4_newline([@%:@[ Expansion of $0 macro ends here]])
])dnl AC_DEFUN([MHD_PKG_CONF_MODULE_VERSION
