#!/bin/bash
# Shell script equivalent of Metamath UNDUPLICATE tool
# Mario Carneiro 7-Jan-2022

usage() {
  cat << HELP
UNDUPLICATE - Eliminate duplicate occurrences of lines in a file

This command sorts a file then removes any duplicate lines from the output.
Syntax:  unduplicate.sh < INFILE > OUTFILE
HELP
}

if [ $# -eq 1 ] && [ "$1" = "-h" -o "$1" = "--help" ]; then usage; exit; fi
if [ $# -ne 0 ]; then usage; exit 1; fi

sort -u
