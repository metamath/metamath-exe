#!/bin/bash
# Shell script equivalent of Metamath UNIQUE tool
# Mario Carneiro 7-Jan-2022

usage() {
  cat << HELP
UNIQUE - Extract lines occurring exactly once in a file

This command finds all unique lines in a file and places them, in
sorted order, into the output file.
Syntax:  unique.sh < INFILE > OUTFILE
HELP
}

if [ $# -eq 1 ] && [ "$1" = "-h" -o "$1" = "--help" ]; then usage; exit; fi
if [ $# -ne 0 ]; then usage; exit 1; fi

sort | uniq -u
