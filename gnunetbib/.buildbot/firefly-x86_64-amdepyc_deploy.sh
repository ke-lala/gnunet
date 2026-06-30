#!/bin/bash

# Deploy bib from buildbot

chmod -R ag+rX .
DEPLOY_USER="www"
rsync -a --delete . $DEPLOY_USER@firefly.gnunet.org:~/bib/
