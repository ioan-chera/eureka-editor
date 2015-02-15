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
cp Eureka.exe $dest

#
#  Data files
#
svn export common $dest/common
svn export games  $dest/games
svn export ports  $dest/ports
svn export mods   $dest/mods

#
#  Documentation
#
cp -av *.txt $dest

#
# all done
#
echo "------------------------------------"
echo "All done."

