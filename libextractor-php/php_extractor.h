/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2004 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.0 of the PHP license,       |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_0.txt.                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id: php_extractor.h,v 1.1 2004/12/23 06:20:22 manfred Exp $ */

#ifndef PHP_EXTRACTOR_H
#define PHP_EXTRACTOR_H

extern zend_module_entry extractor_module_entry;
#define phpext_extractor_ptr &extractor_module_entry

#ifdef PHP_WIN32
#define PHP_EXTRACTOR_API __declspec(dllexport)
#else
#define PHP_EXTRACTOR_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(extractor);
PHP_MSHUTDOWN_FUNCTION(extractor);
PHP_RINIT_FUNCTION(extractor);
PHP_RSHUTDOWN_FUNCTION(extractor);
PHP_MINFO_FUNCTION(extractor);

PHP_FUNCTION(extractor_getkeywords);

/* 
  	Declare any global variables you may need between the BEGIN
	and END macros here:     

ZEND_BEGIN_MODULE_GLOBALS(extractor)
	long  global_value;
	char *global_string;
ZEND_END_MODULE_GLOBALS(extractor)
*/

/* In every utility function you add that needs to use variables 
   in php_extractor_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as EXTRACTOR_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define EXTRACTOR_G(v) TSRMG(extractor_globals_id, zend_extractor_globals *, v)
#else
#define EXTRACTOR_G(v) (extractor_globals.v)
#endif

#endif	/* PHP_EXTRACTOR_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
