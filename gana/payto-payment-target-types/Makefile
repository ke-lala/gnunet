FILES=payto_payment_target_types.rst
all: check $(FILES)


check:
	recfix --check registry.rec

distclean:
	rm -f *.tmp

clean:
	rm -f $(FILES) *.tmp

payto_payment_target_types.rst.tmp: registry.rec rst.template
	../format.sh rst.template < registry.rec > $@

payto_payment_target_types.rst: rst.header payto_payment_target_types.rst.tmp
	cat rst.header payto_payment_target_types.rst.tmp > $@

.PHONY: check
