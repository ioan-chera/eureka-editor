#!/bin/bash
set -e

if [ ! -d glbsp_src ]; then
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
cp -av *.cfg $dest
cp -av misc/about_logo.png $dest

svn export common $dest/common
svn export games  $dest/games
svn export ports  $dest/ports
svn export mods   $dest/mods

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
