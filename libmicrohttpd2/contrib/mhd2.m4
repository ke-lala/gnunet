# mhd2.m4

#  This file is part of GNU libmicrohttpd
#  Copyright (C) 2025 Taler Systems SA
#
#  GNU libmicrohttpd is free software; you can redistribute it and/or modify it under the
#  terms of the GNU Lesser General Public License as published by the Free Software
#  Foundation; either version 3, or (at your option) any later version.
#
#  GNU libmicrohttpd is distributed in the hope that it will be useful, but WITHOUT ANY
#  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
#  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License along with
#  GNU libmicrohttpd; see the file COPYING.  If not, If not, see <http://www.gnu.org/license>

# serial 1

dnl MHD2_VERSION_AT_LEAST([VERSION])
dnl
dnl Check that microhttpd2.h can be used to build a program that prints out
dnl the MHD_VERSION tuple in X.Y.Z format, and that X.Y.Z is greater or equal
dnl to VERSION.  If not, display message and cause the configure script to
dnl exit failurefully.
dnl
dnl This uses AX_COMPARE_VERSION to do the job.
dnl It sets shell var mhd_cv_version, as well.
dnl
AC_DEFUN([MHD2_VERSION_AT_LEAST],
 [AC_CACHE_CHECK([libmicrohttpd2 version],[mhd2_cv_version],
 [AC_LINK_IFELSE([AC_LANG_PROGRAM([[
  #include <stdio.h>
  #include <microhttpd2.h>
]],[[
  int v = MHD_VERSION;
  printf ("%x.%x.%x\n",
          (v >> 24) & 0xff,
          (v >> 16) & 0xff,
          (v >>  8) & 0xff);
]])],
  [mhd2_cv_version=$(./conftest)],
  [mhd2_cv_version=0])])
AX_COMPARE_VERSION([$mhd2_cv_version],[ge],[$1],
  [libmhd2=1],
  [libmhd2=0])
AM_CONDITIONAL([HAVE_MHD2], [test "x$libmhd2" = "x1"])
AC_DEFINE_UNQUOTED([HAVE_MHD2], [$libmhd2],
                   [Defined to 1 if libmicrohttpd2 is available])
])
# mhd2.m4 ends here
