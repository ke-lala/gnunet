# This Makefile.am is in the public domain
SUBDIRS  = m4 po src doc .
EXTRA_DIST = config.rpath \
  ABOUT-NLS

pkgconfigdatadir = $(libdir)/pkgconfig
pkgconfigdata_DATA = libextractor.pc

DISTCLEANFILES = \
  libextractor.pc \
  libextractor.pc.in \
  libextractor-uninstalled.sh \
  libextractor-uninstalled.pc

ACLOCAL_AMFLAGS = -I m4
