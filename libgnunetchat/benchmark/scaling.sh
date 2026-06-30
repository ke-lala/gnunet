#!/bin/sh
COUNT=$1
shift 1

$(dirname $0)/.setup.sh
PING=$(dirname $0)/../.build_benchmark/tools/messenger_ping

IDENTITY="gnunet-identity"

pong() {
  local INDEX=$1
  shift 1

  local PONGS=$((1 + $COUNT - $INDEX))
  local DELAY=$(($INDEX * 5))

  sleep $DELAY
  $PING -P -e "a$INDEX" -c $PONGS $@ > /dev/null
}

for INDEX in $(seq $COUNT); do
  $IDENTITY -C "a$INDEX"
done

$IDENTITY -C "b"

for INDEX in $(seq $COUNT); do
  pong $INDEX $@ &
done

$PING -e "b" -c $COUNT -d 1 -J $@ 

wait
