#!/bin/sh
# Shell script helper for in-place Metamath commands
# Mario Carneiro 5-Jan-2022

usage() {
  cat << HELP
IN_PLACE - Apply a command to a file, modifying it in-place
  Syntax:  inplace.sh FILE cmd args...
This is equivalent to
  cmd args... < FILE > FILE
except that it first buffers the entire file before calling the command.
This can be used on the other shell scripts in this folder, which are
designed for piping and generally do not work correctly when the same
file is chosen as input and output.
HELP
}

if [ $# -lt 2 ]; then usage; exit 1; fi

file=$1; shift
buf=$(cat $file)
echo "$buf" | "$@" > $file
