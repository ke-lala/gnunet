FILES=gnu_name_system_service_ports.h gnu_name_system_service_ports.rst
all: check $(FILES)

check:
	recfix --check registry.rec

distclean:
	rm -f *.tmp
clean:
	rm -f $(FILES) *.tmp

gnu_name_system_service_ports.h.tmp: registry.rec h.template
	../format.sh h.template < registry.rec > $@

gnu_name_system_service_ports.h: h.header gnu_name_system_service_ports.h.tmp h.footer
	cat h.header gnu_name_system_service_ports.h.tmp h.footer > $@

gnu_name_system_service_ports.rst.tmp: registry.rec rst.template
	../format.sh rst.template < registry.rec > $@

rst.header.tmp:
	cat README POLICY rst.header.template > $@

gnu_name_system_service_ports.rst: rst.header.tmp gnu_name_system_service_ports.rst.tmp rst.footer
	cat rst.header.tmp gnu_name_system_service_ports.rst.tmp rst.footer > $@


.PHONY: check clean distclean
