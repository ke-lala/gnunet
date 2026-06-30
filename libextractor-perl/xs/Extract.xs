#include "pextract.h"

MODULE = File::Extract	PACKAGE = File::Extract	PREFIX = EXTRACTOR_

void
init(extractor)
		SV* extractor
	PREINIT:
		SV* list = NULL;
	CODE:
		list = newSvEXTRACTOR_ExtractorList_ornull((EXTRACTOR_ExtractorList*)NULL);
		pextract_hv_store_extractor_list(extractor, list);

void
EXTRACTOR_loadDefaultLibraries(extractor)
		SV* extractor
	PREINIT:
		SV* list = NULL;
		EXTRACTOR_ExtractorList* foo = NULL;
	CODE:
		foo = EXTRACTOR_loadDefaultLibraries();
		list = newSvEXTRACTOR_ExtractorList_ornull(foo);
		pextract_hv_store_extractor_list(extractor, list);

void
EXTRACTOR_loadConfigLibraries(extractor, config)
		SV* extractor
		const char* config
	PREINIT:
		EXTRACTOR_ExtractorList* list = NULL;
		SV* result = NULL;
	CODE:
		list = SvEXTRACTOR_ExtractorList_ornull(pextract_get_extractor_list(extractor));
		result = newSvEXTRACTOR_ExtractorList_ornull(EXTRACTOR_loadConfigLibraries(list, config));
		pextract_hv_store_extractor_list(extractor, result);

void
EXTRACTOR_addLibrary(extractor, library)
		SV* extractor
		const char* library
	PREINIT:
		EXTRACTOR_ExtractorList* list = NULL;
		SV* result = NULL;
	CODE:
		list = SvEXTRACTOR_ExtractorList_ornull(pextract_get_extractor_list(extractor));
		result = newSvEXTRACTOR_ExtractorList_ornull(EXTRACTOR_addLibrary(list, library));
		pextract_hv_store_extractor_list(extractor, result);

void
EXTRACTOR_addLibraryLast(extractor, library)
		SV* extractor
		const char* library
	PREINIT:
		EXTRACTOR_ExtractorList* list = NULL;
		SV* result = NULL;
	CODE:
		list = SvEXTRACTOR_ExtractorList_ornull(pextract_get_extractor_list(extractor));
		result = newSvEXTRACTOR_ExtractorList_ornull(EXTRACTOR_addLibraryLast(list, library));
		pextract_hv_store_extractor_list(extractor, result);

# TODO: debug!
# 
#  perl -MExtUtils::testlib -MData::Dumper -MFile::Extract -e'$e = File::Extract->new(); $e->loadConfigLibraries("libextractor_mime"); $e->loadConfigLibraries("libextractor_png"); print Dumper({$e->getKeywords("/home/rafl/map_50.823448_12.930446_10000_4000_3000.png")}); $e->removeLibrary("libextractor_png"); print Dumper({$e->getLibraries()})'
# 
# segfaults
#
# perl -MExtUtils::testlib -MData::Dumper -MFile::Extract -e'$e = File::Extract->new(); $e->loadDefaultLibraries(); $e->loadConfigLibraries("libextractor_mime"); $e->loadConfigLibraries("libextractor_png"); print Dumper({$e->getKeywords("/home/rafl/map_50.823448_12.930446_10000_4000_3000.png")}); $e->removeLibrary("libextractor_png"); print Dumper({$e->getLibraries()})'
#
# shows unwanted results. Looks like a memory corruption. Most probably caused by libextractor itself.
#

void
EXTRACTOR_removeLibrary(extractor, library)
		SV* extractor
		const char* library
	PREINIT:
		EXTRACTOR_ExtractorList* list = NULL;
		SV* result = NULL;
	CODE:
		list = SvEXTRACTOR_ExtractorList_ornull(pextract_get_extractor_list(extractor));
		result = newSvEXTRACTOR_ExtractorList_ornull(EXTRACTOR_removeLibrary(list, library));
		pextract_hv_store_extractor_list(extractor, result);

void
EXTRACTOR_removeAll(extractor)
		SV* extractor
	PREINIT:
		EXTRACTOR_ExtractorList* list = NULL;
	CODE:
		list = SvEXTRACTOR_ExtractorList_ornull(pextract_get_extractor_list(extractor));
		EXTRACTOR_removeAll(list);
		pextract_hv_store_extractor_list(extractor, &PL_sv_undef);

void
EXTRACTOR_getKeywords(extractor, filename)
		SV* extractor
		const char* filename
	PREINIT:
		EXTRACTOR_ExtractorList* list = NULL;
		EXTRACTOR_KeywordList* result = NULL;
		EXTRACTOR_KeywordList* i = NULL;
		char* keyword_type;
	PPCODE:
		list = SvEXTRACTOR_ExtractorList(pextract_get_extractor_list(extractor));
		result = EXTRACTOR_getKeywords(list, filename);
		for (i = result; i != NULL; i = i->next) {
			keyword_type = (char*)EXTRACTOR_getKeywordTypeAsString(i->keywordType);
			XPUSHs (sv_2mortal(newSVpv(keyword_type, strlen(keyword_type))));
			XPUSHs (sv_2mortal(newSVpv(i->keyword, strlen(i->keyword))));
		}
		EXTRACTOR_freeKeywords(result);

void
getKeywordsFromBuffer(extractor, buffer)
		SV* extractor
		SV* buffer
	PREINIT:
		EXTRACTOR_ExtractorList* list = NULL;
		EXTRACTOR_KeywordList* result = NULL;
		EXTRACTOR_KeywordList* i = NULL;
		char* keyword_type;
		size_t size;
		const void* data;
	PPCODE:
		list = SvEXTRACTOR_ExtractorList(pextract_get_extractor_list(extractor));
		data = SvPV(buffer, size);
		result = EXTRACTOR_getKeywords2(list, data, size);
		for (i = result; i != NULL; i = i->next) {
			keyword_type = (char*)EXTRACTOR_getKeywordTypeAsString(i->keywordType);
			XPUSHs (sv_2mortal(newSVpv(keyword_type, strlen(keyword_type))));
			XPUSHs (sv_2mortal(newSVpv(i->keyword, strlen(i->keyword))));
		}
		EXTRACTOR_freeKeywords(result);

void
getLibraries(extractor)
		SV* extractor
	PREINIT:
		EXTRACTOR_ExtractorList* list = NULL;
		EXTRACTOR_ExtractorList* i = NULL;
	PPCODE:
		list = SvEXTRACTOR_ExtractorList_ornull(pextract_get_extractor_list(extractor));
		i = list;
		while (i != NULL) {
			if (!i->libname)
				continue;

			XPUSHs(sv_2mortal(newSVpv(i->libname, strlen(i->libname))));

			if (i->options) {
				XPUSHs(sv_2mortal(newSVpv(i->options, strlen(i->options))));
			} else {
				XPUSHs(&PL_sv_undef);
			}

			i = i->next;
		}

#BOOT:
#	PEXTRACTOR_CALL_BOOT(boot_File__Extract__ExtractorList);
