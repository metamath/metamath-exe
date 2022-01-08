#!/bin/bash
# Shell script equivalent of Metamath PARALLEL tool
# Mario Carneiro 7-Jan-2022

usage() {
  cat << HELP
RIGHT - Right-justify lines in a file (useful before sorting numbers)

This command right-justifies the lines in a file by putting spaces in
front of them so that they end in the same column as the longest line
in the file.
Syntax:  right.sh < INFILE > OUTFILE
HELP
}

if [ $# -eq 1 ] && [ "$1" = "-h" -o "$1" = "--help" ]; then usage; exit; fi
if [ $# -ne 0 ]; then usage; exit 1; fi

buf=$(cat)
len=$(echo "$buf" | awk '
  BEGIN { max = 0; }
  { if (length($0) > max) max = length($0); }
  END { print max }
')
echo "$buf" | awk -v len=$len '{ printf "%" len "s\n", $0 }'
