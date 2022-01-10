#!/bin/bash
# Shell script equivalent of Metamath COPY tool
# Mario Carneiro 7-Jan-2022

usage() {
  cat << HELP
COPY - Similar to Unix "cat" but safe (same input & output name allowed)

This command copies (concatenates) all input files in a space separated
list to an output file.  The output file may have
the same name as an input file.
Unlike the metamath version, this script does not save a previous
version of the output file with a ~1 extension. (TODO: implement?)
Example: "copy.sh 1.tmp 1.tmp 1.tmp 2.tmp" followed by "unique.sh 1.tmp"
will result in 1.tmp containing those lines of 2.tmp that didn't
previously exist in 1.tmp.
Syntax:  copy.sh OUTFILE INFILE INFILE ...
HELP
}

if [ $# -eq 1 ] && [ "$1" = "-h" -o "$1" = "--help" ]; then usage; exit; fi

outfile=$1; shift
buf=$(cat $@)
echo "$buf" > "$outfile"
