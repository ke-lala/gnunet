# This Makefile.am is in the public domain
SUBDIRS  = .

lib_LTLIBRARIES = \
  libextractor_java.la 

libextractor_java_la_LDFLAGS = \
  -export-dynamic -version-info 0:0:0 
libextractor_java_la_LIBADD = \
  -lextractor

libextractor_java_la_SOURCES = \
  extractor.c jni.h \
  org_gnu_libextractor_Extractor.h
