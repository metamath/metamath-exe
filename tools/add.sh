#!/bin/sh
# Shell script equivalent of Metamath ADD tool
# Mario Carneiro 5-Jan-2022

usage() {
  cat << HELP
ADD - Add a specified string to each line in a file

This command adds a character string prefix and/or suffix to each
line in a file.
Syntax:  add.sh BEGSTR ENDSTR < INFILE > OUTFILE
HELP
}

if [ $# -eq 1 ] && [ "$1" = "-h" ]; then usage; exit; fi
if [ $# -ne 2 ]; then usage; exit 1; fi

while read line; do echo $1${line}$2; done
