FILES=taler_dbevents.h taler_dbevents.rst

all: check $(FILES)

check: registry.rec
	recfix --check registry.rec

distclean:
	rm -f *.tmp

clean:
	rm -f $(FILES) *.tmp

taler_dbevents.h.tmp: registry.rec h.template
	../format.sh h.template < registry.rec > $@

taler_dbevents.h: h.header taler_dbevents.h.tmp h.footer
	cat $^ > $@

taler_dbevents.rst.tmp: registry.rec rst.template
	../format.sh rst.template < registry.rec > $@

taler_dbevents.rst: rst.header taler_dbevents.rst.tmp rst.footer
	cat rst.header taler_dbevents.rst.tmp rst.footer > $@

.PHONY: check clean distclean
