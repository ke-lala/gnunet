# This Makefile.am is in the public domain

examples_DIR = examples2

SUBDIRS = incl_priv include mhd2

if BUILD_TOOLS
SUBDIRS += tools
endif

if BUILD_EXAMPLES
SUBDIRS += $(examples_DIR)
endif

SUBDIRS += tests .

.NOTPARALLEL:
