FILES=gnunet_dht_block_types.h gnunet_dht_block_types.rst
all: check $(FILES)

check:
	recfix --check registry.rec

distclean:
	rm -f *.tmp
clean:
	rm -f $(FILES) *.tmp

gnunet_dht_block_types.h.tmp: registry.rec h.template
	../format.sh h.template < registry.rec > $@

gnunet_dht_block_types.h: h.header gnunet_dht_block_types.h.tmp h.footer
	cat h.header gnunet_dht_block_types.h.tmp h.footer > $@

gnunet_dht_block_types.rst.tmp: registry.rec rst.template
	../format.sh rst.template < registry.rec > $@

rst.header.tmp:
	cat README POLICY rst.header.template > $@

gnunet_dht_block_types.rst: rst.header.tmp gnunet_dht_block_types.rst.tmp rst.footer
	cat rst.header.tmp gnunet_dht_block_types.rst.tmp rst.footer > $@


.PHONY: check clean distclean
