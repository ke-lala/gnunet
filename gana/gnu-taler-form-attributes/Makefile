FILES=taler_form_attributes.ts \
  taler_form_attributes.json \
  taler_form_attributes.rst

all: check $(FILES)

check: registry.rec
	recfix --check registry.rec

distclean:
	rm -f *.tmp

clean:
	rm -f $(FILES) *.tmp

taler_form_attributes.ts.tmp: registry.rec ts.template ts.form-header ts.form-footer
	/usr/bin/truncate --size 0 $@
	../format.sh ts.template < registry.rec >> $@


taler_form_attributes.ts: ts.header taler_form_attributes.ts.tmp ts.footer
	cat $^ > $@


taler_form_attributes.json.tmp: registry.rec json.template json.form-header json.form-footer
	/usr/bin/truncate --size 0 $@
	../format.sh json.template < registry.rec >> $@


taler_form_attributes.json: json.header taler_form_attributes.json.tmp json.footer
	cat $^ > $@


taler_form_attributes.rst.tmp: registry.rec rst.template
	../format.sh rst.template < registry.rec > $@

taler_form_attributes.rst: rst.header taler_form_attributes.rst.tmp rst.footer
	cat $^ > $@

.PHONY: check clean distclean 
