# Minimal makefile for Sphinx documentation
#

REGISTRIES = dot-alt \
  gnu-name-system-default-tlds \
  gnu-name-system-record-flags \
  gnu-name-system-record-types \
  gnu-taler-db-events \
  gnu-taler-error-codes \
  gnu-taler-extensions \
  gnu-taler-form-attributes \
  gnu-taler-kyc-attributes \
  gnunet-dht-block-types \
  gnunet-error-codes \
  gnunet-protocols \
  gnunet-signatures \
  http-status-codes \
  payto-payment-target-types \
  gns-protocol-numbers \
  gns-service-port-numbers

# You can set these variables from the command line, and also
# from the environment for the first two.
SPHINXOPTS    ?=
SPHINXBUILD   ?= sphinx-build
SOURCEDIR     = .
BUILDDIR      = _build


all: rsts html

# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

.PHONY: all clean help texinfo info Makefile

rsts:
	for dir in $(REGISTRIES); do \
        $(MAKE) -C $$dir; \
  done

html:
	@$(SPHINXBUILD) -M html "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

texinfo:
	@$(SPHINXBUILD) -M texinfo "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

info:
	@$(SPHINXBUILD) -M info "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

clean:
	rm -rf $(BUILDDIR)
	for dir in $(REGISTRIES); do \
        $(MAKE) -C $$dir; \
  done

distclean: clean

# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
#%: Makefile
#	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
