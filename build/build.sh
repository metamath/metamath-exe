#!/bin/bash
# create a metamath executable from scratch

# You MUST change to the build folder before running this script.

# Draft version, proof of concept.

BUILDDIR=$(pwd)
TOPDIR=$BUILDDIR/..
SRCDIR=$TOPDIR/src

if [ ! -f $SRCDIR/metamath.c ] || [ !  -f $BUILDDIR/build.sh ]
then
  echo 'This script must be started from the metamath/build directory'
  exit
fi

HAVE_DOXYGEN=0
if command doxygen -v
then
  HAVE_DOXYGEN=1
fi

# clear the build directory, but keep build.sh and Metamath.png
find $BUILDDIR/* -not -name 'build.sh' -not -name 'Metamath.png' -delete

#=========   symlink files to the build directory   ==============

cp --symbolic-link $SRCDIR/* $BUILDDIR
mv $BUILDDIR/configure.ac $BUILDDIR/configure.ac.orig
mv $BUILDDIR/Doxyfile.diff $BUILDDIR/Doxyfile.diff.orig
cp $BUILDDIR/Doxyfile.diff.orig $BUILDDIR/Doxyfile.diff

#=========   patch the version in configure.ac   ===================

# look in metamath.c for a line matching the pattern '  #define MVERSION "<version>" '
# and save the line in VERSION
VERSION=`grep '[[:space:]]*#[[:space:]]*define [[:space:]]*MVERSION [[:space:]]*"[^"]*"' $SRCDIR/metamath.c`

# extract the version (without quotes) from the saved line

# strip everything up to and including the first quote character
VERSION=${VERSION#*\"}
# strip everything from the first remaining quote character on
VERSION=${VERSION%%\"*}

# find the line with the AC_INIT command, prepend the line number
# line-nr:AC_INIT([FULL-PACKAGE-NAME], [VERSION], [REPORT-ADDRESS])
AC_INIT_LINE=`grep -n '[[:space:]]*AC_INIT[[:space:]]*(.*' $BUILDDIR/configure.ac.orig`
# strip everything from the first colon on
AC_INIT_LINE_NR=${AC_INIT_LINE%%:*}
# strip everything up to the first colon
AC_INIT_LINE=${AC_INIT_LINE#*:}
# replace the second parameter to AC_INIT
PATCHED_INIT_LINE=`echo $AC_INIT_LINE | sed "s/\\([[:space:]]*AC_INIT(.*\\),.*,\\(.*\\)/\\1, \[$VERSION\],\\2/"`

# replace the AC_INIT line with new content
head -n $(($AC_INIT_LINE_NR - 1)) $BUILDDIR/configure.ac.orig > $BUILDDIR/configure.ac
echo $PATCHED_INIT_LINE >> $BUILDDIR/configure.ac
tail -n +$(($AC_INIT_LINE_NR + 1)) $BUILDDIR/configure.ac.orig >> $BUILDDIR/configure.ac

# patch the version in the Doxyfile.diff
sed --in-place "s/\\(PROJECT_NUMBER[[:space:]]*=[[:space:]]*\\)\"Metamath-version\".*/\\1\"$VERSION\"/" $BUILDDIR/Doxyfile.diff

rm $BUILDDIR/configure.ac.orig $BUILDDIR/Doxyfile.diff.orig

#===========   run autoconf   =====================

cd $BUILDDIR
autoreconf -i
./configure
make

if [[ $HAVE_DOXYGEN -gt 0 ]]
then
  rm -rf $TOPDIR/doxy
  doxygen Doxyfile.diff
fi
