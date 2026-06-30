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
  | Author: Manfred Weber                                                |
  +----------------------------------------------------------------------+
*/

/* $Id: extractor.c,v 1.1 2004/12/23 06:20:22 manfred Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_extractor.h"
#include <extractor.h>

/* If you declare any globals in php_extractor.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(extractor)
*/

/* True global resources - no need for thread safety here */
static int le_extractor;

/* {{{ extractor_functions[]
 *
 * Every user visible function must have an entry in extractor_functions[].
 */
function_entry extractor_functions[] = {
	PHP_FE(extractor_getkeywords,	NULL)
	{NULL, NULL, NULL}	/* Must be the last line in extractor_functions[] */
};
/* }}} */

/* {{{ extractor_module_entry
 */
zend_module_entry extractor_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"extractor",
	extractor_functions,
	PHP_MINIT(extractor),
	PHP_MSHUTDOWN(extractor),
	PHP_RINIT(extractor),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(extractor),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(extractor),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_EXTRACTOR
ZEND_GET_MODULE(extractor)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("extractor.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_extractor_globals, extractor_globals)
    STD_PHP_INI_ENTRY("extractor.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_extractor_globals, extractor_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_extractor_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_extractor_init_globals(zend_extractor_globals *extractor_globals)
{
	extractor_globals->global_value = 0;
	extractor_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(extractor)
{
	/* If you have INI entries, uncomment these lines 
	ZEND_INIT_MODULE_GLOBALS(extractor, php_extractor_init_globals, NULL);
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(extractor)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(extractor)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(extractor)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(extractor)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "extractor support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/

/* {{{ proto array extractor_getkeywords(string filename)
   returns keywords */
PHP_FUNCTION(extractor_getkeywords)
{
        char *filename = NULL;
        int argc = ZEND_NUM_ARGS();
        int filename_len;
        EXTRACTOR_KeywordList *keywords;
        EXTRACTOR_ExtractorList *extractors;

        if (zend_parse_parameters(argc TSRMLS_CC, "s", &filename, &filename_len) == FAILURE)
                return;

        extractors = EXTRACTOR_loadDefaultLibraries ();
        keywords = EXTRACTOR_getKeywords (extractors, filename);
        array_init(return_value);
        while (keywords != NULL)
        {
                add_next_index_string(return_value,keywords->keyword,1);
                keywords = keywords->next;
        }
        EXTRACTOR_freeKeywords (keywords);
        EXTRACTOR_removeAll (extractors);
        return;
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
