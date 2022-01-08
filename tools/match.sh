#!/bin/bash
# Shell script equivalent of Metamath MATCH tool
# Mario Carneiro 7-Jan-2022

usage() {
  cat << HELP
MATCH - Extract lines containing (or not) a specified string

This command extracts from a file those lines containing (Y) or not
containing (N) a specified string.
Syntax:  match.sh MATCHSTR Y/N < INFILE > OUTFILE
HELP
}

if [ $# -eq 1 ] && [ "$1" = "-h" -o "$1" = "--help" ]; then usage; exit; fi
if [ $# -ne 2 ]; then usage; exit 1; fi

case "$2" in
  Y | y)  grep -F "$1";;
  N | n)  grep -v -F "$1";;
  *)  usage; exit 1;;
esac
