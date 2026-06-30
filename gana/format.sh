#!/bin/sh
##
# Usage: format.sh TEMPLATE [SELEXP]
#
# This runs recfmt w/ template file TEMPLATE, taking input from stdin,
# and writing output to stdout.
#
# Optional arg SELEXP is an expression passed to ‘recsel -e’.  If specified,
# stdin is first processed by recsel and its output is then piped to recfmt
# for formatting.  If recsel exits failurefully (e.g., given invalid SELEXP),
# no output is written and format.sh exits failurefully as well.
##
me=$(basename $0)

version='1.4'
# 1.4  -- create $TMPDIR if it does not exist
# 1.3  -- add support for optional arg SELEXP
# 1.2  -- add check for required arg TEMPLATE
# 1.1  -- add --help/--version support
# 1.0  -- initial release

if [ x"$1" = x--help ] ; then
    sed '/^##/,/^##/!d;/^##/d;s/^# //g;s/^#$//g' $0
    exit 0
fi

if [ x"$1" = x--version ] ; then
    echo $me '(gana)' $version
    exit 0
fi

if [ x"$1" = x ] ; then
    echo >&2 "$me: ERROR: missing arg TEMPLATE (try --help)"
    exit 1
fi

template="$1"

if [ x"$2" = x ] ; then : ; else
    selexp="$2"
fi

if [ "$selexp" ] ; then
    t=$(mktemp)
    trap "rm -f $t" EXIT
    recsel -e "$selexp" > $t &&
    recfmt -f "$template" < $t
else
    exec recfmt -f "$template"
fi

# format.sh ends here
