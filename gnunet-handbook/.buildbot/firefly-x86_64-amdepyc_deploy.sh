#!/bin/bash

# Deploy rest api from buildbot

chmod -R ag+rX _build
rsync --exclude=".*" --exclude="Makefile" -a --delete ./_build/ handbook@firefly.gnunet.org:~/doc_deployment/sphinx/_build/
