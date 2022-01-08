#!/bin/bash
# Shell script equivalent of Metamath PARALLEL tool
# Mario Carneiro 7-Jan-2022

usage() {
  cat << HELP
PARALLEL - Put two files in parallel

This command puts two files side-by-side.
The two files should have the same number of lines; if not, the longer
file is paralleled with empty strings at the end.
Syntax:  parallel.sh INFILE1 INFILE2 BTWNSTR > OUTFILE
HELP
}

if [ $# -eq 1 ] && [ "$1" = "-h" -o "$1" = "--help" ]; then usage; exit; fi
if [ $# -ne 3 ]; then usage; exit 1; fi

paste -d "$3" "$1" "$2"
