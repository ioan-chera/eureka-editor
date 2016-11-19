#!/bin/bash
set -e

if [ ! -d ports ]; then
  echo "Run this script from the top level."
  exit 1
fi

echo "Creating WIN32 package for Eureka..."

dest="Eureka-X.XX"

mkdir $dest

#
#  Executable(s)
#
cp -av Eureka.exe $dest

#
#  Data files
#
cp -av common $dest/common
cp -av games  $dest/games
cp -av ports  $dest/ports
mkdir         $dest/mods

cp -av bindings.cfg $dest

cp -av misc/about_logo.png $dest/common

#
#  Documentation
#
cp -av *.txt $dest
rm $dest/INSTALL.txt

#
# all done
#
echo "------------------------------------"
echo "zip -l -r eureka-XXX-win.zip Eureka-X.XX"
echo ""
