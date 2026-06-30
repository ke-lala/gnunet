#!/bin/bash
# Tests simple migration
set -x

# remove zonebackup should it exist
rm dnszone_gnunet.gnu || true

echo "Starting knot..."
knotd -c basic.conf 2>/dev/null &


if [ ! "gnunet-arm -I -T1" ]; then
    echo "GNUnet not running! Start with gnunet-arm -s"
fi

if [ ! -x "$(command -v "ascension")" ]; then
    echo "Ascension not found! Please install it"
    exit 60
fi

# Run the migration of the gnunet.gnu zone
ascension -n 127.0.0.1 -P 5000 -s gnunet.gnu. -l30

# Test the records
status_sum=0
gnunet-gns -t SOA -u gnunet.gnu -r | grep "ns1"
(("status_sum+=$?"))
gnunet-gns -t A -u firefly.gnunet.gnu -r | grep "147.87.255.21" >/dev/null
(("status_sum+=$?"))
gnunet-gns -t AAAA -u firefly.gnunet.gnu -r | grep "2a07:6b47:100:464::9357:ffd" >/dev/null
(("status_sum+=$?"))
gnunet-gns -t A -u www.gnunet.gnu -r | grep "131.159.74.67" >/dev/null
(("status_sum+=$?"))
gnunet-gns -t A -u git.gnunet.gnu -r | grep "147.87.255.21" >/dev/null
(("status_sum+=$?"))
gnunet-gns -t AAAA -u www.gnunet.gnu -r | grep "2001:4ca0:2001:42:225:90ff:fe6b:d60" >/dev/null
(("status_sum+=$?"))
gnunet-gns -t A -u ns1.gnunet.gnu -r | grep "1.1.1.1" >/dev/null
(("status_sum+=$?"))
gnunet-gns -t A -u ns2.gnunet.gnu -r | grep "1.0.0.1" >/dev/null
(("status_sum+=$?"))
gnunet-gns -t A -u jabber.gnunet.gnu -r | grep "127.0.0.2" >/dev/null
(("status_sum+=$?"))
gnunet-gns -t A -u this.is.a.test.gnunet.gnu -r | grep "127.0.1.3" >/dev/null
(("status_sum+=$?"))

# check identities
gnunet-identity -d
gnunet-namestore -D -z gnunet.gnu -n ping
# Cleanup
gnunet-identity -D gnunet.gnu
gnunet-identity -D is.a.test.gnunet.gnu
gnunet-identity -D a.test.gnunet.gnu
gnunet-identity -D test.gnunet.gnu
gnunet-identity -D ping.gnunet.gnu
rm dnszone_gnunet.gnu
kill -9 "$(cat knot.pid)"

# Results
if [[ 0 -eq $status_sum ]]; then
    echo "PASS"
    exit 0
else
    echo "FAIL"
    exit 1
fi
