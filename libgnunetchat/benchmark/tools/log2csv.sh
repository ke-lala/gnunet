#!/bin/sh
LOG_FILE=$1
shift 1

TRANSPOSE=$(dirname $0)/transpose.sh

CSV_FILE="$LOG_FILE.csv"
CSV_T_FILE="$CSV_FILE.T"

echo "\"Iteration\" $(cat $LOG_FILE | grep mess | awk '{print NR}' | $TRANSPOSE)" > $CSV_T_FILE
echo "\"Messages\" $(cat $LOG_FILE | grep mess | awk '{print $1}' | $TRANSPOSE)" >> $CSV_T_FILE
echo "\"Receipients\" $(cat $LOG_FILE | grep mess | awk '{print $4}' | $TRANSPOSE)" >> $CSV_T_FILE
echo "\"Duration_(in_ms)\" $(cat $LOG_FILE | grep mess | awk '{print $10}' | rev | cut -c3- | rev | $TRANSPOSE)" >> $CSV_T_FILE
echo "\"Minimum_latency_(in_ms)\" $(cat $LOG_FILE | grep rtt | awk '{print $4}' | tr '/' ' ' | awk '{print $1}' | $TRANSPOSE)" >> $CSV_T_FILE
echo "\"Average_latency_(in_ms)\" $(cat $LOG_FILE | grep rtt | awk '{print $4}' | tr '/' ' ' | awk '{print $2}' | $TRANSPOSE)" >> $CSV_T_FILE
echo "\"Maximum_latency_(in_ms)\" $(cat $LOG_FILE | grep rtt | awk '{print $4}' | tr '/' ' ' | awk '{print $3}' | $TRANSPOSE)" >> $CSV_T_FILE
echo "\"Variance_of_latency_(in_ms)\" $(cat $LOG_FILE | grep rtt | awk '{print $4}' | tr '/' ' ' | awk '{print $4}' | $TRANSPOSE)" >> $CSV_T_FILE

cat $CSV_T_FILE | $TRANSPOSE | tr _ ' ' > $CSV_FILE
rm $CSV_T_FILE

