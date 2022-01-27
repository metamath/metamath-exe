#!/bin/sh
# Shell script equivalent of Metamath INSERT tool
# Mario Carneiro 5-Jan-2022

usage() {
  cat << HELP
INSERT - Insert a string at a specified column in each line of a file

This command inserts a string at a specified column in each line
in a file.  It is intended to aid further processing of column-
sensitive files.  Note: the index of the first column is 1, not 0.  If a
line is shorter than COLUMN, then it is padded with spaces so that
STRING is still added at COLUMN.
Syntax:  insert.sh STRING COLUMN < INFILE > OUTFILE
HELP
}

if [ $# -eq 1 ] && [ "$1" = "-h" -o "$1" = "--help" ]; then usage; exit; fi
isNum() { case "$1" in '' | *[!0-9]*) return 1;; esac }
if [ $# -ne 2 ] || ! isNum "$2" || [ $2 -eq 0 ]; then usage; exit 1; fi

awk -v insertStr="$1" -v len=$2 '
BEGIN {
  gsub("%", "%%", insertStr);
  fmtStr = "%-" (len - 1) "s" insertStr "%s\n";
} {
  printf(fmtStr, substr($0, 1, len - 1), substr($0, len));
}'
