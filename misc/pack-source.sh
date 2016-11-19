#!/bin/bash
set -e

if [ ! -d ports ]; then
  echo "Run this script from the top level."
  exit 1
fi

echo "Creating source package for Eureka..."

dest="eureka-X.XX-source"

mkdir $dest

#
#  Source code
#
cp -av Makefile* $dest/

cp -av src $dest/src
cp -av osx $dest/osx
cp -av misc $dest/misc

mkdir $dest/obj_linux
mkdir $dest/obj_win32

#
#  Data files
#
cp -av bindings.cfg $dest

cp -av common $dest/common
cp -av games  $dest/games
cp -av ports  $dest/ports
mkdir         $dest/mods

#
#  Documentation
#
cp -av *.txt $dest

cp -av docs $dest/docs
cp -av changelogs $dest/changelogs

#
# all done
#
echo "------------------------------------"
echo "All done."

