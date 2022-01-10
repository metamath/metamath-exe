#!/bin/bash
# Shell script equivalent of Metamath TYPE tool
# Mario Carneiro 7-Jan-2022

usage() {
  cat << HELP
TYPE (10 lines) - Display 10 lines of a file; similar to Unix \"head\"

This command displays (i.e. types out) the first N lines of a file on the
terminal screen.  If N is not specified, it will default to 10.  If N is
the string "ALL", then the whole file will be typed.
Syntax:  type.sh [N] < INFILE > OUTFILE
HELP
}

if [ $# -eq 1 ] && [ "$1" = "-h" -o "$1" = "--help" ]; then usage; exit; fi

if [ $# -eq 0 ]; then
  head -10
elif [ $# -eq 1 ]; then
  case "$1" in
    a* | A*)  cat; exit;;
    '' | *[!0-9]*)  usage; exit 1;;
    *)  head -$1;;
  esac
else
  usage; exit 1
fi
