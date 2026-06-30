# This Makefile.am is in the public domain
dist_man_MANS = libmicrohttpd2.3

SUBDIRS = .
#SUBDIRS += doxygen

if BUILD_EXAMPLES
#SUBDIRS += examples
endif

DISTCLEANFILES = \
  libmicrohttpd2.cps \
  libmicrohttpd2.fn \
  libmicrohttpd2.toc \
  libmicrohttpd2.tp \
  libmicrohttpd2.tps \
  libmicrohttpd2.aux \
  libmicrohttpd2.dvi

MAINTAINERCLEANFILES = \
  $(srcdir)/libmicrohttpd2.info \
  $(srcdir)/libmicrohttpd2.info-1 \
  $(srcdir)/libmicrohttpd2.info-2

info_TEXINFOS = \
  libmicrohttpd2.texi
libmicrohttpd_TEXINFOS = \
  manual/*.inc \
  examples/*.c \
  lgpl.texi \
  ecos.texi \
  gpl-2.0.texi \
  fdl-1.3.texi

EXTRA_DIST = \
  $(libmicrohttpd_TEXINFOS)

install-info-local:
	@echo " $(MKDIR_P) '$(DESTDIR)$(infodir)'"; \
	$(MKDIR_P) "$(DESTDIR)$(infodir)" || exit 1;

install-html-local:
	@if test -n "$(htmldir)"; then \
	  echo " $(MKDIR_P) '$(DESTDIR)$(htmldir)/libmicrohttpd2.html'"; \
	  $(MKDIR_P) "$(DESTDIR)$(htmldir)/libmicrohttpd2.html" || exit 1; \
	else : ; fi

update-stamp:
	@rm -f '$(srcdir)/stamp-vti' '$(srcdir)/version.texi' && \
	  $(MAKE) $(AM_MAKEFLAGS) '$(srcdir)/version.texi'

.PHONY: update-stamp
