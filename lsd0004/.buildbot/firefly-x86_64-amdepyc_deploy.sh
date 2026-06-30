#!/bin/bash

# Deploy lsd from buildbot

if [ -e index.html ]; then
  rm index.html
fi
ln -s draft-schanzen-r5n.html index.html
chmod -R ag+rX draft-schanzen-r5n.* index.html .
rsync --exclude=".*" --exclude="Makefile" -a --delete ./ lsd@firefly.gnunet.org:~/public/lsd0004/
