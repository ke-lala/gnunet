FILES=gnunet_error_codes.h \
  gnunet_error_codes.c \
  gnunet_error_codes.ts \
  gnunet_error_codes.go \
	gnunet_error_codes.rst

all: check $(FILES)

check: registry.rec
	recfix --check registry.rec

distclean:
	rm -f *.tmp

clean:
	rm -f $(FILES) *.tmp

prep:
	cd ../http-status-codes; make ; cd -

# Because prep is phony, this rule always triggers. To avoid
# combined.tmp cascading into triggering the other rules even
# if nothing changed, we use 'diff' to make sure that 'recsel'
# actually changed the result.
combined.tmp: registry.rec prep
	recsel -t GnunetErrorCode -j HttpStatus -p Description,Name,Value,HttpStatus,HttpStatus.Value,HttpStatus.Identifier ../http-status-codes/registry.rec registry.rec > combined.xtmp
	if ! diff $@ combined.xtmp > /dev/null; then mv combined.xtmp $@; else rm combined.xtmp; fi

combined-escaped.tmp: combined.tmp
	sed 's/"/\\"/g' combined.tmp > $@

length.tmp: combined.tmp
	recsel -p "Count(Description):ECS_LENGTH" combined.tmp > $@

gnunet_error_codes.h.tmp: combined.tmp h.template
	../format.sh h.template < combined.tmp > $@

gnunet_error_codes.h: h.header gnunet_error_codes.h.tmp h.footer
	cat h.header gnunet_error_codes.h.tmp h.footer > $@

gnunet_error_codes.c.tmp: combined-escaped.tmp c.template
	../format.sh c.template < combined-escaped.tmp > $@

gnunet_error_codes-length.c.tmp: length.tmp c-length.template
	../format.sh c-length.template < length.tmp > $@

gnunet_error_codes.c: c.header gnunet_error_codes.c.tmp gnunet_error_codes-length.c.tmp c.footer
	cat c.header gnunet_error_codes.c.tmp gnunet_error_codes-length.c.tmp c.footer > $@

gnunet_error_codes.ts.tmp: combined.tmp ts.template
	../format.sh ts.template < combined.tmp > $@

gnunet_error_codes.ts: ts.header gnunet_error_codes.ts.tmp ts.footer
	cat ts.header gnunet_error_codes.ts.tmp ts.footer > $@

gnunet_error_codes.go.tmp: combined.tmp go.template
	../format.sh go.template < combined.tmp > $@

gnunet_error_codes.go: go.header gnunet_error_codes.go.tmp go.footer
	cat go.header gnunet_error_codes.go.tmp go.footer > $@

gnunet_error_codes.rst.tmp: combined.tmp rst.template
	../format.sh rst.template < combined.tmp > $@

rst.header.tmp:
	cat README POLICY rst.header.template > $@

gnunet_error_codes.rst: rst.header.tmp gnunet_error_codes.rst.tmp rst.footer
	cat rst.header.tmp gnunet_error_codes.rst.tmp rst.footer > $@

.PHONY: check clean distclean prep
