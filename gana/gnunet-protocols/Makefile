FILES=gnunet_overlay_protocols.h gnunet_overlay_protocols.rst
all: check $(FILES)

check:
	recfix --check registry.rec

distclean:
	rm -f *.tmp
clean:
	rm -f $(FILES) *.tmp

gnunet_overlay_protocols.h.tmp: registry.rec h.template
	../format.sh h.template < registry.rec > $@

gnunet_overlay_protocols.h: h.header gnunet_overlay_protocols.h.tmp h.footer
	cat h.header gnunet_overlay_protocols.h.tmp h.footer > $@

gnunet_overlay_protocols.rst.tmp: registry.rec rst.template
	../format.sh rst.template < registry.rec > $@

rst.header.tmp:
	cat rst.header.template README POLICY > $@
	echo "Entries" >> $@
	echo "^^^^^^^" >> $@

gnunet_overlay_protocols.rst: rst.header.tmp gnunet_overlay_protocols.rst.tmp rst.footer
	cat rst.header.tmp gnunet_overlay_protocols.rst.tmp rst.footer > $@


.PHONY: check clean distclean
