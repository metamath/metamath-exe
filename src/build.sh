#!/bin/bash
# create a metamath executable from scratch

# Change to a subfolder of metamath-exe (e.g. the src folder) before
# running this script, unless you provide the -m option.

# Draft version, proof of concept.

#============   evaluate command line parameters   ==========

while getopts d:ehm:v flag
do
  case "${flag}" in
    d) destdir=${OPTARG};;
    e) bin_only=1;;
    h) print_help=1;;
    m) cd "${OPTARG}" && metamathdir=$(pwd);;
    v) version_only=1;;
  esac
done

if [ ${print_help:-0} -gt 0 ]
then
  echo 'Builds all artefacts in the metamath-exe/build subfolder (if not directed'
  echo 'otherwise).  Change to a metamath-exe subfolder first before running'
  echo 'this script, or issue the -m option.'
  echo
  echo 'Possible options are:'
  echo
  echo '-d followed by a directory: clear directory and build all artefacts there.'
  echo "    Relative paths are relative to the destination's top metamath-exe directory."
  echo '-e only build executable, skip documentation.'
  echo '-h print this help and exit.'
  echo '-m followed by a directory: top folder of metamath-exe.'
  echo '    Relative paths are relative to the current directory.'
  echo '-v extract the version from metamath sources, print it and exit'
  exit
fi

#===========   setup environment   =====================

TOPDIR=${metamathdir:-"$(pwd)/.."}
SRCDIR=$TOPDIR/src
BUILDDIR=${destdir:-$TOPDIR/build}

# verify we can navigate to the sources
if [ ! -f $SRCDIR/metamath.c ]
then
  echo 'This script must be run from a subfolder of the metamath-exe directory.'
  echo 'Run ./build.sh -h for more information'
  exit
fi

# clear the build directory
cd "$TOPDIR"
rm -rf $BUILDDIR
mkdir -p $BUILDDIR
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

if [ ${version_only:-0} -gt 0 ]
then
  echo "$VERSION"
  exit
fi

# allow external programs easy access to the metamath version extracted from
# the sources
echo "$VERSION" > metamath_version

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

if [ ${bin_only:-0} -eq 0 ]
then
  if ! command doxygen -v
  then
    echo 'doxygen not found. Cannot build documentation.'
  fi

  # create a Doxyfile.local and use it for creation of documentation locally
  
  # start with the settings given by the distribution
  cp $BUILDDIR/Doxyfile.diff $BUILDDIR/Doxyfile.local
  
  # let the users preferences always override...
  if [ -f $SRCDIR/Doxyfile ]
  then
    cat $SRCDIR/Doxyfile >> $BUILDDIR/Doxyfile.local
  fi
  
  # ... except for the destination directory.  Force this to the build folder.
  echo "OUTPUT_DIRECTORY = $BUILDDIR" >> $BUILDDIR/Doxyfile.local

  doxygen $BUILDDIR/Doxyfile.local
fi
