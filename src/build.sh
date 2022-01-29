#!/bin/bash
# create a metamath executable from scratch

# You MUST change to the src folder before running this script.

# Draft version, proof of concept.

#===========   setup environment   =====================

SRCDIR=$(pwd)

# verify we can navigate to the sources
if [ ! -f $SRCDIR/metamath.c ] || [ ! -f $SRCDIR/build.sh ]
then
  echo 'This script must be run from a subfolder of the metamath directory'
  exit
fi

TOPDIR=$SRCDIR/..
BUILDDIR=$TOPDIR/build

HAVE_DOXYGEN=0
if command doxygen -v
then
  HAVE_DOXYGEN=1
fi

# clear the build directory
rm -rf $BUILDDIR
mkdir $BUILDDIR
cd $BUILDDIR

#=========   symlink files to the build directory   ==============

cp --symbolic-link $SRCDIR/* $BUILDDIR
mv $BUILDDIR/configure.ac $BUILDDIR/configure.ac.orig
mv $BUILDDIR/Doxyfile.diff $BUILDDIR/Doxyfile.diff.orig

#=========   patch the version in configure.ac and Doxyfile.diff  =============

# look in metamath.c for a line matching the pattern '  #define MVERSION "<version>" '
# and save the line in VERSION
VERSION=`grep '[[:space:]]*#[[:space:]]*define[[:space:]][[:space:]]*MVERSION[[:space:]][[:space:]]*"[^"]*"' $SRCDIR/metamath.c`

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

# patch the Doxyfile.diff
cp $BUILDDIR/Doxyfile.diff.orig $BUILDDIR/Doxyfile.diff
sed --in-place "s/\\(PROJECT_NUMBER[[:space:]]*=[[:space:]]*\\)\"Metamath-version\".*/\\1\"$VERSION\"/" $BUILDDIR/Doxyfile.diff

rm $BUILDDIR/configure.ac.orig $BUILDDIR/Doxyfile.diff.orig

#===========   do the build   =====================

autoreconf -i
./configure
make

if [[ $HAVE_DOXYGEN -gt 0 ]]
then
  # create a Doxyfile.tmp and use it for creation f documentation locally
  
  # start with the settings given by the distributiion
  cp $BUILDDIR/Doxyfile.diff $BUILDDIR/Doxyfile.tmp
  
  # let the users preferences always override...
  if [ -f $SRCDIR/Doxyfile ]
  then
    cat $SRCDIR/Doxyfile >> $BUILDDIR/Doxyfile.tmp
  fi
  
  # ... except for the destination directory.  Force this to the build folder.
  echo "OUTPUT_DIRECTORY = $BUILDDIR" >> $BUILDDIR/Doxyfile.tmp

  doxygen $BUILDDIR/Doxyfile.tmp
fi
