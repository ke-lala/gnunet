# This Makefile.am is in the public domain
SUBDIRS = .

AM_CPPFLAGS = \
  -I$(top_srcdir)/src/incl_priv \
  -I$(top_srcdir)/src/include \
  -DMHD_REAL_CPU_COUNT=@MHD_REAL_CPU_COUNT@ \
  -DMHD_CPU_COUNT=$(CPU_COUNT) \
  $(CPPFLAGS_ac)

AM_CFLAGS = $(CFLAGS_ac)

AM_LDFLAGS = $(LDFLAGS_ac)

AM_TESTS_ENVIRONMENT = $(TESTS_ENVIRONMENT_ac)

if USE_COVERAGE
  AM_CFLAGS += -fprofile-arcs -ftest-coverage
endif

LDADD = $(top_builddir)/src/mhd2/libmicrohttpd2.la

$(top_builddir)/src/mhd2/libmicrohttpd2.la: $(top_builddir)/src/mhd2/Makefile
	@echo ' cd $(top_builddir)/src/mhd2 && $(MAKE) $(AM_MAKEFLAGS) libmicrohttpd2.la'; \
	$(am__cd) $(top_builddir)/src/mhd2 && $(MAKE) $(AM_MAKEFLAGS) libmicrohttpd2.la


# Tools
noinst_PROGRAMS = 

if MHD_SUPPORT_THREADS
noinst_PROGRAMS += \
    perf_replies
endif


perf_replies_SOURCES = \
    perf_replies.c mhdtl_str_to_uint.h \
    mhdtl_get_cpu_count.h mhdtl_get_cpu_count.c
