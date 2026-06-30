FILES=taler_account_properties.ts \
  taler_account_properties.rst

all: check $(FILES)

check: registry.rec
	recfix --check registry.rec

distclean:
	rm -f *.tmp

clean:
	rm -f $(FILES) *.tmp

taler_account_properties.ts.tmp: registry.rec ts.template ts.form-header ts.form-footer
	/usr/bin/truncate --size 0 $@
	../format.sh ts.template < registry.rec >> $@


taler_account_properties.ts: ts.header taler_account_properties.ts.tmp ts.footer
	cat $^ > $@


taler_account_properties.json.tmp: registry.rec json.template json.form-header json.form-footer
	/usr/bin/truncate --size 0 $@
	../format.sh json.template < registry.rec >> $@


taler_account_properties.json: json.header taler_account_properties.json.tmp json.footer
	cat $^ > $@


taler_account_properties.rst.tmp: registry.rec rst.template
	../format.sh rst.template < registry.rec > $@

taler_account_properties.rst: rst.header taler_account_properties.rst.tmp rst.footer
	cat $^ > $@

.PHONY: check clean distclean 
