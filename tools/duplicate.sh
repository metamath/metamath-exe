#!/bin/bash
# Shell script equivalent of Metamath DUPLICATE tool
# Mario Carneiro 7-Jan-2022

usage() {
  cat << HELP
DUPLICATE - Extract first occurrence of any line occurring more than
      once in a file, discarding lines occurring exactly once

This command finds all duplicate lines in a file and places them, in
sorted order, into the output file.
Syntax:  duplicate.sh < INFILE > OUTFILE
HELP
}

if [ $# -eq 1 ] && [ "$1" = "-h" -o "$1" = "--help" ]; then usage; exit; fi
if [ $# -ne 0 ]; then usage; exit 1; fi

sort | uniq -d
