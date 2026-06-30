# This Makefile.am is in the public domain
SUBDIRS  = .
noinst_JAVA = \
  Extractor.java \
  MetaData.java \
  Xtract.java

EXTRA_DIST = \
  Extractor.java \
  MetaData.java \
  Xtract.java

install-exec-local:
	mkdir -p $(prefix)/share/java
	cd ../../..; @JAR@ -cvf $(prefix)/share/java/libextractor.jar org/gnu/libextractor/*.class
