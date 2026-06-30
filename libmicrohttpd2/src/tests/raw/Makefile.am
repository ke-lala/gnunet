# This Makefile.am is in the public domain

AM_CPPFLAGS = \
  -I$(top_srcdir)/src/incl_priv \
  -I$(top_srcdir)/src/include \
  -I$(top_srcdir)/src/mhd2 \
  -DMHD_CPU_COUNT=$(CPU_COUNT) \
  $(CPPFLAGS_ac) $(LIBCURL_CPPFLAGS)

AM_CFLAGS = $(CFLAGS_ac) $(PTHREAD_CFLAGS)

AM_LDFLAGS = $(LDFLAGS_ac)

LIBS = @LIBS@ $(PTHREAD_LIBS)

AM_TESTS_ENVIRONMENT = $(TESTS_ENVIRONMENT_ac)

if USE_COVERAGE
  AM_CFLAGS += -fprofile-arcs -ftest-coverage
endif

LDADD = $(top_builddir)/src/mhd2/libmicrohttpd2.la

$(top_builddir)/src/mhd2/libmicrohttpd2.la: $(top_builddir)/src/mhd2/Makefile
	@echo ' cd $(top_builddir)/src/mhd2 && $(MAKE) $(AM_MAKEFLAGS) libmicrohttpd2.la'; \
	$(am__cd) $(top_builddir)/src/mhd2 && $(MAKE) $(AM_MAKEFLAGS) libmicrohttpd2.la

check_PROGRAMS = \
  test_incompatible \
  test_oom \
  test_raw

TESTS = $(check_PROGRAMS)
