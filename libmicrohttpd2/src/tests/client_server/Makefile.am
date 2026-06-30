# This Makefile.am is in the public domain
EMPTY_ITEM =

AM_CPPFLAGS = \
  -I$(top_srcdir)/src/incl_priv \
  -I$(top_srcdir)/src/include \
  -I$(top_srcdir)/src/mhd2 \
  -DMHD_CPU_COUNT=$(CPU_COUNT) \
  $(CPPFLAGS_ac) $(LIBCURL_CPPFLAGS)

AM_CFLAGS = $(CFLAGS_ac) $(PTHREAD_CFLAGS)

AM_LDFLAGS = $(LDFLAGS_ac)

AM_TESTS_ENVIRONMENT = $(TESTS_ENVIRONMENT_ac)

if USE_COVERAGE
  AM_CFLAGS += -fprofile-arcs -ftest-coverage
endif

LIBADD = \
  $(top_builddir)/src/mhd2/libmicrohttpd2.la $(LIBCURL) $(PTHREAD_LIBS)
LDADD = libmhdt.la $(LIBADD)

$(top_builddir)/src/mhd2/libmicrohttpd2.la: $(top_builddir)/src/mhd2/Makefile
	@echo ' cd $(top_builddir)/src/mhd2 && $(MAKE) $(AM_MAKEFLAGS) libmicrohttpd2.la'; \
	$(am__cd) $(top_builddir)/src/mhd2 && $(MAKE) $(AM_MAKEFLAGS) libmicrohttpd2.la

check_PROGRAMS = \
  test_client_server \
  test_authentication \
  test_postparser

if MHD_SUPPORT_HTTPS
check_PROGRAMS += \
  test_tls \
  test_cert_tls
endif

if MHD_SUPPORT_HTTP2
check_PROGRAMS += \
  test_http2

if MHD_SUPPORT_HTTPS
check_PROGRAMS += \
  test_tls2
endif
endif

TESTS = $(check_PROGRAMS)

noinst_LTLIBRARIES = \
  libmhdt.la

libmhdt_la_SOURCES = \
  libtest.c libtest.h \
  libtest_convenience.c \
  libtest_convenience_client_request.c \
  libtest_convenience_server_reply.c

# TODO: fix out-of-tree 'make check'
EXTRA_DIST = \
  data/root-ca.crt \
  data/inter1-ca.crt \
  data/inter2-ca.crt \
  data/test-server.crt \
  data/test-server-key.pem \
  data/check_certs.sh \
  data/make_chain.sh \
  data/chain.crt \
  README \
  zzuf_test_runner.sh
