#!/bin/sh

set -eu

ZZSTARTSEED=0
ZZSTOPSEED=100
ret=0
# fallbacks for direct, non-"make check" usage
if test x"${testdatadir:-NONE}" = xNONE""
then
  testdatadir=../../test
fi
if test x"${bindir:-NONE}" = xNONE""
then
  bindir=`grep "^prefix = " ./Makefile | cut -d ' ' -f 3`
  bindir="$bindir/bin"
fi

if test ! -x `which zzuf`
then
    echo "zzuf not available, not running the test"
    exit 77
fi

if test -x `which timeout`
then
    TIMEOUT="timeout 15"
else
    echo "timeout command not found, will not auto-timeout (may cause hang)"
    TIMEOUT=""
fi

for file in $testdatadir/test*
do
  if test -f "$file"
  then
    tmpfile=`mktemp extractortmp.XXXXXX` || exit 1
    seed=$ZZSTARTSEED
    trap "echo $tmpfile caused SIGSEGV ; exit 1" SEGV
    while [ $seed -lt $ZZSTOPSEED ]
    do
      echo "file $file seed $seed"
      zzuf -c -s $seed cat "$file" > "$tmpfile"
      if ! $TIMEOUT $bindir/extract -i "$tmpfile" > /dev/null
      then
        echo "$tmpfile with seed $seed failed"
  	    mv $tmpfile $tmpfile.keep
	    ret=1
      fi
      seed=`expr $seed + 1`
    done
    rm -f "$tmpfile"
  fi
done

exit $ret
