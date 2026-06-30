#!/bin/bash
# Benchmarks with a 1000 record zone

echo "Starting knot..."
knotd -c benchmark.conf 2>/dev/null &

if [ ! -x "$(command -v "ascension")" ]; then
    echo "Ascension not found! Please install it"
    exit 60
fi

cleanup(){
    gnunet-identity -D 1000
    gnunet-identity -D 10000
    gnunet-identity -D 100000
    gnunet-identity -D 1000000
    rm dnszone_1000
    rm dnszone_10000
    rm dnszone_100000
    rm dnszone_1000000
    kill -9 "$(cat knot.pid)"
}

trap cleanup EXIT

# Time the 1000 records zone
time ascension -n 127.0.0.1 -P 5000 -s 1000. -l30
# Time the 10000 records zone
time ascension -n 127.0.0.1 -P 5000 -s 10000. -l30
# Time the 100000 records zone
time ascension -n 127.0.0.1 -P 5000 -s 100000. -l30
# Time the 1000000 records zone
time ascension -n 127.0.0.1 -P 5000 -s 1000000. -l30

# Cleanup
cleanup
