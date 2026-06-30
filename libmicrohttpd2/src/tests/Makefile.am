# This Makefile.am is in the public domain

SUBDIRS = unit basic raw

if HAVE_POSIX_THREADS
if MHD_SUPPORT_UPGRADE
SUBDIRS += upgrade
endif
if RUN_LIBCURL_TESTS
if MHD_SOCKETS_KIND_POSIX
SUBDIRS += client_server
endif
endif
endif

noinst_HEADERS = mhdt_has_in_name.h

.NOTPARALLEL:
