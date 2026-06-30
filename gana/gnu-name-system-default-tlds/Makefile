FILES=tlds.conf tlds.rst
all: check $(FILES)

check:
	recfix --check registry.rec

distclean:

	rm -f *.tmp
clean:
	rm -f $(FILES) *.tmp

tlds.conf.tmp: registry.rec tlds.template
	../format.sh tlds.template < registry.rec > $@

tlds.conf: tlds.header tlds.conf.tmp
	cat tlds.header tlds.conf.tmp > $@

tlds.rst.tmp: registry.rec rst.template
	../format.sh rst.template < registry.rec > $@

rst.header.tmp:
	cat README POLICY rst.header.template > $@

tlds.rst: rst.header.tmp tlds.rst.tmp
	cat rst.header.tmp tlds.rst.tmp > $@


.PHONY: check clean distclean
