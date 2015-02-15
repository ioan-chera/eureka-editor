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

svn export src $dest/src
svn export osx $dest/osx
svn export glbsp_src $dest/glbsp_src
svn export misc $dest/misc

mkdir $dest/obj_linux
mkdir $dest/obj_linux/glbsp
mkdir $dest/obj_win32
mkdir $dest/obj_win32/glbsp

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

svn export docs $dest/docs
svn export changelogs $dest/changelogs

#
# all done
#
echo "------------------------------------"
echo "All done."

