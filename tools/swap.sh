#!/bin/sh
# Shell script equivalent of Metamath SWAP tool
# Mario Carneiro 5-Jan-2022

usage() {
  cat << HELP
SWAP - Swap the two halves of each line in a file

This command swaps the parts of each line before and after a
specified string MIDDLE.
Syntax:  swap.sh MIDDLE < INFILE > OUTFILE
HELP
}

if [ $# -ne 1 ]; then usage; exit 1; fi

awk -v middle="$1" '{
  start = index($0, middle);
  if (start)
    $0 = substr($0, start + length(middle)) middle substr($0, 1, start - 1);
  print;
}'
