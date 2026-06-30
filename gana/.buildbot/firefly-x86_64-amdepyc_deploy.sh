#!/bin/bash

# Deploy rest api from buildbot

chmod -R ag+rX _build
rsync --exclude=".*" --exclude="Makefile" --exclude="conf.py" -a --delete ./_build/ handbook@firefly.gnunet.org:~/gana/_build/
