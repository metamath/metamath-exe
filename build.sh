#!/bin/bash
# create a metamath executable from scratch

# Change to a subfolder of metamath-exe (e.g. the src folder) before
# running this script, unless you provide the -m option.

# Draft version, proof of concept.

help_text=\
'Builds all artefacts in the metamath-exe/build subfolder (if not directed
otherwise).  Change to a metamath-exe subfolder first before running
this script, or issue the -m option.

Possible options are:

-o followed by a directory: clear directory and build all artefacts there.
    Relative paths are relative to the destination'"'"'s top metamath-exe directory.
-b build binary only, no reconfigure. Faster, but should not be used on first run.
-c Clean the build directory.
-d build documentation using Doxygen, in addition to building the executable.
-h print this help and exit.
-m followed by a directory: top folder of metamath-exe.
    Relative paths are relative to the current directory.
-v extract the version from metamath sources, print it and exit'

# we return back to this directory
cur_dir="$(pwd)"

#============   evaluate command line parameters   ==========

do_autoconf=1
do_clean=0
do_doc=0
print_help=0
version_only=0
unset dest_dir
top_dir="$cur_dir"

while getopts d:ehm:v flag
do
  case "${flag}" in
    b) do_autoconf=0;;
    c) do_clean=1;;
    d) do_doc=1;;
    h) print_help=1;;
    m) cd "${OPTARG}" && top_dir=$(pwd);;
    o) dest_dir=${OPTARG};;
    v) version_only=1;;
  esac
done

if [ ${print_help:-0} -gt 0 ]
then
  echo "$help_text"
  exit
fi

#===========   setup environment   =====================

src_dir="$top_dir/src"
build_dir=${dest_dir:-"$top_dir/build"}

# verify we can navigate to the sources
if [ ! -f "$src_dir/metamath.c" ]
then
  echo 'This script must be run from a subfolder of the metamath-exe directory.'
  echo 'Run ./build.sh -h for more information'
  cd "$cur_dir"
  exit
fi

cd "$top_dir"

# clear the build directory
if [ $do_clean -eq 1 ]
then
  rm -rf "$build_dir"
  if [ $do_autoconf -eq 0 ]; then exit; fi
fi

# Enter the build directory
mkdir -p "$build_dir"
cd "$build_dir"

#=========   symlink files to the build directory   ==============

cp --symbolic-link "$src_dir"/* .
mv configure.ac configure.ac.orig

#=========   patch the version in configure.ac and Doxyfile.diff  =============

# look in metamath.c for a line matching the pattern '  #define MVERSION "<version>" '
# and save the line in VERSION
version=`grep '[[:space:]]*#[[:space:]]*define[[:space:]][[:space:]]*MVERSION[[:space:]][[:space:]]*"[^"]*"' "$src_dir"/metamath.c`

# extract the version (without quotes) from the saved line

# strip everything up to and including the first quote character
version=${version#*\"}
# strip everything from the first remaining quote character on
version=${version%%\"*}

if [ $version_only -eq 1 ]
then
  echo "$version"
  cd "$cur_dir"
  exit
fi

#===========   patch and run the configure.ac   =====================

# allow external programs easy access to the metamath version extracted from
# the sources
echo "$version" > metamath_version

if [ $do_autoconf -eq 1 ]
then

  # find the line with the AC_INIT command, prepend the line number
  # line-nr:AC_INIT([FULL-PACKAGE-NAME], [VERSION], [REPORT-ADDRESS])
  ac_init_line=`grep -n '[[:space:]]*AC_INIT[[:space:]]*(.*' configure.ac.orig`
  # strip everything from the first colon on
  ac_init_line_nr=${ac_init_line%%:*}
  # strip everything up to the first colon
  ac_init_line=${ac_init_line#*:}
  # replace the second parameter to AC_INIT
  patched_init_line=`echo "$ac_init_line" | sed "s/\\([[:space:]]*AC_INIT(.*\\),.*,\\(.*\\)/\\1, \[$version\],\\2/"`

  # replace the AC_INIT line with new content
  head -n $(($ac_init_line_nr - 1)) configure.ac.orig > configure.acrm configure.ac.orig

  echo $patched_init_line >> configure.ac
  tail -n +$(($ac_init_line_nr + 1)) configure.ac.orig >> configure.ac

  rm configure.ac.orig

  autoreconf -i
  ./configure
fi

#===========   do the build   =====================

make

if [ $do_doc -eq 1 ]
then
  if ! command doxygen -v
    echo >&2 'doxygen not found. Cannot build documentation.'
    exit 1
  fi

  # patch the Doxyfile.diff
  cp Doxyfile.diff Doxyfile.diff.orig
  sed --in-place "s/\\(PROJECT_NUMBER[[:space:]]*=[[:space:]]*\\)\"Metamath-version\".*/\\1\"$version\"/" Doxyfile.diff
  rm Doxyfile.diff.orig

  # create a Doxyfile.local and use it for creation of documentation locally

  # start with the settings given by the distribution
  cp Doxyfile.diff Doxyfile.local

  # let the users preferences always override...
  if [ -f "$src_dir"/Doxyfile ]
  then
    cat "$src_dir"/Doxyfile >> Doxyfile.local
  fi

  # ... except for the destination directory.  Force this to the build folder.
  echo "OUTPUT_DIRECTORY = \"$build_dir\"" >> Doxyfile.local

  doxygen Doxyfile.local
fi

cd "$cur_dir"
