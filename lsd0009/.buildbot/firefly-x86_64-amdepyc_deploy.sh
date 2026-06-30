#!/bin/bash

# Deploy rest api from buildbot

if [ -e index.html ]; then
  rm index.html
fi
ln -s draft-guetschow-taler-protocol.html index.html
chmod -R ag+rX draft-guetschow-taler-protocol.* index.html .
rsync --exclude=".*" --exclude="Makefile" -a --delete ./ lsd@firefly.gnunet.org:~/public/lsd0009/
