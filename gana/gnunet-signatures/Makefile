FILES=gnunet_signatures.h gnunet_signatures.rst taler_signatures.go taler_signatures.ts
all: check $(FILES)
check:
	recfix --check registry.rec
distclean:
	rm -f *.tmp
clean:
	rm -f $(FILES) *.tmp
gnunet_signatures.h.tmp: registry.rec h.template
	../format.sh h.template 'Package = "GNUnet"' < registry.rec > $@

gnunet_signatures.h: h.header gnunet_signatures.h.tmp h.footer
	cat h.header gnunet_signatures.h.tmp h.footer > $@

gnunet_signatures.rst.tmp: registry.rec rst.template
	../format.sh rst.template < registry.rec > $@

rst.header.tmp:
	cat README POLICY rst.header.template > $@

gnunet_signatures.rst: rst.header.tmp gnunet_signatures.rst.tmp rst.footer
	cat rst.header.tmp gnunet_signatures.rst.tmp rst.footer > $@

taler_signatures.go.tmp: registry.rec go-taler.template
	../format.sh go-taler.template 'Package = "GNU Taler"' < registry.rec > $@

taler_signatures.go: go.header taler_signatures.go.tmp go.footer
	cat $^ > taler_signatures.go.oct.tmp
	sed -E 's/= 00*([0-9])/= \1/g;s/([A-Z])([A-Z]*)/\1\L\2/g;s/_//g' taler_signatures.go.oct.tmp > $@

taler_signatures.ts.tmp: registry.rec ts-taler.template
	../format.sh ts-taler.template 'Package = "GNU Taler"' < registry.rec > $@ $@

taler_signatures.ts: ts.header taler_signatures.ts.tmp ts.footer
	cat $^ | sed -e "s/= 0000/= 0/" -e "s/= 000/= /" -e "s/= 00/= /" > $@


.PHONY: check clean distclean
