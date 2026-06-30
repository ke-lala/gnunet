FILES=gnu_name_system_protocols.h gnu_name_system_protocols.rst
all: check $(FILES)

check:
	recfix --check registry.rec

distclean:
	rm -f *.tmp
clean:
	rm -f $(FILES) *.tmp

gnu_name_system_protocols.h.tmp: registry.rec h.template
	../format.sh h.template < registry.rec > $@

gnu_name_system_protocols.h: h.header gnu_name_system_protocols.h.tmp h.footer
	cat h.header gnu_name_system_protocols.h.tmp h.footer > $@

gnu_name_system_protocols.rst.tmp: registry.rec rst.template
	../format.sh rst.template < registry.rec > $@

rst.header.tmp:
	cat README POLICY rst.header.template > $@

gnu_name_system_protocols.rst: rst.header.tmp gnu_name_system_protocols.rst.tmp rst.footer
	cat rst.header.tmp gnu_name_system_protocols.rst.tmp rst.footer > $@


.PHONY: check clean distclean
