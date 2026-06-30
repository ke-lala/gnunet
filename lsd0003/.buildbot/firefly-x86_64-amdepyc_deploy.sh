#!/bin/bash

# Deploy rest api from buildbot

if [ -e index.html ]; then
  rm index.html
fi
ln -s draft-summermatter-set-union.html index.html
chmod -R ag+rX draft-summermatter-set-union.* index.html .
rsync --exclude=".*" --exclude="Makefile" -a --delete ./ lsd@firefly.gnunet.org:~/public/lsd0003/
