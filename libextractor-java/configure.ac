# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.57)
AC_INIT([libextractor-java], [1.0.0], [bug-libextractor@gnu.org])
AC_REVISION($Revision: 1.67 $)
AM_INIT_AUTOMAKE([libextractor-java], [1.0.0])
AM_CONFIG_HEADER(native/config.h)

AH_TOP([#define _GNU_SOURCE  1])

# Checks for programs.
AC_CANONICAL_HOST

# The problem that I'm trying to solve here by specifying gcc-3.3 and gcc2
# in the search list ahead of the other stuff is that gcc-3.1 on Mac OS X
# does not work for some of the pdf extractor code.  Now, it would seem
# that we could have a conditional on $host_os that puts just the right
# amount of crazyness into the checks only on the Mac.  Unfortunately, this
# will not work, because (according to some mailing lists and painful
# personal experiences) AC_PROG_CXX does not work inside of conditionals.
# So, instead, we do this crazyness globally.  The list of compilers that
# you see below is taken right out of the autoconf (version 2.57) sources,
# so it should not impede our portability.
AC_PROG_CC(gcc-3.3 gcc2 gcc cc /usr/ucb/cc cl)
AC_PROG_CPP
AC_PROG_CXX(g++-3.3 g++2 g++ c++ gpp aCC CC cxx cc++ cl FCC KCC RCC xlC_r xlC)

AC_CHECK_CLASSPATH
AC_PROG_JAVAC
AC_PROG_JAVA
AC_PROG_JAR
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

case "$host_os" in
linux*)
     AC_DEFINE(LINUX,1,[This is a Linux system])
     ;;
freebsd*)
     AC_DEFINE_UNQUOTED(SOMEBSD,1,[This is a BSD system])
     ;;
openbsd*)
     AC_DEFINE_UNQUOTED(SOMEBSD,1,[This is a BSD system])
     ;;
netbsd*)
     AC_DEFINE_UNQUOTED(SOMEBSD,1,[This is a BSD system])
     ;;
*solaris*)
     AC_DEFINE_UNQUOTED(SOLARIS,1,[This is a Solaris system])
     XTRA_CPPLIBS=-lstdc++
     ;;
darwin*)
     AC_DEFINE_UNQUOTED(DARWIN,1,[This is a Darwin system])
     ;;
cygwin*)
     AC_DEFINE_UNQUOTED(CYGWIN,1,[This is a CYGWIN system])
     LDFLAGS="$LDFLAGS -no-undefined"
     ;;
mingw*)
     AC_DEFINE_UNQUOTED(MINGW,1,[This is a MinGW system])
     AC_DEFINE_UNQUOTED(WINDOWS,1,[This is a Windows system])
     LDFLAGS="$LDFLAGS -no-undefined -Wl,--export-all-symbols -lws2_32 -lplibc"
     ;;
*)
     AC_MSG_RESULT(Unrecognised OS $host_os)
     AC_DEFINE_UNQUOTED(OTHEROS,1,[Some strange OS])
     AC_MSG_RESULT(otheros)
     ;;
esac

# use '-fno-strict-aliasing', but only if the compiler can take it
if gcc -fno-strict-aliasing -S -o /dev/null -xc /dev/null >/dev/null 2>&1; 
then
 CFLAGS="-fno-strict-aliasing $CFLAGS"
fi

# Special check for broken Operating Systems (OS X)
AC_CACHE_CHECK(whether ${CC-cc} accepts -no-cpp-precomp,
cv_prog_cc_darwin_cpp_precomp,
[echo 'void f(){}' > conftest.c
if test -z "`${CC-cc} -no-cpp-precomp -c conftest.c 2>&1`"; then
  cv_prog_cc_darwin_cpp_precomp=yes
else
  cv_prog_cc_darwin_cpp_precomp=no
fi
rm -f conftest*
])
if test $cv_prog_cc_darwin_cpp_precomp = yes; then
  CFLAGS="$CFLAGS -no-cpp-precomp"
fi

AC_HEADER_STDC

# check for GNU LD
AC_LIB_PROG_LD_GNU

AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)

# test for libextractor
extractor=0
AC_MSG_CHECKING(for libextractor)
AC_ARG_WITH(extractor,
   [  --with-extractor=PFX    Base of libextractor installation],
   [AC_MSG_RESULT([$with_extractor])
    case $with_extractor in
      no)
        ;;
      yes)
        AC_CHECK_HEADERS(extractor.h,
          AC_CHECK_LIB([extractor], [EXTRACTOR_plugin_add_defaults],
            extractor=1))
        ;;
      *)
        LDFLAGS="-L$with_extractor/lib $LDFLAGS"
        CPPFLAGS="-I$with_extractor/include $CPPFLAGS"
        AC_CHECK_HEADERS(extractor.h,
          AC_CHECK_LIB([extractor], [EXTRACTOR_plugin_add_defaults],
            EXT_LIB_PATH="-L$with_extractor/lib $EXT_LIB_PATH"
            extractor=1))
        ;;
    esac
   ],
   [AC_MSG_RESULT([--with-extractor not specified])
    AC_CHECK_HEADERS(extractor.h,
     AC_CHECK_LIB([extractor], [EXTRACTOR_plugin_add_defaults],
      extractor=1))])
if test "$extractor" != 1
then
 AC_MSG_ERROR([libextractor-java requires libextractor])
fi


AC_CONFIG_FILES([
Makefile
native/Makefile
org/Makefile
org/gnu/Makefile
org/gnu/libextractor/Makefile
])

AC_OUTPUT
