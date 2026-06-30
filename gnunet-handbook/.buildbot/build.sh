#!/bin/bash

git branch -r \
  | grep -v '\->' \
  | sed "s,\x1B\[[0-9;]*[a-zA-Z],,g" \
  | while read remote; do \
      git branch --track "${remote#origin/}" "$remote"; \
    done

# Conditional extens switch in config
export SPHINX_MULTIVERSION=1

git tag --delete latest || echo "Deleting tag can fail"
# Get latest version branch
latest=$(git for-each-ref --format "%(refname)" | grep -i '^refs/remotes/origin/v[0-9]*\.[0-9]*\.x$' | sort -r | head -n1)
echo $latest" is latest ref"
git tag latest $latest
sphinx-multiversion . _build
