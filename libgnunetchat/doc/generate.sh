#!/bin/sh
cd "${MESON_SOURCE_ROOT}"

VERSION="$(sh 'contrib/get_version.sh')"
sed -i 's/PROJECT_NUMBER\(\s\+=\s\+\)\([0-9\.a-z\-]\+\)/PROJECT_NUMBER\1'"${VERSION}/" 'Doxyfile'
doxygen 'Doxyfile'

if hash pdflatex 2>/dev/null; then
	cd doc/latex
	make
fi
