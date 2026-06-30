#!/bin/bash

# Deploy rest api from buildbot

if [ -e index.html ]; then
  rm index.html
fi
ln -s draft-schanzen-cake.html index.html
chmod -R ag+rX draft-schanzen-cake.* index.html .
rsync --exclude=".*" --exclude="Makefile" -a --delete ./ lsd@firefly.gnunet.org:~/public/lsd0012/
