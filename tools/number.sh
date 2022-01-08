#!/bin/bash
# Shell script equivalent of Metamath NUMBER tool
# Mario Carneiro 7-Jan-2022

usage() {
  cat << HELP
NUMBER - Create a list of numbers

This command creates a list of numbers.  Hint:  Use the RIGHT command to
right-justify the list after creating it.
Syntax:  number.sh FIRST LAST INCR > OUTFILE
HELP
}

if [ $# -eq 1 ] && [ "$1" = "-h" -o "$1" = "--help" ]; then usage; exit; fi
isNum() { [[ "$1" == ?(-)+([0-9]) ]]; }
if [ $# -ne 3 ] || ! isNum "$1" || ! isNum "$2" || ! isNum "$3"; then
  usage; exit 1
fi

x=$1
while [ $x -le $2 ]; do
  echo $x
  x=$(( $x + $3 ))
done
