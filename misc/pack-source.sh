#!/bin/bash
set -e

if [ ! -d glbsp_src ]; then
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
cp -av glbsp_src $dest/glbsp_src
cp -av misc $dest/misc

mkdir $dest/obj_linux
mkdir $dest/obj_linux/glbsp
mkdir $dest/obj_win32
mkdir $dest/obj_win32/glbsp

#
#  Data files
#
cp -av bindings.cfg $dest

cp -av common $dest/common
cp -av games  $dest/games
cp -av ports  $dest/ports
cp -av mods   $dest/mods

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

