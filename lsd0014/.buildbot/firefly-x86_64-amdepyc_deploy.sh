#!/bin/bash

# Deploy rest api from buildbot

if [ -e index.html ]; then
  rm index.html
fi
ln -s draft-schanzen-pils.html index.html
chmod -R ag+rX draft-schanzen-pils.* index.html .
rsync --exclude=".*" --exclude="Makefile" -a --delete ./ lsd@firefly.gnunet.org:~/public/lsd0014/
