#!/bin/sh
$(dirname $0)/.setup.sh

IDENTITY="gnunet-identity"
MESSENGER="gnunet-messenger"

$IDENTITY -C alice
$IDENTITY -C bob
$IDENTITY -C charlie

echo "A" | $MESSENGER -e alice $@ > /dev/null
echo "B1" | $MESSENGER -e bob $@ > /dev/null
echo "SOMETHING" | $MESSENGER -e alice $@ > /dev/null

(sleep 0.1; echo "B2" | $MESSENGER -e bob -R $@ > /dev/null) &
(echo "" | $MESSENGER -e charlie $@) &
(sleep 3.0; killall $MESSENGER)
