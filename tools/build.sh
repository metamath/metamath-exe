#!/bin/sh
# Shell script equivalent of Metamath BUILD tool
# Mario Carneiro 7-Jan-2022

usage() {
  cat << HELP
BUILD - Build a file with multiple tokens per line from a list

This command combines a list of tokens into multiple tokens per line,
as many as will fit per line, separating them with spaces.
The line length is hard-coded to 73 characters long.
Syntax:  build.sh < INFILE > OUTFILE
HELP
}

if [ $# -eq 1 ] && [ "$1" = "-h" -o "$1" = "--help" ]; then usage; exit; fi
if [ $# -ne 0 ]; then usage; exit 1; fi

awk '{
  if ($0 != "") {
    if (out == "")
      out = $0;
    else if (length(out) + length($0) > 72) {
      print out;
      out = $0;
    } else {
      out = out " " $0;
    }
  }
} END {
  if (out != "") print out;
}'
