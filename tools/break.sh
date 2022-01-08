#!/bin/sh
# Shell script equivalent of Metamath BREAK tool
# Mario Carneiro 7-Jan-2022

usage() {
  cat << HELP
BREAK - Break up (parse) a file into a list of tokens (one per line)

This command breaks up a file into tokens, one per line, breaking at
whitespace and any special characters you specify as delimiters.
Use an explicit (quoted) space as SPECCHARS to avoid the default
special characters - ()[],=:;{} - and break only on whitespace.
Syntax:  break.sh SPECCHARS < INFILE > OUTFILE
HELP
}

if [ $# -ne 1 ]; then usage; exit 1; fi

if [ "$1" = "" ]; then specChars="()[],=:;{}"; else specChars=$1; fi

awk -v specChars="$specChars" \
'BEGIN {
  len = length(specChars);
  split(specChars, specCharsArr, "");
} {
  for (i = 1; i <= len; i++) {
    if (!index($0, specCharsArr[i])) continue;
    out = "";
    while (1) {
      p = index($0, specCharsArr[i]);
      if (!p) break;
      out = out substr($0, 1, p - 1) " " substr($0, p, 1) " ";
      $0 = substr($0, p + 1);
    }
    $0 = out $0;
  }
  gsub(/^[ \t]+/, "");
  gsub(/[ \t]+$/, "");
  gsub(/[ \t]+/, "\n");
  if ($0 != "") print;
}'
