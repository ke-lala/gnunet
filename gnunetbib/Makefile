PYTHON=python3
VERSION=0.3-dev

all:
	@rm -rf html/
	$(PYTHON) writeHTML.py anonbib.cfg
	# @mkdir -p `pwd`/html
	# @mv `pwd`/full `pwd`/html/
	# @mv `pwd`/bib `pwd`/html/
	# @mv `pwd`/date.html `pwd`/html/
	# @ln -s `pwd`/html/date.html `pwd`/html/index.html
	# @mv `pwd`/topic.html `pwd`/html/
	# @mv `pwd`/author.html `pwd`/html/
	# @mv `pwd`/bibtex.html `pwd`/html/
	# @cp -r `pwd`/css `pwd`/html/css
	# @cp -r `pwd`/cache `pwd`/html/cache

clean:
	rm -f *~ */*~ *.pyc *.pyo

update:
	$(PYTHON) updateCache.py anonbib.cfg
	$(PYTHON) rank.py anonbib.cfg

suggest:
	$(PYTHON) rank.py suggest anonbib.cfg

test:
	$(PYTHON) tests.py

veryclean: clean
	rm -f author.html date.html topic.html bibtex.html tmp.bib

TEMPLATES=_template_.html _template_bibtex.html
CSS=css/main.css css/pubs.css
BIBTEX=gnunetbib.bib
SOURCE=BibTeX.py config.py metaphone.py reconcile.py updateCache.py \
	writeHTML.py rank.py tests.py
EXTRAS=TODO README Makefile ChangeLog anonbib.cfg gold.gif silver.gif \
	upb.gif ups.gif

DISTFILES=$(TEMPLATES) $(CSS) $(BIBTEX) $(SOURCE) $(EXTRAS)

dist: clean
	rm -rf anonbib-$(VERSION)
	mkdir anonbib-$(VERSION)
	tar cf - $(DISTFILES) | (cd anonbib-$(VERSION); tar xf -)
	mkdir anonbib-$(VERSION)/cache
	tar czf anonbib-$(VERSION).tar.gz anonbib-$(VERSION)
	rm -rf anonbib-$(VERSION)
