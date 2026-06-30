#!/bin/bash
# Copyright (C) 2019 rexxnor
# License AGPLv3+: GNU AGPL version 3 or later <https://www.gnu.org/licenses/agpl.html>
# This is free software: you are free to change and redistribute it.
# There is NO WARRANTY, to the extent permitted by law.
#
# Returns 1 on basic error
# Returns 2 on explicit test case errors
# Returns 3 on implicit test case errors

# Shutdown named
function cleanup {
    pkill named
    gnunet-identity -D gnunet.org
}

# Check for required packages
if ! [ -x "$(command -v named)" ]; then
    echo 'bind/named is not installed' >&2
    exit 1
fi

if ! [ -x "$(command -v ascension)" ]; then
    echo 'ascension is not installed' >&2
    exit 1
fi

if ! [ -x "$(command -v gnunet-arm)" ]; then
    echo 'gnunet is not installed' >&2
    exit 1
fi

gnunet-arm -T 1s -I
if [ "$?" -ne 0 ]; then
    echo "The gnunet peer is not running" >&2
    exit 1
fi

# Get the hostname to simulate external host
myhostname=$(echo -n "$(hostname)")

# Start named with a simple zone
named -c basic_named.conf -p 5000

# Check if domain resolves
nslookup gnunet.org "$myhostname" -port=5000
if [ "$?" -ne 0 ]; then
    echo "Something went wrong with named"
    cleanup
    exit 1
fi

# Let ascension run on gnunet.org test domain
ascension gnunet.org -n "$myhostname" -p 5000 -s -d
if [ "$?" -ne 0 ]; then
    echo "ascension failed adding the records!"
    cleanup
    exit 1
fi

function checkfailexp  {
    if [ "$?" -ne 0 ]; then
        echo "required record not present"
        cleanup
        exit 2
    fi
}

function checkfailimp  {
    if [ "$?" -ne 0 ]; then
        echo "implied record not present"
        cleanup
        exit 3
    fi
}

# TESTING explicit records
gnunet-gns -t PKEY -u @.gnunet.org
checkfailexp

# cleanup if we get this far
cleanup

# finish
echo "All records added successfully!!"
