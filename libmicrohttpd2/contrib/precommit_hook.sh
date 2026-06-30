#!/bin/sh

# use as .git/hooks/pre-commit

# Note: keep this script portable!

# LICENSE
#
#   Copyright (c) 2025 Karlson2k (Evgeny Grin) <k2k@drgrin.dev>
#   Copyright (C) 2019,2024 Christian Grothoff
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

# Enable 'pipefail' if supported -- this breaks git commit if -o piefail is not supported!
# set -o pipefail 2>/dev/null || :

# Redirect all output to stderr
exec 1>&2

# Note: use full set of symbols in regexp instead of ranges to as ranges
# can be exanded differently depending on LC_* and LANG variables.
whtlst_chrsF="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._"
whtlst_chrsO="${whtlst_chrsF}~-" # symbol '-' must be the last!
whtlst_fname="[${whtlst_chrsF}][${whtlst_chrsO}]*"
BAD_NAMES=`git diff --cached --name-only --no-renames --diff-filter=ACR \
  | ${SED-sed} "/^${whtlst_fname}\(\/${whtlst_fname}\)*\$/d; \
                /^po\/en@${whtlst_fname}\$/d"` || exit 5
if test -n "$BAD_NAMES"; then
  echo "The following file(s) have illigal names, rename them before commiting:
$BAD_NAMES"
  exit 2
fi

if command -v "command" >/dev/null 2>&1; then
have_cmd()
{
  command -v "$1" >/dev/null 2>&1
}
elif type "type" >/dev/null 2>&1; then
have_cmd()
{
  type "$1" >/dev/null 2>&1
}
else
have_cmd()
{
  : # dummy fallback
}
fi

: "${UNCRUSTIFY=uncrustify}"
UNCRUSTIFY_CFG="uncrustify.cfg"

if have_cmd ${UNCRUSTIFY}; then :; else
  echo "Uncrustify not detected, code formating cannot be checked."
  exit 6
fi

STAGED_FILES=`git diff --cached --name-only --no-renames --diff-filter=ACM | ${SED-sed} -n '/\.[ch]$/p'` || exit 5
BROKEN_FILES=""

GIT_NOGLOB_PATHSPECS="1"
precomm_save_IFS="$IFS"
IFS="
"
for f in ${STAGED_FILES}; do
  IFS="$precomm_save_IFS"
  if git cat-file blob -- ":0:$f" | "${UNCRUSTIFY}" -q -c "$UNCRUSTIFY_CFG" --check --assume "$f" >/dev/null 2>&1; then :; else
    BROKEN_FILES="$BROKEN_FILES '$f'"
  fi
done
IFS="$precomm_save_IFS"

if test -n "${BROKEN_FILES}"; then
  echo "Run"
  echo "  ${UNCRUSTIFY} -c '$UNCRUSTIFY_CFG' --replace --no-backup${BROKEN_FILES}"
  echo "in '`pwd`' directory and then stage modified files before committing."
  exit 3
fi

exit 0
