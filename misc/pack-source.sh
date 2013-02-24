#!/bin/bash
set -e

if [ ! -d glbsp_src ]; then
  echo "Run this script from the top level."
  exit 1
fi

echo "Creating source package for Eureka..."

cd ..

topdir=$PWD

src=eureka
dest=PACK-SRC

mkdir $dest

#
#  Source code
#
mkdir $dest/src
cp -av $src/src/*.[chr]* $dest/src
cp -av $src/Makefile* $dest/
# cp -av $src/src/*.ico $dest/src

rm -f ~/osx.tar
cd $src
tar cf ~/osx.tar --exclude-vcs osx
cd $topdir
cd $dest
tar xf ~/osx.tar
cd $topdir
rm -f ~/osx.tar

mkdir $dest/glbsp_src
cp -av $src/glbsp_src/*.[chr]* $dest/glbsp_src

mkdir $dest/misc
cp -av $src/misc/*.* $dest/misc
mkdir $dest/misc/debian
cp -av $src/misc/debian/* $dest/misc/debian

mkdir $dest/obj_linux
mkdir $dest/obj_linux/glbsp
mkdir $dest/obj_win32
mkdir $dest/obj_win32/glbsp

#
#  Data files
#
mkdir $dest/common
cp -av $src/common/*.* $dest/common || true

mkdir $dest/games
cp -av $src/games/*.* $dest/games || true

mkdir $dest/ports
cp -av $src/ports/*.* $dest/ports || true

mkdir $dest/mods
cp -av $src/mods/*.* $dest/mods || true

#
#  Documentation
#
cp -av $src/*.txt $dest

mkdir $dest/docs
cp -av $src/docs/*.* $dest/docs

mkdir $dest/changelogs
cp -av $src/changelogs/*.* $dest/changelogs

#
# all done
#
echo "------------------------------------"
echo "All done."

