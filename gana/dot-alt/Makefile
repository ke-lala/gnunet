FILES=dot_alt.rst
all: check $(FILES)

check:
	recfix --check registry.rec

distclean:

	rm -f *.tmp
clean:
	rm -f $(FILES) *.tmp

dot_alt.rst.tmp: registry.rec rst.template
	../format.sh rst.template < registry.rec > $@

rst.header.tmp:
	cat README POLICY rst.header.template > $@

dot_alt.rst: rst.header.tmp dot_alt.rst.tmp
	cat rst.header.tmp dot_alt.rst.tmp > $@

.PHONY: check
