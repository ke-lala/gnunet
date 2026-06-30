# This Makefile.am is in the public domain
SUBDIRS = .

AM_CPPFLAGS = \
  -I$(top_srcdir)/src/include \
  -I$(top_srcdir) \
  $(GNUNET_CPPFLAGS)

# Set this variable if you are using GNUNET libraries for all programs and
# libraries. You don't then need to target-specific _LDFLAGS with GNUNET_LDFLAGS
# AM_LDFLAGS = \
#   $(GNUNET_LDFLAGS) \
#   $(WINFLAGS) \
#   -export-dynamic

lib_LTLIBRARIES = libgnunetext.la

pkgcfgdir= $(prefix)/share/gnunet/config.d/

libexecdir= $(prefix)/lib/gnunet/libexec/

libgnunetext_la_SOURCES = \
  ext_api.c
libgnunetext_la_LIBADD = \
  -lgnunetutil
libgnunetext_la_LDFLAGS = \
  $(GNUNET_LDFLAGS)  $(WINFLAGS) \
  -version-info 0:0:0


bin_PROGRAMS = gnunet-ext

libexec_PROGRAMS = gnunet-service-ext

check_PROGRAMS = \
 test_ext_api

TESTS = $(check_PROGRAMS)

gnunet_service_ext_SOURCES = \
  gnunet-service-ext.c
gnunet_service_ext_LDADD = \
  -lgnunetutil -lgnunetcore -lgnunetdht\
  $(INTLLIBS)
gnunet_service_ext_LDFLAGS = \
  $(GNUNET_LDFLAGS)  $(WINFLAGS) -export-dynamic

gnunet_ext_SOURCES = \
  gnunet-ext.c
gnunet_ext_LDADD = \
  -lgnunetutil \
  $(INTLLIBS)
gnunet_ext_LDFLAGS = \
 $(GNUNET_LDFLAGS) $(WINFLAGS) -export-dynamic



test_ext_api_SOURCES = \
 test_ext_api.c
test_ext_api_LDADD = \
  $(top_builddir)/src/ext/libgnunetext.la \
  -lgnunetutil
test_ext_api_LDFLAGS = \
 $(GNUNET_LDFLAGS)  $(WINFLAGS) -export-dynamic

pkgcfg_DATA = ext.conf
