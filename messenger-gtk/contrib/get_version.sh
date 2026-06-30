#!/bin/sh
# Gets the version number from git, or from the contents of .version
cd "${MESON_SOURCE_ROOT}"
VERSION=
if test -f ".version"
then
  VERSION=$(cat .version)
fi
if test -d "./.git"
then
  git submodule init > /dev/null
  git submodule update > /dev/null
  VERSION=$(git describe --tags)
  VERSION=${VERSION#v}
  echo $VERSION > .version
fi
if test "x$VERSION" = "x"
then
  VERSION="unknown"
fi
echo "$VERSION"
