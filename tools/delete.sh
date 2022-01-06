#!/bin/sh
# Shell script equivalent of Metamath DELETE tool
# Mario Carneiro 5-Jan-2022

usage() {
  cat << HELP
DELETE - Delete a section of each line in a file

This command deletes the part of a line between (and including) the first
occurrence of STARTSTR and the first occurrence of ENDSTR (when both
exist) for all lines in a file.  If either string doesn't exist in a line,
the line will be unchanged.  If STARTSTR is blank (''), the deletion
will start from the beginning of the line.  If ENDSTR is blank, the
deletion will end at the end of the line.
Syntax:  delete.sh STARTSTR ENDSTR < INFILE > OUTFILE
HELP
}

if [ $# -eq 1 ] && [ "$1" = "-h" ]; then usage; exit; fi
if [ $# -ne 2 ]; then usage; exit 1; fi

awk -v startStr="$1" -v endStr="$2" '{
  start = index($0, startStr);
  if (start) {
    off = start + length(startStr);
    start2 = index(substr($0, off), endStr);
    if (start2) {
      end = off - 1 + start2 + length(endStr);
      $0 = substr($0, 1, start - 1) substr($0, end);
    }
  }
  print;
}'
