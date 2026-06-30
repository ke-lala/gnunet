#!/bin/bash
# Copyright (C) 2019 rexxnor
# License AGPLv3+: GNU AGPL version 3 or later <https://www.gnu.org/licenses/agpl.html>
# This is free software: you are free to change and redistribute it.
# There is NO WARRANTY, to the extent permitted by law.
# This file is in the public domain.

## VARS
MYEGO=myego
gnunet-identity -C myego

# HELPERS
get_record_type() {
    arr=$1
    typ=$(echo -n "${arr[0]}" | cut -d' ' -f2)
    echo "$typ"
}

get_value() {
    arr=$1
    val=$(echo -n "${arr[0]}" | cut -d' ' -f4-)
    echo "$val"
}

testing() {
    label=$1
    records=$2
    recordstring=""
    for i in "${records[@]}"
    do
        recordstring+="-R $i"
    done
    gnunet-namestore -z "$MYEGO" -n "$label" "$recordstring"
    if [ 0 -ne $? ]; then
        echo "failed to add record $label: $recordstring"
    fi
    ret=$(gnunet-namestore -D -z "$MYEGO" -n "$label")
    for i in "${records[@]}"
    do
        value=$(get_value "$i")
        if [[ $ret == *"$value"* ]]; then
            echo "Value(s) added successfully!"
            return 0
        else
            exit 1
        fi
    done
}

# TEST CASES
# 1
echo "Testing adding of single A record with -R"
local arr=('1200 A n 127.0.0.1')
testing test1 "${arr[@]}"
# 2
echo "Testing adding of multiple A records with -R"
local arr=('1200 A n 127.0.0.1' '2400 A n 127.0.0.2')
testing test2 "${arr[@]}"
# 3
echo "Testing adding of multiple different records with -R"
local arr=('1200 A n 127.0.0.1' '2400 AAAA n 2002::')
testing test3 "${arr[@]}"
# 4
echo "Testing adding of single GNS2DNS record with -R"
local arr=('86400 GNS2DNS n gnu.org@127.0.0.1')
testing test4 "${arr[@]}"
# 5
echo "Testing adding of single GNS2DNS shadow record with -R"
local arr=('86409 GNS2DNS s gnu.org@127.0.0.250')
testing test5 "${arr[@]}"
# 6
echo "Testing adding of multiple GNS2DNS record with -R"
local arr=('1 GNS2DNS n gnunet.org@127.0.0.1' '3600 GNS2DNS s gnunet.org@127.0.0.2')
testing test6 "${arr[@]}"
# 7
echo "Testing adding MX record with -R"
local arr=('3600 MX n 10,mail')
testing test7 "${arr[@]}"
# 8
echo "Testing adding TXT record with -R"
local arr=('3600 TXT n Pretty_Unicorns')
testing test8 "${arr[@]}"
# 8
echo "Testing adding SRV record with -R"
local arr=('3600 SRV n 0 0 443 testing')
testing _autodiscover_old._tcp "${arr[@]}"
# 9
echo "Testing adding many A records with -R"
local arr=('3600 A n 127.0.0.1' '3600 A n 127.0.0.2' '3600 A n 127.0.0.3' '3600 A n 127.0.0.4' '3600 A n 127.0.0.5')
testing test9 "${arr[@]}"

# CLEANUP
gnunet-identity -D "$MYEGO"
