#!/bin/bash
# Shell script equivalent of Metamath REVERSE tool
# Mario Carneiro 7-Jan-2022

usage() {
  cat << HELP
REVERSE - Reverse the order of the lines in a file

This command reverses the order of the lines in a file.
Syntax:  reverse.sh < INFILE > OUTFILE
HELP
}

if [ $# -eq 1 ] && [ "$1" = "-h" -o "$1" = "--help" ]; then usage; exit; fi
if [ $# -ne 0 ]; then usage; exit 1; fi

tac
