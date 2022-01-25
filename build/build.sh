#!/bin/bash
# create a metamath executable from scratch

# You MUST change to the build folder before running this script.

# Draft version, proof of concept.

BUILDDIR=$(pwd)
TOPDIR=$BUILDDIR/..
SRCDIR=$TOPDIR/src

# clear the build directory, but keep the build.sh
find $BUILDDIR/* -not -name 'build.sh' -delete

#=========   symlink files to the build directory   ==============

cp --symbolic-link $SRCDIR/* $BUILDDIR
mv $BUILDDIR/configure.ac configure.ac.orig

#=========   patch the version in configure.ac   ===================

# look in metamath.c for a line matching the pattern '  #define MVERSION "<version>"
# and save the line in VERSION
VERSION=`grep '[[:space:]]*#[[:space:]]*define[[:space:]]*MVERSION[[:space:]]"[^"]*"' $SRCDIR/metamath.c`
# extract the version (without quotes) from the saved line
VERSION=`echo $VERSION | sed 's/[^"]*"\([^"]*\)"/\1/'`

# find the line with the AC_INIT command, prepend the line number at the beginning
AC_INIT_LINE=`grep -n  '[[:space:]]*AC_INIT[[:space:]]*(.*' $BUILDDIR/configure.ac.orig`
AC_INIT_LINE_NR=`echo $AC_INIT_LINE | sed 's/\([0-9]*\).*/\1/'`
# remove the line number
AC_INIT_LINE=`echo $AC_INIT_LINE | sed 's/[0-9]:\(.*\)/\1/'`
PATCHED_INIT_LINE=`echo $AC_INIT_LINE | sed "s/\\([[:space:]]*AC_INIT(.*\\),.*,\\(.*\\)/\\1, \[$VERSION\],\\2/"`

# replace the AC_INIT line with new content
head -n $(($AC_INIT_LINE_NR - 1)) $BUILDDIR/configure.ac.orig > $BUILDDIR/configure.ac
echo $PATCHED_INIT_LINE >> $BUILDDIR/configure.ac
tail -n +$(($AC_INIT_LINE_NR + 1)) $BUILDDIR/configure.ac.orig >> $BUILDDIR/configure.ac

rm $BUILDDIR/configure.ac.orig

#===========   run autoconf   =====================

cd $BUILDDIR
autoreconf -i
./configure
make
cp -f metamath $SRCDIR
