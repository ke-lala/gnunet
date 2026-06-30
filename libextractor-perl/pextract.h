#ifndef __PEXTRACT_H__
#define __PEXTRACT_H__

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "extractor.h"

SV* pextract_new_object(void* object, const char* package);
void* pextract_get_object(SV* sv, const char* package);

HV* pextract_get_hv_from_file_extract_obj(SV* hvref);
SV* pextract_get_extractor_list(SV* extractor);

void pextract_hv_store_extractor_list_inc(SV* extractor, SV* list);
void pextract_hv_store_extractor_list_noinc(SV* extractor, SV* list);

#define pextract_hv_store_extractor_list pextract_hv_store_extractor_list_inc

void _pextractor_call_XS(pTHX_ void (*subaddr) (pTHX_ CV*), CV* cv, SV** mark);

#define PEXTRACTOR_CALL_BOOT(name) \
	{ \
		extern XS(name); \
		_pextractor_call_XS(aTHX_ name, cv, mark); \
	}

/*
#define newSvExtractor(extractor) pextract_new_object(extractor, "File::Extract::Extractor");
#define SvEXTRACTOR_Extractor(sv) (struct EXTRACTOR_Extractor*)pextract_get_object(sv, "File::Extract::Extractor");
*/

#define newSvEXTRACTOR_ExtractorList(val) pextract_new_object(val, "File::Extract::ExtractorList")
#define SvEXTRACTOR_ExtractorList(sv) (EXTRACTOR_ExtractorList*)pextract_get_object(sv, "File::Extract::ExtractorList")
typedef EXTRACTOR_ExtractorList EXTRACTOR_ExtractorList_ornull;
#define SvEXTRACTOR_ExtractorList_ornull(sv) (((sv) && SvOK(sv)) ? SvEXTRACTOR_ExtractorList(sv) : NULL)
#define newSvEXTRACTOR_ExtractorList_ornull(val) (((val) == NULL) ? &PL_sv_undef : newSvEXTRACTOR_ExtractorList(val))

#endif /* __PEXTRACT_H__ */
