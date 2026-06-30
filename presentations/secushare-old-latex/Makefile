org = secushare

ORG_PATH = /usr/share/emacs/site-lisp/org-mode
INIT = (add-to-list 'load-path \"${ORG_PATH}\") (load-file \".init.el\")

all: pdf

pdf: FORCE
	for f in ${org}; do \
	  e="$$e (find-file \"$$f\") (org-mode) (org-export-as-pdf org-export-headline-levels)"; \
	done; \
	emacs -Q --batch --eval "(progn ${INIT} $$e)"


html: FORCE
	for f in ${org}; do \
	  e="$$e (find-file \"$$f\") (org-mode) (org-export-as-html-batch)"; \
	done; \
	emacs -Q --batch --eval "(progn ${INIT} $$e)"

latex: FORCE
	for f in ${org}; do \
	  e="$$e (find-file \"$$f\") (org-mode) (org-export-as-latex-batch)"; \
	done; \
	emacs -Q --batch --eval "(progn ${INIT} $$e)"

FORCE:
