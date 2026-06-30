FILES=gnu_name_system_record_flags.h gnu_name_system_record_flags.rst
all: check $(FILES)

check:
	recfix --check registry.rec

distclean:
	rm -f *.tmp
clean:
	rm -f $(FILES) *.tmp

gnu_name_system_record_flags.h.tmp: registry.rec h.template
	../format.sh h.template < registry.rec > $@

gnu_name_system_record_flags.h: h.header gnu_name_system_record_flags.h.tmp h.footer
	cat h.header gnu_name_system_record_flags.h.tmp h.footer > $@

gnu_name_system_record_flags.rst.tmp: registry.rec rst.template
	../format.sh rst.template < registry.rec > $@

rst.header.tmp:
	cat README POLICY rst.header.template > $@

gnu_name_system_record_flags.rst: rst.header.tmp gnu_name_system_record_flags.rst.tmp rst.footer
	cat rst.header.tmp gnu_name_system_record_flags.rst.tmp rst.footer > $@


.PHONY: check clean distclean
