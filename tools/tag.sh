#!/bin/sh
# Shell script equivalent of Metamath TAG tool
# Mario Carneiro 5-Jan-2022

usage() {
  cat << HELP
TAG - Like ADD, but restricted to a range of lines

TAG is the same as ADD but has 4 additional arguments that let you
specify a range of lines.  Syntax:
  tag.sh BEGSTR ENDSTR STARTMATCH S# ENDMATCH E# < INFILE > OUTFILE
where
  INFILE = input file
  OUTFILE = output file
  BEGSTR = string to add to beginning of each line
  ENDSTR = string to add to end of each line
  STARTMATCH = a string to match; if empty, match any line
  S# = the 1st, 2nd, etc. occurrence of STARTMATCH to start the range
  ENDMATCH = a string to match; if empty, match any line
  E# = the 1st, 2nd, etc. occurrence of ENDMATCH from the
      start of range line (inclusive) after which to end the range
Example:  To add "!" to the end of lines 51 through 60 inclusive:
  tag.sh '' '!' '' 51 '' 10 < a.txt
Example:  To add "@@@" to the beginning of each line in theorem
"abc" through the end of its proof:
  tag.sh '@@@' '' 'abc $p' 1 '$.' 1 < set.mm
so that later, SUBSTITUTE can be used to affect only those lines.  You
can remove the "@@@" tags with SUBSTITUTE when done.
HELP
}

if [ $# -eq 1 ] && [ "$1" = "-h" -o "$1" = "--help" ]; then usage; exit; fi
isNum() { case "$1" in '' | *[!0-9]*) return 1;; esac; }
if [ $# -ne 6 ] || ! isNum "$4" || ! isNum "$6"; then usage; exit 1; fi

if [ $4 -eq 0 ]; then echo "error: S# must be positive\n"; usage; exit 1; fi
if [ $6 -eq 0 ]; then echo "error: E# must be positive\n"; usage; exit 1; fi

awk -v before="$1" -v after="$2" -v start="$3" -v end="$5" \
  -v startNum=$4 -v endNum=$6 '{
  if (startNum > 0) {
    if (index($0, start)) startNum--;
  }
  if (startNum == 0 && endNum > 0) {
    ended = index($0, end);
    $0 = before $0 after;
    if (ended) endNum--;
  }
  print;
}'
