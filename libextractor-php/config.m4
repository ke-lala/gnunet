dnl $Id: config.m4,v 1.1 2004/12/23 06:20:22 manfred Exp $
dnl config.m4 for extension extractor

PHP_ARG_WITH(extractor, for extractor support,
[  --with-extractor             Include extractor support])

if test "$PHP_EXTRACTOR" != "no"; then 
   
   SEARCH_PATH="/usr/local /usr"     # you might want to change this
   SEARCH_FOR="/include/extractor.h"  # you most likely want to change this
   if test -r $PHP_EXTRACTOR/$SEARCH_FOR; then # path given as parameter
     EXTRACTOR_DIR=$PHP_EXTRACTOR
   else # search default path list
     AC_MSG_CHECKING([for extractor files in default path])
     for i in $SEARCH_PATH ; do
       if test -r $i/$SEARCH_FOR; then
         EXTRACTOR_DIR=$i
         AC_MSG_RESULT(found in $i)
       fi
     done
   fi

   if test -z "$EXTRACTOR_DIR"; then
     AC_MSG_RESULT([not found])
     AC_MSG_ERROR([Please reinstall the extractor distribution])
   fi

   PHP_ADD_INCLUDE($EXTRACTOR_DIR/include)

   PHP_ADD_LIBRARY_WITH_PATH(extractor, $PHP_EXTRACTOR_DIR/lib, EXTRACTOR_SHARED_LIBADD)

   AC_CHECK_LIB(extractor, EXTRACTOR_loadDefaultLibraries, 
   [
      AC_DEFINE(HAVE_EXTRACTORLIB,1,[ ])
   ], [
      AC_MSG_ERROR(extractor library not found or wrong version)
   ],)

  PHP_SUBST(EXTRACTOR_SHARED_LIBADD)

  PHP_NEW_EXTENSION(extractor, extractor.c, $ext_shared)
fi
