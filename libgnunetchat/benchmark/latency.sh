#!/bin/sh
COUNT=$1
ITERATIONS=$2
shift 2

$(dirname $0)/.setup.sh
PING=$(dirname $0)/../.build_benchmark/tools/messenger_ping

for INDEX in $(seq $COUNT); do
  $PING -P -c $ITERATIONS $@ > /dev/null &
  sleep 0.2
done

$PING -c $ITERATIONS $@
