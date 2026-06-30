#include "pextract.h"

void
 _pextractor_call_XS(pTHX_ void (*subaddr) (pTHX_ CV*), CV* cv, SV** mark) {
	 dSP;
	 PUSHMARK(mark);
	 (*subaddr)(aTHX_ cv);
	 PUTBACK;
 }

SV*
pextract_new_object(void* object, const char* package) {
	SV* obj;
	SV* sv;
	HV* stash;

	if (!object) {
		return &PL_sv_undef;
	}

	obj = (SV*)newHV();
	sv_magic(obj, 0, PERL_MAGIC_ext, (const char*)object, 0);
	sv = newRV_inc(obj);
	stash = gv_stashpv(package, 1);
	sv_bless(sv, stash);

	return sv;
}

void*
pextract_get_object(SV* sv, const char* package) {
	MAGIC* mg;

	if (!sv || !SvOK(sv) || !SvROK(sv) || !sv_isobject(sv) || !sv_isa(sv, package) || !(mg = mg_find(SvRV(sv), PERL_MAGIC_ext)))
		return NULL;
	return (void*)mg->mg_ptr;
}

HV*
pextract_get_hv_from_file_extract_obj(SV* hvref) {
	if (!hvref
			|| !SvOK(hvref)
			|| !SvROK(hvref)
			|| !(SvTYPE(SvRV(hvref)) == SVt_PVHV)
			|| !sv_isobject(hvref)
			|| !sv_isa(hvref, "File::Extract"))
		return NULL;
	return (HV*)SvRV((SV*)hvref);
}

SV*
pextract_get_extractor_list(SV* extractor) {
	HV* hv = pextract_get_hv_from_file_extract_obj(extractor);
	if (!hv)
		return NULL;
	return *hv_fetch(hv, "extractor_list", 14, 0);
}

void
pextract_hv_store_extractor_list_noinc(SV* extractor, SV* list) {
	HV* hv = pextract_get_hv_from_file_extract_obj(extractor);
	if (hv) {
		hv_store(hv, "extractor_list", 14, list, 0);
	} else {
		croak("bar");
	}
}

void
pextract_hv_store_extractor_list_inc(SV* extractor, SV* list) {
	HV* hv = pextract_get_hv_from_file_extract_obj(extractor);
	if (hv) {
		SvREFCNT_inc(list);
		hv_store(hv, "extractor_list", 14, list, 0);
	} else {
		croak("bar");
	}
}
