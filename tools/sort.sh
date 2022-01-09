#!/bin/bash
# Shell script equivalent of Metamath SORT tool
# Mario Carneiro 7-Jan-2022

usage() {
  cat << HELP
SORT - Sort the lines in a file with key starting at specified string

This command sorts a file, comparing lines starting at a key string.
If the key string is blank, the line is compared starting at column 1.
If a line doesn't contain the key, it is compared starting at column 1.
Syntax:  sort.sh KEY < INFILE > OUTFILE
HELP
}

if [ $# -eq 1 ] && [ "$1" = "-h" -o "$1" = "--help" ]; then usage; exit; fi
if [ $# -ne 1 ]; then usage; exit 1; fi

if [ "$1" == "" ]; then
  sort
else
  set -o pipefail
  awk -v matchKey="$1" '{
    n = index($0, matchKey);
    key = n ? substr($0, n) : $0;
    if (index(key, "\x1F")) {
      print "unsupported binary file" > "/dev/stderr";
      exit 2;
    }
    print key "\x1F" $0;
  }' | sort -k1,1 -t $'\x1F' | sed 's/.*\x1F//'
fi
