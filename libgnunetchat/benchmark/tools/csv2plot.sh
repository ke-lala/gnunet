#!/bin/sh
OUTPUT=$1
X_AXIS=$2
Y_AXIS=$3
shift 3

HEADER=$(dirname $0)/header.sh

if [ $# -lt 1 ]; then
  exit
fi

X_LABEL=$($HEADER $X_AXIS $1)
Y_LABEL=$($HEADER $Y_AXIS $1)

X_MIN=$(cat $1 | awk 'NR != 1 {print $'$X_AXIS'}' | sort -h | head -n1)
X_MAX=$(cat $1 | awk 'NR != 1 {print $'$X_AXIS'}' | sort -h | tail -n1)

PLOT=""
while [ $# -gt 0 ]; do
  if [ "$PLOT" != "" ]; then
    PLOT="$PLOT, "
  fi
  PLOT="$PLOT'$1' using $X_AXIS:$Y_AXIS title '$2' with boxes lw 1.25"
  shift 2
done

gnuplot -e "set terminal pdf; set output '$OUTPUT'; set datafile separator ' '; set xlabel '$X_LABEL' font ',12'; set ylabel '$Y_LABEL' font ',12'; set boxwidth 0.8; set xrange [$(($X_MIN - 1)):$(($X_MAX + 1))]; plot $PLOT"

