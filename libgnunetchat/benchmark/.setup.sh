#!/bin/sh

gnunet-arm -e 2> /dev/null

rm -r ~/.cache/gnunet 2> /dev/null
rm -r ~/.local/share/gnunet/messenger 2> /dev/null
rm -r ~/.local/share/gnunet/identity 2> /dev/null
rm -r ~/.local/share/gnunet/fs 2> /dev/null
rm -r ~/.local/share/gnunet/namecache 2> /dev/null
rm -r ~/.local/share/gnunet/namestore 2> /dev/null
rm -r ~/.local/share/gnunet/datastore 2> /dev/null
rm -r ~/.local/share/gnunet/rest 2> /dev/null
rm ~/.local/share/gnunet/revocation.dat 2> /dev/null

gnunet-arm -s
sleep 0.5

BUILD_DIR=$(dirname $0)/../.build_benchmark

rm -r $BUILD_DIR 2> /dev/null
meson setup $BUILD_DIR > /dev/null
meson compile -C $BUILD_DIR > /dev/null
