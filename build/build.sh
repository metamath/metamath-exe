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
mv $BUILDDIR/configure.ac $BUILDDIR/configure.ac.orig

#=========   patch the version in configure.ac   ===================

# look in metamath.c for a line matching the pattern '  #define MVERSION "<version>"
# and save the line in VERSION
VERSION=`grep '[[:space:]]*#[[:space:]]*define[[:space:]]*MVERSION[[:space:]]"[^"]*"' $SRCDIR/metamath.c`

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

rm $BUILDDIR/configure.ac.orig

#===========   run autoconf   =====================

cd $BUILDDIR
autoreconf -i
./configure
make
