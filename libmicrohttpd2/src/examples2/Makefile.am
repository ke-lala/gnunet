# This Makefile.am is in the public domain
SUBDIRS  = .

AM_CPPFLAGS = \
  -I$(top_srcdir)/src/include \
  $(CPPFLAGS_ac)

AM_CFLAGS = $(CFLAGS_ac)

AM_LDFLAGS = $(LDFLAGS_ac)

MHD_CPU_COUNT_DEF = -DMHD_CPU_COUNT=$(CPU_COUNT)

AM_TESTS_ENVIRONMENT = $(TESTS_ENVIRONMENT_ac)

LDADD = $(top_builddir)/src/mhd2/libmicrohttpd2.la

if USE_COVERAGE
  AM_CFLAGS += --coverage
endif

$(top_builddir)/src/mhd2/libmicrohttpd2.la: $(top_builddir)/src/mhd2/Makefile
	@echo ' cd $(top_builddir)/src/mhd2 && $(MAKE) $(AM_MAKEFLAGS) libmicrohttpd2.la'; \
	$(am__cd) $(top_builddir)/src/mhd2 && $(MAKE) $(AM_MAKEFLAGS) libmicrohttpd2.la


# example programs
noinst_PROGRAMS = \
  minimal_example2

if HAVE_POSIX_THREADS
  noinst_PROGRAMS += demo
endif

if HAVE_JANSSON
  noinst_PROGRAMS += json_echo
endif

if MHD_SUPPORT_AUTH_BASIC
  noinst_PROGRAMS += minimal_auth_basic
endif

if MHD_SUPPORT_AUTH_DIGEST
  noinst_PROGRAMS += minimal_auth_digest
endif

demo_CFLAGS = $(AM_CFLAGS) $(PTHREAD_CFLAGS) 
demo_LDADD = $(LDADD) $(PTHREAD_LIBS)


json_echo_SOURCES = \
 json_echo.c
json_echo_LDADD = \
 $(top_builddir)/src/mhd2/libmicrohttpd2.la \
 -ljansson
