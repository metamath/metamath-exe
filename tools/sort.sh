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

if [ "$1" != "" ]; then echo "SORT with key unsupported"; exit 2; fi

sort
