#!/bin/bash
# create a metamath executable from scratch

# Change to the top folder of metamath-exe before
# running this script, unless you provide the -m option.

# Draft version, proof of concept.

help_text=\
'Builds all artefacts in the metamath-exe/build subfolder (if not directed
otherwise).  Change to the metamath-exe top folder first before running
this script, or issue the -m option.

Possible options are:

-a Used by autotools. Put hyphens in the version string when used with -v
-b build binary only, no reconfigure. Faster, but should not be used on first run.
-c Clean the build directory.
-d build documentation using Doxygen, in addition to building the executable.
-g build in debug mode: enables debug symbols and turns off optimizations
-h print this help and exit.
-m followed by a directory: top folder of metamath-exe.
    Relative paths are relative to the current directory.
-o followed by a directory: optionally clean directory and build all artefacts there.
    Relative paths are relative to the destination'"'"'s top metamath-exe directory.
-t compile a metamath_test executable for regression tests
-v extract the version from metamath sources, print it and exit'

#============   evaluate command line parameters   ==========

do_autoconf=1
do_clean=0
do_doc=0
debug=0
do_make_test=0
print_help=0
version_only=0
version_for_autoconf=0
unset dest_dir
unset doc_dir
top_dir="$(pwd)"

while getopts abcdghm:o:tv flag
do
  case "${flag}" in
    a) version_for_autoconf=1;;
    b) do_autoconf=0;;
    c) do_clean=1;;
    d) do_doc=1;;
    g) debug=1;;
    h) print_help=1;;
    m) cd "${OPTARG}" && top_dir=$(pwd);;
    o) dest_dir=${OPTARG};;
    t) do_make_test=1;;
    v) version_only=1;;
    *) echo "unknown parameter" >&2
       exit 1;;
  esac
done

if [ $print_help -gt 0 ]
then
  echo "$help_text"
  exit
fi

#===========   setup environment   =====================

src_dir="$top_dir/src"
build_dir=${dest_dir:-"$top_dir/build"}
doc_dir=${dest_dir:-"$top_dir/build"}

# verify we can navigate to the sources
if [ ! -f "$src_dir/metamath.c" ]
then
  echo 'This script must be run from the top folder of the metamath-exe directory.'
  echo 'Run ./build.sh -h for more information'
  exit
fi

cd "$top_dir" || exit

#=========   extract the version from metamath.c  =============

# look in metamath.c for a line matching the pattern '  #define MVERSION "<version>" '
# and save the line in VERSION
version=$(grep '[[:space:]]*#[[:space:]]*define[[:space:]][[:space:]]*MVERSION[[:space:]][[:space:]]*"[^"]*"' "$src_dir/metamath.c")

# extract the version (without quotes) from the saved line

# strip everything up to and including the first quote character
version=${version#*\"}
# strip everything from the first remaining quote character on
version=${version%%\"*}

if [ $version_only -eq 1 ]
then
  if [ $version_for_autoconf -eq 1 ]; then
    echo "$version" | tr ' ' -
  else
    echo "$version"
  fi
  exit
fi

# clear the build directory
if [ $do_clean -eq 1 ]
then
  rm -rf "$build_dir"
  if [ $do_autoconf -eq 0 ]; then exit; fi
fi

# Enter the build directory
mkdir -p "$build_dir"
cd "$build_dir" || exit

# allow external programs easy access to the metamath version extracted from
# the sources
echo "$version" > metamath_version

#===========   run the configure.ac   =====================

if [ $do_autoconf -eq 1 ]
then
  cd "$top_dir" || exit
  autoreconf -i

  cd "$build_dir" || exit
  if [ $debug -eq 1 ]; then
    # FIXME: this gives conflicting -O flags to gcc
    # but It Works on My Machine (TM)
    "$top_dir/configure" -q CFLAGS="-g -O0"
  else
    "$top_dir/configure" -q
  fi
fi

#===========   do the build   =====================

if [ $do_make_test -eq 1 ]
then
  # create an executable running regression tests
  make "CFLAGS=-DTEST_ENABLE"
  mv src/metamath "$top_dir"/metamath_test
else
  # normal executable
  make
  mv src/metamath "$top_dir"
fi

#===========   run Doxygen documentation generator   =====================

if [ $do_doc -eq 1 ]
then
  rm -rf "$doc_dir/html"

  if ! command doxygen -v &> /dev/null; then
    echo >&2 'doxygen not found. Cannot build documentation.'
    exit 1
  fi

  # create a Doxyfile.local and use it for creation of documentation locally

  # start with the settings given by the distribution
  cp "$top_dir/Doxyfile.diff" Doxyfile.local

  # add the project version number
  echo "PROJECT_NUMBER = \"$version\"" >> Doxyfile.local

  # let the users preferences always override...
  if [ -f "$top_dir/Doxyfile" ]
  then
    cat "$top_dir/Doxyfile" >> Doxyfile.local
  fi

  # ... except for the source/destination directory.  Force this to folders
  # based on this build.
  echo "OUTPUT_DIRECTORY = \"$doc_dir\"" >> Doxyfile.local
  echo "INPUT = \"$src_dir\"" >> Doxyfile.local

  # make sure the logo is in the build directory
  cp --force --symbolic-link "$top_dir/doc/Metamath.png" .

  doxygen Doxyfile.local
  echo "Documentation has been generated at $doc_dir/html/index.html"
fi
