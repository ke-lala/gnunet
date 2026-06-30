# SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0)
#
# This file is part of GNU libmicrohttpd.
# (C) 2014-2026 Evgeny Grin (Karlson2k)
# (C) 2006-2021 Christian Grothoff (and other contributing authors)
#
# GNU libmicrohttpd is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# GNU libmicrohttpd is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# Alternatively, you can redistribute GNU libmicrohttpd and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version, together
# with the eCos exception, as follows:
#
#   As a special exception, if other files instantiate templates or
#   use macros or inline functions from this file, or you compile this
#   file and link it with other works to produce a work based on this
#   file, this file does not by itself cause the resulting work to be
#   covered by the GNU General Public License. However the source code
#   for this file must still be made available in accordance with
#   section (3) of the GNU General Public License v2.
#
#   This exception does not invalidate any other reasons why a work
#   based on this file might be covered by the GNU General Public
#   License.
#
# You should have received copies of the GNU Lesser General Public
# License and the GNU General Public License along with this library;
# if not, see <https://www.gnu.org/licenses/>.
#
dnl 
dnl Process this file with autoconf to produce a configure script.
dnl

AC_PREREQ([2.64])
LT_PREREQ([2.4.0])
AC_INIT([GNU libmicrohttpd2],[1.99.0],[libmicrohttpd@gnu.org])
AC_CONFIG_AUX_DIR([build-aux])
MHD_AUX_DIR='build-aux' # Must be set to the same value as in the previous line
AC_CONFIG_HEADERS([src/incl_priv/config/mhd_config.h])
AC_CONFIG_MACRO_DIR([m4])
m4_pattern_forbid([^_?MHD_[A-Z_]+_CC_])dnl

LIB_VER_CURRENT=0
LIB_VER_REVISION=0
LIB_VER_AGE=0
AC_SUBST([LIB_VER_CURRENT])
AC_SUBST([LIB_VER_REVISION])
AC_SUBST([LIB_VER_AGE])


PACKAGE_VERSION_MAJOR='m4_car(m4_unquote(m4_split(AC_PACKAGE_VERSION, [\.])))'
PACKAGE_VERSION_MINOR='m4_argn(2, m4_unquote(m4_split(AC_PACKAGE_VERSION, [\.])))'
PACKAGE_VERSION_SUBMINOR='m4_argn(3, m4_unquote(m4_split(AC_PACKAGE_VERSION, [\.])))'
AS_VAR_ARITH([MHD_W32_DLL_SUFF],[[$LIB_VER_CURRENT - $LIB_VER_AGE]])
AC_SUBST([PACKAGE_VERSION_MAJOR])
AC_SUBST([PACKAGE_VERSION_MINOR])
AC_SUBST([PACKAGE_VERSION_SUBMINOR])
AC_SUBST([MHD_W32_DLL_SUFF])

CONF_FINAL_WARNS=""

MHD_LIB_CPPFLAGS=""
MHD_LIB_CFLAGS=""
MHD_LIB_LDFLAGS=""
MHD_LIBDEPS=""
# for pkg-config
MHD_REQ_PRIVATE=''
MHD_LIBDEPS_PKGCFG=''

AH_TOP([#ifndef MHD_SYS_OPTIONS_H
#error "mhd_config.h" must not be used directly
#error include "mhd_sys_options.h" wrapper instead
#endif /* ! MHD_SYS_OPTIONS_H */])

AS_IF([test -z "$CC" && test -z "$CPP"], [
  AC_MSG_CHECKING([[whether z/OS special settings are required]])
  AS_IF([test `uname -s` = "OS/390"],
    [
      # configure binaries for z/OS
      AS_IF([test -z "$CC"],
            [CC=`pwd`"/contrib/xcc"
             chmod +x $CC || true])
      AS_IF([test -z "$CPP"],
            CPP="c89 -E")
      AC_MSG_RESULT([[yes]])
    ],
    [AC_MSG_RESULT([[no]])]
  )
])


AC_MSG_CHECKING([for build type])
AC_ARG_ENABLE([build-type],
  [AS_HELP_STRING([[--enable-build-type=TYPE]],
  [enable build TYPE, a set of configuration parameters; individual settings ]dnl
  [(asserts, sanitizers, compiler and linker flags) can be overridden by ]dnl
  [additional configure parameters (debug, debugger, trace, neutral, release, ]dnl
  [release-compact, release-hardened) [neutral]])],
  [], [enable_build_type=neutral])
AS_IF([test "x${enable_build_type}" = "x"], [enable_build_type="neutral"])
AS_VAR_IF([enable_build_type], ["no"], [enable_build_type="neutral"])
AS_VAR_IF([enable_build_type], ["yes"], [AC_MSG_ERROR([[Missing TYPE for --enable-build-type=]])])
AS_CASE([${enable_build_type}],
  [debug], [AC_MSG_RESULT([debug. Defaults: enable asserts, sanitizers (if any supported), debug information, compiler optimisation for debugging])],
  [trace], [AC_MSG_RESULT([trace. Defaults: enable asserts, debug prints, debug information, compiler optimisation for debugging])],
  [debugger], [AC_MSG_RESULT([debugger. Defaults: enable asserts, debug information, no compiler optimisation, static lib])],
  [neutral], [AC_MSG_RESULT([neutral. Defaults: use only user-specified compiler and linker flags])],
  [release], [AC_MSG_RESULT([release. Defaults: disable asserts, enable compiler optimisations, LTO])],
  [release-compact], [AC_MSG_RESULT([release-compact. Defaults: disable asserts, enable compiler optimisations for size, enable compact code, LTO])],
  [release-hardened], [AC_MSG_RESULT([release-hardened. Defaults: disable asserts, enable compiler optimisations, enable linker and compiler hardening])],
  [AC_MSG_ERROR([[Unknown build type: ${enable_build_type}]])]
)
AS_VAR_IF([enable_build_type], ["neutral"], [:],
  [
    # For all non-neutral build types do not use automatic "-g -O2" for CFLAGS
    AS_IF([test -z "${CFLAGS}"], [CFLAGS=""])
  ]
)
AS_VAR_IF([enable_build_type], ["debugger"],
  [ # Build only static version unless something else is specified by the user
    AS_IF([test -z "${enable_static}" || test "x${enable_static}" = "xyes"],
      [
        AS_IF([test -z "${enable_shared}"],
          [
            enable_shared="no"
            enable_static="yes"
          ]
        )
      ],
      [
        AS_CASE([${enable_static}],[*libmicrohttpd*],
          [AS_IF([test -z "${enable_shared}"], [enable_shared="no"])],
        )
      ]
    )
  ]
)
AS_CASE([${enable_build_type}],[debug|debugger|trace],
  [ AS_IF([test -z "${enable_silent_rules}"], [ enable_silent_rules="yes" ])]
)

AC_ARG_ENABLE([lto],[AS_HELP_STRING([[--enable-lto[=ARG]]],[add Link Time Optimisation compiler flags (yes, no, auto) [auto]])],
  [
    AS_CASE([${enableval}],
      [yes|no|auto],[],
      [AC_MSG_ERROR([invalid parameter --enable-lto=${enableval}])]
    )
  ],[enable_lto=auto]
)

AS_VAR_IF([enable_lto],["auto"],
  [AS_CASE([${enable_build_type}],[release|release-compact],[],[enable_lto=no])]
)

AM_INIT_AUTOMAKE([gnu] [check-news] [filename-length-max=99] [tar-v7] [silent-rules] [subdir-objects])

AC_CANONICAL_HOST
# Checks for programs.
AC_PROG_GREP
AC_PROG_EGREP
AC_PROG_FGREP
AC_PROG_SED
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
m4_version_prereq([2.70],
  [dnl Autoconf 2.70 or later
# Find C compiler and compiler options to support C11 (C23 with autoconf >=2.73).
# Fallback to earlier C versions if the latest checked version is not supported.
AC_PROG_CC
  ],
  [dnl Autoconf before 2.70
# Find C compiler and compiler options to support C99.
# Fallback to C89 if later C versions are not supported.
AC_PROG_CC_STDC
  ]
)

user_CFLAGS="$CFLAGS"
user_LDFLAGS="$LDFLAGS"
user_CPPFLAGS="$CPPFLAGS"

CFLAGS_ac=""
LDFLAGS_ac=""
CPPFLAGS_ac=""

CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
LDFLAGS="${LDFLAGS_ac} ${user_LDFLAGS}"
AC_CACHE_CHECK([whether to look f][or compiler-specific binary tools],
  [mhd_cv_check_cc_special_bin_tools],
  [
    AS_IF([test "X$enable_lto" = "Xno"],
      [mhd_cv_check_cc_special_bin_tools="no (LTO handling not needed)"],
      [test "X${GCC}" != "Xyes"],
      [mhd_cv_check_cc_special_bin_tools="no (compiler is not GCC nor clang)"],
      [AS_VAR_TEST_SET([NM]) && AS_VAR_TEST_SET([OBJDUMP]) && \
       AS_VAR_TEST_SET([DLLTOOL]) && AS_VAR_TEST_SET([RANLIB]) && \
       AS_VAR_TEST_SET([AR]) && AS_VAR_TEST_SET([STRIP]) && \
       AS_VAR_TEST_SET([RC])]
      [mhd_cv_check_cc_special_bin_tools="no (all binary tools are predefined)"],
      [
        AS_CASE([$CC],
          [*gcc*],[mhd_cv_check_cc_special_bin_tools="yes"],
          [*clang*],[mhd_cv_check_cc_special_bin_tools="yes"],
          [mhd_cv_check_cc_special_bin_tools="no (compiler binary name is not suitable)"]
        )
      ]
    )
  ]
)
AS_VAR_IF([mhd_cv_check_cc_special_bin_tools],["yes"],
  [
    MHD_CHECK_CC_IS_CLANG
    mhd_CC_bin_name=`MHD_PRINT_FIRST_WORD([$CC])`
    mhd_bin_tools_prfx=""
    mhd_bin_tools_sufx=""
    AS_CASE([${mhd_CC_bin_name}],
      [[gcc|*[\\/]gcc|*-gcc|clang|*[\\/]clang|*-clang]],
      [mhd_bin_tools_prfx="${mhd_CC_bin_name}"],
      [[gcc-[0-9]|*[\\/]gcc-[0-9]|*-gcc-[0-9]|clang-[0-9]|*[\\/]clang-[0-9]|*-clang-[0-9]]],
      [[
        mhd_bin_tools_prfx=`$as_expr "X${mhd_CC_bin_name}" : 'X\(.*\)-[0-9]'`
        mhd_bin_tools_sufx=`$as_expr "X${mhd_CC_bin_name}" : 'X.*\(-[0-9]\)'`
        test -n "${mhd_bin_tools_sufx}" || mhd_bin_tools_prfx=""
      ]],
      [[gcc-[0-9][0-9]|*[\\/]gcc-[0-9][0-9]|*-gcc-[0-9][0-9]|clang-[0-9][0-9]|*[\\/]clang-[0-9][0-9]|*-clang-[0-9][0-9]]],
      [[
        mhd_bin_tools_prfx=`$as_expr "X${mhd_CC_bin_name}" : 'X\(.*\)-[0-9][0-9]'`
        mhd_bin_tools_sufx=`$as_expr "X${mhd_CC_bin_name}" : 'X.*\(-[0-9][0-9]\)'`
        test -n "${mhd_bin_tools_sufx}" || mhd_bin_tools_prfx=""
      ]],
      [[gcc[0-9]|*[\\/]gcc[0-9]|*-gcc[0-9]|clang[0-9]|*[\\/]clang[0-9]|*-clang[0-9]]],
      [[
        mhd_bin_tools_prfx=`$as_expr "X${mhd_CC_bin_name}" : 'X\(.*\)[0-9]'`
        mhd_bin_tools_sufx=`$as_expr "X${mhd_CC_bin_name}" : 'X.*\([0-9]\)'`
        test -n "${mhd_bin_tools_sufx}" || mhd_bin_tools_prfx=""
      ]],
      [[gcc[0-9][0-9]|*[\\/]gcc[0-9][0-9]|*-gcc[0-9][0-9]|clang[0-9][0-9]|*[\\/]clang[0-9][0-9]|*-clang[0-9][0-9]]],
      [[
        mhd_bin_tools_prfx=`$as_expr "X${mhd_CC_bin_name}" : 'X\(.*\)[0-9][0-9]'`
        mhd_bin_tools_sufx=`$as_expr "X${mhd_CC_bin_name}" : 'X.*\([0-9][0-9]\)'`
        test -n "${mhd_bin_tools_sufx}" || mhd_bin_tools_prfx=""
      ]]
    )
    AS_CASE([${mhd_bin_tools_prfx}],
      [*gcc],[AS_VAR_IF([mhd_cv_cc_clang_based],["no"],[:],[mhd_bin_tools_prfx=""])],
      [*clang],
      [
        AS_VAR_IF([mhd_cv_cc_clang_based],["yes"],
          [
            mhd_bin_tools_prfx=`$as_expr "X${mhd_bin_tools_prfx}" : 'X\(.*\)clang'`
            mhd_bin_tools_prfx="${mhd_bin_tools_prfx}llvm"
          ],
          [mhd_bin_tools_prfx=""]
        )
      ]
    )
    AS_CASE([${mhd_bin_tools_prfx}],
      [[/*|[A-Za-z]:[\\/]*]],
      [
        # Simplified detection of absolute path
        MHD_CHECK_PROG_ABS([NM],["${mhd_bin_tools_prfx}-nm${mhd_bin_tools_sufx}"])
        MHD_CHECK_PROG_ABS([OBJDUMP],["${mhd_bin_tools_prfx}-objdump${mhd_bin_tools_sufx}"])
        MHD_CHECK_PROG_ABS([DLLTOOL],["${mhd_bin_tools_prfx}-dlltool${mhd_bin_tools_sufx}"])
        MHD_CHECK_PROG_ABS([RANLIB],["${mhd_bin_tools_prfx}-ranlib${mhd_bin_tools_sufx}"])
        MHD_CHECK_PROG_ABS([AR],["${mhd_bin_tools_prfx}-ar${mhd_bin_tools_sufx}"])
        MHD_CHECK_PROG_ABS([STRIP],["${mhd_bin_tools_prfx}-strip${mhd_bin_tools_sufx}"])
        MHD_CHECK_PROG_ABS([RC],["${mhd_bin_tools_prfx}-windres${mhd_bin_tools_sufx}"])
      ],
      [*/*],
      [
         # Relative paths are ambiguous, do not handle them
        mhd_bin_tools_prfx=""
      ],
      [
        MHD_CHECK_PROG_PATH([NM],["${mhd_bin_tools_prfx}-nm${mhd_bin_tools_sufx}"])
        MHD_CHECK_PROG_PATH([OBJDUMP],["${mhd_bin_tools_prfx}-objdump${mhd_bin_tools_sufx}"])
        MHD_CHECK_PROG_PATH([DLLTOOL],["${mhd_bin_tools_prfx}-dlltool${mhd_bin_tools_sufx}"])
        MHD_CHECK_PROG_PATH([RANLIB],["${mhd_bin_tools_prfx}-ranlib${mhd_bin_tools_sufx}"])
        MHD_CHECK_PROG_PATH([AR],["${mhd_bin_tools_prfx}-ar${mhd_bin_tools_sufx}"])
        MHD_CHECK_PROG_PATH([STRIP],["${mhd_bin_tools_prfx}-strip${mhd_bin_tools_sufx}"])
        MHD_CHECK_PROG_PATH([RC],["${mhd_bin_tools_prfx}-windres${mhd_bin_tools_sufx}"])
      ]
    )
  ]
)


MHD_SYS_EXT([CPPFLAGS_ac])
CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"

m4_version_prereq([2.72],
  [
AC_SYS_YEAR2038
  ],
  [
AC_SYS_LARGEFILE
  ]
)
AC_FUNC_FSEEKO

AS_VAR_IF([enable_lto],["no"],[use_lto="no"],
  [
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
    LDFLAGS="${LDFLAGS_ac} ${user_LDFLAGS}"
    lto_CFLAGS=""
    lto_LDFLAGS=""
    use_lto=""
    MHD_CHECK_CC_IS_CLANG([MHD_CHECK_ADD_CC_LDFLAG([-fuse-ld=lld],[lto_LDFLAGS])])
    LDFLAGS="${lto_LDFLAGS} ${LDFLAGS_ac} ${user_LDFLAGS}"
    MHD_FIND_ADD_CC_CFLAG_IFELSE(
      [
        use_lto="yes"
        MHD_PREPEND_FLAG_TO_VAR([lto_LDFLAGS],[$mhd_cc_found_flag])
      ],[use_lto="no"],[lto_CFLAGS],
      [-flto=auto],[-flto=full],[-flto]
    )
    LDFLAGS="${lto_LDFLAGS} ${LDFLAGS_ac} ${user_LDFLAGS}"
    AS_VAR_IF([use_lto],["yes"],
      [
        MHD_CHECK_ADD_CC_CFLAG([-ffat-lto-objects],[lto_CFLAGS],
          [],
          [
            test "X${enable_static}" = "Xyes" && test "X${enable_lto}" = "Xauto" && use_lto="no"
          ]
        )
      ]
    )
    AS_VAR_IF([use_lto],["yes"],
      [MHD_FIND_ADD_CC_CFLAG([lto_CFLAGS],[-flto-partition=one],[-flto-partition=none])]
    )
    AS_VAR_IF([use_lto],["yes"],
      [
        MHD_PREPEND_FLAG_TO_VAR([CFLAGS_ac],[${lto_CFLAGS}])
        MHD_PREPEND_FLAG_TO_VAR([LDFLAGS_ac],[${lto_LDFLAGS}])
      ],
      [
        AS_VAR_IF([enable_lto],["yes"],
          [AC_MSG_ERROR([LTO cannot be enabled, but requested by --enable-lto])]
        )
      ]
    )
    LDFLAGS="${LDFLAGS_ac} ${user_LDFLAGS}"
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
  ]
)

# Workarouds for mis-detection of linker when '-fuse-ld=XXX' flag is used
mhd_saved_CC="$CC"
AS_IF([AS_VAR_TEST_SET([LD]) || test "X${GCC}" != "Xyes"],[],
  [
     # LD is not set by the user and CC is GCC or clang
     MHD_CHECK_CC_IS_CLANG(
       [ # Detect linker used by clang
         # Note: clang does not change output of `clang -print-prog-name=ld` if '-fuse-ld=XXX' is used
         AC_MSG_CHECKING([fo][r the linker used by $CC $CFLAGS $LDFLAGS])
         : > conftest.$ac_ext # Create dummy input
         CC="$CC -@%:@@%:@@%:@" # converted to "$CC -###"
         eval mhd_log_echo='"$as_me:'"${LINENO}: $ac_link"'"'
         AS_ECHO(["${mhd_log_echo}"]) >&AS_MESSAGE_LOG_FD
         eval 'mhd_cc_link_out=`( '"$ac_link"'; ) AS_MESSAGE_LOG_FD>&1 2>&1`' 2>&AS_MESSAGE_LOG_FD
         mhd_cmd_status=$?
         AS_ECHO(["${mhd_cc_link_out}"]) >&AS_MESSAGE_LOG_FD
         AS_ECHO(["$as_me:${LINENO}: \$? = ${mhd_cmd_status}"]) >&AS_MESSAGE_LOG_FD
         rm -f core conftest.$ac_objext conftest.beam conftest$ac_exeext conftest.$ac_ext
         CC="${mhd_saved_CC}"
         mhd_cc_linker_cmd=`AS_ECHO_N(["${mhd_cc_link_out}"]) | ${SED-sed} -n ['s/^ *"\([^"]*\)".*/\1/p'] | ${SED-sed} -n '$p'`
         AS_IF([test -z "${mhd_cc_linker_cmd}"],
           [AC_MSG_RESULT([not detected])],
           [
             LD="${mhd_cc_linker_cmd}"
             AC_MSG_RESULT([${LD}])
           ]
         )
         AS_UNSET([mhd_cc_linker_cmd])
         AS_UNSET([mhd_cmd_status])
         AS_UNSET([mhd_cc_link_out])
         AS_UNSET([mhd_log_echo])
       ],
       [ # Let LT_PATH_LD to detect linker, but force it to use all flags when detecting
         CC="$CC $CFLAGS $CPPFLAGS $LDFLAGS"
       ]
     )
  ]
)
LT_PATH_LD
CC="${mhd_saved_CC}"

LT_INIT([win32-dll])
LT_LANG([Windows Resource])

AC_LANG_CONFTEST([AC_LANG_SOURCE([[
]AC_INCLUDES_DEFAULT[
#include <stdio.h>

int main (int argc, char *argv[])
{
  if (1 <= argc)
    return (0 < printf ("Self-name: %s\n", argv[0])) ? 0 : 1;
  return 2;
}
      ]]
    )
  ]
)
AC_CACHE_CHECK([whether $CC compiles simple C code without warnings],
  [mhd_cv_cc_warnless_c_noflags],
  [
    ac_c_werror_flag="yes"
    AC_LINK_IFELSE([],[mhd_cv_cc_warnless_c_noflags="yes"],
        [mhd_cv_cc_warnless_c_noflags="no"])
    ac_c_werror_flag=""
  ]
)
AS_VAR_IF([mhd_cv_cc_warnless_c_noflags],["yes"],
  [mhd_cv_cc_warnless_c_flags=""],
  [
    AC_CACHE_CHECK([for $CC flags to mute warnings for simple C code],
      [mhd_cv_cc_warnless_c_flags],
      [
        mhd_cv_cc_warnless_c_flags="no"
        ac_c_werror_flag="yes"
        for flag_chk in '-Wno-deprecated' '-fpermissive'
        do
          CFLAGS="${CFLAGS_ac} ${flag_chk} ${user_CFLAGS}"
          AC_LINK_IFELSE([],[mhd_cv_cc_warnless_c_flags="${flag_chk}"])
          CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
          test "X${mhd_cv_cc_warnless_c_flags}" = "Xno" || break
        done
        AS_UNSET([flag_chk])
        ac_c_werror_flag=""
      ]
    )
    AS_VAR_IF([mhd_cv_cc_warnless_c_flags],["no"],[:],
      [MHD_PREPEND_FLAG_TO_VAR([user_CFLAGS],[${mhd_cv_cc_warnless_c_flags}])]
    )
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
  ]
)
rm -f conftest.$ac_ext

AC_ARG_ENABLE([compact-code],
  [AS_HELP_STRING([[--enable-compact-code]],
  [enable use of a reduced size version of the code, resulting in smaller ]
  [binaries with a slight performance hit [auto]])],
  [], [enable_compact_code=auto])
AS_IF([test "x${enable_compact_code}" = "x"], [enable_compact_code="auto"])
AH_TEMPLATE([[MHD_FAVOR_SMALL_CODE]], [Define to '1' to use compact code version])
AH_TEMPLATE([[MHD_FAVOR_FAST_CODE]], [Define to '1' to use fast (and larger) code version])
AS_UNSET([compact_code_MSG])
AS_CASE([${enable_compact_code}], [auto],
  [
    # Parameter not set.
    # Check preprocessor macros
    AC_CHECK_DECL([MHD_FAVOR_SMALL_CODE],
      [
        enable_compact_code="yes"
        compact_code_MSG="enabled by preprocessor macro"
      ],
      [],[/* no includes */]
    )
    AC_CHECK_DECL([MHD_FAVOR_FAST_CODE],
      [
        AS_VAR_IF([enable_compact_code],["yes"],
          [AC_MSG_ERROR([Both MHD_FAVOR_SMALL_CODE and MHD_FAVOR_FAST_CODE macros are defined])]
        )
        enable_compact_code="no"
        compact_code_MSG="set by preprocessor macro"
      ],[],[/* no includes */]
    )

    AS_VAR_IF([enable_compact_code], ["auto"],
      [
        # No preference by preprocessor macros
        AC_CACHE_CHECK([whether compiler is configured to optimize for size],
          [mhd_cv_cc_optim_size],
          [
            AC_COMPILE_IFELSE(
              [
                AC_LANG_PROGRAM([[
#ifndef __OPTIMIZE_SIZE__
#error Looks like compiler does not optimize for size
fail test here %%%@<:@-1@:>@
#endif
              ]],[])
              ],
              [mhd_cv_cc_optim_size="yes"],[mhd_cv_cc_optim_size="no"]
            )
          ]
        )
        AS_VAR_IF([mhd_cv_cc_optim_size], ["yes"],
          [
            enable_compact_code="yes"
            compact_code_MSG="enabled automatically as compiler optimizes for size"
            AC_DEFINE([MHD_FAVOR_SMALL_CODE],[1])
          ]
        )
      ]
    )

    AS_VAR_IF([enable_compact_code], ["auto"],
      [
        # No preference by preprocessor macros and compiler flags
        AS_CASE([${enable_build_type}],[*-compact],
          [
            enable_compact_code="yes"
            compact_code_MSG="enabled by --enable-build-type=${enable_build_type}"
            AC_DEFINE([MHD_FAVOR_SMALL_CODE],[1])
          ]
        )
      ]
    )

    AS_VAR_IF([enable_compact_code], ["auto"],
      [
        # No preference
        enable_compact_code="no"
        compact_code_MSG="by default"
        AC_DEFINE([MHD_FAVOR_FAST_CODE],[1])
      ]
    )
  ],
  [yes],
  [
    compact_code_MSG="enabled by configure parameter"
    AC_CHECK_DECL([MHD_FAVOR_SMALL_CODE],
      [],
      [AC_DEFINE([MHD_FAVOR_SMALL_CODE],[1])],[/* no includes */]
    )
    AC_CHECK_DECL([MHD_FAVOR_FAST_CODE],
      [AC_MSG_ERROR([MHD_FAVOR_FAST_CODE macro is defined, --enable-compact-code could not be used])
      ],
      [],[/* no includes */]
    )
  ],
  [no],
  [
    compact_code_MSG="disabled by configure parameter"
    AC_CHECK_DECL([MHD_FAVOR_FAST_CODE],
      [],
      [AC_DEFINE([MHD_FAVOR_FAST_CODE],[1])],[/* no includes */]
    )
    AC_CHECK_DECL([MHD_FAVOR_SMALL_CODE],
      [AC_MSG_ERROR([MHD_FAVOR_SMALL_CODE macro is defined, --disable-compact-code could not be used])
      ],
      [],[/* no includes */]
    )
  ],
  [AC_MSG_ERROR([[Unknown parameter value: --enable-compact-code=${enable_compact_code}]])]
)

AC_MSG_CHECKING([whether to use a reduced size version of the code])
AC_MSG_RESULT([${enable_compact_code} (${compact_code_MSG})])


CFLAGS="${user_CFLAGS}"
# Compiler options to always enable (if supported)
MHD_FIND_ADD_CC_CFLAG([CFLAGS_ac], [-fno-strict-aliasing], [-qnoansialias])
# '-qlonglong' is XLC option for C89, not used with C99 or later
MHD_CHECK_ADD_CC_CFLAG([-qlonglong], [CFLAGS_ac])

# Set basic optimisation flags
MHD_CHECK_CC_IS_CLANG
AS_CASE([${enable_build_type}],[debug|debugger|trace],
  [ # Debug build, build for walking with debugger or debug printing build
    CFLAGS="${user_CFLAGS}"
    AS_VAR_IF([enable_build_type],["debug"],
      [
        # Clang has ASAN (pointer compare) broken when '-Og' optimisations are used
        AS_IF([test "x${enable_sanitizers}" != "xno" && test "x${mhd_cv_cc_clang_based}" = "xyes"],
          [MHD_CHECK_ADD_CC_CFLAG([-O0], [CFLAGS_ac])],
          [MHD_FIND_ADD_CC_CFLAG([CFLAGS_ac], [-Og], [-O0])]
        )
      ],
      [MHD_CHECK_ADD_CC_CFLAG([-O0], [CFLAGS_ac])]
    )
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
  ]
)
AS_CASE([${enable_build_type}],[release|release-*],
  [ # All release types
    AS_VAR_IF([enable_build_type],["release-compact"],
      [
        CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
        AC_CHECK_DECL([MHD_FAVOR_SMALL_CODE],[],
          [AC_CHECK_DECL([MHD_FAVOR_FAST_CODE],[],
            [MHD_APPEND_FLAG_TO_VAR([CPPFLAGS_ac],[-DMHD_FAVOR_SMALL_CODE=1])],
            [/* no includes */])],[/* no includes */])
        CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
        CFLAGS="${user_CFLAGS}"
        MHD_FIND_ADD_CC_CFLAG_IFELSE(
          [AC_DEFINE([MHD_CC_OPTIMISE_FOR_SIZE],[1],[Define to 1 if compiler optimises binary for size])],
          [],
          [CFLAGS_ac], [-Oz], [-Os], [-xO3], [-O]
        )
        CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
      ],
      [ # All non-compact release types
        CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
        AC_CHECK_DECL([MHD_FAVOR_SMALL_CODE],[],
          [AC_CHECK_DECL([MHD_FAVOR_FAST_CODE],[],
            [MHD_APPEND_FLAG_TO_VAR([CPPFLAGS_ac],[-DMHD_FAVOR_FAST_CODE=1])],
            [/* no includes */])],[/* no includes */])
        CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
        CFLAGS="${user_CFLAGS}"
        MHD_FIND_ADD_CC_CFLAG_IFELSE(
          [AC_DEFINE([MHD_CC_OPTIMISE_FOR_SPEED],[1],[Define to 1 if compiler optimises binary for speed])],
          [],
          [CFLAGS_ac], [-O2], [-xO4], [-O]
        )
        CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
      ]
    )
  ]
)

AS_VAR_IF([enable_build_type],["neutral"],[],
  [ # Any non-neutral build types
    MHD_CHECK_ADD_CC_CFLAGS([-Wall -Wnull-dereference], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wdeclaration-after-statement -Wimplicit -Wnested-externs], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wredundant-decls -Wtrampolines -Wunsafe-loop-optimizations], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wpoison-system-directories], [CFLAGS_ac])
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
    LDFLAGS="${user_LDFLAGS}"
    MHD_CHECK_ADD_CC_LDFLAG([-Wl,--warn-common], [LDFLAGS_ac])
    LDFLAGS="${LDFLAGS_ac} ${user_LDFLAGS}"
  ]
)
AS_CASE([${enable_build_type}],[debug|debugger|trace],
  [ # Debug build, build for walking with debugger or debug printing build
    CFLAGS="${user_CFLAGS}"
    MHD_FIND_ADD_CC_CFLAG([CFLAGS_ac], [-ggdb3], [-g3], [-ggdb], [-g])
    MHD_CHECK_ADD_CC_CFLAG([-Wextra], [CFLAGS_ac])
    MHD_FIND_ADD_CC_CFLAG([CFLAGS_ac], [-Wpedantic], [-pedantic])
    save_find_CFLAGS_ac="$CFLAGS_ac"
    MHD_FIND_ADD_CC_CFLAG_IFELSE(
      [
        AC_CACHE_CHECK([whether format warnings work with v*printf() functions],
          [mhd_cv_wformat_works_vfprintf],
          [
            AS_VAR_IF([mhd_cv_cc_warnless_c_flags],["no"],[:],[ac_c_werror_flag="yes"])
            CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
            AC_COMPILE_IFELSE(
              [
                AC_LANG_SOURCE([[
#include <stdio.h>
#include <stdarg.h>

static int
my_vprint (const char *fm, va_list ap)
{
  return vfprintf (stderr, fm, ap);
}

static int
my_printf (const char *fm, ...)
{
  int res;
  va_list vargs;
  va_start (vargs, fm);
  res = my_vprint(fm, vargs);
  va_end (vargs);
  return res;
}

int main(void)
{
  const char *pnt_str = "printf string\n";
  return (0 < my_printf (pnt_str)) ? 0 : 5;
}
                  ]]
                )
              ],
              [mhd_cv_wformat_works_vfprintf="yes"],
              [mhd_cv_wformat_works_vfprintf="no"]
            )
            CFLAGS="${user_CFLAGS}"
            ac_c_werror_flag=""
          ]
        )
        AS_VAR_IF([mhd_cv_wformat_works_vfprintf],["no"],
          [
            # Check whether "enable" flag is supported as "disable" flag could be silently
            # accepted when the compiled code itself does not produce any warnings
            MHD_CHECK_CC_CFLAG([-Wformat-nonliteral],[CFLAGS_ac],
              [
                MHD_CHECK_ADD_CC_CFLAG([-Wno-format-nonliteral],[CFLAGS_ac],
                  [],[CFLAGS_ac="$save_find_CFLAGS_ac"]
                )
              ],[CFLAGS_ac="$save_find_CFLAGS_ac"]
            )
          ]
        )
      ],[],
      [CFLAGS_ac], [-Wformat=2], [-Wformat]
    )
    AS_UNSET([save_find_CFLAGS_ac])
    MHD_FIND_ADD_CC_CFLAG([CFLAGS_ac], [-Wimplicit-fallthrough=3], [-Wimplicit-fallthrough])
    MHD_CHECK_ADD_CC_CFLAGS([-Wformat-overflow -Wformat-truncation -Wformat-security -Wformat-signedness], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAG([-Wdouble-promotion], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wmissing-include-dirs -Wshift-overflow=2 -Wstringop-overflow=4 -Walloc-zero], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wduplicated-branches -Wduplicated-cond -Wfloat-equal -Wpointer-arith], [CFLAGS_ac])
    MHD_FIND_ADD_CC_CFLAG([CFLAGS_ac], [-Wshadow-all], [-Wshadow])
    MHD_CHECK_ADD_CC_CFLAGS([-Wbad-function-cast -Wcast-qual -Wwrite-strings -Wconversion], [CFLAGS_ac])
    MHD_FIND_ADD_CC_CFLAG([CFLAGS_ac], [-Wcast-align=strict], [-Wcast-align])
    MHD_CHECK_ADD_CC_CFLAGS([-Wjump-misses-init -Wlogical-op -Waggregate-return -Wstrict-prototypes], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wold-style-definition -Wmissing-declarations -Wmissing-prototypes], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wuninitialized -Winit-self -Wshift-negative-value -Wswitch-enum], [CFLAGS_ac])
    MHD_FIND_ADD_CC_CFLAG([CFLAGS_ac], [-Wstrict-overflow=4], [-Wstrict-overflow])
    MHD_FIND_ADD_CC_CFLAG([CFLAGS_ac], [-Wnormalized=nfkc], [-Wnormalized])
    MHD_CHECK_ADD_CC_CFLAGS([-Walloca -Wbidi-chars=any -Warray-bounds -Wpacked -Wvariadic-macros], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wundef], [CFLAGS_ac])
    MHD_FIND_ADD_CC_CFLAG([CFLAGS_ac], [-Wtrailing-whitespace=any], [-Wtrailing-whitespace])

    MHD_CHECK_ADD_CC_CFLAGS([-Wanon-enum-enum-conversion -Warray-bounds-pointer-arithmetic -Wassign-enum], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wbit-int-extension -Wbitfield-enum-conversion -Wparentheses -Wbool-operation], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wcast-function-type -Wcomma -Wcomment -Wcompound-token-split], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wconditional-uninitialized -Wdeprecated], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wdocumentation -Wdocumentation-pedantic], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wempty-init-stmt -Wenum-conversion -Wexpansion-to-defined], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wflexible-array-extensions -Wloop-analysis -Wfour-char-constants -Wgcc-compat], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wformat-pedantic -Wformat-non-iso -Wformat-type-confusion], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAG([-Wgnu], [CFLAGS_ac],
      [],
      [
        MHD_CHECK_ADD_CC_CFLAGS([-Wgnu-anonymous-struct -Wgnu-compound-literal-initializer], [CFLAGS_ac])
        MHD_CHECK_ADD_CC_CFLAGS([-Wgnu-conditional-omitted-operand -Wgnu-designator -Wgnu-empty-initializer], [CFLAGS_ac])
        MHD_CHECK_ADD_CC_CFLAGS([-Wgnu-empty-struct -Wgnu-flexible-array-initializer -Wgnu-folding-constant], [CFLAGS_ac])
        MHD_CHECK_ADD_CC_CFLAGS([-Wgnu-null-pointer-arithmetic -Wgnu-pointer-arith -Wgnu-redeclared-enum], [CFLAGS_ac])
        MHD_CHECK_ADD_CC_CFLAGS([-Wgnu-union-cast -Wgnu-variable-sized-type-not-at-end], [CFLAGS_ac])
      ]
    )
    MHD_CHECK_ADD_CC_CFLAGS([-Wmissing-noreturn -Wmissing-variable-declarations -Wnested-anon-types], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wnewline-eof -Wover-aligned -Wredundant-parens -Widiomatic-parentheses], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wshift-sign-overflow -Wtautological-compare -Wunaligned-access], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wunused -Wzero-as-null-pointer-constant -Wzero-length-array], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Waarch64-sme-attributes -Watomic-implicit-seq-cst -Warray-parameter], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wc++-compat -Wc23-compat -Wcalled-once-parameter -Wdecls-in-multiple-modules], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wduplicate-decl-specifier -Wduplicate-enum -Wembedded-directive], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wfunction-effects -Wignored-qualifiers -Wnonportable-system-include-path], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wincompatible-function-pointer-types-strict -Wmain -Wmain-return-type], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wnullable-to-nonnull-conversion -Woverlength-strings -Wpragmas], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wreserved-identifier -Wshift-bool -Wstatic-in-inline -Wswitch-default], [CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-Wthread-safety -Wunused-function], [CFLAGS_ac])
    MHD_CHECK_CC_CFLAG([-Wused-but-marked-unused],[CFLAGS_ac],
      [
        AC_CACHE_CHECK([whether $[]CC -Wused-but-marked-unused works with system headers],
          [mhd_cv_wused_but_marked_unused_sys_header],
          [
            AS_VAR_IF([mhd_cv_cc_warnless_c_flags],["no"],[:],[ac_c_werror_flag="yes"])
            CFLAGS="${CFLAGS_ac} -Wused-but-marked-unused ${user_CFLAGS}"
            AC_COMPILE_IFELSE(
              [
                AC_LANG_SOURCE([[
#include <stdio.h>

int main(void)
{
  char buf[16];
  return (int) snprintf(buf, 16, "test");
}
                  ]]
                )
              ],
              [mhd_cv_wused_but_marked_unused_sys_header="yes"],
              [mhd_cv_wused_but_marked_unused_sys_header="no"]
            )
            ac_c_werror_flag=""
          ]
        )
        AS_VAR_IF([mhd_cv_wused_but_marked_unused_sys_header],["yes"],
          [MHD_APPEND_FLAG_TO_VAR([CFLAGS_ac],[-Wused-but-marked-unused])]
        )
      ]
    )
    # Check whether "enable" flag is supported as "disable" flag could be silently
    # accepted when the compiled code itself does not produce any warnings
    MHD_CHECK_CC_CFLAG([-Wunreachable-code-break],[CFLAGS_ac],
      [MHD_CHECK_ADD_CC_CFLAG([-Wno-unreachable-code-break], [CFLAGS_ac])]
    )
    #
    # Removed flags:
    #
    # -Wdisabled-macro-expansion - warns about macros from system headers

    CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
    LDFLAGS="${user_LDFLAGS}"
    MHD_CHECK_ADD_CC_LDFLAG([-Wl,--enable-long-section-names], [LDFLAGS_ac])
    LDFLAGS="${LDFLAGS_ac} ${user_LDFLAGS}"
  ]
)
AS_CASE([${enable_build_type}],[release|release-*],
  [ # All release types
    CFLAGS="${user_CFLAGS}"
    MHD_CHECK_ADD_CC_CFLAGS([-floop-block -fgraphite-identity -floop-parallelize-all], [CFLAGS_ac])
    AS_VAR_IF([enable_build_type],["release-compact"],
      [],
      [ # All non-compact release types
        MHD_CHECK_ADD_CC_CFLAGS([-fsched-pressure -fira-loop-pressure -fmerge-all-constants], [CFLAGS_ac]) # These flags may improve size, recheck with LTO and linker garbage collection
        MHD_CHECK_ADD_CC_CFLAGS([-ftree-partial-pre -fgcse-after-reload -fipa-pta], [CFLAGS_ac])
        MHD_CHECK_ADD_CC_CFLAGS([-fisolate-erroneous-paths-attribute -ffinite-loops -floop-nest-optimize], [CFLAGS_ac])
        MHD_CHECK_ADD_CC_CFLAGS([-fpredictive-commoning -frename-registers], [CFLAGS_ac])
        MHD_CHECK_ADD_CC_CFLAGS([-ftree-loop-distribute-patterns -fpeel-loops -fsplit-loops -ftree-vectorize], [CFLAGS_ac])
      ]
    )

    AS_VAR_IF([enable_build_type],["release-hardened"],
      [
        MHD_CHECK_ADD_CC_CFLAGS([-Wformat-security -Wstack-protector], [CFLAGS_ac])
        MHD_CHECK_ADD_CC_CFLAGS([-Wuninitialized -Winit-self -Walloc-zero -Wbidi-chars=any], [CFLAGS_ac])
      ],
      [
        MHD_FIND_ADD_CC_CFLAG([CFLAGS_ac], [-fno-plt], [-mno-plt], [-Gy])
        MHD_CHECK_ADD_CC_CFLAGS([-fno-semantic-interposition], [CFLAGS_ac])
      ]
    )
    AS_VAR_IF([enable_build_type],["release"],
      [ # Flags are not suitable for 'compact' and for 'hardened'
        MHD_CHECK_ADD_CC_CFLAGS([-ffast-math -fno-trapping-math], [CFLAGS_ac])
      ]
    )
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
    LDFLAGS="${user_LDFLAGS}"
    MHD_FIND_ADD_CC_LDFLAG([LDFLAGS_ac], [-Wl,-O2], [-Wl,-O1])
    MHD_CHECK_ADD_CC_LDFLAGS([-Wl,--as-needed], [LDFLAGS_ac])
    AS_VAR_IF([enable_build_type],["release-hardened"],
      [:],
      [MHD_CHECK_ADD_CC_LDFLAGS([-Wl,--gc-sections -Wl,--icf=all], [LDFLAGS_ac])]
    )
    # W32-specific
    MHD_CHECK_ADD_CC_LDFLAG([-Wl,--disable-long-section-names], [LDFLAGS_ac])
    LDFLAGS="${LDFLAGS_ac} ${user_LDFLAGS}"
  ]
)
CFLAGS="${user_CFLAGS}"
MHD_CHECK_CC_CFLAG([-Wused-but-marked-unused],[CFLAGS_ac],
  [
    AC_DEFINE([[HAVE_WUSED_BUT_MARKED_UNUSED]],[[1]],[Define to '1' i][f -Wused-but-marked-unused flag is supported by compiler])
  ]
)
MHD_CHECK_CC_CFLAG([-Wzero-as-null-pointer-constant],[CFLAGS_ac],
  [
    AC_DEFINE([[HAVE_WZERO_AS_NULL_POINTER_CONSTANT]],[[1]],[Define to '1' i][f -Wzero-as-null-pointer-constant flag is supported by compiler])
  ]
)
MHD_CHECK_CC_CFLAG([-Wredundant-decls],[CFLAGS_ac],
  [
    AC_DEFINE([[HAVE_WREDUNDANT_DECLS]],[[1]],[Define to '1' i][f -Wredundant-decls flag is supported by compiler])
  ]
)
MHD_CHECK_CC_CFLAG([-Wswitch-enum],[CFLAGS_ac],
  [
    AC_DEFINE([[HAVE_WSWITCH_ENUM]],[[1]],[Define to '1' i][f -Wswitch-enum flag is supported by compiler])
  ]
)
MHD_CHECK_CC_CFLAG([-Wdocumentation-deprecated-sync],[CFLAGS_ac],
  [
    AC_DEFINE([[HAVE_WDOCUMENTATION_DEPRECATED_SYNC]],[[1]],[Define to '1' i][f -Wdocumentation-deprecated-sync flag is supported by compiler])
  ]
)
MHD_CHECK_CC_CFLAG([-Wdocumentation-unknown-command],[CFLAGS_ac],
  [
    AC_DEFINE([[HAVE_WDOCUMENTATION_UNKNOWN_COMMAND]],[[1]],[Define to '1' i][f -Wdocumentation-unknown-command flag is supported by compiler])
  ]
)
MHD_CHECK_CC_CFLAG([-Wdocumentation-pedantic],[CFLAGS_ac],
  [
    AC_DEFINE([[HAVE_WDOCUMENTATION_PEDANTIC]],[[1]],[Define to '1' i][f -Wdocumentation-pedantic flag is supported by compiler])
  ]
)
AS_UNSET([nocxx_compat_CFLAGS])
MHD_CHECK_CC_CFLAG([-Wc++-compat],[CFLAGS_ac],
  [
    AC_DEFINE([[HAVE_WCXX_COMPAT]],[[1]],[Define to '1' i][f -Wc++-compat flag is supported by compiler])
    MHD_CHECK_CC_CFLAG([-Wno-c++-compat],[CFLAGS_ac],
      [nocxx_compat_CFLAGS="-Wno-c++-compat"]
    )
  ]
)
CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
# Additional flags are checked and added at the end of 'configure'

# Check for headers that are ALWAYS required
AC_CHECK_HEADERS_ONCE([stdio.h string.h stdint.h stdarg.h errno.h limits.h fcntl.h], [],
  [AC_MSG_ERROR([Compiling libmicrohttpd requires standard C and POSIX headers files])], [AC_INCLUDES_DEFAULT])

# Check for basic optional headers
AC_CHECK_HEADERS([stddef.h stdlib.h inttypes.h sys/types.h sys/stat.h unistd.h \
                  sys/uio.h crtdefs.h malloc.h io.h stdbit.h], [], [], [AC_INCLUDES_DEFAULT])

# Check for clock-specific optional headers
AC_CHECK_HEADERS([sys/time.h time.h], [], [], [AC_INCLUDES_DEFAULT])

# Check for system information and parameters optional headers
AC_CHECK_HEADERS([endian.h machine/endian.h sys/endian.h sys/byteorder.h \
                  sys/machine.h machine/param.h sys/param.h sys/isa_defs.h \
                  sys/ioctl.h], [], [], [AC_INCLUDES_DEFAULT])

# Check for network and sockets optional headers
AC_CHECK_HEADERS([sys/socket.h sys/select.h netinet/in_systm.h netinet/in.h \
                  sys/un.h arpa/inet.h netinet/ip.h netinet/tcp.h net/if.h \
                  netdb.h sockLib.h inetLib.h selectLib.h afunix.h], [], [],
  [AC_INCLUDES_DEFAULT
   [
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif /* HAVE_INTTYPES_H */
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#elif defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif /* HAVE_NETINET_IN_SYSTM_H */
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif /* HAVE_NETINET_IP_H */
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif /* HAVE_NETINET_TCP_H */
  ]]
)

# Check for other optional headers
AC_CHECK_HEADERS([sys/msg.h sys/mman.h signal.h], [], [], [AC_INCLUDES_DEFAULT])

AC_CHECK_TYPES([size_t,ssize_t,ptrdiff_t,intptr_t,uintptr_t,uint8_t,time_t,suseconds_t],[],[],
  [[
/* Keep in sync with src/mhd2/sys_base_types.h */
#include <stdint.h> /* uint_fast_XXt, int_fast_XXt */
#if defined(HAVE_STDDEF_H)
#  include <stddef.h> /* size_t, NULL */
#elif defined(HAVE_STDLIB_H)
#  include <stdlib.h> /* should provide size_t, NULL */
#else
#  include <stdio.h> /* should provide size_t, NULL */
#endif
#if defined(HAVE_SYS_TYPES_H)
#  include <sys/types.h> /* ssize_t */
#elif defined(HAVE_UNISTD_H)
#  include <unistd.h> /* should provide ssize_t */
#endif
#ifdef HAVE_CRTDEFS_H
#  include <crtdefs.h> /* W32-specific header */
#endif
  ]]
)
AS_IF([test "x$ac_cv_type_size_t" != "xyes"],
  [AC_MSG_FAILURE(['size_t' type is not provided by system headers])]
)
AS_IF([test "x$ac_cv_type_uint8_t" != "xyes"],
  [AC_MSG_FAILURE(['uint8_t' type is not provided by system headers])]
)
AC_CHECK_SIZEOF([char])
AS_IF([test "x$ac_cv_sizeof_char" != "x1"],
  [AC_MSG_FAILURE(['char' type with size different from '1' is not supported])]
)


AC_CHECK_TYPE([socklen_t],
  [],
  [AC_DEFINE([socklen_t],[int],[Define to suitable 'socklen_t' replacement if 'socklen_t' is not defined by system headers])],
  [[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#include <stdint.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif /* HAVE_INTTYPES_H */
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif /* HAVE_NETINET_IN_SYSTM_H */
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_SOCKLIB_H
#include <sockLib.h>
#endif /* HAVE_SOCKLIB_H */
#if defined(_WIN32) && !defined(__CYGWIN__)
#include <winsock2.h>
#include <ws2tcpip.h>
#endif /* _WIN32 && ! __CYGWIN__ */
  ]]
)

AC_CHECK_HEADER([[search.h]],
  [
    MHD_CHECK_LINK_RUN([[for proper tsearch(), tfind() and tdelete()]],[[mhd_cv_sys_tsearch_usable]],
	  [
	    AS_CASE([$host_os],
	      [openbsd*],
	      [[ # Some OpenBSD versions have wrong return value for tdelete()
	        mhd_cv_sys_tsearch_usable='assuming no'
	      ]],
	      [netbsd*],
	      [[ # NetBSD had leaked root node for years
	        mhd_cv_sys_tsearch_usable='assuming no'
	      ]],
	      [[mhd_cv_sys_tsearch_usable='assuming yes']]
	    )
	  ],
	  [
	    AC_LANG_SOURCE(
	      [[
#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif /* HAVE_STDDEF_H */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include <stdio.h>
#include <search.h>

static int cmp_func(const void *p1, const void *p2)
{
  return (*((const int *)p1)) - (*((const int *)p2));
}

int main(void)
{
  int ret = 0;
  void *root_ptr = NULL;
  int element1 = 1;
  int **element_ptr_ptr1;
  int **element_ptr_ptr2;

  element_ptr_ptr1 =
    (int **) tsearch ((void*) &element1, &root_ptr, &cmp_func);
  if (NULL == element_ptr_ptr1)
  {
    fprintf (stderr, "NULL pointer has been returned when tsearch() called for the first time.\n");
    return ++ret;
  }
  if (*element_ptr_ptr1 != &element1)
  {
    fprintf (stderr, "Wrong pointer has been returned when tsearch() called for the first time.\n");
    return ++ret;
  }
  if (NULL == root_ptr)
  {
    fprintf (stderr, "Root pointer has not been set by tsearch().\n");
    return ++ret;
  }

  element_ptr_ptr2 =
    (int **) tsearch ((void*) &element1, &root_ptr, &cmp_func);
  if (NULL == element_ptr_ptr2)
  {
    fprintf (stderr, "NULL pointer has been returned when tsearch() called for the second time.\n");
    return ++ret;
  }
  if (*element_ptr_ptr2 != &element1)
  {
    fprintf (stderr, "Wrong pointer has been returned when tsearch() called for the second time.\n");
    ++ret;
  }
  if (element_ptr_ptr2 != element_ptr_ptr1)
  {
    fprintf (stderr, "Wrong element has been returned when tsearch() called for the second time.\n");
    ++ret;
  }

  element_ptr_ptr2 =
    (int **) tfind ((void*) &element1, &root_ptr, &cmp_func);
  if (NULL == element_ptr_ptr2)
  {
    fprintf (stderr, "NULL pointer has been returned by tfind().\n");
    ++ret;
  }
  if (*element_ptr_ptr2 != &element1)
  {
    fprintf (stderr, "Wrong pointer has been returned when by tfind().\n");
    ++ret;
  }
  if (element_ptr_ptr2 != element_ptr_ptr1)
  {
    fprintf (stderr, "Wrong element has been returned when tsearch() called for the second time.\n");
    ++ret;
  }

  element_ptr_ptr1 =
    (int **) tdelete ((void*) &element1, &root_ptr, &cmp_func);
  if (NULL == element_ptr_ptr1)
  {
    fprintf (stderr, "NULL pointer has been returned by tdelete().\n");
    ++ret;
  }
  if (NULL != root_ptr)
  {
    fprintf (stderr, "Root pointer has not been set to NULL by tdelete().\n");
    ++ret;
  }

  return ret;
}
	      ]]
	    )
	  ],
	  [AC_DEFINE([[MHD_USE_SYS_TSEARCH]], [[1]], [Define to 1 if you have properly working tsearch(), tfind() and tdelete() functions.])]
	)
  ],
  [], [AC_INCLUDES_DEFAULT]
)
AM_CONDITIONAL([MHD_USE_SYS_TSEARCH], [[test "x$mhd_cv_sys_tsearch_usable" = "xyes" || test "x$mhd_cv_sys_tsearch_usable" = "xassuming yes"]])

# Optional headers used for tests
AC_CHECK_HEADERS([sys/sysctl.h netinet/ip_icmp.h netinet/icmp_var.h], [], [],
  [[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif /* HAVE_SYS_SYSCTL_H */
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#elif defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif /* HAVE_NETINET_IN_SYSTM_H */
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif /* HAVE_NETINET_IP_H */
#ifdef HAVE_NETINET_IP_ICMP_H
#include <netinet/ip_icmp.h>
#endif /* HAVE_NETINET_IP_ICMP_H */
  ]]
)

AC_ARG_ENABLE([compiler-hardening],
  [AS_HELP_STRING([--enable-compiler-hardening], [enable compiler security checks])],
  [],
  [AS_CASE([${enable_build_type}],[*-hardened],
    [enable_compiler_hardening='yes'],[enable_compiler_hardening='no'])]
)
AS_VAR_IF([enable_compiler_hardening],["yes"],
  [
    CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
    AC_CHECK_DECL([_FORTIFY_SOURCE],
      [MHD_APPEND_FLAG_TO_VAR([CPPFLAGS_ac],[-U_FORTIFY_SOURCE])],
      [],[/* no includes */])
    MHD_APPEND_FLAG_TO_VAR([CPPFLAGS_ac],[-D_FORTIFY_SOURCE=2])
    CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
    CFLAGS="${user_CFLAGS}"
    MHD_FIND_ADD_CC_CFLAG([CFLAGS_ac],[-fstack-protector-strong],[-fstack-protector-all],[-fstack-protector])
    MHD_CHECK_ADD_CC_CFLAGS([-fstack-clash-protection],[CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-fcf-protection=full -mbranch-protection=standard],[CFLAGS_ac])
    MHD_CHECK_ADD_CC_CFLAGS([-fno-delete-null-pointer-checks -fno-strict-overflow ],[CFLAGS_ac])
    MHD_FIND_ADD_CC_CFLAG([CFLAGS_ac],[-ftrivial-auto-var-init=pattern],[-ftrivial-auto-var-init=zero])
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
    AS_IF([test "x${enable_static}" = "xyes" && test "x${pic_mode}" != "xyes"],
      [
        # PIE static lib can be used within non-PIE application, but
        # PIE static lib cannot be used in non-PIE shared lib. Let's assume
        # that static lib will not be used in shared lib
        # All "pie" flags will be used automatically by libtool only
        # for static library objects.
        CFLAGS="${user_CFLAGS}"
        # Perform tests here with "-pie" enabled
        LDFLAGS="${LDFLAGS_ac} -pie ${user_LDFLAGS}"
        MHD_CHECK_ADD_CC_CFLAG([-fPIE],[CFLAGS_ac],
          [
            MHD_APPEND_FLAG_TO_VAR([LDFLAGS_ac],[-pie])
          ],
          [
            MHD_CHECK_ADD_CC_CFLAG([-fpie],[CFLAGS_ac],
              [
                MHD_APPEND_FLAG_TO_VAR([LDFLAGS_ac],[-pie])
              ]
            )
          ]
        )
        CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
        LDFLAGS="${LDFLAGS_ac} ${user_LDFLAGS}"
      ]
    )
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
    LDFLAGS="${LDFLAGS_ac} ${user_LDFLAGS}"
  ]
)

# Linker hardening options
# Currently these options are ELF specific, they don't work on Darwin and W32
AC_ARG_ENABLE([linker-hardening],
  [AS_HELP_STRING([--enable-linker-hardening], [enable linker security fixups])],
  [],
  [AS_CASE([${enable_build_type}],[*-hardened],
    [enable_linker_hardening='yes'],[enable_linker_hardening='no'])]
)
AS_VAR_IF([enable_linker_hardening],["yes"],
  [
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
    LDFLAGS="${user_LDFLAGS}"
    MHD_CHECK_ADD_CC_LDFLAG([-Wl,-z,relro],[LDFLAGS_ac],
      [MHD_CHECK_ADD_CC_LDFLAG([-Wl,-z,now],[LDFLAGS_ac])])
    MHD_CHECK_ADD_CC_LDFLAG([-Wl,-z,separate-code],[LDFLAGS_ac])
    # Actually should be "noexec" by default, but let's try to enforce it.
    MHD_CHECK_ADD_CC_LDFLAG([-Wl,-z,noexecstack],[LDFLAGS_ac])
    # W32-specific. Some are enabled by default, but they will be enfored to be sure.
    MHD_CHECK_ADD_CC_LDFLAGS([-Wl,--large-address-aware -Wl,--enable-auto-image-base],[LDFLAGS_ac])
    MHD_CHECK_ADD_CC_LDFLAGS([-Wl,--dynamicbase -Wl,--nxcompat -Wl,--high-entropy-va],[LDFLAGS_ac])
    LDFLAGS="${LDFLAGS_ac} ${user_LDFLAGS}"
  ]
)

MHD_BOOL

AC_CACHE_CHECK([whether the NULL pointer has all zero bits],
  [mhd_cv_ptr_null_all_zeros],
  [
    AC_RUN_IFELSE(
      [
        AC_LANG_SOURCE([[
#include <string.h>
#if defined(HAVE_STDDEF_H)
#include <stddef.h>
#elif defined(HAVE_STDLIB_H)
#include <stdlib.h>
#else
#include <stdio.h>
#endif

int main(void)
{
  void *ptr1;
  void *ptr2;

  ptr1 = &ptr2;
  ptr2 = NULL;
  memset(&ptr1, 0, sizeof(ptr1));
  if (ptr2 != ptr1)
    return 2;

  ptr2 = &ptr1;
  ptr1 = NULL;
  memset(&ptr2, 0, sizeof(ptr2));
  if (0 != memcmp (&ptr1, &ptr2, sizeof(ptr1)))
    return 3;

  ptr1 = (void*)&ptr1;
  ptr2 = (void*)&ptr2;
  memset(&ptr1, 0, sizeof(ptr1));
  memset(&ptr2, 0, sizeof(ptr2));
  if (NULL != ptr1)
    return 4;
  if (NULL != ptr2)
    return 5;

  return 0;
}
          ]]
        )
      ],
      [mhd_cv_ptr_null_all_zeros="yes"],
      [mhd_cv_ptr_null_all_zeros="no"],
      [
        AS_CASE([$host_cpu],dnl
          [[i[234567]86|x86_64|amd64|arm|armeb|armv[0123456789]|armv[0123456789]eb|aarch64|aarch64_be|arm64|mips|mipsel|mips64|mips64el|powerpc|powerpcle|powerpc64|powerpc64le|riscv32|riscv32be|riscv64|riscv64be]],
          [
            AS_CASE([$host_os],dnl
              [[linux*|freebsd|cygwin|mingw*|msys|gnu|netbsd*|openbsd*|darwin*|solaris2*|haiku]],
              [mhd_cv_ptr_null_all_zeros="assuming yes"],
              [mhd_cv_ptr_null_all_zeros="assuming no"]
            )
          ],
          [mhd_cv_ptr_null_all_zeros="assuming no"]
        )
      ]
    )
  ]
)
AS_IF([test "x${mhd_cv_ptr_null_all_zeros}" = "xyes" || test "x${mhd_cv_ptr_null_all_zeros}" = "xassuming yes"],
  [AC_DEFINE([HAVE_NULL_PTR_ALL_ZEROS],[1],[Define to '1' if NULL pointers binary representation is all zero bits])]
)

AC_CACHE_CHECK([whether $CC supports variadic macros],[mhd_cv_cc_macro_variadic],
  [
    AS_CASE([$ac_prog_cc_stdc],
      [c89],[:],
      [c??],[mhd_cv_cc_macro_variadic="yes"]
    )
    AS_VAR_SET_IF([mhd_cv_cc_macro_variadic],[:],
      [
        AS_VAR_IF([mhd_cv_cc_warnless_c_flags],["no"],[:],[ac_c_werror_flag="yes"])
        AC_COMPILE_IFELSE(
          [
            AC_LANG_SOURCE([[
#define GET_THIRD(arg1,arg2,arg3,arg4) (arg3)
#define GET_THIRD_VARIADIC(...) GET_THIRD(__VA_ARGS__)

int main(void)
{
  int arr[GET_THIRD_VARIADIC(-1,-2,5,-3) - 4] = {0};
  if (0 != arr[GET_THIRD_VARIADIC(100,1000,0,10000)])
    return 2;
  return 0;
}
              ]]
            )
          ],
          [mhd_cv_cc_macro_variadic="yes"],
          [mhd_cv_cc_macro_variadic="no"]
        )
        ac_c_werror_flag=""
      ]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_macro_variadic],["yes"],
  [AC_DEFINE([HAVE_MACRO_VARIADIC],[1],[Define to '1' if your compiler supports variadic macros])]
)

CFLAGS="${CFLAGS_ac} ${user_CFLAGS} ${nocxx_compat_CFLAGS}"
AC_CACHE_CHECK([whether $CC supports compound literals],[mhd_cv_cc_compound_literals],
  [
    AS_VAR_IF([mhd_cv_cc_warnless_c_flags],["no"],[:],[ac_c_werror_flag="yes"])
    AC_COMPILE_IFELSE(
      [
        AC_LANG_SOURCE([[
struct test_strct
{
  char c;
  int i;
};

int main(void)
{
  struct test_strct strct_var;
  int i;

  strct_var = (struct test_strct) { 'a', 0 };

  i = (int){0};

  if (strct_var.i != i)  /* Process all variables to avoid warnings */
    return 2;

  return 0;
}
          ]]
        )
      ],
      [mhd_cv_cc_compound_literals="yes"],
      [mhd_cv_cc_compound_literals="no"]
    )
    ac_c_werror_flag=""
  ]
)
AS_VAR_IF([mhd_cv_cc_compound_literals],["yes"],
  [
    AC_DEFINE([HAVE_COMPOUND_LITERALS],[1],[Define to '1' if your compiler supports compound literals])

    AC_CACHE_CHECK([whether $CC supports compound literals as arrays of the scope],[mhd_cv_cc_compound_literals_arr_scope],
      [
        AS_VAR_IF([mhd_cv_cc_warnless_c_flags],["no"],[:],[ac_c_werror_flag="yes"])
        AC_COMPILE_IFELSE(
          [
            AC_LANG_SOURCE([[
struct test_strct
{
  char c;
  int i;
};

static int test_strct_func(struct test_strct *strc)
{
  return 3 == strc->i;
}

int main(void)
{
  struct test_strct *strct_ptr1;
  struct test_strct *strct_ptr2;
  int *int_ptr;
  void *ptr;

  ptr = (struct test_strct[]) { { 'p', 11 }, { 'q', 22 }, { 'r', 33 } };
  strct_ptr1 = (struct test_strct[]) { { 'a', 1 }, { 'b', 2 }, { 'c', 3 } };
  int_ptr = (int[]){0xFF,0xFE,0xFD,0xFC,0xFB,0xFA};

  if (2 != (strct_ptr1 + 1)->i)
    return 3;

  strct_ptr2 = (struct test_strct[]) { { 'a', 8 }, { 'b', 9 }, { 'c', 7 } };

  if (0xFE != int_ptr[1])
    return 4;

  if (*(int_ptr + 4) != 0xFB)
    return 5;

  if (!test_strct_func(strct_ptr1 + 2))
    return 6;

  if (int_ptr[2] != 0xFD)
    return 7;

  if (((struct test_strct *)ptr)[0].c == 'p')
    ptr = (struct test_strct[]) { { 'x', 55 }, { 'y', 66 }, { 'z', 77 } };

  if (*((char *)ptr) != 'x')
    return 8;

  if (*(int_ptr + 5) != 0xFA)
    return 9;

  if (strct_ptr2[1].c != 'b')
    return 10;

  return strct_ptr1[1].i == 2 ? 0 : 11;
}

#ifdef __cplusplus
fail test here %%%@<:@-1@:>@ /* Actually, some C++ compilers *may* handle this properly,
                        * but it is nearly impossible to check it reliably.
                        * False-positive results will lead to run-time errors,
                        * so just block it when C++ is used. */
#error This is a C++ compiler
#endif

              ]]
            )
          ],
          [mhd_cv_cc_compound_literals_arr_scope="yes"],
          [mhd_cv_cc_compound_literals_arr_scope="no"]
        )
        ac_c_werror_flag=""
      ]
    )
    AS_VAR_IF([mhd_cv_cc_compound_literals_arr_scope],["yes"],
      [AC_DEFINE([HAVE_COMPOUND_LITERALS_ARRAYS_SCOPE],[1],[Define to '1' if your compiler supports compound literals as arrays of the scope])]
    )

    AC_CACHE_CHECK([whether $CC supports compound literals as local arrays],[mhd_cv_cc_compound_literals_arr_local],
      [
        AS_VAR_IF([mhd_cv_cc_warnless_c_flags],["no"],[:],[ac_c_werror_flag="yes"])
        AC_COMPILE_IFELSE(
          [
            AC_LANG_SOURCE([[
struct test_strct
{
  char c;
  int i;
};

static int test_strct_func(struct test_strct *strc)
{
  return 0 == strc->i;
}

int main(void)
{
  if (test_strct_func((struct test_strct[]) { { 'a', 0 }, { 'b', 1 }, { 'c', 2 } }))
    return 4;

  return ((struct test_strct[]) { { 'a', 0 }, { 'b', 1 }, { 'c', 2 } })->i;
}
              ]]
            )
          ],
          [mhd_cv_cc_compound_literals_arr_local="yes"],
          [mhd_cv_cc_compound_literals_arr_local="no"]
        )
        ac_c_werror_flag=""
      ]
    )
    AS_VAR_IF([mhd_cv_cc_compound_literals_arr_local],["yes"],
      [AC_DEFINE([HAVE_COMPOUND_LITERALS_ARRAYS_LOCAL],[1],[Define to '1' if your compiler supports compound literals as local arrays])]
    )

    AC_CACHE_CHECK([whether $CC supports compound literals as lvalue],[mhd_cv_cc_compound_literals_lvalues],
      [
        AS_VAR_IF([mhd_cv_cc_warnless_c_flags],["no"],[:],[ac_c_werror_flag="yes"])
        AC_COMPILE_IFELSE(
          [
            AC_LANG_SOURCE([[
struct test_strct
{
  char c;
  int i;
};

static int test_strct_func(struct test_strct *strc)
{
  return 0 != strc->i;
}

int main(void)
{
  int int_var;
  int *int_ptr;

  int_var = ++(int) {0};
  int_ptr = &((int) {1});

  if (int_var != *int_ptr)  /* Process all variables to avoid warnings */
    return 2;

  if (test_strct_func(&( (struct test_strct) { 'a', 0 } )))
    return 4;

  return 0;
}
              ]]
            )
          ],
          [mhd_cv_cc_compound_literals_lvalues="yes"],
          [mhd_cv_cc_compound_literals_lvalues="no"]
        )
        ac_c_werror_flag=""
      ]
    )
    AS_VAR_IF([mhd_cv_cc_compound_literals_lvalues],["yes"],
      [AC_DEFINE([HAVE_COMPOUND_LITERALS_LVALUES],[1],[Define to '1' if your compiler supports compound literals as lvalues])]
    )
  ],
  [
    AS_UNSET([mhd_cv_cc_compound_literals_arr_local])
    AS_UNSET([mhd_cv_cc_compound_literals_lvalue])
    AS_UNSET([mhd_cv_cc_compound_literals_arr_scope])
  ]
)
CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
AC_CACHE_CHECK([whether $CC supports trailing comma in enum],[mhd_cv_enum_trailing_comma],
  [
    AS_VAR_IF([mhd_cv_cc_warnless_c_flags],["no"],[:],[ac_c_werror_flag="yes"])
    AC_COMPILE_IFELSE(
      [AC_LANG_SOURCE([[enum TestEnum {ENM_VAL_A, ENM_VAL_B,};]])],
      [mhd_cv_enum_trailing_comma="yes"],
      [mhd_cv_enum_trailing_comma="no"]
    )
    ac_c_werror_flag=""
  ]
)
AS_VAR_IF([mhd_cv_enum_trailing_comma],["yes"],[],
  [AC_DEFINE([MHD_ENUMS_NEED_TRAILING_VALUE],[1],[Define to '1' if your compiler does not support enum with trailing comma])]
)
AC_CACHE_CHECK([whether $CC supports enum fixed underlying type],[mhd_cv_enum_base_type],
  [
    AC_COMPILE_IFELSE(
      [AC_LANG_SOURCE([[enum TestEnum : unsigned char {ENM_VAL_A, ENM_VAL_B};]])],
      [mhd_cv_enum_base_type="yes"],
      [
        mhd_cv_enum_base_type="no"
        mhd_cv_enum_base_type_warnless="no"
      ]
    )
  ]
)
AS_VAR_IF([mhd_cv_enum_base_type],["yes"],
  [
    AC_DEFINE([HAVE_ENUM_BASE_TYPE],[1],[Define to '1' if your compiler supports enum with fixed underlying type])
    AH_TEMPLATE([HAVE_ENUM_BASE_TYPE_WARNLESS],[Define to '1' i][f your compiler supports enum with fixed underlying type without extra warnings])
    AS_VAR_IF([mhd_cv_cc_warnless_c_flags],["no"],
      [AC_DEFINE([HAVE_ENUM_BASE_TYPE_WARNLESS],[1])],
      [
        AC_CACHE_CHECK([whether enum fixed underlying type works without extra warnings],[mhd_cv_enum_base_type_warnless],
          [
            AS_VAR_IF([mhd_cv_cc_warnless_c_flags],["no"],[:],[ac_c_werror_flag="yes"])
            AC_COMPILE_IFELSE(
              [AC_LANG_SOURCE([[enum TestEnum : unsigned char {ENM_VAL_A, ENM_VAL_B};]])],
              [mhd_cv_enum_base_type_warnless="yes"],
              [mhd_cv_enum_base_type_warnless="no"]
            )
            ac_c_werror_flag=""
          ]
        )
        AS_VAR_IF([mhd_cv_enum_base_type_warnless],["yes"],[AC_DEFINE([HAVE_ENUM_BASE_TYPE_WARNLESS],[1])])
      ]
    )
  ]
)

# MHD uses 'restrict' keyword in a standard way and does not need
# extra combinations checked by AC_C_RESTRICT
AC_CACHE_CHECK([for 'restrict' keyword supported by $CC],
  [mhd_cv_c_restrict],
  [
    ac_c_werror_flag="yes"
    for chk_kw in restrict __restrict__ __restrict _restrict _Restrict; do
        AC_COMPILE_IFELSE(
          [
            AC_LANG_SOURCE([[
#ifdef __cplusplus
#define restrict do not use 'restrict' with C++ compilers [-1]
#endif
void test_sum(unsigned int n,
              unsigned int *${chk_kw} dst_arr,
              const unsigned int *${chk_kw} src_arr,
              const unsigned int *${chk_kw} bias);
void test_sum(unsigned int n,
              unsigned int *${chk_kw} dst_arr,
              const unsigned int *${chk_kw} src_arr,
              const unsigned int *${chk_kw} bias)
{ while (n--) {dst_arr[n] = src_arr[n] + (*bias);} }
              ]]
            )
          ],[mhd_cv_c_restrict="${chk_kw}"]
        )
        test -z "${mhd_cv_c_restrict}" || break
    done
    ac_c_werror_flag=""
    test -n "${mhd_cv_c_restrict}" || mhd_cv_c_restrict="no"
  ]
)
AS_CASE([${mhd_cv_c_restrict}],
  [restrict],[],
  [no],
  [AC_DEFINE([restrict],[/* Unsupported. Empty macro */],[Define to supported 'restrict' replacement keyword])],
  [AC_DEFINE_UNQUOTED([restrict],["${mhd_cv_c_restrict}"])]
)
AC_C_INLINE
AS_VAR_IF([ac_cv_c_inline],["no"],[],
  [
    # Check whether "enable" flag is supported as "disable" flag could be silently
    # accepted when the compiled code itself does not produce any warnings
    CFLAGS="${user_CFLAGS}"
    MHD_CHECK_CC_CFLAG([-Winline],[CFLAGS_ac],
      [MHD_CHECK_ADD_CC_CFLAG([-Wno-inline],[CFLAGS_ac])]
    )
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
  ]
)
AS_UNSET([errattr_CFLAGS])
CFLAGS="${user_CFLAGS}"
MHD_CHECK_CC_CFLAG([-Werror=attributes],[CFLAGS_ac],
  [
    AC_CACHE_CHECK([whether -Werror=attributes actually works],[mhd_cv_cflag_werror_attr_works],
      [
        CFLAGS="${CFLAGS_ac} ${user_CFLAGS} -Werror=attributes"
        AC_COMPILE_IFELSE([
            AC_LANG_PROGRAM(
              [[__attribute__((non_existing_attrb_dummy)) static int SimpleFunc(void) {return 3;}]],
              [[int r = SimpleFunc(); if (r) return r;]]
            )
          ],
          [mhd_cv_cflag_werror_attr_works="no"],
          [mhd_cv_cflag_werror_attr_works="yes"]
        )
      ]
    )
    AS_VAR_IF([mhd_cv_cflag_werror_attr_works],["yes"],
      [errattr_CFLAGS="-Werror=attributes"],[errattr_CFLAGS=""]
    )
  ]
)
CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
AS_IF([test "x$ac_cv_c_inline" != "xno"],
  [
    AC_DEFINE([HAVE_INLINE_FUNCS],[1],[Define to 1 if your C compiler supports inline functions.])
    AC_CACHE_CHECK([for function force static inline keywords supported by $CC],[mhd_cv_cc_kwd_static_forceinline],
      [
        AS_VAR_IF([mhd_cv_cc_warnless_c_flags],["no"],[:],[ac_c_werror_flag="yes"])
        mhd_cv_cc_kwd_static_forceinline="none"
        CFLAGS="${CFLAGS_ac} ${user_CFLAGS} $errattr_CFLAGS"
        for keyword_chk in 'static inline __attribute__((always_inline))' 'static __forceinline' '_Pragma("inline=forced") static inline'
        do
          AS_CASE([${keyword_chk}],
            [*attribute*],
            [AS_IF([test "x$errattr_CFLAGS" = "x"],[continue])]
          )
          AC_LINK_IFELSE([
              AC_LANG_SOURCE([[
#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#endif
${keyword_chk} bool cmpfn(int x, int y)
{ return x > y; }
${keyword_chk} int sumfn(int x, int y)
{ return x + y; }

int main(void)
{
  int a = 1, b = 100, c;
  if (cmpfn(a, b))
    c = sumfn(a, b);
  else
    c = 0 - sumfn(a, b);
  return (cmpfn(0, c) ? 0 : 5);
}
                ]]
              )
            ],
            [mhd_cv_cc_kwd_static_forceinline="${keyword_chk}"]
          )
          test "x${mhd_cv_cc_kwd_static_forceinline}" != "xnone" && break
        done
        CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
        ac_c_werror_flag=""
      ]
    )
  ]
)
AS_IF([test "x$ac_cv_c_inline" != "xno" && test "x${mhd_cv_cc_kwd_static_forceinline}" != "xnone"],
  [AC_DEFINE_UNQUOTED([mhd_static_inline],[$mhd_cv_cc_kwd_static_forceinline],[Define to prefix which will be used with MHD internal static inline functions.])]
  ,
  [AC_DEFINE([mhd_static_inline],[static inline],[Define to prefix which will be used with MHD static inline functions.])]
)

AS_VAR_IF([mhd_cv_cc_warnless_c_flags],["no"],[:],[ac_c_werror_flag="yes"])
AC_CACHE_CHECK([whether 'constexpr' keywords supported by $CC],[mhd_cv_cc_kwd_constexpr],
  [
    AC_LINK_IFELSE([
      AC_LANG_SOURCE([[

static constexpr int zero_const_expr_s = 0;
constexpr unsigned int one_const_expr_u = 1;

enum some_Enm
{
  enm_Val_Zero = 0,
  enm_Val_One = one_const_expr_u,
  enm_Val_Two = 2
};

int main(int argc, char *argv[])
{
  enum some_Enm local_e_var;
  static constexpr unsigned int arr0[1 + zero_const_expr_s] = { one_const_expr_u };
  constexpr int arr1[one_const_expr_u] = { zero_const_expr_s };

  switch(argc)
  {
  case zero_const_expr_s:
    local_e_var = enm_Val_One;
    break;
  case 1:
    local_e_var = enm_Val_Zero;
    break;
  case 10:
    local_e_var = enm_Val_Two;
    break;
  default:
    local_e_var = enm_Val_Two;
    break;
  }

  return (arr0[0] + (unsigned int)local_e_var) ==
         one_const_expr_u ? (arr1[0] - zero_const_expr_s) : (!! argv[0]);
}
          ]]
        )
      ],
      [mhd_cv_cc_kwd_constexpr="yes"],[mhd_cv_cc_kwd_constexpr="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_kwd_constexpr],["yes"],
  [AC_DEFINE([MHD_HAVE_C_CONSTEXPR],[1],[Define to '1' if compiler supports "constexpr" keyword])]
)
AC_CACHE_CHECK([for 'unreachable' keywords supported by $CC],[mhd_cv_cc_kwd_unreachable],
  [
    mhd_cv_cc_kwd_unreachable="none"
    for keyword_chk in '__builtin_unreachable()' '__assume(0)' 'unreachable()'
    do
      AC_LINK_IFELSE([
          AC_LANG_SOURCE([[
#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif

static int zero_if_positive(int val)
{
  if (val > 0)
    return 0;
  ${keyword_chk};
  return 1;
}

int main(void)
{
  return zero_if_positive(1);
}
            ]]
          )
        ],
        [mhd_cv_cc_kwd_unreachable="$keyword_chk"]
      )
      test "x${mhd_cv_cc_kwd_unreachable}" != "xnone" && break
    done
  ]
)
AS_IF([test "x${mhd_cv_cc_kwd_unreachable}" != "xnone"],
  [
    AC_DEFINE_UNQUOTED([MHD_UNREACHABLE_KEYWORD],[$mhd_cv_cc_kwd_unreachable],[Define to keyword supported to indicate unreachable code paths])
    AS_IF([test "x${mhd_cv_cc_kwd_unreachable}" = "xunreachable()" && test "x$ac_cv_header_stddef_h" = "xyes"],
      [AC_DEFINE([MHD_UNREACHABLE_NEEDS_STDDEF_H],[$mhd_cv_cc_kwd_unreachable],[Define to '1' if MHD_UNREACHABLE_KEYWORD requires include of <stddef.h> header])]
    )
  ]
)

AC_CACHE_CHECK([for 'assume' hint keywords supported by $CC],[mhd_cv_cc_kwd_assume],
  [
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS} ${errattr_CFLAGS}"
    mhd_cv_cc_kwd_assume="none"
    for keyword_chk in '__attribute__((assume(statement)))' '__builtin_assume(statement)' '[[assume(statement)]]' '__assume(statement)'
    do
      AC_LINK_IFELSE([
          AC_LANG_SOURCE([[

#define MACRO_ASSUME(statement) ${keyword_chk}

static int zero_or_n(int cnd, int val)
{
  MACRO_ASSUME(cnd >= 0);
  MACRO_ASSUME(cnd <= 1);
  return cnd * val;
}

int main(int argc, char *const *argv)
{
  (void) argv;
  return zero_or_n(0 > argc, 2);
}
            ]]
          )
        ],
        [mhd_cv_cc_kwd_assume="$keyword_chk"]
      )
      test "x${mhd_cv_cc_kwd_assume}" != "xnone" && break
    done
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
  ]
)
AS_IF([test "x${mhd_cv_cc_kwd_assume}" != "xnone"],
  [AC_DEFINE_UNQUOTED([MHD_ASSUME_KEYWORD(statement)],[$mhd_cv_cc_kwd_assume],[Define to keyword supported to indicate a hard promise])]
)
ac_c_werror_flag=""

# Check for 'fallthrough' keywords
save_CFLAGS_ac="${CFLAGS_ac}"
CFLAGS="${user_CFLAGS}"
# Use strictest warning flags
MHD_FIND_ADD_CC_CFLAG([CFLAGS_ac], [-Wimplicit-fallthrough=5],[-Wimplicit-fallthrough=4],[-Wimplicit-fallthrough=3],
[-Wimplicit-fallthrough])
AC_CACHE_CHECK([for 'fallthrough' keyword supported by $CC],[mhd_cv_cc_kwd_fallthrough],
  [
    mhd_cv_cc_kwd_fallthrough="no"
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS} ${errattr_CFLAGS}"
    AS_VAR_IF([mhd_cv_cc_warnless_c_flags],["no"],[:],[ac_c_werror_flag="yes"])
    [for keyword_chk in '[[fallthrough]]' '[[__fallthrough__]]' '[[gnu::fallthrough]]' '[[clang::fallthrough]]' '__attribute__((fallthrough))' '']
    do
      AC_LINK_IFELSE([
          AC_LANG_SOURCE([[
int main(int argc, char *argv[])
{
  int ret = 0;
  (void) argv;
  switch (argc)
  {
    case 0:
      ++ret;
      ${keyword_chk};
    case 3:
      ++ret;
      ${keyword_chk};
    case 10:
      ++ret;
      ${keyword_chk};
    default:
      break;
  }
  return ret;
}
            ]]
          )
        ],
        [mhd_cv_cc_kwd_fallthrough="$keyword_chk"]
      )
      test "x${mhd_cv_cc_kwd_fallthrough}" != "xno" && break
    done
    AS_IF([test -z "${mhd_cv_cc_kwd_fallthrough}"], [mhd_cv_cc_kwd_fallthrough="none needed"])
    ac_c_werror_flag=""
  ]
)
AH_TEMPLATE([mhd_FALLTHROUGH],[Define to keyword marking intentional missing 'break' at the end of 'case:'])
AS_CASE([${mhd_cv_cc_kwd_fallthrough}],
  [no],[AC_DEFINE([mhd_FALLTHROUGH],[((void) 0) /* no keyword supported */])],
  ["none needed"],[AC_DEFINE([mhd_FALLTHROUGH],[((void) 0) /* not needed */])],
  [AC_DEFINE_UNQUOTED([mhd_FALLTHROUGH],[$mhd_cv_cc_kwd_fallthrough])]
)
CFLAGS_ac="${save_CFLAGS_ac}"
CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"

# Check for full support of atomic variables
AC_CACHE_CHECK([whether $CC supports atomic variables],
  [mhd_cv_c_atomic_variables],
  [
    AC_COMPILE_IFELSE(
      [
        AC_LANG_SOURCE(
          [[
#include <stdatomic.h>

#ifdef __STDC_NO_ATOMICS__
#error __STDC_NO_ATOMICS__ is declared
fail test here %%%@<:@-1@:>@
#endif

int main(void)
{
  volatile _Atomic int atmc_var;
  int natmc_var;

  atomic_init(&atmc_var, 0);

  if (0 == atomic_exchange_explicit(&atmc_var, 16, memory_order_relaxed))
    atomic_store_explicit(&atmc_var, 7, memory_order_release);

  natmc_var = atomic_fetch_add_explicit(&atmc_var, 15, memory_order_acq_rel);

  natmc_var = atomic_fetch_sub_explicit(&atmc_var, natmc_var, memory_order_relaxed);

  natmc_var = atomic_fetch_or_explicit(&atmc_var, natmc_var, memory_order_seq_cst);

  natmc_var = atomic_fetch_and_explicit(&atmc_var, natmc_var, memory_order_relaxed);

  natmc_var = atomic_fetch_xor_explicit(&atmc_var, natmc_var, memory_order_release);

  return natmc_var + 1 - atomic_load_explicit(&atmc_var, memory_order_acquire);
}
          ]]
        )
      ],
      [mhd_cv_c_atomic_variables="yes"],[mhd_cv_c_atomic_variables="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_c_atomic_variables],["yes"],
  [
    AC_DEFINE([MHD_SUPPORT_ATOMIC_VARIABLES],[1],[Define to 1 i][f atomic variables are fully supported])
    AC_DEFINE([MHD_SUPPORT_ATOMIC_COUNTERS],[1],[Define to 1 i][f counter subset of atomic operations is supported])
  ],
  [
    # Check for minimal atomic variables support
    AC_CACHE_CHECK([whether $CC supports atomic counters],
      [mhd_cv_c_atomic_counters],
      [
        AC_COMPILE_IFELSE(
          [
            AC_LANG_SOURCE(
              [[
#if defined(HAVE_STDDEF_H)
#  include <stddef.h> /* size_t */
#elif defined(HAVE_STDLIB_H)
#  include <stdlib.h> /* should provide size_t */
#else
#  include <stdio.h> /* should provide size_t */
#endif
#ifdef HAVE_CRTDEFS_H
#  include <crtdefs.h> /* W32-specific header */
#endif

#include <stdatomic.h>

#ifdef __STDC_NO_ATOMICS__
#error __STDC_NO_ATOMICS__ is declared
fail test here %%%@<:@-1@:>@
#endif

int main(void)
{
  volatile _Atomic size_t cntr;

  atomic_init(&cntr, 0);

  atomic_fetch_add_explicit(&cntr, 1, memory_order_relaxed);
  atomic_fetch_sub_explicit(&cntr, 1, memory_order_release);

  return
    (0u == atomic_load_explicit(&cntr, memory_order_relaxed)) ?
    0 : 2;
}
              ]]
            )
          ],
          [mhd_cv_c_atomic_counters="yes"],[mhd_cv_c_atomic_counters="no"]
        )
      ]
    )
    AS_VAR_IF([mhd_cv_c_atomic_counters],["yes"],
      [AC_DEFINE([MHD_SUPPORT_ATOMIC_COUNTERS],[1],[Define to 1 i][f counter subset of atomic operations is supported])]
    )
  ]
)

AC_CHECK_HEADERS([stdalign.h], [], [], [AC_INCLUDES_DEFAULT])
AC_CACHE_CHECK([[for C11 'alignof()' support]], [[mhd_cv_c_alignof]],
  [AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM(
        [[
#ifdef HAVE_STDALIGN_H
#include <stdalign.h>
#endif
        ]], [[
          int var1[(alignof(int) >= 2) ? 1 : -1];
          int var2[alignof(unsigned int) - 1];
          int var3[(alignof(char) > 0) ? 1 : -1];
          int var4[(alignof(long) >= 4) ? 1 : -1];

          /* Mute compiler warnings */
          var1[0] = var2[0] = var3[0] = 0;
          var4[0] = 1;
          if (var1[0] + var2[0] + var3[0] == var4[0])
            return 1;
        ]])
     ], [
          AC_COMPILE_IFELSE(
		    [AC_LANG_PROGRAM(
		        [[
#ifdef HAVE_STDALIGN_H
#include <stdalign.h>
#endif
		        ]], [[
		          /* Should fail if 'alignof()' works */
		          int var1[alignof(nonexisting_type)];

		          /* Mute compiler warnings */
		          var1[0] = 1;
		          if (var1[0] + 1 == 1)
		            return 1;
		        ]])
		    ], [[mhd_cv_c_alignof='no']], [[mhd_cv_c_alignof='yes']])
        ], [[mhd_cv_c_alignof='no']])
  ])
AS_VAR_IF([mhd_cv_c_alignof], ["yes"],
  [AC_DEFINE([[HAVE_C_ALIGNOF]], [1], [Define to 1 if your compiler supports 'alignof()'])])

AC_CACHE_CHECK([whether $CC supports __attribute__((aligned ( )))],[mhd_cv_cc_attr_aligned],
  [
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS} ${errattr_CFLAGS}"
    AC_LINK_IFELSE(
      [
        AC_LANG_SOURCE(
          [[
int main(void)
{
  __attribute__((aligned (4))) signed char schr[1] = {0};
  return schr[0];
}
          ]]
        )
      ],
      [mhd_cv_cc_attr_aligned="yes"],[mhd_cv_cc_attr_aligned="no"]
    )
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_aligned],["yes"],
  [AC_DEFINE([[HAVE_ATTR_ALIGNED]], [1], [Define to 1 if compiler supports __attribute__((aligned (X)))])],
  [
    AC_CACHE_CHECK([whether $CC supports __declspec(align( )))],[mhd_cv_cc_declspec_align],
      [
        CFLAGS="${CFLAGS_ac} ${user_CFLAGS} ${errattr_CFLAGS}"
        AC_LINK_IFELSE(
          [
            AC_LANG_SOURCE(
              [[
int main(void)
{
  __declspec(align(4))) signed char schr[1] = {0};
  return schr[0];
}
              ]]
            )
          ],
          [mhd_cv_cc_declspec_align="yes"],[mhd_cv_cc_declspec_align="no"]
        )
        CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
      ]
    )
    AS_VAR_IF([mhd_cv_cc_declspec_align],["yes"],
      [AC_DEFINE([[HAVE_DECLSPEC_ALIGN]], [1], [Define to 1 if compiler supports __declspec(align(X)))])]
    )
  ]
)
AC_CACHE_CHECK([fo][r C11 alignas() support],[mhd_cv_c_alignas],
  [
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS} ${errattr_CFLAGS}"
    AC_LINK_IFELSE(
      [
        AC_LANG_SOURCE(
          [[
#ifdef HAVE_STDALIGN_H
#if ! defined(__STDC_VERSION__) || (__STDC_VERSION__+0) < 202311
#include <stdalign.h>
#endif
#endif

int main(void)
{
  alignas(4) signed char schr[1] = {0};
  alignas(int) signed char schr_var = 0;
  return schr[0] + schr_var;
}
          ]]
        )
      ],
      [mhd_cv_c_alignas="yes"],[mhd_cv_c_alignas="no"]
    )
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
  ]
)
AS_VAR_IF([mhd_cv_c_alignas],["yes"],
  [AC_DEFINE([[HAVE_C_ALIGNAS]], [1], [Define to 1 if C11 alignas() is supported])]
)
AS_IF([test "x${mhd_cv_cc_attr_aligned}${mhd_cv_cc_declspec_align}${mhd_cv_c_alignas}" != "xnonono"],
  [
    AC_CACHE_CHECK([fo][r large alignment number supported],[mhd_cv_alignment_large_num],
      [
        CFLAGS="${CFLAGS_ac} ${user_CFLAGS} ${errattr_CFLAGS}"
        AS_VAR_IF([mhd_cv_cc_warnless_c_flags],["no"],[:],[ac_c_werror_flag="yes"])
        for mhd_cv_alignment_large_num in 65536 32768 16384 8192 4096 2048 1024 512 256 128 64 32 16 8 4 2 1 0; do
          AC_LINK_IFELSE(
            [
              AC_LANG_SOURCE(
                [[
#ifdef HAVE_STDALIGN_H
#if ! defined(__STDC_VERSION__) || (__STDC_VERSION__+0) < 202311
#include <stdalign.h>
#endif
#endif

/* Priority for alignas() implementation as it should warn more */
#ifdef HAVE_C_ALIGNAS
#define test_alignas(x)  alignas(x)
#elif defined(HAVE_ATTR_ALIGNED)
#define test_alignas(x) __attribute__((aligned (x)))
#elif defined(HAVE_DECLSPEC_ALIGN)
#define test_alignas(x) __declspec(align(x)))
#else
#error No alignas() implementation available
fail test here %%%@<:@-1@:>@
#endif

int main(void)
{
  test_alignas($mhd_cv_alignment_large_num) signed char ret = 0;
  return ret;
}
                ]]
              )
            ],
            [break
            ]
          )
        done
        ac_c_werror_flag=""
        CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
      ]
    )
    AS_IF([test "$mhd_cv_alignment_large_num" -gt "1"],
      [
        AC_DEFINE_UNQUOTED([MHD_ALIGNMENT_LARGE_NUM],[$mhd_cv_alignment_large_num],
            [Define to large enough number that is still supported as an alignment])
      ]
      ,
      [AC_MSG_WARN([no supported aligment number can be detected])]
    )
  ]
)


# Check system type
AC_MSG_CHECKING([[for target host OS]])
os_is_windows="no"
os_is_native_w32="no"
AS_CASE(["$host_os"],
 [*darwin* | *rhapsody* | *macosx*],
 [AC_DEFINE([OSX],[1],[This is an OS X system])
     mhd_host_os='Darwin'
     AC_MSG_RESULT([[$mhd_host_os]])],
 [kfreebsd*-gnu],
 [AC_DEFINE([SOMEBSD],[1],[This is a BSD system])
     AC_DEFINE([FREEBSD],[1],[This is a FreeBSD system])
     mhd_host_os='FreeBSD kernel with GNU userland'
     AC_MSG_RESULT([[$mhd_host_os]])],
 [freebsd*],
 [AC_DEFINE([SOMEBSD],[1],[This is a BSD system])
     AC_DEFINE([FREEBSD],[1],[This is a FreeBSD system])
     mhd_host_os='FreeBSD'
     AC_MSG_RESULT([[$mhd_host_os]])],
 [openbsd*],
 [AC_DEFINE([SOMEBSD],[1],[This is a BSD system])
     AC_DEFINE([OPENBSD],[1],[This is an OpenBSD system])
     mhd_host_os='OpenBSD'
     AC_MSG_RESULT([[$mhd_host_os]])],
 [netbsd*],
 [AC_DEFINE([SOMEBSD],[1],[This is a BSD system])
     AC_DEFINE([NETBSD],[1],[This is a NetBSD system])
     mhd_host_os='NetBSD'
     AC_MSG_RESULT([[$mhd_host_os]])],
 [*solaris*],
 [AC_DEFINE([SOLARIS],[1],[This is a Solaris system])
     AC_DEFINE([_REENTRANT],[1],[Need with solaris or errno does not work])
     mhd_host_os='Solaris'
     AC_MSG_RESULT([[$mhd_host_os]])],
  [linux-gnu],
  [AC_DEFINE([LINUX],[1],[This is a Linux kernel])
     mhd_host_os='GNU/Linux'
     AC_MSG_RESULT([[$mhd_host_os]])],
  [linux-android*],
  [AC_DEFINE([LINUX],[1],[This is a Linux kernel])
     mhd_host_os='Android'
     AC_MSG_RESULT([[$mhd_host_os]])],
  [*linux*],
  [AC_DEFINE([LINUX],[1],[This is a Linux kernel])
     mhd_host_os='Linux'
     AC_MSG_RESULT([[$mhd_host_os]])],
  [*cygwin*],
  [AC_DEFINE([CYGWIN],[1],[This is a Cygwin system])
     mhd_host_os='Windows/Cygwin'
     AC_MSG_RESULT([[$mhd_host_os]])
     os_is_windows="yes"],
  [*mingw*],
  [
    AC_DEFINE([MINGW],[1],[This is a MinGW system])
     AC_DEFINE([WINDOWS],[1],[This is a Windows system])
     mhd_host_os='Windows/MinGW'
     AC_MSG_RESULT([[$mhd_host_os]])
     AC_CHECK_HEADERS([winsock2.h ws2tcpip.h], [], [AC_MSG_ERROR([[Winsock2 headers are required for W32]])], [AC_INCLUDES_DEFAULT])
     AC_CHECK_HEADERS([sdkddkver.h], [], [], [AC_INCLUDES_DEFAULT])
     AC_CACHE_CHECK([for MS lib utility], [ac_cv_use_ms_lib_tool],
       [mslibcheck=`lib 2>&1`
        AS_IF([echo "$mslibcheck" | $GREP -e '^Microsoft (R) Library Manager' - >/dev/null],
          [ac_cv_use_ms_lib_tool=yes],
          [ac_cv_use_ms_lib_tool=no])
         ])
     AS_IF([test "x$ac_cv_use_ms_lib_tool" = "xyes"],
       [AC_SUBST([MS_LIB_TOOL], [[lib]])])
     AC_SUBST([lt_cv_objdir])
     os_is_windows="yes"
     os_is_native_w32="yes"
  ],
  [*openedition*],
  [AC_DEFINE([OS390],[1],[This is a OS/390 system])
    mhd_host_os='OS/390'
    AC_MSG_RESULT([[$mhd_host_os]])],
  [gnu*],
  [AC_DEFINE([[GNU_HURD]], [[1]], [Define to `1' if host machine runs on GNU Hurd.])
    mhd_host_os='GNU Hurd'
    AC_MSG_RESULT([[$mhd_host_os]])
  ],
  [haiku],
  [AC_DEFINE([[mhd_OS_IS_HAIKU]], [[1]], [Define to '1' if the library is hosted on Haiku.])
    mhd_host_os='Haiku'
    AC_MSG_RESULT([[$mhd_host_os]])
  ],
  [
    AC_MSG_RESULT([unrecognised OS])
    mhd_host_os="${host_os}"
    AC_MSG_WARN([Unrecognised OS $host_os])
    AC_DEFINE([OTHEROS],1,[Some strange OS])
  ]
)

AM_CONDITIONAL([CYGWIN_TARGET], [[test "x$os_is_windows" = "xyes" && \
                                  test "x${os_is_native_w32}" != "xyes"]])

AS_VAR_IF([os_is_windows], ["yes"],
  [
    AC_MSG_CHECKING([[whether target W32 version is specified by precompiler defines]])
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
/* Note: check logic is reversed for easy log reading */
#ifdef WINVER
#error WINVER is defined
fail test here %%%@<:@-1@:>@
#endif
#ifdef _WIN32_WINNT
#error _WIN32_WINNT is defined
fail test here %%%@<:@-1@:>@
#endif
#ifdef NTDDI
#error NTDDI is defined
fail test here %%%@<:@-1@:>@
#endif
        ]],[[(void)0]])
      ], [[mhd_w32_ver_preselect=no]], [[mhd_w32_ver_preselect=yes]]
    )
    AC_MSG_RESULT([[${mhd_w32_ver_preselect}]])
    AC_CHECK_HEADERS([windows.h sdkddkver.h], [], [], [AC_INCLUDES_DEFAULT])
    AS_VAR_IF([mhd_w32_ver_preselect],["yes"],
     [
       AC_MSG_CHECKING([[for specified target W32 version]])
       AS_UNSET([[mhd_w32_ver]])
       AS_UNSET([[mhd_w32_ver_msg]])
       AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if _WIN32_WINNT+0 < 0x0501
/* Check before headers inclusion */
#error _WIN32_WINNT is less than 0x0501
fail test here %%%@<:@-1@:>@
#endif

#ifdef HAVE_SDKDDKVER_H
#include <sdkddkver.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#if _WIN32_WINNT+0 < 0x0501
#error _WIN32_WINNT is less than 0x0501
fail test here %%%@<:@-1@:>@
#endif
           ]],[[(void)0]])
         ], [], [
           AC_MSG_RESULT([[pre-WinXP]])
           AC_MSG_ERROR([[libmicrohttpd cannot be compiled for Windows version before Windows XP]])
         ]
       )
       AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#ifdef HAVE_SDKDDKVER_H
#include <sdkddkver.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#if _WIN32_WINNT+0 == 0x0501
#error _WIN32_WINNT is 0x0501
fail test here %%%@<:@-1@:>@
#endif
#if _WIN32_WINNT+0 == 0x0502
#error _WIN32_WINNT is 0x0502
fail test here %%%@<:@-1@:>@
#endif
           ]],[[(void)0]])
         ], [], [
           mhd_w32_ver="WinXP"
           mhd_w32_ver_msg="WinXP (selected by precompiler flags)"
         ]
       )
       AS_VAR_SET_IF([mhd_w32_ver], [],
         [
           AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#ifdef HAVE_SDKDDKVER_H
#include <sdkddkver.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#if _WIN32_WINNT+0 < 0x0600
#error _WIN32_WINNT is less than 0x0600 but greater than 0x0502
fail test here %%%@<:@-1@:>@
#endif
                ]],[[(void)0]])
             ], [], [
               AC_MSG_ERROR([[_WIN32_WINNT value is wrong (less than 0x0600 but greater than 0x0502)]])
             ]
           )

           AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#ifdef HAVE_SDKDDKVER_H
#include <sdkddkver.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#if _WIN32_WINNT+0 == 0x0600
#error _WIN32_WINNT is 0x0600
fail test here %%%@<:@-1@:>@
#endif
                ]],[[(void)0]])
             ], [], [
               mhd_w32_ver="Vista"
               mhd_w32_ver_msg="Vista (selected by precompiler flags)"
             ]
           )
         ]
       )

       AS_VAR_SET_IF([mhd_w32_ver], [],
         [
           AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#ifdef HAVE_SDKDDKVER_H
#include <sdkddkver.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#if _WIN32_WINNT+0 > 0x0600
#error _WIN32_WINNT is greater than 0x0600
fail test here %%%@<:@-1@:>@
#endif
                ]],[[(void)0]])
             ], [
               mhd_w32_ver="unknown"
               mhd_w32_ver_msg="unknown (cannot be detected)"
             ], [
               mhd_w32_ver="newer than Vista"
               mhd_w32_ver_msg="newer than Vista (selected by precompiler flags)"
             ]
           )
         ]
       )
       AC_MSG_RESULT([[${mhd_w32_ver}]])
     ], [
       mhd_w32_ver="Vista"
       mhd_w32_ver_msg="Vista (default, override by CPPFLAGS=-D_WIN32_WINNT=0xNNNN)"
       CPPFLAGS_ac="${CPPFLAGS_ac} -D_WIN32_WINNT=0x0600"
       CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
       AC_MSG_CHECKING([[whether headers accept _WIN32_WINNT=0x0600]])
       AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#ifdef HAVE_SDKDDKVER_H
#include <sdkddkver.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
#include <stdio.h>
            ]],[[(void)0]])
         ], [
           AC_MSG_RESULT([[yes]])
         ], [
           AC_MSG_RESULT([[no]])
           AC_MSG_ERROR([Headers do not accept _WIN32_WINNT=0x0600. Consider override target W32 version by CPPFLAGS=-D_WIN32_WINNT=0xNNNN])
         ]
       )
     ]
    )
  ]
)

AS_IF([test "x${os_is_windows}" = "xyes" && test "x${os_is_native_w32}" = "xyes"],
  [
    AC_CACHE_CHECK([W32 run-time library type], [mhd_cv_wctr_type],
      [
        AC_EGREP_CPP([MHDMARKER: UCRT run-time library in use!], [
#include <stdio.h>
#if defined(_UCRT)
#define CRT_STR "MHDMARKER: UCRT run-time library in use!"
#endif
#if defined(__MSVCRT_VERSION__)
#if (__MSVCRT_VERSION__ >= 0xE00) && (__MSVCRT_VERSION__ < 0x1000)
#define CRT_STR "MHDMARKER: UCRT run-time library in use!"
#endif
#if (__MSVCRT_VERSION__ > 0x1400)
#define CRT_STR "MHDMARKER: UCRT run-time library in use!"
#endif
#endif

#ifndef CRT_STR
#define CRT_STR "MHDMARKER: MSVCRT run-time library in use!"
#endif

int main(void)
{
  printf ("%\n", CRT_STR);
  return 0;
}
          ],
          [mhd_cv_wctr_type="ucrt"], [mhd_cv_wctr_type="msvcrt"])
      ]
    )
    mhd_host_os="${mhd_host_os}-${mhd_cv_wctr_type}"
    AS_VAR_IF([mhd_cv_wctr_type], ["msvcrt"],
      [
        # Use CFLAGS here to override user-supplied wrong CPPFLAGS. Durty trick, but choice is limited.
        AX_APPEND_COMPILE_FLAGS([-U__USE_MINGW_ANSI_STDIO -D__USE_MINGW_ANSI_STDIO=0], [CFLAGS_ac])
        AC_SUBST([W32CRT], [MSVCRT])
      ], [AC_SUBST([W32CRT], [UCRT])]
    )

    CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
    LDFLAGS="${user_LDFLAGS}"
    AS_CASE([$mhd_w32_ver],
      [WinXP],
      [MHD_CHECK_ADD_CC_LDFLAG([-Wl,--major-subsystem-version,5,--minor-subsystem-version,1],[LDFLAGS_ac])],
      [Vista],
      [MHD_CHECK_ADD_CC_LDFLAG([-Wl,--major-subsystem-version,6,--minor-subsystem-version,0],[LDFLAGS_ac])]
    )
    LDFLAGS="${LDFLAGS_ac} ${user_LDFLAGS}"
  ]
)
CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"


AC_ARG_WITH([threads],
   [AS_HELP_STRING([--with-threads=LIB],[choose threading library (posix, w32, auto, none) [auto]])],
   [], [with_threads='auto'])
AS_CASE([[$with_threads]],
  [[win32]], [[with_threads='w32']],
  [[pthreads]], [[with_threads='posix']],
  [[posix]], [[:]],
  [[w32]], [[:]],
  [[none]], [[with_threads='none']],
  [[no]], [[with_threads='none']],
  [[auto]], [[:]],
    [AC_MSG_ERROR([[incorrect parameter "$with_threads" specified for --with-threads]])]
)

# Check for posix threads support, regardless of configure parameters as
# testsuite uses only posix threads.
AX_PTHREAD(
  [
    mhd_have_posix_threads='yes'
    AC_DEFINE([[HAVE_PTHREAD_H]],[[1]],[Define to 1 if you have the <pthread.h> header file.])
    CFLAGS="${CFLAGS_ac} ${PTHREAD_CFLAGS} ${user_CFLAGS}"
    MHD_CHECK_FUNC([pthread_sigmask],
      [[
#include <pthread.h>
#include <signal.h>
      ]],
      [[
        sigset_t nset, oset;
        sigemptyset (&nset);
        sigaddset (&nset, SIGPIPE);
        i][f (0 != pthread_sigmask(SIG_BLOCK, &nset, &oset)) return 1;
      ]],
      [AC_DEFINE([[HAVE_PTHREAD_SIGMASK]],[[1]],[Define to 1 if you have the pthread_sigmask(3) function.])]
    )
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
  ],[[mhd_have_posix_threads='no']])
AM_CONDITIONAL([HAVE_POSIX_THREADS],[test "x$mhd_have_posix_threads" = "xyes"])

mhd_have_w32_threads='no'
AS_IF([[test "x$with_threads" = "xauto"]],
 [
 AS_IF([[test "x$os_is_windows" = "xyes"]],
   [
    AC_MSG_CHECKING([[for W32 threads]])
    AC_LINK_IFELSE(
      [AC_LANG_PROGRAM([[
#include <windows.h>
         ]], [ HANDLE h = CreateThread(NULL, 0, NULL, NULL, 0, NULL);])]
      , [[mhd_have_w32_threads='yes']], [[mhd_have_w32_threads='no']]
      )
    AC_MSG_RESULT([[$mhd_have_w32_threads]])
   ])
 ]
)

AC_MSG_CHECKING([[for threading lib to use with libmicrohttpd ($with_threads)]])
AS_IF([test "x$with_threads" = "xposix"],
  [ # forced posix threads
    AS_IF([test "x$mhd_have_posix_threads" = "xyes"], [USE_THREADS='posix'],
      [ AS_IF([[test "x$os_is_windows" = "xyes"]] ,
          [ AC_MSG_ERROR([[Posix threads are not available. Try to configure --with-threads=auto]])],
          [ AC_MSG_ERROR([[No threading lib is available. Consider installing pthreads]])] )
      ])
    ])
AS_IF([test "x$with_threads" = "xw32"],
  [ # forced w32 threads
    AS_IF([[test "x$mhd_have_w32_threads" = "xyes"]],
      [[ USE_THREADS='w32' ]],
      [ AC_MSG_ERROR([[W32 threads are not available. Try to configure --with-threads=auto]])])
    ])
AS_IF([test "x$with_threads" = "xauto"],
      [# automatic threads lib selection
       AS_IF([[test "x$os_is_native_w32" = "xyes" && test "x$mhd_have_w32_threads" = "xyes"]] ,
        [[ USE_THREADS='w32' ]] ,
        [[ test "x$mhd_have_posix_threads" = "xyes" ]], [[ USE_THREADS='posix' ]],
        [[ test "x$mhd_have_w32_threads" = "xyes" ]], [[ USE_THREADS='w32' ]],
        [ AC_MSG_ERROR([[No threading lib is available. Consider installing pthreads]]) ]
        )])
AS_IF([test "x$with_threads" = "xnone"],
   [USE_THREADS='none'])

AS_IF([test "x$USE_THREADS" = "xposix"],
  [CC="$PTHREAD_CC"
  AC_DEFINE([mhd_THREADS_KIND_POSIX],[1],[define to use pthreads])
  MHD_LIB_CFLAGS="$MHD_LIB_CFLAGS $PTHREAD_CFLAGS"
  MHD_LIBDEPS="$PTHREAD_LIBS $MHD_LIBDEPS"
  MHD_LIBDEPS_PKGCFG="$MHD_LIBDEPS_PKGCFG $PTHREAD_LIBS"],
  [AS_IF([test "x$USE_THREADS" = "xw32"],
   [AC_DEFINE([mhd_THREADS_KIND_W32],[1],[define to use W32 threads])])])
AM_CONDITIONAL([USE_POSIX_THREADS], [test "x$USE_THREADS" = "xposix"])
AM_CONDITIONAL([USE_W32_THREADS], [test "x$USE_THREADS" = "xw32"])
AM_CONDITIONAL([MHD_SUPPORT_THREADS], [test "x$USE_THREADS" != "xnone"])
AC_MSG_RESULT([$USE_THREADS])

AC_ARG_ENABLE([[thread-names]],
   [AS_HELP_STRING([--disable-thread-names],[do not set names on MHD generated threads [auto]])],
   [], [enable_thread_names='auto'])

AS_IF([test "x$enable_thread_names" != "xno" && test "x$USE_THREADS" = "xposix"],[
  # Check for thread name function
  HAVE_THREAD_NAME_FUNC="no"
  SAVE_LIBS="$LIBS"
  LIBS="$PTHREAD_LIBS $LIBS"
  CFLAGS="${CFLAGS_ac} $PTHREAD_CFLAGS ${user_CFLAGS}"
  AC_CHECK_HEADERS([pthread_np.h],[],[],
    [
AC_INCLUDES_DEFAULT
      [
#include <pthread.h>
      ]
    ])

  # Try to find how to set thread name by thread attributes.
  # If pthread_attr_setname_np(3) is not declared, it's not possible to detect
  # form of pthread_attr_setname_np(3) due to C "feature" "implicit declaration".
  AC_CHECK_DECL([[pthread_attr_setname_np]],[],[],[[
#include <pthread.h>
#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
]])

  AS_IF([[test "x$ac_cv_have_decl_pthread_attr_setname_np" = "xyes"]],
    [AC_MSG_CHECKING([[for pthread_attr_setname_np(3) in NetBSD or OSF1 form]])
     AC_LINK_IFELSE(
      [AC_LANG_PROGRAM([[
#include <pthread.h>
#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
]], [[
      pthread_attr_t thr_attr;
      pthread_attr_init(&thr_attr);
      pthread_attr_setname_np(&thr_attr, "name", 0);
      pthread_attr_destroy(&thr_attr);
        ]])],
        [AC_DEFINE([[HAVE_PTHREAD_ATTR_SETNAME_NP_NETBSD]], [[1]], [Define if you have NetBSD form (or OSF1 form) of pthread_attr_setname_np(3) function.])
         HAVE_THREAD_NAME_FUNC="yes"
         AC_MSG_RESULT([[yes]])],
        [AC_MSG_RESULT([[no]])]
        )
    ])

  AS_IF([[test "x$HAVE_THREAD_NAME_FUNC" != "xyes" && test "x$ac_cv_have_decl_pthread_attr_setname_np" = "xyes"]],
    [AC_MSG_CHECKING([[for pthread_attr_setname_np(3) in IBM i or Solaris form]])
     AC_LINK_IFELSE(
      [AC_LANG_PROGRAM([[
#include <pthread.h>
#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
]], [[
      pthread_attr_t thr_attr;
      pthread_attr_init(&thr_attr);
      pthread_attr_setname_np(&thr_attr, "name");
      pthread_attr_destroy(&thr_attr);
        ]])],
        [AC_DEFINE([[HAVE_PTHREAD_ATTR_SETNAME_NP_IBMI]], [[1]], [Define if you have IBM i form (or Solaris form) of pthread_attr_setname_np(3) function.])
         HAVE_THREAD_NAME_FUNC="yes"
         AC_MSG_RESULT([[yes]])],
        [AC_MSG_RESULT([[no]])]
        )
    ])

  # Try to find how to set thread name for started thread - less convenient
  # than setting name by attributes.
  # If pthread_setname_np(3) is not declared, it's not possible to detect
  # form of pthread_setname_np(3) due to C "feature" "implicit declaration".
  AS_IF([[test "x$HAVE_THREAD_NAME_FUNC" != "xyes"]],
    [AC_CHECK_DECL([[pthread_setname_np]],[],[],[[
#include <pthread.h>
#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
       ]])
    ])

  AS_IF([[test "x$HAVE_THREAD_NAME_FUNC" != "xyes" && test "x$ac_cv_have_decl_pthread_setname_np" = "xyes"]],
    [AC_MSG_CHECKING([[for pthread_setname_np(3) in NetBSD or OSF1 form]])
     AC_LINK_IFELSE(
      [AC_LANG_PROGRAM([[
#include <pthread.h>
#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
]], [[int res = pthread_setname_np(pthread_self(), "name", 0); if (res) return res;]])],
        [AC_DEFINE([[HAVE_PTHREAD_SETNAME_NP_NETBSD]], [[1]], [Define if you have NetBSD form (or OSF1 form) of pthread_setname_np(3) function.])
         HAVE_THREAD_NAME_FUNC="yes"
         AC_MSG_RESULT([[yes]])],
        [AC_MSG_RESULT([[no]])]
        )
    ])

  AS_IF([[test "x$HAVE_THREAD_NAME_FUNC" != "xyes" && test "x$ac_cv_have_decl_pthread_setname_np" = "xyes"]],
    [AC_MSG_CHECKING([[for pthread_setname_np(3) in GNU/Linux form]])
     AC_LINK_IFELSE(
       [AC_LANG_PROGRAM([[
#include <pthread.h>
#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
]], [[int res = pthread_setname_np(pthread_self(), "name"); if (res) return res;]])],
        [AC_DEFINE([[HAVE_PTHREAD_SETNAME_NP_GNU]], [[1]], [Define if you have GNU/Linux form of pthread_setname_np(3) function.])
         HAVE_THREAD_NAME_FUNC="yes"
         AC_MSG_RESULT([[yes]])],
        [AC_MSG_RESULT([[no]])]
        )
    ])

  AS_IF([[test "x$HAVE_THREAD_NAME_FUNC" != "xyes" && test "x$ac_cv_have_decl_pthread_setname_np" = "xyes"]],
    [AC_MSG_CHECKING([[for pthread_setname_np(3) in Darwin form]])
     AC_LINK_IFELSE(
       [AC_LANG_PROGRAM([[
#include <pthread.h>
#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
]], [[int res = pthread_setname_np("name"); if (res) return res;]])],
        [AC_DEFINE([[HAVE_PTHREAD_SETNAME_NP_DARWIN]], [[1]], [Define if you have Darwin form of pthread_setname_np(3) function.])
         HAVE_THREAD_NAME_FUNC="yes"
         AC_MSG_RESULT([[yes]])],
        [AC_MSG_RESULT([[no]])]
        )
    ])

  AS_IF([[test "x$HAVE_THREAD_NAME_FUNC" != "xyes"]],
    [
     AC_CHECK_DECL([[pthread_set_name_np]],
       [
        AC_MSG_CHECKING([[for pthread_set_name_np(3) in FreeBSD form]])
        AC_LINK_IFELSE(
          [AC_LANG_PROGRAM([[
#include <pthread.h>
#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
]], [[pthread_set_name_np(pthread_self(), "name");]])],
          [AC_DEFINE([[HAVE_PTHREAD_SET_NAME_NP_FREEBSD]], [[1]], [Define if you have FreeBSD form of pthread_set_name_np(3) function.])
           HAVE_THREAD_NAME_FUNC="yes"
           AC_MSG_RESULT([[yes]])],
          [AC_MSG_RESULT([[no]])]
          )
       ],[],[[
#include <pthread.h>
#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
       ]]
     )
    ])

  LIBS="$SAVE_LIBS"
  CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
])

AS_IF([[test "x$enable_thread_names" != "xno"]],
  [
    AC_MSG_CHECKING([[whether to enable thread names]])
    AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM([], [[
#ifdef MHD_NO_THREAD_NAMES
#error Thread names are disabled.
fail test here %%%@<:@-1@:>@
#endif

/* Keep in sync with mhd_threads.h */
#if defined(mhd_THREADS_KIND_POSIX) && (defined(HAVE_PTHREAD_ATTR_SETNAME_NP_NETBSD) || defined(HAVE_PTHREAD_ATTR_SETNAME_NP_IBMI) || \
    defined(HAVE_PTHREAD_SETNAME_NP_GNU) || defined(HAVE_PTHREAD_SET_NAME_NP_FREEBSD) || defined(HAVE_PTHREAD_SETNAME_NP_DARWIN) || \
    defined(HAVE_PTHREAD_SETNAME_NP_NETBSD) )
(void) 0; /* no-op */
#elif defined(mhd_THREADS_KIND_W32) && defined(_MSC_FULL_VER)
(void) 0; /* no-op */
#else
#error No thread name function is available.
fail test here %%%@<:@-1@:>@
#endif
       ]])
     ], [
       enable_thread_names='yes'
     ], [
       AS_IF([[test "x$enable_thread_names" = "xyes"]],
         [
           AC_MSG_RESULT([[no]])
           AC_MSG_ERROR([[thread names was explicitly requested, but thread name function is not available]])
         ])
       enable_thread_names='no'
     ])
  AC_MSG_RESULT([[$enable_thread_names]])
  ])

AS_IF([[test "x$enable_thread_names" = "xno"]],
  [AC_DEFINE([[MHD_NO_THREAD_NAMES]], [[1]], [Define to 1 to disable setting name on generated threads])])

AM_CONDITIONAL(HAVE_W32, [test "x$os_is_native_w32" = "xyes"])
AM_CONDITIONAL(MHD_SOCKETS_KIND_POSIX, [test "x$os_is_native_w32" != "xyes"])
AM_CONDITIONAL(MHD_SOCKETS_KIND_WINSOCK, [test "x$os_is_native_w32" = "xyes"])
w32_shared_lib_exp=no
AS_IF([test "x$enable_shared" = "xyes" && test "x$os_is_native_w32" = "xyes"],
  [
    AS_IF([test "x$ac_cv_use_ms_lib_tool" = "xyes" || test -n "$DLLTOOL"],
      [
        w32_shared_lib_exp=yes
        use_expfile="no"
        AS_VAR_IF([ac_cv_use_ms_lib_tool], ["yes"], [use_expfile="yes"],
          [
            AC_CACHE_CHECK([whether $DLLTOOL supports export file generation], [mhd_cv_dlltool_expfile],
              [
                AS_IF([AC_RUN_LOG([$DLLTOOL -e conftest.exp >&2 ])],
                  [
                    AS_IF([test -f conftest.exp], [mhd_cv_dlltool_expfile="yes"], [mhd_cv_dlltool_expfile="no"])
                  ], [mhd_cv_dlltool_expfile="no"]
                )
                rm -f conftest.exp
              ]
            )
            use_expfile="${mhd_cv_dlltool_expfile}"
          ]
        )
      ],
      [
        AC_MSG_WARN([[GNU dlltool or MS lib.exe is required for creating shared library export on W32]])
        AC_MSG_WARN([[Export library libmicrohttpd.lib will not be created]])
      ]
    )
  ]
)
AM_CONDITIONAL([W32_SHARED_LIB_EXP], [test "x$w32_shared_lib_exp" = "xyes"])
AM_CONDITIONAL([USE_MS_LIB_TOOL], [test "x$ac_cv_use_ms_lib_tool" = "xyes"])
AM_CONDITIONAL([USE_EXPORT_FILE], [test "x$use_expfile" = "xyes"])

dnl gethostbyname() is not really needed
dnl AC_SEARCH_LIBS([gethostbyname], [nsl])
MHD_FIND_LIB([socket],
  [[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#elif defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#ifdef HAVE_SOCKLIB_H
#include <sockLib.h>
#endif
#if defined(_WIN32) && ! defined(__CYGWIN__)
#include <winsock2.h>
#endif
  ]],
  [(void)socket(0, 0, 0);],
  [socket ws2_32 xnet network],
  [
    AS_VAR_IF([[mhd_cv_find_lib_socket]],[["none required"]], [],
      [
       MHD_LIBDEPS_PKGCFG="${mhd_cv_find_lib_socket} $MHD_LIBDEPS_PKGCFG"
      ]
    )
  ],
  [AC_MSG_ERROR([[cannot find header or library required for function socket()]])]
)

MHD_CHECK_SOCKET_SHUTDOWN_TRIGGER([AC_DEFINE([HAVE_LISTEN_SHUTDOWN],[1],[can use shutdown on listen sockets])])
AM_CONDITIONAL([HAVE_LISTEN_SHUTDOWN], [test "x$mhd_cv_host_shtdwn_trgr_select" = "xyes"])

AC_CACHE_CHECK([fo][r system default FD_SETSIZE value],[mhd_cv_sys_fd_setsize_value],
  [
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS} -UFD_SETSIZE"
    CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS} -UFD_SETSIZE"
    AC_COMPUTE_INT([mhd_cv_sys_fd_setsize_value],[FD_SETSIZE],dnl
      [[
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SOCKLIB_H
#include <sockLib.h>
#endif
#if defined(_WIN32) && ! defined(__CYGWIN__)
#include <winsock2.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
      ]], [mhd_cv_sys_fd_setsize_value="unknown"]
    )
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
    CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
    AS_IF([test "x${mhd_cv_sys_fd_setsize_value}" != "xunknown" && \
      test "${mhd_cv_sys_fd_setsize_value}" -eq "0" \
      ], [mhd_cv_sys_fd_setsize_value="unknown"]
    )
  ]
)
AS_VAR_IF([mhd_cv_sys_fd_setsize_value],["unknown"],[:],
  [
    AC_DEFINE_UNQUOTED([MHD_SYS_FD_SETSIZE_],[${mhd_cv_sys_fd_setsize_value}],
      [Define to system default value of FD_SETSIZE macro]
    )
  ]
)
AM_CONDITIONAL([NEED_SYS_FD_SET_SIZE_VALUE],[test "x${mhd_cv_sys_fd_setsize_value}" = "xunknown"])

AC_CACHE_CHECK([fo][r current FD_SETSIZE value],[mhd_cv_fd_setsize_value],
  [
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
    CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
    AS_IF([test "x${cross_compiling}" != "xno" && test "x${mhd_cv_sys_fd_setsize_value}" != "xunknown"],
      [
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
              [[
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SOCKLIB_H
#include <sockLib.h>
#endif
#if defined(_WIN32) && ! defined(__CYGWIN__)
#include <winsock2.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#define DETECTED_FD_SETZIE_VALUE ${mhd_cv_sys_fd_setsize_value}

#if (FD_SETSIZE) != (DETECTED_FD_SETZIE_VALUE)
#error The current default FD_SETSIZE value is different from ${mhd_cv_sys_fd_setsize_value}
fail test here %%%@<:@-1@:>@
#endif /* (FD_SETSIZE) != (DETECTED_FD_SETZIE_VALUE) */

#if (FD_SETSIZE) != (${mhd_cv_sys_fd_setsize_value})
#error The current default FD_SETSIZE value is different from ${mhd_cv_sys_fd_setsize_value}
fail test here %%%@<:@-1@:>@
#endif /* (FD_SETSIZE) != (${mhd_cv_sys_fd_setsize_value}) */
              ]],[]
            )
          ],
          [mhd_cv_fd_setsize_value="${mhd_cv_sys_fd_setsize_value}"]
        )
      ]
    )
    AS_VAR_SET_IF([mhd_cv_fd_setsize_value],[:],
      [
        AC_COMPUTE_INT([mhd_cv_fd_setsize_value],[FD_SETSIZE],dnl
          [[
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SOCKLIB_H
#include <sockLib.h>
#endif
#if defined(_WIN32) && ! defined(__CYGWIN__)
#include <winsock2.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
          ]], [mhd_cv_fd_setsize_value="unknown"]
        )
      ]
    )
    AS_IF([test "x${mhd_cv_fd_setsize_value}" != "xunknown" && \
      test "${mhd_cv_fd_setsize_value}" -eq "0"],
      [mhd_cv_fd_setsize_value="unknown"]
    )
  ]
)


AC_CACHE_CHECK([whether FD_SETSIZE is overridable],[mhd_cv_fd_setsize_overridable],
  [
    AS_VAR_IF([mhd_cv_fd_setsize_value],["unknown"],
      [
        AS_VAR_IF([mhd_cv_sys_fd_setsize_value],["unknown"],
          [
            # Assume the most popular FD_SETSIZE value
            base_fd_setsize=1024
          ],
          [base_fd_setsize="${mhd_cv_sys_fd_setsize_value}"]
        )
        base_min_fd_setsize="${base_fd_setsize}"
      ],
      [
        AS_VAR_IF([mhd_cv_sys_fd_setsize_value],["unknown"],
          [
            base_fd_setsize="${mhd_cv_fd_setsize_value}"
            base_min_fd_setsize="${base_fd_setsize}"
          ],
          [
            AS_IF([test "${mhd_cv_fd_setsize_value}" -ge "${mhd_cv_sys_fd_setsize_value}"],
              [
                base_fd_setsize="${mhd_cv_fd_setsize_value}"
                base_min_fd_setsize="${mhd_cv_sys_fd_setsize_value}"
              ],
              [
                base_fd_setsize="${mhd_cv_sys_fd_setsize_value}"
                base_min_fd_setsize="${mhd_cv_fd_setsize_value}"
              ]
            )
          ]
        )
      ]
    )
    # Use two times larger FD_SETSIZE value for test
    AS_VAR_ARITH([test_fd_setsize],[${base_fd_setsize} '+' ${base_fd_setsize}])
    AS_IF([test "${test_fd_setsize}" -lt "128"],[test_fd_setsize=128])
    test_type_fd_setsize="larger"

    CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
    CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"

    _AS_ECHO_LOG([testing whether FD_SETSIZE test value (${test_fd_setsize}) is suitable for 'int' type])
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],
          [[
            static int test_arr1[(((long long)${test_fd_setsize}) != ((int)${test_fd_setsize})) ? -1 : 1];
            static int test_arr2[(0 > ((int)${test_fd_setsize})) ? -1 : 1];
            test_arr1[0] = 0;
            test_arr2[0] = 0;
            i][f (test_arr1[0] != test_arr2[0]) return 1;
          ]]
        )
      ],[],
      [
        _AS_ECHO_LOG([The selected test FD_SETSIZE value (${test_fd_setsize}) is too large for 'int' type])
        AS_VAR_ARITH([test_fd_setsize],[${base_min_fd_setsize} '/' '2'])
        test_type_fd_setsize="smaller"
      ]
    )

    _AS_ECHO_LOG([will try ${test_fd_setsize} as FD_SETSIZE value])

    AS_IF([test "x${cross_compiling}" = "xno" && test "x${test_type_fd_setsize}" = "xlarger"],
      [
        _AS_ECHO_LOG([will try ${test_fd_setsize} as FD_SETSIZE value with run test])
        AC_RUN_IFELSE(
          [
            AC_LANG_SOURCE(
              [[
#ifdef FD_SETSIZE
#undef FD_SETSIZE
#endif /* FD_SETSIZE */

#define FD_SETSIZE          ${test_fd_setsize}
#define MY_TEST_FD_SETSIZE  ${test_fd_setsize}

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SOCKLIB_H
#include <sockLib.h>
#endif
#if defined(_WIN32) && ! defined(__CYGWIN__)
#include <winsock2.h>
#define MHD_TEST_SOCKET_TYPE SOCKET
#else
#define MHD_TEST_SOCKET_TYPE int
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <string.h>
#include <stdio.h>

/* Test fo][r mismatch by macro check */
#if (FD_SETSIZE) != (MY_TEST_FD_SETSIZE)
#error System headers ignore custom FD_SETSIZE value, FD_SETSIZE is NOT ${test_fd_setsize}
fail test here %%%@<:@-1@:>@
#endif /* (FD_SETSIZE) != (MY_TEST_FD_SETSIZE) */

/* Additional test fo][r mismatch by macro check */
#if (FD_SETSIZE) != (${test_fd_setsize})
#error System headers ignore custom FD_SETSIZE value, FD_SETSIZE is NOT ${test_fd_setsize}
fail test here %%%@<:@-1@:>@
#endif /* (FD_SETSIZE) != (${test_fd_setsize}) */

static unsigned long var_fd_setsize_value = FD_SETSIZE;
static unsigned long var_my_test_fd_setsize_value = MY_TEST_FD_SETSIZE;

int main(void)
{
  fd_set fds_num_one_copy;
  fd_set fds_num_three_copy;
  fd_set test_fdsets[6];
  int i;
  int ret = 0;
#if defined(_WIN32) && ! defined(__CYGWIN__)
  WSADATA wsa_data;

  if (0 != WSAStartup(MAKEWORD(2, 2), &wsa_data))
    return 21;
  if (MAKEWORD(2, 2) != wsa_data.wVersion)
  {
    WSACleanup();
    return 22;
  }
#endif /* _WIN32 && ! __CYGWIN__ */

  /* Init with zeros to make sure that slack areas are equal */
  memset(test_fdsets, 0, sizeof(test_fdsets));

  /* Correctly init fd_sets */
  for (i = 0; i < 6; ++i)
    FD_ZERO(test_fdsets + i);

  /* Make copies of zero-inited fd_sets */
  memcpy(&fds_num_one_copy, test_fdsets + 1, sizeof(fd_set));
  memcpy(&fds_num_three_copy, test_fdsets + 3, sizeof(fd_set));

  if (var_fd_setsize_value != var_my_test_fd_setsize_value)
  {
    fprintf (stderr, "System headers redefined FD_SETSIZE to another value (%lu).\n",
             (unsigned long) FD_SETSIZE);
    ret = 2;
  }
  else
  {
    /* Set (almost) all FDs in test_fdset[2] */
    for (i = 1; i < FD_SETSIZE; ++i)
      FD_SET((MHD_TEST_SOCKET_TYPE)i, test_fdsets + 2);

    if (! FD_ISSET((MHD_TEST_SOCKET_TYPE)1, test_fdsets + 2))
    {
      fprintf (stderr, "FD number one in original fd_set is unset, while should be set.\n");
      ret |= 3;
    }
    if (! FD_ISSET((MHD_TEST_SOCKET_TYPE)(FD_SETSIZE - 1), test_fdsets + 2))
    {
      fprintf (stderr, "FD number %lu in original fd_set is unset, while should be set.\n",
               (unsigned long) FD_SETSIZE);
      ret |= 3;
    }

    if (FD_ISSET((MHD_TEST_SOCKET_TYPE)1, test_fdsets + 1))
    {
      fprintf (stderr, "FD number one in the first fd_set is unset, while should be set.\n");
      ret |= 4;
    }
    if (FD_ISSET((MHD_TEST_SOCKET_TYPE)(FD_SETSIZE - 1), test_fdsets + 1))
    {
      fprintf (stderr, "FD number %lu in the first fd_set is unset, while should be set.\n",
               (unsigned long) FD_SETSIZE);
      ret |= 4;
    }
    if (0 != memcmp (&fds_num_one_copy, test_fdsets + 1, sizeof(fd_set)))
    {
      fprintf (stderr, "The first fd_set has been altered.\n");
      ret |= 4;
    }

    if (FD_ISSET((MHD_TEST_SOCKET_TYPE)1, test_fdsets + 3))
    {
      fprintf (stderr, "FD number one in the third fd_set is unset, while should be set.\n");
      ret |= 8;
    }
    if (FD_ISSET((MHD_TEST_SOCKET_TYPE)(FD_SETSIZE - 1), test_fdsets + 3))
    {
      fprintf (stderr, "FD number %lu in the third fd_set is unset, while should be set.\n",
               (unsigned long) FD_SETSIZE);
      ret |= 8;
    }
    if (0 != memcmp (&fds_num_three_copy, test_fdsets + 3, sizeof(fd_set)))
    {
      fprintf (stderr, "The third fd_set has been altered.\n");
      ret |= 8;
    }
  }
#if defined(_WIN32) && ! defined(__CYGWIN__)
  WSACleanup();
#endif /* _WIN32 && ! __CYGWIN__ */
  return ret;
}
              ]]
            )
          ],
          [mhd_cv_fd_setsize_overridable="yes"],
          [mhd_cv_fd_setsize_overridable="no"],
          [[# Not used when cross-compiling ]]
        )
      ],
      [
        _AS_ECHO_LOG([will try ${test_fd_setsize} as FD_SETSIZE with simple compile test])
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
              [[
#ifdef FD_SETSIZE
#undef FD_SETSIZE
#endif /* FD_SETSIZE */

#define FD_SETSIZE          ${test_fd_setsize}
#define MY_TEST_FD_SETSIZE  ${test_fd_setsize}

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SOCKLIB_H
#include <sockLib.h>
#endif
#if defined(_WIN32) && ! defined(__CYGWIN__)
#include <winsock2.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

/* Test fo][r mismatch by macro check */
#if (FD_SETSIZE) != (MY_TEST_FD_SETSIZE)
#error System headers ignore custom FD_SETSIZE value, FD_SETSIZE is NOT ${test_fd_setsize}
fail test here %%%@<:@-1@:>@
#endif /* (FD_SETSIZE) != (MY_TEST_FD_SETSIZE) */

/* Additional test fo][r mismatch by macro check */
#if (FD_SETSIZE) != (${test_fd_setsize})
#error System headers ignore custom FD_SETSIZE value, FD_SETSIZE is NOT ${test_fd_setsize}
fail test here %%%@<:@-1@:>@
#endif /* (FD_SETSIZE) != (${test_fd_setsize}) */
              ]],[]
            )
          ],
          [
            _AS_ECHO_LOG([comple test succeed, will check whether another FD_SETSIZE value changes the size of 'fd_set'])

            # Check current size of fd_set
            _AS_ECHO_LOG([find the sizeof(fd_setsize) with current default (${mhd_cv_fd_setsize_value}) FD_SETSIZE value])
            AC_COMPUTE_INT([sizeof_cur_fd_set],[sizeof(fd_set)],dnl
              [[
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SOCKLIB_H
#include <sockLib.h>
#endif
#if defined(_WIN32) && ! defined(__CYGWIN__)
#include <winsock2.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
              ]], [sizeof_cur_fd_set="unknown"]
            )
            _AS_ECHO_LOG([the sizeof(fd_setsize) with current default (${mhd_cv_fd_setsize_value}) FD_SETSIZE value is ${sizeof_cur_fd_set}])

            # Check the size of fd_set with redefined FD_SETSIZE
            _AS_ECHO_LOG([find the sizeof(fd_setsize) with redefined (${test_fd_setsize}) FD_SETSIZE value])
            AC_COMPUTE_INT([sizeof_mod_fd_set],[sizeof(fd_set)],dnl
              [[
#ifdef FD_SETSIZE
#undef FD_SETSIZE
#endif /* FD_SETSIZE */

#define FD_SETSIZE          ${test_fd_setsize}

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SOCKLIB_H
#include <sockLib.h>
#endif
#if defined(_WIN32) && ! defined(__CYGWIN__)
#include <winsock2.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
              ]], [sizeof_mod_fd_set="unknown"]
            )
            _AS_ECHO_LOG([the sizeof(fd_setsize) with redefined (${test_fd_setsize}) FD_SETSIZE value is ${sizeof_mod_fd_set}])
            _AS_ECHO_LOG([detected sizes of 'fd_set': '${sizeof_cur_fd_set}' (by default), '${sizeof_mod_fd_set}' with modified FD_SETSIZE])
            AS_IF([test "x${sizeof_cur_fd_set}" != "x${sizeof_mod_fd_set}"],
              [mhd_cv_fd_setsize_overridable="yes"],
              [mhd_cv_fd_setsize_overridable="no"]
            )
            AS_UNSET([sizeof_mod_fd_set])
            AS_UNSET([sizeof_cur_fd_set])
          ],
          [mhd_cv_fd_setsize_overridable="no"]
        )
      ]
    )
    AS_UNSET([test_type_fd_setsize])
    AS_UNSET([test_fd_setsize])
    AS_UNSET([base_min_fd_setsize])
    AS_UNSET([base_fd_setsize])
  ]
)

AS_VAR_IF([mhd_cv_fd_setsize_overridable],["no"],
  [
    AS_VAR_IF([os_is_native_w32],["yes"],
      [AC_MSG_ERROR([Non-overridable FD_SETSIZE detected for native W32 build. FD_SETSIZE is overridable on W32.])]
    )
    AS_IF([test "x${mhd_cv_fd_setsize_value}" != "xunknown" && test "x${mhd_cv_sys_fd_setsize_value}" != "xunknown" && test "${mhd_cv_fd_setsize_value}" -ne "${mhd_cv_sys_fd_setsize_value}"],
      [AC_MSG_WARN([Detected non-overridable FD_SETSIZE, but the toolchain uses FD_SETSIZE (${mhd_cv_fd_setsize_value}) different from system default (${mhd_cv_sys_fd_setsize_value})])]
    )
  ],
  [AC_DEFINE([HAS_FD_SETSIZE_OVERRIDABLE],[1],[Define to 1 i][f your system allow overriding the value of FD_SETSIZE macro])]
)

AC_CACHE_CHECK([whether socket value is a signed type],[mhd_cv_socket_signed],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SOCKLIB_H
#include <sockLib.h>
#endif
#if defined(_WIN32) && ! defined(__CYGWIN__)
#include <winsock2.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

/* Keep in sync with microhttpd.h */
#if ! defined(_WIN32) || defined(_SYS_TYPES_FD_SET)
typedef int MHD_Socket;
#else /* defined(_WIN32) && ! defined(_SYS_TYPES_FD_SET) */
typedef SOCKET MHD_Socket;
#endif /* defined(_WIN32) && ! defined(_SYS_TYPES_FD_SET) */

int main(void)
{
  int test_arr[2 - 5*(!!(0 < ((MHD_Socket)-1)))];
  test_arr[1] = 0;
  return test_arr[1];
}
          ]]
        )
      ],
      [mhd_cv_socket_signed="yes"],
      [mhd_cv_socket_signed="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_socket_signed],["yes"],
  [AC_DEFINE([HAS_SIGNED_SOCKET],[1],[Define to 1 i][f your socket type is signed])]
)

MHD_CHECK_FUNC([writev],
  [[#include <sys/uio.h>]],
  [[
    struct iovec iov[2];
    char some_str[4] = "OK\n";
    iov[0].iov_base = (void *) some_str;
    iov[0].iov_len = 3;
    i][f (0 > writev(1, iov, 1))
      return 2;
  ]]
)
MHD_FIND_LIB([sendmsg],
  [[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#elif defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#ifdef HAVE_SOCKLIB_H
#include <sockLib.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
  ]],
  [[
    struct msghdr msg;
    struct iovec iov;
    unsigned int i;
    char some_str[5] = "test";

    iov.iov_base = (void*)some_str;
    iov.iov_len = 4;

    for (i = 0; i < (unsigned int) sizeof(msg); i++)
    {
      *(((unsigned char *)&msg) + i) = 0;
    }
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    i][f (0 > sendmsg(1, &msg, 0))
      return -1;
  ]],
  [socket],
  [
    AC_DEFINE([HAVE_SENDMSG],[1],[Define to '1' if your have sendmsg() function])
    AS_VAR_IF([[mhd_cv_find_lib_sendmsg]],[["none required"]], [],
      [
        MHD_LIBDEPS_PKGCFG="${mhd_cv_find_lib_sendmsg} $MHD_LIBDEPS_PKGCFG"
      ]
    )
  ],[],
  [MHD_LIBDEPS]
)

AC_C_BIGENDIAN
AC_C_VARARRAYS

AC_CACHE_CHECK([[whether __func__ magic-macro is available]],
  [[mhd_cv_macro___func___avail]], [dnl
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],[[const char *funcname = __func__ ; if (!funcname) return 1;]])],
    [[mhd_cv_macro___func___avail="yes"]],[[mhd_cv_macro___func___avail="no"]])
])
AS_VAR_IF([mhd_cv_macro___func___avail], ["yes"],
  [AC_DEFINE([HAVE___FUNC__], [1], [Define to 1 if your compiler supports __func__ magic-macro.])],
  [
    AC_CACHE_CHECK([[whether __FUNCTION__ magic-macro is available]],
      [[mhd_cv_macro___function___avail]], [dnl
      AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],[[const char *funcname = __FUNCTION__ ; if (!funcname) return 1;]])],
        [[mhd_cv_macro___function___avail="yes"]],[[mhd_cv_macro___function___avail="no"]])
    ])
    AS_VAR_IF([mhd_cv_macro___function___avail], ["yes"],
      [AC_DEFINE([HAVE___FUNCTION__], [1], [Define to 1 if your compiler supports __FUNCTION__ magic-macro.])],
      [
        AC_CACHE_CHECK([[whether __PRETTY_FUNCTION__ magic-macro is available]],
          [[mhd_cv_macro___pretty_function___avail]], [dnl
          AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],[[const char *funcname = __PRETTY_FUNCTION__ ; if (!funcname) return 1;]])],
            [[mhd_cv_macro___pretty_function___avail="yes"]],[[mhd_cv_macro___pretty_function___avail="no"]])
        ])
        AS_VAR_IF([mhd_cv_macro___pretty_function___avail], ["yes"],
          [AC_DEFINE([HAVE___PRETTY_FUNCTION__], [1], [Define to 1 if your compiler supports __PRETTY_FUNCTION__ magic-macro.])],
        )
      ]
    )
  ]
)
AC_CACHE_CHECK([[whether __builtin_bswap16() is available]],
  [[mhd_cv_func___builtin_bswap16_avail]], [dnl
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include<stdint.h>]], [[uint_least16_t a = 1; uint_least16_t b = __builtin_bswap16(a); a = b; (void) a;]])],
    [[mhd_cv_func___builtin_bswap16_avail="yes"]],[[mhd_cv_func___builtin_bswap16_avail="no"]])
])
AS_IF([[test "x$mhd_cv_func___builtin_bswap16_avail" = "xyes"]],
  [AC_DEFINE([[MHD_HAVE___BUILTIN_BSWAP16]], [[1]], [Define to 1 if you have __builtin_bswap16() builtin function])])

AC_CACHE_CHECK([[whether __builtin_bswap32() is available]],
  [[mhd_cv_func___builtin_bswap32_avail]], [dnl
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include<stdint.h>]], [[uint_least32_t a = 1; uint_least32_t b = __builtin_bswap32(a); a = b; (void) a;]])],
    [[mhd_cv_func___builtin_bswap32_avail="yes"]],[[mhd_cv_func___builtin_bswap32_avail="no"]])
])
AS_IF([[test "x$mhd_cv_func___builtin_bswap32_avail" = "xyes"]],
  [AC_DEFINE([[MHD_HAVE___BUILTIN_BSWAP32]], [[1]], [Define to 1 if you have __builtin_bswap32() builtin function])])
AC_CACHE_CHECK([[whether __builtin_bswap64() is available]],
  [[mhd_cv_func___builtin_bswap64_avail]], [dnl
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include<stdint.h>]], [[uint_least64_t a = 1; uint_least64_t b = __builtin_bswap64(a); a = b; (void) a;]])],
    [[mhd_cv_func___builtin_bswap64_avail="yes"]], [[mhd_cv_func___builtin_bswap64_avail="no"]])
])
AS_IF([[test "x$mhd_cv_func___builtin_bswap64_avail" = "xyes"]],
  [AC_DEFINE([[MHD_HAVE___BUILTIN_BSWAP64]], [[1]], [Define to 1 if you have __builtin_bswap64() builtin function])])

AC_CACHE_CHECK([[whether __builtin_clz() is available]],
  [[mhd_cv_func___builtin_clz_avail]], [dnl
     AC_LINK_IFELSE([AC_LANG_SOURCE([[
int main(void)
{
  unsigned int val = 2u;
  return __builtin_clz(val) > 8 ? 0 : 1;
}
         ]]
        )
      ],
      [[mhd_cv_func___builtin_clz_avail="yes"]],
      [[mhd_cv_func___builtin_clz_avail="no"]]
    )
  ]
)
AS_VAR_IF([mhd_cv_func___builtin_clz_avail],["yes"],
  [AC_DEFINE([[MHD_HAVE___BUILTIN_CLZ]], [[1]], [Define to 1 if __builtin_clz() builtin function is supported by compiler])]
)
AC_CACHE_CHECK([[whether __builtin_clzl() is available]],
  [[mhd_cv_func___builtin_clzl_avail]], [dnl
     AC_LINK_IFELSE([AC_LANG_SOURCE([[
int main(void)
{
  unsigned long val = 2u;
  return __builtin_clzl(val) > 8 ? 0 : 1;
}
         ]]
        )
      ],
      [[mhd_cv_func___builtin_clzl_avail="yes"]],
      [[mhd_cv_func___builtin_clzl_avail="no"]]
    )
  ]
)
AS_VAR_IF([mhd_cv_func___builtin_clzl_avail],["yes"],
  [AC_DEFINE([[MHD_HAVE___BUILTIN_CLZL]], [[1]], [Define to 1 if __builtin_clzl() builtin function is supported by compiler])]
)
AC_CACHE_CHECK([[whether __builtin_clzll() is available]],
  [[mhd_cv_func___builtin_clzll_avail]], [dnl
     AC_LINK_IFELSE([AC_LANG_SOURCE([[
int main(void)
{
  unsigned long long val = 2u;
  return __builtin_clzl(val) > 8 ? 0 : 1;
}
         ]]
        )
      ],
      [[mhd_cv_func___builtin_clzll_avail="yes"]],
      [[mhd_cv_func___builtin_clzll_avail="no"]]
    )
  ]
)
AS_VAR_IF([mhd_cv_func___builtin_clzll_avail],["yes"],
  [AC_DEFINE([[MHD_HAVE___BUILTIN_CLZLL]], [[1]], [Define to 1 if __builtin_clzll() builtin function is supported by compiler])]
)
AC_CACHE_CHECK([[whether __builtin_clzg(v) is available]],
  [[mhd_cv_func___builtin_clzg1_avail]], [dnl
     AC_LINK_IFELSE([AC_LANG_SOURCE([[
int main(void)
{
  unsigned int val = 2u;
  return __builtin_clzg(val) > 8 ? 0 : 1;
}
         ]]
        )
      ],
      [[mhd_cv_func___builtin_clzg1_avail="yes"]],
      [[mhd_cv_func___builtin_clzg1_avail="no"]]
    )
  ]
)
AS_VAR_IF([mhd_cv_func___builtin_clzg1_avail],["yes"],
  [AC_DEFINE([[MHD_HAVE___BUILTIN_CLZG1]], [[1]], [Define to 1 if __builtin_clzg(v) builtin function is supported by compiler])]
)
AC_CACHE_CHECK([[whether __builtin_clzg(v,z) is available]],
  [[mhd_cv_func___builtin_clzg2_avail]], [dnl
     AC_LINK_IFELSE([AC_LANG_SOURCE([[
int main(void)
{
  unsigned int val = 0u;
  return __builtin_clzg(val, 0);
}
         ]]
        )
      ],
      [[mhd_cv_func___builtin_clzg2_avail="yes"]],
      [[mhd_cv_func___builtin_clzg2_avail="no"]]
    )
  ]
)
AS_VAR_IF([mhd_cv_func___builtin_clzg2_avail],["yes"],
  [AC_DEFINE([[MHD_HAVE___BUILTIN_CLZG2]], [[1]], [Define to 1 if __builtin_clzg(v,z) builtin function is supported by compiler])]
)

AC_CACHE_CHECK([whether $CC supports __builtin_expect_with_probability()],[mhd_cv_cc___builtin_expect_with_probability_avail],
  [
    AC_LINK_IFELSE(
      [
        AC_LANG_SOURCE(
          [[
int main(int argc, char *argv[])
{
  (void) argv;
  if (__builtin_expect_with_probability(argc < 1, 0, 0.9999)) return 1;
  return 0;
}
          ]]
        )
      ],
      [[mhd_cv_cc___builtin_expect_with_probability_avail="yes"]],
      [[mhd_cv_cc___builtin_expect_with_probability_avail="no"]]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc___builtin_expect_with_probability_avail],["yes"],
  [AC_DEFINE([[MHD_HAVE___BUILTIN_EXPECT_WITH_PROBABILITY]], [[1]], [Define to 1 if compiler supports __builtin_expect_with_probability() builtin function])],
  [
    AC_CACHE_CHECK([whether $CC supports __builtin_expect()],[mhd_cv_cc___builtin_expect_avail],
      [
        AC_LINK_IFELSE(
          [
            AC_LANG_SOURCE(
              [[
int main(int argc, char *argv[])
{
  (void) argv;
  if (__builtin_expect(argc < 1, 0)) return 1;
  return 0;
}
              ]]
            )
          ],
          [[mhd_cv_cc___builtin_expect_avail="yes"]],
          [[mhd_cv_cc___builtin_expect_avail="no"]]
        )
      ]
    )
    AS_VAR_IF([mhd_cv_cc___builtin_expect_avail],["yes"],
      [AC_DEFINE([[MHD_HAVE___BUILTIN_EXPECT]], [[1]], [Define to 1 if compiler supports __builtin_expect() builtin function])]
    )
  ]
)

AC_CHECK_PROG([HAVE_CURL_BINARY],[curl],[yes],[no])
AM_CONDITIONAL([HAVE_CURL_BINARY],[test "x$HAVE_CURL_BINARY" = "xyes"])
AC_CHECK_PROG([HAVE_MAKEINFO_BINARY],[makeinfo],[yes],[no])
AM_CONDITIONAL([HAVE_MAKEINFO_BINARY],[test "x$HAVE_MAKEINFO_BINARY" = "xyes"])
AM_CONDITIONAL(W32_STATIC_LIB, [test "x$os_is_native_w32" = "xyes"  && test "x$enable_static" = "xyes"])


AC_ARG_ENABLE([[doc]],
  [AS_HELP_STRING([[--disable-doc]], [do not build any documentation])], ,
    [enable_doc=yes])
test "x$enable_doc" = "xno" || enable_doc=yes
AM_CONDITIONAL([BUILD_DOC], [test "x$enable_doc" = "xyes"])

AC_ARG_ENABLE([[examples]],
  [AS_HELP_STRING([[--disable-examples]], [do not build any examples])], ,
    [enable_examples=yes])
test "x$enable_examples" = "xno" || enable_examples=yes
AM_CONDITIONAL([BUILD_EXAMPLES], [test "x$enable_examples" = "xyes"])

AC_ARG_ENABLE([[tools]],
  [AS_HELP_STRING([[--disable-tools]], [do not build testing and demo tools])], ,
    [enable_tools=yes])
test "x$enable_tools" = "xyes" || enable_tools=no
AM_CONDITIONAL([BUILD_TOOLS], [test "x$enable_tools" = "xyes"])

AC_ARG_ENABLE([[heavy-tests]],
  [AS_HELP_STRING([[--enable-heavy-tests[=SCOPE]]], [use SCOPE of heavy tests in test-suite. WARNING:]
  [a dedicated host with minimal number of background processes and no network]
  [activity is recommended to enable. (basic, full)])], [],
    [enable_heavy_tests=no])
use_heavy_tests="no"
use_vheavy_tests="no"
use_heavy_tests_MSG="no"
AS_CASE([${enable_heavy_tests}],
  [yes|basic],
  [
  	enable_heavy_tests="basic"
    use_heavy_tests="yes"
    use_vheavy_tests="no"
    use_heavy_tests_MSG="yes, basic heavy tests (a dedicated host is recommended)"
  ],
  [all|full],
  [
  	enable_heavy_tests="full"
    use_heavy_tests="yes"
    use_vheavy_tests="yes"
    use_heavy_tests_MSG="yes, full set of heavy tests (a dedicated host is recommended)"
  ],
  [no],
  [
    use_heavy_tests="no"
    use_vheavy_tests="no"
    use_heavy_tests_MSG="no"
  ],
  [AC_MSG_ERROR([[Unknown parameter value: --enable-heavy-tests=${enable_heavy_tests}]])]
)
AC_ARG_ENABLE([[slow-tests]],
  [
    AS_HELP_STRING([[--enable-slow-tests]],
      [enable slow (time and/or CPU consuming) tests]
    )
  ],
  [
    AS_CASE([$enableval],[yes|no],[:],
      [AC_MSG_ERROR([Unrecognised parameter value: --enable-slow-tests=$enableval])]
    )
  ],[enable_slow_tests="no"]
)
AS_VAR_IF([use_heavy_tests], ["yes"],
  [
    HEAVY_TESTS_NOTPARALLEL='.NOTPARALLEL:'
    AC_DEFINE([MHD_ENABLE_HEAVY_TESTS], [1], [Define to 1 to enable "heavy" test paths.])
    AS_VAR_IF([use_vheavy_tests], ["yes"],
      [AC_DEFINE([MHD_ENABLE_VHEAVY_TESTS], [1], [Define to 1 to enable "very heavy" test paths.])]
    )
  ],
  [
    HEAVY_TESTS_NOTPARALLEL=" "
  ]
)
AS_VAR_IF([enable_slow_tests], ["yes"], [AC_DEFINE([MHD_ENABLE_SLOW_TESTS], [1], [Define to 1 to enable long-running and CPU-intensive test paths.])])
AM_CONDITIONAL([SLOW_TESTS],[test "x$enable_slow_tests" = "xyes"])
AM_CONDITIONAL([HEAVY_TESTS],[test "x$use_heavy_tests" = "xyes"])
AM_CONDITIONAL([VHEAVY_TESTS],[test "x$use_vheavy_tests" = "xyes"])
AM_CONDITIONAL([TESTS_STRESS_OS],[false])

AC_ARG_ENABLE([[select]],
  [AS_HELP_STRING([[--enable-select[=ARG]]], [enable 'select()' support (yes, no, auto) [auto]])],
  [],[enable_select='auto']
)
AC_CACHE_CHECK([for select() function],[mhd_cv_func_select],
  [
    AS_IF([test "x$os_is_native_w32" = "xyes"],
      [mhd_cv_func_select="yes"],
      [
        AC_LINK_IFELSE(
          [
            AC_LANG_SOURCE([[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_SELECTLIB_H
#include <selectLib.h>
#endif

int main(void)
{
  int res;
  fd_set rfds;
  fd_set wfds;
  fd_set efds;
  struct timeval tv_to;

  tv_to.tv_sec = 1;
  tv_to.tv_usec = 0;
  FD_ZERO(&rfds);
  FD_ZERO(&wfds);
  FD_ZERO(&efds);
  FD_SET(1, &wfds);

  res = select(2, &rfds, &wfds, &efds, &tv_to);
  if (res < 0)
    return 3;

  FD_CLR(1, &efds);
  if (FD_ISSET(1, &efds))
    return 4;

  return 0;
}
              ]]
            )
          ],
          [mhd_cv_func_select="yes"],
          [mhd_cv_func_select="no"]
        )
      ]
    )
  ]
)
AS_VAR_IF([mhd_cv_func_select],["yes"],
  [
    AC_DEFINE([[HAVE_SELECT]],[[1]],[Define to '1' if select() is supported on your platform])
    AS_IF([test "x$enable_select" != "xno"],
      [
        enable_select="yes"
        AC_DEFINE([[MHD_SUPPORT_SELECT]],[[1]],[Define to '1' to enable use of select() system call])
      ]
    )
  ],
  [
    AS_VAR_IF([enable_select],["yes"],
      [AC_MSG_ERROR([Support for select() was explicitly requested but cannot be enabled on this platform.])]
    )
  ]
)
AM_CONDITIONAL([MHD_SUPPORT_SELECT],[test "x${enable_select}" = "xyes"])

AC_ARG_ENABLE([[poll]],
  [AS_HELP_STRING([[--enable-poll[=ARG]]], [enable 'poll()' support (yes, no, auto) [auto]])],
  [],[enable_poll='auto']
)
AS_IF([test "$os_is_native_w32" != "yes"],
  [
    AC_CHECK_HEADERS([poll.h],
      [
        MHD_CHECK_FUNC([poll],
          [[
#include <poll.h>
          ]],
          [[
  struct pollfd fds[2];

  fds[0].fd = 0;
  fds[0].events = POLLIN;
  if (0 > poll(fds, 1, 0))
    return 2;
          ]],
          [have_poll='yes'],[have_poll='no']
        )
      ],[],[AC_INCLUDES_DEFAULT]
    )
  ],
  [
    MHD_CHECK_FUNC([WSAPoll],
      [[
#include <winsock2.h>
      ]],
      [[
  WSAPOLLFD fda[2];
  WSAPoll(fda, 2, 0);
      ]],
      [have_poll='yes'],[have_poll='no']
    )
  ]
)
AS_VAR_IF([have_poll],["yes"],
  [
    AC_DEFINE([[HAVE_POLL]],[[1]],[Define to '1' if poll() is supported on your platform])
    MHD_CHECK_DECLS([POLLRDNORM POLLIN POLLRDBAND POLLWRNORM POLLOUT POLLWRBAND POLLPRI],[[
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
      ]]
    )
    AS_IF([test "x$enable_poll" != "xno"],
      [
        enable_poll="yes"
        AC_DEFINE([[MHD_SUPPORT_POLL]],[[1]],[Define to '1' to enable use of select() system call])
      ]
    )
  ],
  [
    AS_VAR_IF([enable_poll],["yes"],
      [AC_MSG_ERROR([Support for poll() was explicitly requested but cannot be enabled on this platform.])]
    )
  ]
)
AM_CONDITIONAL([MHD_SUPPORT_POLL],[test "x${enable_poll}" = "xyes"])

AC_ARG_ENABLE([[epoll]],
  [AS_HELP_STRING([[--enable-epoll[=ARG]]], [enable epoll support (yes, no, auto) [auto]])],
  [],[enable_epoll='auto']
)
AS_IF([test "$enable_epoll" != "no"],
  [
    MHD_CHECK_FUNC_RUN([epoll_create],
      [[
#include <sys/epoll.h>
#include <unistd.h>
      ]],
      [[
        int epfd = epoll_create(64);
        i][f (0 > epfd) return -epfd;
        (void) close(epfd);
        return 0;
      ]],
      [cacheVar="assuming yes"],
      [
        AC_DEFINE([[HAVE_EPOLL]],[[1]],[Define to '1' if epoll is supported on your platform])
        AC_DEFINE([[MHD_SUPPORT_EPOLL]],[[1]],[Define to '1' to enable 'epoll' functionality])
        enable_epoll='yes'
      ],
      [
        AS_IF([test "$enable_epoll" = "yes"],
          [AC_MSG_ERROR([[Support for epoll was explicitly requested but cannot be enabled on this platform.]])]
        )
        enable_epoll='no'
      ]
    )
    AS_UNSET([warn_msg])
    AS_VAR_IF([mhd_cv_works_func_epoll_create],["assuming yes"],
      [[warn_msg="When cross-compiling it is not possible to check whether 'epoll_create()' really works on the host (final) platform.
'epoll' is enabled as most probably the host kernel supports it (CONFIG_EPOLL option enabled in case of Linux kernel).
Use './configure mhd_cv_works_func_epoll_create=yes' to mute this warning."]]
    )
    AS_VAR_SET_IF([warn_msg],[AC_MSG_WARN([$warn_msg])
        AS_IF([test -n "${CONF_FINAL_WARNS}" ],
           [
             CONF_FINAL_WARNS="${CONF_FINAL_WARNS}

WARNING: "
           ]
        )
        CONF_FINAL_WARNS="${CONF_FINAL_WARNS}${warn_msg}"
      ]
    )
  ]
)
AM_CONDITIONAL([MHD_SUPPORT_EPOLL], [[test "x${enable_epoll}" = "xyes"]])

AS_IF([test "x$enable_epoll" = "xyes"],
  [
    MHD_CHECK_FUNC([epoll_create1],
      [[
#include <sys/epoll.h>
      ]],
      [[
  i][f (0 > epoll_create1(EPOLL_CLOEXEC))
    return 3;
      ]]
    )
  ]
)

AC_ARG_ENABLE([[kqueue]],
  [AS_HELP_STRING([[--enable-kqueue[=ARG]]], [enable kqueue support (yes, no, auto) [auto]])],
  [
    AS_CASE([$enableval],
      [yes|no|auto],[:],
      [AC_MSG_ERROR([invalid parameter value --enable-kqueue=${enableval}])]
    )
  ],[enable_kqueue='auto']
)
AS_VAR_IF([enable_kqueue],["no"],[:],
  [
    MHD_CHECK_FUNC_RUN([kqueue],
      [[
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
      ]],
      [[
        int kq = kqueue();
        i][f (0 > kq) return -kq;
        (void) close(kq);
        return 0;
      ]],
      [cacheVar="assuming yes"],
      [
        AC_DEFINE([[HAVE_KQUEUE]],[[1]],[Define to '1' if kqueue is supported on your platform])
        AC_DEFINE([[MHD_SUPPORT_KQUEUE]],[[1]],[Define to '1' to enable 'kqueue' functionality])
        enable_kqueue='yes'
      ],
      [
        AS_VAR_IF([enable_kqueue],["yes"],
          [AC_MSG_ERROR([[Support for kqueue was explicitly requested but cannot be enabled on this platform.]])]
        )
        enable_kqueue='no'
      ]
    )
  ]
)
AM_CONDITIONAL([MHD_SUPPORT_KQUEUE], [[test "x${enable_kqueue}" = "xyes"]])

AS_VAR_IF([enable_kqueue],["yes"],
  [
    MHD_CHECK_FUNC([kqueuex],
      [[
#include <sys/event.h>
      ]],
      [[
  i][f (0 > kqueuex(KQUEUE_CLOEXEC))
    return 3;
      ]]
    )
    MHD_CHECK_FUNC([kqueue1],
      [[
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <fcntl.h>
      ]],
      [[
  i][f (0 > kqueue1(O_CLOEXEC))
    return 3;
      ]]
    )
  ]
)


AC_CACHE_CHECK([for supported 'noreturn' keyword], [mhd_cv_decl_noreturn],
  [
    mhd_cv_decl_noreturn="none"
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS} ${errattr_CFLAGS}"
    AS_VAR_IF([mhd_cv_cc_warnless_c_flags],["no"],[:],[ac_c_werror_flag="yes"])
    for decl_noret in ['[[noreturn]]'] '_Noreturn' '__attribute__((__noreturn__))' '__declspec(noreturn)'
    do
      AC_LINK_IFELSE([AC_LANG_SOURCE(
          [[
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

${decl_noret} static void myexitfunc(int code)
{
#ifdef HAVE_STDLIB_H
  exit (code);
#else
  (void)code;
#endif
}

int main (int argc, char *const *argv)
{
  (void) argv;
  if (argc > 2)
    myexitfunc (2);
  return 0;
}
          ]]
        )], [mhd_cv_decl_noreturn="${decl_noret}"]
      )
      AS_IF([test "x${mhd_cv_decl_noreturn}" != "xnone"], [break])
    done
    ac_c_werror_flag=""
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
  ]
)
AS_VAR_IF([mhd_cv_decl_noreturn], ["none"],
  [AC_DEFINE([MHD_NORETURN_], [[/* empty */]], [Define to the supported 'noreturn' function declaration])],
  [AC_DEFINE_UNQUOTED([MHD_NORETURN_], [${mhd_cv_decl_noreturn}], [Define to the supported 'noreturn' function declaration])]
)

# Check for types sizes
# Types sizes are used as an indirect indication of maximum allowed values for types
# which is used to exclude by preprocessor some compiler checks for values clips
# Assuming no staffing or uniform staffing for integer types
AC_CACHE_CHECK([size of tv_sec member of struct timeval], [mhd_cv_size_timeval_tv_sec],
  [
    AC_COMPUTE_INT([mhd_cv_size_timeval_tv_sec], [((long int)sizeof(test_var.tv_sec))],
      [[
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */
#ifdef HAVE_TIME_H
#include <time.h>
#endif /* HAVE_TIME_H */
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
extern struct timeval test_var; /* Declaration */
struct timeval test_var = {0, 0}; /* Definition */
      ]],
      [
        # The size is used only to exclude additional checks/comparison in code
        # to avoid compiler warnings. With larger size MHD code will use
        # additional checks which ensure that value will fit but it may produce
        # a harmless compiler warning.
        AC_MSG_WARN([The size cannot be determined, assuming 8.])
        mhd_cv_size_timeval_tv_sec=8
      ]
    )
  ]
)
AC_DEFINE_UNQUOTED([SIZEOF_STRUCT_TIMEVAL_TV_SEC], [$mhd_cv_size_timeval_tv_sec],
  [The size of `tv_sec' member of `struct timeval', as computed by sizeof])
AC_CHECK_SIZEOF([uint_least8_t], [], [[#include <stdint.h>]])
AC_CHECK_SIZEOF([uint_least16_t], [], [[#include <stdint.h>]])
AC_CHECK_SIZEOF([uint_least32_t], [], [[#include <stdint.h>]])
AC_CHECK_SIZEOF([uint_fast32_t], [], [[#include <stdint.h>]])
AC_CHECK_SIZEOF([uint_least64_t], [], [[#include <stdint.h>]])
AC_CHECK_SIZEOF([int_fast64_t], [], [[#include <stdint.h>]])
AC_CHECK_SIZEOF([uint_fast64_t], [], [[#include <stdint.h>]])
AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([unsigned int])
AC_CHECK_SIZEOF([unsigned long])
AC_CHECK_SIZEOF([unsigned long long])
AC_CHECK_SIZEOF([size_t], [],
  [[
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif /* HAVE_STDDEF_H */
#include <stdio.h>
  ]]
)
AC_CHECK_SIZEOF([void*])

AC_CHECK_HEADERS([dlfcn.h],[have_tlsplugin=yes],[have_tlsplugin=no], [AC_INCLUDES_DEFAULT])
AM_CONDITIONAL([MHD_HAVE_TLS_PLUGIN], [[test "x$have_tlsplugin" = xyes]])

AC_CHECK_HEADERS([zlib.h],[have_zlib=yes],[have_zlib=no], [AC_INCLUDES_DEFAULT])
AM_CONDITIONAL([HAVE_ZLIB], [[test "x$have_zlib" = xyes]])

AC_CHECK_HEADERS([jansson.h],[have_json=yes],[have_json=no], [AC_INCLUDES_DEFAULT])
AM_CONDITIONAL([HAVE_JANSSON], [[test "x$have_json" = xyes]])

# Check for generic functions
MHD_CHECK_FUNC([random],
  [
AC_INCLUDES_DEFAULT
[#include <stdlib.h>
  ]],
  [[long int r = random(); (void)r;]],
  [],
  [
    MHD_CHECK_FUNC([rand],
      [
AC_INCLUDES_DEFAULT
[#include <stdlib.h>
      ]],
      [[int r = rand(); (void)r;]],
	)
  ]
)

AC_CHECK_MEMBERS([struct sockaddr.sa_len, struct sockaddr_storage.ss_len,
                  struct sockaddr_in.sin_len, struct sockaddr_in6.sin6_len],
   [], [],
   [
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#elif defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#ifdef HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif
   ])

MHD_CHECK_FUNC_RUN([getsockname],
  [[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <string.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#if defined(_WIN32) && !defined(__CYGWIN__)
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#ifdef HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

  ]],
  [[
  const socklen_t c_addr_size = (socklen_t)sizeof(struct sockaddr_in);
  struct sockaddr_in sa;
  socklen_t addr_size;
  int ret = 1;
#if !defined(_WIN32) || defined(__CYGWIN__)
  int sckt;
  const int invld_sckt = -1;
#else
  SOCKET sckt;
  const SOCKET invld_sckt = INVALID_SOCKET;
  WSADATA wsa_data;

  i][f (0 != WSAStartup(MAKEWORD(2, 2), &wsa_data) || MAKEWORD(2, 2) != wsa_data.wVersion)
    return 20;
#endif

  sckt = socket (AF_INET, SOCK_STREAM, 0);
  i][f (invld_sckt != sckt)
  {
    memset(&sa, 0, c_addr_size);
    sa.sin_family = AF_INET;
    i][f (0 != INADDR_ANY)
      sa.sin_addr.s_addr = htonl (INADDR_ANY);
#ifdef HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
    sa.sin_len = c_addr_size;
#endif
    i][f (0 == bind (sckt, (struct sockaddr *)&sa, c_addr_size))
    {
      i][f (0 == listen (sckt, 1))
      {
        addr_size = c_addr_size;
        i][f (0 == getsockname (sckt, (struct sockaddr  *)&sa, &addr_size))
        {
          i][f (c_addr_size >= addr_size)
          {
            i][f (0 != ntohs(sa.sin_port))
            { ret = 0;
            } el][se ret = 7;
          } el][se ret = 6;
        } el][se ret = 5;
      } el][se ret = 4;
    } el][se ret = 3;
  } el][se ret = 2;
#if !defined(_WIN32) || defined(__CYGWIN__)
  close (sckt);
#else
  closesocket (sckt);
  WSACleanup();
#endif
  return ret;
  ]],
  [cacheVar='assuming yes'],
  [AC_DEFINE([[MHD_USE_GETSOCKNAME]], [[1]], [Define if you have usable `getsockname' function.])]
)

AS_VAR_IF([[os_is_native_w32]], [["yes"]],[],
  [
    MHD_CHECK_FUNC([socketpair],
      [[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif
      ]],
      [[
          int arr[2];
          i][f (0 != socketpair(0, SOCK_STREAM, 0, arr)) return 2;
          close (arr[1]);
          close (arr[0]);
      ]],
      [],
      [AS_UNSET([mhd_cv_socketpair_usable])]
    )
  ]
)

MHD_CHECK_DECLS([AF_UNIX AF_LOCAL SOMAXCONN IPV6_V6ONLY TCP_NODELAY TCP_FASTOPEN \
                 SOCK_NONBLOCK SOCK_CLOEXEC SOCK_NOSIGPIPE MSG_NOSIGNAL MSG_MORE \
                 SOL_SOCKET SO_REUSEADDR SO_REUSEPORT SO_LINGER SO_NOSIGPIPE \
                 SHUT_WR SD_SEND],
  [[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#ifdef HAVE_SOCKLIB_H
#include <sockLib.h>
#endif
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#ifdef HAVE_INETLIB_H
#include <inetLib.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#if !defined(HAVE_NETINET_IN_H) && !defined(HAVE_ARPA_INET_H) && defined(HAVE_NETDB_H)
#include <netdb.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#ifdef HAVE_AFUNIX_H
#include <afunix.h>
#endif
#ifdef HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif
  ]]
)

AC_CACHE_CHECK([for usable PAGESIZE macro], [mhd_cv_macro_pagesize_usable],
  [
    AC_LINK_IFELSE(
      [
        AC_LANG_PROGRAM(
          [[
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifndef PAGESIZE
#error No PAGESIZE macro defined
fail test here %%%@<:@-1@:>@
#endif
          ]],
          [[
            long pgsz = PAGESIZE + 0;
            if (1 > pgsz) return 1;
          ]]
        )
      ],
      [[mhd_cv_macro_pagesize_usable="yes"]], [[mhd_cv_macro_pagesize_usable="no"]]
    )
  ]
)
AS_VAR_IF([[mhd_cv_macro_pagesize_usable]], [["yes"]],
  [
    AC_DEFINE([[MHD_USE_PAGESIZE_MACRO]],[[1]],[Define if you have usable PAGESIZE macro])
    AC_CACHE_CHECK([whether PAGESIZE macro could be used for static init], [mhd_cv_macro_pagesize_usable_static],
      [
        AC_LINK_IFELSE(
          [
            AC_LANG_PROGRAM(
              [[
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifndef PAGESIZE
#error No PAGESIZE macro defined
fail test here %%%@<:@-1@:>@
#endif
static long ac_pgsz = PAGESIZE + 0;
              ]],
              [[
                if (1 > ac_pgsz) return 1;
              ]]
            )
          ],
          [[mhd_cv_macro_pagesize_usable_static="yes"]], [[mhd_cv_macro_pagesize_usable_static="no"]]
        )
      ]
    )
    AS_VAR_IF([[mhd_cv_macro_pagesize_usable_static]], [["yes"]],
      [AC_DEFINE([[MHD_USE_PAGESIZE_MACRO_STATIC]],[[1]],[Define if you have PAGESIZE macro usable for static init])]
    )
  ],
  [
    AC_CACHE_CHECK([for usable PAGE_SIZE macro], [mhd_cv_macro_page_size_usable],
      [
        AC_LINK_IFELSE(
          [
            AC_LANG_PROGRAM(
              [[
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifndef PAGE_SIZE
#error No PAGE_SIZE macro defined
fail test here %%%@<:@-1@:>@
#endif
              ]],
              [[
                long pgsz = PAGE_SIZE + 0;
                if (1 > pgsz) return 1;
              ]]
            )
          ],
          [[mhd_cv_macro_page_size_usable="yes"]], [[mhd_cv_macro_page_size_usable="no"]]
        )
      ]
    )
    AS_VAR_IF([[mhd_cv_macro_page_size_usable]], [["yes"]],
      [
        AC_DEFINE([[MHD_USE_PAGE_SIZE_MACRO]],[[1]],[Define if you have usable PAGE_SIZE macro])
        AC_CACHE_CHECK([whether PAGE_SIZE macro could be used for static init], [mhd_cv_macro_page_size_usable_static],
          [
            AC_LINK_IFELSE(
              [
                AC_LANG_PROGRAM(
                  [[
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifndef PAGE_SIZE
#error No PAGE_SIZE macro defined
fail test here %%%@<:@-1@:>@
#endif
static long ac_pgsz = PAGE_SIZE + 0;
                  ]],
                  [[
                    if (1 > ac_pgsz) return 1;
                  ]]
                )
              ],
              [[mhd_cv_macro_page_size_usable_static="yes"]], [[mhd_cv_macro_page_size_usable_static="no"]]
            )
          ]
        )
        AS_VAR_IF([[mhd_cv_macro_page_size_usable_static]], [["yes"]],
          [AC_DEFINE([[MHD_USE_PAGE_SIZE_MACRO_STATIC]],[[1]],[Define if you have PAGE_SIZE macro usable for static init])]
        )
      ]
    )
  ]
)

# Check for inter-thread signaling type
AC_ARG_ENABLE([[itc]],
  [AS_HELP_STRING([[--enable-itc=TYPE]], [use TYPE of inter-thread communication (pipe, socketpair, eventfd) [auto]])], [],
  [[enable_itc='auto']]
)

AS_CASE([[$enable_itc]],
  [[pipe]], [[:]],
  [[socketpair]], [[:]],
  [[eventfd]], [[:]],
  [[auto]], [AS_VAR_IF([[os_is_windows]], [["yes"]], [[enable_itc='socketpair']])],
  [[eventFD]], [[enable_itc='eventfd']],
  [[socket]], [[enable_itc='socketpair']],
  [[no]], [AC_MSG_ERROR([[inter-thread communication cannot be disabled]])],
    [AC_MSG_ERROR([[unrecognized type "$enable_itc" of inter-thread communication specified by "--enable-itc=$enable_itc"]])]
)
AS_UNSET([[use_itc]])

AS_IF([[test "x$enable_itc" = "xeventfd" || test "x$enable_itc" = "xauto"]],
  [
    MHD_CHECK_FUNC_RUN([eventfd],[[
#include <sys/eventfd.h>
#include <unistd.h>
      ]],
      [[
  static unsigned char buf[8];
  int ret;
  int efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  if (0 > efd)
    return 2;
  ret = 0;
  buf[3] = 1;
  if (8 != write(efd, buf, 8))
    ret = 3;
  else
  {
    if (8 != read(efd, buf, 8))
      ret = 4;
  }
  close(efd);
  return ret;
      ]],
      [
        AS_VAR_IF([enable_itc],["eventfd"],
          [cacheVar="assuming yes"],[cacheVar="assuming no"]
        )
      ],
      [
        use_itc='eventfd'
        enable_itc="$use_itc"
        AC_DEFINE([[MHD_ITC_EVENTFD_]], [[1]], [Define to use eventFD for inter-thread communication])
      ],
      [
        AS_VAR_IF([[enable_itc]], [["eventfd"]], [AC_MSG_ERROR([[eventfd(2) is not usable, consider using other type of inter-thread communication]])])
      ]
    )
    AS_UNSET([warn_msg])
    AS_VAR_IF([mhd_cv_works_func_eventfd],["assuming yes"],
      [[warn_msg="When cross-compiling it is not possible to check whether 'eventfd()' really works on the host (final) platform.
'eventfd' is enabled as requested by configure parameters.
Use './configure mhd_cv_works_func_eventfd=yes' to mute this warning."]]
    )
    AS_VAR_IF([mhd_cv_works_func_eventfd],["assuming no"],
      [[warn_msg="When cross-compiling it is not possible to check whether 'eventfd()' really works on the host (final) platform.
'eventfd()' is disabled. If it is available use './configure mhd_cv_works_func_eventfd=yes' to enable.
Use './configure mhd_cv_works_func_eventfd=no' to mute this warning."]]
    )
    AS_VAR_SET_IF([warn_msg],[AC_MSG_WARN([$warn_msg])
        AS_IF([test -n "${CONF_FINAL_WARNS}" ],
           [
             CONF_FINAL_WARNS="${CONF_FINAL_WARNS}

WARNING: "
           ]
        )
        CONF_FINAL_WARNS="${CONF_FINAL_WARNS}${warn_msg}"
      ]
    )
  ]
)

AS_IF([[test "x$enable_itc" = "xpipe" || test "x$enable_itc" = "xauto"]], [
  AS_VAR_IF([[os_is_native_w32]], [["yes"]], [], [
    AC_CACHE_CHECK([[whether pipe(3) is usable]], [[mhd_cv_pipe_usable]], [
      AC_LINK_IFELSE([
        AC_LANG_PROGRAM([
AC_INCLUDES_DEFAULT
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
        ], [[
          int arr[2];
          int res;
          res = pipe(arr);
          if (res != 0) return 33;
          close (arr[0]);
          close (arr[1]);
        ]])
      ], [[mhd_cv_pipe_usable='yes']], [[mhd_cv_pipe_usable='no']])
    ])
    AS_VAR_IF([[mhd_cv_pipe_usable]], [["yes"]], [
      use_itc='pipe'
      enable_itc="$use_itc"
      AC_DEFINE([[MHD_ITC_PIPE_]], [[1]], [Define to use pipe for inter-thread communication])
      MHD_CHECK_FUNC_RUN([pipe2],[
AC_INCLUDES_DEFAULT
[
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
        ]],[[
          int arr[2];
          int res;
          res = pipe2(arr, O_CLOEXEC | O_NONBLOCK);
          i][f (res != 0) return 33;
          close (arr[0]);
          close (arr[1]);
          return 0;
        ]],[
          # Cross-compiling
          AS_CASE([${host_os}], [kfreebsd*-gnu], [cacheVar='assuming no'],
            [cacheVar='assuming yes'])
        ],
        [AC_DEFINE([[HAVE_PIPE2_FUNC]], [[1]], [Define if you have usable pipe2(2) function])]
      )
    ], [
      AS_VAR_IF([[enable_itc]], [["pipe"]], [AC_MSG_ERROR([[pipe(3) is not usable, consider using other type of inter-thread communication]])])
    ])
  ])
])

AS_IF([[test "x$enable_itc" = "xsocketpair" || test "x$enable_itc" = "xauto"]],
  [
    AS_IF([test "x${os_is_native_w32}" = "xyes"],
      [mhd_cv_socketpair_usable='yes'],
      [test "x${mhd_cv_func_socketpair}" = "xyes"],
      [
        AC_CACHE_CHECK([[whether socketpair(3) is usable]], [[mhd_cv_socketpair_usable]],
          [
            AC_LINK_IFELSE([
                AC_LANG_PROGRAM(
                  [[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif
                  ]], [[
              int arr[2];
              int res;
#if defined(AF_UNIX)
              res = socketpair(AF_UNIX, SOCK_STREAM, 0, arr);
#elif defined(AF_LOCAL)
              res = socketpair(AF_LOCAL, SOCK_STREAM, 0, arr);
#else
#error AF_LOCAL and AF_UNIX are both undefined
              fail test here %%%@<:@-1@:>@
#endif
              if (res != 0) return 1;
              close (arr[1]);
              close (arr[0]);
                  ]]
                )
              ],
              [[mhd_cv_socketpair_usable='yes']],
              [[mhd_cv_socketpair_usable='no']]
            )
          ]
        )
      ],
      [AS_UNSET([mhd_cv_socketpair_usable])]
    )

    AS_VAR_IF([[mhd_cv_socketpair_usable]], [["yes"]],
      [
        use_itc='socketpair'
        enable_itc="$use_itc"
        AC_DEFINE([[MHD_ITC_SOCKETPAIR_]], [[1]], [Define to use socketpair for inter-thread communication])
      ],
      [
        AS_VAR_IF([[enable_itc]], [["socketpair"]], [
          AC_MSG_ERROR([[socketpair(3) is not usable, consider using other type of inter-thread communication]])]
        )
      ]
    )
  ]
)

AS_IF([[test -z "$use_itc"]], [AC_MSG_ERROR([[cannot find usable type of inter-thread communication]])])


MHD_CHECK_FUNC([accept4],
  [[
#if defined(HAVE_SYS_TYPES_H)
#  include <sys/types.h>
#endif
#include <sys/socket.h>
  ]],
  [[
  struct sockaddr sk_addr;
  socklen_t addr_size;
  i][f (0 > accept4(0, &sk_addr, &addr_size, 0))
    return 3;
  ]]
)
MHD_CHECK_FUNC([gmtime_r],
  [[
#if defined(HAVE_SYS_TYPES_H)
#  include <sys/types.h>
#endif
#include <time.h>
  ]],
  [[
  time_t timer = (time_t) 0;
  struct tm res;

  i][f (&res != gmtime_r(&timer, &res))
    return 3;
  ]]
)
MHD_CHECK_FUNC([memmem],
  [[
#if defined(HAVE_STDDEF_H)
#  include <stddef.h>
#elif defined(HAVE_STDLIB_H)
#  include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#include <string.h>
  ]],
  [[
  const char *haystack = "abcd";
  size_t hslen = 3;
  const char *needle = "bc";
  size_t needlelen = 2;

  i][f ((haystack + 1) != memmem(haystack, hslen, needle, needlelen))
    return 3;
  ]]
)
MHD_CHECK_FUNC([snprintf],
  [[
#include <stdio.h>
  ]],
  [[
  char buf[2];

  i][f (1 != snprintf(buf, 2, "a"))
    return 3;
  /* Do not use the next check to avoid compiler warning */
  /* i][f (4 != snprintf(buf, 2, "abcd"))
    return 4; */
  ]]
)
AC_CHECK_DECL([gmtime_s],
  [
    AC_MSG_CHECKING([[whether gmtime_s is in C11 form]])
    AC_LINK_IFELSE(
        [ AC_LANG_PROGRAM(
          [[
#define __STDC_WANT_LIB_EXT1__ 1
#include <time.h>
#ifdef __cplusplus
extern "C"
#endif
             struct tm* gmtime_s(const time_t* time, struct tm* result);
           ]], [[
             static struct tm res;
             static time_t t = 0;
             gmtime_s (&t, &res);
          ]])
        ],
        [
          AC_DEFINE([HAVE_C11_GMTIME_S], [1], [Define to 1 if you have the `gmtime_s' function in C11 form.])
          AC_MSG_RESULT([[yes]])
        ],
        [
          AC_MSG_RESULT([[no]])
          AC_MSG_CHECKING([[whether gmtime_s is in W32 form]])
          AC_LINK_IFELSE(
            [ AC_LANG_PROGRAM(
              [[
#include <time.h>
#ifdef __cplusplus
extern "C"
#endif
errno_t gmtime_s(struct tm* _tm, const time_t* time);
              ]], [[
                 static struct tm res;
                 static time_t t = 0;
                 gmtime_s (&res, &t);
              ]])
            ],
            [
              AC_DEFINE([HAVE_W32_GMTIME_S], [1], [Define to 1 if you have the `gmtime_s' function in W32 form.])
              AC_MSG_RESULT([[yes]])
            ],
            [AC_MSG_RESULT([[no]])
            ])
        ])
  ], [],
  [[#define __STDC_WANT_LIB_EXT1__ 1
#include <time.h>]])


MHD_FIND_LIB([clock_gettime],[[#include <time.h>]],
  [[
    struct timespec tp;
    i][f (0 > clock_gettime(CLOCK_REALTIME, &tp))
      return 3;
  ]],
  [rt],
  [
    AC_DEFINE([HAVE_CLOCK_GETTIME], [1], [Define to '1' if you have clock_gettime() function])
    AS_VAR_IF([[mhd_cv_find_lib_clock_gettime]],[["none required"]], [],
      [
        MHD_LIBDEPS_PKGCFG="${mhd_cv_find_lib_clock_gettime} $MHD_LIBDEPS_PKGCFG"
      ]
    )
  ],[],
  [MHD_LIBDEPS]
)

MHD_CHECK_FUNC([mach_continuous_approximate_time],
  [[
#include <mach/mach.h>
#include <mach/mach_time.h>
  ]],
  [[
    struct mach_timebase_info mtb_info;
    i][f (KERN_SUCCESS != mach_timebase_info(&mtb_info))
      return 2;
    i][f (0 == mach_continuous_approximate_time() * mtb_info.numer / mtb_info.denom)
      return 3;
  ]], [], [
    MHD_CHECK_FUNC([mach_approximate_time],
      [[
#include <mach/mach.h>
#include <mach/mach_time.h>
      ]],
      [[
        struct mach_timebase_info mtb_info;
        i][f (KERN_SUCCESS != mach_timebase_info(&mtb_info))
          return 2;
        i][f (0 == mach_approximate_time() * mtb_info.numer / mtb_info.denom)
          return 3;
      ]]
    )
  ]
)

MHD_CHECK_FUNC([clock_get_time],
  [[
#include <mach/mach.h>
#include <mach/clock.h>

#if !defined(SYSTEM_CLOCK) && defined(REALTIME_CLOCK)
#  define SYSTEM_CLOCK REALTIME_CLOCK
#endif
  ]],
  [[
    clock_serv_t cs;
    mach_timespec_t mt;
    host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cs);
    clock_get_time(cs, &mt);
    mach_port_deallocate(mach_task_self(), cs);
  ]]
)

MHD_CHECK_FUNC([gethrtime],
  [[
#ifdef HAVE_SYS_TIME_H
/* Solaris define gethrtime() in sys/time.h */
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */
#ifdef HAVE_TIME_H
/* HP-UX define gethrtime() in time.h */
#include <time.h>
#endif /* HAVE_TIME_H */
  ]],
  [[
    hrtime_t hrt = gethrtime();
    i][f (0 == hrt)
      return 3;
  ]]
)

AS_VAR_IF([ac_cv_header_time_h], ["yes"],
  [
    MHD_CHECK_FUNC([timespec_get],
      [[
#include <time.h>

#ifndef TIME_UTC
#error TIME_UTC must be defined to use timespec_get()
fail test here %%%@<:@-1@:>@
#endif
      ]],
      [[
  struct timespec ts;
  i][f (TIME_UTC != timespec_get (&ts, TIME_UTC))
    return 3;
      ]]
    )
  ]
)

MHD_CHECK_FUNC_GETTIMEOFDAY

# IPv6
AC_CACHE_CHECK([for IPv6],[mhd_cv_have_inet6],
  [
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#elif defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#ifdef HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif
          ]], [[
  int af=AF_INET6;
  struct sockaddr_in6 sa;
  printf("%d %p\n", (int) af, (void*) &sa);
          ]]
        )
      ],
      [AS_VAR_SET([mhd_cv_have_inet6],["yes"])],
      [AS_VAR_SET([mhd_cv_have_inet6],["no"])]
    )
  ]
)
AS_VAR_IF([mhd_cv_have_inet6],["yes"],
  [AC_DEFINE([HAVE_INET6], [1], [Define to '1' if you have IPv6 headers])]
)

MHD_CHECK_FUNC([[sysconf]], [[#include <unistd.h>]], [[long a = sysconf(0); if (a) return 1;]])

MHD_CHECK_FUNC([[sysctl]], [[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#if defined(HAVE_STDDEF_H)
#include <stddef.h>
#elif defined(HAVE_STDLIB_H)
#include <stdlib.h>
#endif
  ]], [[
      int mib[2] = {0, 0}; /* Avoid any platform-specific values */
      i][f (sysctl(mib, 2, NULL, NULL, NULL, 0)) return 1;
  ]],
  [
    MHD_CHECK_DECLS([CTL_NET PF_INET IPPROTO_ICMP ICMPCTL_ICMPLIM],
      [[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif /* HAVE_SYS_SYSCTL_H */
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif /* HAVE_SYS_SYSCTL_H */
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#elif defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif /* HAVE_NETINET_IN_SYSTM_H */
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif /* HAVE_NETINET_IP_H */
#ifdef HAVE_NETINET_IP_ICMP_H
#include <netinet/ip_icmp.h>
#endif /* HAVE_NETINET_IP_ICMP_H */
#ifdef HAVE_NETINET_ICMP_VAR_H
#include <netinet/icmp_var.h>
#endif /* HAVE_NETINET_ICMP_VAR_H */
      ]]
    )
  ]
)

MHD_CHECK_FUNC([[sysctlbyname]], [[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#if defined(HAVE_STDDEF_H)
#include <stddef.h>
#elif defined(HAVE_STDLIB_H)
#include <stdlib.h>
#endif
  ]], [[sysctlbyname("test", NULL, NULL, NULL, 0);]]
)

MHD_CHECK_FUNC([[usleep]], [[#include <unistd.h>]], [[usleep(100000);]])
MHD_CHECK_FUNC([[nanosleep]], [[#include <time.h>]], [[struct timespec ts2, ts1 = {0, 0}; nanosleep(&ts1, &ts2);]])

# NOTE: require setting of errattr_CFLAGS above
AS_VAR_IF([mhd_cv_cc_warnless_c_flags],["no"],[:],[ac_c_werror_flag="yes"])
CFLAGS="${CFLAGS_ac} ${user_CFLAGS} ${errattr_CFLAGS}"
AC_CACHE_CHECK([whether $CC supports __attribute__((used))],[mhd_cv_cc_attr_used],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
extern __attribute__((used)) int
test_func(void);

extern __attribute__((used)) int
test_func(void) {return 0;}

int main(void) {return test_func();}
          ]])
      ],
      [mhd_cv_cc_attr_used="yes"],[mhd_cv_cc_attr_used="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_used],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_USED],[1],
      [Define to '1' if your compiler supports __attribute__((used))]
    )
  ]
)
AC_CACHE_CHECK([whether $CC supports __attribute__((pure))],[mhd_cv_cc_attr_pure],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
extern __attribute__((pure)) int
test_func(const char *ptr);

extern __attribute__((pure)) int
test_func(const char *ptr) {return (0 == (*ptr));}

int main(void) {return test_func("");}
          ]])
      ],
      [mhd_cv_cc_attr_pure="yes"],[mhd_cv_cc_attr_pure="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_pure],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_PURE],[1],
      [Define to '1' if your compiler supports __attribute__((pure))]
    )
  ]
)
AC_CACHE_CHECK([whether $CC supports __attribute__((const))],[mhd_cv_cc_attr_const],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
extern __attribute__((const)) int
test_func(int a);

extern __attribute__((const)) int
test_func(int a) {return a + 1;}

int main(void) {return test_func(-1);}
          ]])
      ],
      [mhd_cv_cc_attr_const="yes"],[mhd_cv_cc_attr_const="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_const],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_CONST],[1],
      [Define to '1' if your compiler supports __attribute__((const))]
    )
  ]
)
AC_CACHE_CHECK([whether $CC supports __attribute__((visibility("default")))],[mhd_cv_cc_attr_visibility_default],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
extern __attribute__((visibility("default"))) int
test_extrn_func(void);

extern __attribute__((visibility("default"))) int
test_extrn_func(void) {return 0;}

int main(void) {return test_extrn_func();}
          ]])
      ],
      [mhd_cv_cc_attr_visibility_default="yes"],[mhd_cv_cc_attr_visibility_default="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_visibility_default],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_VISIBILITY_DEFAULT],[1],
      [Define to '1' if your compiler supports __attribute__((visibility("default")))]
    )

    AC_CACHE_CHECK([whether $CC supports __attribute__((visibility("internal")))],[mhd_cv_cc_attr_visibility_internal],
      [
        AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
__attribute__((visibility("internal"))) int
test_intern_func(void);

__attribute__((visibility("internal"))) int
test_intern_func(void) {return 0;}

int main(void) {return test_intern_func();}
              ]])
          ],
          [mhd_cv_cc_attr_visibility_internal="yes"],[mhd_cv_cc_attr_visibility_internal="no"]
        )
      ]
    )
    AS_VAR_IF([mhd_cv_cc_attr_visibility_internal],["yes"],
      [
        AC_DEFINE([HAVE_ATTR_VISIBILITY_INTERNAL],[1],
          [Define to '1' if your compiler supports __attribute__((visibility("internal")))]
        )
      ]
    )

    AC_CACHE_CHECK([whether $CC supports __attribute__((visibility("hidden")))],[mhd_cv_cc_attr_visibility_hidden],
      [
        AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
__attribute__((visibility("hidden"))) int
test_hidden_func(void);

__attribute__((visibility("hidden"))) int
test_hidden_func(void) {return 0;}

int main(void) {return test_hidden_func();}
              ]])
          ],
          [mhd_cv_cc_attr_visibility_hidden="yes"],[mhd_cv_cc_attr_visibility_hidden="no"]
        )
      ]
    )
    AS_VAR_IF([mhd_cv_cc_attr_visibility_hidden],["yes"],
      [
        AC_DEFINE([HAVE_ATTR_VISIBILITY_HIDDEN],[1],
          [Define to '1' if your compiler supports __attribute__((visibility("hidden")))]
        )
      ]
    )

    CFLAGS="${user_CFLAGS}"
    MHD_CHECK_CC_CFLAG([-fvisibility=hidden],[CFLAGS_ac],
      [
        MHD_APPEND_FLAG_TO_VAR([MHD_LIB_CFLAGS],[-fvisibility=hidden])
      ],
      [
        AC_MSG_WARN([[$CC supports __attribute__((visibility("default"))), but does not support -fvisibility=hidden. Check compiler and compiler flags.]])
      ]
    )
  ]
)
CFLAGS="${CFLAGS_ac} ${user_CFLAGS} ${errattr_CFLAGS}"
AC_CACHE_CHECK([whether $CC supports __attribute__ ((externally_visible))],[mhd_cv_cc_attr_extern_vis],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
int test_func(void) __attribute__ ((externally_visible));
__attribute__ ((externally_visible)) int
test_func(void) {return 0; }

int main(void) { return test_func(); }
          ]])
      ],
      [mhd_cv_cc_attr_extern_vis="yes"],[mhd_cv_cc_attr_extern_vis="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_extern_vis],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_EXTERN_VISIBLE],[1],
      [Define to '1' if your compiler supports __attribute__((externally_visible))]
    )
  ]
)
AC_CACHE_CHECK([whether $CC supports __attribute__ ((warn_unused_result))],[mhd_cv_cc_attr_warn_unused_res],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
static __attribute__ ((warn_unused_result)) int
test_func(void) {return 0; }

int main(void) { return test_func(); }
          ]])
      ],
      [mhd_cv_cc_attr_warn_unused_res="yes"],[mhd_cv_cc_attr_warn_unused_res="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_warn_unused_res],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_WARN_UNUSED_RES],[1],
      [Define to '1' if your compiler supports __attribute__ ((warn_unused_result))]
    )
  ]
)
AC_CACHE_CHECK([whether $CC supports __attribute__ ((returns_nonnull))],[mhd_cv_cc_attr_ret_nonnull],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
static __attribute__ ((returns_nonnull)) int *
test_func(void) {
  static int i = 0;
  return &i;
}

int main(void) {
  return *(test_func());
}
          ]])
      ],
      [mhd_cv_cc_attr_ret_nonnull="yes"],[mhd_cv_cc_attr_ret_nonnull="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_ret_nonnull],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_RET_NONNULL],[1],
      [Define to '1' if your compiler supports __attribute__ ((returns_nonnull))]
    )
  ]
)
AC_CACHE_CHECK([whether $CC supports __attribute__ ((assume_aligned (N)))],[mhd_cv_cc_attr_func_assume_aligned],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
static __attribute__((assume_aligned (1))) void*
test_func(void) {
  static signed char c = 0;
  return (void*) &c;
}

int main(void) {
  return test_func() ? 0 : 1;
}
          ]])
      ],
      [mhd_cv_cc_attr_func_assume_aligned="yes"],[mhd_cv_cc_attr_func_assume_aligned="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_func_assume_aligned],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_FUNC_ASSUME_ALIGNED],[1],
      [Define to '1' if your compiler supports __attribute__((assume_aligned(N)))]
    )
  ]
)
AC_CACHE_CHECK([whether $CC supports __attribute__ ((alloc_size (N)))],[mhd_cv_cc_attr_func_alloc_size],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#if defined(HAVE_STDDEF_H)
#  include <stddef.h> /* NULL */
#else
#  include <string.h> /* should provide NULL */
#endif

static __attribute__((alloc_size(1))) void*
test_alloc(unsigned short size) {
  static char buf_alloc[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  static unsigned short allocated = 0;
  void *ret;
  if (sizeof(buf_alloc) - allocated < size)
    return NULL;
  ret = (void*) (buf_alloc + allocated);
  allocated -= size;
  return ret;
}

int main(void) {
  return test_alloc(1) ? 0 : 1;
}
          ]])
      ],
      [mhd_cv_cc_attr_func_alloc_size="yes"],[mhd_cv_cc_attr_func_alloc_size="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_func_alloc_size],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_FUNC_ALLOC_SIZE],[1],
      [Define to '1' if your compiler supports __attribute__((alloc_size(N)))]
    )
  ]
)
AC_CACHE_CHECK([whether $CC supports __attribute__ ((malloc))],[mhd_cv_cc_attr_func_malloc],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#if defined(HAVE_STDDEF_H)
#  include <stddef.h> /* NULL */
#else
#  include <string.h> /* should provide NULL */
#endif

static __attribute__((malloc)) void*
test_alloc(void) {
  static char buf_alloc[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  static int used_flag = 0;
  if (used_flag)
    return NULL;
  used_flag = !0;
  return buf_alloc;
}

int main(void) {
  return test_alloc() ? 0 : 1;
}
          ]])
      ],
      [mhd_cv_cc_attr_func_malloc="yes"],[mhd_cv_cc_attr_func_malloc="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_func_malloc],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_FUNC_MALLOC],[1],
      [Define to '1' if your compiler supports __attribute__((malloc))]
    )
  ]
)
AC_CACHE_CHECK([whether $CC supports __attribute__ ((malloc(deallocator)))],[mhd_cv_cc_attr_func_malloc_dealloc],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#if defined(HAVE_STDDEF_H)
#  include <stddef.h> /* NULL */
#else
#  include <string.h> /* should provide NULL */
#endif

static int used_flag = 0;
static char buf_alloc[8] = {0, 0, 0, 0, 0, 0, 0, 0};

static void
test_dealloc(void *ptr) {
  if (! used_flag)
    return;
  if (ptr != (void*) buf_alloc)
    return;
  used_flag = 0;
}

static __attribute__((malloc(test_dealloc))) void*
test_alloc(void) {
  if (used_flag)
    return NULL;
  used_flag = !0;
  return (void*) buf_alloc;
}

int main(void) {
  test_dealloc(test_alloc());
  return 0;
}
          ]])
      ],
      [mhd_cv_cc_attr_func_malloc_dealloc="yes"],[mhd_cv_cc_attr_func_malloc_dealloc="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_func_malloc_dealloc],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_FUNC_MALLOC_DEALLOC],[1],
      [Define to '1' if your compiler supports __attribute__((malloc(deallocator)))]
    )
  ]
)
AC_CACHE_CHECK([whether $CC supports __attribute__ ((nonnull))],[mhd_cv_cc_attr_nonnull],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
static __attribute__ ((nonnull)) int
test_func(int *p) {return 0 == *p;}

int main(void) {
  int i = 0;
  return test_func(&i) ? 0 : 5;
}
          ]])
      ],
      [mhd_cv_cc_attr_nonnull="yes"],[mhd_cv_cc_attr_nonnull="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_nonnull],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_NONNULL],[1],
      [Define to '1' if your compiler supports __attribute__ ((nonnull))]
    )
  ]
)
AC_CACHE_CHECK([whether $CC supports __attribute__ ((nonnull(NUM)))],[mhd_cv_cc_attr_nonnull_num],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
static __attribute__ ((nonnull(2))) int
test_func(int *p1, int *p2) {
  return (((int *)(void *)0) == p1) && (0 == *p2);
}

int main(void) {
  int i = 0;
  return test_func( (int *)(void *)0, &i) ? 0 : 5;
}
          ]])
      ],
      [mhd_cv_cc_attr_nonnull_num="yes"],[mhd_cv_cc_attr_nonnull_num="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_nonnull_num],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_NONNULL_NUM],[1],
      [Define to '1' if your compiler supports __attribute__ ((nonnull(NUM)))]
    )
  ]
)
AC_CACHE_CHECK([whether $CC supports __attribute__ ((access (read_only,NUM)))],[mhd_cv_cc_attr_access_read],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
static __attribute__ ((access (read_only,1))) int
test_func(int *p) {return 0 == *p;}

int main(void) {
  int i = 0;
  return test_func(&i) ? 0 : 5;
}
          ]])
      ],
      [mhd_cv_cc_attr_access_read="yes"],[mhd_cv_cc_attr_access_read="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_access_read],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_ACCESS_READ],[1],
      [Define to '1' if your compiler supports __attribute__ ((access (read_only,NUM)))]
    )
  ]
)
AC_CACHE_CHECK([whether $CC supports __attribute__ ((access (write_only,NUM)))],[mhd_cv_cc_attr_access_write],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
static __attribute__ ((access (write_only,1))) void
test_func(int *p) {*p = 0;}

int main(void) {
  int i = 1;
  test_func(&i);
  return i;
}
          ]])
      ],
      [mhd_cv_cc_attr_access_write="yes"],[mhd_cv_cc_attr_access_write="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_access_write],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_ACCESS_WRITE],[1],
      [Define to '1' if your compiler supports __attribute__ ((access (write_only,NUM)))]
    )
  ]
)
AC_CACHE_CHECK([whether $CC supports __attribute__ ((access (read_write,NUM)))],[mhd_cv_cc_attr_access_read_write],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
static __attribute__ ((access (read_write,1))) void
test_func(int *p) {*p = *p - 1;}

int main(void) {
  int i = 1;
  test_func(&i);
  return i;
}
          ]])
      ],
      [mhd_cv_cc_attr_access_read_write="yes"],[mhd_cv_cc_attr_access_read_write="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_access_read_write],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_ACCESS_READ_WRITE],[1],
      [Define to '1' if your compiler supports __attribute__ ((access (read_write,NUM)))]
    )
  ]
)
AC_CACHE_CHECK([whether $CC supports __attribute__ ((access (read_only,NUM,NUM_SIZE)))],[mhd_cv_cc_attr_access_read_size],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
static __attribute__ ((access (read_only,2,1))) int
test_func(unsigned int num, int *arr) {return 2 == arr[num - 1];}

int main(void) {
  int arr[4] = {5, 4, 3, 2};
  return test_func(4, arr) ? 0 : 5;
}
          ]])
      ],
      [mhd_cv_cc_attr_access_read_size="yes"],[mhd_cv_cc_attr_access_read_size="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_access_read_size],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_ACCESS_READ_SIZE],[1],
      [Define to '1' if your compiler supports __attribute__ ((access (read_only,NUM,NUM_SIZE)))]
    )
  ]
)
AC_CACHE_CHECK([whether $CC supports __attribute__ ((access (write_only,NUM,NUM_SIZE)))],[mhd_cv_cc_attr_access_write_size],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
static __attribute__ ((access (write_only,2,1))) void
test_func(unsigned int num, int *arr) {arr[num-1] = 0;}

int main(void) {
  int arr[4] = {5, 4, 3, 2};
  test_func(4, arr);
  return arr[3];
}
          ]])
      ],
      [mhd_cv_cc_attr_access_write_size="yes"],[mhd_cv_cc_attr_access_write_size="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_access_write_size],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_ACCESS_WRITE_SIZE],[1],
      [Define to '1' if your compiler supports __attribute__ ((access (write_only,NUM,NUM_SIZE)))]
    )
  ]
)
AC_CACHE_CHECK([whether $CC supports __attribute__ ((access (read_write,NUM,NUM_SIZE)))],[mhd_cv_cc_attr_access_read_write_size],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
static __attribute__ ((access (read_write,2,1))) void
test_func(unsigned int num, int *arr) {arr[num-1] = arr[1] - 4;}

int main(void) {
  int arr[4] = {5, 4, 3, 2};
  test_func(4, arr);
  return arr[3];
}
          ]])
      ],
      [mhd_cv_cc_attr_access_read_write_size="yes"],[mhd_cv_cc_attr_access_read_write_size="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_access_read_write_size],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_ACCESS_READ_WRITE_SIZE],[1],
      [Define to '1' if your compiler supports __attribute__ ((access (read_write,NUM,NUM_SIZE)))]
    )
  ]
)
AC_CACHE_CHECK([whether $CC supports __attribute__ ((fd_arg_read (NUM)))],[mhd_cv_cc_attr_fd_arg_read],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#ifndef _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS 1
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <stdio.h>
#ifdef HAVE_IO_H
#include <io.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

static __attribute__ ((fd_arg_read (1))) int
test_func(int fd)
{
  int data_in;
  int read_size;
  read_size = (int) read(fd, &data_in, sizeof(data_in));
  if (read_size < (int) sizeof(data_in)) return 4;
  return data_in;
}

int main(void) {
  int fd = open("test.txt", O_RDONLY);
  int res = test_func(fd);
  close (fd);
  return res;
}
          ]])
      ],
      [mhd_cv_cc_attr_fd_arg_read="yes"],[mhd_cv_cc_attr_fd_arg_read="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_fd_arg_read],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_FD_ARG_READ],[1],
      [Define to '1' if your compiler supports __attribute__ ((fd_arg_read (NUM)))]
    )
  ]
)
AC_CACHE_CHECK([whether $CC supports __attribute__ ((null_terminated_string_arg (NUM)))],[mhd_cv_cc_attr_null_term_str],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
static __attribute__ ((null_terminated_string_arg (1))) int
test_func(const char *str) {return (0 == str[0]) ? 5 : 0;}

int main(void) {
  return test_func("test");
}
          ]])
      ],
      [mhd_cv_cc_attr_null_term_str="yes"],[mhd_cv_cc_attr_null_term_str="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_null_term_str],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_NULL_TERM_STR],[1],
      [Define to '1' if your compiler supports __attribute__ ((null_terminated_string_arg (NUM)))]
    )
  ]
)
AC_CACHE_CHECK([whether $CC supports __attribute__((enum_extensibility (closed)))],[mhd_cv_cc_attr_enum_extns_closed],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
enum __attribute__((enum_extensibility (closed))) Test_Enum
{Val_A = 0, Val_B = 3};

int main(void)
{
  enum Test_Enum var = Val_A;
  return (var != Val_B) ? 0 : 3;
}
          ]])
      ],
      [mhd_cv_cc_attr_enum_extns_closed="yes"],[mhd_cv_cc_attr_enum_extns_closed="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_enum_extns_closed],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_ENUM_EXTNS_CLOSED],[1],
      [Define to '1' if your compiler supports __attribute__((enum_extensibility (closed)))]
    )
  ]
)
AC_CACHE_CHECK([whether $CC supports __attribute__((flag_enum))],[mhd_cv_cc_attr_flag_enum],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#ifdef HAVE_ATTR_ENUM_EXTNS_CLOSED
#define ATTR_EXTNS_CLOSED_IF_SUPPORTED __attribute__((enum_extensibility (closed)))
#else
#define ATTR_EXTNS_CLOSED_IF_SUPPORTED /* empty */
#endif
enum ATTR_EXTNS_CLOSED_IF_SUPPORTED __attribute__((flag_enum)) Test_Enum
{Flag_A = 1 << 0, Flag_B = 1 << 1, Flag_C = 1 << 2, Flag_D = 1 << 3};

int main(void)
{
  enum Test_Enum var = Flag_A;
  var |= Flag_C;
  var = var | Flag_D | Flag_A;
  return (var != Flag_B) ? 0 : 3;
}
          ]])
      ],
      [mhd_cv_cc_attr_flag_enum="yes"],[mhd_cv_cc_attr_flag_enum="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_attr_flag_enum],["yes"],
  [
    AC_DEFINE([HAVE_ATTR_FLAG_ENUM],[1],
      [Define to '1' if your compiler supports __attribute__((flag_enum))]
    )
  ]
)
ac_c_werror_flag=""
AC_CACHE_CHECK([[whether $CC supports array[static N] with fixed N as a function parameter]],[mhd_cv_cc_func_param_arr_static_fixed],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
static int
test_func(int arr[static 4]) { return arr[2] - 4; }

int main(void) {
  int arr[4] = {5, 4, 3, 2};
  return test_func(arr);
}
          ]])
      ],
      [mhd_cv_cc_func_param_arr_static_fixed="yes"],[mhd_cv_cc_func_param_arr_static_fixed="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_func_param_arr_static_fixed],["yes"],
  [
    AC_DEFINE([HAVE_FUNC_PARAM_ARR_STATIC_FIXED],[1],
      [Define to '1' if your compiler supports 'array[static N]' with fixed N as function parameter]
    )
  ]
)
AC_CACHE_CHECK([[whether $CC supports array[static N] with variable N as a function parameter]],[mhd_cv_cc_func_param_arr_static_var],
  [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
static int
test_func(int num, int arr[static num]) { return arr[num-2] - 4; }

int main(void) {
  int arr[4] = {5, 4, 3, 2};
  return test_func(4, arr);
}
          ]])
      ],
      [mhd_cv_cc_func_param_arr_static_var="yes"],[mhd_cv_cc_func_param_arr_static_var="no"]
    )
  ]
)
AS_VAR_IF([mhd_cv_cc_func_param_arr_static_var],["yes"],
  [
    AC_DEFINE([HAVE_FUNC_PARAM_ARR_STATIC_VAR],[1],
      [Define to '1' if your compiler supports 'array[static N]' with variable N as a function parameter]
    )
  ]
)
CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"

# libcurl (required for testing)
AC_ARG_ENABLE([curl],
  [AS_HELP_STRING([--disable-curl],[disable cURL based testcases])],
  [enable_curl=${enableval}])
curl=0
AS_IF([test "$enable_curl" != "no"],
 [LIBCURL_CHECK_CONFIG([yes],[7.16.4],[enable_curl=yes],
    [
      AS_IF([test "x$enable_curl" = "xyes"],
       [AC_MSG_WARN([[cURL-based tests cannot be enabled because libcurl is missing]])])
      enable_curl=no
    ])
])
AM_CONDITIONAL([RUN_LIBCURL_TESTS], [test "x$enable_curl" = "xyes"])
AS_IF([test "x$enable_curl" = "xyes"],
  [MSG_CURL="yes"],
  [MSG_CURL="no, many unit tests will not run"]
)

MHD_CHECK_FUNC([magic_open],
  [[
#include <magic.h>
  ]],
  [[
    char var_data[256];
    const char *var_mime;
    magic_t var_magic = magic_open (MAGIC_MIME_TYPE);
    (void)magic_load (var_magic, "filename");
    var_data[0] = 0;
    var_mime = magic_buffer (var_magic, var_data, 1);
    i][f (! var_mime)
      return 1;
    magic_close (var_magic);
  ]],
  [AC_DEFINE([MHD_HAVE_LIBMAGIC], [1], [Define to 1 if you have suitable libmagic.])],
  [],
  [-lmagic]
)
AM_CONDITIONAL([MHD_HAVE_LIBMAGIC], [[test "x$mhd_cv_have_func_magic_open" = "xyes"]])

# large file support (> 4 GB)
MHD_CHECK_FUNC([lseek64],
  [[
#ifndef _LARGEFILE64_SOURCE
#  define _LARGEFILE64_SOURCE 1
#endif
#if defined(HAVE_SYS_TYPES_H)
#  include <sys/types.h>
#endif
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
  ]],
  [[
  i][f (((off64_t) -1) == lseek64(0, (off64_t) 0, SEEK_SET))
    return 3;
  ]]
)
MHD_CHECK_FUNC([pread64],
  [[
#ifndef _LARGEFILE64_SOURCE
#  define _LARGEFILE64_SOURCE 1
#endif
#if defined(HAVE_SYS_TYPES_H)
#  include <sys/types.h>
#endif
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
  ]],
  [[
  char buf[5];
  i][f (0 > pread64(0, (void *) buf, 1, (off64_t) 0))
    return 3;
  ]]
)
MHD_CHECK_FUNC([pread],
  [[
#if defined(HAVE_SYS_TYPES_H)
#  include <sys/types.h>
#endif
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
  ]],
  [[
  char buf[5];
  i][f (0 > pread(0, (void *) buf, 1, 0))
    return 3;
  ]]
)

AS_VAR_IF([have_poll],["yes"],[MHD_CHECK_POLL_QUIRKS()])

# check for various sendfile functions
AC_ARG_ENABLE([sendfile],
   [AS_HELP_STRING([--disable-sendfile],
               [disable usage of sendfile() for HTTP connections [auto]])],
   [],
   [enable_sendfile="auto"])
AS_CASE([$enable_sendfile],
  [[auto | yes]],[[found_sendfile="no"]],
  [[no]],[[found_sendfile="disabled"]],
  [AC_MSG_ERROR([[unknown value specified: --enable-sendfile=$enable_sendfile]])]
)
AS_VAR_IF([[found_sendfile]], [["no"]],
  [
    AC_MSG_CHECKING([[for Linux-style sendfile(2)]])
    AC_LINK_IFELSE(
      [AC_LANG_PROGRAM(
        [[
#include <sys/sendfile.h>

static void empty_func(void)
{
/* Check for declaration */
#ifndef sendfile
  (void)sendfile;
#endif
}
        ]],
        [[
          int fd1=0, fd2=2;
          off_t o = 0;
          size_t s = 5;
          ssize_t r;
          r = sendfile (fd1, fd2, &o, s);
          if (r)
            empty_func();
        ]]
       )
      ],
      [
        AC_DEFINE([HAVE_LINUX_SENDFILE], [1], [Define to 1 if you have linux-style sendfile(2).])
        found_sendfile="yes, Linux-style"
        AC_MSG_RESULT([[yes]])
        MHD_CHECK_FUNC([sendfile64],
          [[
#ifndef _LARGEFILE64_SOURCE
#  define _LARGEFILE64_SOURCE 1
#endif
#include <sys/sendfile.h>
          ]],
          [[
  off64_t f_offset = (off64_t) 0;
  if (0 > sendfile64 (0, 1, &f_offset, 1))
    return 3;
          ]]
        )
      ],
      [AC_MSG_RESULT([[no]])
      ]
    )
  ]
)
AS_VAR_IF([[found_sendfile]], [["no"]],
  [
   AC_MSG_CHECKING([[for FreeBSD-style sendfile(2)]])
   AC_LINK_IFELSE(
     [AC_LANG_PROGRAM(
       [[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <sys/uio.h>

static void empty_func(void)
{
/* Check for declaration */
#ifndef sendfile
  (void)sendfile;
#endif
}
       ]],
       [[
         int fd1=0, fd2=1;
         off_t o = 0;
         size_t s = 5;
         off_t r1;
         int r2;
         r2 = sendfile (fd1, fd2, o, s, (struct sf_hdtr *)0, &r1, 0);
         if (r2)
           empty_func();
       ]]
      )
     ],
     [
       AC_DEFINE([HAVE_FREEBSD_SENDFILE], [1], [Define to 1 if you have FreeBSD-style sendfile(2).])
       found_sendfile="yes, FreeBSD-style"
       AC_MSG_RESULT([[yes]])
     ],
     [AC_MSG_RESULT([[no]])
     ]
   )
  ]
)
AS_VAR_IF([[found_sendfile]], [["no"]],
  [
   AC_MSG_CHECKING([[for Darwin-style sendfile(2)]])
   AC_LINK_IFELSE(
     [AC_LANG_PROGRAM(
       [[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <sys/uio.h>

static void empty_func(void)
{
/* Check for declaration */
#ifndef sendfile
  (void)sendfile;
#endif
}
       ]],
       [[
         int fd=0, s=1;
         off_t o = 0;
         off_t l = 5;
         int r;
         r = sendfile (fd, s, o, &l, (struct sf_hdtr *)0, 0);
         if (r)
           empty_func();
       ]]
      )
     ],
     [
       AC_DEFINE([HAVE_DARWIN_SENDFILE], [1], [Define to 1 if you have Darwin-style sendfile(2).])
       found_sendfile="yes, Darwin-style"
       AC_MSG_RESULT([[yes]])
     ],
     [AC_MSG_RESULT([[no]])
     ]
   )
  ]
)

AS_VAR_IF([[found_sendfile]], [["no"]],
  [
   AC_MSG_CHECKING([[for old Solaris-style sendfile(3)]])
   SAVE_LIBS="$LIBS"
   LIBS="-lsendfile $LIBS"
   AC_LINK_IFELSE(
     [AC_LANG_PROGRAM(
       [[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <sys/sendfile.h>

static void empty_func(void)
{
/* Check for declaration */
#ifndef sendfile
  (void)sendfile;
#endif
}
       ]],
       [[
         int fd1=0, fd2=1;
         off_t o = 0;
         size_t l = 5;
         ssize_t r;
         r = sendfile (fd1, fd2, &o, l);
         if (r)
           empty_func();
       ]]
      )
     ],
     [
       # Solaris uses the same format as GNU/Linux. Old versions need libsendfile.
       AC_DEFINE([HAVE_LINUX_SENDFILE], [1], [Define to 1 if you have linux-style sendfile(2).])
       found_sendfile="yes, Solaris-style"
       MHD_LIBDEPS="-lsendfile $MHD_LIBDEPS"
       MHD_LIBDEPS_PKGCFG="-lsendfile $MHD_LIBDEPS_PKGCFG"
       AC_MSG_RESULT([[yes]])
       MHD_CHECK_FUNC([sendfile64],
         [[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <sys/sendfile.h>
         ]],
         [[
  off64_t f_offset = (off64_t) 0;
#ifndef sendfile64
  (void)sendfile64;
#endif
  if (0 > sendfile64 (0, 1, &f_offset, 1))
    return 3;
         ]]
       )
     ],
     [AC_MSG_RESULT([[no]])
     ]
   )
   LIBS="$SAVE_LIBS"
  ]
)
AS_IF([[test "x$found_sendfile" = "xno" && test "x$enable_sendfile" = "xyes"]],
  [AC_MSG_ERROR([[sendfile() usage was requested by configure parameter, but no usable sendfile() function is detected]])]
)

# optional: disable log and HTTP automatic messages
AC_ARG_ENABLE([messages],
  [AS_HELP_STRING([--disable-messages],
      [disable log messages and text bodies for ]
      [automatic HTTP responses (to reduce the binary size)]
    )
  ],
  [],[enable_messages="yes"]
)

AC_MSG_CHECKING([[whether to support internal logging functionality and build messages for log]])
AC_ARG_ENABLE([log-messages],
  [AS_HELP_STRING([--disable-log-messages],
      [disable logger functionality and exclude log mesages from binary]
    )
  ],
  [],[enable_log_messages="$enable_messages"]
)
AS_VAR_IF([enable_log_messages],["yes"],
  [AC_DEFINE([MHD_SUPPORT_LOG_FUNCTIONALITY],[1],[Define to '1' to enable internal logging and log messages.])],
  [enable_log_messages="no"]
)
AC_MSG_RESULT([[$enable_log_messages]])
AM_CONDITIONAL([MHD_SUPPORT_LOG_FUNCTIONALITY], [test "x$enable_log_messages" != "xno"])

AC_MSG_CHECKING([[whether to build text bodies for automatic HTTP response messages]])
AC_ARG_ENABLE([http-messages],
  [AS_HELP_STRING([--disable-http-messages],
      [use empty bodies for automatic HTTP responses (less verbose for clients)]
    )
  ],
  [],[enable_http_messages="$enable_messages"]
)
AS_VAR_IF([enable_http_messages],["yes"],
  [AC_DEFINE([MHD_ENABLE_AUTO_MESSAGES_BODIES],[1],[Define to '1' to enable verbose text bodies for automatic HTTP replies.])],
  [enable_http_messages="no"]
)
AC_MSG_RESULT([[$enable_http_messages]])
AM_CONDITIONAL([MHD_ENABLE_AUTO_MESSAGES_BODIES], [test "x$enable_http_messages" != "xno"])


# optional: have postprocessor?
AC_MSG_CHECKING([[whether to enable POST parser]])
AC_ARG_ENABLE([postparser],
   [AS_HELP_STRING([--disable-postparser],
               [disable MHD POST parser functionality])],
   [enable_postparser=${enableval}],
   [enable_postparser=yes])
AS_IF([[test "x$enable_postparser" != "xno"]],
  [ enable_postparser=yes
    AC_DEFINE([MHD_SUPPORT_POST_PARSER],[1],[Define to 1 if libmicrohttpd is compiled with POST parser support.]) ])
AM_CONDITIONAL([MHD_SUPPORT_POST_PARSER], [test "x$enable_postparser" != "xno"])
AC_MSG_RESULT([[$enable_postparser]])

have_gnutls=no
have_gnutls_pkgcfg=no
have_openssl=no
have_openssl_pkgcfg=no
have_mbedtls=no
have_mbedtls_pkgcfg=no
multiple_tls="no"
AS_UNSET([MHD_TLS_LIB_CPPFLAGS])
AS_UNSET([MHD_TLS_LIB_LDFLAGS])
AS_UNSET([MHD_TLS_LIBDEPS])
MSG_TLS_BACKENDS="none"

AC_ARG_ENABLE([http2],
   [AS_HELP_STRING([--disable-http2], [disable HTTP/2 support])],
   [
     AS_CASE([${enableval}],
       [yes],[:],
       [no],[:],
       [AC_MSG_ERROR([wrong parameter value --enable-http2=${enableval}])]
     )
   ],
   [enable_http2="yes"]
)
AS_VAR_IF([enable_http2],["yes"],
  [AC_DEFINE([MHD_SUPPORT_HTTP2],[1],[Define to '1' to enable support for HTTP/2 protocol])]
)
AM_CONDITIONAL([MHD_SUPPORT_HTTP2], [test "x$enable_http2" = "xyes"])

# optional: HTTPS support.  Enabled if GnuTLS is available.
PKG_PROG_PKG_CONFIG
AS_UNSET([GNUTLS_MOD_VERSION])
AS_UNSET([OPENSSL_MOD_VERSION])
AS_UNSET([MBEDTLS_MOD_VERSION])
AC_ARG_ENABLE([https],
   [AS_HELP_STRING([--enable-https],
               [enable HTTPS support (yes, no, auto)[auto]])],
   [enable_https=${enableval}])
AS_IF([test "x$enable_https" != "xno"],
  [
    have_gnutls_pkgcfg=no
    AC_MSG_CHECKING([[how to find GnuTLS library]])
    AC_ARG_WITH([[gnutls]],
      [AS_HELP_STRING([[--with-gnutls[=PRFX]]],
          [use GnuTLS for HTTPS support, optional PRFX overrides pkg-config data for GnuTLS headers (PRFX/include) and libs (PRFX/lib)])],
      [
        AS_CASE([$with_gnutls],
          [no],[
            have_gnutls="no"
            AS_UNSET([GNUTLS_CPPFLAGS])
            AS_UNSET([GNUTLS_CFLAGS])
            AS_UNSET([GNUTLS_LDFLAGS])
            AS_UNSET([GNUTLS_LIBS])
            AC_MSG_RESULT([[GnuTLS disabled]])
          ],
          [yes],[
            AC_MSG_RESULT([[automatically, forced]])
          ],
          [
            AC_MSG_RESULT([[-I$with_gnutls/include -L$with_gnutls/lib -lgnutls]])
            LDFLAGS="${LDFLAGS_ac} -L$with_gnutls/lib ${user_LDFLAGS}"
            CPPFLAGS="${CPPFLAGS_ac} -I$with_gnutls/include ${user_CPPFLAGS}"
            have_gnutls_pkgcfg=no
            # A simple check for the working header and the library
            MHD_CHECK_FUNC([gnutls_check_version],
              [[
]AC_INCLUDES_DEFAULT[
#include <gnutls/gnutls.h>
              ]],
              [
               if(!gnutls_check_version("3.0.0"))
                 return 3;
              ],
              [
                have_gnutls=yes
                GNUTLS_CPPFLAGS="-I$with_gnutls/include"
                AS_UNSET([GNUTLS_CFLAGS])
                GNUTLS_LDFLAGS="-L$with_gnutls/lib"
                GNUTLS_LIBS="-lgnutls"
              ],
              [AC_MSG_ERROR([cannot find usable libgnutls at specified prefix $with_gnutls])],
              [-lgnutls]
            )
            CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
            CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
            LDFLAGS="${LDFLAGS_ac} ${user_LDFLAGS}"
          ])
       ],
       [AC_MSG_RESULT([[automatically]])]
    )

    AS_IF([test "x$with_gnutls" != "xno" && test "x$have_gnutls" != "xyes"],
      [
        PKG_CHECK_MODULES([GNUTLS], [[gnutls]],
          [
            CPPFLAGS="${CPPFLAGS_ac} $GNUTLS_CFLAGS ${user_CPPFLAGS}"
            # A simple check for the working header and the library
            MHD_CHECK_FUNC([gnutls_check_version],
              [[
]AC_INCLUDES_DEFAULT[
#include <gnutls/gnutls.h>
              ]],
              [
               if(!gnutls_check_version("3.0.0"))
                 return 3;
              ],
              [
                have_gnutls="yes"
                have_gnutls_pkgcfg="yes"
                # GNUTLS_CFLAGS is actually CPPFLAGS
                GNUTLS_CPPFLAGS="$GNUTLS_CFLAGS"
                AS_UNSET([GNUTLS_CFLAGS])
                # GNUTLS_LIBS is a combination of LDFLAGS and LIBS
                AS_UNSET([GNUTLS_LDFLAGS])
              ],
              [
                AS_VAR_IF([with_gnutls],["yes"],
                  [AC_MSG_ERROR([cannot find usable libgnutls])]
                )
                AC_MSG_WARN([pkg-config reports that GnuTLS is present, but GnuTLS cannot be used])
                AS_UNSET([GNUTLS_CPPFLAGS])
                AS_UNSET([GNUTLS_CFLAGS])
                AS_UNSET([GNUTLS_LDFLAGS])
                AS_UNSET([GNUTLS_LIBS])
              ],
              [$GNUTLS_LIBS]
            )
          ],
          [
            # check for GnuTLS at default paths
            have_gnutls_pkgcfg="no"
            # A simple check for the working header and the library
            MHD_CHECK_FUNC([gnutls_check_version],
              [[
]AC_INCLUDES_DEFAULT[
#include <gnutls/gnutls.h>
              ]],
              [
               if(!gnutls_check_version("3.0.0"))
                 return 3;
              ],
              [
                have_gnutls=yes
                AS_UNSET([GNUTLS_CPPFLAGS])
                AS_UNSET([GNUTLS_CFLAGS])
                AS_UNSET([GNUTLS_LDFLAGS])
                GNUTLS_LIBS="-lgnutls"
              ],
              [
                AS_VAR_IF([with_gnutls],["yes"],
                  [AC_MSG_ERROR([cannot find usable libgnutls])]
                )
                AS_UNSET([GNUTLS_CPPFLAGS])
                AS_UNSET([GNUTLS_CFLAGS])
                AS_UNSET([GNUTLS_LDFLAGS])
                AS_UNSET([GNUTLS_LIBS])
              ],
              [-lgnutls]
            )
          ]
        )
      ]
    )
    AS_VAR_IF([have_gnutls],["yes"],
      [
        AC_CACHE_CHECK([[whether GnuTLS is modern enough]], [mhd_cv_gnutls_ver_ok],
          [
            CPPFLAGS="${CPPFLAGS_ac} ${GNUTLS_CPPFLAGS} ${user_CPPFLAGS}"
            AC_COMPILE_IFELSE(
              [
                AC_LANG_PROGRAM(
                  [[
]AC_INCLUDES_DEFAULT[
#include <gnutls/gnutls.h>
                  ]],
                  [[
#if !defined(GNUTLS_VERSION_NUMBER) || GNUTLS_VERSION_NUMBER+0 <= 0x020c14
#error Too old version of GnuTLS that requires libgcript initialisation
fail test here %%%@<:@-1@:>@
#endif
                  ]]
                )
              ],
              [[mhd_cv_gnutls_ver_ok="yes"]], [[mhd_cv_gnutls_ver_ok="no"]]
            )
            CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
          ]
        )
        AS_VAR_IF([mhd_cv_gnutls_ver_ok],["yes"],[:],
          [
            have_gnutls="no"
            AS_VAR_IF([with_gnutls],["yes"],
              [AC_MSG_ERROR([cannot find usable libgnutls])]
            )
          ]
        )
      ]
    )

    AS_VAR_IF([have_gnutls],["yes"],
      [
        AS_VAR_IF([have_gnutls_pkgcfg],["yes"],
          [
            MHD_PKG_CONF_MODULE_VERSION([GNUTLS],[],[],[GNUTLS_MOD_VERSION='0'])
          ],[GNUTLS_MOD_VERSION='0']
        )
      ],
      [
        have_gnutls="no"
        have_gnutls_pkgcfg="no"
        GNUTLS_MOD_VERSION="no"
        AS_UNSET([GNUTLS_CPPFLAGS])
        AS_UNSET([GNUTLS_CFLAGS])
        AS_UNSET([GNUTLS_LDFLAGS])
        AS_UNSET([GNUTLS_LIBS])
      ]
    )
    AC_SUBST([GNUTLS_LIB_VERSION],[${GNUTLS_MOD_VERSION}])
    AM_SUBST_NOTMAKE([GNUTLS_LIB_VERSION])

    AC_MSG_CHECKING([[how to find OpenSSL library]])
    AC_ARG_WITH([[openssl]],
      [AS_HELP_STRING([[--with-openssl[=PRFX]]],
          [use OpenSSL for HTTPS support, optional PRFX overrides pkg-config data for OpenSSL headers (PRFX/include) and libs (PRFX/lib)])],
      [
        AS_CASE([$with_openssl],
          [no],[
            have_openssl="no"
            AC_MSG_RESULT([[OpenSSL disabled]])
            AS_UNSET([OPENSSL_CPPFLAGS])
            AS_UNSET([OPENSSL_CFLAGS])
            AS_UNSET([OPENSSL_LDFLAGS])
            AS_UNSET([OPENSSL_LIBS])
          ],
          [yes],[
            AC_MSG_RESULT([[automatically, forced]])
          ],
          [
            AC_MSG_RESULT([[-I$with_openssl/include -L$with_openssl/lib -lssl -lcrypto]])
            LDFLAGS="${LDFLAGS_ac} -L$with_openssl/lib ${user_LDFLAGS}"
            CPPFLAGS="${CPPFLAGS_ac} -I$with_openssl/include ${user_CPPFLAGS}"
            have_openssl_pkgcfg="no"
            # A simple check for the working header and the library
            MHD_CHECK_FUNC([SSL_CTX_new],
              [[
#include <openssl/ssl.h>
              ]],
              [[
               SSL_CTX_free(SSL_CTX_new(TLS_server_method()));
              ]],
              [
                have_openssl="yes"
                OPENSSL_CPPFLAGS="-I$with_openssl/include"
                AS_UNSET([OPENSSL_CFLAGS])
                OPENSSL_LDFLAGS="-L$with_openssl/lib"
                OPENSSL_LIBS="-lssl -lcrypto"
              ],
              [AC_MSG_ERROR([cannot find usable OpenSSL library at specified prefix $with_openssl])],
              [-lssl -lcrypto]
            )
            CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
            CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
            LDFLAGS="${LDFLAGS_ac} ${user_LDFLAGS}"
          ])
       ],
       [AC_MSG_RESULT([[automatically]])]
    )

    AS_IF([test "x$with_openssl" != "xno" && test "x$have_openssl" != "xyes"],
      [
        PKG_CHECK_MODULES([OPENSSL], [[openssl >= 3.0]],
          [
            CPPFLAGS="${CPPFLAGS_ac} $OPENSSL_CFLAGS ${user_CPPFLAGS}"
            # A simple check for the working header and the library
            MHD_CHECK_FUNC([SSL_CTX_new],
              [[
#include <openssl/ssl.h>
              ]],
              [[
               SSL_CTX_free(SSL_CTX_new(TLS_server_method()));
              ]],
              [
                have_openssl=yes
                have_openssl_pkgcfg='yes'
                # OPENSSL_CFLAGS is actually CPPFLAGS
                OPENSSL_CPPFLAGS="$OPENSSL_CFLAGS"
                AS_UNSET([OPENSSL_CFLAGS])
                # OPENSSL_LIBS is a combination of LDFLAGS and LIBS
                AS_UNSET([OPENSSL_LDFLAGS])
              ],
              [
                AS_VAR_IF([with_openssl],["yes"],
                  [AC_MSG_ERROR([cannot find usable OpenSSL library])]
                )
                AC_MSG_WARN([pkg-config reports that OpenSSL is present, but OpenSSL cannot be used])
                AS_UNSET([OPENSSL_CPPFLAGS])
                AS_UNSET([OPENSSL_CFLAGS])
                AS_UNSET([OPENSSL_LDFLAGS])
                AS_UNSET([OPENSSL_LIBS])
              ],
              [$OPENSSL_LIBS]
            )
          ],
          [
            # check for OpenSSL at default paths
            have_openssl_pkgcfg="no"
            # A simple check for the working header and the library
            MHD_CHECK_FUNC([SSL_CTX_new],
              [[
#include <openssl/ssl.h>
              ]],
              [[
               SSL_CTX_free(SSL_CTX_new(TLS_server_method()));
              ]],
              [
                have_openssl="yes"
                AS_UNSET([OPENSSL_CPPFLAGS])
                AS_UNSET([OPENSSL_CFLAGS])
                AS_UNSET([OPENSSL_LDFLAGS])
                OPENSSL_LIBS="-lssl -lcrypto"
              ],
              [
                AS_VAR_IF([with_openssl],["yes"],
                  [AC_MSG_ERROR([cannot find usable OpenSSL library])]
                )
                AS_UNSET([OPENSSL_CPPFLAGS])
                AS_UNSET([OPENSSL_CFLAGS])
                AS_UNSET([OPENSSL_LDFLAGS])
                AS_UNSET([OPENSSL_LIBS])
              ],
              [-lssl -lcrypto]
            )
          ]
        )
      ]
    )
    AS_VAR_IF([have_openssl],["yes"],
      [
        AC_CACHE_CHECK([[whether OpenSSL is modern enough]], [mhd_cv_openssl_ver_ok],
          [
            CPPFLAGS="${CPPFLAGS_ac} ${OPENSSL_CPPFLAGS} ${user_CPPFLAGS}"
            AC_COMPILE_IFELSE(
              [
                AC_LANG_PROGRAM(
                  [[
#include <openssl/opensslv.h>
                  ]],
                  [[
#ifndef OPENSSL_VERSION_PREREQ
#error OPENSSL_VERSION_PREREQ is not defined. OpenSSL is too old.
fail test here %%%@<:@-1@:>@
#endif
#if !OPENSSL_VERSION_PREREQ(3,0)
#error OpenSSL version is too old and not suitable.
fail test here %%%@<:@-1@:>@
#endif
                  ]]
                )
              ],
              [[mhd_cv_openssl_ver_ok="yes"]], [[mhd_cv_openssl_ver_ok="no"]]
            )
            CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
          ]
        )
        AS_VAR_IF([mhd_cv_openssl_ver_ok],["yes"],[:],
          [
            have_openssl="no"
            AS_VAR_IF([with_openssl],["yes"],
              [AC_MSG_ERROR([cannot find usable OpenSSL library])]
            )
          ]
        )
      ]
    )

    AS_VAR_IF([have_openssl],["yes"],
      [
        AS_VAR_IF([have_openssl_pkgcfg],["yes"],
          [
            MHD_PKG_CONF_MODULE_VERSION([OPENSSL],[openssl >= 3.0],[],[OPENSSL_MOD_VERSION='0'])
          ],[OPENSSL_MOD_VERSION='0']
        )
      ],
      [
        have_openssl="no"
        have_openssl_pkgcfg="no"
        OPENSSL_MOD_VERSION="no"
        AS_UNSET([OPENSSL_CPPFLAGS])
        AS_UNSET([OPENSSL_CFLAGS])
        AS_UNSET([OPENSSL_LDFLAGS])
        AS_UNSET([OPENSSL_LIBS])
      ]
    )
    AC_SUBST([OPENSSL_LIB_VERSION],[${OPENSSL_MOD_VERSION}])
    AM_SUBST_NOTMAKE([OPENSSL_LIB_VERSION])

    AC_MSG_CHECKING([[how to find MbedTLS library]])
    mhd_mbedtls_pkgspec=''
    AC_ARG_WITH([[mbedtls]],
      [AS_HELP_STRING([[--with-mbedtls[=PRFX]]],
          [use MbedTLS for HTTPS support, optional PRFX overrides pkg-config data for MbedTLS headers (PRFX/include) and libs (PRFX/lib)])],
      [
        AS_CASE([$with_mbedtls],
          [no],[
            have_mbedtls="no"
            have_mbedtls_pkgcfg="no"
            AC_MSG_RESULT([[MbedTLS disabled]])
          ],
          [yes],[
            have_mbedtls="find"
            have_mbedtls_pkgcfg="auto"
            AC_MSG_RESULT([[automatically, forced]])
          ],
          [
            AC_MSG_RESULT([[-I$with_mbedtls/include -L$with_mbedtls/lib]])
            
            AS_UNSET([MBEDTLS_FULL_CPPFLAGS])
            AS_UNSET([MBEDTLS_FULL_CFLAGS])
            AS_UNSET([MBEDTLS_FULL_LDFLAGS])
            AS_UNSET([MBEDTLS_FULL_LIBS])
            AS_UNSET([MBEDTLS_CRYPTO_CPPFLAGS])
            AS_UNSET([MBEDTLS_CRYPTO_CFLAGS])
            AS_UNSET([MBEDTLS_CRYPTO_LDFLAGS])
            AS_UNSET([MBEDTLS_CRYPTO_LIBS])
            AS_UNSET([MBEDTLS_X509_CPPFLAGS])
            AS_UNSET([MBEDTLS_X509_CFLAGS])
            AS_UNSET([MBEDTLS_X509_LDFLAGS])
            AS_UNSET([MBEDTLS_X509_LIBS])
            AS_UNSET([MBEDTLS_TLS_CPPFLAGS])
            AS_UNSET([MBEDTLS_TLS_CFLAGS])
            AS_UNSET([MBEDTLS_TLS_LDFLAGS])
            AS_UNSET([MBEDTLS_TLS_LIBS])
            have_mbedtls="find"
            have_mbedtls_pkgcfg="no"
          ]
        )
      ],
      [
        have_mbedtls_pkgcfg="auto"
        with_mbedtls=""
        AC_MSG_RESULT([[automatically]])
      ]
    )

    AS_IF([test "x$with_mbedtls" != "xno" && test "x$have_mbedtls" != "xyes"],
      [
        AS_VAR_SET_IF([MBEDTLS_CRYPTO_CFLAGS],
          [have_user_MBEDTLS_CRYPTO_CFLAGS="yes"],
          [have_user_MBEDTLS_CRYPTO_CFLAGS="no"]
        )
        AS_VAR_SET_IF([MBEDTLS_CRYPTO_LIBS],
          [have_user_MBEDTLS_CRYPTO_LIBS="yes"],
          [have_user_MBEDTLS_CRYPTO_LIBS="no"]
        )
        AS_VAR_SET_IF([MBEDTLS_X509_CFLAGS],
          [have_user_MBEDTLS_X509_CFLAGS="yes"],
          [have_user_MBEDTLS_X509_CFLAGS="no"]
        )
        AS_VAR_SET_IF([MBEDTLS_X509_LIBS],
          [have_user_MBEDTLS_X509_LIBS="yes"],
          [have_user_MBEDTLS_X509_LIBS="no"]
        )
        AS_VAR_SET_IF([MBEDTLS_TLS_CFLAGS],
          [have_user_MBEDTLS_TLS_CFLAGS="yes"],
          [have_user_MBEDTLS_TLS_CFLAGS="no"]
        )
        AS_VAR_SET_IF([MBEDTLS_TLS_LIBS],
          [have_user_MBEDTLS_TLS_LIBS="yes"],
          [have_user_MBEDTLS_TLS_LIBS="no"]
        )
        AS_IF([test "x$have_mbedtls" != "xyes" && test "x${have_mbedtls_pkgcfg}" != "xno"],
          [
            mbedtsl_min_ver="3.0"
            PKG_CHECK_MODULES([MBEDTLS_CRYPTO], [[mbedcrypto-4 >= 4.0]],
              [
                mbedtsl_min_ver="4.0"
                mbedtls_modules="mbedcrypto-4"
                PKG_CHECK_MODULES([MBEDTLS_X509], [[mbedx509-4 >= ${mbedtsl_min_ver}]],
                  [
                    mbedtls_modules="${mbedtls_modules}, mbedx509-4"
                    PKG_CHECK_MODULES([MBEDTLS_TLS], [[mbedtls-4 >= ${mbedtsl_min_ver}]],
                      [
                        mbedtls_modules="${mbedtls_modules} and mbedtls-4"
                        have_mbedtls_pkgcfg="yes"
                        have_mbedtls="yes"
                        mhd_mbedtls_pkgspec="mbedtls-4 >= ${mbedtsl_min_ver}"
                      ],[]
                    )
                  ],[]
                )
              ],[]
            )
            AS_VAR_IF([have_mbedtls],["yes"],[:],
              [
                AS_VAR_IF([have_user_MBEDTLS_CRYPTO_CFLAGS],["no"],[AS_UNSET([MBEDTLS_CRYPTO_CFLAGS])])
                AS_VAR_IF([have_user_MBEDTLS_CRYPTO_LIBS],["no"],[AS_UNSET([MBEDTLS_CRYPTO_LIBS])])
                AS_VAR_IF([have_user_MBEDTLS_X509_CFLAGS],["no"],[AS_UNSET([MBEDTLS_X509_CFLAGS])])
                AS_VAR_IF([have_user_MBEDTLS_X509_LIBS],["no"],[AS_UNSET([MBEDTLS_X509_LIBS])])
                AS_VAR_IF([have_user_MBEDTLS_TLS_CFLAGS],["no"],[AS_UNSET([MBEDTLS_TLS_CFLAGS])])
                AS_VAR_IF([have_user_MBEDTLS_TLS_LIBS],["no"],[AS_UNSET([MBEDTLS_TLS_LIBS])])
                PKG_CHECK_MODULES([MBEDTLS_CRYPTO], [[mbedcrypto-3 >= ${mbedtsl_min_ver}]],
                  [
                    mbedtls_modules="mbedcrypto-3"
                    PKG_CHECK_MODULES([MBEDTLS_X509], [[mbedx509-3 >= ${mbedtsl_min_ver}]],
                      [
                        mbedtls_modules="${mbedtls_modules}, mbedx509-3"
                        PKG_CHECK_MODULES([MBEDTLS_TLS], [[mbedtls-3 >= ${mbedtsl_min_ver}]],
                          [
                            mbedtls_modules="${mbedtls_modules} and mbedtls-3"
                            have_mbedtls_pkgcfg="yes"
                            have_mbedtls="yes"
                            mhd_mbedtls_pkgspec="mbedtls-3 >= ${mbedtsl_min_ver}"
                          ],[]
                        )
                      ],[]
                    )
                  ],[]
                )
              ]
            )
            AS_VAR_IF([have_mbedtls],["yes"],[:],
              [
                AS_VAR_IF([have_user_MBEDTLS_CRYPTO_CFLAGS],["no"],[AS_UNSET([MBEDTLS_CRYPTO_CFLAGS])])
                AS_VAR_IF([have_user_MBEDTLS_CRYPTO_LIBS],["no"],[AS_UNSET([MBEDTLS_CRYPTO_LIBS])])
                AS_VAR_IF([have_user_MBEDTLS_X509_CFLAGS],["no"],[AS_UNSET([MBEDTLS_X509_CFLAGS])])
                AS_VAR_IF([have_user_MBEDTLS_X509_LIBS],["no"],[AS_UNSET([MBEDTLS_X509_LIBS])])
                AS_VAR_IF([have_user_MBEDTLS_TLS_CFLAGS],["no"],[AS_UNSET([MBEDTLS_TLS_CFLAGS])])
                AS_VAR_IF([have_user_MBEDTLS_TLS_LIBS],["no"],[AS_UNSET([MBEDTLS_TLS_LIBS])])
                PKG_CHECK_MODULES([MBEDTLS_CRYPTO], [[mbedcrypto >= ${mbedtsl_min_ver}]],
                  [
                    mbedtls_modules="mbedcrypto"
                    PKG_CHECK_MODULES([MBEDTLS_X509], [[mbedx509 >= ${mbedtsl_min_ver}]],
                      [
                        mbedtls_modules="${mbedtls_modules}, mbedx509"
                        PKG_CHECK_MODULES([MBEDTLS_TLS], [[mbedtls >= ${mbedtsl_min_ver}]],
                          [
                            mbedtls_modules="${mbedtls_modules} and mbedtls"
                            have_mbedtls_pkgcfg="yes"
                            have_mbedtls="yes"
                            mhd_mbedtls_pkgspec="mbedtls >= ${mbedtsl_min_ver}"
                          ],[]
                        )
                      ],[]
                    )
                  ],[]
                )
              ]
            )
            AS_VAR_IF([have_mbedtls],["yes"],
              [
                AS_UNSET([MBEDTLS_FULL_CPPFLAGS])
                AS_UNSET([MBEDTLS_FULL_CFLAGS])
                AS_UNSET([MBEDTLS_FULL_LDFLAGS])
                AS_UNSET([MBEDTLS_FULL_LIBS])
                # pkg-config's *_CFLAGS are actually *_CPPFLAGS
                MBEDTLS_CRYPTO_CPPFLAGS="${MBEDTLS_CRYPTO_CFLAGS}"
                MBEDTLS_X509_CPPFLAGS="${MBEDTLS_X509_CFLAGS}"
                MBEDTLS_TLS_CPPFLAGS="${MBEDTLS_TLS_CFLAGS}"
                AS_UNSET([MBEDTLS_CRYPTO_CFLAGS])
                AS_UNSET([MBEDTLS_X509_CFLAGS])
                AS_UNSET([MBEDTLS_TLS_CFLAGS])
                # pkg-config's  *_LIBS are combinations of *_LDFLAGS and *_LIBS
                AS_UNSET([MBEDTLS_CRYPTO_LDFLAGS])
                AS_UNSET([MBEDTLS_X509_LDFLAGS])
                AS_UNSET([MBEDTLS_TLS_LDFLAGS])
                
                # Smart-combine three libraries flags
                MBEDTLS_FULL_CPPFLAGS="${MBEDTLS_CRYPTO_CPPFLAGS}"
                AS_CASE([" ${MBEDTLS_X509_CPPFLAGS} "],
                  [*" ${MBEDTLS_FULL_CPPFLAGS} "*],[:],
                  [
                    AS_CASE([" ${MBEDTLS_FULL_CPPFLAGS} "],
                      [*" ${MBEDTLS_X509_CPPFLAGS} "*],[MBEDTLS_FULL_CPPFLAGS="${MBEDTLS_X509_CPPFLAGS}"],
                      [MBEDTLS_FULL_CPPFLAGS="${MBEDTLS_FULL_CPPFLAGS} ${MBEDTLS_X509_CPPFLAGS}"]
                    )
                  ]
                )
                AS_CASE([" ${MBEDTLS_TLS_CPPFLAGS} "],
                  [*" ${MBEDTLS_FULL_CPPFLAGS} "*],[:],
                  [
                    AS_CASE([" ${MBEDTLS_FULL_CPPFLAGS} "],
                      [*" ${MBEDTLS_TLS_CPPFLAGS} "*],[MBEDTLS_FULL_CPPFLAGS="${MBEDTLS_TLS_CPPFLAGS}"],
                      [MBEDTLS_FULL_CPPFLAGS="${MBEDTLS_FULL_CPPFLAGS} ${MBEDTLS_TLS_CPPFLAGS}"]
                    )
                  ]
                )
                MBEDTLS_FULL_LIBS="${MBEDTLS_CRYPTO_LIBS}"
                AS_CASE([" ${MBEDTLS_X509_LIBS} "],
                  [*" ${MBEDTLS_FULL_LIBS} "*],[:],
                  [
                    AS_CASE([" ${MBEDTLS_FULL_LIBS} "],
                      [*" ${MBEDTLS_X509_LIBS} "*],[MBEDTLS_FULL_LIBS="${MBEDTLS_X509_LIBS}"],
                      [MBEDTLS_FULL_LIBS="${MBEDTLS_X509_LIBS} ${MBEDTLS_FULL_LIBS}"]
                    )
                  ]
                )
                AS_CASE([" ${MBEDTLS_TLS_LIBS} "],
                  [*" ${MBEDTLS_FULL_LIBS} "*],[:],
                  [
                    AS_CASE([" ${MBEDTLS_FULL_LIBS} "],
                      [*" ${MBEDTLS_TLS_LIBS} "*],[MBEDTLS_FULL_LIBS="${MBEDTLS_TLS_LIBS}"],
                      [MBEDTLS_FULL_LIBS="${MBEDTLS_TLS_LIBS} ${MBEDTLS_FULL_LIBS}"]
                    )
                  ]
                )
                
                CPPFLAGS="${CPPFLAGS_ac} ${MBEDTLS_FULL_CPPFLAGS} ${user_CPPFLAGS}"
                
                MHD_CHECK_FUNC([mbedtls_ssl_init],[[#include <mbedtls/ssl.h>]],
                  [[
                    mbedtls_ssl_context ssl;
                    mbedtls_ssl_init (&ssl);
                    mbedtls_ssl_free (&ssl);
                  ]],
                  [],
                  [AC_MSG_ERROR([pkg-config reports that $mbedtls_modules modules are present, but MbedTLS cannot be used])],
                  ["${MBEDTLS_FULL_LIBS}"]
                )
                
                CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
              ],
              [
                AS_UNSET([mbedtls_modules])
                AS_VAR_IF([have_user_MBEDTLS_CRYPTO_CFLAGS],["no"],[AS_UNSET([MBEDTLS_CRYPTO_CFLAGS])])
                AS_VAR_IF([have_user_MBEDTLS_CRYPTO_LIBS],["no"],[AS_UNSET([MBEDTLS_CRYPTO_LIBS])])
                AS_VAR_IF([have_user_MBEDTLS_X509_CFLAGS],["no"],[AS_UNSET([MBEDTLS_X509_CFLAGS])])
                AS_VAR_IF([have_user_MBEDTLS_X509_LIBS],["no"],[AS_UNSET([MBEDTLS_X509_LIBS])])
                AS_VAR_IF([have_user_MBEDTLS_TLS_CFLAGS],["no"],[AS_UNSET([MBEDTLS_TLS_CFLAGS])])
                AS_VAR_IF([have_user_MBEDTLS_TLS_LIBS],["no"],[AS_UNSET([MBEDTLS_TLS_LIBS])])
              ]
            )
            AS_UNSET([have_user_MBEDTLS_CRYPTO_CFLAGS])
            AS_UNSET([have_user_MBEDTLS_CRYPTO_LIBS])
            AS_UNSET([have_user_MBEDTLS_X509_CFLAGS])
            AS_UNSET([have_user_MBEDTLS_X509_LIBS])
            AS_UNSET([have_user_MBEDTLS_TLS_CFLAGS])
            AS_UNSET([have_user_MBEDTLS_TLS_LIBS])
          ]
        )

        AS_VAR_IF([have_mbedtls],["yes"],[:],
          [
            have_mbedtls_pkgcfg="no"
            AS_CASE(["x${with_mbedtls}"],
              ["xyes"],[
                MBEDTLS_FULL_CPPFLAGS=""
                MBEDTLS_FULL_LDFLAGS=""
                ],
              ["x"],[
                MBEDTLS_FULL_CPPFLAGS=""
                MBEDTLS_FULL_LDFLAGS=""
                ],
              [
                MBEDTLS_FULL_CPPFLAGS="-I$with_mbedtls/include"
                MBEDTLS_FULL_LDFLAGS="-L$with_mbedtls/lib"
              ]
            )
            CPPFLAGS="${CPPFLAGS_ac} ${MBEDTLS_FULL_CPPFLAGS} ${user_CPPFLAGS}"
            LDFLAGS="${LDFLAGS_ac} ${MBEDTLS_FULL_LDFLAGS} ${user_LDFLAGS}"

            MHD_FIND_LIB([mbedtls_md_init],[[#include <mbedtls/md.h>]],
              [[
                /* These functions must be enabled i][f TLS is built */
                mbedtls_md_context_t ctx;
                mbedtls_md_init (&ctx);
                mbedtls_md_free (&ctx);
              ]],
              [tfpsacrypto-4 tfpsacrypto mbedcrypto-3 mbedcrypto],
              [
                AS_CASE([${MBEDTLS_CRYPTO_LIBS}],
                  [*-4],[check_names_x509="mbedx509-4"
                         check_names_tls="mbedtls-4"],
                  [*-3],[check_names_x509="mbedx509-3"
                         check_names_tls="mbedtls-3"],
                  [*crypto],[check_names_x509="mbedx509"
                             check_names_tls="mbedtls"],
                  [check_names_x509="mbedx509-4 mbedx509-3 mbedx509"
                   check_names_tls="mbedtls-4 mbedtls-3 mbedtls"]
                )
                MHD_FIND_LIB([mbedtls_x509_crt_init],[[#include <mbedtls/x509_crt.h>]],
                  [[
                    /* These functions must be enabled i][f TLS based on X509 certificates is built */
                    mbedtls_x509_crt crt;
                    mbedtls_x509_crt_init(&crt);
                    mbedtls_x509_crt_free(&crt);
                  ]],
                  [${check_names_x509}],
                  [
                    AS_CASE([${MBEDTLS_X509_LIBS}],
                      [*-4],[check_names_tls="mbedtls-4"],
                      [*-3],[check_names_tls="mbedtls-3"],
                      [*x509],[check_names_tls="mbedtls"]
                    )
                    MHD_FIND_LIB([mbedtls_ssl_init],[[#include <mbedtls/ssl.h>]],
                      [[
                        mbedtls_ssl_context ssl;
                        mbedtls_ssl_init (&ssl);
                        mbedtls_ssl_free (&ssl);
                      ]],
                      [${check_names_tls}],
                      [
                        have_mbedtls="yes"
                        MBEDTLS_FULL_LIBS="${MBEDTLS_TLS_LIBS} ${MBEDTLS_X509_LIBS} ${MBEDTLS_CRYPTO_LIBS}"
                        MBEDTLS_CRYPTO_CPPFLAGS="${MBEDTLS_FULL_CPPFLAGS}"
                        MBEDTLS_CRYPTO_LDFLAGS="${MBEDTLS_FULL_LDFLAGS}"
                        MBEDTLS_X509_CPPFLAGS="${MBEDTLS_FULL_CPPFLAGS}"
                        MBEDTLS_X509_LDFLAGS="${MBEDTLS_FULL_LDFLAGS}"
                        MBEDTLS_TLS_CPPFLAGS="${MBEDTLS_FULL_CPPFLAGS}"
                        MBEDTLS_TLS_LDFLAGS="${MBEDTLS_FULL_LDFLAGS}"
                      ],[],
                      [MBEDTLS_TLS_LIBS],[${MBEDTLS_X509_LIBS} ${MBEDTLS_CRYPTO_LIBS}]
                    )
                  ],[],
                  [MBEDTLS_X509_LIBS],[${MBEDTLS_CRYPTO_LIBS}]
                )
                AS_UNSET([check_names_tls])
                AS_UNSET([check_names_x509])
              ],[],
              [MBEDTLS_CRYPTO_LIBS]
            )
            CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
            LDFLAGS="${LDFLAGS_ac} ${user_LDFLAGS}"
          ]
        )
      ]
    )

    AS_VAR_IF([have_mbedtls],["yes"],
      [
        AS_IF([test -n "${mhd_mbedtls_pkgspec}"],
          [
            MHD_PKG_CONF_MODULE_VERSION([MBEDTLS],[${mhd_mbedtls_pkgspec}],[],[MBEDTLS_MOD_VERSION='0'])
          ],[MBEDTLS_MOD_VERSION='0']
        )
      ],
      [
        have_mbedtls="no"
        AS_CASE(["x$with_mbedtls"],
          ["xyes"],[AC_MSG_ERROR([connot find usable MbedTLS library])],
          ["x"],[with_mbedtls="no"],
          ["no"],[:]
          [AC_MSG_ERROR([cannot find usable MbedTLS library at specified prefix $with_mbedtls])]
        )
        MBEDTLS_MOD_VERSION="no"
        AS_UNSET([MBEDTLS_FULL_CPPFLAGS])
        AS_UNSET([MBEDTLS_FULL_CFLAGS])
        AS_UNSET([MBEDTLS_FULL_LDFLAGS])
        AS_UNSET([MBEDTLS_FULL_LIBS])
        AS_UNSET([MBEDTLS_CRYPTO_CPPFLAGS])
        AS_UNSET([MBEDTLS_CRYPTO_CFLAGS])
        AS_UNSET([MBEDTLS_CRYPTO_LDFLAGS])
        AS_UNSET([MBEDTLS_CRYPTO_LIBS])
        AS_UNSET([MBEDTLS_X509_CPPFLAGS])
        AS_UNSET([MBEDTLS_X509_CFLAGS])
        AS_UNSET([MBEDTLS_X509_LDFLAGS])
        AS_UNSET([MBEDTLS_X509_LIBS])
        AS_UNSET([MBEDTLS_TLS_CPPFLAGS])
        AS_UNSET([MBEDTLS_TLS_CFLAGS])
        AS_UNSET([MBEDTLS_TLS_LDFLAGS])
        AS_UNSET([MBEDTLS_TLS_LIBS])
      ]
    )
    AS_UNSET([mhd_mbedtls_pkgspec])
    AC_SUBST([MBEDTLS_LIB_VERSION],[${MBEDTLS_MOD_VERSION}])
    AM_SUBST_NOTMAKE([MBEDTLS_LIB_VERSION])

    AS_IF([test "x$have_gnutls" = "xyes" || test "x$have_openssl" = "xyes" || test "x$have_mbedtls" = "xyes"],
      [
        enable_https="yes"
        multiple_tls="no"
        MSG_TLS_BACKENDS=""

        AS_VAR_IF([have_gnutls],["yes"],
          [
            AS_IF([test -n "${MSG_TLS_BACKENDS}"],
              [
                MSG_TLS_BACKENDS="${MSG_TLS_BACKENDS}, "
                multiple_tls="yes"
              ]
            )
            AC_DEFINE([MHD_SUPPORT_GNUTLS],[1],[Define to '1' i][f GnuTLS library should be used])
            MSG_TLS_BACKENDS="${MSG_TLS_BACKENDS}GnuTLS"
            MHD_APPEND_FLAG_TO_VAR([MHD_TLS_LIB_CPPFLAGS],[$GNUTLS_CPPFLAGS])
            MHD_APPEND_FLAG_TO_VAR([MHD_TLS_LIB_LDFLAGS],[$GNUTLS_LDFLAGS])
            MHD_PREPEND_FLAG_TO_VAR([MHD_TLS_LIBDEPS],[$GNUTLS_LIBS])
          ]
        )
        AS_VAR_IF([have_openssl],["yes"],
          [
            AS_IF([test -n "${MSG_TLS_BACKENDS}"],
              [
                MSG_TLS_BACKENDS="${MSG_TLS_BACKENDS}, "
                multiple_tls="yes"
              ]
            )
            AC_DEFINE([MHD_SUPPORT_OPENSSL],[1],[Define to '1' i][f OpenSSL library should be used])
            MSG_TLS_BACKENDS="${MSG_TLS_BACKENDS}OpenSSL"
            MHD_APPEND_FLAG_TO_VAR([MHD_TLS_LIB_CPPFLAGS],[$OPENSSL_CPPFLAGS])
            MHD_APPEND_FLAG_TO_VAR([MHD_TLS_LIB_LDFLAGS],[$OPENSSL_LDFLAGS])
            MHD_PREPEND_FLAG_TO_VAR([MHD_TLS_LIBDEPS],[$OPENSSL_LIBS])
          ]
        )
        AS_VAR_IF([have_mbedtls],["yes"],
          [
            AS_IF([test -n "${MSG_TLS_BACKENDS}"],
              [
                MSG_TLS_BACKENDS="${MSG_TLS_BACKENDS}, "
                multiple_tls="yes"
              ]
            )
            AC_DEFINE([MHD_SUPPORT_MBEDTLS],[1],[Define to '1' i][f MbedTLS library should be used])
            MSG_TLS_BACKENDS="${MSG_TLS_BACKENDS}MbedTLS"
            MHD_APPEND_FLAG_TO_VAR([MHD_TLS_LIB_CPPFLAGS],[$MBEDTLS_FULL_CPPFLAGS])
            MHD_APPEND_FLAG_TO_VAR([MHD_TLS_LIB_LDFLAGS],[$MBEDTLS_FULL_LDFLAGS])
            MHD_PREPEND_FLAG_TO_VAR([MHD_TLS_LIBDEPS],[$MBEDTLS_FULL_LIBS])
          ]
        )
        AS_VAR_IF([multiple_tls],["yes"],
          [
            MSG_HTTPS="yes (multiple backends)"
            AC_DEFINE([mhd_HAVE_SEVERAL_TLS_BACKENDS],[1],[Define to '1' if several TLS backend are linked])
          ],
          [MSG_HTTPS="yes (single backend)"]
        )
        AS_IF([test -z "$MSG_TLS_BACKENDS"],[AC_MSG_FAILURE([configure internal error: no TLS backends])])
        AC_DEFINE([MHD_SUPPORT_HTTPS],[1],[Define to '1' i][f HTTPS protocol should be supported])
      ],
      [
        AS_VAR_IF([enable_https],["yes"],
          [AC_MSG_ERROR([no suitable TLS lib found, HTTPS cannot be enabled])]
        )
        enable_https="no"
        MSG_HTTPS="no (no suitable TLS lib found)"
        MSG_TLS_BACKENDS="none"
      ]
    )
  ],
  [
    enable_https="no"
    MSG_HTTPS="no (disabled)"
  ]
)

AC_MSG_CHECKING(whether to support HTTPS)
AS_VAR_IF([enable_https],["yes"],
  [
    AS_CASE([$MSG_HTTPS],[yes*],[:],
      [AC_MSG_FAILURE([configure internal error: wrong MSG_HTTPS])]
    )
  ],
  [
    enable_https="no"
    MSG_TLS_BACKENDS="none"
    AS_UNSET([MHD_TLS_LIB_CPPFLAGS])
    AS_UNSET([MHD_TLS_LIB_LDFLAGS])
    AS_UNSET([MHD_TLS_LIBDEPS])
    AS_CASE([$MSG_HTTPS],[no*],[:],
      [AC_MSG_FAILURE([configure internal error: wrong MSG_HTTPS])]
    )
  ]
)
AC_MSG_RESULT([$MSG_HTTPS])

AM_CONDITIONAL([MHD_SUPPORT_GNUTLS], [[test "x$have_gnutls" = "xyes"]])
AC_SUBST([GNUTLS_CPPFLAGS])
AC_SUBST([GNUTLS_LDFLAGS])
AC_SUBST([GNUTLS_LIBS])
AM_CONDITIONAL([MHD_SUPPORT_OPENSSL], [[test "x$have_openssl" = "xyes"]])
AC_SUBST([OPENSSL_CPPFLAGS])
AC_SUBST([OPENSSL_LDFLAGS])
AC_SUBST([OPENSSL_LIBS])
AM_CONDITIONAL([MHD_SUPPORT_MBEDTLS], [[test "x$have_mbedtls" = "xyes"]])
AC_SUBST([MBEDTLS_CRYPTO_CPPFLAGS])
AC_SUBST([MBEDTLS_CRYPTO_LDFLAGS])
AC_SUBST([MBEDTLS_CRYPTO_LIBS])
AC_SUBST([MBEDTLS_FULL_CPPFLAGS])
AC_SUBST([MBEDTLS_FULL_LDFLAGS])
AC_SUBST([MBEDTLS_FULL_LIBS])
AM_CONDITIONAL([MHD_ENABLE_MULTITLS], [test "x$multiple_tls" = "xyes"])
AM_CONDITIONAL([MHD_SUPPORT_HTTPS], [test "x$enable_https" = "xyes"])

AS_VAR_IF([have_gnutls], ["yes"],
  [
    AC_CACHE_CHECK([for GnuTLS quirks], [mhd_cv_gnutls_mthread_broken],
      [
        mhd_cv_gnutls_mthread_broken="no"
        AS_IF([test -r /etc/redhat-release],
          [
            AS_IF([$FGREP ' release 6.' /etc/redhat-release >/dev/null || $FGREP '(Santiago)' /etc/redhat-release >/dev/null],
              [mhd_cv_gnutls_mthread_broken="found"],
            )
          ]
        )
        AS_VAR_IF([mhd_cv_gnutls_mthread_broken], ["no"],
          [
            AS_IF([command -v rpm >/dev/null],
              [
                AS_IF([test "r`rpm -E '%{rhel}' 2>/dev/null`" = "r6"],
                  [mhd_cv_gnutls_mthread_broken="found"],
                )
              ]
            )
          ]
        )
      ]
    )
    AC_CACHE_CHECK([for gnutls-cli binary], [mhd_cv_gnutls_cli],
      [
        mhd_cv_gnutls_cli="no"
        AS_IF([command -v gnutls-cli >/dev/null 2>&1],
          [AS_IF([AC_RUN_LOG([gnutls-cli --version >&2])], [mhd_cv_gnutls_cli="yes"])]
        )
      ]
    )
  ]
)
AM_CONDITIONAL([HAVE_GNUTLS_MTHREAD_BROKEN], [[test "x${mhd_cv_gnutls_mthread_broken}" = "xfound"]])
AM_CONDITIONAL([USE_UPGRADE_TLS_TESTS], [[test "x${mhd_cv_gnutls_mthread_broken}" = "xno" || test "x${mhd_cv_gnutls_cli}" = "xyes"]])

# optional: HTTP Basic Auth support. Enabled by default
AC_MSG_CHECKING([[whether to support HTTP Basic authentication]])
AC_ARG_ENABLE([basic-auth],
  [AS_HELP_STRING([--disable-basic-auth],[disable HTTP Basic Authentication support])],
  [],
  [enable_basic_auth="yes"]
)
AS_IF([[test "x$enable_basic_auth" != "xno"]],
  [
    enable_basic_auth="yes"
    AC_DEFINE([MHD_SUPPORT_AUTH_BASIC],[1],[Define to '1' if libmicrohttpd should be compiled with Basic Auth support.])
  ]
)
AM_CONDITIONAL([MHD_SUPPORT_AUTH_BASIC], [test "x$enable_basic_auth" = "xyes"])
AC_MSG_RESULT([[$enable_basic_auth]])

# optional: HTTP Digest Auth support. Enabled by default
AC_MSG_CHECKING([[whether to support HTTP Digest authentication]])
AC_ARG_ENABLE([digest-auth],
  [AS_HELP_STRING([--disable-digest-auth], [disable HTTP Digest Authentication support])],
  [],
  [enable_digest_auth="yes"]
)
AS_IF([[test "x$enable_digest_auth" != "xno"]],
  [ enable_digest_auth=yes
    AC_DEFINE([MHD_SUPPORT_AUTH_DIGEST],[1],[Define to 1 if libmicrohttpd is compiled with Digest Auth support.]) ])
AM_CONDITIONAL([MHD_SUPPORT_AUTH_DIGEST], [test "x$enable_digest_auth" != "xno"])
AC_MSG_RESULT([[$enable_digest_auth]])

AS_VAR_IF([enable_digest_auth], ["yes"],
  [
    AC_MSG_CHECKING([[for Digest Authentication default nonce timeout value]])
    AC_ARG_ENABLE([digest-auth-def-timeout],
      [AS_HELP_STRING([--enable-digest-auth-def-timeout=NNN],
                      [set default Digest Auth nonce validity time to NNN seconds])],
      [],
      [enable_digest_auth_def_timeout=""]
    )
    AS_VAR_IF([enable_digest_auth_def_timeout], ["no"],
      [
        AC_MSG_WARN([Default Diget Auth nonce validity time cannot be disabled, --disable-digest-auth-def-timeout parameter is ignored])
        enable_digest_auth_def_timeout=""
      ]
    )
    AS_IF([test -z "${enable_digest_auth_def_timeout}"],
      [
        enable_digest_auth_def_timeout="90"
        enable_dauth_def_timeout_MSG="${enable_digest_auth_def_timeout} seconds (default)"
      ],
      [
        AS_IF([[test "x${enable_digest_auth_def_timeout}" = "x`echo ${enable_digest_auth_def_timeout}|${SED-sed} 's/[^0-9]//g'`" && \
                test "${enable_digest_auth_def_timeout}" -ge "0" 2>/dev/null ]],[:],
          [AC_MSG_ERROR([Invalid parameter --enable-digest-auth-def-timeout=${enable_digest_auth_def_timeout}. Timeout value must be a positive integer.])
          ]
        )
        AC_COMPILE_IFELSE(
          [
            AC_LANG_PROGRAM([],
              [[
  static int arr[((int) 2) - 4 * (int)(${enable_digest_auth_def_timeout} != ((unsigned int)${enable_digest_auth_def_timeout}))];
  (void) arr;
              ]]
            )
          ],
          [],
          [AC_MSG_ERROR([The value specified by --enable-digest-auth-def-timeout=${enable_digest_auth_def_timeout} is too large.])]
        )
        enable_dauth_def_timeout_MSG="${enable_digest_auth_def_timeout} seconds (set by parameter)"
      ]
    )
    AC_DEFINE_UNQUOTED([MHD_AUTH_DIGEST_DEF_TIMEOUT],[${enable_digest_auth_def_timeout}],
        [The default HTTP Digest Auth default nonce timeout value (in seconds)])
    AC_MSG_RESULT([[${enable_dauth_def_timeout_MSG}]])

    AC_MSG_CHECKING([[for Digest Authentication default maximum nc value]])
    AC_ARG_ENABLE([digest-auth-def-max-nc],
      [AS_HELP_STRING([--enable-digest-auth-def-max-nc=NNN],
                      [set default Digest Auth maximum nc (nonce count) value to NNN])],
      [],
      [enable_digest_auth_def_max_nc=""]
    )
    AS_VAR_IF([enable_digest_auth_def_max_nc], ["no"],
      [
        AC_MSG_WARN([Default Diget Auth maximum nc cannot be disabled, --disable-digest-auth-def-max-nc parameter is ignored])
        enable_digest_auth_def_max_nc=""
      ]
    )
    AS_IF([test -z "${enable_digest_auth_def_max_nc}"],
      [
        enable_digest_auth_def_max_nc="1000"
        enable_dauth_def_max_nc_MSG="${enable_digest_auth_def_max_nc} (default)"
      ],
      [
        AS_IF([[test "x${enable_digest_auth_def_max_nc}" = "x`echo ${enable_digest_auth_def_max_nc}|${SED-sed} 's/[^0-9]//g'`" && \
                test "${enable_digest_auth_def_max_nc}" -ge "0" 2>/dev/null ]],[:],
          [AC_MSG_ERROR([Invalid parameter --enable-digest-auth-def-max-nc=${enable_digest_auth_def_max_nc}. The value must be a positive integer.])
          ]
        )
        AC_COMPILE_IFELSE(
          [
            AC_LANG_PROGRAM(
              [[
#include <stdint.h>
              ]],
              [[
  static int arr[((int) 2) - 4 * (int)(${enable_digest_auth_def_max_nc} != ((uint_fast32_t)${enable_digest_auth_def_max_nc}))];
  (void) arr;
              ]]
            )
          ],
          [],
          [AC_MSG_ERROR([The value specified by --enable-digest-auth-def-max-nc=${enable_digest_auth_def_max_nc} is too large.])]
        )
        enable_dauth_def_max_nc_MSG="${enable_digest_auth_def_max_nc} (set by parameter)"
      ]
    )
    AC_DEFINE_UNQUOTED([MHD_AUTH_DIGEST_DEF_MAX_NC],[${enable_digest_auth_def_max_nc}],
        [The default HTTP Digest Auth default maximum nc (nonce count) value])
    AC_MSG_RESULT([[${enable_dauth_def_max_nc_MSG}]])

    dauth_defs_MSG="timeout: ${enable_dauth_def_timeout_MSG}, max nc: ${enable_dauth_def_max_nc_MSG}"
  ],
  [
    dauth_defs_MSG="N/A"
  ]
)

AM_CONDITIONAL([MHD_SUPPORT_AUTH],[test "x$enable_basic_auth" != "xno" || test "x$enable_digest_auth" != "xno"])

# optional: HTTP "Upgrade" support. Enabled by default
AC_MSG_CHECKING([[whether to support HTTP "Upgrade"]])
AC_ARG_ENABLE([[httpupgrade]],
    [AS_HELP_STRING([[--disable-httpupgrade]], [disable HTTP "Upgrade" support])],
    [AS_VAR_IF([[enable_httpupgrade]],[["no"]],[],[[enable_httpupgrade='yes']])],
    [[enable_httpupgrade='yes']])
AS_VAR_IF([[enable_httpupgrade]],[["yes"]],
  [
   AC_DEFINE([[MHD_SUPPORT_UPGRADE]],[[1]],[Define to 1 if libmicrohttpd is compiled with HTTP Upgrade support.]) ])
AM_CONDITIONAL([MHD_SUPPORT_UPGRADE], [[test "x$enable_httpupgrade" = "xyes"]])
AC_MSG_RESULT([[$enable_httpupgrade]])

# optional: HTTP cookie parsing support. Enabled by default
AC_MSG_CHECKING([[whether to support HTTP cookie parsing]])
AC_ARG_ENABLE([[cookie]],
    [AS_HELP_STRING([[--disable-cookie]], [disable HTTP cookie parser support])],
    [AS_VAR_IF([[enable_cookie]],[["no"]],[],[[enable_cookie='yes']])],
    [[enable_cookie='yes']])
AS_VAR_IF([[enable_cookie]],[["yes"]],
  [
   AC_DEFINE([[MHD_SUPPORT_COOKIES]],[[1]],[Define to 1 if libmicrohttpd is compiled with HTTP cookie parsing support.]) ])
AM_CONDITIONAL([MHD_SUPPORT_COOKIES], [[test "x$enable_cookie" = "xyes"]])
AC_MSG_RESULT([[$enable_cookie]])

# optional: MD5 support for Digest Auth. Enabled by default.
AC_ARG_ENABLE([[md5]],
  [AS_HELP_STRING([[--enable-md5=TYPE]],
    [enable TYPE of MD5 hashing code (yes, no, builtin, tlslib) [yes if dauth enabled]])],
  [
    AS_VAR_IF([enable_md5],["internal"],[enable_md5='builtin'])
    AS_VAR_IF([enable_md5],["built-in"],[enable_md5='builtin'])
    AS_VAR_IF([enable_digest_auth],["yes"],[],
      [
        AS_VAR_IF([enable_md5],["no"],[],
          [
            AC_MSG_WARN([The parameter --enable-md5=${enable_md5} is ignored as Digest Authentication is disabled])
            enable_md5='no'
          ]
        )
      ]
    )
  ], [[enable_md5="${enable_digest_auth}"]]
)
AS_UNSET([have_md5_gnutls])
AS_UNSET([have_md5_openssl])
AS_UNSET([have_md5_mbedtls])
AS_CASE([${enable_md5}],[yes|tlslib],
  [
    found_md5_tls="no"
    AS_IF([test "x$enable_https" = "xyes"],
      [
        # Check GnuTLS
        AS_VAR_IF([have_gnutls],["yes"],
          [
            AC_CACHE_CHECK([whether GnuTLS supports MD5 hashing],[mhd_cv_gnutls_md5],
              [
                CPPFLAGS="${CPPFLAGS_ac} ${GNUTLS_CPPFLAGS} ${user_CPPFLAGS}"
                CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
                LDFLAGS="${LDFLAGS_ac} ${GNUTLS_LDFLAGS} ${user_LDFLAGS}"
                save_LIBS="$LIBS"
                LIBS="${GNUTLS_LIBS} ${LIBS}"
                AC_LINK_IFELSE(
                  [
                    AC_LANG_PROGRAM(
                      [[
#include <gnutls/crypto.h>
                      ]],
                      [[
    gnutls_hash_hd_t hash_handle;
    unsigned char digest[16];
    int exit_code;

    if (0 == gnutls_hash_init(&hash_handle, GNUTLS_DIG_MD5))
    {
      if (0 == gnutls_hash(hash_handle, "", 1))
      {
        gnutls_hash_output(hash_handle, digest);
        if (0x93 == digest[0])
          exit_code = 0;
        else
          exit_code = 7;
      }
      else
        exit_code = 5;
      gnutls_hash_deinit(hash_handle, (void *)0);
    }
    else
      exit_code = 2;
    return exit_code;
                      ]]
                    )
                  ],
                  [mhd_cv_gnutls_md5='yes'],[mhd_cv_gnutls_md5='no']
                )
                LIBS="${save_LIBS}"
                CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
                CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
                CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
                LDFLAGS="${LDFLAGS_ac} ${user_LDFLAGS}"
              ]
            )
            AS_VAR_IF([mhd_cv_gnutls_md5],["no"],
              [
                AS_VAR_IF([enable_md5],["tlslib"],
                  [AC_MSG_WARN([GnuTLS MD5 implementation is not available])]
                )
              ],
              [
                AC_DEFINE([[MHD_MD5_EXTR_GNUTLS]],[[1]],
                  [Define to 1 if libmicrohttpd is compiled with MD5 hashing by GnuTLS.])
                have_md5_gnutls="yes"
                found_md5_tls="yes"
              ]
            )
          ]
        ) # end GnuTLS check

        # Check OpenSSL
        AS_VAR_IF([have_openssl],["yes"],
           [
            AC_CACHE_CHECK([whether OpenSSL supports MD5 hashing],[mhd_cv_openssl_md5],
              [
                CPPFLAGS="${CPPFLAGS_ac} ${OPENSSL_CPPFLAGS} ${user_CPPFLAGS}"
                CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
                LDFLAGS="${LDFLAGS_ac} ${OPENSSL_LDFLAGS} ${user_LDFLAGS}"
                save_LIBS="$LIBS"
                LIBS="${OPENSSL_LIBS} ${LIBS}"
                AC_LINK_IFELSE(
                  [
                    AC_LANG_PROGRAM(
                      [[
#include <openssl/evp.h>
                      ]],
                      [[
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    unsigned char digest[16];
    unsigned int len;
    if (ctx) {
      EVP_DigestInit_ex(ctx, EVP_md5(), NULL);
      EVP_DigestUpdate(ctx, "", 1);
      EVP_DigestFinal_ex(ctx, digest, &len);
      EVP_MD_CTX_free(ctx);
    }
                      ]]
                    )
                  ],
                  [mhd_cv_openssl_md5='yes'],[mhd_cv_openssl_md5='no']
                )
                LIBS="${save_LIBS}"
                CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
                CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
                LDFLAGS="${LDFLAGS_ac} ${user_LDFLAGS}"
              ]
            )
            AS_VAR_IF([mhd_cv_openssl_md5],["no"],
              [
                AS_VAR_IF([enable_md5],["tlslib"],
                  [AC_MSG_WARN([OpenSSL MD5 implementation is not available])]
                )
              ],
              [
                AC_DEFINE([[MHD_MD5_EXTR_OPENSSL]],[[1]],
                  [Define to 1 if libmicrohttpd is compiled with MD5 hashing by OpenSSL.])
                have_md5_openssl="yes"
                found_md5_tls="yes"
              ]
            )
          ]
        )

        # Check MbedTLS
        AS_VAR_IF([have_mbedtls],["yes"],
          [
            AC_CACHE_CHECK([whether MbedTLS supports MD5 hashing],[mhd_cv_mbedtls_md5],
              [
                CPPFLAGS="${CPPFLAGS_ac} ${MBEDTLS_CRYPTO_CPPFLAGS} ${user_CPPFLAGS}"
                CFLAGS="${CFLAGS_ac} ${MBEDTLS_CRYPTO_CFLAGS} ${user_CFLAGS}"
                LDFLAGS="${LDFLAGS_ac} ${MBEDTLS_CRYPTO_LDFLAGS} ${user_LDFLAGS}"
                save_LIBS="$LIBS"
                LIBS="${MBEDTLS_CRYPTO_LIBS} ${LIBS}"
                AC_LINK_IFELSE(
                  [
                    AC_LANG_PROGRAM(
                      [[
#include <mbedtls/md5.h>
                      ]],
                      [[
    mbedtls_md5_context ctx;
    unsigned char digest[16];
    mbedtls_md5_init(&ctx);
    mbedtls_md5_starts(&ctx);
    mbedtls_md5_update(&ctx, (const unsigned char *)"", 1);
    mbedtls_md5_finish(&ctx, digest);
    mbedtls_md5_free(&ctx);
                      ]]
                    )
                  ],
                  [mhd_cv_mbedtls_md5='yes'],[mhd_cv_mbedtls_md5='no']
                )
                LIBS="${save_LIBS}"
                CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
                CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
                LDFLAGS="${LDFLAGS_ac} ${user_LDFLAGS}"
              ]
            )
            AS_VAR_IF([mhd_cv_mbedtls_md5],["no"],
              [
                AS_VAR_IF([enable_md5],["tlslib"],
                  [AC_MSG_WARN([MbedTLS MD5 implementation is not available])]
                )
              ],
              [
                AC_DEFINE([[MHD_MD5_EXTR_MBEDTLS]],[[1]],
                  [Define to 1 if libmicrohttpd is compiled with MD5 hashing by MbedTLS.])
                have_md5_mbedtls="yes"
                found_md5_tls="yes"
              ]
            )
          ]
        ) # end mbedtls check
      ]
    ) # end AS_IF (enable_https)

    AS_IF([test "x$enable_md5" = "xyes"],
      [AS_VAR_IF([found_md5_tls],["yes"],
        [enable_md5="tlslib"],
        [
          enable_md5="builtin"
          AS_UNSET([have_md5_gnutls])
          AS_UNSET([have_md5_openssl])
          AS_UNSET([have_md5_mbedtls])
        ]
      )],
      [AS_VAR_IF([found_md5_tls],["yes"],
        [enable_md5="tlslib"],
        [AC_MSG_ERROR([TLS library support requested for MD5, but no library supports it])]
      )]
    )
  ]
) # end "enable_md5 in yes|tlslib"

AC_MSG_CHECKING([[whether to support MD5]])
AS_UNSET([enable_md5_MSG])
AS_CASE([${enable_md5}],
  [builtin],[enable_md5_MSG='yes, built-in'],
  [tlslib],[enable_md5_MSG='yes, external (TLS library)'],
  [no],[enable_md5_MSG='no'],
  [yes],[AC_MSG_ERROR([configure internal error: unexpected variable value])],
  [AC_MSG_ERROR([Unrecognized parameter --enable-md5=${enable_md5}])]
)
AS_IF([test "x${enable_md5}" = "xbuiltin" || test "x${enable_md5}" = "xtlslib" ],
  [
    AC_DEFINE([[MHD_SUPPORT_MD5]],[[1]],
      [Define to 1 if libmicrohttpd is compiled with MD5 hashing support.])
  ]
)
AS_IF([test "x${enable_md5}" = "xtlslib" ],
  [
    AC_DEFINE([[MHD_MD5_EXTR]],[[1]],
      [Define to 1 if libmicrohttpd is compiled with MD5 hashing by TLS library.])
  ]
)
AM_CONDITIONAL([MHD_SUPPORT_MD5], [[test "x${enable_md5}" = "xbuiltin" || test "x${enable_md5}" = "xtlslib" ]])
AM_CONDITIONAL([MHD_USE_MD5_INTR], [[test "X${enable_md5}" = "Xbuiltin"]])
AM_CONDITIONAL([MHD_USE_MD5_GNUTLS],[test "X${have_md5_gnutls}" = "Xyes"])
AM_CONDITIONAL([MHD_USE_MD5_OPENSSL],[test "X${have_md5_openssl}" = "Xyes"])
AM_CONDITIONAL([MHD_USE_MD5_MBEDTLS],[test "X${have_md5_mbedtls}" = "Xyes"])
AC_MSG_RESULT([[${enable_md5_MSG}]])

# optional: SHA-256 support for Digest Auth. Enabled by default.
AC_ARG_ENABLE([[sha256]],
  [AS_HELP_STRING([[--enable-sha256=TYPE]],
    [enable TYPE of SHA-256 hashing code (yes, no, builtin, tlslib) [yes if dauth enabled]])],
  [
    AS_VAR_IF([enable_sha256],["internal"],[enable_sha256='builtin'])
    AS_VAR_IF([enable_sha256],["built-in"],[enable_sha256='builtin'])
    AS_VAR_IF([enable_digest_auth],["yes"],[],
      [
        AS_VAR_IF([enable_sha256],["no"],[],
          [
            AC_MSG_WARN([The parameter --enable-sha256=${enable_sha256} is ignored as Digest Authentication is disabled])
            enable_sha256='no'
          ]
        )
      ]
    )
  ], [[enable_sha256="${enable_digest_auth}"]]
)

AS_UNSET([have_sha256_gnutls])
AS_UNSET([have_sha256_openssl])
AS_UNSET([have_sha256_mbedtls])
# SHA-256 external vs. internal check
AS_CASE([${enable_sha256}],[yes|tlslib],
  [
    found_sha256_tls="no"
    AS_IF([test "x$enable_https" = "xyes"],
      [
        # Check GnuTLS
        AS_VAR_IF([have_gnutls],["yes"],
          [
            AC_CACHE_CHECK([whether GnuTLS supports sha256 hashing],[mhd_cv_gnutls_sha256],
              [
                CPPFLAGS="${CPPFLAGS_ac} ${GNUTLS_LIB_CPPFLAGS} ${user_CPPFLAGS}"
                CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
                LDFLAGS="${LDFLAGS_ac} ${GNUTLS_LDFLAGS} ${user_LDFLAGS}"
                save_LIBS="$LIBS"
                LIBS="${GNUTLS_LIBS} ${LIBS}"
                AC_LINK_IFELSE(
                  [
                    AC_LANG_PROGRAM(
                      [[
#include <gnutls/crypto.h>
                      ]],
                      [[
    gnutls_hash_hd_t hash_handle;
    unsigned char digest[32];
    int exit_code;

    if (0 == gnutls_hash_init(&hash_handle, GNUTLS_DIG_SHA256))
    {
      if (0 == gnutls_hash(hash_handle, "", 1))
      {
        gnutls_hash_output(hash_handle, digest);
        if (0x6e == digest[0])
          exit_code = 0;
        else
          exit_code = 7;
      }
      else
        exit_code = 5;
      gnutls_hash_deinit(hash_handle, (void *)0);
    }
    else
      exit_code = 2;
    return exit_code;
                      ]]
                    )
                  ],
                  [mhd_cv_gnutls_sha256='yes'],[mhd_cv_gnutls_sha256='no']
                )
                LIBS="${save_LIBS}"
                CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
                CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
                CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
                LDFLAGS="${LDFLAGS_ac} ${user_LDFLAGS}"
              ]
            )
            AS_VAR_IF([mhd_cv_gnutls_sha256],["no"],
              [
                AS_VAR_IF([enable_sha256],["tlslib"],
                  [AC_MSG_WARN([GnuTLS SHA-256 implementation is not available])]
                )
              ],
              [
                AC_DEFINE([[MHD_SHA256_EXTR_GNUTLS]],[[1]],
                  [Define to 1 if libmicrohttpd is compiled with SHA-256 hashing by GnuTLS.])
                have_sha256_gnutls="yes"
                found_sha256_tls="yes"
              ]
            )
          ]
        ) # end check GnuTLS

        # Check OpenSSL
        AS_VAR_IF([have_openssl],["yes"],
          [
            AC_CACHE_CHECK([whether OpenSSL supports SHA-256 hashing],[mhd_cv_openssl_sha256],
              [
                CPPFLAGS="${CPPFLAGS_ac} ${OPENSSL_CPPFLAGS} ${user_CPPFLAGS}"
                CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
                LDFLAGS="${LDFLAGS_ac} ${OPENSSL_LDFLAGS} ${user_LDFLAGS}"
                save_LIBS="$LIBS"
                LIBS="${OPENSSL_LIBS} ${LIBS}"
                AC_LINK_IFELSE(
                  [
                    AC_LANG_PROGRAM(
                      [[
#include <openssl/evp.h>
                      ]],
                      [[
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    unsigned char digest[32];
    unsigned int len;
    if (ctx) {
      EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
      EVP_DigestUpdate(ctx, "", 1);
      EVP_DigestFinal_ex(ctx, digest, &len);
      EVP_MD_CTX_free(ctx);
    }
                      ]]
                    )
                  ],
                  [mhd_cv_openssl_sha256='yes'],[mhd_cv_openssl_sha256='no']
                )
                LIBS="${save_LIBS}"
                CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
                CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
                LDFLAGS="${LDFLAGS_ac} ${user_LDFLAGS}"
              ]
            )
            AS_VAR_IF([mhd_cv_openssl_sha256],["no"],
              [
                AS_VAR_IF([enable_sha256],["tlslib"],
                  [AC_MSG_WARN([OpenSSL SHA-256 implementation is not available])]
                )
              ],
              [
                AC_DEFINE([[MHD_SHA256_EXTR_OPENSSL]],[[1]],
                  [Define to 1 if libmicrohttpd is compiled with SHA-256 hashing by OpenSSL.])
                have_sha256_openssl="yes"
                found_sha256_tls="yes"
              ]
            )
          ]
        ) # end check OpenSSL

        # Check MbedTLS
        AS_VAR_IF([have_mbedtls],["yes"],
          [
            AC_CACHE_CHECK([whether MbedTLS supports SHA256 hashing],[mhd_cv_mbedtls_sha256],
              [
                CPPFLAGS="${CPPFLAGS_ac} ${MBEDTLS_CRYPTO_CPPFLAGS} ${user_CPPFLAGS}"
                CFLAGS="${CFLAGS_ac} ${MBEDTLS_CRYPTO_CFLAGS} ${user_CFLAGS}"
                LDFLAGS="${LDFLAGS_ac} ${MBEDTLS_CRYPTO_LDFLAGS} ${user_LDFLAGS}"
                save_LIBS="$LIBS"
                LIBS="${MBEDTLS_CRYPTO_LIBS} ${LIBS}"
                AC_LINK_IFELSE(
                  [
                    AC_LANG_PROGRAM(
                      [[
#include <mbedtls/sha256.h>
                      ]],
                      [[
    mbedtls_sha256_context ctx;
    unsigned char digest[16];
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, (const unsigned char *)"", 1);
    mbedtls_sha256_finish(&ctx, digest);
    mbedtls_sha256_free(&ctx);
                      ]]
                    )
                  ],
                  [mhd_cv_mbedtls_sha256='yes'],[mhd_cv_mbedtls_sha256='no']
                )
                LIBS="${save_LIBS}"
                CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
                CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
                LDFLAGS="${LDFLAGS_ac} ${user_LDFLAGS}"
              ]
            )
            AS_VAR_IF([mhd_cv_mbedtls_sha256],["no"],
              [
                AS_VAR_IF([enable_sha256],["tlslib"],
                  [AC_MSG_WARN([MbedTLS SHA256 implementation is not available])]
                )
              ],
              [
                AC_DEFINE([[MHD_SHA256_EXTR_MBEDTLS]],[[1]],
                  [Define to 1 if libmicrohttpd is compiled with SHA256 hashing by MbedTLS.])
                have_sha256_mbedtls="yes"
                found_sha256_tls="yes"
              ]
            )
          ]
        ) # end mbedtls check
      ]
    ) # end AS_IF (enable_https)

    AS_IF([test "x$enable_sha256" = "xyes"],
      [AS_VAR_IF([found_sha256_tls],["yes"],
        [enable_sha256="tlslib"],
        [
          enable_sha256="builtin"
          AS_UNSET([have_sha256_gnutls])
          AS_UNSET([have_sha256_openssl])
          AS_UNSET([have_sha256_mbedtls])
        ]
      )],
      [AS_VAR_IF([found_sha256_tls],["yes"],
        [enable_sha256="tlslib"],
        [AC_MSG_ERROR([TLS library support requested for SHA256, but no library supports it])]
      )]
    )
  ]
)
AC_MSG_CHECKING([[whether to support SHA-256]])
AS_UNSET([enable_sha256_MSG])
AS_CASE([${enable_sha256}],
  [builtin],[enable_sha256_MSG='yes, built-in'],
  [tlslib],[enable_sha256_MSG='yes, external (TLS library)'],
  [no],[enable_sha256_MSG='no'],
  [yes],[AC_MSG_ERROR([configure internal error: unexpected variable value])],
  [AC_MSG_ERROR([Unrecognized parameter --enable-sha256=${enable_sha256}])]
)
AS_IF([test "x${enable_sha256}" = "xbuiltin" ||
       test "x${enable_sha256}" = "xtlslib" ],
  [
    AC_DEFINE([[MHD_SUPPORT_SHA256]],[[1]],
      [Define to 1 if libmicrohttpd is compiled with SHA-256 hashing support.])
  ]
)
AS_IF([test "x${enable_sha256}" = "xtlslib" ],
  [
    AC_DEFINE([[MHD_SHA256_EXTR]],[[1]],
      [Define to 1 if libmicrohttpd is compiled with SHA-256 hashing by TLS library.])
  ]
)
AM_CONDITIONAL([MHD_SUPPORT_SHA256], [[test "x${enable_sha256}" = "xbuiltin" || test "x${enable_sha256}" = "xtlslib" ]])
AM_CONDITIONAL([MHD_USE_SHA256_INTR], [[test "X${enable_sha256}" = "Xbuiltin"]])
AM_CONDITIONAL([MHD_USE_SHA256_GNUTLS], [[test "X${have_sha256_gnutls}" = "Xyes"]])
AM_CONDITIONAL([MHD_USE_SHA256_OPENSSL], [[test "X${have_sha256_openssl}" = "Xyes"]])
AM_CONDITIONAL([MHD_USE_SHA256_MBEDTLS], [[test "X${have_sha256_mbedtls}" = "Xyes"]])
AC_MSG_RESULT([[${enable_sha256_MSG}]])

# optional: SHA-512/256 support for Digest Auth. Enabled by default.
AC_ARG_ENABLE([[sha512-256]],
  [AS_HELP_STRING([[--enable-sha512-256=TYPE]],
    [enable TYPE of SHA-512/256 hashing code (yes, no, builtin, tlslib) [yes if dauth enabled]])],
  [
    AS_VAR_IF([enable_sha512_256],["internal"],[enable_sha512_256='builtin'])
    AS_VAR_IF([enable_sha512_256],["built-in"],[enable_sha512_256='builtin'])
    AS_VAR_IF([enable_digest_auth],["yes"],[],
      [
        AS_VAR_IF([enable_sha512_256],["no"],[],
          [
            AC_MSG_WARN([The parameter --enable-sha512x256=${enable_sha512x256} is ignored as Digest Authentication is disabled])
            enable_sha512_256='no'
          ]
        )
      ]
    )
  ], [[enable_sha512_256="${enable_digest_auth}"]]
)

AS_UNSET([have_sha512_256_openssl])
AS_UNSET([have_sha512_256_mbedtls])
# SHA-512/256 external vs internal check
AS_CASE([${enable_sha512_256}],[yes|tlslib],
  [
    found_sha512_256_tls="no"
    AS_IF([test "x$enable_https" = "xyes"],
      [
        # Check OpenSSL
        AS_VAR_IF([have_openssl],["yes"],
          [
            AC_CACHE_CHECK([whether OpenSSL supports SHA-512/256 hashing],[mhd_cv_openssl_sha512_256],
              [
                CPPFLAGS="${CPPFLAGS_ac} ${OPENSSL_CPPFLAGS} ${user_CPPFLAGS}"
                CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
                LDFLAGS="${LDFLAGS_ac} ${OPENSSL_LDFLAGS} ${user_LDFLAGS}"
                save_LIBS="$LIBS"
                LIBS="${OPENSSL_LIBS} ${LIBS}"
                AC_LINK_IFELSE(
                  [
                    AC_LANG_PROGRAM(
                      [[
#include <openssl/evp.h>
                      ]],
                      [[
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    unsigned char digest[32];
    unsigned int len;
    if (ctx) {
      EVP_DigestInit_ex(ctx, EVP_sha512_256(), NULL);
      EVP_DigestUpdate(ctx, "", 1);
      EVP_DigestFinal_ex(ctx, digest, &len);
      EVP_MD_CTX_free(ctx);
    }
                      ]]
                    )
                  ],
                  [mhd_cv_openssl_sha512_256='yes'],[mhd_cv_openssl_sha512_256='no']
                )
                LIBS="${save_LIBS}"
                CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
                CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
                LDFLAGS="${LDFLAGS_ac} ${user_LDFLAGS}"
              ]
            )
            AS_VAR_IF([mhd_cv_openssl_sha512_256],["yes"],
              [
                AC_DEFINE([[MHD_SHA512_256_EXTR_OPENSSL]],[[1]],
                  [Define to 1 if libmicrohttpd is compiled with SHA-512/256 hashing by OpenSSL.])
                have_sha512_256_openssl="yes"
                found_sha512_256_tls="yes"
              ]
            )
          ]
        )
        # Check mbedTLS - test for SHA-512 support (C code uses SHA-512 implementation)
        AS_VAR_IF([have_mbedtls],["yes"],
          [
            AC_CACHE_CHECK([whether mbedTLS supports SHA-512 hashing for SHA-512/256],[mhd_cv_mbedtls_sha512],
              [
                CPPFLAGS="${CPPFLAGS_ac} ${MBEDTLS_CPPFLAGS} ${user_CPPFLAGS}"
                CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
                LDFLAGS="${LDFLAGS_ac} ${MBEDTLS_LDFLAGS} ${user_LDFLAGS}"
                save_LIBS="$LIBS"
                LIBS="${MBEDTLS_LIBS} ${LIBS} -lmbedcrypto"
                AC_LINK_IFELSE(
                  [
                    AC_LANG_PROGRAM(
                      [[
#include <mbedtls/sha512.h>
                      ]],
                      [[
    mbedtls_sha512_context ctx;
    unsigned char digest[64];
    mbedtls_sha512_init(&ctx);
    mbedtls_sha512_starts(&ctx, 0);
    mbedtls_sha512_update(&ctx, (const unsigned char *)"", 1);
    mbedtls_sha512_finish(&ctx, digest);
    mbedtls_sha512_free(&ctx);
                      ]]
                    )
                  ],
                  [mhd_cv_mbedtls_sha512='yes'],[mhd_cv_mbedtls_sha512='no']
                )
                LIBS="${save_LIBS}"
                CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
                CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
                LDFLAGS="${LDFLAGS_ac} ${user_LDFLAGS}"
              ]
            )
            AS_VAR_IF([mhd_cv_mbedtls_sha512],["yes"],
              [
                AC_DEFINE([[MHD_SHA512_EXTR_MBEDTLS]],[[1]],
                  [Define to 1 if libmicrohttpd is compiled with SHA-512 hashing by mbedTLS.])
                have_sha512_256_mbedtls="yes"
                found_sha512_256_tls="yes"
              ]
            )
          ]
        )
      ]
    ) # end AS_IF (enable_https)

    AS_IF([test "x$enable_sha512_256" = "xyes"],
      [AS_VAR_IF([found_sha512_256_tls],["yes"],
        [enable_sha512_256="tlslib"],
        [
          enable_sha512_256="builtin"
          AS_UNSET([have_sha512_256_openssl])
          AS_UNSET([have_sha512_256_mbedtls])
        ]
      )],
      [AS_VAR_IF([found_sha512_256_tls],["yes"],
        [enable_sha256="tlslib"],
        [AC_MSG_ERROR([TLS library support requested for SHA512/256, but no library supports it])]
      )]
    )
  ]
)


AC_MSG_CHECKING([[whether to support SHA-512/256]])
AS_UNSET([enable_sha512_256_MSG])
AS_CASE([${enable_sha512_256}],
  [builtin],[enable_sha512_256_MSG='yes, built-in'],
  [tlslib],[enable_sha512_256_MSG='yes, external (TLS library)'],
  [yes],[AC_MSG_ERROR([configure internal error: unexpected variable value])],
  [no],[enable_sha512_256_MSG='no'],
  [AC_MSG_ERROR([Unrecognized parameter --enable-sha512-256=${enable_sha512_256}])]
)

AS_IF([test "x${enable_sha512_256}" = "xbuiltin" ||
       test "x${enable_sha512_256}" = "xtlslib" ],
  [
   AC_DEFINE([[MHD_SUPPORT_SHA512_256]],[[1]],
     [Define to 1 if libmicrohttpd is compiled with SHA-512/256 hashing support.])
  ]
)

AS_IF([test "x${enable_sha512_256}" = "xtlslib" ],
  [
    AC_DEFINE([[MHD_SHA512_256_EXTR]],[[1]],
      [Define to 1 if libmicrohttpd is compiled with SHA-512/256 hashing by TLS library.])
  ]
)
AM_CONDITIONAL([MHD_SUPPORT_SHA512_256], [[test "x${enable_sha512_256}" = "xbuiltin" || test "x${enable_sha512_256}" = "xtlslib"]])
AM_CONDITIONAL([MHD_USE_SHA512_256_INTR], [[test "X${enable_sha512_256}" = "Xbuiltin"]])
AM_CONDITIONAL([MHD_USE_SHA512_256_OPENSSL], [[test "X${have_sha512_256_openssl}" = "Xyes"]])
AM_CONDITIONAL([MHD_USE_SHA512_256_MBEDTLS], [[test "X${have_sha512_256_mbedtls}" = "Xyes"]])


AC_MSG_RESULT([[${enable_sha512_256_MSG}]])


# Check if digest auth can be enabled, requires at least one hash algo
AS_IF([test "x$enable_digest_auth" != "xno"],
  [
    AS_IF([test "x${enable_md5}" = "xno" &&  test "x${enable_sha256}" = "xno" && test "x${enable_sha512_256}" != "xyes"],
      [AC_MSG_ERROR([At least one hashing algorithm must be enabled if Digest Auth is enabled])]
    )
  ]
)


AC_CACHE_CHECK([[for calloc()]], [[mhd_cv_have_func_calloc]],
  [
   AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#elif defined(HAVE_MALLOC_H)
#include <malloc.h>
#else
#include <unistd.h>
#endif
       ]],[[void * ptr = calloc(1, 2); if (ptr) return 1;]])
     ],
     [[mhd_cv_have_func_calloc='yes']],
     [[mhd_cv_have_func_calloc='no']]
   )
  ]
)
AS_VAR_IF([[mhd_cv_have_func_calloc]], [["yes"]],
  [AC_DEFINE([[HAVE_CALLOC]], [[1]], [Define to 1 if you have the usable `calloc' function.])])
AM_CONDITIONAL([HAVE_SYS_CALLOC], [[test "x${mhd_cv_have_func_calloc}" = "xyes"]])

# Some systems have IPv6 disabled in kernel at run-time
AS_IF([[test "x${mhd_cv_have_inet6}" = "xyes" && test "x${cross_compiling}" = "xno"]],
 [
   AC_CACHE_CHECK([whether IPv6 could be used for testing],[mhd_cv_ipv6_for_testing],
     [
       AC_RUN_IFELSE(
         [
           AC_LANG_SOURCE([[
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#ifdef HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

static void zr_mem(void *ptr, socklen_t size)
{ char *mem = ptr; while(size--) {mem[0] = 0; mem++;} }

int main(void)
{
  int ret = 30;
  struct sockaddr_in6 sa;
#if !defined(_WIN32) || defined(__CYGWIN__)
  int sckt;
  const int invld_sckt = -1;
#else
  SOCKET sckt;
  const SOCKET invld_sckt = INVALID_SOCKET;
  WSADATA wsa_data;

  WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif
  zr_mem(&sa, sizeof(sa));
  sa.sin6_family = AF_INET6;
  sa.sin6_port = 0;
  sa.sin6_addr = in6addr_loopback;
#ifdef HAVE_STRUCT_SOCKADDR_IN6_SIN6_LEN
  sa.sin6_len = sizeof(sa);
#endif
  sckt = socket (PF_INET6, SOCK_STREAM, 0);
  if (invld_sckt != sckt)
  {
    if (0 == bind (sckt, (struct sockaddr *)&sa, sizeof(sa)))
    {
      if (0 == listen (sckt, 1))
        ret = 0;
      else
        ret = 1; /* listen() failed */
    } else ret = 2; /* bind() failed */
#if !defined(_WIN32) || defined(__CYGWIN__)
    close (sckt);
#else
    closesocket (sckt);
#endif
  } else ret = 3; /* socket() failed */
#if defined(_WIN32) && !defined(__CYGWIN__)
  WSACleanup();
#endif
  return ret;
}
           ]])
         ], [[mhd_cv_ipv6_for_testing="yes"]], [[mhd_cv_ipv6_for_testing="no"]], [[mhd_cv_ipv6_for_testing="no"]]
       )
     ]
   )
 ]
)
AS_VAR_IF([mhd_cv_ipv6_for_testing],["yes"],
	[AC_DEFINE([[USE_IPV6_TESTING]], [[1]], [Define to 1 if your kernel supports IPv6 and IPv6 is enabled and useful for testing.])]
)
AM_CONDITIONAL([USE_IPV6_TESTING], [[test "x$mhd_cv_ipv6_for_testing" = "xyes"]])


AS_VAR_IF([enable_tools],["yes"],
  [
    AC_CHECK_HEADERS([features.h sys/pstat.h vxCpuLib.h],[],[],[AC_INCLUDES_DEFAULT])
    MHD_CHECK_DECLS([_SC_NPROCESSORS_ONLN _SC_NPROC_ONLN _SC_CRAY_NCPU _SC_NPROCESSORS_CONF \
                     CTL_HW HW_NCPUONLINE HW_NCPU HW_AVAILCPU],
      [[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif /* HAVE_SYS_SYSCTL_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
      ]]
    )
    MHD_CHECK_FUNC([pstat_getdynamic],[[
#include <sys/param.h>
#include <sys/pstat.h>
      ]],
      [[
        struct pst_dynamic psd_data;
        i][f (1 != pstat_getdynamic(&psd_data, sizeof(psd_data), (size_t)1, 0))
          return 2;
        i][f (0 >= psd_data.psd_proc_cnt)
          return 3;
      ]]
    )
    MHD_CHECK_FUNC([vxCpuEnabledGet],[[#include <vxCpuLib.h>]],
      [[
        cpuset_t enb_set;
        enb_set = vxCpuEnabledGet();
        (void) enb_set;
      ]]
    )
    AC_CHECK_HEADERS([sched.h sys/_cpuset.h sys/cpuset.h],[],[],[AC_INCLUDES_DEFAULT])
    # glibc / Linux kernel
    MHD_CHECK_FUNC([getpid],[[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#include <unistd.h>
      ]],
      [[
        pid_t cur_pid;
        cur_pid = getpid();
        (void) &cur_pid; /* Mute possible warning */
      ]],
      [
        MHD_CHECK_FUNC([sched_getaffinity],[[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#include <unistd.h>
#include <sched.h>
          ]],
          [[
            cpu_set_t cur_set;
            i][f (0 != sched_getaffinity(getpid(), sizeof(cur_set), &cur_set))
              return 2;
            i][f (0 == CPU_SET (0, &cur_set))
              return 3; /* Actually this could be a valid result */
          ]],
          [
            MHD_CHECK_FUNC([CPU_COUNT],[[
#include <stddef.h>
#include <sched.h>
              ]],
              [[
                cpu_set_t cur_set;
                CPU_ZERO(&cur_set);
                i][f (0 != CPU_COUNT(&cur_set))
                  return 2;
              ]],
              [
                MHD_CHECK_FUNC([CPU_COUNT_S],[[
#include <stddef.h>
#include <sched.h>
                  ]],
                  [[
                    static const unsigned int set_size_cpus = 2048u;
                    const size_t set_size_bytes = (size_t) CPU_ALLOC_SIZE(set_size_cpus);
                    cpu_set_t *p_set;
                    p_set = CPU_ALLOC(set_size_cpus);
                    i][f (!p_set)
                     return 2;
                    CPU_ZERO_S(set_size_bytes, p_set);
                    i][f (0 != CPU_COUNT_S(set_size_bytes, p_set))
                    {
                      CPU_FREE(p_set);
                      return 3;
                    }
                    CPU_FREE(p_set);
                  ]],
                  [MHD_CHECK_DECLS([CPU_SETSIZE],[],[],[#include <sched.h>])]
                )
              ]
            )
          ]
        )
        # NetBSD
        # Should work only with -lrt, but actually works without it.
        MHD_CHECK_FUNC([sched_getaffinity_np],[[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#include <unistd.h>
#include <sched.h>
          ]],
          [[
            cpuset_t *cpuset_ptr;
            cpuid_t cpu_num = 0;
            cpuset_ptr = cpuset_create();
            i][f (!cpuset_ptr)
              return 2;
            i][f (0 != sched_getaffinity_np(getpid(), cpuset_size(cpuset_ptr), cpuset_ptr))
            {
              cpuset_destroy(cpuset_ptr);
              return 3;
            }
            i][f (0 >= cpuset_isset(cpu_num, cpuset_ptr))
            {
              cpuset_destroy(cpuset_ptr);
              return 4; /* Actually this could be a valid result */
            }
            cpuset_destroy(cpuset_ptr);
          ]]
        )
      ]
    )
    # FreeBSD
    MHD_CHECK_FUNC([cpuset_getaffinity],[[
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */
#include <sys/cpuset.h>
      ]],
      [[
        cpuset_t cur_mask;
        i][f (0 != cpuset_getaffinity(CPU_LEVEL_WHICH, CPU_WHICH_PID, (id_t) -1, sizeof(cur_mask), &cur_mask))
          return 2;
      ]],
      [
        MHD_CHECK_FUNC([CPU_COUNT],[[
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */
#ifdef HAVE_SYS__CPUSET_H
#include <sys/_cpuset.h>
#endif /* HAVE_SYS_PARAM_H */
#include <sys/cpuset.h>
          ]],
          [[
            cpuset_t test_mask;
            CPU_ZERO(&test_mask);
            i][f (0 !=  CPU_COUNT(&test_mask))
              return 2;
          ]],
          [
            MHD_CHECK_FUNC([CPU_COUNT_S],[[
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */
#ifdef HAVE_SYS__CPUSET_H
#include <sys/_cpuset.h>
#endif /* HAVE_SYS_PARAM_H */
#include <sys/cpuset.h>
              ]],
              [[
                static const unsigned int max_cpu_num = 2048u;
                cpuset_t *mask_ptr;
                mask_ptr = CPU_ALLOC(max_cpu_num);
                i][f (! mask_ptr)
                  return 2;
                i][f (0 != cpuset_getaffinity(CPU_LEVEL_WHICH, CPU_WHICH_PID, (id_t) -1,
                                              CPU_ALLOC_SIZE(max_cpu_num), mask_ptr))
                {
                  CPU_FREE(mask_ptr);
                  return 3;
                }
                i][f (0 == CPU_COUNT_S(CPU_ALLOC_SIZE(max_cpu_num), &test_mask))
                {
                  CPU_FREE(mask_ptr);
                  return 4;
                }
                CPU_FREE(mask_ptr);
              ]]
            )
          ]
        )
      ]
    )
    AS_VAR_IF([mhd_cv_have_func_CPU_COUNT_S],["yes"],
      [
        AC_CACHE_CHECK([whether CPU_COUNT_S() expects max CPU number as 'size' parameter],[mhd_cv_func_cpu_count_s_cpus],
          [
            AS_VAR_IF([cross-compiling],["yes"],[mhd_cv_func_cpu_count_s_cpus="assuming no"],
              [
                AC_LINK_IFELSE(
                  [
                    AC_LANG_PROGRAM([[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */
#ifdef HAVE_SYS__CPUSET_H
#include <sys/_cpuset.h>
#endif /* HAVE_SYS_PARAM_H */
#ifdef HAVE_SYS_CPUSET_H
#include <sys/cpuset.h>
#endif /* HAVE_SYS_CPUSET_H */
#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif /* HAVE_STDDEF_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_SCHED_H
#include <sched.h>
#endif /* HAVE_SCHED_H */
#include <stdio.h>

#ifdef HAVE_SCHED_GETAFFINITY
#define TEST_CPUSET_TYPE cpu_set_t
#else  /* ! HAVE_SCHED_GETAFFINITY */
#define TEST_CPUSET_TYPE cpuset_t
#endif /* ! HAVE_SCHED_GETAFFINITY */
                      ]],
                      [[
  TEST_CPUSET_TYPE *p_testset;
  /* The size of the cpuset that is larger then the test cpuset (in CPUs) */
  static const unsigned int testset_size_oversized_cpus = (8 * sizeof(long)) * 1024;
  const size_t testset_size_oversized_bytes = CPU_ALLOC_SIZE(testset_size_oversized_cpus);
  /* The size of the test cpuset in number of CPUs */
  const unsigned int testset_size_cpus = (unsigned int) testset_size_oversized_bytes;
  /* The size of the test cpuset in bytes */
  const size_t testset_size_bytes = CPU_ALLOC_SIZE(testset_size_cpus);
  /* The last one CPU in the test set */
  const unsigned int test_cpu_num1 = testset_size_cpus - 1;
  /* The penultimate CPU in the test set */
  const unsigned int test_cpu_num2 = testset_size_cpus - 2;
  /* The CPU numbers that should be cleared */
  const unsigned int test_cpu_clear1 = testset_size_cpus - 3;
  const unsigned int test_cpu_clear2 = testset_size_cpus - 4;
  unsigned int count_res;
  int ret = 0;

  /* Allocate oversize area to ensure that memory outside the buffer is not used */
  p_testset = CPU_ALLOC(testset_size_oversized_cpus);
  if (! p_testset)
  {
    fprintf (stderr, "Error allocating memory.\n");
    return 99;
  }
  /* Set the some CPU numbers and then clear them */
  CPU_SET_S(test_cpu_clear1, testset_size_bytes, p_testset);
  CPU_SET_S(test_cpu_clear2, testset_size_bytes, p_testset);
  CPU_ZERO_S(testset_size_bytes, p_testset);
  /* Set two CPUs on */
  CPU_SET_S(test_cpu_num1, testset_size_bytes, p_testset);
  CPU_SET_S(test_cpu_num2, testset_size_bytes, p_testset);
  count_res = (unsigned int) CPU_COUNT_S(testset_size_bytes, p_testset);
  if (0 == count_res)
  {
    fprintf (stderr, "Full cpuset cannot be read by CPU_COUNT_S() when using the number of bytes as the size parameter.\n");
    /* Set the some CPU numbers and then clear them */
    CPU_SET_S(test_cpu_clear1, testset_size_cpus, p_testset);
    CPU_SET_S(test_cpu_clear2, testset_size_cpus, p_testset);
    CPU_ZERO_S(testset_size_cpus, p_testset);
    /* Set two CPUs on */
    CPU_SET_S(test_cpu_num1, testset_size_cpus, p_testset);
    CPU_SET_S(test_cpu_num2, testset_size_cpus, p_testset);
    count_res = (unsigned int) CPU_COUNT_S(testset_size_cpus, p_testset);
    if (2 == count_res)
    {
      fprintf (stderr, "Full cpuset is read by CPU_COUNT_S() only when using the maximum CPU number as the size parameter.\n");
      ret = 3;
    }
    else
    {
      fprintf (stderr, "Wrong result returned by CPU_COUNT_S() when using the maximum CPU number as the size parameter.\n");
      fprintf (stderr, "Number of 'enabled' CPUs: 2\n");
      fprintf (stderr, "Number of counted CPUs:   %u\n", count_res);
      fprintf (stderr, "CPU_COUNT_S() could be unreliable.\n");
      ret = 70;
    }
  }
  else if (2 == count_res)
  {
    fprintf (stderr, "Full cpuset is read by CPU_COUNT_S() when using the number of bytes as the size parameter.\n");
  }
  else
  {
    fprintf (stderr, "Wrong result returned by CPU_COUNT_S() when using the number of bytes as the size parameter.\n");
    fprintf (stderr, "Number of 'enabled' CPUs: 2\n");
    fprintf (stderr, "Number of counted CPUs:   %u\n", count_res);
    fprintf (stderr, "CPU_COUNT_S() could be unreliable.\n");
    ret = 71;
  }
  CPU_FREE(p_testset);
  if (0 != ret)
    return ret;
                      ]]
                    )
                  ],
                  [
                    AM_RUN_LOG([./conftest$EXEEXT])
                    count_test_res=$?
                    AS_IF([test $count_test_res -eq 0],[mhd_cv_func_cpu_count_s_cpus="no"],
                      [test $count_test_res -eq 3],[mhd_cv_func_cpu_count_s_cpus="yes"],
                      [
                        AC_MSG_WARN([Unexpected value returned by CPU_COUNT_S() test program. Please report to ${PACKAGE_BUGREPORT}])
                        mhd_cv_func_cpu_count_s_cpus="assuming no"
                      ]
                    )
                  ],
                  [
                    AC_MSG_WARN([Cannot build CPU_COUNT_S() test program. Please report to ${PACKAGE_BUGREPORT}])
                    mhd_cv_func_cpu_count_s_cpus="assuming no"
                  ]
                )
              ]
            )
          ]
        )
        AS_VAR_IF([mhd_cv_func_cpu_count_s_cpus],["yes"],
          [AC_DEFINE([MHD_FUNC_CPU_COUNT_S_GETS_CPUS],[1],
             [Define to '1' if CPU_COUNT_S() function expects max CPU number as 'size' parameter])
          ]
        )
      ]
    )
  ]
)

# Check for fork() and waitpid(). They are used for tests.
AC_MSG_CHECKING([[for fork()]])
mhd_have_fork_waitpid='no'
AC_LINK_IFELSE(
  [
   AC_LANG_PROGRAM(
     [[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
     ]], [[
  pid_t p = fork ();
  if (0 == p)
    return 1;
     ]])
  ], [
   AC_DEFINE([[HAVE_FORK]], [[1]], [Define to 1 if you have the usable `fork' function.])
   AC_MSG_RESULT([[yes]])

   AC_MSG_CHECKING([[for waitpid()]])
   AC_LINK_IFELSE(
     [
      AC_LANG_PROGRAM(
        [[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/wait.h>
        ]], [[
     pid_t p = fork ();
     if (0 == p)
       return 1;
     waitpid (p, (void*)0, 0)
        ]])
     ], [
      AC_DEFINE([[HAVE_WAITPID]], [[1]], [Define to 1 if you have the usable `waitpid' function.])
      AC_MSG_RESULT([[yes]])
      mhd_have_fork_waitpid='yes'
    ],[
       AC_MSG_RESULT([[no]])
    ])
],[
   AC_MSG_RESULT([[no]])
])

AM_CONDITIONAL([HAVE_FORK_WAITPID], [test "x$mhd_have_fork_waitpid" = "xyes"])

# gcov compilation
AC_MSG_CHECKING(whether to compile with support for code coverage analysis)
AC_ARG_ENABLE([coverage],
              AS_HELP_STRING([--enable-coverage],
                             [compile the library with code coverage support]),
              [use_gcov=${enableval}],
              [use_gcov=no])
AC_MSG_RESULT($use_gcov)
AM_CONDITIONAL([USE_COVERAGE], [test "x$use_gcov" = "xyes"])

AX_COUNT_CPUS
AC_SUBST([MHD_REAL_CPU_COUNT],[${CPU_COUNT}])
AM_SUBST_NOTMAKE([MHD_REAL_CPU_COUNT])
AC_MSG_CHECKING([for number of CPU cores to use in tests])
AS_VAR_IF([use_heavy_tests], ["yes"],
  [
    # Enable usage of many core if heavy tests are enabled
    AS_IF([[test "$CPU_COUNT" -gt "32"]], [[CPU_COUNT="32"]])dnl Limit resource usage
  ],
  [
    # Limit usage to just a few cores if heavy tests are not enabled
    AS_IF([[test "$CPU_COUNT" -gt "6"]], [[CPU_COUNT="6"]])
    AS_IF([[test "$CPU_COUNT" -lt "2"]], [[CPU_COUNT="2"]])
  ]
)
AC_MSG_RESULT([$CPU_COUNT])


AC_ARG_ENABLE([[asserts]],
  [AS_HELP_STRING([[--enable-asserts]],
    [enable test build with debug asserts])],
  [],
  [enable_asserts='auto']
)
AS_UNSET([use_asserts_MSG])
AC_CACHE_CHECK([[whether NDEBUG macro is defined]], [mhd_cv_macro_ndebug_def],
  [
    AC_LINK_IFELSE(
      [
        AC_LANG_SOURCE([[
#ifndef NDEBUG
#error NDEBUG is NOT defined
chome me now
#endif

int main(void)
{
  return 0;
}
          ]]
        )
      ],
      [[mhd_cv_macro_ndebug_def='yes']],
      [[mhd_cv_macro_ndebug_def='no']]
    )
  ]
)
AS_VAR_IF([enable_asserts],["yes"],
  [
    AS_VAR_IF([mhd_cv_macro_ndebug_def],["yes"],
      [
        AC_MSG_FAILURE([Parameter --enable-asserts is specified, but NDEBUG macro is defined as well])
      ]
    )
    use_asserts_MSG="yes, enabled by configure parameter"
  ]
)
AC_CACHE_CHECK([[whether _DEBUG macro is defined]], [mhd_cv_macro__debug_def],
  [
    AC_LINK_IFELSE(
      [
        AC_LANG_SOURCE([[
#ifndef _DEBUG
#error _DEBUG is NOT defined
chome me now
#endif

int main(void)
{
  return 0;
}
          ]]
        )
      ],
      [[mhd_cv_macro__debug_def='yes']],
      [[mhd_cv_macro__debug_def='no']]
    )
  ]
)
AS_VAR_IF([enable_asserts],["no"],
  [
    AS_VAR_IF([mhd_cv_macro__debug_def],["yes"],
      [
        AC_MSG_FAILURE([Parameter --disable-asserts is specified, but _DEBUG macro is defined as well])
      ]
    )
    use_asserts_MSG="no, set by configure parameter"
  ]
)
AS_IF([test "x${mhd_cv_macro_ndebug_def}" = "xyes" && test "x${mhd_cv_macro__debug_def}" = "xyes"],
  [AC_MSG_FAILURE([Both NDEBUG and _DEBUG macros are defined])]
)
AS_VAR_IF([enable_asserts],["auto"],
  [
    AS_VAR_IF([mhd_cv_macro_ndebug_def],["yes"],
      [
        enable_asserts="no"
        use_asserts_MSG="no, set by NDEBUG preprocessor macro"
      ]
    )
  ]
)
AS_VAR_IF([enable_asserts],["auto"],
  [
    AS_VAR_IF([mhd_cv_macro__debug_def],["yes"],
      [
        enable_asserts="yes"
        use_asserts_MSG="yes, enabled by _DEBUG preprocessor macro"
      ]
    )
  ]
)
AS_VAR_IF([enable_asserts],["auto"],
  [
    AC_CACHE_CHECK([[whether DEBUG macro is defined]], [mhd_cv_macro_debug_def],
      [
        AC_LINK_IFELSE(
          [
            AC_LANG_SOURCE([[
#ifndef DEBUG
#error DEBUG is NOT defined
chome me now
#endif

int main(void)
{
  return 0;
}
              ]]
            )
          ],
          [[mhd_cv_macro_debug_def='yes']],
          [[mhd_cv_macro_debug_def='no']]
        )
      ]
    )
    AS_VAR_IF([mhd_cv_macro_debug_def],["yes"],
      [
        enable_asserts="yes"
        use_asserts_MSG="yes, enabled by DEBUG preprocessor macro"
      ]
    )
  ]
)
AC_MSG_CHECKING([[whether to enable debug asserts]])
AS_VAR_IF([enable_asserts],["auto"],
  [
    AS_CASE([${enable_build_type}],
      [debug|debugger|trace],
      [
        enable_asserts="yes"
        use_asserts_MSG="yes, enabled by --enable-bulid-type=${enable_build_type}"
      ],
      [
        enable_asserts="no"
        use_asserts_MSG="no"
      ]
    )
  ]
)
AS_CASE([[$enable_asserts]], [[yes]], [[:]], [[no]], [[:]], [[enable_asserts='no']])
AC_MSG_RESULT([[${use_asserts_MSG=no}]])

AS_VAR_IF([[enable_asserts]], [["yes"]],
  [
    AS_VAR_IF([[mhd_cv_macro__debug_def]], [["yes"]], [:],
      [
        MHD_PREPEND_FLAG_TO_VAR([CPPFLAGS_ac],[-D_DEBUG=1])
        CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
      ]
    )
    AC_CACHE_CHECK([[whether system assert() is available]], [mhd_cv_sys_assert_avail],
      [
        AC_LINK_IFELSE(
          [
            AC_LANG_SOURCE([[
#include <assert.h>

static int pos_val(void) {return 5;}
static int neg_val(void) {return -5;}
int main(void)
{
  int pos_var = pos_val(), neg_var = neg_val();
  assert(neg_var > pos_var); /* Must trigger assert. */
  return pos_var + neg_var;
}
            ]])
          ],
          [[mhd_cv_sys_assert_avail='yes']],
          [[mhd_cv_sys_assert_avail='no']]
        )
      ]
    )
    AS_VAR_IF([[mhd_cv_sys_assert_avail]], [["no"]], [],
      [AC_DEFINE([[HAVE_ASSERT]], [[1]], [Define if you have usable assert() and assert.h])])
  ],
  [
    AS_VAR_IF([[mhd_cv_macro_ndebug_def]], [["yes"]], [:],
      [
        MHD_PREPEND_FLAG_TO_VAR([CPPFLAGS_ac],[-DNDEBUG=1])
        CPPFLAGS="${CPPFLAGS_ac} ${user_CPPFLAGS}"
      ]
    )
  ]
)

AC_ARG_ENABLE([[trace-fds-polling]],
  [AS_HELP_STRING([[--enable-trace-fds-polling]],
    [enable test build with debug printing of FDs polling])],
  [
    AS_CASE([${enable_trace_fds_polling}],
      [yes|no],[:],
      [AC_MSG_ERROR([wrong parameter --enable-trace-fds-polling=${enable_trace_fds_polling}])]
    )
  ],
  [enable_trace_fds_polling='auto']
)
AS_UNSET([use_trace_fds_polling_MSG])
AS_UNSET([use_trace_fds_polling])
AC_CACHE_CHECK([[whether MHD_USE_TRACE_POLLING_FDS macro is defined]], [mhd_cv_macro_trace_polling_fds_def],
  [
    AC_LINK_IFELSE(
      [
        AC_LANG_SOURCE([[
#ifndef MHD_USE_TRACE_POLLING_FDS
#error MHD_USE_TRACE_POLLING_FDS is NOT defined
chome me now
#endif

int main(void)
{ return 0; }
          ]]
        )
      ],
      [[mhd_cv_macro_trace_polling_fds_def='yes']],
      [[mhd_cv_macro_trace_polling_fds_def='no']]
    )
  ]
)
AC_MSG_CHECKING([whether to enable debug-print of FDs polling])
AS_VAR_IF([mhd_cv_macro_trace_polling_fds_def],["yes"],
  [
    use_trace_fds_polling="yes"
    use_trace_fds_polling_MSG="yes, forced by MHD_USE_TRACE_POLLING_FDS macro"
  ]
)
AS_VAR_SET_IF([use_trace_fds_polling],[:],
  [
    AS_CASE([${enable_trace_fds_polling}],
      [yes],[
        use_trace_fds_polling="yes"
        use_trace_fds_polling_MSG="yes, enabled by --enable-trace-fds-polling"
      ],
      [no],[
        use_trace_fds_polling="no"
        use_trace_fds_polling_MSG="no, disabled by --disable-trace-fds-polling"
      ],
      [auto],[
        AS_CASE([${enable_build_type}],
          [trace],[
            use_trace_fds_polling="yes"
            use_trace_fds_polling_MSG="yes, enabled by --enable-bulid-type=${enable_build_type}"
          ],
          [
            use_trace_fds_polling="no"
            use_trace_fds_polling_MSG="no"
          ]
        )
      ],
      [AC_MSG_FAILURE([configure is broken])]
    )
  ]
)
AS_IF([test "X${use_trace_fds_polling}" = "Xyes" && test "X${mhd_cv_macro_trace_polling_fds_def}" != "Xyes"],
  [AC_DEFINE([[MHD_USE_TRACE_POLLING_FDS]], [[1]], [Define to enable debug-print of FDs polling])]
)
test -n "${use_trace_fds_polling_MSG}" || use_trace_fds_polling_MSG}="${use_trace_fds_polling}"
AC_MSG_RESULT([${use_trace_fds_polling_MSG}])

AC_ARG_ENABLE([[trace-conn-add-close]],
  [AS_HELP_STRING([[--enable-trace-conn-add-close]],
    [enable test build with debug printing of connections adding and closing])],
  [
    AS_CASE([${enable_trace_conn_add_close}],
      [yes|no],[:],
      [AC_MSG_ERROR([wrong parameter --enable-trace-conn-add-close=${enable_trace_conn_add_close}])]
    )
  ],
  [enable_trace_conn_add_close='auto']
)
AS_UNSET([use_trace_conn_add_close_MSG])
AS_UNSET([use_trace_conn_add_close])
AC_CACHE_CHECK([[whether MHD_USE_TRACE_CONN_ADD_CLOSE macro is defined]],
  [mhd_cv_macro_trace_conn_add_close_def],
  [
    AC_LINK_IFELSE(
      [
        AC_LANG_SOURCE([[
#ifndef MHD_USE_TRACE_CONN_ADD_CLOSE
#error MHD_USE_TRACE_CONN_ADD_CLOSE is NOT defined
chome me now
#endif

int main(void)
{ return 0; }
          ]]
        )
      ],
      [[mhd_cv_macro_trace_conn_add_close_def='yes']],
      [[mhd_cv_macro_trace_conn_add_close_def='no']]
    )
  ]
)
AC_MSG_CHECKING([whether to enable debug-print of of connections adding and closing])
AS_VAR_IF([mhd_cv_macro_trace_conn_add_close_def],["yes"],
  [
    use_trace_conn_add_close="yes"
    use_trace_conn_add_close_MSG="yes, forced by MHD_USE_TRACE_CONN_ADD_CLOSE macro"
  ]
)
AS_VAR_SET_IF([use_trace_conn_add_close],[:],
  [
    AS_CASE([${enable_trace_conn_add_close}],
      [yes],[
        use_trace_conn_add_close="yes"
        use_trace_conn_add_close_MSG="yes, enabled by --enable-trace-conn-add-close"
      ],
      [no],[
        use_trace_conn_add_close="no"
        use_trace_conn_add_close_MSG="no, disabled by --disable-trace-conn-add-close"
      ],
      [auto],[
        AS_CASE([${enable_build_type}],
          [trace],[
            use_trace_conn_add_close="yes"
            use_trace_conn_add_close_MSG="yes, enabled by --enable-bulid-type=${enable_build_type}"
          ],
          [
            use_trace_conn_add_close="no"
            use_trace_conn_add_close_MSG="no"
          ]
        )
      ],
      [AC_MSG_FAILURE([configure is broken])]
    )
  ]
)
AS_IF([test "X${use_trace_conn_add_close}" = "Xyes" && test "X${mhd_cv_macro_trace_conn_add_close_def}" != "Xyes"],
  [AC_DEFINE([[MHD_USE_TRACE_CONN_ADD_CLOSE]], [[1]], [Define to enable debug-print of connection adding and closing])]
)
test -n "${use_trace_conn_add_close_MSG}" || use_trace_conn_add_close_MSG="${use_trace_conn_add_close}"
AC_MSG_RESULT([${use_trace_conn_add_close_MSG}])

AC_ARG_ENABLE([[trace-suspend-resume]],
  [AS_HELP_STRING([[--enable-trace-suspend-resume]],
    [enable test build with debug printing of requests suspending and resuming])],
  [
    AS_CASE([${enable_trace_suspend_resume}],
      [yes|no],[:],
      [AC_MSG_ERROR([wrong parameter --enable-trace-suspend-resume=${enable_trace_suspend_resume}])]
    )
  ],
  [enable_trace_suspend_resume='auto']
)
AS_UNSET([use_trace_suspend_resume_MSG])
AS_UNSET([use_trace_suspend_resume])
AC_CACHE_CHECK([[whether MHD_USE_TRACE_SUSPEND_RESUME macro is defined]],
  [mhd_cv_macro_trace_suspend_resume_def],
  [
    AC_LINK_IFELSE(
      [
        AC_LANG_SOURCE([[
#ifndef MHD_USE_TRACE_SUSPEND_RESUME
#error MHD_USE_TRACE_SUSPEND_RESUME is NOT defined
chome me now
#endif

int main(void)
{ return 0; }
          ]]
        )
      ],
      [[mhd_cv_macro_trace_suspend_resume_def='yes']],
      [[mhd_cv_macro_trace_suspend_resume_def='no']]
    )
  ]
)
AC_MSG_CHECKING([whether to enable debug-print of of requests suspending and resuming])
AS_VAR_IF([mhd_cv_macro_trace_suspend_resume_def],["yes"],
  [
    use_trace_suspend_resume="yes"
    use_trace_suspend_resume_MSG="yes, forced by MHD_USE_TRACE_SUSPEND_RESUME macro"
  ]
)
AS_VAR_SET_IF([use_trace_suspend_resume],[:],
  [
    AS_CASE([${enable_trace_suspend_resume}],
      [yes],[
        use_trace_suspend_resume="yes"
        use_trace_suspend_resume_MSG="yes, enabled by --enable-trace-suspend-resume"
      ],
      [no],[
        use_trace_suspend_resume="no"
        use_trace_suspend_resume_MSG="no, disabled by --disable-trace-suspend-resume"
      ],
      [auto],[
        AS_CASE([${enable_build_type}],
          [trace],[
            use_trace_suspend_resume="yes"
            use_trace_suspend_resume_MSG="yes, enabled by --enable-bulid-type=${enable_build_type}"
          ],
          [
            use_trace_suspend_resume="no"
            use_trace_suspend_resume_MSG="no"
          ]
        )
      ],
      [AC_MSG_FAILURE([configure is broken])]
    )
  ]
)
AS_IF([test "X${use_trace_suspend_resume}" = "Xyes" && test "X${mhd_cv_macro_trace_suspend_resume_def}" != "Xyes"],
  [AC_DEFINE([[MHD_USE_TRACE_SUSPEND_RESUME]], [[1]], [Define to enable debug-print of requests suspending and resuming])]
)
test -n "${use_trace_suspend_resume_MSG}" || use_trace_suspend_resume_MSG="${use_trace_suspend_resume}"
AC_MSG_RESULT([${use_trace_suspend_resume_MSG}])

AS_UNSET([enabled_sanitizers])
TESTS_ENVIRONMENT_ac=""
AM_ASAN_OPTIONS=""
AM_UBSAN_OPTIONS=""
AM_LSAN_OPTIONS=""
AS_UNSET([ASAN_OPTIONS])
AS_UNSET([UBSAN_OPTIONS])
AS_UNSET([LSAN_OPTIONS])

AC_MSG_CHECKING([whether to enable run-time sanitizers])
AC_ARG_ENABLE([sanitizers],
  [AS_HELP_STRING([[--enable-sanitizers[=address,undefined,leak,user-poison]]],
  [enable run-time sanitizers, specify the list of types of sanitizers to enable, ]
  [leave the list empty (or set to "auto") to enable all supported and available ]
  [sanitizers, or specify "auto-fallback" to use sanitizers even without ]
  [installed sanitizer run-time library])],
  [], [AS_CASE([${enable_build_type}],[debug],
    [enable_sanitizers='auto-optional'],[enable_sanitizers='no'])]
)
AS_IF([test "x${enable_sanitizers}" = "x"], [enable_sanitizers="auto"])
AS_VAR_IF([enable_sanitizers], ["yes"], [enable_sanitizers="auto"])
AS_VAR_IF([enable_sanitizers], ["autofallback"], [enable_sanitizers="auto-fallback"])
AS_VAR_IF([enable_sanitizers], ["autooptional"], [enable_sanitizers="auto-optional"])
AS_IF([test "x${enable_sanitizers}" = "xno"],
  [
    enable_sanitizers="no"
    enable_san_address="no"
    enable_san_undef="no"
    enable_san_leak="no"
    enable_san_upoison="no"
  ],
  [test "x${enable_sanitizers}" = "xauto" || test "x${enable_sanitizers}" = "xauto-optional"],
  [
    AS_VAR_IF([enable_compiler_hardening],["yes"],
      [
        AS_VAR_IF([enable_sanitizers],["auto"],
          [AC_MSG_ERROR([sanitizers cannot be enabled with compiler hardening])],
          [AC_MSG_WARN([sanitizers cannot be enabled with compiler hardening])]
        )
        enable_sanitizers="no"
        enable_san_address="no"
        enable_san_undef="no"
        enable_san_leak="no"
        enable_san_upoison="no"
      ],
      [
        enable_san_address="auto"
        enable_san_undef="auto"
        enable_san_leak="auto"
        enable_san_upoison="auto"
      ]
    )
  ],
  [test "x${enable_sanitizers}" = "xauto-fallback"],
  [
    enable_san_address="auto"
    enable_san_undef="auto-fallback"
    enable_san_leak="auto"
    enable_san_upoison="auto"
  ],
  [
    AS_UNSET([san])
    enable_san_address="no"
    enable_san_undef="no"
    enable_san_leak="no"
    enable_san_upoison="no"
    f][or san in `AS_ECHO([${enable_sanitizers}]) | tr ',' ' '`
    do
      AS_CASE([$san],
        [address], [enable_san_address="yes"],
        [undefined], [enable_san_undef="yes"],
        [leak], [enable_san_leak="yes"],
        [user-poison|user_poison], [enable_san_upoison="yes"],
        [no|yes|auto|auto-fallback|autofallback], [AC_MSG_ERROR(["$san" cannot be used with other options for --enable-sanitizers=])],
        [AC_MSG_ERROR([Unknown parameter "$san" for --enable-sanitizers=])]
      )
    done
    AS_IF([test "x${enable_san_upoison}" = "xyes" && test "x${enable_san_address}" = "xno"],
      [AC_MSG_ERROR([User memory poisoning cannot be used without address sanitizer])]
    )
    enable_sanitizers="selected"
  ]
)
AS_CASE([${enable_sanitizers}],
  [selected], [AC_MSG_RESULT([selected])],
  [auto], [AC_MSG_RESULT([yes, detect and use supported sanitizers])],
  [auto-fallback], [AC_MSG_RESULT([yes, detect and use supported sanitizers even without run-time lib])],
  [auto-optional], [AC_MSG_RESULT([yes, detect and use supported sanitizers if any])],
  [AC_MSG_RESULT([no])]
)
AS_VAR_IF([enable_sanitizers], ["no"], [:],
 [
   AS_UNSET([san_FLAGS]) # the sanitizer flags to be added to both CFLAGS and LDFLAGS
   AS_UNSET([san_CFLAGS]) # the sanitizer flags to be added to CFLAGS
   AC_CACHE_CHECK([whether '-fsanitize=' works for $CC],
     [mhd_cv_cc_sanitizer_works],
     [
       CFLAGS="${CFLAGS_ac} -fsanitize=wrongFeatureName ${user_CFLAGS}"
       AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[])],
         [mhd_cv_cc_sanitizer_works=no], [mhd_cv_cc_sanitizer_works=yes])
     ]
   )
   AS_VAR_IF([mhd_cv_cc_sanitizer_works], ["yes"],
     [
       AS_VAR_IF([enable_san_address], ["no"], [:],
         [
           AC_CACHE_CHECK([for address sanitizer], [mhd_cv_cc_sanitizer_address],
             [
               CFLAGS="${CFLAGS_ac} ${san_CFLAGS} ${san_FLAGS} -fsanitize=address ${user_CFLAGS}"
               AC_LINK_IFELSE([AC_LANG_PROGRAM([],[])],
                [mhd_cv_cc_sanitizer_address=yes], [mhd_cv_cc_sanitizer_address=no])
             ]
           )
           AS_VAR_IF([mhd_cv_cc_sanitizer_address],["yes"],
             [
               AC_DEFINE([MHD_ASAN_ACTIVE], [1], [Define to '1' if you have address sanitizer enabled])
               AX_APPEND_FLAG([-fsanitize=address], [san_FLAGS])
               enabled_sanitizers="${enabled_sanitizers}${enabled_sanitizers:+, }address"
               AS_VAR_IF([enable_san_leak], ["no"], [:],
                 [
                   AC_CACHE_CHECK([whether leak detect is not rejected by address sanitizer], [mhd_cv_cc_sanitizer_address_leak],
                     [
                       CFLAGS="${CFLAGS_ac} ${san_CFLAGS} ${san_FLAGS} ${user_CFLAGS}"
                       ASAN_OPTIONS="exitcode=88:detect_leaks=1:halt_on_error=1"
                       export ASAN_OPTIONS
                       AC_RUN_IFELSE([AC_LANG_PROGRAM([],[])],
                         [mhd_cv_cc_sanitizer_address_leak=yes], [mhd_cv_cc_sanitizer_address_leak=no],
                         [
                           # Cross-compiling with sanitizers?
                           mhd_cv_cc_sanitizer_address_leak='assuming no'
                         ]
                       )
                       AS_UNSET([ASAN_OPTIONS])
                     ]
                   )
                 ]
               )
               AC_CACHE_CHECK([for pointer compare sanitizer], [mhd_cv_cc_sanitizer_pointer_compare],
                 [
                   CFLAGS="${CFLAGS_ac} ${san_CFLAGS} ${san_FLAGS} -fsanitize=pointer-compare ${user_CFLAGS}"
                   AC_LINK_IFELSE([AC_LANG_PROGRAM([],[])],
                    [mhd_cv_cc_sanitizer_pointer_compare=yes], [mhd_cv_cc_sanitizer_pointer_compare=no])
                 ]
               )
               AS_VAR_IF([mhd_cv_cc_sanitizer_pointer_compare],["yes"],
                 [
                   AX_APPEND_FLAG([-fsanitize=pointer-compare], [san_FLAGS])
                   enabled_sanitizers="${enabled_sanitizers}${enabled_sanitizers:+, }pointer compare"
                 ]
               )
               AC_CACHE_CHECK([for pointer subtract sanitizer], [mhd_cv_cc_sanitizer_pointer_subtract],
                 [
                   CFLAGS="${CFLAGS_ac} ${san_CFLAGS} ${san_FLAGS} -fsanitize=pointer-subtract ${user_CFLAGS}"
                   AC_LINK_IFELSE([AC_LANG_PROGRAM([],[])],
                    [mhd_cv_cc_sanitizer_pointer_subtract=yes], [mhd_cv_cc_sanitizer_pointer_subtract=no])
                 ]
               )
               AS_VAR_IF([mhd_cv_cc_sanitizer_pointer_subtract],["yes"],
                 [
                   AX_APPEND_FLAG([-fsanitize=pointer-subtract], [san_FLAGS])
                   enabled_sanitizers="${enabled_sanitizers}${enabled_sanitizers:+, }pointer subtract"
                 ]
               )
               AS_VAR_IF([enable_san_upoison], ["no"], [:],
                 [
                   AC_CHECK_HEADERS([sanitizer/asan_interface.h], [], [], [AC_INCLUDES_DEFAULT])
                   AS_VAR_IF([ac_cv_header_sanitizer_asan_interface_h],["yes"],
                     [
                       CFLAGS="${CFLAGS_ac} ${san_CFLAGS} ${san_FLAGS} ${errattr_CFLAGS} ${user_CFLAGS}"
                       MHD_CHECK_FUNC([__asan_address_is_poisoned],[[#include <sanitizer/asan_interface.h>]],
                         [[int a_var=1; i][f(__asan_address_is_poisoned((void*) &a_var)) return 3;]]
                       )
                       MHD_CHECK_FUNC([__asan_region_is_poisoned],[[#include <sanitizer/asan_interface.h>]],
                         [[int a_var=1; i][f(((void*) 0) != __asan_region_is_poisoned((void*) &a_var, sizeof(a_var))) return 3;]]
                       )
                       AC_CACHE_CHECK([whether special function attribute is needed for user-poison], [mhd_cv_func_u_p_attribute_needed],
                         [
                           ASAN_OPTIONS="exitcode=88:detect_invalid_pointer_pairs=3:halt_on_error=1"
                           export ASAN_OPTIONS
                           CFLAGS="${CFLAGS_ac} ${san_CFLAGS} ${san_FLAGS} ${errattr_CFLAGS} ${user_CFLAGS}"
                           AC_RUN_IFELSE(
                             [
                               AC_LANG_SOURCE(
                                 [[
#include <stdint.h>
#include <stdlib.h>
#include <sanitizer/asan_interface.h>

static const size_t first_pos = 0;
static const size_t mid_pos = 64;
static const size_t last_pos = 128;
static const size_t zone_size = 16;
static const size_t buf_size = 128 + 16;

static int ptr_compare(void *ptr1, uint8_t *ptr2)
{
  if ((((uintptr_t) (uint8_t *)ptr1) >= ((uintptr_t)ptr2)))
    return ((char *) ptr1)[0] < ((char *) ptr2)[0];
  return ((char *) ptr1)[0] > ((char *) ptr2)[0];
}

static int ptr_subtract(void *ptr1, uint8_t *ptr2)
{
  return ((size_t)(((uintptr_t)(uint8_t*)ptr1) - ((uintptr_t)ptr2))) <= last_pos;
}

int main(int argc, char *argv[])
{
  char *buf = (char*) malloc (buf_size);
  char *a;
  char *b;
  int ret;

  (void) argv;
  if (NULL == buf)
    return 10;
  ASAN_POISON_MEMORY_REGION (buf + first_pos + zone_size, mid_pos - first_pos - zone_size);
  ASAN_POISON_MEMORY_REGION (buf + mid_pos + zone_size, last_pos - mid_pos - zone_size);

  if (0 < argc)
    a = buf + last_pos;
  else
    a = buf + first_pos;
  b = buf + mid_pos;

  *a = '0';
  *b = '9';

  if (ptr_compare((void *)a, (uint8_t*) b))
  {
    if (ptr_subtract((void *)a, (uint8_t*) b))
      ret = 0;
    else
      ret = 10;
  }
  else
    ret = 5;
  ASAN_UNPOISON_MEMORY_REGION (buf, buf_size);
  free (buf);

  return ret;
}
                                 ]]
                               )
                             ],
                             [mhd_cv_func_u_p_attribute_needed="no"], [mhd_cv_func_u_p_attribute_needed="yes"],
                             [
                               # Cross-compiling with sanitizers??
                               mhd_cv_func_up_attribute_needed='assuming no'
                             ]
                           )
                           AS_UNSET([ASAN_OPTIONS])
                         ]
                       )
                     ]
                   )
                   AS_VAR_IF([mhd_cv_func_u_p_attribute_needed],["yes"],[:],
                     [
                       AC_DEFINE([FUNC_PTRCOMPARE_CAST_WORKAROUND_WORKS],[1],[Define to '1' if cast to 'uintptr_t' works for safely processing user-poisoned pointer])
                     ]
                   )
                   AS_IF([test "x${mhd_cv_func_u_p_attribute_needed}" = "xyes" && test "x${ac_cv_header_sanitizer_asan_interface_h}" = "xyes"],
                     [
                       AC_CACHE_CHECK([whether '__attribute__((no_sanitize("pointer-compare")))' and '__attribute__((no_sanitize("pointer-subtract")))' work],
                         [mhd_cv_func_attribute_nosanitize_ptr],
                         [
                           ASAN_OPTIONS="exitcode=88:detect_invalid_pointer_pairs=3:halt_on_error=1"
                           export ASAN_OPTIONS
                           CFLAGS="${CFLAGS_ac} ${san_CFLAGS} ${san_FLAGS} ${errattr_CFLAGS} ${user_CFLAGS}"
                           AC_RUN_IFELSE(
                             [
                               AC_LANG_SOURCE(
                                 [[
#include <stdint.h>
#include <stdlib.h>
#include <sanitizer/asan_interface.h>

static const size_t first_pos = 0;
static const size_t mid_pos = 64;
static const size_t last_pos = 128;
static const size_t zone_size = 16;
static const size_t buf_size = 128 + 16;

__attribute__((no_sanitize("pointer-compare")))
static int ptr_compare(void *ptr1, uint8_t *ptr2)
{
  if ((((const uint8_t*)ptr1) >= ((const uint8_t*)ptr2)))
    return ((char *) ptr1)[0] < ((char *) ptr2)[0];
  return ((char *) ptr1)[0] > ((char *) ptr2)[0];
}

__attribute__((no_sanitize("pointer-subtract")))
static int ptr_subtract(void *ptr1, uint8_t *ptr2)
{
  return ((size_t)(((const uint8_t*)ptr1) - \
          ((const uint8_t*)ptr2))) <= last_pos;
}

int main(int argc, char *argv[])
{
  char *buf = (char*) malloc (buf_size);
  char *a;
  char *b;
  int ret;

  (void) argv;
  if (NULL == buf)
    return 10;
  ASAN_POISON_MEMORY_REGION (buf + first_pos + zone_size, mid_pos - first_pos - zone_size);
  ASAN_POISON_MEMORY_REGION (buf + mid_pos + zone_size, last_pos - mid_pos - zone_size);

  if (0 < argc)
    a = buf + last_pos;
  else
    a = buf + first_pos;
  b = buf + mid_pos;

  *a = '0';
  *b = '9';

  if (ptr_compare((void *)a, (uint8_t*) b))
  {
    if (ptr_subtract((void *)a, (uint8_t*) b))
      ret = 0;
    else
      ret = 10;
  }
  else
    ret = 5;
  ASAN_UNPOISON_MEMORY_REGION (buf, buf_size);
  free (buf);

  return ret;
}
                                 ]]
                               )
                             ],
                             [mhd_cv_func_attribute_nosanitize_ptr=yes], [mhd_cv_func_attribute_nosanitize_ptr=no],
                             [
                               # Cross-compiling with sanitizers??
                               mhd_cv_func_attribute_nosanitize_ptr='assuming no'
                             ]
                           )
                           AS_UNSET([ASAN_OPTIONS])
                         ]
                       )
                       AS_VAR_IF([mhd_cv_func_attribute_nosanitize_ptr], ["yes"],
                         [
                           AC_DEFINE([FUNC_ATTR_PTRCOMPARE_WORKS],[1],[Define to '1' if '__attribute__((no_sanitize("pointer-compare")))' works])
                           AC_DEFINE([FUNC_ATTR_PTRSUBTRACT_WORKS],[1],[Define to '1' if '__attribute__((no_sanitize("pointer-subtract")))' works])
                         ],
                         [
                           AC_CACHE_CHECK([whether '__attribute__((no_sanitize("address")))' works for pointers compare], [mhd_cv_func_attribute_nosanitize_addr],
                             [
                               ASAN_OPTIONS="exitcode=88:detect_invalid_pointer_pairs=3:halt_on_error=1"
                               export ASAN_OPTIONS
                               CFLAGS="${CFLAGS_ac} ${san_CFLAGS} ${san_FLAGS} ${errattr_CFLAGS} ${user_CFLAGS}"
                               AC_RUN_IFELSE(
                                 [
                                   AC_LANG_SOURCE(
                                     [[
#include <stdint.h>
#include <stdlib.h>
#include <sanitizer/asan_interface.h>

static const size_t first_pos = 0;
static const size_t mid_pos = 64;
static const size_t last_pos = 128;
static const size_t zone_size = 16;
static const size_t buf_size = 128 + 16;

__attribute__((no_sanitize("address")))
static int ptr_compare(void *ptr1, uint8_t *ptr2)
{
  if ((((const uint8_t*)ptr1) >= ((const uint8_t*)ptr2)))
    return ((char *) ptr1)[0] < ((char *) ptr2)[0];
  return ((char *) ptr1)[0] > ((char *) ptr2)[0];
}

__attribute__((no_sanitize("address")))
static int ptr_subtract(void *ptr1, uint8_t *ptr2)
{
  return ((size_t)(((const uint8_t*)ptr1) - \
          ((const uint8_t*)ptr2))) <= last_pos;
}

int main(int argc, char *argv[])
{
  char *buf = (char*) malloc (buf_size);
  char *a;
  char *b;
  int ret;

  (void) argv;
  if (NULL == buf)
    return 10;
  ASAN_POISON_MEMORY_REGION (buf + first_pos + zone_size, mid_pos - first_pos - zone_size);
  ASAN_POISON_MEMORY_REGION (buf + mid_pos + zone_size, last_pos - mid_pos - zone_size);

  if (0 < argc)
    a = buf + last_pos;
  else
    a = buf + first_pos;
  b = buf + mid_pos;

  *a = '0';
  *b = '9';

  if (ptr_compare((void *)a, (uint8_t*) b))
  {
    if (ptr_subtract((void *)a, (uint8_t*) b))
      ret = 0;
    else
      ret = 10;
  }
  else
    ret = 5;
  ASAN_UNPOISON_MEMORY_REGION (buf, buf_size);
  free (buf);

  return ret;
}
                                     ]]
                                   )
                                 ],
                                 [mhd_cv_func_attribute_nosanitize_addr=yes], [mhd_cv_func_attribute_nosanitize_addr=no],
                                 [
                                   # Cross-compiling with sanitizers??
                                   mhd_cv_func_attribute_nosanitize_addr='assuming no'
                                 ]
                               )
                               AS_UNSET([ASAN_OPTIONS])
                             ]
                           )
                           AS_VAR_IF([mhd_cv_func_attribute_nosanitize_addr], ["yes"],
                             [AC_DEFINE([FUNC_ATTR_NOSANITIZE_WORKS],[1],[Define to '1' if '__attribute__((no_sanitize("address")))' works for pointers compare])]
                           )
                         ]
                       )
                     ]
                   )
                 ]
               )
             ]
           )
           AS_IF([test "x${enable_san_address}" = "xyes" && test "x${mhd_cv_cc_sanitizer_address}" != "xyes"],
             [AC_MSG_ERROR([Address sanitizer cannot be enabled])]
           )
           enable_san_address="${mhd_cv_cc_sanitizer_address}"
         ]
       )
       AS_VAR_IF([enable_san_undef], ["no"], [:],
         [
           dnl Ensure that '#' will be processed correctly
           [
            test_undf_prog='
#include <stdio.h>

static void func_out_b(char *arr)
{
  arr[0] = 0;
  arr[16] = 2;
}

static unsigned int int_deref(void *ptr)
{
  return (*((unsigned int*)ptr)) + 2;
}

static int func1(void)
{
  char chr[16];
  func_out_b (chr);
  return (int) (int_deref(chr + 1) + int_deref(chr + 2));
}

int main(void)
{
  unsigned long ulvar;
  signed char ch1;
  ulvar = -1 * func1();
  ch1 = ulvar * 6UL;
  printf("%lu\n", ulvar + ch1);
  return 0;
}
            '
           ]
           AC_CACHE_CHECK([for undefined behavior sanitizer], [mhd_cv_cc_sanitizer_undefined],
             [
               CFLAGS="${CFLAGS_ac} ${san_FLAGS} ${san_CFLAGS} -fsanitize=undefined ${user_CFLAGS}"
               AC_LINK_IFELSE([AC_LANG_SOURCE([${test_undf_prog}])],
                [mhd_cv_cc_sanitizer_undefined=yes], [mhd_cv_cc_sanitizer_undefined=no])
             ]
           )
           AS_VAR_IF([mhd_cv_cc_sanitizer_undefined],["yes"],
             [
               AX_APPEND_FLAG([-fsanitize=undefined], [san_FLAGS])
               enabled_sanitizers="${enabled_sanitizers}${enabled_sanitizers:+, }undefined"
             ],
             [
               AS_CASE([${enable_san_undef}], [yes|auto-fallback],
                 [
                   AC_CACHE_CHECK([for undefined behavior sanitizer with '-fsanitize-undefined-trap-on-error'], [mhd_cv_cc_sanitizer_undefined_trap],
                     [
                       CFLAGS="${CFLAGS_ac} ${san_FLAGS} ${san_CFLAGS} -fsanitize=undefined -fsanitize-undefined-trap-on-error ${user_CFLAGS}"
                       AC_LINK_IFELSE([AC_LANG_SOURCE([${test_undf_prog}])],
                        [mhd_cv_cc_sanitizer_undefined_trap=yes], [mhd_cv_cc_sanitizer_undefined_trap=no])
                     ]
                   )
                   AS_VAR_IF([mhd_cv_cc_sanitizer_undefined_trap], ["yes"],
                     [
                       AX_APPEND_FLAG([-fsanitize=undefined], [san_FLAGS])
                       AX_APPEND_FLAG([-fsanitize-undefined-trap-on-error], [san_FLAGS])
                       enabled_sanitizers="${enabled_sanitizers}${enabled_sanitizers:+, }undefined"
                       AC_MSG_WARN([Enabled sanitizer without run-time library, error reporting will be limited])
                     ],
                     [
                       AS_IF([test -z "${enabled_sanitizers}"],
                         [
                           # Last resort
                           AC_CACHE_CHECK([for undefined behavior sanitizer with '-fsanitize-trap=all'], [mhd_cv_cc_sanitizer_undefined_trap_all],
                             [
                               CFLAGS="${CFLAGS_ac} ${san_FLAGS} ${san_CFLAGS} -fsanitize=undefined -fsanitize-trap=all ${user_CFLAGS}"
                               AC_LINK_IFELSE([AC_LANG_SOURCE([${test_undf_prog}])],
                                [mhd_cv_cc_sanitizer_undefined_trap_all=yes], [mhd_cv_cc_sanitizer_undefined_trap_all=no])
                             ]
                           )
                           AS_VAR_IF([mhd_cv_cc_sanitizer_undefined_trap_all],["yes"],
                             [
                               AX_APPEND_FLAG([-fsanitize=undefined], [san_FLAGS])
                               AX_APPEND_FLAG([-fsanitize-trap=all], [san_FLAGS])
                               CFLAGS="${CFLAGS_ac} ${san_FLAGS} ${san_CFLAGS} -fsanitize=undefined -fsanitize-trap=all ${user_CFLAGS}"
                               enabled_sanitizers="${enabled_sanitizers}${enabled_sanitizers:+, }undefined"
                               AC_MSG_WARN([Enabled sanitizer without run-time library, error reporting will be limited])
                             ]
                           )
                         ]
                       )
                     ]
                   )
                   AS_CASE(["$enabled_sanitizers"], [*undefined],
                     [
                       AS_VAR_IF([mhd_cv_cc_sanitizer_undefined], ["yes"],[],
                         [
                           # A workaround for broken clang which is trying to use UBSan lib
                           # even when instructed to not use it
                           CFLAGS="${CFLAGS_ac} ${san_FLAGS} ${san_CFLAGS} ${user_CFLAGS}"
                           AX_APPEND_LINK_FLAGS([-fsanitize-trap=implicit-conversion],
                             [san_FLAGS], [], [AC_LANG_SOURCE([${test_undf_prog}])])
                         ]
                       )
                     ]
                   )
                 ]
               )
             ]
           )
           AS_CASE(["$enabled_sanitizers"], [*undefined],
             [
               CFLAGS="${CFLAGS_ac} ${san_FLAGS} ${san_CFLAGS} ${user_CFLAGS}"
               AX_APPEND_LINK_FLAGS([-fsanitize=bounds-strict -fsanitize=local-bounds -fsanitize=implicit-conversion -fsanitize=nullability-arg],
                 [san_CFLAGS], [], [AC_LANG_SOURCE([${test_undf_prog}])])
             ]
           )
           AS_UNSET([test_undf_prog])
           AS_CASE(["$enabled_sanitizers"],
             [*undefined], [enable_san_undef="yes"],
             [
               AS_VAR_IF([enable_san_undef], [yes], [AC_MSG_ERROR([Undefined behavior sanitizer cannot be enabled])])
               enable_san_undef="no"
             ]
           )
         ]
       )
       AS_VAR_IF([enable_san_leak], ["no"], [:],
         [
           AC_CACHE_CHECK([for leak sanitizer], [mhd_cv_cc_sanitizer_leak],
             [
               CFLAGS="${CFLAGS_ac} ${san_FLAGS} ${san_CFLAGS} -fsanitize=leak ${user_CFLAGS}"
               AC_LINK_IFELSE([AC_LANG_PROGRAM([],[])],
                [mhd_cv_cc_sanitizer_leak=yes], [mhd_cv_cc_sanitizer_leak=no])
             ]
           )
           AS_VAR_IF([mhd_cv_cc_sanitizer_leak],["yes"],
             [
               AX_APPEND_FLAG([-fsanitize=leak], [san_FLAGS])
               enabled_sanitizers="${enabled_sanitizers}${enabled_sanitizers:+, }leak"
             ]
           )
           AS_IF([test "x${enable_san_leak}" = "xyes" && test "x${mhd_cv_cc_sanitizer_leak}" != "xyes"],
             [AC_MSG_ERROR([User poison cannot be enabled])]
           )
           enable_san_leak="${mhd_cv_cc_sanitizer_leak}"
         ]
       )
       AS_IF([test -z "${enabled_sanitizers}"],
         [
           AS_VAR_IF([enable_sanitizers], ["auto-optional"],
             [
               san_FLAGS=""
               san_CFLAGS=""
             ],
             [
               AC_MSG_ERROR([cannot find any sanitizer supported by $CC])
             ]
           )
         ],
         [
           AS_VAR_IF([enable_san_upoison], ["no"], [:],
             [
               AC_MSG_CHECKING([whether to enable user memory poisoning])
               AS_IF([test "x${mhd_cv_cc_sanitizer_address}" = "xyes" && test "x${ac_cv_header_sanitizer_asan_interface_h}" = "xyes" && \
                 (test "x${mhd_cv_func_u_p_attribute_needed}" != "xyes" || test "x${mhd_cv_func_attribute_nosanitize_ptr}" = "xyes" || \
                  test "x${mhd_cv_func_attribute_nosanitize_addr}" = "xyes")],
                 [
                   AC_DEFINE([MHD_ASAN_POISON_ACTIVE], [1], [Define to '1' if user memory poison is used])
                   enabled_sanitizers="${enabled_sanitizers}${enabled_sanitizers:+, }user-poison"
                   enable_san_upoison="yes"
                   AC_MSG_RESULT([yes])
                 ],
                 [
                   AC_MSG_RESULT([no])
                   AS_VAR_IF([enable_san_upoison], ["yes"],
                     [AC_MSG_ERROR([User memory poisoning cannot be enabled])])
                   enable_san_upoison="no"
                 ]
               )
             ]
           )
           AS_VAR_IF([enable_san_address], ["yes"],
             [
               CFLAGS="${CFLAGS_ac} ${san_FLAGS} ${san_CFLAGS} ${user_CFLAGS}"
               AC_CHECK_DECL([_FORTIFY_SOURCE],
                 [MHD_APPEND_FLAG_TO_VAR([CPPFLAGS_ac],[-U_FORTIFY_SOURCE])],
                 [],[/* no includes */])
               AX_APPEND_FLAG([-D_FORTIFY_SOURCE=0], [san_CFLAGS])
             ],
             [
               AS_CASE([$enable_sanitizers], [auto|auto-fallback],
                 [AC_MSG_WARN([$CC does not support address sanitizer])])
             ]
           )
           CFLAGS="${CFLAGS_ac} ${san_FLAGS} ${san_CFLAGS} ${user_CFLAGS}"
           # Always stop on sanitizer error
           AX_APPEND_COMPILE_FLAGS([-fno-sanitize-recover=all], [san_CFLAGS])
           # Get a better output for sanitizers error reporting
           AX_APPEND_COMPILE_FLAGS([-fno-omit-frame-pointer -fno-optimize-sibling-calls],
             [san_CFLAGS])
           AS_VAR_IF([enable_san_address], ["yes"],
             [
               AM_ASAN_OPTIONS="exitcode=88:strict_string_checks=1:detect_stack_use_after_return=1"
               AM_ASAN_OPTIONS="${AM_ASAN_OPTIONS}:check_initialization_order=1:strict_init_order=1:redzone=64"
               AM_ASAN_OPTIONS="${AM_ASAN_OPTIONS}:max_free_fill_size=1024:detect_invalid_pointer_pairs=3"
               AM_ASAN_OPTIONS="${AM_ASAN_OPTIONS}:handle_ioctl=1:halt_on_error=1"
               AS_VAR_IF([enable_san_upoison], ["yes"], [AM_ASAN_OPTIONS="${AM_ASAN_OPTIONS}:allow_user_poisoning=1"])
               AS_VAR_IF([enable_san_leak], ["yes"],
                 [AS_VAR_IF([mhd_cv_cc_sanitizer_address_leak], ["yes"],
                   [AM_ASAN_OPTIONS="${AM_ASAN_OPTIONS}:detect_leaks=1"])
                 ], [AM_ASAN_OPTIONS="${AM_ASAN_OPTIONS}:detect_leaks=0"]
               )
             ]
           )
           AS_VAR_IF([enable_san_undef], [yes],
             [AM_UBSAN_OPTIONS="exitcode=87:print_stacktrace=1:halt_on_error=1"])
           AS_VAR_IF([enable_san_leak], ["yes"],
             [AM_LSAN_OPTIONS="use_unaligned=1"]
           )
           TESTS_ENVIRONMENT_ac='\
    ASAN_OPTIONS="$(AM_ASAN_OPTIONS)" ; export ASAN_OPTIONS ; \
    UBSAN_OPTIONS="$(AM_UBSAN_OPTIONS)" ; export UBSAN_OPTIONS ; \
    LSAN_OPTIONS="$(AM_LSAN_OPTIONS)" ; export LSAN_OPTIONS ;'
         ]
       )
     ]
   )
   CFLAGS_ac="${CFLAGS_ac} ${san_FLAGS} ${san_CFLAGS}"
   CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
 ]
)

# fuzzing tests
run_zzuf_tests="no"
zzuf_socat_mandatory="no"
AS_VAR_IF([use_heavy_tests],["yes"],
  [
    AS_VAR_IF([enable_curl],["yes"],
      [
        AC_PATH_PROG([ZZUF],[zzuf],[no])
        AS_VAR_IF([ZZUF],["no"],
          [
            run_zzuf_tests="no"
            run_zzuf_tests_MSG="no, zzuf tool not found"
          ],
          [
	        AC_PATH_PROG([SOCAT],[socat],[no])
            AS_IF([test "x${ac_cv_func_accept4}" = "xyes" && test "x${enable_asserts}" = "xno"],
              [
                zzuf_socat_mandatory="yes"
		        AS_VAR_IF([SOCAT],["no"],
                  [run_zzuf_tests_MSG="no, socat not found, but required for fuzzing tests on this platform without asserts enabled"],
                  [
		            run_zzuf_tests="yes"
		            run_zzuf_tests_MSG="yes, with zzuf and socat tools"
                  ]
                )
              ],
              [test -n "${enabled_sanitizers}"],
              [
                zzuf_socat_mandatory="yes"
		        AS_VAR_IF([SOCAT],["no"],
                  [run_zzuf_tests_MSG="no, socat not found, but required for fuzzing tests when sanitizers enabled"],
                  [
		            run_zzuf_tests="yes"
		            run_zzuf_tests_MSG="yes, with zzuf and socat tools"
                  ]
                )
              ],
              [
                run_zzuf_tests="yes"
                run_zzuf_tests_MSG="yes, with zzuf tool"
              ]
            )
          ]
        )
      ],
      [
        run_zzuf_tests="no"
        run_zzuf_tests_MSG="no, tests with libcurl are not enabled"
      ]
    )
  ],
  [
    run_zzuf_tests="no"
    run_zzuf_tests_MSG="no, heavy tests are not enabled"
  ]
)
AM_CONDITIONAL([RUN_ZZUF_TESTS],[test "x$run_zzuf_tests" = "xyes"])
AM_CONDITIONAL([FORCE_USE_ZZUF_SOCAT],[test "x$zzuf_socat_mandatory" = "xyes"])

# Final flags that may interfere with autoconf detections
AS_CASE([${enable_build_type}],[debug|debugger|trace],
  [ # Debug build, build for walking with debugger or debug printing build
    CFLAGS="${user_CFLAGS}"
    MHD_CHECK_ADD_CC_CFLAGS([-Wextra-semi -Wextra-semi-stmt], [CFLAGS_ac])
    MHD_FIND_ADD_CC_CFLAG([CFLAGS_ac], [-Wleading-whitespace=spaces], [-Wleading-whitespace])
    CFLAGS="${CFLAGS_ac} ${user_CFLAGS}"
  ]
)

AM_CONDITIONAL([USE_SANITIZERS],
  [test -n "$enabled_sanitizers" && test "x$mhd_cv_cc_sanitizer_works" = "xyes"])
AC_SUBST([AM_ASAN_OPTIONS])
AC_SUBST([AM_UBSAN_OPTIONS])
AC_SUBST([AM_LSAN_OPTIONS])
AC_SUBST([TESTS_ENVIRONMENT_ac])

MHD_LIB_LDFLAGS="$MHD_LIB_LDFLAGS"

AC_SUBST([CPU_COUNT])
AC_SUBST([HEAVY_TESTS_NOTPARALLEL])
AM_SUBST_NOTMAKE([HEAVY_TESTS_NOTPARALLEL])
AC_SUBST(MHD_LIB_CPPFLAGS)
AC_SUBST(MHD_LIB_CFLAGS)
AC_SUBST(MHD_LIB_LDFLAGS)
AC_SUBST(MHD_LIBDEPS)
AC_SUBST(MHD_TLS_LIB_CPPFLAGS)
AC_SUBST(MHD_TLS_LIB_LDFLAGS)
AC_SUBST(MHD_TLS_LIBDEPS)

# for pkg-config
AC_SUBST([MHD_REQ_PRIVATE])
AC_SUBST([MHD_LIBDEPS_PKGCFG])

# Restore flags as set by the user
CFLAGS="${user_CFLAGS}"
LDFLAGS="${user_LDFLAGS}"
CPPFLAGS="${user_CPPFLAGS}"
AC_SUBST([CFLAGS])
AC_SUBST([LDFLAGS])
AC_SUBST([CPPFLAGS])
AC_SUBST([LIBS])

# Configure-defined flags
AC_SUBST([CFLAGS_ac])
AC_SUBST([LDFLAGS_ac])
AC_SUBST([CPPFLAGS_ac])

# Used for 'po' directory staff
AC_SUBST([ac_configure_args])
AC_SUBST([EMPTY_VAR],[])
AC_SUBST([MHD_AUX_DIR])
AC_CONFIG_FILES([po/po-configure.ac])
AC_CONFIG_COMMANDS([po/Makefile.in],
  [
    echo "Skipping update of po/Makefile.in"
    echo "Real update of po/Makefile.in for 'make dist' is performed by po-config.status"
  ]
)
AC_CONFIG_COMMANDS([po-directories],
  [
    echo "Skipping po-directories command."
    echo "Real po-directories command for 'make dist' is implemented in po-config.status"
  ]
)

AC_SUBST([ZZUF])
AC_SUBST([SOCAT])

# SBOMs
AC_MSG_CHECKING([fo][r the effective library licence])
AS_UNSET([licence_SPDX])
AS_UNSET([licence_descr])
licence_num="0"
AS_VAR_IF([have_gnutls],["yes"],
  [AS_IF([test "2" -gt "$licence_num"],[licence_num="2"])]
)
AS_VAR_IF([have_openssl],["yes"],
  [AS_IF([test "3" -gt "$licence_num"],[licence_num="3"])]
)
AS_VAR_IF([have_mbedtls],["yes"],
  [AS_IF([test "3" -gt "$licence_num"],[licence_num="3"])]
)
AS_CASE([$licence_num],
  [0],
  [
    licence_SPDX="LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0)"
    licence_descr="LGPLv2.1+ or GPLv2+ with eCos exception"
  ],
  [2],
  [
    licence_SPDX="LGPL-2.1-or-later"
    licence_descr="LGPL version 2.1 or any later version"
  ],
  [3],
  [
    licence_SPDX="LGPL-3.0-or-later"
    licence_descr="LGPL version 3.0 or any later version"
  ],
  [AC_MSG_ERROR([internal error: unexpected licence selector value: ${licence_num}])]
)
AC_MSG_RESULT([${licence_descr}])
AC_SUBST([MHD_LICENSE_SPDX],[${licence_SPDX}])
AM_SUBST_NOTMAKE([MHD_LICENSE_SPDX])

AC_ARG_ENABLE([sboms],
[AS_HELP_STRING([[--enable-sboms[=TYPEs]]],[enable SBOMs generation (all,spdx,cdx) [all]])],
[
  AS_CASE([${enable_sboms}],
    [yes|no|all|cdx|spdx|cdx,spdx|spdx,cdx],[],
    [AC_MSG_ERROR([Unknown parameter value: --enable-sboms=${enable_sboms}])]
  )
],[enable_sboms="yes"]
)

use_sbom_spdx="no"
AS_CASE([${enable_sboms}],
  [yes|all|spdx|*,spdx|spdx,*|*,spdx,*],[use_sbom_spdx="yes"]
)
use_sbom_cdx="no"
AS_CASE([${enable_sboms}],
  [yes|all|cdx|*,cdx|cdx,*|*,cdx,*],[use_sbom_cdx="yes"]
)
AM_CONDITIONAL([ENABLE_SBOM_SPDX],[test "X${use_sbom_spdx}" = "Xyes"])
AM_CONDITIONAL([ENABLE_SBOM_CDX],[test "X${use_sbom_cdx}" = "Xyes"])

# should experimental code be compiled (code that may not yet compile)?
AC_MSG_CHECKING(whether to compile experimental code)
AC_ARG_ENABLE([experimental],
   [AS_HELP_STRING([--enable-experimental], [enable compiling experimental code])],
   [enable_experimental=${enableval}],
   [enable_experimental=no])
AC_MSG_RESULT($enable_experimental)
AM_CONDITIONAL([HAVE_EXPERIMENTAL], [test "x$enable_experimental" = "xyes"])


AC_CONFIG_FILES([
src/mhd2/libmicrohttpd2.pc
src/mhd2/w32_lib_res.rc
w32/common/microhttpd2.rc
Makefile
contrib/Makefile
doc/Makefile
m4/Makefile
src/Makefile
src/incl_priv/Makefile
src/incl_priv/config/Makefile
src/include/Makefile
src/mhd2/Makefile
src/tools/Makefile
src/tests/Makefile
src/tests/unit/Makefile
src/tests/basic/Makefile
src/tests/upgrade/Makefile
src/tests/raw/Makefile
src/tests/client_server/Makefile
src/examples2/Makefile
])
AC_OUTPUT

# Finally: summary
# Format flags without extra spaces for visual beauty
fin_CPPFLAGS="$user_CPPFLAGS"
fin_CFLAGS="$user_CFLAGS"
fin_LDFLAGS="$user_LDFLAGS"
MHD_PREPEND_FLAG_TO_VAR([fin_CPPFLAGS],[$CPPFLAGS_ac])
MHD_PREPEND_FLAG_TO_VAR([fin_CFLAGS],[$CFLAGS_ac])
MHD_PREPEND_FLAG_TO_VAR([fin_LDFLAGS],[$LDFLAGS_ac])
fin_lib_CPPFLAGS="$user_CPPFLAGS"
fin_lib_CFLAGS="$user_CFLAGS"
fin_lib_LDFLAGS="$user_LDFLAGS"
fin_lib_LIBS="${MHD_LIBDEPS}"
MHD_PREPEND_FLAG_TO_VAR([fin_lib_CPPFLAGS],[$MHD_LIB_CPPFLAGS])
MHD_PREPEND_FLAG_TO_VAR([fin_lib_CFLAGS],[$MHD_LIB_CFLAGS])
MHD_PREPEND_FLAG_TO_VAR([fin_lib_LDFLAGS],[$MHD_LIB_LDFLAGS])
MHD_PREPEND_FLAG_TO_VAR([fin_lib_CPPFLAGS],[$CPPFLAGS_ac])
MHD_PREPEND_FLAG_TO_VAR([fin_lib_CFLAGS],[$CFLAGS_ac])
MHD_PREPEND_FLAG_TO_VAR([fin_lib_LDFLAGS],[$LDFLAGS_ac])
MHD_PREPEND_FLAG_TO_VAR([fin_lib_LIBS],[${MHD_TLS_LIBDEPS}])
AC_MSG_NOTICE([Toolchain settings:
  CC=$CC
  User/system/default flags:
    CPPFLAGS="$user_CPPFLAGS"
    CFLAGS=  "$user_CFLAGS"
    LDFLAGS= "$user_LDFLAGS"
  Final set of the flags for tests and examples:
    CPPFLAGS="$fin_CPPFLAGS"
    CFLAGS=  "$fin_CFLAGS"
    LDFLAGS= "$fin_LDFLAGS"
    LIBS=    "$LIBS"
  Final set of the flags for ${PACKAGE_NAME} library:
    CPPFLAGS="$fin_lib_CPPFLAGS"
    CFLAGS=  "$fin_lib_CFLAGS"
    LDFLAGS= "$fin_lib_LDFLAGS"
    LIBS=    "$fin_lib_LIBS"
])

AS_UNSET([fin_CPPFLAGS])
AS_UNSET([fin_CFLAGS])
AS_UNSET([fin_LDFLAGS])
AS_UNSET([fin_lib_CPPFLAGS])
AS_UNSET([fin_lib_CFLAGS])
AS_UNSET([fin_lib_LDFLAGS])

AS_VAR_IF([os_is_windows], ["yes"],
  [os_ver_msg="
  Target W32 ver   : ${mhd_w32_ver_msg}"], [AS_UNSET([[os_ver_msg]])])

AC_MSG_NOTICE([[${PACKAGE_NAME} ${PACKAGE_VERSION} Configuration Summary:
  [ Target platform ]
  Target directory : ${prefix}
  Cross-compiling  : ${cross_compiling}
  Operating System : ${mhd_host_os}${os_ver_msg}
  Threading lib    : ${USE_THREADS}
  Shutdown of listening socket triggers select: ${mhd_cv_host_shtdwn_trgr_select}

  [ Library features and options ]
  HTTP/2 support   : ${enable_http2}
  Inter-thread comm: ${use_itc}
  select() support : ${enable_select}
  poll() support   : ${enable_poll=no}
  epoll support    : ${enable_epoll=no}
  kqueue support   : ${enable_kqueue=no}
  sendfile()       : ${found_sendfile}
  HTTPS support    : ${MSG_HTTPS}
  TLS backends     : ${MSG_TLS_BACKENDS}
  Cookie parser    : ${enable_cookie}
  POST parser      : ${enable_postparser}
  Basic auth.      : ${enable_basic_auth}
  Digest auth.     : ${enable_digest_auth}
  Digest auth. defaults: ${dauth_defs_MSG}
  MD5              : ${enable_md5_MSG}
  SHA-256          : ${enable_sha256_MSG}
  SHA-512/256      : ${enable_sha512_256_MSG}
  HTTP "Upgrade"   : ${enable_httpupgrade}
  Logging support  : ${enable_log_messages}
  Verbose auto replies: ${enable_http_messages}

  [ Code build options ]
  Compact code     : ${enable_compact_code} (${compact_code_MSG})
  LTO flags        : ${use_lto}
  Use thread names : ${enable_thread_names}
  Use debug asserts: ${use_asserts_MSG=no}
  Use sanitizers   : ${enabled_sanitizers:=no}
  
  [ Tracing debug prints ]
  Trace FDs polling: ${use_trace_fds_polling_MSG}
  Conns add/close  : ${use_trace_conn_add_close_MSG}
  Suspend/resume   : ${use_trace_suspend_resume_MSG}

  [ Build items ]
  Build static lib : ${enable_static}
  Build shared lib : ${enable_shared}
  Build docs       : ${enable_doc}
  Build examples   : ${enable_examples}
  Build tools      : ${enable_tools}

  [ Test-suite settings ]
  Test with libcurl: ${MSG_CURL}
  Slow tests       : ${enable_slow_tests}
  Heavy tests      : ${use_heavy_tests_MSG}
  Fuzzing tests    : ${run_zzuf_tests_MSG=no}
]])

AC_MSG_NOTICE([[
  [ Licence due to TLS backends used ]
  Library licence  : ${licence_descr}
]])

AS_IF([test "x$enable_basic_auth" != "xyes" || \
   test "x$enable_digest_auth" != "xyes" || \
   test "x${enable_md5}" = "xno" || \
   test "x${enable_sha256}" = "xno" || \
   test "x${enable_sha512_256}" = "xno" || \
   test "x${USE_THREADS}" = "xnone" || \
   test "x$enable_httpupgrade" != "xyes" || \
   test "x$enable_cookie" != "xyes" || \
   test "x$enable_httpupgrade" != "xyes" || \
   test "x$enable_postparser" != "xyes"],
   [AC_MSG_WARN([This will be a custom build with missing symbols. Do NOT use this build in a distribution. Building with these kinds of configure options is only for custom builds for embedded systems.])]
)
AS_IF([test -n "${CONF_FINAL_WARNS}"],[AC_MSG_WARN([${CONF_FINAL_WARNS}])])
