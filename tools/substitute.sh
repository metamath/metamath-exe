#!/bin/sh
# Shell script equivalent of Metamath SUBSTITUTE tool
# Mario Carneiro 5-Jan-2022

usage() {
  cat << HELP
SUBSTITUTE - Make a simple substitution on each line of the file

This command performs a simple string substitution in each line of a file.
If the string to be replaced is "\n", then every other line will
be joined to the one below it.  If the replacement string is "\n", then
each line will be split into two if there is a match.
The MATCHSTR specifies a string that must also exist on a line
before the substitution takes place; null means match any line.
The OCCURRENCE is an integer (1 = first occurrence on each line, etc.)
or A for all occurrences on each line.
Syntax:  substitute.sh OLDSTR NEWSTR OCCURRENCE MATCHSTR < INFILE > OUTFILE
HELP
}

if [ $# -eq 1 ] && [ "$1" = "-h" ]; then usage; exit; fi
if [ $# -ne 4 ]; then usage; exit 1; fi
case "$3" in
  A | a)  all=1;;
  '' | '0' | *[!0-9]*)  usage; exit 1;;
  *)  occs=$3;;
esac

# multiline oldStr not supported
awk -v oldStr="$1" -v newStr="$2" -v matchStr="$4" \
  -v all="$all" -v occs=$occs \
'BEGIN {
  gsub(/[][\\.^$(){}|*+?]/, "\\\\&", oldStr);
  gsub(/[&\\]/, "\\\\&", newStr);
} {
  if (index($0, matchStr)) {
    if (all) gsub(oldStr, newStr);
    else gensub(oldStr, newStr, occs + 1);
  }
  print;
}'
